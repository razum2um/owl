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

#include <utils.h>
#include <gui/gui-utils.h>
#include <gui/my_stock.h>
#include <gui/events.h>

static GdkCursor* invisible_cursor = NULL;

static GtkFileFilter* all_file_filter = NULL;
static GtkFileFilter* pdf_file_filter = NULL;

my_tool_btn_t* create_my_tool_button( const gchar* stock_name, const gchar* title ) {
    GtkWidget* vbox;
    my_tool_btn_t* tool_btn;

    tool_btn = ( my_tool_btn_t* )malloc( sizeof( my_tool_btn_t ) );

    if ( tool_btn == NULL ) {
        return NULL;
    }

    tool_btn->button = gtk_button_new();
    gtk_button_set_relief( GTK_BUTTON( tool_btn->button ), GTK_RELIEF_NONE );

    vbox = gtk_vbox_new( FALSE, 3 );
    gtk_container_add( GTK_CONTAINER( tool_btn->button ), vbox );

    tool_btn->image = gtk_image_new_from_stock( stock_name, GTK_ICON_SIZE_LARGE_TOOLBAR );
    gtk_box_pack_start( GTK_BOX( vbox ), tool_btn->image, FALSE, FALSE, 0 );

    tool_btn->label = gtk_label_new( title );
    gtk_box_pack_start( GTK_BOX( vbox ), tool_btn->label, FALSE, FALSE, 0 );

    return tool_btn;
}

my_tool_btn_t* create_my_toggle_tool_button( const gchar* stock_name, const gchar* title ) {
    GtkWidget* vbox;
    my_tool_btn_t* tool_btn;

    tool_btn = ( my_tool_btn_t* )malloc( sizeof( my_tool_btn_t ) );

    if ( tool_btn == NULL ) {
        return NULL;
    }

    tool_btn->button = gtk_toggle_button_new();
    gtk_button_set_relief( GTK_BUTTON( tool_btn->button ), GTK_RELIEF_NONE );

    vbox = gtk_vbox_new( FALSE, 3 );
    gtk_container_add( GTK_CONTAINER( tool_btn->button ), vbox );

    tool_btn->image = gtk_image_new_from_stock( stock_name, GTK_ICON_SIZE_LARGE_TOOLBAR );
    gtk_box_pack_start( GTK_BOX( vbox ), tool_btn->image, FALSE, FALSE, 0 );

    tool_btn->label = gtk_label_new( title );
    gtk_box_pack_start( GTK_BOX( vbox ), tool_btn->label, FALSE, FALSE, 0 );

    return tool_btn;
}

GtkWidget* create_doc_tab_label( pdf_tab_t* pdf_tab, document_t* document ) {
    char path[ 256 ];
    GtkWidget* hbox;
    GtkWidget* image;
    GtkWidget* label;
    GtkWidget* button;
    GtkWidget* event_box;

    event_box = gtk_event_box_new();

    hbox = gtk_hbox_new( FALSE, 3 );
    gtk_container_add( GTK_CONTAINER( event_box ), hbox );

    snprintf(
        path,
        sizeof( path ),
        "%s%spixmaps%stab-close.png",
        OWL_INSTALL_PATH,
        PATH_SEPARATOR,
        PATH_SEPARATOR
    );

    image = gtk_image_new_from_file( path );

    label = gtk_label_new( document->filename );
    gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 2 );

    button = gtk_button_new();
    gtk_button_set_image( GTK_BUTTON( button ), image );
    gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_NONE );
    gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 2 );

    g_signal_connect(
        G_OBJECT( event_box ),
        "button-press-event",
        G_CALLBACK( event_doctab_header_click ),
        NULL
    );

    g_signal_connect(
        G_OBJECT( button ),
        "clicked",
        G_CALLBACK( event_close_tab ),
        pdf_tab
    );

    gtk_widget_show_all( event_box );

    return event_box;
}

GdkCursor* get_invisible_cursor( void ) {
    if ( invisible_cursor != NULL ) {
        return invisible_cursor;
    }

    char source_bits[] = { 0x00 };
    char mask_bits[] = { 0x00 };
    GdkColor color = { 0, 0, 0, 0 };
    GdkPixmap* source;
    GdkPixmap* mask;

    source = gdk_bitmap_create_from_data( NULL, source_bits, 1, 1 );
    mask = gdk_bitmap_create_from_data( NULL, mask_bits, 1, 1 );

    invisible_cursor = gdk_cursor_new_from_pixmap( source, mask, &color, &color, 0, 0 );

    gdk_pixmap_unref( source );
    gdk_pixmap_unref( mask );

    return invisible_cursor;
}

GtkFileFilter* get_all_file_filter( void ) {
    if ( all_file_filter != NULL ) {
        return all_file_filter;
    }

    all_file_filter = gtk_file_filter_new();
    g_object_ref( G_OBJECT( all_file_filter ) );

    gtk_file_filter_set_name( all_file_filter, "All files" );
    gtk_file_filter_add_pattern( all_file_filter, "*" );

    return all_file_filter;
}

GtkFileFilter* get_pdf_file_filter( void ) {
    if ( pdf_file_filter != NULL ) {
        return pdf_file_filter;
    }

    pdf_file_filter = gtk_file_filter_new();
    g_object_ref( G_OBJECT( pdf_file_filter ) );

    gtk_file_filter_set_name( pdf_file_filter, "PDF files" );
    gtk_file_filter_add_pattern( pdf_file_filter, "*.pdf" );

    return pdf_file_filter;
}
