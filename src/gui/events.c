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

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <utils.h>
#include <storage.h>
#include <watcher.h>
#include <main.h>
#include <gui/events.h>
#include <gui/doc_tab.h>
#include <gui/main_wnd.h>
#include <gui/pdf_view.h>
#include <gui/gui-utils.h>
#include <engine/document.h>
#include <config/history.h>
#include <config/settings.h>

extern pdf_tab_t* popup_pdf_tab;

void event_close_tab( GtkButton* button, gpointer data ) {
    int page_index;
    pdf_tab_t* pdf_tab;
    document_t* document;

    pdf_tab = ( pdf_tab_t* )data;

    document = document_storage_get_doc_by_widget( pdf_tab->root );
    g_return_if_fail( document != NULL );

    page_index = gtk_notebook_page_num( GTK_NOTEBOOK( main_notebook ), pdf_tab->root );
    g_return_if_fail( page_index != -1 );

    do_close_document_tab( pdf_tab, document, page_index );
}

void event_close_this_tab( GtkButton* button, gpointer data ) {
    int page_index;
    document_t* document;

    document = document_storage_get_doc_by_widget( popup_pdf_tab->root );
    g_return_if_fail( document != NULL );

    page_index = gtk_notebook_page_num( GTK_NOTEBOOK( main_notebook ), popup_pdf_tab->root );
    g_return_if_fail( page_index != -1 );

    do_close_document_tab( popup_pdf_tab, document, page_index );
}

void event_close_other_tabs( GtkButton* button, gpointer data ) {
    int i;

    for ( i = gtk_notebook_get_n_pages( GTK_NOTEBOOK( main_notebook ) ) - 1; i >= 0; i-- ) {
        GtkWidget* child;
        pdf_tab_t* pdf_tab;
        document_t* document;

        child = gtk_notebook_get_nth_page( GTK_NOTEBOOK( main_notebook ), i );

        pdf_tab = document_storage_get_tab_by_widget( child );
        g_return_if_fail( pdf_tab != NULL );

        if ( pdf_tab == popup_pdf_tab ) {
            continue;
        }

        document = document_storage_get_doc_by_widget( child );
        g_return_if_fail( document != NULL );

        do_close_document_tab( pdf_tab, document, i );
    }
}

void event_goto_prev_page( GtkButton* button, gpointer data ) {
    GtkWidget* widget;

    widget = ( GtkWidget* )data;

    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    gtk_pdfview_goto_page( widget, -1, 0.0, TRUE, FALSE );
}

void event_goto_next_page( GtkButton* button, gpointer data ) {
    GtkWidget* widget;

    widget = ( GtkWidget* )data;

    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    gtk_pdfview_goto_page( widget, 1, 0.0, TRUE, FALSE );
}

void event_goto_prev_page_toolbar( GtkToolButton* button, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    gtk_pdfview_goto_page( pdf_tab->pdf_view, -1, 0.0, TRUE, FALSE );
}

void event_goto_next_page_toolbar( GtkToolButton* button, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    gtk_pdfview_goto_page( pdf_tab->pdf_view, 1, 0.0, TRUE, FALSE );
}

gboolean event_zoom_in( GtkWidget* widget, GdkEventButton* event, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return FALSE;
    }

    if ( event->state & GDK_CONTROL_MASK ) {
        doctab_set_statusbar( pdf_tab, 0, "Pick a page to zoom in or ESC to exit" );

        gtk_pdfview_pick_page( pdf_tab->pdf_view, do_zoom_page_in_cb, do_pick_page_canceled_cb );
    } else {
        do_zoom( pdf_tab, document, -1, 10 );
    }

    return FALSE;
}

gboolean event_zoom_out( GtkWidget* widget, GdkEventButton* event, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return FALSE;
    }

    if ( event->state & GDK_CONTROL_MASK ) {
        doctab_set_statusbar( pdf_tab, 0, "Pick a page to zoom out or ESC to exit" );

        gtk_pdfview_pick_page( pdf_tab->pdf_view, do_zoom_page_out_cb, do_pick_page_canceled_cb );
    } else {
        do_zoom( pdf_tab, document, -1, -10 );
    }

    return FALSE;
}

