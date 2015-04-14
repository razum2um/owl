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

#include <gtk/gtk.h>

#include <gui/main_wnd.h>
#include <gui/events.h>
#include <gui/my_stock.h>
#include <gui/gui-utils.h>
#include <config/settings.h>

my_tool_btn_t* toolbar_open;
my_tool_btn_t* toolbar_zoom_in;
my_tool_btn_t* toolbar_zoom_out;
my_tool_btn_t* toolbar_zoom_page;
my_tool_btn_t* toolbar_zoom_width;
my_tool_btn_t* toolbar_rotate_left;
my_tool_btn_t* toolbar_rotate_right;
my_tool_btn_t* toolbar_prev_page;
my_tool_btn_t* toolbar_next_page;
my_tool_btn_t* toolbar_selection;

static GtkWidget* create_zoom_box( void ) {
    GtkWidget* hbox;
    GtkWidget* label;

    hbox = gtk_hbox_new( FALSE, 3 );

    /* Zoom In button */

    toolbar_zoom_in = create_my_tool_button( MY_STOCK_ZOOM_IN, "Zoom In" );
    gtk_box_pack_start( GTK_BOX( hbox ), toolbar_zoom_in->button, FALSE, FALSE, 0 );

    g_signal_connect(
        G_OBJECT( toolbar_zoom_in->button ),
        "button-press-event",
        G_CALLBACK( event_zoom_in ),
        NULL
    );

    /* Zoom entry */

    entry_zoom = gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY( entry_zoom ), "100" );
    gtk_entry_set_alignment( GTK_ENTRY( entry_zoom ), 1.0f );
    gtk_entry_set_width_chars( GTK_ENTRY( entry_zoom ), 4 );

    g_signal_connect(
        G_OBJECT( entry_zoom ),
        "key-press-event",
        G_CALLBACK( event_set_zoom ),
        NULL
    );

    label = gtk_label_new( "%" );

    gtk_box_pack_start( GTK_BOX( hbox ), entry_zoom, TRUE, TRUE, 0 );
    gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 0 );

    /* Zoom Out button */

    toolbar_zoom_out = create_my_tool_button( MY_STOCK_ZOOM_OUT, "Zoom Out" );
    gtk_box_pack_start( GTK_BOX( hbox ), toolbar_zoom_out->button, FALSE, FALSE, 0 );

    g_signal_connect(
        G_OBJECT( toolbar_zoom_out->button ),
        "button-press-event",
        G_CALLBACK( event_zoom_out ),
        NULL
    );

    toolbar_zoom_width = create_my_tool_button( MY_STOCK_ZOOM_WIDTH, "Fit Width" );
    gtk_box_pack_start( GTK_BOX( hbox ), toolbar_zoom_width->button, FALSE, FALSE, 0 );

    g_signal_connect(
        G_OBJECT( toolbar_zoom_width->button ),
        "clicked",
        G_CALLBACK( event_menu_fit_width ),
        NULL
    );

    toolbar_zoom_page = create_my_tool_button( MY_STOCK_ZOOM_PAGE, "Fit Page" );
    gtk_box_pack_start( GTK_BOX( hbox ), toolbar_zoom_page->button, FALSE, FALSE, 0 );

    g_signal_connect(
        G_OBJECT( toolbar_zoom_page->button ),
        "clicked",
        G_CALLBACK( event_menu_fit_page ),
        NULL
    );

    GTK_WIDGET_UNSET_FLAGS( toolbar_zoom_in->button, GTK_CAN_FOCUS );
    GTK_WIDGET_UNSET_FLAGS( toolbar_zoom_out->button, GTK_CAN_FOCUS );
    GTK_WIDGET_UNSET_FLAGS( toolbar_zoom_page->button, GTK_CAN_FOCUS );
    GTK_WIDGET_UNSET_FLAGS( toolbar_zoom_width->button, GTK_CAN_FOCUS );

    return hbox;
}

static GtkWidget* create_rotate_box( void ) {
    GtkWidget* hbox;

    hbox = gtk_hbox_new( FALSE, 3 );

    /* Rotate Left button */

    toolbar_rotate_left = create_my_tool_button( MY_STOCK_ROTATE_LEFT, "Rotate Left" );
    gtk_box_pack_start( GTK_BOX( hbox ), toolbar_rotate_left->button, FALSE, FALSE, 0 );

    g_signal_connect(
        G_OBJECT( toolbar_rotate_left->button ),
        "button-press-event",
        G_CALLBACK( event_rotate_left ),
        NULL
    );

    /* Rotate Right button */

    toolbar_rotate_right = create_my_tool_button( MY_STOCK_ROTATE_RIGHT, "Rotate Right" );
    gtk_box_pack_start( GTK_BOX( hbox ), toolbar_rotate_right->button, FALSE, FALSE, 0 );

    g_signal_connect(
        G_OBJECT( toolbar_rotate_right->button ),
        "button-press-event",
        G_CALLBACK( event_rotate_right ),
        NULL
    );

    GTK_WIDGET_UNSET_FLAGS( toolbar_rotate_left->button, GTK_CAN_FOCUS );
    GTK_WIDGET_UNSET_FLAGS( toolbar_rotate_right->button, GTK_CAN_FOCUS );

    return hbox;
}

