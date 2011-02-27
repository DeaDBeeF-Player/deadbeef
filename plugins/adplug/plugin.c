/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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

DB_fileinfo_t *
adplug_open (uint32_t hints);
int
adplug_init (DB_fileinfo_t *_info, DB_playItem_t *it);
void
adplug_free (DB_fileinfo_t *);
int
adplug_read (DB_fileinfo_t *, char *bytes, int size);
int
adplug_seek_sample (DB_fileinfo_t *, int sample);
int
adplug_seek (DB_fileinfo_t *, float time);
DB_playItem_t *
adplug_insert (DB_playItem_t *after, const char *fname);
int
adplug_start (void);
int
adplug_stop (void);

// define plugin interface
DB_decoder_t adplug_plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "adplug",
    .plugin.name = "Adplug player",
    .plugin.descr = "Adplug player (ADLIB OPL2/OPL3 emulator)",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses modified AdPlug library\n"
        "Copyright (C) 1999 - 2010 Simon Peter, et al.\n"
        "http://adplug.sourceforge.net/\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = adplug_start,
    .plugin.stop = adplug_stop,
    .open = adplug_open,
    .init = adplug_init,
    .free = adplug_free,
    .read = adplug_read,
    .seek = adplug_seek,
    .seek_sample = adplug_seek_sample,
    .insert = adplug_insert,
    .exts = adplug_exts,
    .filetypes = adplug_filetypes
};