gboolean event_set_zoom( GtkWidget* widget, GdkEventKey* event, gpointer data ) {
    int error;
    int percent;
    const gchar* text;
    pdf_tab_t* pdf_tab;
    document_t* document;

    if ( ( event->keyval != GDK_Return ) &&
         ( event->keyval != GDK_KP_Enter ) ) {
        return FALSE;
    }

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return FALSE;
    }

    text = gtk_entry_get_text( GTK_ENTRY( widget ) );

    if ( sscanf( text, "%d", &percent ) != 1 ) {
        return FALSE;
    }

    FORCE_MIN_MAX( percent, MIN_ZOOM, MAX_ZOOM );

    document_set_scale( document, percent );

    gtk_pdfview_set_zoom_type( pdf_tab->pdf_view, ZOOM_FREE );

    gtk_pdfview_invalidate_selection( pdf_tab->pdf_view );

    doctab_update_adjustment_limits( pdf_tab );
    do_update_zoom_entry( pdf_tab, document );
    do_update_render_region( pdf_tab, document, FALSE );

    return FALSE;
}

gboolean event_rotate_left( GtkWidget* widget, GdkEventButton* event, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return FALSE;
    }

    if ( event->state & GDK_CONTROL_MASK ) {
        doctab_set_statusbar( pdf_tab, 0, "Pick a page to rotate left or ESC to exit" );

        gtk_pdfview_pick_page( pdf_tab->pdf_view, do_rotate_page_left_cb, do_pick_page_canceled_cb );
    } else {
        do_rotate( pdf_tab, document, -1, -90 );
    }

    return FALSE;
}

gboolean event_rotate_right( GtkWidget* widget, GdkEventButton* event, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return FALSE;
    }

    if ( event->state & GDK_CONTROL_MASK ) {
        doctab_set_statusbar( pdf_tab, 0, "Pick a page to rotate right or ESC to exit" );

        gtk_pdfview_pick_page( pdf_tab->pdf_view, do_rotate_page_right_cb, do_pick_page_canceled_cb );
    } else {
        do_rotate( pdf_tab, document, -1, 90 );
    }

    return FALSE;
}

void event_selection_mode_toggled( GtkToggleButton* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;
    gboolean selection_mode;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        gtk_toggle_button_set_active( widget, FALSE );

        return;
    }

    selection_mode = gtk_toggle_button_get_active( widget );

    gtk_pdfview_set_selection_mode(
        pdf_tab->pdf_view,
        selection_mode
    );

    do_update_render_region( pdf_tab, document, FALSE );

    /* Update the status bar */

    if ( selection_mode ) {
        doctab_set_statusbar( pdf_tab, 0, "%s", "Selection activated (right-click selected area for action)" );
    } else {
        doctab_set_statusbar( pdf_tab, 3000, "%s", "Selection deactivated" );
    }
}

void event_scroll_v_adjustment_changed( GtkAdjustment* adjustment, gpointer data ) {
    int value;
    pdf_tab_t* pdf_tab;
    document_t* document;

    pdf_tab = ( pdf_tab_t* )data;

    document = document_storage_get_doc_by_widget( pdf_tab->root );

    g_return_if_fail( document != NULL );

    value = ( int )gtk_adjustment_get_value( adjustment );

    gtk_pdfview_set_v_offset( pdf_tab->pdf_view, value );
}

void event_scroll_h_adjustment_changed( GtkAdjustment* adjustment, gpointer data ) {
    int value;
    pdf_tab_t* pdf_tab;
    document_t* document;

    pdf_tab = ( pdf_tab_t* )data;

    document = document_storage_get_doc_by_widget( pdf_tab->root );

    g_return_if_fail( document != NULL );

    value = ( int )gtk_adjustment_get_value( adjustment );

    gtk_pdfview_set_h_offset( pdf_tab->pdf_view, value );
}

gboolean event_current_doc_changed( GtkNotebook* notebook, gint arg1, gpointer data ) {
    int error;
    char tmp[ 16 ];
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return FALSE;
    }

    snprintf( tmp, sizeof( tmp ), "%d", document_get_page_count( document ) );
    gtk_entry_set_text( GTK_ENTRY( pdf_tab->entry_last_pg ), tmp );

    main_window_set_title( document->filename );

    do_update_current_page( pdf_tab );
    do_update_zoom_entry( pdf_tab, document );
    do_update_page_specific_menu_items( pdf_tab );

    return FALSE;
}

