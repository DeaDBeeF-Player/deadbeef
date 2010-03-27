/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <tag_c.h>
#include "../../deadbeef.h"
#include "taglib.h"

DB_functions_t *deadbeef;

int
taglib_plugin_start (void) {
    return 0;
}

int
taglib_plugin_stop (void) {
    return 0;
}

static DB_taglib_t plugin = {
    .misc.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .misc.plugin.api_vminor = DB_API_VERSION_MINOR,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.id = "taglib",
    .misc.plugin.name = "Taglib tag writer",
    .misc.plugin.descr = "Tag writing plugin based on Taglib",
    .misc.plugin.author = "Alexey Yakovenko",
    .misc.plugin.email = "waker@users.sourceforge.net",
    .misc.plugin.website = "http://deadbeef.sf.net",
    .misc.plugin.start = taglib_plugin_start,
    .misc.plugin.stop = taglib_plugin_stop,
};

DB_plugin_t *
taglib_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
