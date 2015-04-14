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

#ifndef _GUI_GUI_UTILS_H_
#define _GUI_GUI_UTILS_H_

#include <gtk/gtk.h>

#include <gui/doc_tab.h>
#include <engine/document.h>

typedef struct my_tool_btn {
    GtkWidget* button;
    GtkWidget* image;
    GtkWidget* label;
} my_tool_btn_t;

my_tool_btn_t* create_my_tool_button( const gchar* stock_name, const gchar* title );
my_tool_btn_t* create_my_toggle_tool_button( const gchar* stock_name, const gchar* title );
GtkWidget* create_doc_tab_label( pdf_tab_t* pdf_tab, document_t* document );

GdkCursor* get_invisible_cursor( void );

GtkFileFilter* get_all_file_filter( void );
GtkFileFilter* get_pdf_file_filter( void );

#endif /* _GUI_GUI_UTILS_H_ */
