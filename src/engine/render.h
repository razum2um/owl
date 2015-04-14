/* Owl PDF viewer
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs, Peter Szilagyi
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

#ifndef _ENGINE_RENDER_H_
#define _ENGINE_RENDER_H_

#include <glib.h>

#define MAX_PAGE_COUNT 50

struct document;

typedef struct render_command {
    int page_index;
    int lazy_rendering;
    struct render_command* next;
} render_command_t;

typedef int page_render_done_t( struct document* doc, int page );

typedef struct render_engine {
    struct document* doc;

    volatile int running;
    GThread* render_thread;

    GMutex* lock;
    GCond* queue_sync;
    render_command_t* command_queue_head;
    render_command_t* command_queue_tail;

    page_render_done_t* listener;
} render_engine_t;

render_engine_t* render_engine_new( struct document* doc );
void render_engine_destroy( render_engine_t* engine );

int render_engine_start( render_engine_t* engine );
int render_engine_stop( render_engine_t* engine, int wait );

int render_engine_flush_queue( render_engine_t* engine );
int render_engine_queue_command( render_engine_t* engine, render_command_t* cmd );

int render_engine_set_listener( render_engine_t* engine, page_render_done_t* listener );

render_command_t* render_engine_create_command( int page_index, int lazy_rendering );

#endif /* _ENGINE_RENDER_H_ */
