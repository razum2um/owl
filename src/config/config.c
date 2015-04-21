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

#include <glib.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <utils.h>

static char* config_directory = NULL;

static int init_config_directory( void ) {
    struct stat st;

    /* Find out the correct path for the config directory */

    config_directory = g_build_filename(g_get_user_config_dir(), "owl", NULL);
    if ( config_directory == NULL ) {
        return -1;
    }

    /* Create the config directory if it doesn't exist */

    if ( g_stat( config_directory, &st ) != 0 ) {
        g_mkdir( config_directory, 0700 );
    }

    return 0;
}

const char* get_config_directory( void ) {
    return config_directory;
}

int init_config( void ) {
    int error;

    error = init_config_directory();

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
