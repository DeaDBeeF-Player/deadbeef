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

#include "../../deadbeef.h"
#include "emidi.h"

static const char *exts[] = { "mid",NULL };
const char *filetypes[] = { "MID", NULL };

// define plugin interface
DB_decoder_t emidi_plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.name = "EMU de MIDI player",
    .plugin.descr = "MIDI player based on EMU de MIDI library",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = emidi_start,
    .plugin.stop = emidi_stop,
    .plugin.id = "emidi",
    .open = emidi_open,
    .init = emidi_init,
    .free = emidi_free,
    .read_int16 = emidi_read,
    .seek = emidi_seek,
    .seek_sample = NULL,
    .insert = emidi_insert,
    .exts = exts,
    .filetypes = filetypes,
};