GtkWidget* create_tool_bar( void ) {
    GtkWidget* toolbar;
    GtkWidget* zoom_box;
    GtkWidget* rotate_box;
    GtkToolItem* toolitem;

    toolbar = gtk_toolbar_new();

    toolitem = gtk_tool_item_new();
    toolbar_open = create_my_tool_button( GTK_STOCK_OPEN, "Open" );
    gtk_container_add( GTK_CONTAINER( toolitem ), toolbar_open->button );

    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    g_signal_connect(
        G_OBJECT( toolbar_open->button ),
        "clicked",
        G_CALLBACK( event_menu_open_document ),
        NULL
    );

    /* Zooming */

    toolitem = gtk_separator_tool_item_new();
    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    toolitem = gtk_tool_item_new();
    zoom_box = create_zoom_box();
    gtk_container_add( GTK_CONTAINER( toolitem ), zoom_box );

    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    /* Rotation */

    toolitem = gtk_separator_tool_item_new();
    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    toolitem = gtk_tool_item_new();
    rotate_box = create_rotate_box();
    gtk_container_add( GTK_CONTAINER( toolitem ), rotate_box );

    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    /* Prev & next page */

    toolitem = gtk_separator_tool_item_new();
    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    toolitem = gtk_tool_item_new();
    toolbar_prev_page = create_my_tool_button( MY_STOCK_PREV_PAGE_LARGE, "Prev Page" );
    GTK_WIDGET_UNSET_FLAGS( toolbar_prev_page->button, GTK_CAN_FOCUS );
    gtk_container_add( GTK_CONTAINER( toolitem ), toolbar_prev_page->button );

    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    g_signal_connect(
        G_OBJECT( toolbar_prev_page->button ),
        "clicked",
        G_CALLBACK( event_goto_prev_page_toolbar ),
        NULL
    );

    toolitem = gtk_tool_item_new();
    toolbar_next_page = create_my_tool_button( MY_STOCK_NEXT_PAGE_LARGE, "Next Page" );
    GTK_WIDGET_UNSET_FLAGS( toolbar_next_page->button, GTK_CAN_FOCUS );
    gtk_container_add( GTK_CONTAINER( toolitem ), toolbar_next_page->button );

    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    g_signal_connect(
        G_OBJECT( toolbar_next_page->button ),
        "clicked",
        G_CALLBACK( event_goto_next_page_toolbar ),
        NULL
    );

    /* Selection */

    toolitem = gtk_separator_tool_item_new();
    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    toolitem = gtk_tool_item_new();
    toolbar_selection = create_my_toggle_tool_button( MY_STOCK_SELECTION, "Selection" );
    GTK_WIDGET_UNSET_FLAGS( toolbar_selection->button, GTK_CAN_FOCUS );
    gtk_container_add( GTK_CONTAINER( toolitem ), toolbar_selection->button );

    gtk_toolbar_insert( GTK_TOOLBAR( toolbar ), toolitem, -1 );

    g_signal_connect(
        G_OBJECT( toolbar_selection->button ),
        "toggled",
        G_CALLBACK( event_selection_mode_toggled ),
        NULL
    );

    return toolbar;
}

static void update_tool_button_style( my_tool_btn_t* button, int style ) {
    switch ( style ) {
        case T_STYLE_IMAGE_AND_TEXT :
            gtk_widget_show( button->image );
            gtk_widget_show( button->label );
            break;

        case T_STYLE_IMAGE :
            gtk_widget_show( button->image );
            gtk_widget_hide( button->label );
            break;

        case T_STYLE_TEXT :
            gtk_widget_hide( button->image );
            gtk_widget_show( button->label );
            break;
    }
}

void toolbar_update_style( void ) {
    int style;

    if ( settings_get_int( "tool-button-style", &style ) < 0 ) {
        return;
    }

    update_tool_button_style( toolbar_open, style );
    update_tool_button_style( toolbar_zoom_in, style );
    update_tool_button_style( toolbar_zoom_out, style );
    update_tool_button_style( toolbar_zoom_page, style );
    update_tool_button_style( toolbar_zoom_width, style );
    update_tool_button_style( toolbar_rotate_left, style );
    update_tool_button_style( toolbar_rotate_right, style );
    update_tool_button_style( toolbar_prev_page, style );
    update_tool_button_style( toolbar_next_page, style );
    update_tool_button_style( toolbar_selection, style );
}
