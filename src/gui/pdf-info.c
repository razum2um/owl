#include <gtk/gtk.h>

#include <gui/pdf-info.h>
#include <gui/main_wnd.h>

typedef enum {
    ROW_TYPE_LABEL,
    ROW_TYPE_ENTRY
} row_type_t;

enum {
    FONT_COL_NAME,
    FONT_COL_FULL_NAME,
    FONT_COL_TYPE,
    FONT_COL_EMBEDDED,
    FONT_COL_SUBSET,
    FONT_COL_COUNT
};

static gchar* format_date( GTime utime ) {
    time_t time = ( time_t )utime;
    char s[ 256 ];
    struct tm* t;
    size_t len;

    t = localtime( &time );

    if ( ( time == 0 ) || ( t == NULL ) ) {
        return NULL;
    }

    len = strftime( s, sizeof( s ), "%c", t );

    if ( len == 0 || s[0] == 0 ) {
        return NULL;
    }

    return g_locale_to_utf8( s, -1, NULL, NULL, NULL );
}

static gchar* font_type_to_string( PopplerFontType font_type ) {
    switch ( font_type ) {
        case POPPLER_FONT_TYPE_TYPE1 :
            return "Type 1";

        case POPPLER_FONT_TYPE_TYPE1C :
            return "Type 1C";

        case POPPLER_FONT_TYPE_TYPE1COT :
            return "Type 1C (OT)";

        case POPPLER_FONT_TYPE_TYPE3 :
            return "Type 3";

        case POPPLER_FONT_TYPE_TRUETYPE :
            return "TrueType";

        case POPPLER_FONT_TYPE_TRUETYPEOT :
            return "TrueType (OT)";

        case POPPLER_FONT_TYPE_CID_TYPE0 :
            return "CID Type 0";

        case POPPLER_FONT_TYPE_CID_TYPE0C :
            return "CID Type 0C";

        case POPPLER_FONT_TYPE_CID_TYPE0COT :
            return "CID Type 0C (OT)";

        case POPPLER_FONT_TYPE_CID_TYPE2 :
            return "CID Type 2";

        case POPPLER_FONT_TYPE_CID_TYPE2OT :
            return "CID Type 2 (OT)";

        case POPPLER_FONT_TYPE_UNKNOWN :
        default:
            return "Unknown";
    }
}

static void table_append_row( GtkWidget* widget, char* label, char* data, int* row, row_type_t type ) {
    GtkWidget* hbox;
    GtkWidget* right = NULL;

    hbox = gtk_hbox_new( FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( hbox ), gtk_label_new( label ), FALSE, FALSE, 0 );

    gtk_table_attach(
        GTK_TABLE( widget ),
        hbox,
        0, 1, *row, *row + 1,
        GTK_FILL, GTK_FILL, 3, 3
    );

    switch ( type ) {
        case ROW_TYPE_LABEL :
            right = gtk_hbox_new( FALSE, 0 );
            gtk_box_pack_start( GTK_BOX( right ), gtk_label_new( data == NULL ? "" : data ), FALSE, FALSE, 0 );

            break;

        case ROW_TYPE_ENTRY :
            right = gtk_entry_new();
            gtk_entry_set_editable( GTK_ENTRY( right ), FALSE );
            gtk_entry_set_text( GTK_ENTRY( right ), data == NULL ? "" : data );

            break;
    }

    gtk_table_attach(
        GTK_TABLE( widget ),
        right,
        1, 2, *row, *row + 1,
        GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3
    );

    *row += 1;
}

