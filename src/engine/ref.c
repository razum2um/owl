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

#include <engine/document.h>
#include <engine/ref.h>

static void ref_parse_from_dest( ref_t* ref, PopplerDocument* doc, PopplerDest* dest ) {
    if ( dest == NULL ) {
        return;
    }

    switch ( dest->type ) {
        case POPPLER_DEST_NAMED : {
            PopplerDest* dest_named = poppler_document_find_dest( doc, dest->named_dest );

            ref_parse_from_dest( ref, doc, dest_named );
            poppler_dest_free( dest_named );

            break;
        }

        case POPPLER_DEST_XYZ   :
        case POPPLER_DEST_FIT   :
        case POPPLER_DEST_FITH  :
        case POPPLER_DEST_FITV  :
        case POPPLER_DEST_FITR  :
        case POPPLER_DEST_FITB  :
        case POPPLER_DEST_FITBH :
        case POPPLER_DEST_FITBV :
            ref->page_num = dest->page_num - 1;
            break;

        case POPPLER_DEST_UNKNOWN :
            printf( "POPPLER_DEST_UNKNOWN\n" );
            break;
    }
}

void ref_parse_from_action( ref_t* ref, PopplerDocument* doc, PopplerAction* action, gboolean follow ) {
    if ( action == NULL ) {
        return;
    }

    switch ( action->type ) {
        case POPPLER_ACTION_GOTO_DEST :
            ref_parse_from_dest( ref, doc, action->goto_dest.dest );

            if ( follow && ref->goto_dest != NULL ) {
                ref->goto_dest( ref->page_num, ref->goto_dest_data );
            }
            break;

        case POPPLER_ACTION_URI :
            if ( follow && ref->goto_uri != NULL ) {
                ref->goto_uri( action->uri.uri );
            }
            break;

        case POPPLER_ACTION_GOTO_REMOTE :
        case POPPLER_ACTION_LAUNCH :
        case POPPLER_ACTION_NAMED :
        case POPPLER_ACTION_MOVIE :
        case POPPLER_ACTION_RENDITION :
        case POPPLER_ACTION_OCG_STATE :
        case POPPLER_ACTION_JAVASCRIPT :
            break;

        case POPPLER_ACTION_UNKNOWN :
            printf( "unknown action\n" );
            break;

#ifndef _WIN32
        case POPPLER_ACTION_NONE :
            break;
#endif /* _WIN32 */

    }
}

static void hook_up_action( PopplerDocument* doc, PopplerAction* action,
                            GtkTreeStore* store, GtkTreeIter* tree_iter, GtkTreeIter* tree_child ) {
    ref_t* ref;
    char num[ 16 ];
    gchar* title;
    gchar* p;
    gunichar u;

    ref = ( ref_t* )calloc( 1, sizeof( ref_t ) );

    if ( ref == NULL ) {
        return;
    }

    ref_parse_from_action( ref, doc, action, FALSE );

    snprintf( num, sizeof( num ), "%d", ref->page_num + 1 );

    title = g_strdup( ( ( PopplerActionAny* )action )->title );
    p = g_utf8_find_next_char( title, NULL );

    while ( p != NULL ) {
        u = g_utf8_get_char( p );

        if ( u == 0 ) {
            break;
        }

        if ( u == '\v' || u == '\f' ) {
            *p = '\n';
        }

        p = g_utf8_find_next_char( p, NULL );
    }

    gtk_tree_store_append( store, tree_child, tree_iter );

    gtk_tree_store_set(
        store,
        tree_child,
        REF_COL_TITLE, title,
        REF_COL_PAGE_NUM, num,
        REF_COL_DATA, ref,
        -1
    );

    g_free( title );
}

static void traverse_index( PopplerDocument* doc, PopplerIndexIter* index_iter,
                            GtkTreeStore* store, GtkTreeIter* tree_iter ) {
    PopplerIndexIter* index_child;
    GtkTreeIter tree_child;
    PopplerAction* action;

    do {
        action = poppler_index_iter_get_action( index_iter );

        hook_up_action( doc, action, store, tree_iter, &tree_child );

        poppler_action_free( action );

        index_child = poppler_index_iter_get_child( index_iter );

        if ( index_child ) {
            traverse_index( doc, index_child, store, &tree_child );
        }

        poppler_index_iter_free( index_child );
    } while ( poppler_index_iter_next( index_iter ) );
}

