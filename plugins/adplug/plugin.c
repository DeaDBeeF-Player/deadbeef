/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

// this is a decoder plugin skeleton
// use to create new decoder plugins

#include "../../deadbeef.h"

extern const char *adplug_exts[];
extern const char *adplug_filetypes[];

int
adplug_init (DB_playItem_t *it);
void
adplug_free (void);
int
adplug_read_int16 (char *bytes, int size);
int
adplug_seek_sample (int sample);
int
adplug_seek (float time);
DB_playItem_t *
adplug_insert (DB_playItem_t *after, const char *fname);
int
adplug_start (void);
int
adplug_stop (void);

// define plugin interface
DB_decoder_t adplug_plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "short plugin name",
    .plugin.descr = "plugin description",
    .plugin.author = "author name",
    .plugin.email = "author email",
    .plugin.website = "author/plugin website",
    .plugin.start = adplug_start,
    .plugin.stop = adplug_stop,
    .init = adplug_init,
    .free = adplug_free,
    .read_int16 = adplug_read_int16,
    .seek = adplug_seek,
    .seek_sample = adplug_seek_sample,
    .insert = adplug_insert,
    .exts = adplug_exts,
    .id = "adplug",
    .filetypes = adplug_filetypes
};