void pdf_info_show_properties( document_t* document ) {
    GtkWidget* dialog;
    GtkWidget* notebook;
    GtkWidget* table;
    int row = 0;
    gchar* str;
    gchar* title;
    gchar* format;
    gchar* author;
    gchar* subject;
    gchar* keywords;
    gchar* creator;
    gchar* producer;
    gchar* linearized;
    gchar* metadata;
    GTime creation_date;
    GTime mod_date;
    PopplerPageLayout layout;
    PopplerPageMode mode;
    PopplerPermissions permissions;
    PopplerViewerPreferences view_prefs;

    GtkTreeStore* font_store;
    GtkWidget* font_view;
    GtkTreeIter iter;
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;
    GtkWidget* font_scroll_wnd;
    PopplerFontInfo* font_info;
    PopplerFontsIter* font_iter;
    gboolean error;

    dialog = gtk_dialog_new_with_buttons(
        "Properties",
        GTK_WINDOW( main_window ),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        NULL
    );

    notebook = gtk_notebook_new();
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), notebook );

    /* PDF metadata */

    table = gtk_table_new( 10, 2, FALSE );
    gtk_container_set_border_width( GTK_CONTAINER( table ), 3 );
    gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), table, gtk_label_new( "Metadata" ) );

    g_object_get(
        document->doc,
        "title", &title,
        "format", &format,
        "author", &author,
        "subject", &subject,
        "keywords", &keywords,
        "creation-date", &creation_date,
        "mod-date", &mod_date,
        "creator", &creator,
        "producer", &producer,
        "linearized", &linearized,
        "page-mode", &mode,
        "page-layout", &layout,
        "permissions", &permissions,
        "viewer-preferences", &view_prefs,
        "metadata", &metadata,
        NULL
    );

    table_append_row( table, "Title:", title, &row, ROW_TYPE_ENTRY );
    table_append_row( table, "Author:", author, &row, ROW_TYPE_ENTRY );
    table_append_row( table, "Subject:", subject, &row, ROW_TYPE_ENTRY );
    table_append_row( table, "Keywords:", keywords, &row, ROW_TYPE_ENTRY );

    str = format_date( creation_date );
    table_append_row( table, "Created:", str, &row, ROW_TYPE_LABEL );
    g_free( str );

    str = format_date( mod_date );
    table_append_row( table, "Modified:", str, &row, ROW_TYPE_LABEL );
    g_free( str );

    table_append_row( table, "Creator:", creator, &row, ROW_TYPE_LABEL );
    table_append_row( table, "Producer:", producer, &row, ROW_TYPE_LABEL );
    table_append_row( table, "Format:", format, &row, ROW_TYPE_LABEL );
    //table_append_row( table, "Linearized:", linearized, &row, ROW_TYPE_LABEL );

    /* font info */

    font_info = poppler_font_info_new( document->doc );
    error = poppler_font_info_scan( font_info, document_get_page_count( document ), &font_iter );

    if ( error == FALSE ) {
        goto display;
    }

    font_store = gtk_tree_store_new(
        FONT_COL_COUNT,
        G_TYPE_STRING,  /* font name */
        G_TYPE_STRING,  /* font full name */
        G_TYPE_STRING,  /* font type */
        G_TYPE_BOOLEAN, /* embedded */
        G_TYPE_BOOLEAN  /* subset */
    );

    do {
        gtk_tree_store_append( font_store, &iter, NULL );

        gtk_tree_store_set(
            font_store,
            &iter,
            FONT_COL_NAME, poppler_fonts_iter_get_name( font_iter ),
            FONT_COL_FULL_NAME, poppler_fonts_iter_get_full_name( font_iter ),
            FONT_COL_TYPE, font_type_to_string( poppler_fonts_iter_get_font_type( font_iter ) ),
            FONT_COL_EMBEDDED, poppler_fonts_iter_is_embedded( font_iter ),
            FONT_COL_SUBSET, poppler_fonts_iter_is_subset( font_iter ),
            -1
        );
    } while ( poppler_fonts_iter_next( font_iter ) );

    font_view = gtk_tree_view_new_with_model( GTK_TREE_MODEL( font_store ) );

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Name",
        renderer,
        "text", FONT_COL_NAME,
        NULL
    );
    gtk_tree_view_column_set_resizable( column, TRUE );
    gtk_tree_view_append_column( GTK_TREE_VIEW( font_view ), column );

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Full name",
        renderer,
        "text", FONT_COL_FULL_NAME,
        NULL
    );
    gtk_tree_view_column_set_resizable( column, TRUE );
    gtk_tree_view_append_column( GTK_TREE_VIEW( font_view ), column );

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Type",
        renderer,
        "text", FONT_COL_TYPE,
        NULL
    );
    gtk_tree_view_column_set_resizable( column, TRUE );
    gtk_tree_view_append_column( GTK_TREE_VIEW( font_view ), column );

    renderer = gtk_cell_renderer_toggle_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Embedded",
        renderer,
        "active", FONT_COL_EMBEDDED,
        NULL
    );
    gtk_tree_view_column_set_resizable( column, TRUE );
    gtk_tree_view_append_column( GTK_TREE_VIEW( font_view ), column );

    renderer = gtk_cell_renderer_toggle_new();
    column = gtk_tree_view_column_new_with_attributes(
        "Subset",
        renderer,
        "active", FONT_COL_SUBSET,
        NULL
    );
    gtk_tree_view_column_set_resizable( column, TRUE );
    gtk_tree_view_append_column( GTK_TREE_VIEW( font_view ), column );

    font_scroll_wnd = gtk_scrolled_window_new( NULL, NULL );

    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW( font_scroll_wnd ),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC
    );

    gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( font_scroll_wnd ), font_view );
    gtk_notebook_append_page( GTK_NOTEBOOK( notebook ), font_scroll_wnd, gtk_label_new( "Font info" ) );

    /* run dialog */

 display:
    gtk_widget_show_all( dialog );

    gtk_dialog_run( GTK_DIALOG( dialog ) );

    gtk_widget_destroy( dialog );

    g_free( title );
    g_free( format );
    g_free( author );
    g_free( subject );
    g_free( keywords );
    g_free( creator );
    g_free( producer );
    //g_free( linearized );
    g_free( metadata );
}
