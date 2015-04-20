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
#include <errno.h>

#include <utils.h>
#include <storage.h>
#include <gui/doc_tab.h>
#include <gui/pdf_view.h>
#include <engine/document.h>
#include <config/history.h>

document_t* document_new( const char* filename ) {
    char* sep;
    document_t* doc;

    /* Create the document instance */

    doc = ( document_t* )malloc( sizeof( document_t ) );

    if ( doc == NULL ) {
        goto error1;
    }

    memset( doc, 0, sizeof( document_t ) );

    /* Split the given filename and path */

    if ( strncmp( filename, "file://", 7 ) == 0 ) {
        filename += 7;
    }

    sep = g_strrstr( filename, PATH_SEPARATOR );

    doc->path = g_strndup( filename, sep - filename );
    doc->filename = strdup( sep + 1 );

    if ( ( doc->path == NULL ) ||
         ( doc->filename == NULL ) ) {
        goto error2;
    }

    g_mutex_init(&doc->page_lock);
    g_mutex_init(&doc->reload_lock);

    doc->render_engine = render_engine_new( doc );

    if ( doc->render_engine == NULL ) {
        goto error2;
    }

    doc->match_table = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, NULL );

    if ( doc->match_table == NULL ) {
        goto error2;
    }

    return doc;

 error2:
    g_mutex_clear( &doc->reload_lock );
    g_mutex_clear( &doc->page_lock );
    free( doc->path );
    free( doc->filename );

 error1:
    return NULL;
}

static void document_destroy_page_table( page_t* page, int count ) {
    int i;

    for ( i = 0; i < count; i++, page++ ) {
        if ( page->surf != NULL ) {
            cairo_surface_destroy( page->surf );
            page->surf = NULL;
        }

        if ( page->link_mapping != NULL ) {
            poppler_page_free_link_mapping( page->link_mapping );
            page->link_mapping = NULL;
        }

        if ( page->page != NULL ) {
            g_object_unref( G_OBJECT( page->page ) );
            page->page = NULL;
        }
    }
}

static void document_destroy_match_table_values( gpointer key, gpointer value, gpointer user_data ) {
    if ( value == user_data ) {
        return;
    }

    GList* list = ( GList* )value;
    GList* item;

    for ( item = list; item->next; item = item->next ) {
        g_free( item->data );
    }

    g_list_free( list );
}

void document_destroy( document_t* doc ) {
    /* Stop & destroy the render engine */

    render_engine_stop( doc->render_engine, TRUE );
    render_engine_destroy( doc->render_engine );
    doc->render_engine = NULL;

    /* Destroy the page table */

    document_destroy_page_table( doc->page_table, doc->page_count );

    if ( doc->page_table != NULL ) {
        free( doc->page_table );
        doc->page_table = NULL;
    }

    g_mutex_clear( &doc->page_lock );
    g_mutex_clear( &doc->reload_lock );

    /* Destroy the poppler document */

    if ( doc->doc != NULL ) {
        g_object_unref( G_OBJECT( doc->doc ) );
        doc->doc = NULL;
    }

    /* Destroy reference table */

    if ( doc->ref_table != NULL ) {
        ref_table_destroy( doc->ref_table );
        doc->ref_table = NULL;
    }

    /* Destroy find context and thread */
    if ( doc->fc != NULL ) {
        find_stop( doc->fc );
    }

    g_hash_table_foreach( doc->match_table, document_destroy_match_table_values, doc->match_table );
    g_hash_table_destroy( doc->match_table );

    /* ... and the others :) */

    free( doc->path );
    free( doc->filename );
    free( doc );
}

static void document_apply_scale_and_rotate( page_t* page ) {
    double scale = ( double )page->scale / 100.0f;

    int scaled_width = ( int )( page->original_width * scale );
    int scaled_height = ( int )( page->original_height * scale );

    /* Calculate the width & height of the surface */

    switch ( page->rotate ) {
        case 90 :
        case 270 :
            page->current_width = scaled_height;
            page->current_height = scaled_width;

            break;

        case 0 :
        case 180 :
            page->current_width = scaled_width;
            page->current_height = scaled_height;

            break;
    }
}

