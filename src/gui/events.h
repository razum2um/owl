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

#ifndef _GUI_EVENTS_H_
#define _GUI_EVENTS_H_

#include <gtk/gtk.h>

#include <gui/doc_tab.h>
#include <engine/document.h>

#define MIN_ZOOM 10
#define MAX_ZOOM 500

/* Event helper functions */

int get_current_document_info( pdf_tab_t** pdf_tab, document_t** document, int* page_index );

int do_open_document( const char* uri );

void do_save_document( document_t* document, char* uri );
void do_update_current_page( pdf_tab_t* pdf_tab );
void do_update_page_count( pdf_tab_t* pdf_tab, document_t* document );
void do_update_zoom_entry( pdf_tab_t* pdf_tab, document_t* document );
void do_update_render_region( pdf_tab_t* pdf_tab, document_t* document, int lazy_rendering );
void do_update_page_specific_menu_items( pdf_tab_t* pdf_tab );
void do_save_selection_as_text( pdf_tab_t* pdf_tab, document_t* document );
void do_save_selection_as_image( pdf_tab_t* pdf_tab, document_t* document, GdkPixbuf** pixbuf );

void do_zoom( pdf_tab_t* pdf_tab, document_t* document, int page_index, int amount );
void do_zoom_fit_width( void );
void do_zoom_fit_page( void );
void do_rotate( pdf_tab_t* pdf_tab, document_t* document, int page_index, int amount );

void do_close_document_tab( pdf_tab_t* pdf_tab, document_t* document, int page_index );

void do_doctab_find( pdf_tab_t* pdf_tab, find_dir_t direction );
void do_doctab_show_search_bar( GtkWidget* widget );
void do_doctab_hide_search_bar( GtkWidget* widget );

/* Menu events */

void event_menu_open_document( GtkMenuItem* widget, gpointer data );
void event_menu_save_as_document( GtkMenuItem* widget, gpointer data );
void event_menu_close_current_document( GtkMenuItem* widget, gpointer data );
void event_menu_close_all_document( GtkMenuItem* widget, gpointer data );
void event_menu_open_settings( GtkMenuItem* widget, gpointer data );
void event_menu_open_preferences( GtkMenuItem* widget, gpointer data );
void event_menu_print( GtkMenuItem* widget, gpointer data );
void event_menu_page_setup( GtkMenuItem* widget, gpointer data );

void event_menu_zoom_in( GtkMenuItem* widget, gpointer data );
void event_menu_zoom_out( GtkMenuItem* widget, gpointer data );
void event_menu_fit_width( GtkMenuItem* widget, gpointer data );
void event_menu_fit_page( GtkMenuItem* widget, gpointer data );
void event_menu_rotate_left( GtkMenuItem* widget, gpointer data );
void event_menu_rotate_right( GtkMenuItem* widget, gpointer data );
void event_menu_back_in_doc( GtkMenuItem* widget, gpointer data );
void event_menu_forward_in_doc( GtkMenuItem* widget, gpointer data );
void event_menu_prev_page( GtkMenuItem* widget, gpointer data );
void event_menu_next_page( GtkMenuItem* widget, gpointer data );
void event_menu_first_page( GtkMenuItem* widget, gpointer data );
void event_menu_last_page( GtkMenuItem* widget, gpointer data );
void event_menu_prev_doc( GtkMenuItem* widget, gpointer data );
void event_menu_next_doc( GtkMenuItem* widget, gpointer data );
void event_menu_reverse_pages( GtkMenuItem* widget, gpointer data );
void event_menu_set_multipage( GtkMenuItem* widget, gpointer data );
void event_menu_toggle_cont_mode( GtkCheckMenuItem* checkmenuitem, gpointer data );
void event_menu_set_fullscreen_mode( GtkCheckMenuItem* widget, gpointer data );

void event_menu_zoom_page_in( GtkMenuItem* widget, gpointer data );
void event_menu_zoom_page_out( GtkMenuItem* widget, gpointer data );
void event_menu_rotate_page_left( GtkMenuItem* widget, gpointer data );
void event_menu_rotate_page_right( GtkMenuItem* widget, gpointer data );

/* Document tab events */

void event_doctab_goto_page( GtkEntry* widget, gpointer data );
void event_doctab_find_next( GtkButton* button, gpointer data );
void event_doctab_find_prev( GtkButton* button, gpointer data );
void event_doctab_find_close( GtkButton* button, gpointer data );
gboolean event_doctab_find_keypress( GtkWidget* widget, GdkEventKey* event, gpointer data );
void event_doctab_find_text_changed( GtkEditable *editable, gpointer data );
gboolean event_doctab_toc_query_tooltip( GtkWidget* widget, gint x, gint y, gboolean keyboard_mode,
                                         GtkTooltip* tooltip, gpointer data );
void event_doctab_size_allocate( GtkWidget* widget, GtkAllocation* allocation, gpointer data );
gboolean event_doctab_header_click( GtkWidget* widget, GdkEventButton* event, gpointer data );

/* Other UI events */

void event_close_tab( GtkButton* button, gpointer data );
void event_close_this_tab( GtkButton* button, gpointer data );
void event_close_other_tabs( GtkButton* button, gpointer data );
void event_goto_prev_page( GtkButton* button, gpointer data );
void event_goto_next_page( GtkButton* button, gpointer data );
void event_goto_prev_page_toolbar( GtkToolButton* button, gpointer data );
void event_goto_next_page_toolbar( GtkToolButton* button, gpointer data );
gboolean event_zoom_in( GtkWidget* widget, GdkEventButton* event, gpointer data );
gboolean event_zoom_out( GtkWidget* widget, GdkEventButton* event, gpointer data );
gboolean event_set_zoom( GtkWidget* widget, GdkEventKey* event, gpointer data );
gboolean event_rotate_left( GtkWidget* widget, GdkEventButton* event, gpointer data );
gboolean event_rotate_right( GtkWidget* widget, GdkEventButton* event, gpointer data );
void event_selection_mode_toggled( GtkToggleButton* widget, gpointer data );
void event_scroll_v_adjustment_changed( GtkAdjustment* adjustment, gpointer data );
void event_scroll_h_adjustment_changed( GtkAdjustment* adjustment, gpointer data );
void event_current_page_changed( GtkWidget* widget );
gboolean event_current_doc_changed( GtkNotebook* notebook, gint arg1, gpointer data );
void event_doc_removed( GtkNotebook* notebook, GtkWidget* child, guint page_num, gpointer data );
void event_fileexit_activated( void );
void event_window_destroy( GtkWidget* widget, gpointer data );

int event_document_opened( document_t* doc, open_event_t event );
int event_page_render_done( document_t* doc, int page_index );

void event_save_selection_as_text( gpointer data );
void event_save_selection_as_image( gpointer data );

void event_toc_selection_changed( GtkTreeSelection * select, gpointer data );

void event_show_about( GtkWidget* widget, gpointer data );

/* Callbacks */

void do_rotate_page_left_cb( int page_index );
void do_rotate_page_right_cb( int page_index );
void do_zoom_page_in_cb( int page_index );
void do_zoom_page_out_cb( int page_index );
void do_pick_page_canceled_cb( void );

#endif /* _GUI_EVENTS_H_ */
