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
#include <utils.h>
#include <config/settings.h>
#include <gui/gui-utils.h>
#include <gui/main_wnd.h>
#include <gui/menu.h>
#include <gui/events.h>
#include <gui/pdf_view.h>
#include <engine/document.h>

enum {
    PROP_0,
    PROP_DOCUMENT
};

static const int SEL_HANDLE_SIZE = 5;
const int PAGE_BORDER = 2;
const int PAGE_GAP = 10;

void pdfview_update_auto_zoom( GtkPdfView* pdf_view ) {
    if ( pdf_view->zoom_type == ZOOM_FIT_PAGE ) {
        do_zoom_fit_page();
    } else if ( pdf_view->zoom_type == ZOOM_FIT_WIDTH ) {
        do_zoom_fit_width();
    }
}

static void pdfview_size_request( GtkWidget* widget, GtkRequisition* requisition ) {
    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );
    g_return_if_fail( requisition != NULL );

    requisition->width = 1;
    requisition->height = 1;
}

static void pdfview_size_allocate( GtkWidget* widget, GtkAllocation* allocation ) {
    GtkPdfView* pdf_view;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );
    g_return_if_fail( allocation != NULL );

    widget->allocation = *allocation;

    if ( GTK_WIDGET_REALIZED( widget ) ) {
        gdk_window_move_resize(
            widget->window,
            allocation->x, allocation->y,
            allocation->width, allocation->height
        );

        pdf_view = GTK_PDFVIEW( widget );

        pdf_view->size_allocated = TRUE;

        doctab_update_adjustment_limits( document_storage_get_tab_by_pdfview( widget ) );

        pdfview_update_auto_zoom( pdf_view );
    }
}

static void pdfview_realize( GtkWidget* widget ) {
    GdkWindowAttr attributes;

    g_return_if_fail( widget != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( widget ) );

    GTK_WIDGET_SET_FLAGS( widget, GTK_REALIZED );

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.event_mask = \
        gtk_widget_get_events( widget ) | \
        GDK_EXPOSURE_MASK | \
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | \
        GDK_BUTTON_MOTION_MASK | GDK_POINTER_MOTION_MASK | \
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK;

    widget->window = gdk_window_new(
        gtk_widget_get_parent_window( widget ),
        &attributes,
        GDK_WA_X | GDK_WA_Y
    );

    gdk_window_set_user_data( widget->window, widget );

    widget->style = gtk_style_attach( widget->style, widget->window );

    gtk_widget_add_events( widget, GDK_SCROLL_MASK );
    gtk_style_set_background( widget->style, widget->window, GTK_STATE_NORMAL );
}

void pdfview_get_sel_rect( GtkPdfView* pdf_view, int* x1, int* y1, int* x2, int* y2 ) {
    *x1 = MIN( pdf_view->sel_x1, pdf_view->sel_x2 );
    *x2 = MAX( pdf_view->sel_x1, pdf_view->sel_x2 );
    *y1 = MIN( pdf_view->sel_y1, pdf_view->sel_y2 );
    *y2 = MAX( pdf_view->sel_y1, pdf_view->sel_y2 );
}

void pdfview_get_sel_gdk_rect( GtkPdfView* pdf_view, GdkRectangle* rect ) {
    rect->x = MIN( pdf_view->sel_x1, pdf_view->sel_x2 ) - ( SEL_HANDLE_SIZE + 1 );
    rect->y = MIN( pdf_view->sel_y1, pdf_view->sel_y2 ) - ( SEL_HANDLE_SIZE + 1 );
    rect->width = ABS( pdf_view->sel_x2 - pdf_view->sel_x1 ) + 1 + 2 * ( SEL_HANDLE_SIZE + 1 );
    rect->height = ABS( pdf_view->sel_y2 - pdf_view->sel_y1 ) + 1 + 2 * ( SEL_HANDLE_SIZE + 1 );
}

static int pdfview_sel_contains( GtkPdfView* pdf_view, int x, int y ) {
    int x1;
    int x2;
    int y1;
    int y2;

    pdfview_get_sel_rect( pdf_view, &x1, &y1, &x2, &y2 );

    return ( x >= x1 ) && ( x <= x2 ) && ( y >= y1 ) && ( y <= y2 );
}

