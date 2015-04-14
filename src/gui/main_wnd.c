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

#include <gtk/gtk.h>

#include <utils.h>
#include <config/settings.h>
#include <gui/main_wnd.h>
#include <gui/pdf_view.h>
#include <gui/events.h>
#include <engine/document.h>

GtkWidget* main_window;
GtkWidget* main_notebook;
GtkWidget* toolbar;
GtkWidget* entry_zoom;
GtkWidget* button_rotate_left;
GtkWidget* button_rotate_right;

static GtkWidget* fullscreen_window;
static GtkWidget* menu_bar;
static GtkWidget* load_progress_bar;

static void set_app_icons( void ) {
    int sizes[] = { 16, 32, 48, 64, 128 };
    GList* icons = NULL;
    GdkPixbuf* pixbuf = NULL;
    char path[ 256 ];
    int i;

    for ( i = 0; i < sizeof( sizes ) / sizeof( int ); i++ ) {
        snprintf(
            path,
            sizeof( path ),
            "%s%spixmaps%sicon-%d.png",
            OWL_INSTALL_PATH,
            PATH_SEPARATOR,
            PATH_SEPARATOR,
            sizes[ i ]
        );

        if ( ( pixbuf = gdk_pixbuf_new_from_file( path, NULL ) ) != NULL ) {
            icons = g_list_append( icons, gdk_pixbuf_new_from_file( path, NULL ) );
        }
    }

    if ( icons != NULL ) {
        gtk_window_set_default_icon_list( icons );
        g_list_free( icons );
    }
}

int init_main_window( void ) {
    GtkWidget* vbox;

    main_window = gtk_window_new( GTK_WINDOW_TOPLEVEL );

    main_window_set_title( NULL );

    set_app_icons();

    g_signal_connect(
        G_OBJECT( main_window ),
        "destroy",
        G_CALLBACK( event_window_destroy ),
        NULL
    );

    vbox = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER( main_window ), vbox );

    /* Menubar */

    menu_bar = create_menu_bar();
    gtk_box_pack_start( GTK_BOX( vbox ), menu_bar, FALSE, FALSE, 2 );

    /* Toolbar */

    toolbar = create_tool_bar();
    gtk_box_pack_start( GTK_BOX( vbox ), toolbar, FALSE, FALSE, 2 );

    /* Notebook for the document pages */

    main_notebook = gtk_notebook_new();

    gtk_notebook_set_scrollable( GTK_NOTEBOOK( main_notebook ), TRUE );
    gtk_notebook_set_show_tabs( GTK_NOTEBOOK( main_notebook ), FALSE );
    gtk_notebook_set_show_border( GTK_NOTEBOOK( main_notebook ), FALSE );
    gtk_notebook_popup_disable( GTK_NOTEBOOK( main_notebook ) );

    gtk_container_set_border_width( GTK_CONTAINER( main_notebook ), 0 );

    gtk_box_pack_start( GTK_BOX( vbox ), main_notebook, TRUE, TRUE, 2 );

    g_signal_connect_after(
        G_OBJECT( main_notebook ),
        "switch-page",
        G_CALLBACK( event_current_doc_changed ),
        NULL
    );

    g_signal_connect_after(
        G_OBJECT( main_notebook ),
        "page-removed",
        G_CALLBACK( event_doc_removed ),
        NULL
    );

    /* Progress bar for document loading */

    load_progress_bar = gtk_progress_bar_new();

    gtk_box_pack_start( GTK_BOX( vbox ), load_progress_bar, FALSE, FALSE, 2 );

    /* Show window */

    gtk_window_maximize( GTK_WINDOW( main_window ) );

    gtk_widget_show_all( main_window );

    toolbar_update_style();

    /* Hide unnecessary widgets */

    int show_toolbar;

    if ( ( settings_get_int( "show-toolbar", &show_toolbar ) == 0 ) && !show_toolbar ) {
        gtk_widget_hide( toolbar );
    }

    gtk_widget_hide( load_progress_bar );

    return 0;
}

void main_window_set_title( const char* title ) {
    if ( title != NULL ) {
        char buf[ 256 ];
        snprintf( buf, sizeof( buf ), "%s - Owl", title );
        gtk_window_set_title( GTK_WINDOW( main_window ), buf );
    } else {
        gtk_window_set_title( GTK_WINDOW( main_window ), "Owl" );
    }
}

void main_window_next_tab( void ) {
    gtk_notebook_next_page( GTK_NOTEBOOK( main_notebook ) );
}

void main_window_prev_tab( void ) {
    gtk_notebook_prev_page( GTK_NOTEBOOK( main_notebook ) );
}

void main_window_set_fullscreen_mode( gboolean enabled ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    if ( enabled != gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( view_fullscreen_mode ) ) ) {
        gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM( view_fullscreen_mode ), enabled );
        return;
    }

    if ( enabled ) {
        fullscreen_window = gtk_window_new( GTK_WINDOW_TOPLEVEL );

        gtk_widget_ref( pdf_tab->pdf_view );
        gtk_container_remove( GTK_CONTAINER( pdf_tab->viewport ), pdf_tab->pdf_view );
        gtk_container_add( GTK_CONTAINER( fullscreen_window ), pdf_tab->pdf_view );
        gtk_widget_unref( pdf_tab->pdf_view );

        gtk_pdfview_set_fullscreen_mode( pdf_tab->pdf_view, TRUE );

        gtk_widget_hide( main_window );
        gtk_window_fullscreen( GTK_WINDOW( fullscreen_window ) );
        gtk_widget_show_all( fullscreen_window );
    } else {
        gtk_widget_ref( pdf_tab->pdf_view );
        gtk_container_remove( GTK_CONTAINER( fullscreen_window ), pdf_tab->pdf_view );
        gtk_container_add( GTK_CONTAINER( pdf_tab->viewport ), pdf_tab->pdf_view );
        gtk_widget_unref( pdf_tab->pdf_view );

        gtk_pdfview_set_fullscreen_mode( pdf_tab->pdf_view, FALSE );

        gtk_widget_destroy( fullscreen_window );
        gtk_widget_show( main_window );
    }
}

static int load_progress_count = 0;
static int load_progress_timeout;

static gboolean main_window_update_open_progress( gpointer data ) {
    gtk_progress_bar_pulse( GTK_PROGRESS_BAR( load_progress_bar ) );

    return TRUE;
}

static void main_window_set_load_progress_text( void ) {
    char buf[ 64 ];

    if ( load_progress_count == 1 ) {
        snprintf( buf, sizeof( buf ), "Loading document..." );
    } else {
        snprintf( buf, sizeof( buf ), "Loading %d documents...", load_progress_count );
    }

    gtk_progress_bar_set_text( GTK_PROGRESS_BAR( load_progress_bar ), buf );
}

int main_window_attach_load_progress( void ) {
    if ( ++load_progress_count == 1 ) {
        load_progress_timeout = g_timeout_add( 150, main_window_update_open_progress, NULL );
        gtk_widget_show( load_progress_bar );
    }

    main_window_set_load_progress_text();

    return load_progress_count;
}

void main_window_detach_load_progress( int reference ) {
    if ( --load_progress_count == 0 ) {
        gtk_widget_hide( load_progress_bar );
        g_source_remove( load_progress_timeout );
    } else {
        main_window_set_load_progress_text();
    }
}
