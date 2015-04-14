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

#include <math.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include <storage.h>
#include <config/settings.h>
#include <gui/gui-utils.h>
#include <gui/main_wnd.h>
#include <gui/menu.h>
#include <gui/events.h>
#include <gui/pdf_view.h>
#include <engine/document.h>

extern const int PAGE_BORDER;
extern const int PAGE_GAP;

void pdfview_class_init( GtkPdfViewClass* class );
void pdfview_init( GtkPdfView* pdf_view );

void pdfview_get_sel_rect( GtkPdfView* pdf_view, int* x1, int* y1, int* x2, int* y2 );
void pdfview_get_sel_gdk_rect( GtkPdfView* pdf_view, GdkRectangle* rect );

void pdfview_goto_page( GtkPdfView* pdf_view, int page_index, double valign, int relative, int add_to_hist );

int pdfview_page_to_screen( GtkPdfView* pdf_view, int page_x, int page_y,
                            int page_index, int* screen_x, int* screen_y );

int pdfview_screen_to_page( GtkPdfView* pdf_view, int screen_x, int screen_y,
                            int* page_index, int* page_x, int* page_y );

void pdfview_update_auto_zoom( GtkPdfView* pdf_view );

void pdfview_set_fullscreen_mode( GtkPdfView* pdf_view, gboolean enabled );

GtkType gtk_pdfview_get_type( void ) {
    static GtkType gtk_pdfview_type = 0;

    if ( gtk_pdfview_type == 0 ) {
        static const GtkTypeInfo gtk_pdfview_info = {
            "GtkPdfView",
            sizeof( GtkPdfView ),
            sizeof( GtkPdfViewClass ),
            ( GtkClassInitFunc )pdfview_class_init,
            ( GtkObjectInitFunc )pdfview_init,
            NULL,
            NULL,
            ( GtkClassInitFunc )NULL
        };

        gtk_pdfview_type = gtk_type_unique( GTK_TYPE_WIDGET, &gtk_pdfview_info );
    }

    return gtk_pdfview_type;
}

GtkWidget* gtk_pdfview_new( document_t* doc ) {
    return g_object_new( gtk_pdfview_get_type(), "document", doc, NULL );
}

void gtk_pdfview_goto_page( GtkWidget* widget, int page_index, double valign, int relative, int add_to_hist ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdfview_goto_page( pdf_view, page_index, valign, relative, add_to_hist );
}

void gtk_pdfview_set_reloading( GtkWidget* widget, gboolean reloading ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_view->reloading = reloading;

    if ( !reloading ) {
        int page_count = document_get_page_count( pdf_view->document );

        if ( pdf_view->current_page >= page_count ) {
            pdf_view->current_page = MAX( 0, page_count - 1 );
        }
    }

    gtk_pdfview_draw( widget, NULL );
}

void gtk_pdfview_history_back( GtkWidget* widget ) {
    GtkPdfView* pdf_view;
    goto_history_t* entry;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    if ( pdf_view->goto_position == -1 ) {
        return;
    }

    printf( "%s() goto_position = %d\n", __FUNCTION__, pdf_view->goto_position );

    entry = g_list_nth_data( pdf_view->goto_list, pdf_view->goto_position );
    pdf_view->goto_position--;

    gtk_pdfview_goto_page( widget, entry->page_number, entry->valign, FALSE, FALSE );
}

void gtk_pdfview_history_forward( GtkWidget* widget ) {
    GtkPdfView* pdf_view;
    goto_history_t* entry;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    if ( pdf_view->goto_position == g_list_length( pdf_view->goto_list ) ) {
        return;
    }

    pdf_view->goto_position++;

    printf( "%s() goto_position = %d\n", __FUNCTION__, pdf_view->goto_position );

    entry = g_list_nth_data( pdf_view->goto_list, pdf_view->goto_position );

    printf( "%s() entry->page_number=%d\n", __FUNCTION__, entry->page_number );

    gtk_pdfview_goto_page( widget, entry->page_number, entry->valign, FALSE, FALSE );
}

static void gtk_pdfview_update_current_page( GtkWidget* widget ) {
    GtkPdfView* pdf_view;
    int row_count;
    int row_width;
    int row_height;
    int height = 0;
    int i;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    if ( pdf_view->display_type == SINGLE_PAGE ) {
        return;
    }

    row_count = document_get_row_count( pdf_view->document, pdf_view->cols );

    for ( i = 0; i < row_count; i++ ) {
        document_row_get_geometry( pdf_view->document, i, pdf_view->cols, &row_width, &row_height );

        if ( height + row_height >= pdf_view->v_offset ) {
            pdf_view->current_page = i * pdf_view->cols;
            return;
        }

        height += row_height + PAGE_GAP + 2 * PAGE_BORDER;
    }

    pdf_view->current_page = document_get_page_count( pdf_view->document ) - 1;
}

