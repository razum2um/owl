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

#ifndef _GUI_PDF_VIEW_H_
#define _GUI_PDF_VIEW_H_

#include <gtk/gtk.h>
#include <poppler.h>

#include <engine/document.h>

#define STEP_INCREMENT 45
#define PAGE_INCREMENT (4 * STEP_INCREMENT)

G_BEGIN_DECLS

#define GTK_PDFVIEW(obj) GTK_CHECK_CAST(obj, gtk_pdfview_get_type(), GtkPdfView)
#define GTK_PDFVIEW_CLASS(class) GTK_CHECK_CLASS_CAST(class, gtk_pdfview_get_type(), GtkPdfViewClass)
#define GTK_IS_PDFVIEW(obj) GTK_CHECK_TYPE(obj, gtk_pdfview_get_type())

typedef struct _GtkPdfView GtkPdfView;
typedef struct _GtkPdfViewClass GtkPdfViewClass;

typedef enum {
    ZOOM_FREE,
    ZOOM_FIT_PAGE,
    ZOOM_FIT_WIDTH
} zoom_t;

typedef enum {
    CONTINUOUS,
    SINGLE_PAGE
} display_t;

typedef struct goto_history {
    int page_number;
    double valign;
} goto_history_t;

struct _GtkPdfView {
    GtkWidget widget;

    document_t* document;

    gdouble button_x;
    gdouble button_y;
    int button_pressed;

    gboolean size_allocated;
    gboolean reloading;
    gboolean fullscreen_mode;
    gint fullscreen_timeout_tag;

    gdouble sel_x1;
    gdouble sel_y1;
    gdouble sel_x2;
    gdouble sel_y2;
    gdouble* button_sel_x;
    gdouble* button_sel_y;

    gboolean selection_mode;
    gboolean selection_active;
    gboolean selection_drag;

    gboolean highlight_mode;
    int highlight_page_index;
    PopplerRectangle highlight_rect;

    GdkColor color_selection;
    GdkColor color_highlight;

    gboolean pick_page;
    void ( *pick_callback )( int page_index );
    void ( *pick_canceled_callback )( void );

    int current_page;

    int cols;
    int v_offset;
    int h_offset;

    zoom_t zoom_type;
    display_t display_type;

    int goto_position;
    GList* goto_list;
};

struct _GtkPdfViewClass {
    GtkWidgetClass parent_class;
};

GtkType gtk_pdfview_get_type( void );

GtkWidget* gtk_pdfview_new( document_t* doc );

void gtk_pdfview_goto_page( GtkWidget* widget, int page_index, double valign, int relative, int add_to_hist );
void gtk_pdfview_set_reloading( GtkWidget* widget , gboolean reloading );

void gtk_pdfview_history_back( GtkWidget* widget );
void gtk_pdfview_history_forward( GtkWidget* widget );

int gtk_pdfview_get_current_page( GtkWidget* widget );
gboolean gtk_pdfview_get_selection_mode( GtkWidget* widget );
void gtk_pdfview_get_sel_rect( GtkWidget* widget, int* x1, int* y1, int* x2, int* y2 );
void gtk_pdfview_get_sel_gdk_rect( GtkWidget* widget, GdkRectangle* rect );

void gtk_pdfview_set_selection_mode( GtkWidget* widget, gboolean selection_mode );

int gtk_pdfview_screen_to_page( GtkWidget* widget, int screen_x, int screen_y,
                                int* page_index, int* page_x, int* page_y );

void gtk_pdfview_set_highlight( GtkWidget* widget, int page_num, PopplerRectangle* rect );
void gtk_pdfview_clear_highlight( GtkWidget* widget );

gboolean gtk_pdfview_get_fullscreen_mode( GtkWidget* widget );
void gtk_pdfview_set_fullscreen_mode( GtkWidget* widget, gboolean enabled );

void gtk_pdfview_set_v_offset( GtkWidget* widget, int offset );
void gtk_pdfview_set_h_offset( GtkWidget* widget, int offset );
int gtk_pdfview_get_cols( GtkWidget* widget );

void gtk_pdfview_get_geometry( GtkWidget* widget, int* width, int* height );
void gtk_pdfview_row_get_original_geometry( GtkWidget* widget, int row, int* width, int* height );
void gtk_pdfview_current_row_get_geometry( GtkWidget* widget, int* width, int* height );

void gtk_pdfview_set_multipage( GtkWidget* widget, int cols );

zoom_t gtk_pdfview_get_zoom_type( GtkWidget* widget );
void gtk_pdfview_set_zoom_type( GtkWidget* widget, zoom_t type );
void gtk_pdfview_update_auto_zoom( GtkWidget* widget );

display_t gtk_pdfview_get_display_type( GtkWidget* widget );
void gtk_pdfview_set_display_type( GtkWidget* widget, display_t type );

void gtk_pdfview_pick_page( GtkWidget* widget, void ( *cb )( int page_index ), void ( *canceled_cb )( void ) );

void gtk_pdfview_invalidate_selection( GtkWidget* widget );

gboolean gtk_pdfview_get_size_allocated( GtkWidget* widget );

void gtk_pdfview_draw( GtkWidget* widget, GdkRectangle* area );

void gtk_pdfview_reload_selection_color( GtkWidget* widget );
void gtk_pdfview_reload_highlight_color( GtkWidget* widget );


G_END_DECLS

#endif /* _GUI_PDF_VIEW_H_ */
