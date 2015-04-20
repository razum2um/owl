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

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <storage.h>
#include <utils.h>
#include <watcher.h>
#include <gui/events.h>
#include <gui/pdf_view.h>
#include <gui/main_wnd.h>
#include <config/history.h>
#include <config/settings.h>

int get_current_document_info( pdf_tab_t** pdf_tab, document_t** document, int* page_index ) {
    int current;
    GtkWidget* child;

    current = gtk_notebook_current_page( GTK_NOTEBOOK( main_notebook ) );

    if ( current == -1 ) {
        return -ENOENT;
    }

    child = gtk_notebook_get_nth_page( GTK_NOTEBOOK( main_notebook ), current );

    if ( pdf_tab != NULL ) {
        *pdf_tab = document_storage_get_tab_by_widget( child );
        g_return_val_if_fail( *pdf_tab != NULL, -EINVAL );
    }

    if ( document != NULL ) {
        *document = document_storage_get_doc_by_widget( child );
        g_return_val_if_fail( *document != NULL, -EINVAL );
    }

    if ( page_index != NULL ) {
        *page_index = current;
    }

    return 0;
}

typedef struct open_check_data {
    char* filename;
    int already_open;
} open_check_data_t;

static int already_open_check_helper( document_t* document, pdf_tab_t* pdf_tab, void* _data ) {
    char path[ 256 ];
    open_check_data_t* data = ( open_check_data_t* )_data;

    snprintf(
        path,
        sizeof( path ),
        "file://%s%s%s",
        document->path,
        PATH_SEPARATOR,
        document->filename
    );

    if ( strcmp( path, data->filename ) == 0 ) {
        data->already_open = 1;
    }

    return 0;
}

int do_open_document( const char* uri ) {
    char buffer[ 256 ];
    document_t* document;
    open_check_data_t data;

    data.filename = ( char* )uri;
    data.already_open = 0;

    document_storage_foreach( already_open_check_helper, ( void* )&data );

    if ( data.already_open ) {
        return -1;
    }

    /* Create the new document */

    document = document_new( uri );

    if ( document == NULL ) {
        return -ENOMEM;
    }

    snprintf( buffer, sizeof( buffer ), "file://%s", document->path );
    settings_set_string( "open-file-path", buffer );

    /* Set the required listeners */

    document_set_open_listener( document, event_document_opened );
    render_engine_set_listener( document->render_engine, event_page_render_done );

    /* Start the loader thread */

    document_start_open( document );

    return 0;
}

void do_save_document( document_t* document, char* uri ) {
    size_t size;
    FILE* new_file;
    FILE* original_file;
    char buffer[ 8192 ];

    snprintf( buffer, sizeof( buffer ), "%s%s%s", document->path, PATH_SEPARATOR, document->filename );

    /* Make sure that the src and dest file is not the same. */

    if ( strcmp( buffer, uri ) == 0 ) {
        return;
    }

    original_file = fopen( buffer, "rb" );

    if ( original_file == NULL ) {
        return;
    }

    new_file = fopen( uri, "wb" );

    if ( new_file == NULL ) {
        fclose( original_file );
        return;
    }

    do {
        size = fread( buffer, 1, sizeof( buffer ), original_file );

        if ( ferror( original_file ) ) {
            break;
        }

        if ( size > 0 ) {
            fwrite( buffer, 1, size, new_file );
        }
    } while ( size == sizeof( buffer ) );

    fclose( new_file );
    fclose( original_file );
}

void do_update_current_page( pdf_tab_t* pdf_tab ) {
    char tmp[ 16 ];
    int current_page;
    document_t* document;

    document = document_storage_get_doc_by_pdfview( pdf_tab->pdf_view );
    g_return_if_fail( document != NULL );

    current_page = gtk_pdfview_get_current_page( pdf_tab->pdf_view );

    /* Update the current page entry on the navigator bar */

    snprintf( tmp, sizeof( tmp ), "%d", current_page + 1 );

    gtk_entry_set_text( GTK_ENTRY( pdf_tab->entry_curr_pg ), tmp );

    /* Update the highlight in TOC */

    if ( pdf_tab->cur_highlighted_page != -1 ) {
        ref_table_highlight_page( document->ref_table, pdf_tab->toc_view, NULL, pdf_tab->cur_highlighted_page + 1, 0 );
    }

    ref_table_highlight_page( document->ref_table, pdf_tab->toc_view, NULL, current_page + 1, 1 );

    pdf_tab->cur_highlighted_page = current_page;
}

