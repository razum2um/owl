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

#include <string.h>

#include <utils.h>
#include <storage.h>
#include <gui/events.h>
#include <gui/main_wnd.h>
#include <gui/print.h>
#include <gui/pdf-info.h>
#include <gui/pdf_view.h>
#include <gui/settings.h>
#include <gui/gui-utils.h>
#include <config/settings.h>

void event_menu_open_document( GtkMenuItem* widget, gpointer _data ) {
    char* path;
    GtkWidget* dialog;
    GtkFileFilter* pdf_filter;

    dialog = gtk_file_chooser_dialog_new(
        "Open PDF File",
        GTK_WINDOW( main_window ),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL
    );

    pdf_filter = get_pdf_file_filter();

    gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( dialog ), get_all_file_filter() );
    gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( dialog ), pdf_filter );
    gtk_file_chooser_set_filter( GTK_FILE_CHOOSER( dialog ), pdf_filter );
    gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER( dialog ), TRUE );

    if ( settings_get_string( "open-file-path", &path ) == 0 ) {
        gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), path );
    }

    if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT ) {
        GSList* item;
        GSList* file_list;

        file_list = gtk_file_chooser_get_uris( GTK_FILE_CHOOSER( dialog ) );

        item = file_list;

        while ( item != NULL ) {
            do_open_document( ( char* )item->data );

            g_free( item->data );
            item = g_slist_next( item );
        }

        g_slist_free( file_list );
    }

    gtk_widget_destroy( dialog );
}

void event_menu_save_as_document( GtkMenuItem* widget, gpointer data ) {
    int error;
    char* tmp;
    GtkWidget* dialog;
    document_t* document;

    error = get_current_document_info( NULL, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    dialog = gtk_file_chooser_dialog_new(
        "Save File",
        GTK_WINDOW( main_window ),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER( dialog ), TRUE );

    if ( settings_get_string( "save-file-path", &tmp ) == 0 ) {
        gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), tmp );
    }

    gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( dialog ), document->filename );

    if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT ) {
        char tmp_path[ 256 ];
        char* filename;

        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );

        do_save_document( document, filename );

        /* Save the path */

        tmp = g_strrstr( filename, PATH_SEPARATOR );

        if ( tmp != NULL ) {
            *tmp = 0;
        }

        snprintf( tmp_path, sizeof( tmp_path ), "file://%s", filename );
        settings_set_string( "save-file-path", tmp_path );

        g_free( filename );
    }

    gtk_widget_destroy( dialog );
}

void event_menu_close_current_document( GtkMenuItem* widget, gpointer data ) {
    int error;
    int page_index;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, &page_index );

    if ( error < 0 ) {
        return;
    }

    do_close_document_tab( pdf_tab, document, page_index );
}

void event_menu_close_all_document( GtkMenuItem* widget, gpointer data ) {
    int page_index;
    pdf_tab_t* pdf_tab;
    document_t* document;

    while ( get_current_document_info( &pdf_tab, &document, &page_index ) == 0 ) {
        do_close_document_tab( pdf_tab, document, page_index );
    }
}

void event_menu_open_settings( GtkMenuItem* widget, gpointer data ) {
    show_settings_window();
}

void event_menu_open_preferences( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    pdf_info_show_properties( document );
}

void event_menu_print( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    print_operation_start( pdf_tab, document );
}

void event_menu_page_setup( GtkMenuItem* widget, gpointer data ) {
    print_page_setup();
}

void event_menu_zoom_in( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_zoom( pdf_tab, document, -1, 10 );
}

void event_menu_zoom_out( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_zoom( pdf_tab, document, -1, -10 );
}

void event_menu_zoom_page_in( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    doctab_set_statusbar( pdf_tab, 0, "Pick a page to zoom in or ESC to exit" );

    gtk_pdfview_pick_page( pdf_tab->pdf_view, do_zoom_page_in_cb, do_pick_page_canceled_cb );
}

void event_menu_zoom_page_out( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    doctab_set_statusbar( pdf_tab, 0, "Pick a page to zoom out or ESC to exit" );

    gtk_pdfview_pick_page( pdf_tab->pdf_view, do_zoom_page_out_cb, do_pick_page_canceled_cb );
}