static int document_create_page_table( document_t* doc, page_t* old_page_table, int old_page_count ) {
    int i;
    int page_count;
    page_t* page;
    char path[ 256 ];
    document_history_t* history;

    snprintf( path, sizeof( path ), "%s%s%s", doc->path, PATH_SEPARATOR, doc->filename );

    history = history_get_document_info( path, FALSE );

    page_count = poppler_document_get_n_pages( doc->doc );

    doc->page_table = ( page_t* )malloc( sizeof( page_t ) * page_count );

    if ( doc->page_table == NULL ) {
        goto error1;
    }

    memset( doc->page_table, 0, sizeof( page_t ) * page_count );

    for ( i = 0, page = &doc->page_table[ 0 ]; i < page_count; i++, page++ ) {
        double width;
        double height;

        page->page = poppler_document_get_page( doc->doc, i );

        if ( page->page == NULL ) {
            goto error2;
        }

        page->link_mapping = poppler_page_get_link_mapping( page->page );

        poppler_page_get_size( page->page, &width, &height );

        page->original_width = ( int )width;
        page->original_height = ( int )height;

        if ( ( old_page_table != NULL ) &&
             ( i < old_page_count ) ) {
            /* The page is found in the old page table. This method is used
               when the document is changed and reloading it. */

            page_t* old_page;

            old_page = &old_page_table[ i ];

            page->scale = old_page->scale;
            page->rotate = old_page->rotate;
        } else {
            /* We don't have the old page table, so we have three different ways:
               - the document has history and it contains per page info for the current page
               - the document has history but it doesn't contain per page info for the current page,
                 use the default values from the history
               - no history information for the document, use the "hardcoded" values :) */

            if ( history != NULL ) {
                page_history_t* page_hist;

                page_hist = g_hash_table_lookup( history->page_table, GINT_TO_POINTER( i ) );

                if ( page_hist != NULL ) {
                    page->scale = page_hist->scale;
                    page->rotate = page_hist->rotate;
                } else {
                    page->scale = history->scale;
                    page->rotate = history->rotate;
                }
            } else {
                page->scale = 100;
                page->rotate = 0;
            }
        }

        document_apply_scale_and_rotate( page );
    }

    doc->page_count = page_count;

    return 0;

 error2:
    document_destroy_page_table( doc->page_table, doc->page_count );

 error1:
    return -ENOMEM;
}

static gpointer document_open_thread( gpointer arg ) {
    int error;
    char uri[ 256 ];
    document_t* doc;
    GError* gerror = NULL;

    doc = ( document_t* )arg;

#define NOTIFY_OPEN_LISTENER(e) \
    if ( doc->open_listener != NULL ) { \
        doc->open_listener( doc, e ); \
    }

    NOTIFY_OPEN_LISTENER( DOC_OPEN_STARTED );

    /* Open the document */

    snprintf(
        uri,
        sizeof( uri ),
        "file://%s%s%s",
        doc->path,
        PATH_SEPARATOR,
        doc->filename
    );

    poppler_lock();
    doc->doc = poppler_document_new_from_file( uri, doc->password, &gerror );
    poppler_unlock();

    if ( doc->doc == NULL ) {
        goto error1;
    }

    /* Create the table of pages from the document */

    error = document_create_page_table( doc, NULL, 0 );

    if ( error < 0 ) {
        goto error2;
    }

    /* Initialize reference table */

    doc->ref_table = ref_table_new( doc );

    NOTIFY_OPEN_LISTENER( DOC_OPEN_DONE_OK );

    /* Start the render thread */

    render_engine_start( doc->render_engine );

    return NULL;

 error2:
    g_object_unref( G_OBJECT( doc->doc ) );
    doc->doc = NULL;

 error1:
    if ( gerror != NULL ) {
        if ( gerror->code == POPPLER_ERROR_ENCRYPTED ) {
            NOTIFY_OPEN_LISTENER( DOC_OPEN_DONE_ENCRYPTED );
        } else {
            NOTIFY_OPEN_LISTENER( DOC_OPEN_DONE_FAILED );
        }

        g_error_free( gerror );
    }

#undef NOTIFY_OPEN_LISTENER

    return NULL;
}

int document_start_open( document_t* document ) {
    document->open_thread = g_thread_new(
	"document-open",
        document_open_thread,
        ( gpointer )document
    );

    return 0;
}