void do_update_page_count( pdf_tab_t* pdf_tab, document_t* document ) {
    char tmp[ 16 ];
    int page_count;

    page_count = document_get_page_count( document );

    snprintf( tmp, sizeof( tmp ), "%d", page_count );

    gtk_entry_set_text( GTK_ENTRY( pdf_tab->entry_last_pg ), tmp );
}

void do_update_zoom_entry( pdf_tab_t* pdf_tab, document_t* document ) {
    int current_page;
    int scale;
    char tmp[ 16 ];

    current_page = gtk_pdfview_get_current_page( pdf_tab->pdf_view );
    scale = document_get_scale_for_page( document, current_page );

    snprintf( tmp, sizeof( tmp ), "%d", scale );

    gtk_entry_set_text( GTK_ENTRY( entry_zoom ), tmp );
}

void do_update_render_region( pdf_tab_t* pdf_tab, document_t* document, int lazy_rendering ) {
    int i;
    int current_page;
    int doc_page_count;
    render_command_t* cmd;

    int first_page;
    int last_page;

    int enable_page_count_check;
    int max_page_count;

    int enable_memory_check;
    int used_cache_memory;
    int max_cache_memory;

    settings_get_int( "enable-render-limit-pages", &enable_page_count_check );

    if ( enable_page_count_check ) {
        settings_get_int( "render-limit-pages", &max_page_count );

        max_page_count = MIN( MAX_PAGE_COUNT, max_page_count * 2 );
    } else {
        max_page_count = MAX_PAGE_COUNT;
    }

    settings_get_int( "enable-render-limit-mb", &enable_memory_check );
    settings_get_int( "render-limit-mb", &max_cache_memory );
    max_cache_memory *= 1024 * 1024;

    current_page = gtk_pdfview_get_current_page( pdf_tab->pdf_view );
    doc_page_count = document_get_page_count( document );

    /* Flush the command queue of the render engine, so the rendering of the
       next page will be started immediately. */

    render_engine_flush_queue( document->render_engine );

    /* The current page is the first one to render */

    cmd = render_engine_create_command( current_page, lazy_rendering );
    render_engine_queue_command( document->render_engine, cmd );

    used_cache_memory = document_get_memory_size_for_page( document, current_page );

    first_page = current_page;
    last_page = current_page;

    for ( i = 0; i < max_page_count; i++ ) {
        int page;

        if ( ( i % 2 ) == 0 ) {
            page = current_page + ( 1 + ( i / 2 ) );
        } else {
            page = current_page - ( 1 + ( i / 2 ) );
        }

        if ( ( page >= 0 ) &&
             ( page < doc_page_count ) ) {
            int page_mem_size;

            page_mem_size = document_get_memory_size_for_page( document, page );

            if ( ( enable_memory_check ) &&
                 ( ( used_cache_memory + page_mem_size ) > max_cache_memory ) ) {
                break;
            }

            first_page = MIN( first_page, page );
            last_page = MAX( last_page, page );

            used_cache_memory += page_mem_size;

            cmd = render_engine_create_command( page, lazy_rendering );
            render_engine_queue_command( document->render_engine, cmd );
        }
    }

    /* Update the rendered region of the document */

    document_set_rendered_region( document, first_page, last_page );
}

void do_update_page_specific_menu_items( pdf_tab_t* pdf_tab ) {
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON( toolbar_selection->button ),
        gtk_pdfview_get_selection_mode( pdf_tab->pdf_view )
    );

    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM( view_cont_mode ),
        gtk_pdfview_get_display_type( pdf_tab->pdf_view ) == CONTINUOUS
    );
}

