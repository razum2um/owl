/* Owl PDF viewer
 *
 * Copyright (c) 2009 Zoltan Kovacs, Peter Szilagyi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <utils.h>
#include <gui/events.h>
#include <config/config.h>
#include <mgmt/management.h>

#define MGMT_PORT_FILE "mgmt_port"

static int mgmt_sock = -1;
static int mgmt_port;
static GThread* mgmt_thread;

static GList* mgmt_clients = NULL;

int management_port_open_file( const char* path ) {
    char* tmp;
    size_t remaining;

    if ( mgmt_sock == -1 ) {
        return -1;
    }

    tmp = ( char* )path;
    remaining = strlen( tmp );

    while ( remaining > 0 ) {
        ssize_t done = send( mgmt_sock, tmp, remaining, 0 );

        if ( done < 0 ) {
            return -1;
        }

        tmp += done;
        remaining -= done;
    }

    send( mgmt_sock, "\n", 1, 0 );

    return 0;
}

static int load_management_info( int* port ) {
    int file;
    int size;
    char* data;
    char tmp[ 256 ];
    struct stat st;

    snprintf(
        tmp,
        sizeof( tmp ),
        "%s%s%s",
        get_config_directory(),
        PATH_SEPARATOR,
        MGMT_PORT_FILE
    );

    file = open( tmp, O_RDONLY );

    if ( file < 0 ) {
        return -1;
    }

    if ( fstat( file, &st ) != 0 ) {
        close( file );
        return -1;
    }

    if ( st.st_size > ( sizeof( tmp ) - 1 ) ) {
        printf( "Warning: management info file size is bigger than the internal buffer!\n" );
        close( file );
        return -1;
    }

    data = tmp;
    size = ( int )st.st_size;

    while ( size > 0 ) {
        int done = read( file, data, size );

        size -= done;
        data += done;
    }

    close( file );

    tmp[ st.st_size ] = 0;

    if ( sscanf( tmp, "%d", port ) != 1 ) {
        return -1;
    }

    return 0;
}

int management_port_open( void ) {
    int error;
    struct sockaddr_in conn_addr;

    error = load_management_info( &mgmt_port );

    if ( error < 0 ) {
        goto error1;
    }

    mgmt_sock = socket( AF_INET, SOCK_STREAM, 0 );

    if ( mgmt_sock < 0 ) {
        goto error1;
    }

    conn_addr.sin_family = AF_INET;
    inet_pton( AF_INET, "127.0.0.1", ( void* )&conn_addr.sin_addr );
    conn_addr.sin_port = htons( mgmt_port );

    error = connect( mgmt_sock, ( struct sockaddr* )&conn_addr, sizeof( struct sockaddr_in ) );

    if ( error < 0 ) {
        goto error2;
    }

    return 0;

 error2:
    close( mgmt_sock );
    mgmt_sock = -1;

 error1:
    return -1;
}

int management_port_close( void ) {
    if ( mgmt_sock != -1 ) {
        close( mgmt_sock );
        mgmt_sock = -1;
    }

    return 0;
}

static void store_management_info( void ) {
    int size;
    int file;
    char* data;
    char tmp[ 256 ];

    snprintf(
        tmp,
        sizeof( tmp ),
        "%s%s%s",
        get_config_directory(),
        PATH_SEPARATOR,
        MGMT_PORT_FILE
    );

    file = open( tmp, O_WRONLY | O_TRUNC | O_CREAT, 0600 );

    if ( file < 0 ) {
        return;
    }

    snprintf( tmp, sizeof( tmp ), "%d", mgmt_port );

    data = tmp;
    size = strlen( tmp );

    while ( size > 0 ) {
        int done = write( file, data, size );

        size -= done;
        data += done;
    }

    close( file );
}

int init_management_port( void ) {
    int i;
    int ok;
    int error;
    struct sockaddr_in bind_addr;

    mgmt_sock = socket( AF_INET, SOCK_STREAM, 0 );

    if ( mgmt_sock < 0 ) {
        goto error1;
    }

    bind_addr.sin_family = AF_INET;
    inet_pton( AF_INET, "127.0.0.1", ( void* )&bind_addr.sin_addr );

    /* Try to bind to 5 different ports */

    ok = 0;

    for ( i = 0; i < 5; i++ ) {
        mgmt_port = random() % 60000 + 1024;
        bind_addr.sin_port = htons( mgmt_port );

        error = bind( mgmt_sock, ( struct sockaddr* )&bind_addr, sizeof( struct sockaddr_in ) );

        if ( error == 0 ) {
            ok = 1;
            break;
        }
    }

    if ( !ok ) {
        goto error2;
    }

    error = listen( mgmt_sock, 10 );

    if ( error < 0 ) {
        goto error2;
    }

    store_management_info();

    return 0;

 error2:
    close( mgmt_sock );
    mgmt_sock = -1;

 error1:
    return -1;
}