static void ref_table_toc_new( ref_table_t* ref_table, PopplerDocument* doc ) {
    PopplerIndexIter* index_iter;

    index_iter = poppler_index_iter_new( doc );

    if ( index_iter == NULL ) {
        return;
    }

    ref_table->toc = gtk_tree_store_new(
        REF_COL_COUNT,
        G_TYPE_STRING,  /* title string */
        G_TYPE_STRING,  /* page number */
        G_TYPE_POINTER, /* pointer to ref_t */
        G_TYPE_INT      /* font weight */
    );

    traverse_index( doc, index_iter, ref_table->toc, NULL );

    poppler_index_iter_free( index_iter );
}

ref_table_t* ref_table_new( document_t* document ) {
    ref_table_t* ref_table;

    ref_table = ( ref_table_t* )malloc( sizeof( ref_table_t ) );

    if ( ref_table == NULL ) {
        return NULL;
    }

    memset( ref_table, 0, sizeof( ref_table_t ) );

    ref_table_toc_new( ref_table, document->doc );

    return ref_table;
}

void ref_table_free_toc( GtkTreeModel* model, GtkTreeIter* parent ) {
    GtkTreeIter iter;
    ref_t* ref;
    int i = 0;

    while ( gtk_tree_model_iter_nth_child( model, &iter, parent, i++ ) ) {
        gtk_tree_model_get( model, &iter, REF_COL_DATA, &ref, -1 );
        free( ref );

        ref_table_free_toc( model, &iter );
    }
}

void ref_table_destroy( ref_table_t* ref_table ) {
    if ( ref_table->toc != NULL ) {
        ref_table_free_toc( GTK_TREE_MODEL( ref_table->toc ), NULL );
        gtk_tree_store_clear( ref_table->toc );
    }
}

gboolean ref_table_search_equal_func( GtkTreeModel* model, gint column, const gchar* key,
                                      GtkTreeIter* iter, gpointer data ) {
    gchar* title;
    gchar* title_down;
    gchar* key_down;
    gboolean ret;
    gint len;

    gtk_tree_model_get( model, iter, column, &title, -1 );

    title_down = g_utf8_strdown( title, -1 );
    key_down = g_utf8_strdown( key, -1 );

    len = g_utf8_strlen( title_down, -1 );

    ret = ( g_strstr_len( title_down, len, key_down ) == NULL );

    g_free( key_down );
    g_free( title_down );
    g_free( title );

    return ret;
}

void ref_table_highlight_page( ref_table_t* ref_table, GtkWidget* toc_view, GtkTreeIter* parent, int page_index, int highlight_on ) {
    int i;
    GtkTreeIter iter;
    GtkTreeModel* model;

    if ( ( ref_table == NULL ) ||
         ( ref_table->toc == NULL ) ) {
        return;
    }

    i = 0;
    model = GTK_TREE_MODEL( ref_table->toc );

    while ( gtk_tree_model_iter_nth_child( model, &iter, parent, i ) ) {
        int page_num;
        char* page_num_str;

        gtk_tree_model_get( model, &iter, REF_COL_PAGE_NUM, &page_num_str, -1 );
        sscanf( page_num_str, "%d", &page_num );

        if ( page_num > page_index ) {
            goto do_highlight;
        }

        i++;
    }

 do_highlight:
    if ( i > 0 ) {
        i--;
    }

    gtk_tree_model_iter_nth_child( model, &iter, parent, i );

    gtk_tree_store_set(
        ref_table->toc,
        &iter,
        REF_COL_FONT,
        highlight_on ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
        -1
    );


    if ( gtk_tree_model_iter_has_child( model, &iter ) ) {
        ref_table_highlight_page( ref_table, toc_view, &iter, page_index, highlight_on );
    } else {
        GtkTreePath* path;

        path = gtk_tree_model_get_path( model, &iter );
        gtk_tree_view_expand_to_path( GTK_TREE_VIEW( toc_view ), path );
        gtk_tree_path_free( path );
    }
}
