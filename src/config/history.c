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

#include <config/history.h>

#ifdef _WIN32

document_history_t* history_get_document_info( const char* path, int create ) {
    return NULL;
}

int history_add_page_info( document_history_t* history, int page_index, int rotate, int scale ) {
    return 0;
}

int load_document_history( void ) {
    return 0;
}

int save_document_history( void ) {
    return 0;
}

int init_history( void ) {
    return 0;
}

#else

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <expat.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <utils.h>
#include <config/config.h>

#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

static GHashTable* history_table;

static char* element_data;
static int element_data_size;
static int element_rotate;
static int element_scale;
static int element_page_index;
static document_history_t* history_item;

static void XMLCALL start_element( void* data, const char* name, const char** atts ) {
    element_data = NULL;
    element_data_size = 0;

    if ( strcmp( name, "document" ) == 0 ) {
        history_item = ( document_history_t* )malloc( sizeof( document_history_t ) );

        memset( history_item, 0, sizeof( document_history_t ) );

        history_item->paned_position = -1;
        history_item->page_table = g_hash_table_new( g_direct_hash, g_direct_equal );
    } else if ( strcmp( name, "page" ) == 0 ) {
        int i;

        for ( i = 0; atts[ i ] != NULL; i += 2 ) {
            if ( strcmp( atts[ i ], "index" ) == 0 ) {
                sscanf( atts[ i + 1 ], "%d", &element_page_index );

                element_page_index--;
            }
        }
    }
}

static void XMLCALL end_element( void* data, const char* name ) {
    if ( strcmp( name, "document" ) == 0 ) {
        g_hash_table_insert(
            history_table,
            history_item->path,
            history_item
        );
    } else if ( strcmp( name, "path" ) == 0 ) {
        history_item->path = strdup( element_data );
    } else if ( strcmp( name, "v-offset" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->v_offset );
    } else if ( strcmp( name, "h-offset" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->h_offset );
    } else if ( strcmp( name, "current-page" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->current_page );
    } else if ( strcmp( name, "zoom-type" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->zoom_type );
    } else if ( strcmp( name, "display-type" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->display_type );
    } else if ( strcmp( name, "paned-position" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->paned_position );
    } else if ( strcmp( name, "recently-open" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->recently_open );
    } else if ( strcmp( name, "rotate-global" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->rotate );
    } else if ( strcmp( name, "scale-global" ) == 0 ) {
        sscanf( element_data, "%d", &history_item->scale );
    } else if ( strcmp( name, "rotate" ) == 0 ) {
        sscanf( element_data, "%d", &element_rotate );
    } else if ( strcmp( name, "scale" ) == 0 ) {
        sscanf( element_data, "%d", &element_scale );
    } else if ( strcmp( name, "page" ) == 0 ) {
        page_history_t* page_hist;

        page_hist = ( page_history_t* )malloc( sizeof( page_history_t ) );

        if ( page_hist != NULL ) {
            page_hist->rotate = element_rotate;
            page_hist->scale = element_scale;

            g_hash_table_insert(
                history_item->page_table,
                GINT_TO_POINTER( element_page_index ),
                page_hist
            );
        }
    }

    if ( element_data != NULL ) {
        free( element_data );
        element_data = NULL;
    }
}

static void XMLCALL element_data_handler( void* _data, const char* data, int len ) {
    int new_size;

    new_size = element_data_size + len;

    element_data = realloc( element_data, new_size + 1 );

    /* TODO: check! */

    memcpy( element_data + element_data_size, data, len );
    element_data_size += len;

    element_data[ element_data_size ] = 0;
}

int load_document_history( void ) {
    int fd;
    int done;
    char buffer[ 512 ];
    XML_Parser parser;

    snprintf( buffer, sizeof( buffer ), "%s%shistory.xml", get_config_directory(), PATH_SEPARATOR );

    fd = open( buffer, O_RDONLY );

    if ( fd < 0 ) {
        goto error1;
    }

    parser = XML_ParserCreate( NULL );

    if ( parser == NULL ) {
        goto error2;
    }

    XML_SetElementHandler( parser, start_element, end_element);
    XML_SetDefaultHandler( parser, element_data_handler );

    do {
        int length;

        length = read( fd, buffer, sizeof( buffer ) );

        done = length < sizeof( buffer );

        if ( XML_Parse( parser, buffer, length, done ) == XML_STATUS_ERROR ) {
            fprintf(
                stderr,
                "%s at line %" XML_FMT_INT_MOD "u\n",
                XML_ErrorString( XML_GetErrorCode( parser ) ),
                XML_GetCurrentLineNumber( parser )
            );

            break;
        }
    } while ( !done );

    XML_ParserFree( parser );

    close( fd );

    return 0;

 error2:
    close( fd );

 error1:
    return -1;
}