typedef struct mgmt_client {
    int socket;
    size_t input_size;
    char* input_buffer;
} mgmt_client_t;

static void mgmt_handle_new_client( void ) {
    int socket;
    socklen_t inc_len;
    struct sockaddr inc_addr;

    mgmt_client_t* client;

    socket = accept( mgmt_sock, &inc_addr, &inc_len );

    if ( socket < 0 ) {
        return;
    }

    client = ( mgmt_client_t* )malloc( sizeof( mgmt_client_t ) );

    if ( client == NULL ) {
        close( socket );
        return;
    }

    client->socket = socket;
    client->input_size = 0;
    client->input_buffer = NULL;

    mgmt_clients = g_list_append( mgmt_clients, client );
}

static int mgmt_handle_client_data( mgmt_client_t* client ) {
    int result;
    char* tmp;
    char* start;
    size_t new_size;
    char buffer[ 512 ];

    result = recv( client->socket, buffer, sizeof( buffer ), MSG_NOSIGNAL );

    if ( result <= 0 ) {
        goto error;
    }

    new_size = client->input_size + result;

    client->input_buffer = ( char* )realloc( client->input_buffer, new_size + 1 );

    if ( client->input_buffer == NULL ) {
        goto error;
    }

    memcpy( client->input_buffer + client->input_size, buffer, result );
    client->input_size = new_size;
    client->input_buffer[ new_size ] = 0;

    start = client->input_buffer;

    while ( ( tmp = strchr( start, '\n' ) ) != NULL ) {
        *tmp++ = 0;

        do_open_document( start );

        start = tmp;
    }

    if ( start > client->input_buffer ) {
        size_t remaining = strlen( start );

        if ( remaining == 0 ) {
            free( client->input_buffer );
            client->input_buffer = NULL;
        } else {
            memmove( client->input_buffer, start, remaining );
            client->input_buffer[ remaining ] = 0;

            tmp = ( char* )realloc( client->input_buffer, remaining + 1 );

            if ( tmp == NULL ) {
                return -1;
            }

            client->input_buffer = tmp;
        }

        client->input_size = remaining;
    }

    return 0;

 error:
    close( client->socket );

    free( client->input_buffer );
    free( client );

    return -1;
}

static gpointer mgmt_thread_entry( gpointer data ) {
    int error;
    int max_fd;

    GList* item;
    fd_set read_set;

    while ( 1 ) {
        FD_ZERO( &read_set );

        max_fd = mgmt_sock;
        FD_SET( mgmt_sock, &read_set );

        /* Add management clients */

        item = g_list_first( mgmt_clients );

        while ( item != NULL ) {
            mgmt_client_t* client = item->data;

            max_fd = MAX( max_fd, client->socket );
            FD_SET( client->socket, &read_set );

            item = g_list_next( item );
        }

        error = select( max_fd + 1, &read_set, NULL, NULL, NULL );

        if ( error < 0 ) {
            fprintf( stderr, "Failed to select management clients!\n" );
            break;
        }

        if ( error == 0 ) {
            continue;
        }

        /* Check the listener socket */

        if ( FD_ISSET( mgmt_sock, &read_set ) ) {
            mgmt_handle_new_client();
        }

        item = g_list_first( mgmt_clients );

        while ( item != NULL ) {
            mgmt_client_t* client = item->data;

            if ( FD_ISSET( client->socket, &read_set ) ) {
                if ( mgmt_handle_client_data( client ) < 0 ) {
                    GList* next = g_list_next( item );
                    mgmt_clients = g_list_remove_link( mgmt_clients, item );
                    item = next;

                    continue;
                }
            }

            item = g_list_next( item );
        }
    }

    return NULL;
}

int start_management_port( void ) {
    mgmt_thread = g_thread_new(
	"mgmt",
        mgmt_thread_entry,
        NULL
    );

    return 0;
}
