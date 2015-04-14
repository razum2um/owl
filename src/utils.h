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

#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef _WIN32
#define OWL_INSTALL_PATH "C:\\"
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif /* _WIN32 */

#define FORCE_MIN_MAX(x, min, max)        \
    do {                                  \
        if ( (x) < (min) ) {              \
            (x) = (min);                  \
        } else if ( (x) > (max) ) {       \
            (x) = (max);                  \
        }                                 \
    } while ( 0 )

void init_poppler_lock( void );
void destroy_poppler_lock( void );
void poppler_lock( void );
void poppler_unlock( void );

int normalize_angle( int angle );

int follow_uri( char* uri );

#endif /* _UTILS_H_ */
