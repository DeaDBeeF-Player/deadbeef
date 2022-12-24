/*
    SID plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>

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

static const char *exts[] = { "sid", NULL };
static const char settings_dlg[] =
    "property \"Enable HVSC Songlength DB\" checkbox hvsc_enable 0;\n"
    "property \"Full path to Songlengths.md5/.txt\" file hvsc_path \"\";\n"
    "property \"Samplerate\" entry sid.samplerate 44100;\n"
    "property \"Bits per sample\" select[2] sid.bps 0 16 8;\n"
    "property \"Default song length (sec)\" entry sid.defaultlength 180;\n"
    "property \"Mono synth\" checkbox sid.mono 0;\n"
;

// define plugin interface
DB_decoder_t sid_plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.name = "SID player",
    .plugin.id = "sidplayfp",
    .plugin.descr = "SID player based on libsidplay2",
    .plugin.copyright = 
        "SID plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation, either version 2 of the License, or\n"
        "(at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = csid_start,
    .plugin.stop = csid_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.message = sid_message,
    .open = csid_open,
    .init = csid_init,
    .free = csid_free,
    .read = csid_read,
    .seek = csid_seek,
    .seek_sample = NULL,
    .insert = csid_insert,
//    .numvoices = csid_numvoices,
//    .mutevoice = csid_mutevoice,
    .exts = exts,
};