void event_doc_removed( GtkNotebook* notebook, GtkWidget* child, guint page_num, gpointer data ) {
    if ( gtk_notebook_get_n_pages( notebook ) == 0 ) {
        if ( GTK_IS_TOGGLE_BUTTON( toolbar_selection->button ) ) {
            /* If this function is called because the notebook page is
               removed automatically by gtk after a main window
               destroy event, there is no button_selection widget;
               hence the type check. */

            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( toolbar_selection->button ), FALSE );
        }

        main_window_set_title( NULL );
    }
}

void event_fileexit_activated( void ) {
    exit_application();
}

void event_window_destroy( GtkWidget* widget, gpointer data ) {
    exit_application();
}

typedef struct open_progress {
    int reference;
} open_progress_t;

static void event_document_open_started( document_t* doc ) {
    open_progress_t* progress;

    progress = ( open_progress_t* )malloc( sizeof( open_progress_t ) );

    if ( progress == NULL ) {
        return;
    }

    GDK_THREADS_ENTER();

    progress->reference = main_window_attach_load_progress();

    GDK_THREADS_LEAVE();

    doc->private = ( void* )progress;
}

static void event_document_open_ok( document_t* doc ) {
    char path[ 256 ];
    int show_tabs = 0;
    pdf_tab_t* pdf_tab;
    GtkWidget* tab_label;
    document_history_t* history;
    open_progress_t* progress;

    progress = ( open_progress_t* )doc->private;
    doc->private = NULL;

    snprintf( path, sizeof( path ), "%s%s%s", doc->path, PATH_SEPARATOR, doc->filename );

    history = history_get_document_info( path, FALSE );

    pdf_tab = create_pdf_tab( doc );
    tab_label = create_doc_tab_label( pdf_tab, doc );
    pdf_tab->event_box = tab_label;

    GDK_THREADS_ENTER();

    main_window_detach_load_progress( progress->reference );
    free( progress );

    /* Put the new document to the storage */

    document_storage_add_new( pdf_tab, doc );

    /* Make the new document visible on the GUI :) */

    gtk_notebook_append_page_menu(
        GTK_NOTEBOOK( main_notebook ),
        pdf_tab->root,
        tab_label,
        gtk_label_new( doc->filename )
    );

    gtk_notebook_set_tab_reorderable( GTK_NOTEBOOK( main_notebook ), pdf_tab->root, TRUE );

    gtk_notebook_set_current_page(
        GTK_NOTEBOOK( main_notebook ),
        gtk_notebook_get_n_pages( GTK_NOTEBOOK( main_notebook ) ) - 1
    );

    settings_get_int( "always-show-tabs", &show_tabs );

    if ( ( show_tabs ) || ( gtk_notebook_get_n_pages( GTK_NOTEBOOK( main_notebook ) ) == 2 ) ) {
        gtk_notebook_set_show_tabs( GTK_NOTEBOOK( main_notebook ), TRUE );
    }

    gtk_widget_grab_focus( pdf_tab->pdf_view );

    if ( history != NULL ) {
        history->recently_open = 1;

        if ( ( history->paned_position != -1 ) &&
             ( pdf_tab->paned != NULL ) ) {
            gtk_paned_set_position( GTK_PANED( pdf_tab->paned ), history->paned_position );
        }

        gtk_pdfview_set_zoom_type( pdf_tab->pdf_view, history->zoom_type );
        gtk_pdfview_set_display_type( pdf_tab->pdf_view, history->display_type );

        if ( history->display_type == SINGLE_PAGE ) {
            gtk_pdfview_goto_page( pdf_tab->pdf_view, history->current_page, 0.0, FALSE, FALSE );
        }
    } else {
        gtk_pdfview_goto_page( pdf_tab->pdf_view, 0, 0.0, FALSE, FALSE );
    }

    do_update_render_region( pdf_tab, doc, TRUE );
    do_update_page_specific_menu_items( pdf_tab );

    GDK_THREADS_LEAVE();

    watcher_add_document( doc );
}

