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
#include <gdk/gdktypes.h>
#include <gdk/gdkkeysyms.h>

#include <gui/main_wnd.h>
#include <gui/events.h>
#include <gui/main_wnd.h>
#include <gui/my_stock.h>

GtkWidget* view_fullscreen_mode;
GtkWidget* view_cont_mode;

GtkWidget* create_menu_bar( void ) {
    GtkWidget* tmp;
    GtkWidget* menu_bar;

    GtkWidget* file_menu_item;
    GtkWidget* file_menu;
    GtkWidget* file_open;
    GtkWidget* file_save_as;
    GtkWidget* file_close_tab;
    GtkWidget* file_close_all_tab;
    GtkWidget* file_settings;
    GtkWidget* file_preferences;
    GtkWidget* file_page_setup;
    GtkWidget* file_print;
    GtkWidget* file_exit;

    GtkWidget* view_menu_item;
    GtkWidget* view_menu;
#if 0 /* ... not yet ... */
    GtkWidget* view_back_in_doc;
    GtkWidget* view_forward_in_doc;
#endif
    GtkWidget* view_zoom_in;
    GtkWidget* view_zoom_out;
    GtkWidget* view_fit_width;
    GtkWidget* view_fit_page;
    GtkWidget* view_rotate_left;
    GtkWidget* view_rotate_right;
    GtkWidget* view_reverse_pages;
    GtkWidget* view_multipage_submenu_item;
    GtkWidget* view_multipage_submenu;

    GtkWidget* page_menu_item;
    GtkWidget* page_menu;
    GtkWidget* page_zoom_in;
    GtkWidget* page_zoom_out;
    GtkWidget* page_rotate_left;
    GtkWidget* page_rotate_right;

    GtkWidget* go_menu_item;
    GtkWidget* go_menu;
    GtkWidget* go_prev_page;
    GtkWidget* go_next_page;
    GtkWidget* go_first_page;
    GtkWidget* go_last_page;
    GtkWidget* go_prev_doc;
    GtkWidget* go_next_doc;

    GtkWidget* help_menu_item;
    GtkWidget* help_menu;
    GtkWidget* help_about;

    GtkAccelGroup* accel_group;

    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group( GTK_WINDOW( main_window ), accel_group );

    /* File menu */

    file_menu = gtk_menu_new();
    file_menu_item = gtk_menu_item_new_with_mnemonic( "_File" );
    gtk_menu_item_set_submenu( GTK_MENU_ITEM( file_menu_item ), file_menu );

    file_open = gtk_image_menu_item_new_from_stock( GTK_STOCK_OPEN, NULL );

    gtk_widget_add_accelerator(
        file_open,
        "activate",
        accel_group,
        GDK_o,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_open );

    g_signal_connect(
        G_OBJECT( file_open ),
        "activate",
        G_CALLBACK( event_menu_open_document ),
        NULL
    );

    file_save_as = gtk_image_menu_item_new_from_stock( GTK_STOCK_SAVE_AS, NULL );

    gtk_widget_add_accelerator(
        file_save_as,
        "activate",
        accel_group,
        GDK_a,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_save_as );

    g_signal_connect(
        G_OBJECT( file_save_as ),
        "activate",
        G_CALLBACK( event_menu_save_as_document ),
        NULL
    );

    file_close_tab = gtk_menu_item_new_with_label( "Close tab" );

    gtk_widget_add_accelerator(
        file_close_tab,
        "activate",
        accel_group,
        GDK_w,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_close_tab );

    g_signal_connect(
        G_OBJECT( file_close_tab ),
        "activate",
        G_CALLBACK( event_menu_close_current_document ),
        NULL
    );

    file_close_all_tab = gtk_menu_item_new_with_label( "Close all tab" );

    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_close_all_tab );

    g_signal_connect(
        G_OBJECT( file_close_all_tab ),
        "activate",
        G_CALLBACK( event_menu_close_all_document ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), tmp );

    file_settings = gtk_menu_item_new_with_label( "Settings" );
    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_settings );

    gtk_widget_add_accelerator(
        file_settings,
        "activate",
        accel_group,
        GDK_s,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( file_settings ),
        "activate",
        G_CALLBACK( event_menu_open_settings ),
        NULL
    );

    file_preferences = gtk_menu_item_new_with_label( "Properties" );
    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_preferences );

    gtk_widget_add_accelerator(
        file_preferences,
        "activate",
        accel_group,
        GDK_d,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( file_preferences ),
        "activate",
        G_CALLBACK( event_menu_open_preferences ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), tmp );

    file_page_setup = gtk_menu_item_new_with_label( "Page setup..." );
    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_page_setup );

    g_signal_connect(
        G_OBJECT( file_page_setup ),
        "activate",
        G_CALLBACK( event_menu_page_setup ),
        NULL
    );

    file_print = gtk_menu_item_new_with_label( "Print..." );
    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_print );

    gtk_widget_add_accelerator(
        file_print,
        "activate",
        accel_group,
        GDK_p,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( file_print ),
        "activate",
        G_CALLBACK( event_menu_print ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), tmp );

    file_exit = gtk_image_menu_item_new_from_stock( GTK_STOCK_QUIT, NULL );

    gtk_widget_add_accelerator(
        file_exit,
        "activate",
        accel_group,
        GDK_q,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( file_menu ), file_exit );

    g_signal_connect(
        G_OBJECT( file_exit ),
        "activate",
        G_CALLBACK( event_fileexit_activated ),
        NULL
    );

    /* View menu */

    view_menu = gtk_menu_new();
    view_menu_item = gtk_menu_item_new_with_mnemonic( "_View" );
    gtk_menu_item_set_submenu( GTK_MENU_ITEM( view_menu_item ), view_menu );

    view_zoom_in = gtk_image_menu_item_new_from_stock( MY_STOCK_ZOOM_IN, NULL );

    gtk_widget_add_accelerator(
        view_zoom_in,
        "activate",
        accel_group,
        GDK_KP_Add,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_widget_add_accelerator(
        view_zoom_in,
        "activate",
        accel_group,
        GDK_plus,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_zoom_in );

    g_signal_connect(
        G_OBJECT( view_zoom_in ),
        "activate",
        G_CALLBACK( event_menu_zoom_in ),
        NULL
    );

    view_zoom_out = gtk_image_menu_item_new_from_stock( MY_STOCK_ZOOM_OUT, NULL );

    gtk_widget_add_accelerator(
        view_zoom_out,
        "activate",
        accel_group,
        GDK_KP_Subtract,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_widget_add_accelerator(
        view_zoom_out,
        "activate",
        accel_group,
        GDK_minus,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_zoom_out );

    g_signal_connect(
        G_OBJECT( view_zoom_out ),
        "activate",
        G_CALLBACK( event_menu_zoom_out ),
        NULL
    );

    view_fit_width = gtk_image_menu_item_new_from_stock( MY_STOCK_ZOOM_WIDTH, NULL );

    gtk_widget_add_accelerator(
        view_fit_width,
        "activate",
        accel_group,
        GDK_w,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_fit_width );

    g_signal_connect(
        G_OBJECT( view_fit_width ),
        "activate",
        G_CALLBACK( event_menu_fit_width ),
        NULL
    );

    view_fit_page = gtk_image_menu_item_new_from_stock( MY_STOCK_ZOOM_PAGE, NULL );

    gtk_widget_add_accelerator(
        view_fit_page,
        "activate",
        accel_group,
        GDK_p,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_fit_page );

    g_signal_connect(
        G_OBJECT( view_fit_page ),
        "activate",
        G_CALLBACK( event_menu_fit_page ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), tmp );

    view_rotate_left = gtk_image_menu_item_new_from_stock( MY_STOCK_ROTATE_LEFT, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_rotate_left );

    gtk_widget_add_accelerator(
        view_rotate_left,
        "activate",
        accel_group,
        GDK_l,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( view_rotate_left ),
        "activate",
        G_CALLBACK( event_menu_rotate_left ),
        NULL
    );

    view_rotate_right = gtk_image_menu_item_new_from_stock( MY_STOCK_ROTATE_RIGHT, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_rotate_right );

    gtk_widget_add_accelerator(
        view_rotate_right,
        "activate",
        accel_group,
        GDK_r,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( view_rotate_right ),
        "activate",
        G_CALLBACK( event_menu_rotate_right ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), tmp );

#if 0 /* don't fuck with us! */
    view_back_in_doc = gtk_menu_item_new_with_label( "Back in the Document" );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_back_in_doc );

    g_signal_connect(
        G_OBJECT( view_back_in_doc ),
        "activate",
        G_CALLBACK( event_menu_back_in_doc ),
        NULL
    );

    view_forward_in_doc = gtk_menu_item_new_with_label( "Forward in the Document" );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_forward_in_doc );

    g_signal_connect(
        G_OBJECT( view_forward_in_doc ),
        "activate",
        G_CALLBACK( event_menu_forward_in_doc ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), tmp );
