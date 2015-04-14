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

#include <gtk/gtk.h>

#include <storage.h>
#include <config/settings.h>
#include <engine/render.h>
#include <gui/main_wnd.h>
#include <gui/pdf_view.h>
#include <gui/settings.h>
#include <gui/events.h>

static GtkWidget* check_show_docs_from_last_time;
static GtkWidget* check_always_show_tabs;
static GtkWidget* check_show_navbar;
static GtkWidget* check_show_toolbar;
static GtkWidget* check_hide_pointer;
static GtkWidget* check_open_docs_in_running;
static GtkWidget* spin_pointer_idle_time;
static GtkWidget* entry_browser_path;
static GtkWidget* combo_tool_btn_style;

static GtkWidget* color_button_selection;
static GtkWidget* color_button_highlight;

static GtkWidget* check_enable_render_limit_pages;
static GtkWidget* spin_render_limit_pages;
static GtkWidget* check_enable_render_limit_mb;
static GtkWidget* spin_render_limit_mb;

static void check_button_propagate_sensitivity( GtkToggleButton* toggle_button, gpointer data ) {
    gboolean active;
    GtkWidget* widget;

    active = gtk_toggle_button_get_active( toggle_button );
    widget = ( GtkWidget* )data;

    gtk_widget_set_sensitive( widget, active );
}

static void check_button_assign_sensitivity( GtkWidget* check_button, GtkWidget* widget ) {
    check_button_propagate_sensitivity( GTK_TOGGLE_BUTTON( check_button ), widget );

    g_signal_connect(
        G_OBJECT( check_button ),
        "toggled",
        G_CALLBACK( check_button_propagate_sensitivity ),
        widget
    );
}

static GtkWidget* create_check_button( char* settings_key, GtkWidget* vbox ) {
    GtkWidget* check_button;
    char* label;
    int value;

    if ( settings_get_int( settings_key, &value ) < 0 ) {
        return NULL;
    }

    label = settings_get_desc( settings_key );

    if ( label == NULL ) {
        return NULL;
    }

    check_button = gtk_check_button_new_with_label( label );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_button ), value );

    gtk_box_pack_start( GTK_BOX( vbox ), check_button, FALSE, FALSE, 0 );

    return check_button;
}

static void set_from_check_button( GtkWidget* check_button, char* settings_key, void ( *callback )( char* key ) ) {
    int value_old;
    int value_new;

    if ( settings_get_int( settings_key, &value_old ) < 0 ) {
        return;
    }

    value_new = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( check_button ) );

    if ( value_new != value_old ) {
        settings_set_int( settings_key, value_new );

        if ( callback != NULL ) {
            callback( settings_key );
        }
    }
}

static GtkWidget* create_color_button( char* settings_key, GtkWidget* vbox ) {
    GtkWidget* hbox;
    GtkWidget* color_button;
    GdkColor color;
    char* label;

    if ( settings_get_color( settings_key, &color ) < 0 ) {
        return NULL;
    }

    label = settings_get_desc( settings_key );

    if ( label == NULL ) {
        return NULL;
    }

    color_button = gtk_color_button_new_with_color( &color );

    gtk_color_button_set_title( GTK_COLOR_BUTTON( color_button ), label );

    hbox = gtk_hbox_new( FALSE, 3 );

    gtk_box_pack_start( GTK_BOX( hbox ), color_button, FALSE, FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( hbox ), gtk_label_new( label ), FALSE, FALSE, 5 );

    gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 0 );

    return color_button;
}

static void set_from_color_button( GtkWidget* color_button, char* settings_key, void ( *callback )( char* key ) ) {
    GdkColor color_old;
    GdkColor color_new;

    if ( settings_get_color( settings_key, &color_old ) < 0 ) {
        return;
    }

    gtk_color_button_get_color( GTK_COLOR_BUTTON( color_button ), &color_new );

    if ( ( color_new.red != color_old.red ) ||
         ( color_new.green != color_old.green ) ||
         ( color_new.blue != color_old.blue ) ) {
        settings_set_color( settings_key, &color_new );

        if ( callback != NULL ) {
            callback( settings_key );
        }
    }
}