int gtk_pdfview_get_current_page( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, 0 );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), 0 );

    pdf_view = GTK_PDFVIEW( widget );

    return pdf_view->current_page;
}

gboolean gtk_pdfview_get_selection_mode( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    pdf_view = GTK_PDFVIEW( widget );

    return pdf_view->selection_mode;
}

void gtk_pdfview_set_selection_mode( GtkWidget* widget, gboolean selection_mode ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_view->selection_mode = selection_mode;

    if ( !selection_mode ) {
        pdf_view->selection_active = 0;
    }
}

void gtk_pdfview_get_sel_rect( GtkWidget* widget, int* x1, int* y1, int* x2, int* y2 ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdfview_get_sel_rect( pdf_view, x1, y1, x2, y2 );
}

void gtk_pdfview_get_sel_gdk_rect( GtkWidget* widget, GdkRectangle* rect ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdfview_get_sel_gdk_rect( pdf_view, rect );
}

int gtk_pdfview_screen_to_page( GtkWidget* widget, int screen_x, int screen_y,
                                int* page_index, int* page_x, int* page_y ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, 0 );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), 0 );

    pdf_view = GTK_PDFVIEW( widget );

    return pdfview_screen_to_page( pdf_view, screen_x, screen_y, page_index, page_x, page_y );
}

void gtk_pdfview_set_highlight( GtkWidget* widget, int page_num, PopplerRectangle* rect ) {
    GtkPdfView* pdf_view;
    page_t* page;
    int y;
    double valign = 0.0;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_view->highlight_rect = *rect;
    pdf_view->highlight_page_index = page_num;
    pdf_view->highlight_mode = 1;

    page = document_get_page( pdf_view->document, page_num );

    g_return_if_fail( page != NULL );

    y = ( rect->y1 + rect->y2 ) / 2;

    switch ( page->rotate ) {
        case 0:
        case 180:
            valign = 1.0 - ( double )y / page->original_height;
            break;

        case 90:
        case 270:
            valign = 1.0 - ( double )y / page->original_width;
            break;
    }

    pdfview_goto_page( pdf_view, page_num, valign, FALSE, FALSE );

    gtk_pdfview_draw( widget, NULL );
}

void gtk_pdfview_clear_highlight( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_view->highlight_mode = 0;
}

gboolean gtk_pdfview_get_fullscreen_mode( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    pdf_view = GTK_PDFVIEW( widget );

    return pdf_view->fullscreen_mode;
}

void gtk_pdfview_set_fullscreen_mode( GtkWidget* widget, gboolean enabled ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdfview_set_fullscreen_mode( pdf_view, enabled );
}

void gtk_pdfview_draw( GtkWidget* widget, GdkRectangle* area ) {
    if ( area != NULL ) {
        gtk_widget_queue_draw_area(
            widget,
            area->x,
            area->y,
            area->width,
            area->height
        );
    } else {
        gtk_widget_queue_draw_area(
            widget,
            0,
            0,
            widget->allocation.width,
            widget->allocation.height
        );
    }
}

void gtk_pdfview_set_v_offset( GtkWidget* widget, int offset ) {
    GtkPdfView* pdf_view;
    int old_page_index;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    if ( pdf_view->v_offset == offset ) {
        return;
    }

    if ( ( pdf_view->selection_mode ) && ( pdf_view->selection_active ) ) {
        int diff = pdf_view->v_offset - offset;

        pdf_view->sel_y1 += diff;
        pdf_view->sel_y2 += diff;
    }

    pdf_view->v_offset = offset;

    old_page_index = pdf_view->current_page;

    gtk_pdfview_update_current_page( widget );

    if ( old_page_index != pdf_view->current_page ) {
        pdf_tab_t* pdf_tab = document_storage_get_tab_by_pdfview( widget );

        pdfview_update_auto_zoom( pdf_view );

        do_update_render_region( pdf_tab, pdf_view->document, TRUE );
        do_update_current_page( pdf_tab );
    }

    gtk_pdfview_draw( widget, NULL );
}