#define WRITE_XML(level, format, arg...)                                             \
    {                                                                                \
        int _i;                                                                      \
        for ( _i = 0; _i < level * 2; _i++ ) buffer[_i] = ' ';                       \
        length = snprintf( buffer + _i, sizeof( buffer ) - _i, format "\n", ##arg ); \
        length = write( fd, buffer, _i + length );                                   \
    }

static void save_document_page_iterator( gpointer key, gpointer value, gpointer data ) {
    int fd;
    int length;
    char buffer[ 512 ];
    page_history_t* history;

    fd = GPOINTER_TO_INT( data );
    history = ( page_history_t* )value;

    WRITE_XML( 3, "<page index=\"%d\">", GPOINTER_TO_INT( key ) + 1 );
    WRITE_XML( 4, "<rotate>%d</rotate>", history->rotate );
    WRITE_XML( 4, "<scale>%d</scale>", history->scale );
    WRITE_XML( 3, "</page>" );
}

static void save_document_history_iterator( gpointer key, gpointer value, gpointer data ) {
    int fd;
    int length;
    char buffer[ 512 ];
    document_history_t* history;

    fd = GPOINTER_TO_INT( data );
    history = ( document_history_t* )value;

    WRITE_XML( 1, "<document>" );
    WRITE_XML( 2, "<path>%s</path>", history->path );
    WRITE_XML( 2, "<v-offset>%d</v-offset>", history->v_offset );
    WRITE_XML( 2, "<h-offset>%d</h-offset>", history->h_offset );
    WRITE_XML( 2, "<current-page>%d</current-page>", history->current_page );
    WRITE_XML( 2, "<zoom-type>%d</zoom-type>", history->zoom_type );
    WRITE_XML( 2, "<display-type>%d</display-type>", history->display_type );

    if ( history->paned_position != -1 ) {
        WRITE_XML( 2, "<paned-position>%d</paned-position>", history->paned_position );
    }

    WRITE_XML( 2, "<recently-open>%d</recently-open>", history->recently_open );
    WRITE_XML( 2, "<rotate-global>%d</rotate-global>", history->rotate );
    WRITE_XML( 2, "<scale-global>%d</scale-global>", history->scale );
    WRITE_XML( 2, "<pages>" );

    g_hash_table_foreach( history->page_table, save_document_page_iterator, data );

    WRITE_XML( 2, "</pages>" );
    WRITE_XML( 1, "</document>" );
}

int save_document_history( void ) {
    int fd;
    int length;
    char buffer[ 512 ];

    snprintf( buffer, sizeof( buffer ), "%s%shistory.xml", get_config_directory(), PATH_SEPARATOR );

    fd = open( buffer, O_WRONLY | O_TRUNC | O_CREAT, 0600 );

    if ( fd < 0 ) {
        return fd;
    }

    WRITE_XML( 0, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" );
    WRITE_XML( 0, "<history>" );

    g_hash_table_foreach( history_table, save_document_history_iterator, GINT_TO_POINTER( fd ) );

    WRITE_XML( 0, "</history>" );

    close( fd );

    return 0;
}

#undef WRITE_XML

document_history_t* history_get_document_info( const char* path, int create ) {
    document_history_t* history;

    history = g_hash_table_lookup( history_table, path );

    if ( history != NULL ) {
        return history;
    }

    if ( !create ) {
        return NULL;
    }

    history = ( document_history_t* )malloc( sizeof( document_history_t ) );

    if ( history == NULL ) {
        goto error1;
    }

    memset( history, 0, sizeof( document_history_t ) );

    history->path = strdup( path );

    if ( history->path == NULL ) {
        goto error2;
    }

    history->page_table = g_hash_table_new( g_direct_hash, g_direct_equal );

    if ( history->page_table == NULL ) {
        goto error3;
    }

    history->v_offset = 0;
    history->h_offset = 0;
    history->zoom_type = 0;
    history->rotate = 0;
    history->scale = 100;

    g_hash_table_insert( history_table, history->path, history );

    return history;

 error3:
    free( history->path );

 error2:
    free( history );

 error1:
    return NULL;
}

int history_add_page_info( document_history_t* history, int page_index, int rotate, int scale ) {
    page_history_t* page_hist;

    page_hist = ( page_history_t* )malloc( sizeof( page_history_t ) );

    if ( page_hist == NULL ) {
        return -ENOMEM;
    }

    page_hist->scale = scale;
    page_hist->rotate = rotate;

    g_hash_table_insert( history->page_table, GINT_TO_POINTER( page_index ), page_hist );

    return 0;
}

int history_iterate( history_callback_t* callback, void* data ) {
    g_hash_table_foreach( history_table, callback, data );

    return 0;
}

int init_history( void ) {
    history_table = g_hash_table_new( g_str_hash, g_str_equal );

    if ( history_table == NULL ) {
        return -ENOMEM;
    }

    return 0;
}

#endif /* _WIN32 */
