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

#ifndef _GUI_DOC_TAB_H_
#define _GUI_DOC_TAB_H_

#include <gtk/gtk.h>

#include <engine/document.h>

typedef struct pdf_tab {
    GtkWidget* root;
    GtkWidget* paned;
    GtkWidget* viewport;
    GtkWidget* toc_view;
    GtkWidget* pdf_view;
    GtkWidget* event_box;

    GtkWidget* search_bar;
    GtkWidget* entry_find;
    GtkWidget* v_sep_find;
    GtkWidget* image_find;
    GtkWidget* label_find;

    GtkWidget* navigator_bar;
    GtkObject* h_adjustment;
    GtkObject* v_adjustment;
    GtkWidget* h_scroll_bar;
    GtkWidget* v_scroll_bar;
    GtkWidget* entry_curr_pg;
    GtkWidget* entry_last_pg;
    GtkWidget* label_statusbar;

    int history_used;
    int cur_highlighted_page;
    guint statusbar_timer_tag;
} pdf_tab_t;

pdf_tab_t* create_pdf_tab( document_t* document );

typedef enum {
    SEARCH_STATUS_CLEAR,
    SEARCH_STATUS_BUSY,
    SEARCH_STATUS_WRAPPED,
    SEARCH_STATUS_NONE
} search_status_t;

void doctab_set_search_status( pdf_tab_t* pdf_tab, search_status_t status );

void doctab_set_statusbar( pdf_tab_t* pdf_tab, int timeout, const char* format, ... ) __attribute__(( __format__( __printf__, 3, 4 ) ));

void doctab_update_adjustment_limits( pdf_tab_t* pdf_tab );

void doctab_v_adjustment_add_value( pdf_tab_t* pdf_tab, int value );
int doctab_v_adjustment_get_value( pdf_tab_t* pdf_tab );
void doctab_v_adjustment_set_value( pdf_tab_t* pdf_tab, int value );
void doctab_v_adjustment_set_begin( pdf_tab_t* pdf_tab );
void doctab_v_adjustment_set_end( pdf_tab_t* pdf_tab );

int doctab_h_adjustment_get_value( pdf_tab_t* pdf_tab );
void doctab_h_adjustment_set_value( pdf_tab_t* pdf_tab, int value );
void doctab_h_adjustment_add_value( pdf_tab_t* pdf_tab, int value );

#endif /* _GUI_DOC_TAB_H_ */