void gtk_pdfview_set_h_offset( GtkWidget* widget, int offset ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    if ( pdf_view->h_offset == offset ) {
        return;
    }

    if ( pdf_view->selection_mode && pdf_view->selection_active ) {
        int diff = pdf_view->h_offset - offset;

        pdf_view->sel_x1 += diff;
        pdf_view->sel_x2 += diff;
    }

    pdf_view->h_offset = offset;

    gtk_pdfview_draw( widget, NULL );
}

int gtk_pdfview_get_cols( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, 0 );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), 0 );

    pdf_view = GTK_PDFVIEW( widget );

    return pdf_view->cols;
}

void gtk_pdfview_get_geometry( GtkWidget* widget, int* width, int* height ) {
    GtkPdfView* pdf_view;
    int row_count;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    document_get_geometry( pdf_view->document, pdf_view->cols, width, height );
    row_count = document_get_row_count( pdf_view->document, pdf_view->cols );

    *width += ( pdf_view->cols - 1 ) * PAGE_GAP + pdf_view->cols * 2 * PAGE_BORDER;
    *height += ( row_count - 1 ) * PAGE_GAP + row_count * 2 * PAGE_BORDER;
}

void gtk_pdfview_row_get_original_geometry( GtkWidget* widget, int row, int* width, int* height ) {
    GtkPdfView* pdf_view;
    int cols;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    document_row_get_original_geometry( pdf_view->document, row, pdf_view->cols, width, height );

    cols = MIN( pdf_view->cols, document_get_page_count( pdf_view->document ) - row * pdf_view->cols );

    *width += ( cols - 1 ) * PAGE_GAP + cols * 2 * PAGE_BORDER;
    *height += 2 * PAGE_BORDER;
}

void gtk_pdfview_current_row_get_geometry( GtkWidget* widget, int* width, int* height ) {
    GtkPdfView* pdf_view;
    int cols;
    int row;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    row = pdf_view->current_page / pdf_view->cols;

    document_row_get_geometry( pdf_view->document, row, pdf_view->cols, width, height );

    cols = MIN( pdf_view->cols, document_get_page_count( pdf_view->document ) - row * pdf_view->cols );

    *width += ( cols - 1 ) * PAGE_GAP + cols * 2 * PAGE_BORDER;
    *height += 2 * PAGE_BORDER;
}

void gtk_pdfview_set_multipage( GtkWidget* widget, int cols ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_view->cols = cols;

    pdfview_update_auto_zoom( pdf_view );

    gtk_pdfview_draw( widget, NULL );
}

zoom_t gtk_pdfview_get_zoom_type( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, -1 );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), -1 );

    pdf_view = GTK_PDFVIEW( widget );

    return pdf_view->zoom_type;
}

void gtk_pdfview_set_zoom_type( GtkWidget* widget, zoom_t type ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_view->zoom_type = type;
}

void gtk_pdfview_update_auto_zoom( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdfview_update_auto_zoom( pdf_view );
}

display_t gtk_pdfview_get_display_type( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, -1 );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), -1 );

    pdf_view = GTK_PDFVIEW( widget );

    return pdf_view->display_type;
}

void gtk_pdfview_set_display_type( GtkWidget* widget, display_t type ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    if ( type == pdf_view->display_type ) {
        return;
    }

    pdf_view->display_type = type;
}

void gtk_pdfview_pick_page( GtkWidget* widget, void ( *cb )( int page_index ), void ( *canceled_cb )( void ) ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_view->pick_page = TRUE;
    pdf_view->pick_callback = cb;
    pdf_view->pick_canceled_callback = canceled_cb;

    gdk_window_set_cursor( widget->window, gdk_cursor_new( GDK_CROSS ) );
}

void gtk_pdfview_invalidate_selection( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    if ( pdf_view->selection_mode && pdf_view->selection_active ) {
        pdf_view->selection_active = 0;
    }
}

gboolean gtk_pdfview_get_size_allocated( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    pdf_view = GTK_PDFVIEW( widget );

    return pdf_view->size_allocated;
}

void gtk_pdfview_reload_selection_color( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL  );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    settings_get_color( "color-selection", &pdf_view->color_selection );

    gtk_pdfview_draw( widget, NULL );
}

void gtk_pdfview_reload_highlight_color( GtkWidget* widget ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL  );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    pdf_view = GTK_PDFVIEW( widget );

    settings_get_color( "color-highlight", &pdf_view->color_highlight );

    gtk_pdfview_draw( widget, NULL );
}
