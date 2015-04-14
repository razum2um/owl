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

#include <poppler.h>

#include <utils.h>
#include <gui/doc_tab.h>
#include <gui/print.h>
#include <config/config.h>

extern GtkWidget* main_window;

static GtkPrintSettings* settings = NULL;
static GtkPageSetup* page_setup = NULL;

static char settings_path[ 256 ];
static char page_setup_path[ 256 ];

static void begin_print( GtkPrintOperation* operation, GtkPrintContext* context, gpointer data ) {
    document_t* doc = ( document_t* )data;

    gtk_print_operation_set_n_pages( operation, document_get_page_count( doc ) );
}

static void done( GtkPrintOperation* operation, GtkPrintOperationResult result, gpointer data ) {
    pdf_tab_t* pdf_tab;
    GError* error = NULL;

    pdf_tab = ( pdf_tab_t* )data;

    switch ( result ) {
        case GTK_PRINT_OPERATION_RESULT_ERROR :
            gtk_print_operation_get_error( operation, &error );

            if ( error != NULL ) {
                doctab_set_statusbar( pdf_tab, 0, "Error: %s", error->message );

                g_error_free( error );
                error = NULL;
            }
            break;

        case GTK_PRINT_OPERATION_RESULT_CANCEL :
            doctab_set_statusbar( pdf_tab, 3000, "Printing has been canceled" );
            break;

        default :
            doctab_set_statusbar( pdf_tab, 0, NULL );
            break;
    }
}

static void draw_page( GtkPrintOperation* operation, GtkPrintContext* context, gint page_index, gpointer data ) {
    document_t* doc;
    page_t* page;
    cairo_t* cr;
    gdouble width;
    gdouble height;
    double scale_x;
    double scale_y;
    double scale;
    double v_dpi;
    double h_dpi;
    double v_margin = 0;
    double h_margin = 0;
    double x;
    double y;

    doc = ( document_t* )data;

    cr = gtk_print_context_get_cairo_context( context );

    width = gtk_print_context_get_width( context );
    height = gtk_print_context_get_height( context );

    page = document_get_page( doc, page_index );

    if ( page == NULL ) {
        gtk_print_operation_cancel( operation );
        return;
    }

    /* white background */

    cairo_set_source_rgb( cr, 1, 1, 1 );
    cairo_rectangle( cr, 0, 0, width, height );
    cairo_fill( cr );

    /* scale */

    scale_x = width / page->original_width;
    scale_y = height / page->original_height;
    scale = MIN( scale_x, scale_y );

    cairo_scale( cr, scale, scale );

    /* center page */

    v_dpi = gtk_print_context_get_dpi_y( context );
    h_dpi = gtk_print_context_get_dpi_x( context );

    v_margin += gtk_page_setup_get_top_margin( page_setup, GTK_UNIT_INCH );
    v_margin += gtk_page_setup_get_bottom_margin( page_setup, GTK_UNIT_INCH );

    h_margin += gtk_page_setup_get_left_margin( page_setup, GTK_UNIT_INCH );
    h_margin += gtk_page_setup_get_right_margin( page_setup, GTK_UNIT_INCH );

    x = MAX( 0, ( width - page->original_width * scale - h_margin * v_dpi ) / 2 );
    y = MAX( 0, ( height - page->original_height * scale - v_margin * h_dpi ) / 2 );

    cairo_translate( cr, x, y );

    /* render */

#ifdef _WIN32
#else
    poppler_lock();
    poppler_page_render( page->page, cr );
    poppler_unlock();
#endif /* _WIN32 */
}

static gboolean paginate( GtkPrintOperation* operation, GtkPrintContext* context, gpointer data ) {
    return TRUE;
}

static void status_changed( GtkPrintOperation* operation, gpointer data ) {
    pdf_tab_t* pdf_tab;
    const char* string;

    pdf_tab = ( pdf_tab_t* )data;

    string = gtk_print_operation_get_status_string( operation );

    doctab_set_statusbar( pdf_tab, 0, "%s", string );
}

void print_operation_start( pdf_tab_t* pdf_tab, document_t* document ) {
    GtkPrintOperation* operation;
    GtkPrintOperationResult res;

    operation = gtk_print_operation_new();

    if ( settings == NULL ) {
        settings = gtk_print_settings_new_from_file( settings_path, NULL );
    }

    if ( settings != NULL ) {
        gtk_print_operation_set_print_settings( operation, settings );
    }

    if ( page_setup == NULL ) {
        page_setup = gtk_page_setup_new();
        gtk_page_setup_load_file( page_setup, page_setup_path, NULL );
    }

    if ( page_setup != NULL ) {
        gtk_print_operation_set_default_page_setup( operation, page_setup );
    }

    g_signal_connect(
        operation,
        "begin-print",
        G_CALLBACK( begin_print ),
        document
    );

    g_signal_connect(
        operation,
        "done",
        G_CALLBACK( done ),
        pdf_tab
    );

    g_signal_connect(
        operation,
        "draw-page",
        G_CALLBACK( draw_page ),
        document
    );

    g_signal_connect(
        operation,
        "paginate",
        G_CALLBACK( paginate ),
        NULL
    );

    g_signal_connect(
        operation,
        "status-changed",
        G_CALLBACK( status_changed ),
        pdf_tab
    );

    gtk_print_operation_set_show_progress( operation, TRUE );

    res = gtk_print_operation_run(
        operation,
        GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
        GTK_WINDOW( main_window ),
        NULL
    );

    if ( res == GTK_PRINT_OPERATION_RESULT_APPLY ) {
        settings = gtk_print_operation_get_print_settings( operation );

        if ( settings != NULL ) {
            gtk_print_settings_to_file( settings, settings_path, NULL );
        }
    }
}

void print_page_setup( void ) {
    GtkPageSetup* new_page_setup;

    if ( settings == NULL ) {
        settings = gtk_print_settings_new_from_file( settings_path, NULL );
    }

    if ( page_setup == NULL ) {
        page_setup = gtk_page_setup_new();
        gtk_page_setup_load_file( page_setup, page_setup_path, NULL );
    }

    new_page_setup = gtk_print_run_page_setup_dialog( GTK_WINDOW( main_window ), page_setup, settings );

    if ( page_setup != NULL ) {
        g_object_unref( page_setup );
    }

    page_setup = new_page_setup;

    gtk_page_setup_to_file( page_setup, page_setup_path, NULL );
}

void init_print( void ) {
    const char* confdir = get_config_directory();

    snprintf( settings_path, sizeof( settings_path ), "%s%sprint-settings", confdir, PATH_SEPARATOR );
    snprintf( page_setup_path, sizeof( page_setup_path ), "%s%sprint-page-setup", confdir, PATH_SEPARATOR );
}
