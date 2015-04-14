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
#include <gdk/gdkkeysyms.h>

#include <storage.h>
#include <utils.h>
#include <config/history.h>
#include <engine/ref.h>
#include <gui/events.h>
#include <gui/pdf_view.h>
#include <gui/menu.h>

pdf_tab_t* popup_pdf_tab;

void event_doctab_goto_page( GtkEntry* widget, gpointer data ) {
    int page_number;
    const gchar* text;
    pdf_tab_t* pdf_tab;
    document_t* document;

    pdf_tab = ( pdf_tab_t* )data;

    text = gtk_entry_get_text( widget );
    g_return_if_fail( text != NULL );

    document = document_storage_get_doc_by_widget( pdf_tab->root );
    g_return_if_fail( document != NULL );

    if ( sscanf( text, "%d", &page_number ) != 1 ) {
        return;
    }

    gtk_pdfview_goto_page( pdf_tab->pdf_view, page_number - 1, 0.0, FALSE, FALSE );
}

void event_doctab_find_next( GtkButton* button, gpointer data ) {
    pdf_tab_t* pdf_tab = ( pdf_tab_t* )data;

    do_doctab_find( pdf_tab, FIND_FORWARD );
}

void event_doctab_find_prev( GtkButton* button, gpointer data ) {
    pdf_tab_t* pdf_tab = ( pdf_tab_t* )data;

    do_doctab_find( pdf_tab, FIND_BACK );
}

void event_doctab_find_close( GtkButton* button, gpointer data ) {
    pdf_tab_t* pdf_tab = ( pdf_tab_t* )data;

    do_doctab_hide_search_bar( pdf_tab->pdf_view );
}

gboolean event_doctab_find_keypress( GtkWidget* widget, GdkEventKey* event, gpointer data ) {
    pdf_tab_t* pdf_tab = ( pdf_tab_t* )data;

    switch ( event->keyval ) {
        case GDK_Escape :
            do_doctab_hide_search_bar( pdf_tab->pdf_view );
            gtk_widget_grab_focus( pdf_tab->pdf_view );

            break;

        case GDK_Return :
        case GDK_KP_Enter :
            if ( event->state & GDK_SHIFT_MASK ) {
                do_doctab_find( pdf_tab, FIND_BACK );
            } else {
                do_doctab_find( pdf_tab, FIND_FORWARD );
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

void event_doctab_find_text_changed( GtkEditable *editable, gpointer data ) {
    pdf_tab_t* pdf_tab;
    document_t* doc;

    pdf_tab = ( pdf_tab_t* )data;

    doc = document_storage_get_doc_by_pdfview( pdf_tab->pdf_view );

    if ( doc == NULL ) {
        return;
    }

    if ( doc->fc != NULL && doc->fc->running ) {
        find_stop( doc->fc );
        doc->fc = NULL;
    }

    doctab_set_search_status( pdf_tab, SEARCH_STATUS_CLEAR );
}

gboolean event_doctab_toc_query_tooltip( GtkWidget* widget, gint x, gint y, gboolean keyboard_mode,
                                         GtkTooltip* tooltip, gpointer data ) {
    pdf_tab_t* pdf_tab = ( pdf_tab_t* )data;
    char buf[ 256 ];
    char* title;
    char* page_num;
    GtkTreeModel* model;
    GtkTreePath* path;
    GtkTreeIter iter;
    gboolean error;

    error = gtk_tree_view_get_path_at_pos(
        GTK_TREE_VIEW( pdf_tab->toc_view ),
        x,
        y - 25 /* no fucking way to query the header height, why indeed! */,
        &path,
        NULL, NULL, NULL
    );

    if ( error == FALSE ) {
        return FALSE;
    }

    model = gtk_tree_view_get_model( GTK_TREE_VIEW( pdf_tab->toc_view ) );

    gtk_tree_model_get_iter( model, &iter, path );
    gtk_tree_model_get( model, &iter, REF_COL_TITLE, &title, REF_COL_PAGE_NUM, &page_num, -1 );

    snprintf( buf, sizeof( buf ), "%s [page %s]", title, page_num );

    gtk_tooltip_set_text( tooltip, buf );

    g_free( title );
    g_free( page_num );
    gtk_tree_path_free( path );

    return TRUE;
}

void event_doctab_size_allocate( GtkWidget* widget, GtkAllocation* allocation, gpointer data ) {
    char path[ 256 ];
    pdf_tab_t* tab = ( pdf_tab_t* )data;
    document_history_t* history;
    document_t* doc;

    doc = GTK_PDFVIEW( widget )->document;

    snprintf( path, sizeof( path ), "%s%s%s", doc->path, PATH_SEPARATOR, doc->filename );

    history = history_get_document_info( path, FALSE );

    if ( ( !tab->history_used ) &&
         ( history != NULL ) ) {
        tab->history_used = 1;

        doctab_update_adjustment_limits( tab );

        doctab_v_adjustment_set_value( tab, history->v_offset );
        doctab_h_adjustment_set_value( tab, history->h_offset );
    }
}

typedef struct header_lookup_data {
    GtkWidget* event_box;
    pdf_tab_t** pdf_tab;
} header_lookup_data_t;

static int header_lookup_helper( document_t* document, pdf_tab_t* pdf_tab, void* _data ) {
    header_lookup_data_t* data = ( header_lookup_data_t* )_data;

    if ( data->event_box == pdf_tab->event_box ) {
        *( data->pdf_tab ) = pdf_tab;
    }

    return 0;
}

gboolean event_doctab_header_click( GtkWidget* widget, GdkEventButton* event, gpointer _data ) {
    header_lookup_data_t data;

    if ( event->button != 3 ) {
        return FALSE;
    }

    popup_pdf_tab = NULL;

    data.event_box = widget;
    data.pdf_tab = &popup_pdf_tab;

    document_storage_foreach( header_lookup_helper, ( void* )&data );

    if ( popup_pdf_tab == NULL ) {
        fprintf( stderr, "PDF tab not found for popup menu!\n" );
        return FALSE;
    }

    popup_notebook_menu( event->button, event->time );

    return TRUE;
}