int pdfview_page_to_screen( GtkPdfView* pdf_view, int page_x, int page_y,
                            int page_index, int* screen_x, int* screen_y ) {
    double x;
    double y;
    page_t* page;
    double norm;

    page = document_get_page( pdf_view->document, page_index );

    if ( page == NULL ) {
        return -1;
    }

    switch ( page->rotate ) {
        case 0 :
            /* no rotation; translate origin to the top left corner of the page */
            x = page_x;
            y = page->original_height - page_y;
            break;

        case 90 :
            /* rotate right: translate + rotate */
            x = page_y;
            y = page_x;
            break;

        case 180 :
            /* upside-down: translate + rotate */
            x = page->original_width - page_x;
            y = page_y;
            break;

        case 270 :
            /* rotate left: translate + rotate */
            x = page->original_height - page_y;
            y = page->original_width - page_x;
            break;

        default :
            return -1;
    }

    /* scaling */
    norm = page->scale / 100.0;

    if ( screen_x != NULL ) {
        *screen_x = x * norm;
    }

    if ( screen_y != NULL ) {
        *screen_y = y * norm;
    }

    return 0;
}

int pdfview_screen_to_page( GtkPdfView* pdf_view, int screen_x, int screen_y,
                            int* page_index, int* page_x, int* page_y ) {
    int width;
    int height;
    int row_count;
    int row_width;
    int row_height;
    int current_page;
    int cols;
    int col;
    int row;
    int x;
    int y;

    gdk_window_get_geometry( GTK_WIDGET( pdf_view )->window, NULL, NULL, &width, &height, NULL );

    row_count = document_get_row_count( pdf_view->document, pdf_view->cols );

    y = -pdf_view->v_offset;

    for ( row = 0; row < row_count; row++ ) {
        document_row_get_geometry( pdf_view->document, row, pdf_view->cols, &row_width, &row_height );

        cols = MIN( pdf_view->cols, document_get_page_count( pdf_view->document ) - row * pdf_view->cols );

        row_width += ( cols - 1 ) * PAGE_GAP + cols * PAGE_BORDER;

        x = MAX( 0, ( width - row_width ) / 2 ) - pdf_view->h_offset;
        col = 0;
        current_page = row * pdf_view->cols;

        while ( col < cols ) {
            page_t* page = document_get_page( pdf_view->document, current_page);
            int y2 = y + ( row_height - page->current_height ) / 2;

            if ( screen_x >= x && screen_x <= x + page->current_width &&
                 screen_y >= y2 && screen_y <= y2 + page->current_height ) {
                /* poppler maintains the origin at the bottom left corner,
                   with axis y pointing upwards */
                switch ( page->rotate ) {
                    case 0 :
                        /* no rotation; translate origin to the top left corner of the page */
                        *page_x = screen_x - x;
                        *page_y = page->current_height - ( screen_y - y2 );
                        break;

                    case 90 :
                        /* rotate right: translate + rotate */
                        *page_x = screen_y - y2;
                        *page_y = screen_x - x;
                        break;

                    case 180 :
                        /* upside-down: translate + rotate */
                        *page_x = page->current_width - ( screen_x - x );
                        *page_y = screen_y - y2;
                        break;

                    case 270 :
                        /* rotate left: translate + rotate */
                        *page_x = page->current_height - ( screen_y - y2 );
                        *page_y = page->current_width - ( screen_x - x );
                        break;
                }

                /* scaling and return */
                *page_index = current_page;
                *page_x *= 100.0 / page->scale;
                *page_y *= 100.0 / page->scale;

                return 0;
            }

            x += page->current_width + PAGE_GAP + 2 * PAGE_BORDER;

            ++col;
            ++current_page;
        }

        y += row_height + PAGE_GAP + 2 * PAGE_BORDER;
    }

    return -1;
}

