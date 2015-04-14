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

#ifndef _ENGINE_FIND_H_
#define _ENGINE_FIND_H_

#include <glib.h>
#include <poppler.h>

typedef enum {
    FIND_NONE,
    FIND_OK,
    FIND_OK_WRAPPED
} find_result_t;

typedef enum {
    FIND_FORWARD,
    FIND_BACK
} find_dir_t;

typedef enum {
    FIND_STATE_INIT,
    FIND_STATE_MATCH,
    FIND_STATE_SEARCH,
    FIND_STATE_NONE
} find_state_t;

struct document;
struct page;

typedef struct {
    struct document* doc;
    struct page* page;

    char* text;

    find_dir_t direction;

    int page_index;
    find_state_t state;
    int no_match_since;
    gboolean wrapped;

    GList* match_list;
    GList* match;

    volatile int running;
    int pending_request;

    GThread* find_thread;
    GMutex* find_lock;
    GCond* find_cond;
} find_context_t;

find_context_t* find_context_new( struct document* doc, int page_index, const char* text );

void find_push( find_context_t* fc, find_dir_t direction );
void find_stop( find_context_t* fc );

#endif /* _ENGINE_FIND_H_ */
