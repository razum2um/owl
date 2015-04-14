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
#include <gtk/gtk.h>

#include <config/settings.h>
#include <config/config.h>
#include <gui/doc_tab.h>
#include <gui/pdf_view.h>
#include <gui/my_stock.h>
#include <gui/events.h>

static GtkWidget* create_navigator_bar( pdf_tab_t* tab ) {
    GtkWidget* hbox;
    GtkWidget* table;
    GtkWidget* padding;
    GtkWidget* button;
    GtkWidget* tmp;

    table = gtk_table_new( 1, 3, TRUE );

    hbox = gtk_hbox_new( FALSE, 2 );

    button = gtk_button_new_from_stock( MY_STOCK_PREV_PAGE );
    GTK_WIDGET_UNSET_FLAGS( button, GTK_CAN_FOCUS );
    gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_NONE );
    gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 2 );

    g_signal_connect(
        G_OBJECT( button ),
        "clicked",
        G_CALLBACK( event_goto_prev_page ),
        tab->pdf_view
    );

    tab->entry_curr_pg = gtk_entry_new();
    gtk_widget_set_size_request( tab->entry_curr_pg, 40, -1 );
    gtk_entry_set_alignment( GTK_ENTRY( tab->entry_curr_pg ), 1.0f );
    gtk_box_pack_start( GTK_BOX( hbox ), tab->entry_curr_pg, FALSE, FALSE, 2 );

    g_signal_connect(
        G_OBJECT( tab->entry_curr_pg ),
        "activate",
        G_CALLBACK( event_doctab_goto_page ),
        tab
    );

    padding = gtk_label_new( "/" );
    gtk_box_pack_start( GTK_BOX( hbox ), padding, FALSE, FALSE, 2 );

    tab->entry_last_pg = gtk_entry_new();
    gtk_widget_set_size_request( tab->entry_last_pg, 40, -1 );
    gtk_entry_set_alignment( GTK_ENTRY( tab->entry_last_pg ), 1.0f );
    gtk_entry_set_editable( GTK_ENTRY( tab->entry_last_pg ), FALSE );
    gtk_box_pack_start( GTK_BOX( hbox ), tab->entry_last_pg, FALSE, FALSE, 2 );

    button = gtk_button_new_from_stock( MY_STOCK_NEXT_PAGE );
    GTK_WIDGET_UNSET_FLAGS( button, GTK_CAN_FOCUS );
    gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_NONE );
    gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 2 );

    g_signal_connect(
        G_OBJECT( button ),
        "clicked",
        G_CALLBACK( event_goto_next_page ),
        tab->pdf_view
    );

    gtk_table_attach(
        GTK_TABLE( table ), hbox,
        1, 2, 0, 1,
        0, GTK_FILL,
        0, 0
    );

    tmp = gtk_hbox_new( TRUE, 0 );
    gtk_box_pack_start( GTK_BOX( hbox ), tmp, TRUE, TRUE, 2 );

    tab->label_statusbar = gtk_label_new( "" );
    gtk_label_set_ellipsize( GTK_LABEL( tab->label_statusbar ), PANGO_ELLIPSIZE_END );
    gtk_misc_set_alignment( GTK_MISC( tab->label_statusbar ), 1.0, 0.5 );

    gtk_table_attach(
        GTK_TABLE( table ), tab->label_statusbar,
        2, 3, 0, 1,
        GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
        10, 0
    );

    return table;
}

