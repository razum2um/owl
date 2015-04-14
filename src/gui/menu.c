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

#include <gui/events.h>
#include <gui/menu.h>

static GtkWidget* selection_menu;
static GtkWidget* fullscreen_menu;
static GtkWidget* notebook_menu;

void init_popup_menu( void ) {
    GtkWidget* sel_as_text;
    GtkWidget* sel_as_image;
    GtkWidget* leave_fs;
    GtkWidget* close_tab;
    GtkWidget* close_others;

    /* Selection popup menu */

    selection_menu = gtk_menu_new();

    sel_as_text = gtk_menu_item_new_with_label( "Copy selection text to clipboard" );
    gtk_menu_shell_append( GTK_MENU_SHELL( selection_menu ), sel_as_text );

    g_signal_connect_swapped(
        G_OBJECT( sel_as_text ),
        "activate",
        G_CALLBACK( event_save_selection_as_text ),
        NULL
    );

    gtk_menu_shell_append(
        GTK_MENU_SHELL( selection_menu ),
        gtk_separator_menu_item_new()
    );

    sel_as_image = gtk_menu_item_new_with_label( "Save selection image to file" );
    gtk_menu_shell_append( GTK_MENU_SHELL( selection_menu ), sel_as_image );

    g_signal_connect_swapped(
        G_OBJECT( sel_as_image ),
        "activate",
        G_CALLBACK( event_save_selection_as_image ),
        NULL
    );

    gtk_widget_show_all( selection_menu );

    /* Fullscreen popup menu */

    fullscreen_menu = gtk_menu_new();

    leave_fs = gtk_menu_item_new_with_label( "Exit from presentation mode" );
    gtk_menu_shell_append( GTK_MENU_SHELL( fullscreen_menu ), leave_fs );

    g_signal_connect_swapped(
        G_OBJECT( leave_fs ),
        "activate",
        G_CALLBACK( event_menu_set_fullscreen_mode ),
        NULL
    );

    gtk_widget_show_all( fullscreen_menu );

    /* Notebook menu */

    notebook_menu = gtk_menu_new();

    close_tab = gtk_menu_item_new_with_label( "Close tab" );
    gtk_menu_shell_append( GTK_MENU_SHELL( notebook_menu ), close_tab );

    g_signal_connect_swapped(
        G_OBJECT( close_tab ),
        "activate",
        G_CALLBACK( event_close_this_tab ),
        NULL
    );

    close_others = gtk_menu_item_new_with_label( "Close other tabs" );
    gtk_menu_shell_append( GTK_MENU_SHELL( notebook_menu ), close_others );

    g_signal_connect_swapped(
        G_OBJECT( close_others ),
        "activate",
        G_CALLBACK( event_close_other_tabs ),
        NULL
    );

    gtk_widget_show_all( notebook_menu );
}

void popup_selection_menu( guint button, guint32 time ) {
    gtk_menu_popup(
        GTK_MENU( selection_menu ),
        NULL, NULL, NULL, NULL,
        button,
        time
    );
}

void popup_fullscreen_menu( guint button, guint32 time ) {
    gtk_menu_popup(
        GTK_MENU( fullscreen_menu ),
        NULL, NULL, NULL, NULL,
        button,
        time
    );
}

void popup_notebook_menu( guint button, guint32 time ) {
    gtk_menu_popup(
        GTK_MENU( notebook_menu ),
        NULL, NULL, NULL, NULL,
        button,
        time
    );
}