static GtkWidget* create_spin_button( char* settings_key, int min, int max, GtkWidget* vbox, GtkWidget** hbox ) {
    GtkWidget* hbox_internal;
    GtkWidget* spin_button;
    char* label;
    int value;

    if ( settings_get_int( settings_key, &value ) < 0 ) {
        return NULL;
    }

    label = settings_get_desc( settings_key );

    if ( label == NULL ) {
        return NULL;
    }

    spin_button = gtk_spin_button_new_with_range( min, max, 1.0 );

    gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( spin_button ), TRUE );
    gtk_spin_button_set_wrap( GTK_SPIN_BUTTON( spin_button ), FALSE );
    gtk_spin_button_set_digits( GTK_SPIN_BUTTON( spin_button ), 0 );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON( spin_button ), value );

    hbox_internal = gtk_hbox_new( FALSE, 3 );

    if ( hbox != NULL ) {
        *hbox = hbox_internal;

        /* hbox is tracked, give it a nice padding on the left */
        gtk_box_pack_start( GTK_BOX( hbox_internal ), gtk_label_new( "" ), FALSE, FALSE, 10 );
    }

    gtk_box_pack_start( GTK_BOX( hbox_internal ), gtk_label_new( label ), FALSE, FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( hbox_internal ), spin_button, FALSE, FALSE, 0 );

    gtk_box_pack_start( GTK_BOX( vbox ), hbox_internal, FALSE, FALSE, 0 );

    return spin_button;
}

static void set_from_spin_button( GtkWidget* spin_button, char* settings_key, void ( *callback )( char* key ) ) {
    int value_old;
    int value_new;

    if ( settings_get_int( settings_key, &value_old ) < 0 ) {
        return;
    }

    value_new = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON( spin_button ) );

    if ( value_new != value_old ) {
        settings_set_int( settings_key, value_new );

        if ( callback != NULL ) {
            callback( settings_key );
        }
    }
}

static GtkWidget* create_entry( char* settings_key, GtkWidget* vbox, GtkWidget** hbox ) {
    GtkWidget* entry;
    GtkWidget* hbox_internal;
    char* label;
    char* value;

    if ( settings_get_string( settings_key, &value ) < 0 ) {
        return NULL;
    }

    label = settings_get_desc( settings_key );

    if ( label == NULL ) {
        return NULL;
    }

    entry = gtk_entry_new();

    if ( strlen( value ) > 0 ) {
        gtk_entry_set_text( GTK_ENTRY( entry ), value );
    }

    hbox_internal = gtk_hbox_new( FALSE, 3 );

    if ( hbox != NULL ) {
        *hbox = hbox_internal;
    }

    gtk_box_pack_start( GTK_BOX( hbox_internal ), gtk_label_new( label ), FALSE, FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( hbox_internal ), entry, FALSE, FALSE, 0 );

    gtk_box_pack_start( GTK_BOX( vbox ), hbox_internal, FALSE, FALSE, 0 );

    return entry;
}

static void set_from_entry( GtkWidget* entry, char* settings_key, void ( *callback )( char* key ) ) {
    char* value_old;
    char* value_new;

    if ( settings_get_string( settings_key, &value_old ) < 0 ) {
        return;
    }

    value_new = ( char* )gtk_entry_get_text( GTK_ENTRY( entry ) );

    if ( strcmp( value_old, value_new ) != 0 ) {
        settings_set_string( settings_key, value_new );

        if ( callback != NULL ) {
            callback( settings_key );
        }
    }
}

static GtkWidget* create_combobox_with_mapping( char* settings_key, GtkWidget* vbox, const char** values, GtkWidget** hbox  ) {
    int i;
    GtkWidget* combobox;
    GtkWidget* hbox_internal;
    char* label;
    int value;

    if ( settings_get_int( settings_key, &value ) < 0 ) {
        return NULL;
    }

    label = settings_get_desc( settings_key );

    if ( label == NULL ) {
        return NULL;
    }

    combobox = gtk_combo_box_new_text();

    for ( i = 0; values[ i ] != NULL; i++ ) {
        gtk_combo_box_append_text( GTK_COMBO_BOX( combobox ), values[ i ] );
    }

    gtk_combo_box_set_active( GTK_COMBO_BOX( combobox ), value );

    hbox_internal = gtk_hbox_new( FALSE, 3 );

    if ( hbox != NULL ) {
        *hbox = hbox_internal;
    }

    gtk_box_pack_start( GTK_BOX( hbox_internal ), gtk_label_new( label ), FALSE, FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( hbox_internal ), combobox, FALSE, FALSE, 0 );

    gtk_box_pack_start( GTK_BOX( vbox ), hbox_internal, FALSE, FALSE, 0 );

    return combobox;
}