static void event_document_open_failed( document_t* document ) {
    GtkWidget* error;
    open_progress_t* progress;
    document_history_t* history;
    char path[ 256 ];

    progress = ( open_progress_t* )document->private;
    document->private = NULL;

    snprintf( path, sizeof( path ), "%s%s%s", document->path, PATH_SEPARATOR, document->filename );
    history = history_get_document_info( path, FALSE );

    if ( history != NULL ) {
        history->recently_open = 0;
    }

    GDK_THREADS_ENTER();

    main_window_detach_load_progress( progress->reference );
    free( progress );

    error = gtk_message_dialog_new(
        GTK_WINDOW( main_window ),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "Failed to open %s: not a valid PDF document!",
        document->filename
    );

    gtk_window_set_title( GTK_WINDOW( error ), "Owl - Open Error" );
    gtk_dialog_run( GTK_DIALOG( error ) );
    gtk_widget_destroy( error );

    GDK_THREADS_LEAVE();

    document_destroy( document );
}

static void event_document_open_encrypted( document_t* document ) {
    GtkWidget* dialog;
    GtkWidget* hbox;
    GtkWidget* entry;
    char buf[256];
    open_progress_t* progress;
    int error;

    progress = ( open_progress_t* )document->private;
    document->private = NULL;

    GDK_THREADS_ENTER();

    main_window_detach_load_progress( progress->reference );
    free( progress );

    dialog = gtk_dialog_new_with_buttons(
        "Owl - Encrypted Document",
        GTK_WINDOW( main_window ),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
        NULL
    );

    gtk_dialog_set_default_response( GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT );

    hbox = gtk_hbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), hbox );

    if ( document->password == NULL ) {
        snprintf(
            buf, sizeof( buf ),
            "The document has been encrypted.\nEnter password to open '%s':",
            document->filename
        );
    } else {
        snprintf(
            buf, sizeof( buf ),
            "Nice try... but also wrong.\nEnter the correct password to open '%s':",
            document->filename
        );
    }

    gtk_box_pack_start( GTK_BOX( hbox ), gtk_label_new( buf ), FALSE, FALSE, 3 );

    entry = gtk_entry_new();
    gtk_entry_set_visibility( GTK_ENTRY( entry ), FALSE );
    gtk_entry_set_activates_default( GTK_ENTRY( entry ), TRUE );
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), entry );

    gtk_widget_show_all( dialog );
    error = gtk_dialog_run( GTK_DIALOG( dialog ) );

    if ( error == GTK_RESPONSE_ACCEPT ) {
        if ( document->password != NULL ) {
            free( document->password );
            document->password = NULL;
        }

        document->password = strdup( gtk_entry_get_text( GTK_ENTRY( entry ) ) );

        document_start_open( document );
    } else {
        document_destroy( document );
    }

    gtk_widget_destroy( dialog );

    GDK_THREADS_LEAVE();
}

static void event_document_reload_started( document_t* doc ) {
    pdf_tab_t* pdf_tab;

    GDK_THREADS_ENTER();

    pdf_tab = document_storage_get_tab_by_doc( doc );

    assert( pdf_tab != NULL );

    gtk_pdfview_set_reloading( pdf_tab->pdf_view, TRUE );

    GDK_THREADS_LEAVE();
}

static void event_document_reload_ok( document_t* doc ) {
    pdf_tab_t* pdf_tab;

    GDK_THREADS_ENTER();

    pdf_tab = document_storage_get_tab_by_doc( doc );

    assert( pdf_tab != NULL );

    gtk_pdfview_set_reloading( pdf_tab->pdf_view, FALSE );

    doctab_update_adjustment_limits( pdf_tab );
    do_update_current_page( pdf_tab );
    do_update_page_count( pdf_tab, doc );
    do_update_render_region( pdf_tab, doc, TRUE );

    GDK_THREADS_LEAVE();
}

int event_document_opened( document_t* doc, open_event_t event ) {
    switch ( event ) {
        case DOC_OPEN_STARTED :
            event_document_open_started( doc );
            break;

        case DOC_OPEN_DONE_OK :
            event_document_open_ok( doc );
            break;

        case DOC_OPEN_DONE_FAILED :
            event_document_open_failed( doc );
            break;

        case DOC_OPEN_DONE_ENCRYPTED :
            event_document_open_encrypted( doc );
            break;

        case DOC_RELOAD_STARTED :
            event_document_reload_started( doc );
            break;

        case DOC_RELOAD_DONE_OK :
            event_document_reload_ok( doc );
            break;

        default :
            break;
    }

    return 0;
}