static GtkWidget* create_search_bar( pdf_tab_t* tab ) {
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* close_button;
    GtkWidget* label;
    GtkWidget* prev_button;
    GtkWidget* next_button;

    vbox = gtk_vbox_new( FALSE, 2 );

    hbox = gtk_hbox_new( FALSE, 2 );

    gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 2 );
    gtk_box_pack_start( GTK_BOX( vbox ), gtk_hseparator_new(), FALSE, FALSE, 2 );

    close_button = gtk_button_new();

    gtk_button_set_image(
        GTK_BUTTON( close_button ),
        gtk_image_new_from_stock( GTK_STOCK_CLOSE, GTK_ICON_SIZE_SMALL_TOOLBAR )
    );

    g_signal_connect(
        G_OBJECT( close_button ),
        "clicked",
        G_CALLBACK( event_doctab_find_close ),
        tab
    );

    GTK_WIDGET_UNSET_FLAGS( close_button, GTK_CAN_FOCUS );
    gtk_button_set_relief( GTK_BUTTON( close_button ), GTK_RELIEF_NONE );
    gtk_box_pack_start( GTK_BOX( hbox ), close_button, FALSE, FALSE, 2 );

    label = gtk_label_new( "Find:" );
    gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, FALSE, 2 );

    tab->entry_find = gtk_entry_new();
    gtk_box_pack_start( GTK_BOX( hbox ), tab->entry_find, FALSE, FALSE, 2 );

    g_signal_connect(
        G_OBJECT( tab->entry_find ),
        "key-press-event",
        G_CALLBACK( event_doctab_find_keypress ),
        tab
    );

    g_signal_connect(
        G_OBJECT( tab->entry_find ),
        "changed",
        G_CALLBACK( event_doctab_find_text_changed ),
        tab
    );

    prev_button = gtk_button_new_from_stock( GTK_STOCK_GO_BACK );
    gtk_button_set_relief( GTK_BUTTON( prev_button ), GTK_RELIEF_NONE );
    gtk_box_pack_start( GTK_BOX( hbox ), prev_button, FALSE, FALSE, 2 );

    g_signal_connect(
        G_OBJECT( prev_button ),
        "clicked",
        G_CALLBACK( event_doctab_find_prev ),
        tab
    );

    next_button = gtk_button_new_from_stock( GTK_STOCK_GO_FORWARD );
    gtk_button_set_relief( GTK_BUTTON( next_button ), GTK_RELIEF_NONE );
    gtk_box_pack_start( GTK_BOX( hbox ), next_button, FALSE, FALSE, 2 );

    g_signal_connect(
        G_OBJECT( next_button ),
        "clicked",
        G_CALLBACK( event_doctab_find_next ),
        tab
    );

    tab->v_sep_find = gtk_vseparator_new();
    gtk_box_pack_start( GTK_BOX( hbox ), tab->v_sep_find, FALSE, FALSE, 5 );

    tab->image_find = gtk_image_new();
    gtk_box_pack_start( GTK_BOX( hbox ), tab->image_find, FALSE, FALSE, 2 );

    tab->label_find = gtk_label_new( "" );
    gtk_box_pack_start( GTK_BOX( hbox ), tab->label_find, FALSE, FALSE, 5 );

    return vbox;
}