static void set_from_combobox( GtkWidget* combobox, char* settings_key, void ( *callback )( char* key ) ) {
    int value_old;
    int value_new;

    if ( settings_get_int( settings_key, &value_old ) < 0 ) {
        return;
    }

    value_new = gtk_combo_box_get_active( GTK_COMBO_BOX( combobox ) );

    if ( value_old != value_new ) {
        settings_set_int( settings_key, value_new );

        if ( callback != NULL ) {
            callback( settings_key );
        }
    }

}

static void always_show_tabs_cb( char* key ) {
    int show_tabs;

    if ( settings_get_int( key, &show_tabs ) < 0 ) {
        return;
    }

    if ( show_tabs ) {
        gtk_notebook_set_show_tabs( GTK_NOTEBOOK( main_notebook ), TRUE );
    } else {
        if ( gtk_notebook_get_n_pages( GTK_NOTEBOOK( main_notebook ) ) == 1 ) {
            gtk_notebook_set_show_tabs( GTK_NOTEBOOK( main_notebook ), FALSE );
        }
    }
}

static int hide_navbar_helper( document_t* document, pdf_tab_t* pdf_tab, void* data ) {
    gtk_widget_hide( pdf_tab->navigator_bar );
    return 0;
}

static int show_navbar_helper( document_t* document, pdf_tab_t* pdf_tab, void* data ) {
    gtk_widget_show( pdf_tab->navigator_bar );
    return 0;
}

static void show_navbar_cb( char* key ) {
    int show_navbar;

    if ( settings_get_int( key, &show_navbar ) < 0 ) {
        return;
    }

    if ( show_navbar ) {
        document_storage_foreach( show_navbar_helper, NULL );
    } else {
        document_storage_foreach( hide_navbar_helper, NULL );
    }
}

static void show_toolbar_cb( char* key ) {
    int show_toolbar;

    if ( settings_get_int( key, &show_toolbar ) < 0 ) {
        return;
    }

    if ( show_toolbar ) {
        gtk_widget_show( toolbar );
    } else {
        gtk_widget_hide( toolbar );
    }
}

static int selection_color_helper( document_t* document, pdf_tab_t* pdf_tab, void* data ) {
    gtk_pdfview_reload_selection_color( pdf_tab->pdf_view );
    return 0;
}

static void selection_color_cb( char* key ) {
    document_storage_foreach( selection_color_helper, NULL );
}

static int highlight_color_helper( document_t* document, pdf_tab_t* pdf_tab, void* data ) {
    gtk_pdfview_reload_highlight_color( pdf_tab->pdf_view );
    return 0;
}

static void highlight_color_cb( char* key ) {
    document_storage_foreach( highlight_color_helper, NULL );
}

static int update_render_region;

static void render_limit_pages_cb( char* key ) {
    update_render_region = 1;
}

static void render_limit_mb_cb( char* key ) {
    update_render_region = 1;
}

static int update_render_region_helper( document_t* document, pdf_tab_t* pdf_tab, void* data ) {
    do_update_render_region( pdf_tab, document, TRUE );

    return 0;
}

static void toolbar_style_cb( char* key ) {
    toolbar_update_style();
}

static void apply_settings( void ) {
    update_render_region = 0;

    set_from_check_button( check_show_docs_from_last_time, "show-docs-from-last-time", NULL );
    set_from_check_button( check_open_docs_in_running, "open-in-running-instance", NULL );
    set_from_check_button( check_always_show_tabs, "always-show-tabs", always_show_tabs_cb );
    set_from_check_button( check_show_navbar, "show-navbar", show_navbar_cb );
    set_from_check_button( check_show_toolbar, "show-toolbar", show_toolbar_cb );
    set_from_check_button( check_hide_pointer, "hide-pointer", NULL );
    set_from_spin_button( spin_pointer_idle_time, "pointer-idle-time", NULL );
    set_from_entry( entry_browser_path, "browser-path", NULL );
    set_from_combobox( combo_tool_btn_style, "tool-button-style", toolbar_style_cb );

    set_from_color_button( color_button_selection, "color-selection", selection_color_cb );
    set_from_color_button( color_button_highlight, "color-highlight", highlight_color_cb );

    set_from_check_button( check_enable_render_limit_pages, "enable-render-limit-pages", render_limit_pages_cb );
    set_from_spin_button( spin_render_limit_pages, "render-limit-pages", render_limit_pages_cb );
    set_from_check_button( check_enable_render_limit_mb, "enable-render-limit-mb", render_limit_mb_cb );
    set_from_spin_button( spin_render_limit_mb, "render-limit-mb", render_limit_mb_cb );

    if ( update_render_region ) {
        document_storage_foreach( update_render_region_helper, NULL );
    }

    save_settings();
}