void event_menu_fit_width( GtkMenuItem* widget, gpointer data ) {
    do_zoom_fit_width();
}

void event_menu_fit_page( GtkMenuItem* widget, gpointer data ) {
    do_zoom_fit_page();
}

void event_menu_rotate_left( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_rotate( pdf_tab, document, -1, -90 );
}

void event_menu_rotate_right( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_rotate( pdf_tab, document, -1, 90 );
}

void event_menu_rotate_page_left( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    doctab_set_statusbar( pdf_tab, 0, "Pick a page to rotate left or ESC to exit" );

    gtk_pdfview_pick_page( pdf_tab->pdf_view, do_rotate_page_left_cb, do_pick_page_canceled_cb );
}

void event_menu_rotate_page_right( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    doctab_set_statusbar( pdf_tab, 0, "Pick a page to rotate right or ESC to exit" );

    gtk_pdfview_pick_page( pdf_tab->pdf_view, do_rotate_page_right_cb, do_pick_page_canceled_cb );
}

void event_menu_back_in_doc( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    gtk_pdfview_history_back( pdf_tab->pdf_view );
}

void event_menu_forward_in_doc( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    doctab_set_statusbar( pdf_tab, 0, "Pick a page to rotate right or ESC to exit" );

    gtk_pdfview_history_forward( pdf_tab->pdf_view );
}

void event_menu_prev_page( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    gtk_pdfview_goto_page( pdf_tab->pdf_view, -1, 0.0, TRUE, FALSE );
}

void event_menu_next_page( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    gtk_pdfview_goto_page( pdf_tab->pdf_view, 1, 0.0, TRUE, FALSE );
}

void event_menu_first_page( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    gtk_pdfview_goto_page( pdf_tab->pdf_view, 0, 0.0, FALSE, FALSE );
}

void event_menu_last_page( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    gtk_pdfview_goto_page( pdf_tab->pdf_view, document_get_page_count( document ) - 1, 0.0, FALSE, FALSE );
}

void event_menu_prev_doc( GtkMenuItem* widget, gpointer data ) {
    main_window_prev_tab();
}

void event_menu_next_doc( GtkMenuItem* widget, gpointer data ) {
    main_window_next_tab();
}

void event_menu_reverse_pages( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    document_reverse_pages( document );

    do_update_render_region( pdf_tab, document, TRUE );
}

void event_menu_set_multipage( GtkMenuItem* widget, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;
    int* cols;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    cols = ( int* )data;

    gtk_pdfview_set_multipage( pdf_tab->pdf_view, *cols );

    doctab_update_adjustment_limits( pdf_tab );
}

void event_menu_toggle_cont_mode( GtkCheckMenuItem* checkmenuitem, gpointer data ) {
    int error;
    pdf_tab_t* pdf_tab;
    gboolean active;
    display_t display_type;
    display_t new_display_type;

    error = get_current_document_info( &pdf_tab, NULL, NULL );

    if ( error < 0 ) {
        return;
    }

    active = gtk_check_menu_item_get_active( checkmenuitem );

    display_type = gtk_pdfview_get_display_type( pdf_tab->pdf_view );
    new_display_type = active ? CONTINUOUS : SINGLE_PAGE;

    if ( display_type == new_display_type ) {
        return;
    }

    gtk_pdfview_set_display_type( pdf_tab->pdf_view, new_display_type );

    doctab_update_adjustment_limits( pdf_tab );

    gtk_pdfview_goto_page( pdf_tab->pdf_view, 0, 0.0, TRUE, FALSE );
}

void event_menu_set_fullscreen_mode( GtkCheckMenuItem* widget, gpointer data ) {
    gboolean active = FALSE;

    /* If not called from its natural menubar callback, but from the
       pdfview popup menu instead, the widget is not actually a check
       menu item. In that case, the desired state is FALSE anyway. */

    if ( ( widget != NULL ) && ( GTK_IS_CHECK_MENU_ITEM( widget ) ) ) {
        active = gtk_check_menu_item_get_active( widget );
    }

    main_window_set_fullscreen_mode( active );
}