#endif

    view_reverse_pages = gtk_menu_item_new_with_label( "Reverse pages" );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_reverse_pages );

    g_signal_connect(
        G_OBJECT( view_reverse_pages ),
        "activate",
        G_CALLBACK( event_menu_reverse_pages ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), tmp );

    view_cont_mode = gtk_check_menu_item_new_with_label( "Continuous Mode" );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_cont_mode );

    gtk_widget_add_accelerator(
        view_cont_mode,
        "activate",
        accel_group,
        GDK_c,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( view_cont_mode ),
        "toggled",
        G_CALLBACK( event_menu_toggle_cont_mode ),
        NULL
    );

    view_fullscreen_mode = gtk_check_menu_item_new_with_label( "Presentation Mode" );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_fullscreen_mode );

    gtk_widget_add_accelerator(
        view_fullscreen_mode,
        "activate",
        accel_group,
        GDK_F11,
        0,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( view_fullscreen_mode ),
        "activate",
        G_CALLBACK( event_menu_set_fullscreen_mode ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), tmp );

    view_multipage_submenu_item = gtk_menu_item_new_with_label( "Multi-page mode" );
    gtk_menu_shell_append( GTK_MENU_SHELL( view_menu ), view_multipage_submenu_item );

    view_multipage_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu( GTK_MENU_ITEM( view_multipage_submenu_item ), view_multipage_submenu);

    int i;
    GSList* group = NULL;
    int key_table[] = { GDK_1, GDK_2, GDK_3, GDK_4, GDK_5 };

    for ( i = 1; i <= 5; i++ ) {
        char label[ 64 ];

        if ( i == 1 ) {
            snprintf( label, sizeof( label ), "%d page", i );
        } else {
            snprintf( label, sizeof( label ), "%d pages", i );
        }

        tmp = gtk_radio_menu_item_new_with_label( group, label );
        gtk_widget_add_accelerator(
            tmp,
            "activate",
            accel_group,
            key_table[ i - 1 ],
            GDK_CONTROL_MASK,
            GTK_ACCEL_VISIBLE
        );

        group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM( tmp ) );
        gtk_menu_shell_append( GTK_MENU_SHELL( view_multipage_submenu ), tmp );

        int* cols = ( int* )malloc( sizeof( int ) );

        if ( cols == NULL ) {
            break;
        }

        *cols = i;

        g_signal_connect(
            G_OBJECT( tmp ),
            "activate",
            G_CALLBACK( event_menu_set_multipage ),
            ( gpointer )cols
        );
    }

    /* Page menu */

    page_menu = gtk_menu_new();
    page_menu_item = gtk_menu_item_new_with_mnemonic( "_Page" );
    gtk_menu_item_set_submenu( GTK_MENU_ITEM( page_menu_item ), page_menu );

    page_zoom_in = gtk_image_menu_item_new_from_stock( MY_STOCK_ZOOM_IN, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL( page_menu ), page_zoom_in );

    gtk_widget_add_accelerator(
        page_zoom_in,
        "activate",
        accel_group,
        GDK_KP_Add,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_widget_add_accelerator(
        page_zoom_in,
        "activate",
        accel_group,
        GDK_plus,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( page_zoom_in ),
        "activate",
        G_CALLBACK( event_menu_zoom_page_in ),
        NULL
    );

    page_zoom_out = gtk_image_menu_item_new_from_stock( MY_STOCK_ZOOM_OUT, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL( page_menu ), page_zoom_out );

    gtk_widget_add_accelerator(
        page_zoom_out,
        "activate",
        accel_group,
        GDK_KP_Subtract,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_widget_add_accelerator(
        page_zoom_out,
        "activate",
        accel_group,
        GDK_minus,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( page_zoom_out ),
        "activate",
        G_CALLBACK( event_menu_zoom_page_out ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( page_menu ), tmp );

    page_rotate_left = gtk_image_menu_item_new_from_stock( MY_STOCK_ROTATE_LEFT, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL( page_menu ), page_rotate_left );

    gtk_widget_add_accelerator(
        page_rotate_left,
        "activate",
        accel_group,
        GDK_l,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( page_rotate_left ),
        "activate",
        G_CALLBACK( event_menu_rotate_page_left ),
        NULL
    );

    page_rotate_right = gtk_image_menu_item_new_from_stock( MY_STOCK_ROTATE_RIGHT, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL( page_menu ), page_rotate_right );

    gtk_widget_add_accelerator(
        page_rotate_right,
        "activate",
        accel_group,
        GDK_r,
        GDK_SHIFT_MASK | GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( page_rotate_right ),
        "activate",
        G_CALLBACK( event_menu_rotate_page_right ),
        NULL
    );

    /* Go menu */

    go_menu = gtk_menu_new();
    go_menu_item = gtk_menu_item_new_with_mnemonic( "_Go" );
    gtk_menu_item_set_submenu( GTK_MENU_ITEM( go_menu_item ), go_menu );

    go_prev_page = gtk_menu_item_new_with_label( "Previous Page" );

    gtk_widget_add_accelerator(
        go_prev_page,
        "activate",
        accel_group,
        GDK_Up,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), go_prev_page );

    g_signal_connect(
        G_OBJECT( go_prev_page ),
        "activate",
        G_CALLBACK( event_menu_prev_page ),
        NULL
    );

    go_next_page = gtk_menu_item_new_with_label( "Next Page" );

    gtk_widget_add_accelerator(
        go_next_page,
        "activate",
        accel_group,
        GDK_Down,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), go_next_page );

    g_signal_connect(
        G_OBJECT( go_next_page ),
        "activate",
        G_CALLBACK( event_menu_next_page ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), tmp );

    go_first_page = gtk_menu_item_new_with_label( "First Page" );
    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), go_first_page );

    g_signal_connect(
        G_OBJECT( go_first_page ),
        "activate",
        G_CALLBACK( event_menu_first_page ),
        NULL
    );

    go_last_page = gtk_menu_item_new_with_label( "Last Page" );
    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), go_last_page );

    g_signal_connect(
        G_OBJECT( go_last_page ),
        "activate",
        G_CALLBACK( event_menu_last_page ),
        NULL
    );

    tmp = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), tmp );

    go_prev_doc = gtk_menu_item_new_with_label( "Prev document" );
    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), go_prev_doc );

    gtk_widget_add_accelerator(
        go_prev_doc,
        "activate",
        accel_group,
        GDK_Page_Up,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( go_prev_doc ),
        "activate",
        G_CALLBACK( event_menu_prev_doc ),
        NULL
    );

    go_next_doc = gtk_menu_item_new_with_label( "Next document" );
    gtk_menu_shell_append( GTK_MENU_SHELL( go_menu ), go_next_doc );

    gtk_widget_add_accelerator(
        go_next_doc,
        "activate",
        accel_group,
        GDK_Page_Down,
        GDK_CONTROL_MASK,
        GTK_ACCEL_VISIBLE
    );

    g_signal_connect(
        G_OBJECT( go_next_doc ),
        "activate",
        G_CALLBACK( event_menu_next_doc ),
        NULL
    );

    /* Help menu */

    help_menu = gtk_menu_new();
    help_menu_item = gtk_menu_item_new_with_mnemonic( "_Help" );
    gtk_menu_item_set_submenu( GTK_MENU_ITEM( help_menu_item ), help_menu );

    help_about = gtk_image_menu_item_new_from_stock( GTK_STOCK_ABOUT, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL( help_menu ), help_about );

    g_signal_connect(
        G_OBJECT( help_about ),
        "activate",
        G_CALLBACK( event_show_about ),
        NULL
    );

    /* The menubar */

    menu_bar = gtk_menu_bar_new();
    gtk_menu_shell_append( GTK_MENU_SHELL( menu_bar ), file_menu_item );
    gtk_menu_shell_append( GTK_MENU_SHELL( menu_bar ), view_menu_item );
    gtk_menu_shell_append( GTK_MENU_SHELL( menu_bar ), page_menu_item );
    gtk_menu_shell_append( GTK_MENU_SHELL( menu_bar ), go_menu_item );
    gtk_menu_shell_append( GTK_MENU_SHELL( menu_bar ), help_menu_item );

    return menu_bar;
}
