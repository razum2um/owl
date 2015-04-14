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
#include <errno.h>

#include <storage.h>

static GHashTable* view_table;
static GHashTable* widget_table;
static GHashTable* document_table;

int document_storage_add_new( pdf_tab_t* pdf_tab, document_t* doc ) {
    storage_entry_t* entry;

    entry = ( storage_entry_t* )malloc( sizeof( storage_entry_t ) );

    if ( entry == NULL ) {
        return -ENOMEM;
    }

    entry->pdf_tab = pdf_tab;
    entry->document = doc;

    g_hash_table_insert(
        view_table,
        ( gpointer )pdf_tab->pdf_view,
        ( gpointer )entry
    );

    g_hash_table_insert(
        widget_table,
        ( gpointer )pdf_tab->root,
        ( gpointer )entry
    );

    g_hash_table_insert(
        document_table,
        ( gpointer )doc,
        ( gpointer )entry
    );

    return 0;
}

int document_storage_remove( pdf_tab_t* pdf_tab, document_t* doc ) {
    storage_entry_t* entry;

    entry = ( storage_entry_t* )g_hash_table_lookup(
        document_table,
        ( gconstpointer )doc
    );

    g_return_val_if_fail( entry != NULL, -EINVAL );

    g_hash_table_remove(
        view_table,
        ( gconstpointer )pdf_tab->pdf_view
    );

    g_hash_table_remove(
        widget_table,
        ( gconstpointer )pdf_tab->root
    );

    g_hash_table_remove(
        document_table,
        ( gconstpointer )doc
    );

    free( entry );

    return 0;
}

pdf_tab_t* document_storage_get_tab_by_doc( document_t* doc ) {
    storage_entry_t* entry;

    entry = ( storage_entry_t* )g_hash_table_lookup(
        document_table,
        ( gconstpointer )doc
    );

    if ( entry == NULL ) {
        return NULL;
    }

    return entry->pdf_tab;
}

pdf_tab_t* document_storage_get_tab_by_widget( GtkWidget* widget ) {
    storage_entry_t* entry;

    entry = ( storage_entry_t* )g_hash_table_lookup(
        widget_table,
        ( gconstpointer )widget
    );

    return entry->pdf_tab;
}

pdf_tab_t* document_storage_get_tab_by_pdfview( GtkWidget* widget ) {
    storage_entry_t* entry;

    entry = ( storage_entry_t* )g_hash_table_lookup(
        view_table,
        ( gconstpointer )widget
    );

    return entry->pdf_tab;
}

document_t* document_storage_get_doc_by_widget( GtkWidget* widget ) {
    storage_entry_t* entry;

    entry = ( storage_entry_t* )g_hash_table_lookup(
        widget_table,
        ( gconstpointer )widget
    );

    return entry->document;
}

document_t* document_storage_get_doc_by_pdfview( GtkWidget* widget ) {
    storage_entry_t* entry;

    entry = ( storage_entry_t* )g_hash_table_lookup(
        view_table,
        ( gconstpointer )widget
    );

    return entry->document;
}

typedef struct storage_iter_data {
    storage_iter_callback_t* callback;
    void* data;
} storage_iter_data_t;

static void document_storage_foreach_helper( gpointer key, gpointer value, gpointer _data ) {
    storage_entry_t* entry;
    storage_iter_data_t* data = ( storage_iter_data_t* )_data;

    entry = ( storage_entry_t* )value;

    data->callback( entry->document, entry->pdf_tab, data->data );
}

int document_storage_foreach( storage_iter_callback_t* callback, void* _data ) {
    storage_iter_data_t data = {
        .callback = callback,
        .data = _data
    };

    g_hash_table_foreach(
        widget_table,
        document_storage_foreach_helper,
        ( gpointer )&data
    );

    return 0;
}

int init_document_storage( void ) {
    view_table = g_hash_table_new(
        g_direct_hash,
        g_direct_equal
    );

    if ( view_table == NULL ) {
        goto error1;
    }

    widget_table = g_hash_table_new(
        g_direct_hash,
        g_direct_equal
    );

    if ( widget_table == NULL ) {
        goto error2;
    }

    document_table = g_hash_table_new(
        g_direct_hash,
        g_direct_equal
    );

    if ( document_table == NULL ) {
        goto error3;
    }

    return 0;

 error3:
    g_object_unref( G_OBJECT( document_table ) );

 error2:
    g_object_unref( G_OBJECT( widget_table ) );

 error1:
    return -ENOMEM;
}
