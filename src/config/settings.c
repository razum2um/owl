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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utils.h>
#include <config/config.h>
#include <config/settings.h>

static GHashTable* settings;
static char settings_file[ 256 ];

typedef struct {
    char* key;
    char* value;
    char* desc;
} settings_t;

static settings_t settings_keys[] = {
    /* managed internally */
    { "open-file-path", "", NULL },
    { "save-file-path", "", NULL },

    /* exposed through settings dialog */
    { "show-docs-from-last-time", "1", "Show documents from last time" },
    { "always-show-tabs", "0", "Always show tabs" },
    { "show-navbar", "1", "Show navigation bar" },
    { "show-toolbar", "1", "Show toolbar" },
    { "hide-pointer", "1", "Hide pointer in presentation mode" },
    { "pointer-idle-time", "3", "Pointer idle time [s]" },
    { "tool-button-style", "0", "Toolbar button style" }, /* 0 = both text & image, 1 = only image, 2 = only text */
    { "browser-path", "", "Browser path" },

    { "enable-render-limit-pages", "1", "Limit the maximum number of pre-rendered pages" },
    { "render-limit-pages", "10", "Number of pages to pre-render\nbefore and after the current page" },
    { "enable-render-limit-mb", "0", "Limit the maximum memory used for page cache" },
    { "render-limit-mb", "100", "Memory limit for page cache [MB]" },
    { "open-in-running-instance", "0", "Open new documents in a running Owl instance" },

    { "color-selection", "#000040008000", "Selection color" },
    { "color-highlight", "#0000c0000000", "Highlight color" },

    { NULL, NULL, NULL }
};

static void destroy_func( gpointer data ) {
    free( data );
}

static void load_defaults( void ) {
    int ret;
    settings_t* item;
    char* value;

    for ( item = settings_keys; item->key != NULL; item++ ) {
        ret = settings_get_string( item->key, &value );

        if ( ret < 0 ) {
            settings_set_string( item->key, item->value );
        }
    }
}

static void load_default_browser( void ) {
    char* browser;

    if ( settings_get_string( "browser-path", &browser ) < 0 ) {
        return;
    }

    if ( strlen( browser ) > 0 ) {
        return;
    }

    browser = getenv( "BROWSER" );

    if ( browser != NULL ) {
        settings_set_string( "browser-path", browser );
    }
}

void init_settings( void ) {
    snprintf(
        settings_file,
        sizeof( settings_file ),
        "%s%ssettings",
        get_config_directory(),
        PATH_SEPARATOR
    );

    settings = g_hash_table_new_full( g_str_hash, g_str_equal, destroy_func, destroy_func );

    load_settings();
    load_defaults();
    load_default_browser();
}

void load_settings( void ) {
    FILE* in;
    char line[ 512 ];
    char* key;
    char* value;
    char* end;

    in = fopen( settings_file, "rt" );

    if ( in == NULL ) {
        return;
    }

    while ( fgets( line, sizeof( line ), in ) != NULL ) {
        key = line;

        value = strchr( line, '=' );

        if ( value == NULL ) {
            continue;
        }

        *value++ = 0;

        key = strdup( line );

        if ( key == NULL ) {
            goto out;
        }

        end = value + strlen( value ) - 1;

        while ( ( *end == '\n' ) || ( *end == '\r' ) ) {
            *end-- = 0;
        }

        g_hash_table_insert( settings, key, strdup( value ) );
    }

 out:
    fclose( in );
}

void save_settings( void ) {
    FILE* out;
    int ret;
    settings_t* item;
    char* value;

    out = fopen( settings_file, "wt" );

    if ( out == NULL ) {
        return;
    }

    for ( item = settings_keys; item->key != NULL; item++ ) {
        ret = settings_get_string( item->key, &value );

        if ( ret < 0 ) {
            break;
        }

        fprintf( out, "%s=%s\n", item->key, value );
    }

    fclose( out );
}

char* settings_get_desc( char* key ) {
    settings_t* item;

    for ( item = settings_keys; item->key != NULL; item++ ) {
        if ( strcmp( item->key, key ) == 0 ) {
            return item->desc;
        }
    }

    return NULL;
}

int settings_get_string( char* key, char** value ) {
    gpointer val = g_hash_table_lookup( settings, key );

    if ( val == NULL ) {
        return -1;
    }

    *value = ( char* )val;

    return 0;
}

int settings_get_int( char* key, int* value ) {
    gpointer val = g_hash_table_lookup( settings, key );

    if ( val == NULL ) {
        return -1;
    }

    if ( sscanf( ( char* )val, "%d", value ) == 1 ) {
        return 0;
    }

    return -1;
}

int settings_get_color( char* key, GdkColor* color ) {
    gpointer val = g_hash_table_lookup( settings, key );

    if ( val == NULL ) {
        return -1;
    }

    if ( gdk_color_parse( ( char* )val, color ) ) {
        return 0;
    }

    return -1;
}

void settings_set_string( char* key, char* value ) {
    g_hash_table_replace( settings, strdup( key ), strdup( value ) );
}

void settings_set_int( char* key, int value ) {
    char val[ 16 ];

    snprintf( val, sizeof( val ), "%d", value );

    g_hash_table_replace( settings, strdup( key ), strdup ( val ) );
}

void settings_set_color( char* key, GdkColor* color ) {
    gchar* val;

    val = gdk_color_to_string( color );

    g_hash_table_replace( settings, strdup( key ), val );
}
