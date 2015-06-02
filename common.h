/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  utility routines and global variables

  Copyright (C) 2009-2013 Alexey Yakovenko

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Alexey Yakovenko waker@users.sourceforge.net
*/
#ifndef __COMMON_H
#define __COMMON_H

#include <limits.h>
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <limits.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

// those are defined in main.c
extern char confdir[PATH_MAX]; // $HOME/.config
extern char dbconfdir[PATH_MAX]; // $HOME/.config/deadbeef
extern char dbinstalldir[PATH_MAX]; // see deadbeef->get_prefix
extern char dbdocdir[PATH_MAX]; // see deadbeef->get_doc_dir
extern char dbplugindir[PATH_MAX]; // see deadbeef->get_plugin_dir
extern char dbpixmapdir[PATH_MAX]; // see deadbeef->get_pixmap_dir
extern char dbcachedir[PATH_MAX];

#endif // __COMMON_H