static int pdfview_draw_highlight( GtkPdfView* pdf_view, cairo_t* cr, page_t* page, int x, int y ) {
    int x1;
    int x2;
    int y1;
    int y2;
    int error;

    error = pdfview_page_to_screen(
        pdf_view,
        pdf_view->highlight_rect.x1,
        pdf_view->highlight_rect.y1,
        pdf_view->highlight_page_index,
        &x1,
        &y1
    );

    if ( error < 0 ) {
        return 0;
    }

    error = pdfview_page_to_screen(
        pdf_view,
        pdf_view->highlight_rect.x2,
        pdf_view->highlight_rect.y2,
        pdf_view->highlight_page_index,
        &x2,
        &y2
    );

    if ( error < 0 ) {
        return 0;
    }

    if ( x1 > x2 ) {
        int tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if ( y1 > y2 ) {
        int tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    x1 += x;
    x2 += x;
    y1 += y;
    y2 += y;

    cairo_set_line_width( cr, 1 );

    cairo_rectangle(
        cr,
        x1 + page->scale / 100,
        y1,
        x2 - x1 + 2 * page->scale / 100,
        y2 - y1 + 2 * page->scale / 100
    );

    cairo_set_source_rgba(
        cr,
        pdf_view->color_highlight.red / 65536.0,
        pdf_view->color_highlight.green / 65536.0,
        pdf_view->color_highlight.blue / 65536.0,
        0.2
    );

    cairo_fill_preserve( cr );

    cairo_set_source_rgba(
        cr,
        pdf_view->color_highlight.red / 65536.0,
        pdf_view->color_highlight.green / 65536.0,
        pdf_view->color_highlight.blue / 65536.0,
        0.25
    );

    cairo_stroke( cr );

    return 0;
}

static int pdfview_draw_selection( GtkPdfView* pdf_view, cairo_t* cr ) {
    int x1;
    int x2;
    int y1;
    int y2;

    pdfview_get_sel_rect( pdf_view, &x1, &y1, &x2, &y2 );

    if ( ( x1 == x2 ) && ( y1 == y2 ) ) {
        return 0;
    }

    cairo_set_line_width( cr, 1 );

    cairo_rectangle( cr, x1, y1, x2 - x1 + 1, y2 - y1 + 1 );

    cairo_set_source_rgba(
        cr,
        pdf_view->color_selection.red / 65536.0,
        pdf_view->color_selection.green / 65536.0,
        pdf_view->color_selection.blue / 65536.0,
        0.125
    );

    cairo_fill_preserve( cr );

    cairo_set_source_rgba(
        cr,
        pdf_view->color_selection.red / 65536.0,
        pdf_view->color_selection.green / 65536.0,
        pdf_view->color_selection.blue / 65536.0,
        0.5
    );

    cairo_stroke( cr );

    int i;
    int x[] = { x1, x2, x2, x1 };
    int y[] = { y1, y1, y2, y2 };

    for ( i = 0; i < 4; i++ ) {
        cairo_arc_negative(
            cr,
            x[ i ],
            y[ i ],
            SEL_HANDLE_SIZE,
            i * M_PI_2,
            ( i + 1 ) * M_PI_2
        );

        cairo_set_source_rgba(
            cr,
            pdf_view->color_selection.red / 65536.0,
            pdf_view->color_selection.green / 65536.0,
            pdf_view->color_selection.blue / 65536.0,
            0.25
        );

        cairo_fill_preserve( cr );

        cairo_set_source_rgba(
            cr,
            pdf_view->color_selection.red / 65536.0,
            pdf_view->color_selection.green / 65536.0,
            pdf_view->color_selection.blue / 65536.0,
            0.5
        );

        cairo_stroke( cr );
    }

    return 0;
}

static int pdfview_draw_page( page_t* page, cairo_surface_t* surface, cairo_t* cr, int row_height, int x, int y ) {
    /* Vertically align to center */

    y += ( row_height - page->current_height ) / 2;

    /* The rect of the current page */

    cairo_set_line_width( cr, PAGE_BORDER );
    cairo_set_source_rgb( cr, 0, 0, 0 );
    cairo_rectangle(
        cr,
        x,
        y,
        page->current_width + PAGE_BORDER + 1,
        page->current_height + PAGE_BORDER + 1
    );
    cairo_stroke( cr );

    /* Draw the PDF page to the widget */

    if ( surface != NULL ) {
        cairo_set_source_surface( cr, surface, x + PAGE_BORDER, y + PAGE_BORDER );
        cairo_paint( cr );
    }

    return 0;
}

static void pdfview_draw_row( GtkPdfView* pdf_view, int row, cairo_t* cr, int width, int height, int* y ) {
    int row_width;
    int row_height;
    int cols;

    document_row_get_geometry( pdf_view->document, row, pdf_view->cols, &row_width, &row_height );

    if ( ( *y + row_height ) < 0 ) {
        goto out;
    }

    cols = MIN( pdf_view->cols, document_get_page_count( pdf_view->document ) - row * pdf_view->cols );

    row_width += ( cols - 1 ) * PAGE_GAP + cols * PAGE_BORDER;

    int x = MAX( 0, ( width - row_width ) / 2 ) - pdf_view->h_offset;
    int col = 0;
    int page_index = row * pdf_view->cols;

    while ( col < cols ) {
        page_t* page = document_get_page( pdf_view->document, page_index);

        cairo_surface_t* surface = document_get_page_surface( pdf_view->document, page_index );

        pdfview_draw_page( page, surface, cr, row_height, x, *y );

        if ( surface != NULL ) {
            cairo_surface_destroy( surface );
        }

        if ( pdf_view->highlight_mode && page_index == pdf_view->highlight_page_index ) {
            pdfview_draw_highlight( pdf_view, cr, page, x, *y );
        }

        x += page->current_width + PAGE_GAP + 2 * PAGE_BORDER;

        ++col;
        ++page_index;
    }

 out:
    *y += row_height;
}

static gboolean pdfview_expose( GtkWidget* widget, GdkEventExpose* event ) {
    int width;
    int height;
    cairo_t* cr;
    GdkWindow* window;
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );
    g_return_val_if_fail( event != NULL, FALSE );

    window = widget->window;
    pdf_view = GTK_PDFVIEW( widget );

    gdk_window_get_geometry( window, NULL, NULL, &width, &height, NULL );

    cr = gdk_cairo_create( widget->window );

    /* Fill the background */

    if ( pdf_view->fullscreen_mode ) {
        cairo_set_source_rgb(
            cr,
            0,
            0,
            0
        );
    } else {
        cairo_set_source_rgb(
            cr,
            widget->style->bg[ 0 ].red / 65536.0,
            widget->style->bg[ 0 ].green / 65536.0,
            widget->style->bg[ 0 ].blue / 65536.0
        );
    }

    cairo_rectangle( cr, 0, 0, width, height );
    cairo_fill( cr );

    if ( pdf_view->reloading ) {
        goto out;
    }

    /* Draw the pages */

    int y = -pdf_view->v_offset;

    if ( pdf_view->display_type == CONTINUOUS ) {
        int row;
        int row_count = document_get_row_count( pdf_view->document, pdf_view->cols );

        for ( row = 0; row < row_count; row++ ) {
            pdfview_draw_row( pdf_view, row, cr, width, height, &y );

            y += PAGE_GAP + 2 * PAGE_BORDER;

            if ( y > height ) {
                break;
            }
        }
    } else {
        pdfview_draw_row( pdf_view, pdf_view->current_page / pdf_view->cols, cr, width, height, &y );
    }

    /* Draw selection */

    if ( ( pdf_view->selection_mode ) && ( pdf_view->selection_active ) ) {
        pdfview_draw_selection( pdf_view, cr );
    }

 out:
    cairo_destroy( cr );

    return TRUE;
}

static gboolean pdfview_scroll( GtkWidget* widget, GdkEventScroll* event ) {
    GtkPdfView* pdf_view;
    pdf_tab_t* pdf_tab;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    pdf_view = GTK_PDFVIEW( widget );

    pdf_tab = document_storage_get_tab_by_pdfview( widget );

    switch ( event->direction ) {
        case GDK_SCROLL_UP :
            doctab_v_adjustment_add_value( pdf_tab, -STEP_INCREMENT );
            break;

        case GDK_SCROLL_DOWN :
            doctab_v_adjustment_add_value( pdf_tab, STEP_INCREMENT );
            break;

        case GDK_SCROLL_LEFT :
            doctab_h_adjustment_add_value( pdf_tab, -STEP_INCREMENT );
            break;

        case GDK_SCROLL_RIGHT :
            doctab_h_adjustment_add_value( pdf_tab, STEP_INCREMENT );
            break;
    }

    gtk_pdfview_draw( widget, NULL );

    return FALSE;
}

void pdfview_goto_page( GtkPdfView* pdf_view, int page_index, double valign, int relative, int add_to_hist ) {
    pdf_tab_t* pdf_tab;
    int current_row;
    int widget_width;
    int widget_height;
    int row_width;
    int row_height;
    int v_offset = 0;
    int i;

    if ( relative ) {
        current_row = pdf_view->current_page / pdf_view->cols + page_index;
    } else {
        current_row = page_index / pdf_view->cols;
    }

    FORCE_MIN_MAX( current_row, 0, document_get_row_count( pdf_view->document, pdf_view->cols ) - 1 );

    if ( pdf_view->display_type == SINGLE_PAGE ) {
        pdf_view->current_page = current_row * pdf_view->cols;
    } else {
        for ( i = 0; i < current_row; i++ ) {
            document_row_get_geometry( pdf_view->document, i, pdf_view->cols, &row_width, &row_height );
            v_offset += row_height + PAGE_GAP + 2 * PAGE_BORDER;
        }
    }

    gdk_window_get_geometry( GTK_WIDGET( pdf_view )->window, NULL, NULL, &widget_width, &widget_height, NULL );
    document_row_get_geometry( pdf_view->document, current_row, pdf_view->cols, &row_width, &row_height );

    v_offset -= valign * ( widget_height - row_height - 2 * PAGE_BORDER);

    if ( add_to_hist ) {
        goto_history_t* entry;

        entry = ( goto_history_t* )malloc( sizeof( goto_history_t ) );

        if ( entry != NULL ) {
            entry->page_number = current_row * pdf_view->cols;
            entry->valign = valign;

            while ( g_list_length( pdf_view->goto_list ) > ( pdf_view->goto_position + 1 ) ) {
                goto_history_t* tmp;

                tmp = g_list_last( pdf_view->goto_list )->data;

                pdf_view->goto_list = g_list_remove( pdf_view->goto_list, tmp );

                free( tmp );
            }

            pdf_view->goto_list = g_list_append( pdf_view->goto_list, entry );
            pdf_view->goto_position++;

            g_return_if_fail( pdf_view->goto_position == g_list_length( pdf_view->goto_list ) - 1 );
        }
    }

    pdf_tab = document_storage_get_tab_by_pdfview( GTK_WIDGET( pdf_view ) );

    doctab_v_adjustment_set_value( pdf_tab, v_offset );

    pdfview_update_auto_zoom( pdf_view );

    do_update_render_region( pdf_tab, pdf_view->document, TRUE );
    do_update_current_page( pdf_tab );

    gtk_pdfview_draw( GTK_WIDGET( pdf_view ), NULL );
}

static gboolean pdfview_key_press( GtkWidget* widget, GdkEventKey* event ) {
    GtkPdfView* pdf_view;
    pdf_tab_t* pdf_tab;
    int height;

    pdf_view = GTK_PDFVIEW( widget );

    pdf_tab = document_storage_get_tab_by_pdfview( widget );

    switch ( event->keyval ) {
        case GDK_Page_Up :
            if ( ( event->state & GDK_CONTROL_MASK ) == 0 ) {
                gdk_window_get_geometry( widget->window, NULL, NULL, NULL, &height, NULL );
                doctab_v_adjustment_add_value(
                    pdf_tab,
                    MIN( -1, STEP_INCREMENT - height )
                );
            }

            break;

        case GDK_Page_Down :
            if ( ( event->state & GDK_CONTROL_MASK ) == 0 ) {
                gdk_window_get_geometry( widget->window, NULL, NULL, NULL, &height, NULL );
                doctab_v_adjustment_add_value(
                    pdf_tab,
                    MAX( 1, height - STEP_INCREMENT )
                );
            }

            break;

        case GDK_Up :
            doctab_v_adjustment_add_value( pdf_tab, -STEP_INCREMENT );
            break;

        case GDK_Down :
            doctab_v_adjustment_add_value( pdf_tab, STEP_INCREMENT );
            break;

        case GDK_Left :
            doctab_h_adjustment_add_value( pdf_tab, -STEP_INCREMENT );
            break;

        case GDK_Right :
            doctab_h_adjustment_add_value( pdf_tab, STEP_INCREMENT );
            break;

        case GDK_space :
            pdfview_goto_page( pdf_view, 1, 0.0, TRUE, FALSE );
            break;

        case GDK_BackSpace :
            pdfview_goto_page( pdf_view, -1, 0.0, TRUE, FALSE );
            break;

        case GDK_Home :
            if ( event->state & GDK_CONTROL_MASK ) {
                pdfview_goto_page( pdf_view, 0, 0.0, FALSE, FALSE );
            } else {
                pdfview_goto_page( pdf_view, 0, 0.0, TRUE, FALSE );
            }

            break;

        case GDK_End :
            if ( event->state & GDK_CONTROL_MASK ) {
                pdfview_goto_page( pdf_view, document_get_page_count( pdf_view->document ) - 1, 1.0, FALSE, FALSE );
            } else {
                pdfview_goto_page( pdf_view, 0, 1.0, TRUE, FALSE );
            }

            break;

        case GDK_f :
            if ( event->state & GDK_CONTROL_MASK ) {
                do_doctab_show_search_bar( widget );
            }

            break;

        case GDK_Escape :
            if ( pdf_view->selection_mode && pdf_view->selection_active ) {
                pdf_view->selection_active = 0;
            }

            if ( pdf_view->pick_page ) {
                pdf_view->pick_page = FALSE;
                pdf_view->pick_canceled_callback();

                gdk_window_set_cursor( widget->window, NULL );
            } else {
                do_doctab_hide_search_bar( widget );
            }

            if ( pdf_view->fullscreen_mode ) {
                main_window_set_fullscreen_mode( FALSE );
            }

            break;

        case GDK_F11 :
            main_window_set_fullscreen_mode( !pdf_view->fullscreen_mode );
            break;

        default :
            return FALSE;
    }

    gtk_pdfview_draw( widget, NULL );

    return TRUE;
}

static int pdfview_goto_dest( int page_num, void* data ) {
    GtkPdfView* pdf_view;

    if ( data == NULL ) {
        return -1;
    }

    pdf_view = ( GtkPdfView* )data;

    pdfview_goto_page( pdf_view, page_num, 0.0, FALSE, TRUE );

    return 0;
}

static gboolean pdfview_find_link( GtkWidget* widget, int screen_x, int screen_y, int follow ) {
    GtkPdfView* pdf_view;
    int page_num = -1;
    int x;
    int y;
    page_t* page;
    GList* node;
    int error;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    pdf_view = GTK_PDFVIEW( widget );

    error = pdfview_screen_to_page(
        pdf_view,
        screen_x,
        screen_y,
        &page_num,
        &x,
        &y
    );

    if ( error < 0 ) {
        return FALSE;
    }

    page = document_get_page( pdf_view->document, page_num );

    if ( page == NULL ) {
        return FALSE;
    }

    for ( node = page->link_mapping; node; node = node->next ) {
        PopplerLinkMapping* link = ( PopplerLinkMapping* )node->data;

        if ( ( x >= link->area.x1 ) &&
             ( x <= link->area.x2 ) &&
             ( y >= link->area.y1 ) &&
             ( y <= link->area.y2 ) ) {
            ref_t ref;

            if ( follow ) {
                ref.goto_dest = pdfview_goto_dest;
                ref.goto_uri = follow_uri;
                ref.goto_dest_data = pdf_view;
            }

            ref_parse_from_action( &ref, pdf_view->document->doc, link->action, follow );

            return TRUE;
        }
    }

    return FALSE;
}

static gboolean pdfview_button_press( GtkWidget* widget, GdkEventButton* event ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    gtk_widget_grab_focus( widget );

    pdf_view = GTK_PDFVIEW( widget );

    if ( event->button == 3 ) {
        if ( pdf_view->selection_mode && pdfview_sel_contains( pdf_view, event->x, event->y ) ) {
            popup_selection_menu( event->button, event->time );
        } else if ( pdf_view->fullscreen_mode ) {
            popup_fullscreen_menu( event->button, event->time );
        }

        return TRUE;
    }

    if ( event->button != 1 ) {
        return FALSE;
    }

    if ( pdf_view->button_pressed ) {
        return FALSE;
    }

    if ( pdf_view->pick_page ) {
        int page_index;
        int page_x;
        int page_y;
        int error;

        error = pdfview_screen_to_page( pdf_view, event->x, event->y, &page_index, &page_x, &page_y );

        if ( error == 0 ) {
            pdf_view->pick_callback( page_index );
            pdf_view->pick_page = FALSE;
        }

        return FALSE;
    }

    pdf_view->button_x = event->x;
    pdf_view->button_y = event->y;

    if ( pdf_view->selection_mode ) {
        if ( !pdf_view->selection_active ) {
            /* initialize selection rectangle */

            pdf_view->sel_x1 = event->x;
            pdf_view->sel_y1 = event->y;
            pdf_view->sel_x2 = event->x;
            pdf_view->sel_y2 = event->y;

            pdf_view->button_sel_x = &pdf_view->sel_x1;
            pdf_view->button_sel_y = &pdf_view->sel_y1;

            pdf_view->selection_active = 1;
        } else {
            /* selection exists, check if corner is hit */

            pdf_view->selection_drag = 0;

            if ( ABS( event->x - pdf_view->sel_x1 ) <= SEL_HANDLE_SIZE ) {
                pdf_view->button_sel_x = &pdf_view->sel_x1;
            } else if ( ABS( event->x - pdf_view->sel_x2 ) <= SEL_HANDLE_SIZE ) {
                pdf_view->button_sel_x = &pdf_view->sel_x2;
            } else {
                pdf_view->button_sel_x = NULL;
            }

            if ( ABS( event->y - pdf_view->sel_y1 ) <= SEL_HANDLE_SIZE ) {
                pdf_view->button_sel_y = &pdf_view->sel_y1;
            } else if ( ABS( event->y - pdf_view->sel_y2 ) <= SEL_HANDLE_SIZE ) {
                pdf_view->button_sel_y = &pdf_view->sel_y2;
            } else {
                pdf_view->button_sel_y = NULL;
            }

            if ( pdf_view->button_sel_x == NULL || pdf_view->button_sel_y == NULL ) {
                /* none of the corners were hit; either drag selection or ignore */

                if ( pdfview_sel_contains( pdf_view, event->x, event->y ) ) {
                    pdf_view->selection_drag = 1;
                    gdk_window_set_cursor( widget->window, gdk_cursor_new( GDK_FLEUR ) );
                }
            }
        }
    } else {
        pdfview_find_link( widget, event->x, event->y, TRUE );
        gdk_window_set_cursor( widget->window, NULL );
    }

    pdf_view->button_pressed = 1;

    gtk_pdfview_draw( widget, NULL );

    return FALSE;
}

static gboolean pdfview_button_release( GtkWidget* widget, GdkEventButton* event ) {
    GtkPdfView* pdf_view;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    pdf_view = GTK_PDFVIEW( widget );

    if ( !pdf_view->pick_page && ( !pdf_view->selection_mode || pdf_view->selection_drag ) ) {
        gdk_window_set_cursor( widget->window, NULL );
    }

    pdf_view->selection_drag = 0;
    pdf_view->button_pressed = 0;

    return FALSE;
}

static gboolean pdfview_hide_cursor( gpointer data ) {
    GtkWidget* widget = ( GtkWidget* )data;

    gdk_window_set_cursor( widget->window, get_invisible_cursor() );

    return FALSE;
}

static void pdfview_cancel_hide_cursor( GtkPdfView* pdf_view ) {
    if ( pdf_view->fullscreen_timeout_tag != 0 ) {
        g_source_remove( pdf_view->fullscreen_timeout_tag );
        pdf_view->fullscreen_timeout_tag = 0;
    }
}

static void pdfview_schedule_hide_cursor( GtkPdfView* pdf_view ) {
    int timeout;
    int hide_pointer;

    if ( ( settings_get_int( "hide-pointer", &hide_pointer ) != 0 ) || ( !hide_pointer ) ) {
        return;
    }

    pdfview_cancel_hide_cursor( pdf_view );

    if ( settings_get_int( "pointer-idle-time", &timeout ) < 0 ) {
        return;
    }

    pdf_view->fullscreen_timeout_tag = g_timeout_add(
        ( timeout == 0 ) ? 100 : ( timeout * 1000 ),
        pdfview_hide_cursor,
        GTK_WIDGET( pdf_view )
    );
}

static void pdfview_update_cursor( GtkWidget* widget, int event_x, int event_y) {
    GtkPdfView* pdf_view = GTK_PDFVIEW( widget );

    if ( pdf_view->pick_page ) {
        return;
    }

    if ( pdf_view->selection_mode ) {
        if ( !pdf_view->selection_drag ) {
            int x1;
            int x2;
            int y1;
            int y2;

            pdfview_get_sel_rect( pdf_view, &x1, &y1, &x2, &y2 );

            int i;
            int x[] = { x1, x2, x2, x1 };
            int y[] = { y1, y1, y2, y2 };

            GdkCursorType c[] = {
                GDK_TOP_LEFT_CORNER,
                GDK_TOP_RIGHT_CORNER,
                GDK_BOTTOM_RIGHT_CORNER,
                GDK_BOTTOM_LEFT_CORNER
            };

            for ( i = 0; i < 4; i++ ) {
                if ( ( ABS( event_x - x[ i ] ) < SEL_HANDLE_SIZE ) &&
                     ( ABS( event_y - y[ i ] ) < SEL_HANDLE_SIZE ) ) {
                    gdk_window_set_cursor( widget->window, gdk_cursor_new( c[ i ] ) );
                    return;
                }
            }

            gdk_window_set_cursor( widget->window, NULL );
        }
    } else if ( !pdf_view->button_pressed ) {
        if ( pdfview_find_link( widget, event_x, event_y, FALSE ) ) {
            gdk_window_set_cursor( widget->window, gdk_cursor_new( GDK_HAND2 ) );
        } else {
            gdk_window_set_cursor( widget->window, NULL );
        }
    }

    if ( pdf_view->fullscreen_mode ) {
        pdfview_schedule_hide_cursor( pdf_view );
    }
}

void pdfview_set_fullscreen_mode( GtkPdfView* pdf_view, gboolean enabled ) {
    pdf_view->fullscreen_mode = enabled;

    if ( enabled ) {
        pdfview_schedule_hide_cursor( pdf_view );
    } else {
        pdfview_cancel_hide_cursor( pdf_view );
        gdk_window_set_cursor( GTK_WIDGET( pdf_view )->window, NULL );
    }
}

static gboolean pdfview_motion_notify( GtkWidget* widget, GdkEventMotion* event ) {
    GtkPdfView* pdf_view;
    pdf_tab_t* pdf_tab;
    int delta_x;
    int  delta_y;

    g_return_val_if_fail( widget != NULL, FALSE );
    g_return_val_if_fail( GTK_IS_PDFVIEW( widget ), FALSE );

    pdf_view = GTK_PDFVIEW( widget );

    pdfview_update_cursor( widget, event->x, event->y );

    if ( !pdf_view->button_pressed ) {
        return FALSE;
    }

    delta_x = ( int )( pdf_view->button_x - event->x );
    delta_y = ( int )( pdf_view->button_y - event->y );

    pdf_view->button_x = event->x;
    pdf_view->button_y = event->y;

    if ( pdf_view->selection_mode ) {
        GdkRectangle rect_old;
        GdkRectangle rect_new;
        GdkRectangle rect_union;
        int width;
        int height;

        gdk_window_get_geometry( widget->window, NULL, NULL, &width, &height, NULL );

        pdfview_get_sel_gdk_rect( pdf_view, &rect_old );

        if ( pdf_view->button_sel_x != NULL && pdf_view->button_sel_y != NULL ) {
            /* resize selection */

            *( pdf_view->button_sel_x ) = event->x;
            *( pdf_view->button_sel_y ) = event->y;

            FORCE_MIN_MAX( pdf_view->sel_x1, 0, width - 1 );
            FORCE_MIN_MAX( pdf_view->sel_x2, 0, width - 1 );
            FORCE_MIN_MAX( pdf_view->sel_y1, 0, height - 1 );
            FORCE_MIN_MAX( pdf_view->sel_y2, 0, height - 1 );
        } else if ( pdf_view->selection_drag ) {
            /* drag selection */

            if ( MIN( pdf_view->sel_x1, pdf_view->sel_x2 ) - delta_x < 0 ) {
                delta_x = MIN( pdf_view->sel_x1, pdf_view->sel_x2 );
            }

            if ( MAX( pdf_view->sel_x1, pdf_view->sel_x2 ) - delta_x >= width ) {
                delta_x = MAX( pdf_view->sel_x1, pdf_view->sel_x2 ) - width + 1;
            }

            if ( MIN( pdf_view->sel_y1, pdf_view->sel_y2 ) - delta_y < 0 ) {
                delta_y = MIN( pdf_view->sel_y1, pdf_view->sel_y2 );
            }

            if ( MAX( pdf_view->sel_y1, pdf_view->sel_y2 ) - delta_y >= height ) {
                delta_y = MAX( pdf_view->sel_y1, pdf_view->sel_y2 ) - height + 1;
            }

            pdf_view->sel_x1 -= delta_x;
            pdf_view->sel_x2 -= delta_x;
            pdf_view->sel_y1 -= delta_y;
            pdf_view->sel_y2 -= delta_y;
        } else {
            goto scroll;
        }

        /* redraw selection area */

        pdfview_get_sel_gdk_rect( pdf_view, &rect_new );
        gdk_rectangle_union( &rect_old, &rect_new, &rect_union );

        gtk_pdfview_draw( widget, &rect_union );

        return FALSE;
    }

 scroll:
    pdf_tab = document_storage_get_tab_by_pdfview( widget );

    doctab_v_adjustment_add_value( pdf_tab, delta_y );
    doctab_h_adjustment_add_value( pdf_tab, delta_x );

    gtk_pdfview_draw( widget, NULL );

    return FALSE;
}

static void pdfview_destroy( GtkObject* object ) {
    GtkPdfView* pdfview;
    GtkPdfViewClass* class;

    g_return_if_fail( object != NULL );
    g_return_if_fail( GTK_IS_PDFVIEW( object ) );

    pdfview = GTK_PDFVIEW( object );
    class = gtk_type_class( gtk_widget_get_type() );

    if ( GTK_OBJECT_CLASS( class )->destroy ) {
        ( *GTK_OBJECT_CLASS( class )->destroy )( object );
    }
}

static void pdfview_set_property( GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec ) {
    GtkPdfView* pdf_view = GTK_PDFVIEW( object );

    switch ( prop_id ) {
        case PROP_DOCUMENT :
            pdf_view->document = ( document_t* )g_value_get_pointer( value );
            break;

        default :
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void pdfview_get_property( GObject* object, guint prop_id, GValue* value, GParamSpec* pspec ) {
    GtkPdfView* pdf_view = GTK_PDFVIEW( object );

    switch ( prop_id ) {
        case PROP_DOCUMENT :
            g_value_set_pointer( value, pdf_view->document );
            break;

        default :
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

void pdfview_class_init( GtkPdfViewClass* class ) {
    GObjectClass* gobject_class;
    GtkWidgetClass* widget_class;
    GtkObjectClass* object_class;

    widget_class = ( GtkWidgetClass* )class;
    object_class = ( GtkObjectClass* )class;
    gobject_class = G_OBJECT_CLASS( class );

    widget_class->realize = pdfview_realize;
    widget_class->size_request = pdfview_size_request;
    widget_class->size_allocate = pdfview_size_allocate;
    widget_class->expose_event = pdfview_expose;
    widget_class->scroll_event = pdfview_scroll;
    widget_class->key_press_event = pdfview_key_press;
    widget_class->button_press_event = pdfview_button_press;
    widget_class->button_release_event = pdfview_button_release;
    widget_class->motion_notify_event = pdfview_motion_notify;

    object_class->destroy = pdfview_destroy;

    gobject_class->set_property = pdfview_set_property;
    gobject_class->get_property = pdfview_get_property;

    g_object_class_install_property(
        gobject_class,
        PROP_DOCUMENT,
        g_param_spec_pointer(
            "document",
            "document",
            "document",
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READABLE | G_PARAM_WRITABLE
        )
    );
}

void pdfview_init( GtkPdfView* pdf_view ) {
    pdf_view->goto_position = -1;
    pdf_view->goto_list = NULL;

    pdf_view->zoom_type = ZOOM_FREE;
    pdf_view->display_type = CONTINUOUS;
    pdf_view->cols = 1;

    pdf_view->fullscreen_mode = FALSE;

    settings_get_color( "color-selection", &pdf_view->color_selection );
    settings_get_color( "color-highlight", &pdf_view->color_highlight );
}
