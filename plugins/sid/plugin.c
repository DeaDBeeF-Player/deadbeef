/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../deadbeef.h"
#include "csid.h"

static const char *exts[] = { "sid",NULL };
const char *filetypes[] = { "SID", NULL };

static const char settings_dlg[] =
    "property \"Enable HVSC\" checkbox hvsc_enable 0;\n"
    "property \"HVSC path\" file hvsc_path \"\";\n"
;

// define plugin interface
DB_decoder_t sid_plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.name = "SID decoder",
    .plugin.descr = "SID player based on libsidplay2",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = NULL,
    .plugin.stop = csid_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.id = "stdsid",
    .init = csid_init,
    .free = csid_free,
    .read_int16 = csid_read,
    .seek = csid_seek,
    .seek_sample = NULL,
    .insert = csid_insert,
    .numvoices = csid_numvoices,
    .mutevoice = csid_mutevoice,
    .exts = exts,
    .filetypes = filetypes,
};