pdf_tab_t* create_pdf_tab( document_t* document ) {
    int width;
    int height;
    int show_navbar;

    GtkWidget* hbox;
    GtkWidget* vbox;
    GtkWidget* table;
    pdf_tab_t* tab;

    tab = ( pdf_tab_t* )malloc( sizeof( pdf_tab_t ) );

    if ( tab == NULL ) {
        return tab;
    }

    memset( tab, 0, sizeof( pdf_tab_t ) );

    tab->cur_highlighted_page = -1;
    tab->root = gtk_vbox_new( FALSE, 2 );
    tab->paned = NULL;

    hbox = gtk_hbox_new( FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( tab->root ), hbox, TRUE, TRUE, 2 );

    /* TOC view */

    if ( document->ref_table->toc != NULL ) {
        GtkCellRenderer* toc_renderer;
        GtkTreeViewColumn* toc_column;
        GtkWidget* toc_scrolled_win;
        GtkTreeSelection* toc_sel;

        tab->paned = gtk_hpaned_new();
        gtk_paned_set_position( GTK_PANED( tab->paned ), 300 );

        tab->toc_view = gtk_tree_view_new_with_model( GTK_TREE_MODEL( document->ref_table->toc ) );
        gtk_widget_set_has_tooltip( tab->toc_view, TRUE );

        g_signal_connect(
            G_OBJECT( tab->toc_view ),
            "query-tooltip",
            G_CALLBACK( event_doctab_toc_query_tooltip ),
            tab
        );

        /* title string col */

        toc_renderer = gtk_cell_renderer_text_new();
        toc_column = gtk_tree_view_column_new_with_attributes(
            "Table of Contents",
            toc_renderer,
            "text", REF_COL_TITLE,
            "weight", REF_COL_FONT,
            NULL
        );

        gtk_tree_view_column_set_resizable( toc_column, TRUE );
        gtk_tree_view_append_column( GTK_TREE_VIEW( tab->toc_view ), toc_column );

        /* page number col */

        toc_renderer = gtk_cell_renderer_text_new();
        g_object_set( ( gpointer )toc_renderer, "xalign", 1.0, NULL );
        toc_column = gtk_tree_view_column_new_with_attributes(
            "#",
            toc_renderer,
            "text", REF_COL_PAGE_NUM,
            "weight", REF_COL_FONT,
            NULL
        );

        gtk_tree_view_append_column( GTK_TREE_VIEW( tab->toc_view ), toc_column );

        /* selection */

        toc_sel = gtk_tree_view_get_selection( GTK_TREE_VIEW( tab->toc_view ) );
        gtk_tree_selection_set_mode( toc_sel, GTK_SELECTION_BROWSE );

        g_signal_connect(
            G_OBJECT( toc_sel ),
            "changed",
            G_CALLBACK( event_toc_selection_changed ),
            tab
        );

        /* setup searching */

        gtk_tree_view_set_enable_search( GTK_TREE_VIEW( tab->toc_view ), TRUE );
        gtk_tree_view_set_search_column( GTK_TREE_VIEW( tab->toc_view ), REF_COL_TITLE );

        gtk_tree_view_set_search_equal_func(
            GTK_TREE_VIEW( tab->toc_view ),
            ref_table_search_equal_func,
            NULL,
            NULL
        );

        /* packing */

        toc_scrolled_win = gtk_scrolled_window_new( NULL, NULL );

        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW( toc_scrolled_win ),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC
        );

        gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( toc_scrolled_win ), tab->toc_view );
        gtk_paned_add1( GTK_PANED( tab->paned ), toc_scrolled_win );
        gtk_box_pack_start( GTK_BOX( hbox ), tab->paned, TRUE, TRUE, 0 );
    }

    vbox = gtk_vbox_new( FALSE, 2 );

    if ( tab->paned != NULL ) {
        gtk_paned_add2( GTK_PANED( tab->paned ), vbox );
    } else {
        gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 2 );
    }

    /* PDF view */

    table = gtk_table_new( 2, 2, FALSE );
    gtk_box_pack_start( GTK_BOX( vbox ), table, TRUE, TRUE, 0 );

    tab->pdf_view = gtk_pdfview_new( document );

    tab->viewport = gtk_viewport_new( NULL, NULL );
    gtk_container_add( GTK_CONTAINER( tab->viewport ), tab->pdf_view );

    gtk_table_attach(
        GTK_TABLE( table ), tab->viewport,
        0, 1, 0, 1,
        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
        0, 0
    );

    GTK_WIDGET_SET_FLAGS( tab->pdf_view, GTK_CAN_FOCUS );

    g_signal_connect(
        G_OBJECT( tab->pdf_view ),
        "size-allocate",
        G_CALLBACK( event_doctab_size_allocate ),
        tab
    );

    /* Search bar */

    tab->search_bar = create_search_bar( tab );

    gtk_box_pack_start( GTK_BOX( vbox ), tab->search_bar, FALSE, FALSE, 0 );

    /* Navigator bar */

    tab->navigator_bar = create_navigator_bar( tab );

    gtk_box_pack_start( GTK_BOX( vbox ), tab->navigator_bar, FALSE, FALSE, 0 );

    /* Vertical scroll bar */

    document_get_geometry( document, gtk_pdfview_get_cols( tab->pdf_view ), &width, &height );

    tab->v_adjustment = gtk_adjustment_new(
        0, 0,
        height - 1 + PAGE_INCREMENT,
        STEP_INCREMENT,
        PAGE_INCREMENT,
        PAGE_INCREMENT
    );

    g_signal_connect(
        G_OBJECT( tab->v_adjustment ),
        "value-changed",
        G_CALLBACK( event_scroll_v_adjustment_changed ),
        tab
    );

    tab->v_scroll_bar = gtk_vscrollbar_new( GTK_ADJUSTMENT( tab->v_adjustment ) );

    gtk_table_attach(
        GTK_TABLE( table ), tab->v_scroll_bar,
        1, 2, 0, 1,
        0, GTK_FILL | GTK_EXPAND,
        0, 0
    );

    /* Horizontal scroll bar */

    tab->h_adjustment = gtk_adjustment_new(
        0, 0,
        width - 1 + PAGE_INCREMENT,
        STEP_INCREMENT,
        PAGE_INCREMENT,
        PAGE_INCREMENT
    );

    g_signal_connect(
        G_OBJECT( tab->h_adjustment ),
        "value-changed",
        G_CALLBACK( event_scroll_h_adjustment_changed ),
        tab
    );

    tab->h_scroll_bar = gtk_hscrollbar_new( GTK_ADJUSTMENT( tab->h_adjustment ) );

    gtk_table_attach(
        GTK_TABLE( table ), tab->h_scroll_bar,
        0, 1, 1, 2,
        GTK_FILL | GTK_EXPAND, 0,
        0, 0
    );

    gtk_widget_show_all( tab->root );

    /* Hide unnecessary widgets */

    gtk_widget_hide( tab->search_bar );
    gtk_widget_hide( tab->v_sep_find );

    if ( ( settings_get_int( "show-navbar", &show_navbar ) == 0 ) &&
         ( !show_navbar ) ) {
        gtk_widget_hide( tab->navigator_bar );
    }

    return tab;
}