static gpointer document_reload_thread( gpointer arg ) {
    int error;
    int page_count;
    char uri[ 256 ];
    document_t* doc;
    page_t* page_table;
    GError* gerror;

#define NOTIFY_OPEN_LISTENER(e) \
    if ( doc->open_listener != NULL ) { \
        doc->open_listener( doc, e ); \
    }

    doc = ( document_t* )arg;

    g_mutex_lock( &doc->reload_lock );

    /* Stop the render engine */

    render_engine_stop( doc->render_engine, TRUE );

    page_count = doc->page_count;
    page_table = doc->page_table;

    doc->page_count = 0;
    doc->page_table = NULL;

    NOTIFY_OPEN_LISTENER( DOC_RELOAD_STARTED );

    /* Destroy the reference table */

    if ( doc->ref_table != NULL ) {
        ref_table_destroy( doc->ref_table );
    }

    /* Destroy the document */

    g_object_unref( G_OBJECT( doc->doc ) );

    /* Open the document */

    snprintf(
        uri,
        sizeof( uri ),
        "file://%s%s%s",
        doc->path,
        PATH_SEPARATOR,
        doc->filename
    );

    doc->doc = poppler_document_new_from_file( uri, doc->password, &gerror );

    if ( doc->doc == NULL ) {
        goto error1;
    }

    /* Create the table of pages from the document */

    error = document_create_page_table( doc, page_table, page_count );

    /* Destroy the old page table */

    document_destroy_page_table( page_table, page_count );
    free( page_table );

    if ( error < 0 ) {
        goto error2;
    }

    /* Initialize reference table */

    doc->ref_table = ref_table_new( doc );

    /* Start the render engine */

    render_engine_start( doc->render_engine );

    NOTIFY_OPEN_LISTENER( DOC_RELOAD_DONE_OK );

    g_mutex_unlock( &doc->reload_lock );

    return NULL;

 error2:
    g_object_unref( G_OBJECT( doc->doc ) );
    doc->doc = NULL;

 error1:
    if ( gerror != NULL ) {
        if ( gerror->code == POPPLER_ERROR_ENCRYPTED ) {
            NOTIFY_OPEN_LISTENER( DOC_OPEN_DONE_ENCRYPTED );
        } else {
            NOTIFY_OPEN_LISTENER( DOC_OPEN_DONE_FAILED );
        }

        g_error_free( gerror );
    }

#undef NOTIFY_OPEN_LISTENER

    g_mutex_unlock( &doc->reload_lock );

    document_destroy_page_table( page_table, page_count );
    free( page_table );

    return NULL;
}

int document_do_reload( document_t* document ) {
    document->open_thread = g_thread_new(
	"document-reload",
        document_reload_thread,
        ( gpointer )document
    );

    return 0;
}

int document_update_history( document_t* document, int current_page ) {
    int i;
    page_t* page;
    char path[ 256 ];
    document_history_t* history;
    pdf_tab_t* pdf_tab;

    snprintf(
        path,
        sizeof( path ),
        "%s%s%s",
        document->path,
        PATH_SEPARATOR,
        document->filename
    );

    history = history_get_document_info( path, TRUE );

    if ( history == NULL ) {
        return -ENOMEM;
    }

    pdf_tab = document_storage_get_tab_by_doc( document );

    history->v_offset = doctab_v_adjustment_get_value( pdf_tab );
    history->h_offset = doctab_h_adjustment_get_value( pdf_tab );
    history->current_page = gtk_pdfview_get_current_page( pdf_tab->pdf_view );
    history->zoom_type = gtk_pdfview_get_zoom_type( pdf_tab->pdf_view );
    history->display_type = gtk_pdfview_get_display_type( pdf_tab->pdf_view );

    document_get_common_rotate_and_scale( document, &history->rotate, &history->scale );

    for ( i = 0, page = &document->page_table[ i ]; i < document->page_count; i++, page++ ) {
        if ( ( page->rotate != history->rotate ) ||
            ( page->scale != history->scale ) ) {
            history_add_page_info( history, i, page->rotate, page->scale );
        }
    }

    return 0;
}

int document_reverse_pages( document_t* document ) {
    int i;
    page_t page;

    g_mutex_lock( &document->page_lock );

    for ( i = 0; i < document->page_count / 2; i++ ) {
        memcpy(
            &page,
            &document->page_table[ i ],
            sizeof( page_t )
        );
        memcpy(
            &document->page_table[ i ],
            &document->page_table[ document->page_count - ( i + 1 ) ],
            sizeof( page_t )
        );
        memcpy(
            &document->page_table[ document->page_count - ( i + 1 ) ],
            &page,
            sizeof( page_t )
        );
    }

    g_mutex_unlock( &document->page_lock );

    return 0;
}

