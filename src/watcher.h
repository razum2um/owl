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

#ifndef _WATCHER_H_
#define _WATCHER_H_

#include <engine/document.h>

#if defined( _WIN32 ) || defined( __FreeBSD__ )
#else
typedef struct watch_entry {
    int wd;
    char path[ 256 ];
    document_t* doc;
} watch_entry_t;
#endif /* _WIN32 || __FreeBSD__ */

int watcher_add_document( document_t* document );
int watcher_remove_document( document_t* document );

int init_watcher_thread( void );
int stop_watcher_thread( void );

#endif /* _WATCHER_H_ */