void doctab_set_search_status( pdf_tab_t* pdf_tab, search_status_t status ) {
    GtkLabel* label = GTK_LABEL( pdf_tab->label_find );
    GtkImage* image = GTK_IMAGE( pdf_tab->image_find );

    switch ( status ) {
        case SEARCH_STATUS_CLEAR :
            gtk_label_set_text( label, NULL );
            gtk_image_clear( image );
            break;

        case SEARCH_STATUS_BUSY :
            gtk_label_set_text( label, "Searching..." );
            gtk_image_set_from_stock( image, MY_STOCK_FIND_BUSY, GTK_ICON_SIZE_SMALL_TOOLBAR );
            break;

        case SEARCH_STATUS_WRAPPED :
            gtk_label_set_text( label, "Wrapped search" );
            gtk_image_set_from_stock( image, MY_STOCK_FIND_WRAPPED, GTK_ICON_SIZE_SMALL_TOOLBAR );
            break;

        case SEARCH_STATUS_NONE :
            gtk_label_set_text( label, "Phrase not found" );
            gtk_image_set_from_stock( image, MY_STOCK_FIND_NONE, GTK_ICON_SIZE_SMALL_TOOLBAR );
            break;
    }

    if ( status == SEARCH_STATUS_CLEAR ) {
        gtk_widget_hide( pdf_tab->v_sep_find );
    } else {
        gtk_widget_show( pdf_tab->v_sep_find );
    }
}

static gboolean doctab_statusbar_timeout( gpointer data ) {
    pdf_tab_t* pdf_tab;

    pdf_tab = ( pdf_tab_t* )data;

    doctab_set_statusbar( pdf_tab, 0, NULL );

    return FALSE;
}

void doctab_set_statusbar( pdf_tab_t* pdf_tab, int timeout, const char* format, ... ) {
    va_list ap;
    char buffer[ 512 ];

    if ( format == NULL ) {
        buffer[ 0 ] = 0;
    } else {
        va_start( ap, format );
        vsnprintf( buffer, sizeof( buffer ), format, ap );
        va_end( ap );

        if ( pdf_tab->statusbar_timer_tag != 0 ) {
            g_source_remove( pdf_tab->statusbar_timer_tag );
            pdf_tab->statusbar_timer_tag = 0;
        }

        if ( timeout > 0 ) {
            pdf_tab->statusbar_timer_tag = g_timeout_add( timeout, doctab_statusbar_timeout, pdf_tab );
        }
    }

    gtk_label_set_text( GTK_LABEL( pdf_tab->label_statusbar ), buffer );
}

static void doctab_adjustment_set( GtkAdjustment* adj, int value, int lower, int upper ) {
    gtk_adjustment_set_lower( adj, lower );
    gtk_adjustment_set_upper( adj, upper );
    gtk_adjustment_set_value( adj, value );
}

static void doctab_update_v_adjustment_limits( pdf_tab_t* pdf_tab, int doc_height, int widget_height ) {
    gdouble value;
    gdouble value_new;
    gdouble upper;
    gdouble upper_new;

    value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );
    upper = gtk_adjustment_get_upper( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );

    upper_new = MAX( 0, doc_height - widget_height ) + PAGE_INCREMENT;
    value_new = value * upper_new / upper;

    doctab_adjustment_set(
        GTK_ADJUSTMENT( pdf_tab->v_adjustment ),
        MIN( value_new, upper_new - PAGE_INCREMENT ),
        0, upper_new
    );

    if ( upper_new < PAGE_INCREMENT + 0.1 ) {
        gtk_widget_hide( pdf_tab->v_scroll_bar );
    } else if ( !gtk_pdfview_get_fullscreen_mode( pdf_tab->pdf_view ) ) {
        gtk_widget_show( pdf_tab->v_scroll_bar );
    }
}