page_t* document_get_page( document_t* document, int index ) {
    if ( ( index < 0 ) ||
         ( index >= document->page_count ) ) {
        return NULL;
    }

    return &document->page_table[ index ];
}

int document_get_scale_for_page( document_t* document, int page_index ) {
    g_return_val_if_fail( page_index >= 0, 100 );
    g_return_val_if_fail( page_index < document->page_count, 100 );

    return document->page_table[ page_index ].scale;
}

int document_get_rotate_for_page( document_t* document, int page_index ) {
    g_return_val_if_fail( page_index >= 0, 0 );
    g_return_val_if_fail( page_index < document->page_count, 0 );

    return document->page_table[ page_index ].rotate;
}

int document_get_memory_size_for_page( document_t* document, int page_index ) {
    page_t* page;

    g_return_val_if_fail( page_index >= 0, 0 );
    g_return_val_if_fail( page_index < document->page_count, 0 );

    page = &document->page_table[ page_index ];

    return ( page->current_width * page->current_height * 4 /* rgb32 */ );
}

cairo_surface_t* document_get_page_surface( document_t* document, int index ) {
    cairo_surface_t* surface;

    if ( ( index < 0 ) ||
         ( index >= document->page_count ) ) {
        return NULL;
    }

    g_mutex_lock( &document->page_lock );

    surface = document->page_table[ index ].surf;

    if ( surface != NULL ) {
        cairo_surface_reference( surface );
    }

    g_mutex_unlock( &document->page_lock );

    return surface;
}

int document_get_page_count( document_t* document ) {
    return document->page_count;
}

int document_get_row_count( document_t* document, int cols ) {
    int rows = document->page_count / cols;

    if ( document->page_count % cols != 0 ) {
        ++rows;
    }

    return rows;
}

void document_get_geometry( document_t* document, int cols, int* width, int* height ) {
    int i;
    int max_w = 0;
    int max_h = 0;
    page_t* page;

    *width = 0;
    *height = 0;

    for ( i = 0, page = document->page_table; i < document->page_count; i++, page++ ) {
        if ( i % cols == 0 ) {
            *width = MAX( *width, max_w );
            *height += max_h;
            max_w = 0;
            max_h = 0;
        }

        max_w += page->current_width;
        max_h = MAX( max_h, page->current_height );
    }

    *width = MAX( *width, max_w );
    *height += max_h;
}

void document_row_get_geometry( document_t* document, int row, int cols, int* width, int* height ) {
    int i;
    int min_i;
    int max_i;
    page_t* page;

    min_i = row * cols;
    max_i = MIN( document->page_count, ( row + 1 ) * cols );

    *width = 0;
    *height = 0;

    for ( i = min_i, page = document->page_table + min_i; i < max_i; i++, page++ ) {
        *height = MAX( *height, page->current_height );
        *width += page->current_width;
    }
}

void document_row_get_original_geometry( document_t* document, int row, int cols, int* width, int* height ) {
    int i;
    int min_i;
    int max_i;
    page_t* page;

    min_i = row * cols;
    max_i = MIN( document->page_count, ( row + 1 ) * cols );

    *width = 0;
    *height = 0;

    for ( i = min_i, page = document->page_table + min_i; i < max_i; i++, page++ ) {
        switch ( page->rotate ) {
            case 0:
            case 180:
                *height = MAX( *height, page->original_height );
                *width += page->original_width;

                break;

            case 90:
            case 270:
                *height = MAX( *height, page->original_width );
                *width += page->original_height;

                break;
        }
    }
}

typedef struct doc_rs_info {
    int rotate;
    int scale;
    int count;
} doc_rs_info_t;

