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

#ifndef _CONFIG_SETTINGS_H_
#define _CONFIG_SETTINGS_H_

#include <gdk/gdkcolor.h>

#include <glib.h>

void load_settings( void );
void save_settings( void );

char* settings_get_desc( char* key );

int settings_get_string( char* key, char** value );
int settings_get_int( char* key, int* value );
int settings_get_color( char* key, GdkColor* color );

void settings_set_string( char* key, char* value );
void settings_set_int( char* key, int value );
void settings_set_color( char* key, GdkColor* color );

void init_settings( void );

#endif /* _CONFIG_SETTINGS_H_ */
