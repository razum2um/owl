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

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <gtk/gtk.h>

#include <gui/doc_tab.h>
#include <engine/document.h>

typedef struct storage_entry {
    pdf_tab_t* pdf_tab;
    document_t* document;
} storage_entry_t;

typedef int storage_iter_callback_t( document_t* document, pdf_tab_t* pdf_tab, void* data );

int document_storage_add_new( pdf_tab_t* pdf_tab, document_t* doc );
int document_storage_remove( pdf_tab_t* pdf_tab, document_t* doc );

pdf_tab_t* document_storage_get_tab_by_doc( document_t* doc );
pdf_tab_t* document_storage_get_tab_by_widget( GtkWidget* widget );
pdf_tab_t* document_storage_get_tab_by_pdfview( GtkWidget* widget );
document_t* document_storage_get_doc_by_widget( GtkWidget* widget );
document_t* document_storage_get_doc_by_pdfview( GtkWidget* widget );

int document_storage_foreach( storage_iter_callback_t* callback, void* data );

int init_document_storage( void );

#endif /* _STORAGE_H_ */
