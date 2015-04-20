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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <cairo.h>
#include <inttypes.h>
#include <sys/time.h>

#include <utils.h>
#include <engine/render.h>
#include <engine/document.h>

static cairo_surface_t* do_render_page( page_t* page ) {
    double scale = page->scale / 100.0f;

    cairo_t* cr;
    cairo_surface_t* surface;

    surface = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24,
        page->current_width,
        page->current_height
    );

    if ( surface == NULL ) {
        goto error1;
    }

    cr = cairo_create( surface );

    if ( cr == NULL ) {
        goto error2;
    }

    /* Fill the background with WHITE */

    cairo_set_source_rgb( cr, 1, 1, 1 );
    cairo_rectangle( cr, 0, 0, page->current_width, page->current_height );
    cairo_fill( cr );

    /* Scaling */

    cairo_scale( cr, scale, scale );

    /* Rotation */

    switch ( page->rotate ) {
        case 0 :
            cairo_translate( cr, 0, 0 );
            break;

        case 90 :
            cairo_translate( cr, page->original_height, 0 );
            break;

        case 180 :
            cairo_translate( cr, page->original_width, page->original_height );
            break;

        case 270 :
            cairo_translate( cr, 0, page->original_width );
            break;
    }

    cairo_rotate( cr, ( double )page->rotate * M_PI / 180.0 );

    /* Let poppler render the page */

#ifdef _WIN32
#else
    poppler_lock();
    poppler_page_render( page->page, cr );
    poppler_unlock();
#endif /* _WIN32 */

    cairo_destroy( cr );

    return surface;

 error2:
    cairo_surface_destroy( surface );

 error1:
    return NULL;
}

static void* render_thread_entry( void* arg ) {
    render_engine_t* engine;

    engine = ( render_engine_t* )arg;

    while ( engine->running ) {
        page_t* page;
        int page_index;
        int lazy_rendering;
        render_command_t* cur_cmd;

        g_mutex_lock( &engine->lock );

        while ( engine->running &&
                engine->command_queue_head == NULL ) {
            g_cond_wait( &engine->queue_sync, &engine->lock );
        }

        if ( engine->command_queue_head != NULL ) {
            cur_cmd = engine->command_queue_head;
            engine->command_queue_head = cur_cmd->next;

            if ( engine->command_queue_head == NULL ) {
                engine->command_queue_tail = NULL;
            }
        } else {
            cur_cmd = NULL;
        }

        g_mutex_unlock( &engine->lock );

        if ( !engine->running ) {
            break;
        }

        assert( cur_cmd != NULL );

        page_index = cur_cmd->page_index;
        lazy_rendering = cur_cmd->lazy_rendering;

        free( cur_cmd );

        page = document_get_page( engine->doc, page_index );

        if ( page != NULL ) {
            cairo_surface_t* surface;

            if ( ( page->surf == NULL ) ||
                 ( !lazy_rendering ) ) {
                surface = do_render_page( page );

                if ( surface != NULL ) {
                    document_set_page_surface( engine->doc, page_index, surface );
                }

                if ( engine->listener != NULL ) {
                    engine->listener( engine->doc, page_index );
                }
            }
        }
    }

    return NULL;
}

render_engine_t* render_engine_new( document_t* doc ) {
    render_engine_t* engine;

    engine = ( render_engine_t* )malloc( sizeof( render_engine_t ) );

    if ( engine == NULL ) {
        return NULL;
    }

    memset( engine, 0, sizeof( render_engine_t ) );

    g_mutex_init(&engine->lock);
    g_cond_init(&engine->queue_sync);
    engine->doc = doc;

    return engine;
}

void render_engine_destroy( render_engine_t* engine ) {
    g_mutex_clear( &engine->lock );
    g_cond_clear( &engine->queue_sync );

    free( engine );
}

int render_engine_start( render_engine_t* engine ) {
    engine->running = TRUE;

    engine->render_thread = g_thread_new(
        "render",
        render_thread_entry,
        ( gpointer )engine
    );

    return 0;
}

int render_engine_stop( render_engine_t* engine, int wait ) {
    engine->running = FALSE;

    g_cond_signal( &engine->queue_sync );

    if ( ( wait ) &&
         ( engine->render_thread != NULL ) ) {
        g_thread_join( engine->render_thread );
    }

    return 0;
}

int render_engine_flush_queue( render_engine_t* engine ) {
    render_command_t* head;

    g_mutex_lock( &engine->lock );

    head = engine->command_queue_head;

    engine->command_queue_head = NULL;
    engine->command_queue_tail = NULL;

    g_mutex_unlock( &engine->lock );

    while ( head != NULL ) {
        render_command_t* cmd;

        cmd = head;
        head = cmd->next;

        free( cmd );
    }

    return 0;
}

int render_engine_queue_command( render_engine_t* engine, render_command_t* cmd ) {
    cmd->next = NULL;

    g_mutex_lock( &engine->lock );

    if ( engine->command_queue_head == NULL ) {
        engine->command_queue_head = cmd;
        engine->command_queue_tail = cmd;
    } else {
        engine->command_queue_tail->next = cmd;
        engine->command_queue_tail = cmd;
    }

    g_mutex_unlock( &engine->lock );
    g_cond_signal( &engine->queue_sync );

    return 0;
}

int render_engine_set_listener( render_engine_t* engine, page_render_done_t* listener ) {
    engine->listener = listener;

    return 0;
}

render_command_t* render_engine_create_command( int page_index, int lazy_rendering ) {
    render_command_t* cmd;

    cmd = ( render_command_t* )malloc( sizeof( render_command_t ) );

    if ( cmd == NULL ) {
        return NULL;
    }

    cmd->page_index = page_index;
    cmd->lazy_rendering = lazy_rendering;

    return cmd;
}

