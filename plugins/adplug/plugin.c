/*
    DeaDBeeF ADPLUG plugin
    Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
*/

// this is a decoder plugin skeleton
// use to create new decoder plugins

#include <deadbeef/deadbeef.h>

extern const char *adplug_exts[];

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
adplug_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname);
int
adplug_start (void);
int
adplug_stop (void);

static const char settings_dlg[] =
    "property \"Prefer Ken emu over Satoh (surround won't work)\" checkbox adplug.use_ken 0;\n"
    "property \"Enable surround\" checkbox adplug.surround 1;\n"
;

// define plugin interface
DB_decoder_t adplug_plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "adplug",
    .plugin.name = "Adplug player",
    .plugin.descr = "Adplug player (ADLIB OPL2/OPL3 emulator)",
    .plugin.copyright = 
    "ADPLUG DeaDBeeF Player Plugin\n"
    "Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
    "\n"
    "This software is provided 'as-is', without any express or implied\n"
    "warranty.  In no event will the authors be held liable for any damages\n"
    "arising from the use of this software.\n"
    "\n"
    "Permission is granted to anyone to use this software for any purpose,\n"
    "including commercial applications, and to alter it and redistribute it\n"
    "freely, subject to the following restrictions:\n"
    "\n"
    "1. The origin of this software must not be misrepresented; you must not\n"
    " claim that you wrote the original software. If you use this software\n"
    " in a product, an acknowledgment in the product documentation would be\n"
    " appreciated but is not required.\n"
    "\n"
    "2. Altered source versions must be plainly marked as such, and must not be\n"
    " misrepresented as being the original software.\n"
    "\n"
    "3. This notice may not be removed or altered from any source distribution.\n"
    "\n"
    "\n"
    "\n"
    "adplug (modified)\n"
    "Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.\n"
    "deadbeef-related modifications (c) 2009-2014 Oleksiy Yakovenko\n"
    "\n"
    "This library is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU Lesser General Public\n"
    "License as published by the Free Software Foundation; either\n"
    "version 2.1 of the License, or (at your option) any later version.\n"
    "\n"
    "This library is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
    "Lesser General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU Lesser General Public\n"
    "License along with this library; if not, write to the Free Software\n"
    "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = adplug_start,
    .plugin.stop = adplug_stop,
    .plugin.configdialog = settings_dlg,
    .open = adplug_open,
    .init = adplug_init,
    .free = adplug_free,
    .read = adplug_read,
    .seek = adplug_seek,
    .seek_sample = adplug_seek_sample,
    .insert = adplug_insert,
    .exts = adplug_exts,
};

