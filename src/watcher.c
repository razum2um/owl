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

#include <watcher.h>

#if defined( _WIN32 ) || defined( __FreeBSD__ )

int watcher_add_document( document_t* document ) {
    return 0;
}

int watcher_remove_document( document_t* document ) {
    return 0;
}

int init_watcher_thread( void ) {
    return 0;
}

int stop_watcher_thread( void ) {
    return 0;
}

#else

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/glist.h>
#include <glib/gprintf.h>

static int inotify_fd = -1;
static volatile int running;
static pthread_t watcher_thread;

static GList* new_files = NULL;
static pthread_mutex_t new_file_lock;

static GList* removed_files = NULL;
static pthread_mutex_t removed_file_lock;

static GList* watched_files = NULL;

int watcher_add_document( document_t* document ) {
    watch_entry_t* entry;

    entry = ( watch_entry_t* )malloc( sizeof( watch_entry_t ) );

    if ( entry == NULL ) {
        return -ENOMEM;
    }

    entry->wd = -1;
    entry->doc = document;

    snprintf( entry->path, sizeof( entry->path ), "%s/%s", document->path, document->filename );

    pthread_mutex_lock( &new_file_lock );
    new_files = g_list_append( new_files, entry );
    pthread_mutex_unlock( &new_file_lock );

    pthread_kill( watcher_thread, SIGUSR1 );

    return 0;
}

int watcher_remove_document( document_t* document ) {
    pthread_mutex_lock( &removed_file_lock );
    removed_files = g_list_append( removed_files, document );
    pthread_mutex_unlock( &removed_file_lock );

    pthread_kill( watcher_thread, SIGUSR1 );

    return 0;
}

static void sigusr_handler( int signum ) {
}

static void handle_new_files( void ) {
    pthread_mutex_lock( &new_file_lock );

    while ( g_list_length( new_files ) > 0 ) {
        watch_entry_t* entry;

        entry = ( watch_entry_t* )g_list_nth_data( new_files, 0 );
        new_files = g_list_remove( new_files, entry );

        entry->wd = inotify_add_watch( inotify_fd, entry->path, IN_CLOSE_WRITE );

        if ( entry->wd < 0 ) {
            free( entry );

            continue;
        }

        watched_files = g_list_append( watched_files, entry );
    }

    pthread_mutex_unlock( &new_file_lock );
}

static void handle_removed_files( void ) {
    pthread_mutex_lock( &removed_file_lock );

    while ( g_list_length( removed_files ) > 0 ) {
        int i;
        int error;
        document_t* doc;

        int found = 0;
        watch_entry_t* entry;

        doc = ( document_t* )g_list_nth_data( removed_files, 0 );
        removed_files = g_list_remove( removed_files, doc );

        for ( i = 0; i < g_list_length( watched_files ); i++ ) {
            entry = g_list_nth_data( watched_files, i );

            if ( entry->doc == doc ) {
                found = 1;

                break;
            }
        }

        if ( !found ) {
            printf( "handle_removed_files(): Document %p not found!\n", doc );

            continue;
        }

        error = inotify_rm_watch( inotify_fd, entry->wd );

        if ( error < 0 ) {
            printf( "handle_removed_files(): Failed to remove inotify watch!\n" );
        }

        watched_files = g_list_remove( watched_files, entry );

        free( entry );
    }

    pthread_mutex_unlock( &removed_file_lock );
}

static watch_entry_t* get_watch_entry( int wd ) {
    int i;
    watch_entry_t* entry;

    i = 0;

    while ( 1 ) {
        entry = ( watch_entry_t* )g_list_nth_data( watched_files, i );

        if ( entry == NULL ) {
            break;
        }

        if ( entry->wd == wd ) {
            return entry;
        }

        i++;
    }

    return NULL;
}

static void watcher_document_changed( document_t* doc ) {
    document_do_reload( doc );
}

static void* watcher_thread_entry( void* arg ) {
    int count;
    char* buffer;
    fd_set read_set;

    buffer = ( char* )malloc( 8192 );

    if ( buffer == NULL ) {
        return NULL;
    }

    signal( SIGUSR1, sigusr_handler );

    while ( running ) {
        FD_ZERO( &read_set );
        FD_SET( inotify_fd, &read_set );

        count = select( inotify_fd + 1, &read_set, NULL, NULL, NULL );

        if ( !running ) {
            break;
        }

        if ( ( count == -1 ) &&
            ( errno != EINTR ) ) {
            printf( "watcher_thread_entry(): Select failed: %s\n", strerror( errno ) );

            break;
        }

        handle_new_files();
        handle_removed_files();

        if ( count > 0 ) {
            int pos;
            int data;

            data = read( inotify_fd, buffer, 8192 );

            if ( data < 0 ) {
                printf( "watcher_thread_entry(): Inotify read failed: %d\n", data );

                break;
            }

            pos = 0;

            while ( pos < data ) {
                watch_entry_t* entry;
                struct inotify_event* event;

                event = ( struct inotify_event* )&buffer[ pos ];

                entry = get_watch_entry( event->wd );

                if ( entry != NULL ) {
                    struct stat st;

                    if ( ( stat( entry->path, &st ) == 0 ) &&
                         ( st.st_size > 0 ) ) {
                        watcher_document_changed( entry->doc );
                    }
                }

                pos += sizeof( struct inotify_event ) + event->len;
            }
        }
    }

    free( buffer );

    return NULL;
}

int init_watcher_thread( void ) {
    int error;

    inotify_fd = inotify_init();

    if ( inotify_fd < 0 ) {
        error = inotify_fd;

        goto error1;
    }

    error = pthread_mutex_init( &new_file_lock, NULL );

    if ( error < 0 ) {
        goto error2;
    }

    error = pthread_mutex_init( &removed_file_lock, NULL );

    if ( error < 0 ) {
        goto error3;
    }

    running = 1;

    pthread_create(
        &watcher_thread,
        NULL,
        watcher_thread_entry,
        NULL
    );

    return 0;

 error3:
    pthread_mutex_destroy( &new_file_lock );

 error2:
    close( inotify_fd );

 error1:
    return error;
}

int stop_watcher_thread( void ) {
    running = 0;

    pthread_kill( watcher_thread, SIGUSR1 );
    pthread_join( watcher_thread, NULL );

    return 0;
}

#endif
