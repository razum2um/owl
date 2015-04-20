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

#ifndef _ENGINE_DOCUMENT_H_
#define _ENGINE_DOCUMENT_H_

#include <glib.h>
#include <poppler.h>

#include <engine/find.h>
#include <engine/ref.h>
#include <engine/render.h>

typedef enum open_event {
    DOC_OPEN_STARTED,
    DOC_OPEN_DONE_OK,
    DOC_OPEN_DONE_FAILED,
    DOC_OPEN_DONE_ENCRYPTED,
    DOC_RELOAD_STARTED,
    DOC_RELOAD_DONE_OK,
    DOC_RELOAD_DONE_FAILED
} open_event_t;

struct document;

typedef int open_listener_t( struct document* doc, open_event_t event );

typedef struct page {
    int scale;
    int rotate;

    int original_width;
    int original_height;
    int current_width;
    int current_height;

    cairo_surface_t* surf;

    PopplerPage* page;

    GList* link_mapping;
} page_t;

typedef struct document {
    /* Generic stuff */

    char* path;
    char* filename;
    char* password;
    void* private;

    /* Document open related members */

    GThread* open_thread;
    open_listener_t* open_listener;
    GMutex reload_lock;

    /* Pages :) */

    int page_count;
    page_t* page_table;
    GMutex page_lock;

    /* Poppler document stuffs */

    PopplerDocument* doc;

    /* Rendering related stuffs */

    render_engine_t* render_engine;

    /* Reference related stuffs */

    ref_table_t* ref_table;

    /* Search related stuff */

    find_context_t* fc;
    GHashTable* match_table;
} document_t;

document_t* document_new( const char* filename );
void document_destroy( document_t* document );

int document_start_open( document_t* document );
int document_do_reload( document_t* document );
int document_update_history( document_t* document, int current_page );
int document_reverse_pages( document_t* document );

page_t* document_get_page( document_t* document, int index );
int document_get_scale_for_page( document_t* document, int page_index );
int document_get_rotate_for_page( document_t* document, int page_index );
int document_get_memory_size_for_page( document_t* document, int page_index );
cairo_surface_t* document_get_page_surface( document_t* document, int index );
int document_get_page_count( document_t* document );
int document_get_row_count( document_t* document, int cols );
void document_get_geometry( document_t* document, int cols, int* width, int* height );
void document_row_get_geometry( document_t* document, int row, int cols, int* width, int* height );
void document_row_get_original_geometry( document_t* document, int row, int cols, int* width, int* height );
int document_get_common_rotate_and_scale( document_t* document, int* rotate, int* scale );

int document_set_page_surface( document_t* document, int index, cairo_surface_t* surface );
int document_set_rendered_region( document_t* document, int start_index, int end_index );
int document_set_scale( document_t* document, int scale );
int document_set_scale_for_page( document_t* document, int page_index, int scale );
int document_set_rotate( document_t* document, int rotate );
int document_set_rotate_for_page( document_t* document, int page_index, int rotate );
int document_set_open_listener( document_t* document, open_listener_t* listener );

#endif /* _ENGINE_DOCUMENT_H_ */
