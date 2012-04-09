/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __COMMON_H
#define __COMMON_H

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX    1024    /* max # of characters in a path name */
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

// those are defined in main.c
extern char confdir[PATH_MAX]; // $HOME/.config
extern char dbconfdir[PATH_MAX]; // $HOME/.config/deadbeef
extern char dbinstalldir[PATH_MAX]; // see deadbeef->get_prefix
extern char dbdocdir[PATH_MAX]; // see deadbeef->get_doc_dir
extern char dbplugindir[PATH_MAX]; // see deadbeef->get_plugin_dir
extern char dbpixmapdir[PATH_MAX]; // see deadbeef->get_pixmap_dir


#endif // __COMMON_H
