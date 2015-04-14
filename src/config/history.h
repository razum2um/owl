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

#ifndef _CONFIG_HISTORY_H_
#define _CONFIG_HISTORY_H_

#include <glib.h>

typedef struct page_history {
    int scale;
    int rotate;
} page_history_t;

typedef struct document_history {
    char* path;
    int v_offset;
    int h_offset;
    int current_page;
    int zoom_type;
    int display_type;
    int paned_position;
    int recently_open;

    int rotate;
    int scale;

    GHashTable* page_table;
} document_history_t;

typedef void history_callback_t( void* key, void* value, void* data );

document_history_t* history_get_document_info( const char* path, int create );
int history_add_page_info( document_history_t* history, int page_index, int rotate, int scale );

int history_iterate( history_callback_t* callback, void* data );

int load_document_history( void );
int save_document_history( void );

int init_history( void );

#endif /* _CONFIG_HISTORY_H_ */
