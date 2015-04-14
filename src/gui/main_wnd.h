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

#ifndef _GUI_MAIN_WND_H_
#define _GUI_MAIN_WND_H_

#include <gtk/gtk.h>

#include <gui/gui-utils.h>

enum {
    T_STYLE_IMAGE_AND_TEXT,
    T_STYLE_IMAGE,
    T_STYLE_TEXT
};

extern GtkWidget* main_window;
extern GtkWidget* main_notebook;
extern GtkWidget* toolbar;
extern GtkWidget* entry_zoom;
extern GtkWidget* button_rotate_left;
extern GtkWidget* button_rotate_right;
extern my_tool_btn_t* toolbar_selection;

extern GtkWidget* view_fullscreen_mode;
extern GtkWidget* view_cont_mode;

GtkWidget* create_tool_bar( void );
GtkWidget* create_menu_bar( void );

void toolbar_update_style( void );

int init_main_window( void );

void main_window_set_title( const char* title );

void main_window_next_tab( void );
void main_window_prev_tab( void );

void main_window_set_fullscreen_mode( gboolean enabled );

int main_window_attach_load_progress( void );
void main_window_detach_load_progress( int reference );

#endif /* _GUI_MAIN_WND_H_ */