void do_close_document_tab( pdf_tab_t* pdf_tab, document_t* document, int page_index ) {
    char path[ 256 ];
    document_history_t* history;

    /* Remove the file from the watcher thread */

    snprintf( path, sizeof( path ), "%s%s%s", document->path, PATH_SEPARATOR, document->filename );

    watcher_remove_document( document );

    /* Update document history */

    document_update_history( document, gtk_pdfview_get_current_page( pdf_tab->pdf_view ) + 1 );

    history = history_get_document_info( path, TRUE );

    if ( history != NULL ) {
        history->recently_open = 0;

        if ( pdf_tab->paned == NULL ) {
            history->paned_position = -1;
        } else {
            history->paned_position = gtk_paned_get_position( GTK_PANED( pdf_tab->paned ) );
        }
    }

    /* Remove the tab from the notebook */

    gtk_notebook_remove_page( GTK_NOTEBOOK( main_notebook ), page_index );

    if ( gtk_notebook_get_n_pages( GTK_NOTEBOOK( main_notebook ) ) == 1 ) {
        int show_tabs = 0;

        settings_get_int( "always-show-tabs", &show_tabs );

        if ( !show_tabs ) {
            gtk_notebook_set_show_tabs( GTK_NOTEBOOK( main_notebook ), FALSE );
        }
    }

    /* Destroy the document */

    document_storage_remove( pdf_tab, document );
    document_destroy( document );
    free( pdf_tab );
}

void do_zoom( pdf_tab_t* pdf_tab, document_t* document, int page_index, int amount ) {
    int current_scale;

    if ( page_index < 0 ) {
        const gchar* text = gtk_entry_get_text( GTK_ENTRY( entry_zoom ) );

        if ( sscanf( text, "%d", &current_scale ) != 1 ) {
            return;
        }
    } else {
        current_scale = document_get_scale_for_page( document, page_index );
    }

    if ( amount > 0 ) {
        current_scale *= 1.41421356;
    } else {
        current_scale *= 0.70710678;
    }

    FORCE_MIN_MAX( current_scale, MIN_ZOOM, MAX_ZOOM );

    if ( page_index < 0 ) {
        document_set_scale( document, current_scale );
    } else {
        document_set_scale_for_page( document, page_index, current_scale );
    }

    gtk_pdfview_set_zoom_type( pdf_tab->pdf_view, ZOOM_FREE );

    gtk_pdfview_invalidate_selection( pdf_tab->pdf_view );

    doctab_update_adjustment_limits( pdf_tab );
    do_update_zoom_entry( pdf_tab, document );
    do_update_render_region( pdf_tab, document, FALSE );
}

void do_zoom_fit_width( void ) {
    int widget_width;
    int width;
    int height;
    int error;
    int new_scale;
    int cols;
    int i;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    cols = gtk_pdfview_get_cols( pdf_tab->pdf_view );

    gdk_window_get_geometry( pdf_tab->pdf_view->window, NULL, NULL, &widget_width, NULL, NULL );

    for ( i = 0; i < document->page_count; i++ ) {
        if ( i % cols == 0 ) {
            gtk_pdfview_row_get_original_geometry( pdf_tab->pdf_view, i / cols, &width, &height );
        }

        new_scale = ( double )widget_width / width * 100;

        document_set_scale_for_page( document, i, new_scale );
    }

    gtk_pdfview_set_zoom_type( pdf_tab->pdf_view, ZOOM_FIT_WIDTH );

    gtk_pdfview_invalidate_selection( pdf_tab->pdf_view );

    doctab_update_adjustment_limits( pdf_tab );
    do_update_zoom_entry( pdf_tab, document );
    do_update_render_region( pdf_tab, document, FALSE );
}

void do_zoom_fit_page( void ) {
    int widget_height;
    int widget_width;
    int width;
    int height;
    int error;
    int new_scale_h;
    int new_scale_w;
    int cols;
    int i;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    cols = gtk_pdfview_get_cols( pdf_tab->pdf_view );

    gdk_window_get_geometry( pdf_tab->pdf_view->window, NULL, NULL, &widget_width, &widget_height, NULL );

    for ( i = 0; i < document->page_count; i++ ) {
        if ( i % cols == 0 ) {
            gtk_pdfview_row_get_original_geometry( pdf_tab->pdf_view, i / cols, &width, &height );
        }

        new_scale_w = ( double )widget_width / width * 100;
        new_scale_h = ( double )widget_height / height * 100;

        document_set_scale_for_page( document, i, MIN( new_scale_w, new_scale_h ) );
    }

    gtk_pdfview_set_zoom_type( pdf_tab->pdf_view, ZOOM_FIT_PAGE );

    gtk_pdfview_invalidate_selection( pdf_tab->pdf_view );

    doctab_update_adjustment_limits( pdf_tab );
    do_update_zoom_entry( pdf_tab, document );
    do_update_render_region( pdf_tab, document, FALSE );
}

