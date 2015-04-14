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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>

#include <storage.h>
#include <watcher.h>
#include <utils.h>
#include <main.h>
#include <gui/main_wnd.h>
#include <gui/menu.h>
#include <gui/my_stock.h>
#include <gui/print.h>
#include <gui/events.h>
#include <gui/pdf_view.h>
#include <config/settings.h>
#include <config/history.h>
#include <config/config.h>
#include <mgmt/management.h>

static void load_doc_helper( void* key, void* value, void* data ) {
    document_history_t* history;

    history = ( document_history_t* )value;

    if ( !history->recently_open ) {
        return;
    }

    do_open_document( history->path );
}

static void load_docs_from_last_time( void ) {
    int show_docs;

    if ( ( settings_get_int( "show-docs-from-last-time", &show_docs ) < 0 ) ||
         ( !show_docs ) ) {
        return;
    }

    history_iterate( load_doc_helper, NULL );
}

static void load_docs_from_cli( int argc, char** argv ) {
    int i;
    char real_path[ PATH_MAX ];

    for ( i = 1; i < argc; i++ ) {
#ifdef _WIN32
        /* TODO */
        continue;
#else
        if ( realpath( argv[ i ], real_path ) == NULL ) {
            continue;
        }
#endif /* _WIN32 */

        do_open_document( real_path );
    }
}

static void load_documents( int argc, char** argv ) {

    load_docs_from_last_time();
    load_docs_from_cli( argc, argv );
}

static int exit_application_helper( document_t* document, pdf_tab_t* pdf_tab, void* data ) {
    char path[ 256 ];
    int current_page;
    document_history_t* history;

    snprintf( path, sizeof( path ), "%s%s%s", document->path, PATH_SEPARATOR, document->filename );

    /* Stop the render engine of the document */

    render_engine_stop( document->render_engine, FALSE );

    /* Update document history */

    current_page = gtk_pdfview_get_current_page( pdf_tab->pdf_view ) + 1;
    document_update_history( document, current_page );

    history = history_get_document_info( path, TRUE );

    if ( history != NULL ) {
        if ( pdf_tab->paned == NULL ) {
            history->paned_position = -1;
        } else {
            history->paned_position = gtk_paned_get_position( GTK_PANED( pdf_tab->paned ) );
        }
    }

    return 0;
}

void exit_application( void ) {
    char path[ 256 ];

    /* Stop the file watcher thread */

    stop_watcher_thread();

    /* Stop the rendering engines */

    snprintf( path, sizeof( path ), "%s%sopen-docs", get_config_directory(), PATH_SEPARATOR );

    document_storage_foreach( exit_application_helper, NULL );

    /* Quit from the GTK mainloop */

    gtk_main_quit();
}

static int destroy_document_helper( document_t* document, pdf_tab_t* pdf_tab, void* data ) {
    document_destroy( document );

    return 0;
}

void destroy_documents( void ) {
    document_storage_foreach( destroy_document_helper, NULL );
}

static int handle_args_with_management_port( int argc, char** argv ) {
    int i;
    int error;
    int open_in_running;

    if ( argc < 2 ) {
        return -1;
    }

    if ( ( settings_get_int( "open-in-running-instance", &open_in_running ) != 0 ) ||
         ( open_in_running == 0 ) ) {
        return -1;
    }

    if ( strcmp( argv[ 1 ], "-nw" ) == 0 ) {
        return -1;
    }

    error = management_port_open();

    if ( error < 0 ) {
        return -1;
    }

    for ( i = 1; i < argc; i++ ) {
        char real_path[ PATH_MAX ];

        if ( realpath( argv[ i ], real_path ) != NULL ) {
            management_port_open_file( real_path );
        }
    }

    management_port_close();

    return 0;
}

static void print_usage( void ) {
    printf( "Owl PDF reader v" OWL_VERSION "\n" );
    printf( "Developers: Peter Szilagyi, Zoltan Kovacs\n\n" );
    printf( "Usage:\n" );
    printf( "  owl [-nw] [doc1] ...\n\n" );
    printf( "Parameters:\n" );
    printf( "  -nw - Open the new document(s) in a new Owl instance\n" );
}

int main( int argc, char** argv ) {
    if ( ( argc > 1 ) &&
         ( ( strcmp( argv[ 1 ], "-h" ) == 0 ) ||
           ( strcmp( argv[ 1 ], "--help" ) == 0 ) ) ) {
        print_usage();
        return EXIT_SUCCESS;
    }

    srandom( time( NULL ) );

    init_config();
    init_settings();

    if ( handle_args_with_management_port( argc, argv ) == 0 ) {
        return EXIT_SUCCESS;
    }

    init_management_port();

    /* Initialize G* stuffs */

    g_thread_init( NULL );
    gdk_threads_init();
    gtk_init( &argc, &argv );

    /* Initialize owl components */

    init_history();
    init_print();
    init_watcher_thread();
    init_document_storage();
    init_stock_icons();
    init_main_window();
    init_popup_menu();
    load_document_history();

    init_poppler_lock();

    /* Load documents from command line */

    load_documents( argc, argv );
    start_management_port();

    /* Start the GTK mainloop */

    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();

    save_document_history();
    save_settings();

    destroy_documents();
    destroy_poppler_lock();
    management_port_close();

    return EXIT_SUCCESS;
}