static void doctab_update_h_adjustment_limits( pdf_tab_t* pdf_tab, int doc_width, int widget_width ) {
    gdouble value;
    gdouble value_new;
    gdouble upper;
    gdouble upper_new;

    value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->h_adjustment ) );
    upper = gtk_adjustment_get_upper( GTK_ADJUSTMENT( pdf_tab->h_adjustment ) );

    upper_new = MAX( 0, doc_width - widget_width ) + PAGE_INCREMENT;
    value_new = value * upper_new / upper;

    doctab_adjustment_set(
        GTK_ADJUSTMENT( pdf_tab->h_adjustment ),
        MIN( value_new, upper_new - PAGE_INCREMENT ),
        0, upper_new
    );

    if ( upper_new < PAGE_INCREMENT + 0.1 ) {
        gtk_widget_hide( pdf_tab->h_scroll_bar );
    } else if ( !gtk_pdfview_get_fullscreen_mode( pdf_tab->pdf_view ) ) {
        gtk_widget_show( pdf_tab->h_scroll_bar );
    }
}

void doctab_update_adjustment_limits( pdf_tab_t* pdf_tab ) {
    int doc_width;
    int doc_height;
    int widget_width;
    int widget_height;
    display_t display_type;

    gdk_window_get_geometry( pdf_tab->pdf_view->window, NULL, NULL, &widget_width, &widget_height, NULL );

    display_type = gtk_pdfview_get_display_type( pdf_tab->pdf_view );

    if ( display_type == CONTINUOUS ) {
        gtk_pdfview_get_geometry( pdf_tab->pdf_view, &doc_width, &doc_height );
    } else {
        gtk_pdfview_current_row_get_geometry( pdf_tab->pdf_view, &doc_width, &doc_height );
    }

    if ( !gtk_pdfview_get_size_allocated( pdf_tab->pdf_view ) ) {
        return;
    }

    doctab_update_v_adjustment_limits( pdf_tab, doc_height, widget_height );
    doctab_update_h_adjustment_limits( pdf_tab, doc_width, widget_width );
}

void doctab_v_adjustment_add_value( pdf_tab_t* pdf_tab, int amount ) {
    gdouble value;
    gdouble upper;

    if ( amount == 0 ) {
        return;
    }

    value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );
    upper = gtk_adjustment_get_upper( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );

    gtk_adjustment_set_value(
        GTK_ADJUSTMENT( pdf_tab->v_adjustment ),
        MIN( upper - PAGE_INCREMENT, value + amount )
    );
}

int doctab_v_adjustment_get_value( pdf_tab_t* pdf_tab ) {
    return ( int )gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );
}

void doctab_v_adjustment_set_value( pdf_tab_t* pdf_tab, int value ) {
    gdouble old_value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );

    if ( value != old_value ) {
        gdouble upper = gtk_adjustment_get_upper( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );

        gtk_adjustment_set_value(
            GTK_ADJUSTMENT( pdf_tab->v_adjustment ),
            MIN( upper - PAGE_INCREMENT, value )
        );
    }
}

void doctab_v_adjustment_set_begin( pdf_tab_t* pdf_tab ) {
    gdouble value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );

    if ( value > 0 ) {
        gtk_adjustment_set_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ), 0 );
    }
}

void doctab_v_adjustment_set_end( pdf_tab_t* pdf_tab ) {
    gdouble upper = gtk_adjustment_get_upper( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );
    gdouble value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ) );

    if ( value < upper ) {
        gtk_adjustment_set_value( GTK_ADJUSTMENT( pdf_tab->v_adjustment ), upper - PAGE_INCREMENT );
    }
}

int doctab_h_adjustment_get_value( pdf_tab_t* pdf_tab ) {
    return ( int )gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->h_adjustment ) );
}

void doctab_h_adjustment_set_value( pdf_tab_t* pdf_tab, int value ) {
    gdouble old_value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->h_adjustment ) );

    if ( value != old_value ) {
        gdouble upper = gtk_adjustment_get_upper( GTK_ADJUSTMENT( pdf_tab->h_adjustment ) );

        gtk_adjustment_set_value(
            GTK_ADJUSTMENT( pdf_tab->h_adjustment ),
            MIN( upper - PAGE_INCREMENT, value )
        );
    }
}

void doctab_h_adjustment_add_value( pdf_tab_t* pdf_tab, int amount ) {
    gdouble value;
    gdouble upper;

    if ( amount == 0 ) {
        return;
    }

    value = gtk_adjustment_get_value( GTK_ADJUSTMENT( pdf_tab->h_adjustment ) );
    upper = gtk_adjustment_get_upper( GTK_ADJUSTMENT( pdf_tab->h_adjustment ) );

    gtk_adjustment_set_value(
        GTK_ADJUSTMENT( pdf_tab->h_adjustment ),
        MIN( upper - PAGE_INCREMENT, value + amount )
    );
}
