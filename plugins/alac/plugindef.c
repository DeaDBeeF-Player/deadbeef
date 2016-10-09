#include "../../deadbeef.h"
#include "alac_plugin.h"
#include "../../shared/mp4tagutil.h"

DB_functions_t *deadbeef;

static const char * exts[] = { "mp4", "m4a", NULL };

// define plugin interface
DB_decoder_t alac_plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "alac",
    .plugin.name = "ALAC player",
    .plugin.descr = "plays alac files from MP4 and M4A files",
    .plugin.copyright = 
        "ALAC plugin for deadbeef\n"
        "Copyright (C) 2012-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "Uses the reverse engineered ALAC decoder (C) 2005 David Hammerton\n"
        "All rights reserved.\n"
        "\n"
        "Permission is hereby granted, free of charge, to any person\n"
        "obtaining a copy of this software and associated documentation\n"
        "files (the \"Software\"), to deal in the Software without\n"
        "restriction, including without limitation the rights to use,\n"
        "copy, modify, merge, publish, distribute, sublicense, and/or\n"
        "sell copies of the Software, and to permit persons to whom the\n"
        "Software is furnished to do so, subject to the following conditions:\n"
        "\n"
        "The above copyright notice and this permission notice shall be\n"
        "included in all copies or substantial portions of the Software.\n"
        "\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
        "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES\n"
        "OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
        "NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT\n"
        "HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
        "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
        "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
        "OTHER DEALINGS IN THE SOFTWARE.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .open = alacplug_open,
    .init = alacplug_init,
    .free = alacplug_free,
    .read = alacplug_read,
    .seek = alacplug_seek,
    .seek_sample = alacplug_seek_sample,
    .insert = alacplug_insert,
    .read_metadata = mp4_read_metadata,
    .write_metadata = mp4_write_metadata,
    .exts = exts,
};

DB_plugin_t *
alac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&alac_plugin);
}