static void browse_browser_path( GtkWidget* widget, gpointer data ) {
    GtkWidget* dialog;
    GtkWidget* path_entry;

    path_entry = ( GtkWidget* )data;

    dialog = gtk_file_chooser_dialog_new(
        "Choose browser",
        GTK_WINDOW( main_window ),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL
    );

    if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT ) {
        char* filename;

        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );

        gtk_entry_set_text( GTK_ENTRY( path_entry ), filename );

        g_free( filename );
    }

    gtk_widget_destroy( dialog );
}

void show_settings_window( void ) {
    GtkWidget* dialog;
    GtkWidget* notebook;
    GtkWidget* vbox;
    GtkWidget* hbox = NULL;
    GtkWidget* tmp;

    /* dialog setup */

    dialog = gtk_dialog_new_with_buttons(
        "Settings",
        GTK_WINDOW( main_window ),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
        NULL
    );

    gtk_window_set_position( GTK_WINDOW( dialog ), GTK_WIN_POS_CENTER );
    gtk_dialog_set_default_response( GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT );

    /* notebook */

    notebook = gtk_notebook_new();
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), notebook );

    /* general tab */

    vbox = gtk_vbox_new( FALSE, 0 );
    gtk_container_set_border_width( GTK_CONTAINER( vbox ), 3 );
    gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), vbox, gtk_label_new( "General" ) );

    check_show_docs_from_last_time = create_check_button( "show-docs-from-last-time", vbox );
    check_open_docs_in_running = create_check_button( "open-in-running-instance", vbox );

    gtk_box_pack_start( GTK_BOX( vbox ), gtk_hseparator_new(), FALSE, FALSE, 5 );

    check_hide_pointer = create_check_button( "hide-pointer", vbox );
    spin_pointer_idle_time = create_spin_button( "pointer-idle-time", 1, 10, vbox, &hbox );

    check_button_assign_sensitivity( check_hide_pointer, hbox );

    gtk_box_pack_start( GTK_BOX( vbox ), gtk_hseparator_new(), FALSE, FALSE, 5 );

    entry_browser_path = create_entry( "browser-path", vbox, &hbox );

    tmp = gtk_button_new_with_label( "Browse" );
    gtk_box_pack_start( GTK_BOX( hbox ), tmp, FALSE, FALSE, 0 );

    g_signal_connect(
        G_OBJECT( tmp ),
        "clicked",
        G_CALLBACK( browse_browser_path ),
        entry_browser_path
    );


    /* appearance tab */

    vbox = gtk_vbox_new( FALSE, 0 );
    gtk_container_set_border_width( GTK_CONTAINER( vbox ), 3 );
    gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), vbox, gtk_label_new( "Appearance" ) );

    check_always_show_tabs = create_check_button( "always-show-tabs", vbox );
    check_show_navbar = create_check_button( "show-navbar", vbox );
    check_show_toolbar = create_check_button( "show-toolbar", vbox );

    gtk_box_pack_start( GTK_BOX( vbox ), gtk_hseparator_new(), FALSE, FALSE, 5 );

    const char* tool_style_values[] = {
        "Image & text",
        "Image only",
        "Text only",
        NULL
    };

    combo_tool_btn_style = create_combobox_with_mapping( "tool-button-style", vbox, tool_style_values, &hbox );

    check_button_assign_sensitivity( check_show_toolbar, hbox );

    gtk_box_pack_start( GTK_BOX( vbox ), gtk_hseparator_new(), FALSE, FALSE, 5 );

    color_button_selection = create_color_button( "color-selection", vbox );
    color_button_highlight = create_color_button( "color-highlight", vbox );

    /* render tab */

    vbox = gtk_vbox_new( FALSE, 0 );
    gtk_container_set_border_width( GTK_CONTAINER( vbox ), 3 );
    gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), vbox, gtk_label_new( "Render" ) );

    check_enable_render_limit_pages = create_check_button( "enable-render-limit-pages", vbox );
    spin_render_limit_pages = create_spin_button( "render-limit-pages", 1, MAX_PAGE_COUNT / 2, vbox, &hbox );

    check_button_assign_sensitivity( check_enable_render_limit_pages, hbox );

    check_enable_render_limit_mb = create_check_button( "enable-render-limit-mb", vbox );
    spin_render_limit_mb = create_spin_button( "render-limit-mb", 8, 512, vbox, &hbox );

    check_button_assign_sensitivity( check_enable_render_limit_mb, hbox );

    /* run dialog */

    gtk_widget_show_all( dialog );

    if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT ) {
        apply_settings();
    }

    gtk_widget_destroy( dialog );
}
