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

#ifndef _ENGINE_REF_H_
#define _ENGINE_REF_H_

#include <gtk/gtk.h>
#include <poppler.h>

struct document;

typedef struct {
    int page_num;
    void* goto_dest_data;

    int ( *goto_dest )( int page_num, void* data );
    int ( *goto_uri )( char* uri );
} ref_t;

enum {
    REF_COL_TITLE = 0,
    REF_COL_PAGE_NUM,
    REF_COL_DATA,
    REF_COL_FONT,
    REF_COL_COUNT
};

typedef struct {
    GtkTreeStore* toc;
} ref_table_t;

ref_table_t* ref_table_new( struct document* document );
void ref_table_destroy( ref_table_t* ref_table );

void ref_parse_from_action( ref_t* ref, PopplerDocument* doc, PopplerAction* action, gboolean follow );

gboolean ref_table_search_equal_func( GtkTreeModel* model, gint column, const gchar* key,
                                      GtkTreeIter* iter, gpointer data );

void ref_table_highlight_page( ref_table_t* ref_table, GtkWidget* toc_view, GtkTreeIter* parent, int page_index, int highlight_on );

#endif /* _ENGINE_REF_H_ */
