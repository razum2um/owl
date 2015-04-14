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

#ifdef _WIN32
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif /* _WIN32 */

#include <glib.h>

#include <utils.h>
#include <config/settings.h>

/* Poppler is not thread safe. What an epic failure. */
static GMutex* big_poppler_lock;

void init_poppler_lock( void ) {
    big_poppler_lock = g_mutex_new();
}

void destroy_poppler_lock( void ) {
    g_mutex_free( big_poppler_lock );
}

void poppler_lock( void ) {
    g_mutex_lock( big_poppler_lock );
}

void poppler_unlock( void ) {
    g_mutex_unlock( big_poppler_lock );
}

int normalize_angle( int angle ) {
    while ( angle < 0 ) {
        angle += 360;
    }

    if ( angle >= 360 ) {
        angle %= 360;
    }

    return angle;
}

int follow_uri( char* uri ) {
    int error;
    size_t size;
    char* command;
    char* browser;

    if ( settings_get_string( "browser-path", &browser ) < 0 ) {
        return -1;
    }

    if ( strlen( browser ) == 0 ) {
        return -1;
    }

    size = strlen( browser ) + strlen( uri ) + 4;
    command = ( char* )malloc( size );

    if ( command == NULL ) {
        return -1;
    }

    snprintf( command, size, "%s '%s'", browser, uri );

    error = system( command );

    free( command );

    return error;
}