int document_get_common_rotate_and_scale( document_t* document, int* rotate, int* scale ) {
    int i;
    int j;
    page_t* page;
    doc_rs_info_t* best_rs;
    GList* rs_info_list = NULL;

    for ( i = 0, page = &document->page_table[ 0 ]; i < document->page_count; i++, page++ ) {
        int found;
        doc_rs_info_t* rs_info;

        found = FALSE;

        for ( j = 0; j < g_list_length( rs_info_list ); j++ ) {
            rs_info = ( doc_rs_info_t* )g_list_nth_data( rs_info_list, j );

            if ( ( rs_info->rotate == page->rotate ) &&
                 ( rs_info->scale == page->scale ) ) {
                found = TRUE;

                break;
            }
        }

        if ( !found ) {
            rs_info = ( doc_rs_info_t* )malloc( sizeof( doc_rs_info_t ) );

            if ( rs_info != NULL ) {
                rs_info->rotate = page->rotate;
                rs_info->scale = page->scale;
                rs_info->count = 1;

                rs_info_list = g_list_append( rs_info_list, rs_info );
            }
        } else {
            rs_info->count++;
        }
    }

    if ( g_list_length( rs_info_list ) == 0 ) {
        return -EINVAL;
    }

    best_rs = g_list_nth_data( rs_info_list, 0 );

    for ( i = 1; i < g_list_length( rs_info_list ); i++ ) {
        doc_rs_info_t* tmp;

        tmp = ( doc_rs_info_t* )g_list_nth_data( rs_info_list, i );

        if ( tmp->count > best_rs->count ) {
            best_rs = tmp;
        }
    }

    if ( rotate != NULL ) {
        *rotate = best_rs->rotate;
    }

    if ( scale != NULL ) {
        *scale = best_rs->scale;
    }

    for ( i = 0; i < g_list_length( rs_info_list ); i++ ) {
        free( g_list_nth_data( rs_info_list, i ) );
    }

    g_list_free( rs_info_list );

    return 0;
}

int document_set_page_surface( document_t* document, int index, cairo_surface_t* surface ) {
    cairo_surface_t* tmp;

    if ( ( index < 0 ) ||
         ( index >= document->page_count ) ) {
        return -EINVAL;
    }

    g_mutex_lock( &document->page_lock );

    tmp = document->page_table[ index ].surf;

    if ( tmp != NULL ) {
        cairo_surface_destroy( tmp );
    }

    document->page_table[ index ].surf = surface;

    g_mutex_unlock( &document->page_lock );

    return 0;
}

int document_set_rendered_region( document_t* document, int start_index, int end_index ) {
    int i;
    page_t* page;

    g_mutex_lock( &document->page_lock );

    /* Clean up pages before the region */

    for ( i = 0, page = &document->page_table[ 0 ]; i < start_index; i++, page++ ) {
        if ( page->surf != NULL ) {
            cairo_surface_destroy( page->surf );
            page->surf = NULL;
        }
    }

    /* Clean up pages after the region */

    for ( i = end_index + 1, page = &document->page_table[ end_index + 1 ]; i < document->page_count; i++, page++ ) {
        if ( page->surf != NULL ) {
            cairo_surface_destroy( page->surf );
            page->surf = NULL;
        }
    }

    g_mutex_unlock( &document->page_lock );

    return 0;
}

int document_set_scale( document_t* document, int scale ) {
    int i;
    page_t* page;

    for ( i = 0, page = &document->page_table[ 0 ]; i < document->page_count; i++, page++ ) {
        page->scale = scale;
        document_apply_scale_and_rotate( page );
    }

    return 0;
}

int document_set_scale_for_page( document_t* document, int page_index, int scale ) {
    page_t* page;

    g_return_val_if_fail( page_index >= 0, -EINVAL );
    g_return_val_if_fail( page_index < document->page_count, -EINVAL );

    page = &document->page_table[ page_index ];
    page->scale = scale;

    document_apply_scale_and_rotate( page );

    return 0;
}

int document_set_rotate( document_t* document, int rotate ) {
    int i;
    page_t* page;

    for ( i = 0, page = &document->page_table[ 0 ]; i < document->page_count; i++, page++ ) {
        page->rotate = rotate;
        document_apply_scale_and_rotate( page );
    }

    return 0;
}

int document_set_rotate_for_page( document_t* document, int page_index, int rotate ) {
    page_t* page;

    g_return_val_if_fail( page_index >= 0, -EINVAL );
    g_return_val_if_fail( page_index < document->page_count, -EINVAL );

    page = &document->page_table[ page_index ];
    page->rotate = rotate;

    document_apply_scale_and_rotate( page );

    return 0;
}

int document_set_open_listener( document_t* doc, open_listener_t* listener ) {
    doc->open_listener = listener;

    return 0;
}