int event_page_render_done( document_t* doc, int page_index ) {
    pdf_tab_t* pdf_tab;

    g_return_val_if_fail( doc != NULL, -EINVAL );

    pdf_tab = document_storage_get_tab_by_doc( doc );

    if ( pdf_tab == NULL ) {
        return 0;
    }

    GDK_THREADS_ENTER();
    gtk_pdfview_draw( pdf_tab->pdf_view, NULL );
    GDK_THREADS_LEAVE();

    return 0;
}

void event_save_selection_as_text( gpointer data ) {
    pdf_tab_t* pdf_tab;
    document_t* document;
    int error;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_save_selection_as_text( pdf_tab, document );
}

void event_save_selection_as_image( gpointer data ) {
    GtkWidget* dialog;
    pdf_tab_t* pdf_tab;
    document_t* document;
    GdkPixbuf* pixbuf = NULL;
    int error;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_save_selection_as_image( pdf_tab, document, &pixbuf );

    if ( pixbuf == NULL ) {
        return;
    }

    dialog = gtk_file_chooser_dialog_new(
        "Save Image To File",
        GTK_WINDOW( main_window ),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL
    );

    if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT ) {
        char* filename;
        GError* gerror = NULL;

        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );

        gdk_pixbuf_save( pixbuf, filename, "png", &gerror, NULL );

        if ( gerror != NULL ) {
            printf( "gdk_pixbuf_save failed: %s\n", gerror->message );

            g_error_free( gerror );
            gerror = NULL;
        }

        g_free( filename );
    }

    gdk_pixbuf_unref( pixbuf );

    gtk_widget_destroy( dialog );
}

void event_toc_selection_changed( GtkTreeSelection * select, gpointer data ) {
    GtkTreeModel* model;
    GtkTreeIter iter;
    gboolean error;
    ref_t* ref;
    pdf_tab_t* pdf_tab = ( pdf_tab_t* )data;

    error = gtk_tree_selection_get_selected( select, &model, &iter );

    if ( error == FALSE ) {
        return;
    }

    gtk_tree_model_get( model, &iter, REF_COL_DATA, &ref, -1 );

    gtk_pdfview_goto_page( pdf_tab->pdf_view, ref->page_num, 0.0, FALSE, TRUE );
}

static const char* poppler_backend( void ) {
    switch ( poppler_get_backend() ) {
        case POPPLER_BACKEND_SPLASH :
            return "Splash";
        case POPPLER_BACKEND_CAIRO :
            return "Cairo";
        case POPPLER_BACKEND_UNKNOWN :
        default :
            return "Unknown";
    };
}

void event_show_about( GtkWidget* widget, gpointer data ) {
    GtkWidget* about;
    char text[ 256 ];
    char path[ 256 ];

    about = gtk_message_dialog_new_with_markup(
        GTK_WINDOW( main_window ),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_CLOSE,
        NULL
    );

    snprintf(
        text,
        sizeof( text ),
        "<big><b>Owl PDF reader " OWL_VERSION "</b></big>\n\n"
        "Developers: Peter Szilagyi, Zoltan Kovacs\n\n"
        "Poppler version: %s\n"
        "Poppler backend: %s",
        poppler_get_version(),
        poppler_backend()
    );

    gtk_message_dialog_set_markup( GTK_MESSAGE_DIALOG( about ), text );

    gtk_window_set_title( GTK_WINDOW( about ), "Owl - About" );

    snprintf(
        path,
        sizeof( path ),
        "%s%spixmaps%sicon-128.png",
        OWL_INSTALL_PATH,
        PATH_SEPARATOR,
        PATH_SEPARATOR
    );

    gtk_message_dialog_set_image(
        GTK_MESSAGE_DIALOG( about ),
        gtk_image_new_from_file( path )
    );

    gtk_widget_show_all( about );

    gtk_dialog_run( GTK_DIALOG( about ) );
    gtk_widget_destroy( about );
}