void do_rotate( pdf_tab_t* pdf_tab, document_t* document, int page_index, int amount ) {
    int current_rotate;

    g_return_if_fail( ( amount % 90 ) == 0 );

    if ( page_index < 0 ) {
        int current_page = gtk_pdfview_get_current_page( pdf_tab->pdf_view );
        current_rotate = document_get_rotate_for_page( document, current_page );
    } else {
        current_rotate = document_get_rotate_for_page( document, page_index );
    }

    current_rotate = normalize_angle( current_rotate + amount );

    if ( page_index < 0 ) {
        document_set_rotate( document, current_rotate );
    } else {
        document_set_rotate_for_page( document, page_index, current_rotate );
    }

    gtk_pdfview_invalidate_selection( pdf_tab->pdf_view );

    doctab_update_adjustment_limits( pdf_tab );
    do_update_render_region( pdf_tab, document, FALSE );

    gtk_pdfview_update_auto_zoom( pdf_tab->pdf_view );
}

void do_save_selection_as_text( pdf_tab_t* pdf_tab, document_t* document ) {
    int sel_x1;
    int sel_y1;
    int sel_x2;
    int sel_y2;
    int page_x1;
    int page_y1;
    int page_x2;
    int page_y2;
    int current_page;
    int page_start;
    int page_end;
    char* textbuf = NULL;
    int textlen = 0;
    GtkPdfView* pdf_view = GTK_PDFVIEW( pdf_tab->pdf_view );

    gtk_pdfview_get_sel_rect( pdf_tab->pdf_view, &sel_x1, &sel_y1, &sel_x2, &sel_y2 );

    if ( gtk_pdfview_screen_to_page( pdf_tab->pdf_view, sel_x1, sel_y1, &page_start, &page_x1, &page_y1 ) < 0 ||
         gtk_pdfview_screen_to_page( pdf_tab->pdf_view, sel_x2, sel_y2, &page_end,   &page_x2, &page_y2 ) < 0 ) {
        return;
    }

    for ( current_page = page_start; current_page <= page_end; current_page++ ) {
        page_t* page = document_get_page( pdf_view->document, current_page );
        char* text;
        int length;
        PopplerRectangle rect;

        if ( page_x1 < page_x2 ) {
            rect.x1 = page_x1;
            rect.x2 = page_x2;
        } else {
            rect.x1 = page_x2;
            rect.x2 = page_x1;
        }

        if ( current_page == page_start ) {
            rect.y1 = page_y1;
        } else {
            rect.y1 = page->original_height;
        }

        if ( current_page == page_end ) {
            rect.y2 = page_y2;
        } else {
            rect.y2 = 0;
        }

        if ( rect.y1 > rect.y2 ) {
            gdouble tmp = rect.y1;
            rect.y1 = rect.y2;
            rect.y2 = tmp;
        }

        text = poppler_page_get_selected_text(
            page->page,
            POPPLER_SELECTION_GLYPH,
            &rect
        );

        if ( text == NULL ) {
            continue;
        }

        length = strlen( text );

        if ( length > 0 ) {
            textlen += length;

            if ( textbuf == NULL ) {
                textbuf = ( char* )malloc( sizeof( char ) * ( textlen + 1 ) );
                textbuf[ 0 ] = '\0';
            } else {
                textbuf = ( char* )realloc( textbuf, sizeof( char ) * ( textlen + 1 ) );
            }

            strncat( textbuf, text, length );
            g_free( text );
        }
    }

    GtkClipboard* clipboard;

    clipboard = gtk_clipboard_get( GDK_SELECTION_PRIMARY );
    gtk_clipboard_set_text( clipboard, textbuf, textlen );

    clipboard = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    gtk_clipboard_set_text( clipboard, textbuf, textlen );

    free( textbuf );
}

