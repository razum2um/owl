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

#include <utils.h>
#include <gui/my_stock.h>

static const struct {
    const char* icon_name;
    const char* icon_file;
} stock_icons[] = {
    { MY_STOCK_ROTATE_LEFT, "rotate-left.png" },
    { MY_STOCK_ROTATE_RIGHT, "rotate-right.png" },
    { MY_STOCK_PREV_PAGE, "prev-page.png" },
    { MY_STOCK_NEXT_PAGE, "next-page.png" },
    { MY_STOCK_PREV_PAGE_LARGE, "prev-page-large.png" },
    { MY_STOCK_NEXT_PAGE_LARGE, "next-page-large.png" },
    { MY_STOCK_SELECTION, "selection.png" },
    { MY_STOCK_ZOOM_IN, "zoom-in.png" },
    { MY_STOCK_ZOOM_OUT, "zoom-out.png" },
    { MY_STOCK_ZOOM_PAGE, "zoom-fit-page.png" },
    { MY_STOCK_ZOOM_WIDTH, "zoom-fit-width.png" },
    { MY_STOCK_FIND_BUSY, "find-busy.png" },
    { MY_STOCK_FIND_NONE, "find-none.png" },
    { MY_STOCK_FIND_WRAPPED, "find-wrapped.png" }
};

static GtkStockItem stock_items[] = {
    { MY_STOCK_ROTATE_LEFT, "Rotate _Left", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_ROTATE_RIGHT, "Rotate _Right", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_PREV_PAGE, NULL, ( GdkModifierType )0, 0, "" },
    { MY_STOCK_NEXT_PAGE, NULL, ( GdkModifierType )0, 0, "" },
    { MY_STOCK_PREV_PAGE_LARGE, "Prev Page", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_NEXT_PAGE_LARGE, "Next Page", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_SELECTION, "Selection", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_ZOOM_IN, "Zoom In",  ( GdkModifierType )0, 0, "" },
    { MY_STOCK_ZOOM_OUT, "Zoom Out", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_ZOOM_PAGE, "Fit Page", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_ZOOM_WIDTH, "Fit Width", ( GdkModifierType )0, 0, "" },
    { MY_STOCK_FIND_BUSY, NULL, ( GdkModifierType )0, 0, ""  },
    { MY_STOCK_FIND_NONE, NULL, ( GdkModifierType )0, 0, ""  },
    { MY_STOCK_FIND_WRAPPED, NULL, ( GdkModifierType )0, 0, ""  }
};

int init_stock_icons( void ) {
    int i;
    int icon_count;
    char path[ 256 ];
    GtkIconFactory* factory;

    factory = gtk_icon_factory_new();

    gtk_icon_factory_add_default( factory );

    icon_count = G_N_ELEMENTS( stock_icons );

    for ( i = 0; i < icon_count; i++ ) {
        GdkPixbuf* pixbuf;

        snprintf(
            path,
            sizeof( path ),
            "%s%spixmaps%s%s",
            OWL_INSTALL_PATH,
            PATH_SEPARATOR,
            PATH_SEPARATOR,
            stock_icons[ i ].icon_file
        );

        pixbuf = gdk_pixbuf_new_from_file( path, NULL );

        if ( pixbuf != NULL ) {
            GtkIconSet* icon_set;

            icon_set = gtk_icon_set_new_from_pixbuf( pixbuf );

            gtk_icon_factory_add( factory, stock_icons[ i ].icon_name, icon_set );
            gtk_icon_set_unref( icon_set );
        }
    }

    g_object_unref( G_OBJECT( factory ) );

    gtk_stock_add_static( stock_items, G_N_ELEMENTS( stock_items ) );

    return 0;
}
