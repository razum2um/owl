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

#include <stdlib.h>
#include <string.h>

#include <storage.h>
#include <utils.h>
#include <gui/doc_tab.h>
#include <gui/pdf_view.h>
#include <engine/find.h>
#include <engine/document.h>
#include <gui/doc_tab.h>
#include <gui/pdf_view.h>

static find_result_t find_next( find_context_t* fc );
static void find_context_destroy( find_context_t* fc );
static gpointer find_thread_entry( gpointer arg );
static void find_start( find_context_t* fc );
static find_result_t find_next( find_context_t* fc );

static void find_start( find_context_t* fc ) {
    fc->running = TRUE;

    fc->find_thread = g_thread_create(
        find_thread_entry,
        ( gpointer )fc,
        TRUE,
        NULL
    );
}

static gpointer find_thread_entry( gpointer arg ) {
    find_context_t* fc = ( find_context_t* )arg;
    find_result_t result;
    pdf_tab_t* pdf_tab;

    pdf_tab = document_storage_get_tab_by_doc( fc->doc );

    if ( pdf_tab == NULL ) {
        return NULL;
    }

    while ( fc->running ) {
        g_mutex_lock( &fc->find_lock );

        if ( !fc->pending_request ) {
            g_cond_wait( &fc->find_cond, &fc->find_lock );
        }

        fc->pending_request = 0;

        g_mutex_unlock( &fc->find_lock );

        if ( !fc->running ) {
            break;
        }

        result = find_next( fc );

        if ( !fc->running ) {
            break;
        }

        GDK_THREADS_ENTER();

        if ( result == FIND_NONE ) {
            doctab_set_search_status( pdf_tab, SEARCH_STATUS_NONE );
        } else {
            if ( result == FIND_OK ) {
                doctab_set_search_status( pdf_tab, SEARCH_STATUS_CLEAR );
            }

            if ( result == FIND_OK_WRAPPED ) {
                doctab_set_search_status( pdf_tab, SEARCH_STATUS_WRAPPED );
            }

            gtk_pdfview_set_highlight(
                pdf_tab->pdf_view,
                fc->page_index,
                ( PopplerRectangle* )fc->match->data
            );
        }

        GDK_THREADS_LEAVE();
    }

    find_context_destroy( fc );

    return NULL;
}

static find_result_t find_next( find_context_t* fc ) {
    switch ( fc->state ) {
        case FIND_STATE_INIT :
            fc->page = document_get_page( fc->doc, fc->page_index );

            {
                int size = strlen( fc->text ) + 32;
                char* key = ( char* )malloc( size );
                gpointer match;

                if ( key == NULL ) {
                    return FIND_NONE;
                }

                snprintf( key, size, "%p%s", fc->page->page, fc->text );

                if ( ( match = g_hash_table_lookup( fc->doc->match_table, key ) ) == fc->doc->match_table ) {
                    fc->match_list = NULL;
                    free( key );
                } else if ( match == NULL ) {
                    poppler_lock();
                    fc->match_list = poppler_page_find_text( fc->page->page, fc->text );
                    poppler_unlock();

                    if ( fc->match_list != NULL ) {
                        g_hash_table_insert( fc->doc->match_table, key, fc->match_list );
                    } else {
                        g_hash_table_insert( fc->doc->match_table, key, fc->doc->match_table );
                    }
                } else {
                    fc->match_list = ( GList* )match;
                    free( key );
                }
            }

            if ( fc->match_list != NULL ) {
                if ( fc->direction == FIND_FORWARD ) {
                    fc->match = fc->match_list;
                } else {
                    fc->match = g_list_last( fc->match_list );
                }

                fc->state = FIND_STATE_MATCH;

                if ( fc->wrapped ) {
                    fc->wrapped = FALSE;
                    return FIND_OK_WRAPPED;
                }

                return FIND_OK;
            }

            if ( ++fc->no_match_since == document_get_page_count( fc->doc ) ) {
                fc->state = FIND_STATE_NONE;
                return FIND_NONE;
            }

            fc->match = NULL;
            fc->state = FIND_STATE_SEARCH;

            return find_next( fc );

        case FIND_STATE_MATCH :
            fc->no_match_since = 0;
            fc->state = FIND_STATE_SEARCH;

            return find_next( fc );

        case FIND_STATE_SEARCH :
            if ( fc->match != NULL ) {
                if ( fc->direction == FIND_FORWARD ) {
                    fc->match = fc->match->next;
                } else {
                    fc->match = fc->match->prev;
                }

                if ( fc->match != NULL) {
                    fc->wrapped = FALSE;
                    fc->state = FIND_STATE_MATCH;

                    return FIND_OK;
                }
            }

            if ( fc->direction == FIND_FORWARD ) {
                if ( ++fc->page_index == document_get_page_count( fc->doc ) ) {
                    fc->page_index = 0;
                    fc->wrapped = TRUE;
                }
            } else {
                if ( --fc->page_index == -1 ) {
                    fc->page_index = document_get_page_count( fc->doc ) - 1;
                    fc->wrapped = TRUE;
                }
            }

            fc->state = FIND_STATE_INIT;

            return find_next( fc );

        case FIND_STATE_NONE :
            return FIND_NONE;
    }

    /* for compiler satisfaction */
    return FIND_NONE;
}

find_context_t* find_context_new( document_t* doc, int page_index, const char* text ) {
    find_context_t* fc;

    fc = ( find_context_t* )malloc( sizeof( find_context_t ) );

    if ( fc == NULL ) {
        goto error1;
    }

    memset( fc, 0, sizeof ( find_context_t ) );

    fc->doc = doc;
    fc->state = FIND_STATE_INIT;
    fc->page_index = page_index;
    fc->no_match_since = 0;

    fc->text = strdup( text );

    if ( fc->text == NULL ) {
        goto error2;
    }

    g_mutex_init(&fc->find_lock);
    g_cond_init(&fc->find_cond);

    find_start( fc );

    return fc;

 error2:
    free( fc );

 error1:
    return NULL;
}

static void find_context_destroy( find_context_t* fc ) {
    g_mutex_clear( &fc->find_lock );
    g_cond_clear( &fc->find_cond );

    free( fc->text );
    free( fc );
}

void find_push( find_context_t* fc, find_dir_t direction ) {
    g_mutex_lock( &fc->find_lock );

    fc->direction = direction;
    fc->pending_request = 1;

    g_mutex_unlock( &fc->find_lock );
    g_cond_signal( &fc->find_cond );
}

void find_stop( find_context_t* fc ) {
    fc->running = 0;

    g_cond_signal( &fc->find_cond );
}