void do_save_selection_as_image( pdf_tab_t* pdf_tab, document_t* document, GdkPixbuf** pixbuf ) {
    int sel_x1;
    int sel_y1;
    int sel_x2;
    int sel_y2;
    int width;
    int height;
    GdkRectangle rect;

    gtk_pdfview_get_sel_rect( pdf_tab->pdf_view, &sel_x1, &sel_y1, &sel_x2, &sel_y2 );

    width = sel_x2 - sel_x1 + 1;
    height = sel_y2 - sel_y1 + 1;

    *pixbuf = gdk_pixbuf_new(
        GDK_COLORSPACE_RGB,
        TRUE,
        8,
        width,
        height
    );

    if ( *pixbuf == NULL ) {
        printf( "initial pixbuf creation failed\n" );
        return;
    }

    GTK_PDFVIEW( pdf_tab->pdf_view )->selection_mode = 0;

    while ( gtk_events_pending() ) {
        gtk_main_iteration();
    }

    gtk_pdfview_get_sel_gdk_rect( pdf_tab->pdf_view, &rect );
    gtk_pdfview_draw( pdf_tab->pdf_view, &rect );

    while ( gtk_events_pending() ) {
        gtk_main_iteration();
    }

    *pixbuf = gdk_pixbuf_get_from_drawable(
        *pixbuf,
        pdf_tab->pdf_view->window,
        gdk_colormap_get_system(),
        sel_x1,
        sel_y1,
        0,
        0,
        width,
        height
    );

    GTK_PDFVIEW( pdf_tab->pdf_view )->selection_mode = 1;

    if ( *pixbuf == NULL ) {
        printf( "gdk_pixbuf_get_from_drawable failed\n" );
        return;
    }
}

void do_doctab_find( pdf_tab_t* pdf_tab, find_dir_t direction ) {
    const char* text;
    document_t* doc;

    doc = document_storage_get_doc_by_pdfview( pdf_tab->pdf_view );

    if ( doc == NULL ) {
        return;
    }

    text = gtk_entry_get_text( GTK_ENTRY( pdf_tab->entry_find ) );

    if ( text == NULL || strlen( text ) == 0 ) {
        return;
    }

    /* find context initialization */

    if ( doc->fc != NULL && strcmp( text, doc->fc->text ) ) {
        find_stop( doc->fc );
        doc->fc = NULL;
    }

    if ( doc->fc == NULL ) {
        doc->fc = find_context_new(
            doc,
            gtk_pdfview_get_current_page( pdf_tab->pdf_view ),
            text
        );
    }

    /* ready to go */

    doctab_set_search_status( pdf_tab, SEARCH_STATUS_BUSY );

    find_push( doc->fc, direction );
}

void do_doctab_show_search_bar( GtkWidget* widget ) {
    pdf_tab_t* pdf_tab;

    pdf_tab = document_storage_get_tab_by_pdfview( widget );

    if ( !GTK_WIDGET_VISIBLE( pdf_tab->search_bar ) ) {
        gtk_widget_show( pdf_tab->search_bar );
    }

    gtk_widget_grab_focus( pdf_tab->entry_find );
    gtk_editable_select_region( GTK_EDITABLE( pdf_tab->entry_find ), 0, -1 );
}

void do_doctab_hide_search_bar( GtkWidget* widget ) {
    pdf_tab_t* pdf_tab;

    pdf_tab = document_storage_get_tab_by_pdfview( widget );

    if ( GTK_WIDGET_VISIBLE( pdf_tab->search_bar ) ) {
        gtk_widget_hide( pdf_tab->search_bar );

        gtk_widget_grab_focus( pdf_tab->pdf_view );
        gtk_pdfview_clear_highlight( pdf_tab->pdf_view );
    }
}

void do_zoom_page_in_cb( int page_index ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_zoom( pdf_tab, document, page_index, 10 );

    doctab_set_statusbar( pdf_tab, 0, NULL );
}

void do_zoom_page_out_cb( int page_index ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_zoom( pdf_tab, document, page_index, -10 );

    doctab_set_statusbar( pdf_tab, 0, NULL );
}

void do_rotate_page_left_cb( int page_index ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_rotate( pdf_tab, document, page_index, -90 );

    doctab_set_statusbar( pdf_tab, 0, NULL );
}

void do_rotate_page_right_cb( int page_index ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    do_rotate( pdf_tab, document, page_index, 90 );

    doctab_set_statusbar( pdf_tab, 0, NULL );
}

void do_pick_page_canceled_cb( void ) {
    int error;
    pdf_tab_t* pdf_tab;
    document_t* document;

    error = get_current_document_info( &pdf_tab, &document, NULL );

    if ( error < 0 ) {
        return;
    }

    doctab_set_statusbar( pdf_tab, 0, NULL );
}
