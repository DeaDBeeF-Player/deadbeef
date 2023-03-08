/*
    MMS transport VFS plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

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

#include <deadbeef/deadbeef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libmms/mmsx.h>

DB_functions_t *deadbeef;
typedef struct {
    DB_vfs_t *vfs;
    char *fname;
    mmsx_t *stream;
    const mms_io_t *io;
    int need_abort;
    int64_t pos;
} MMS_FILE;

static DB_vfs_t plugin;

const uint32_t mms_bandwidths[]={14400,19200,28800,33600,34430,57600,
    115200,262200,393216,524300,1544000,10485800};
		
		
const char * mms_bandwidth_strs[]={"14.4 Kbps (Modem)", "19.2 Kbps (Modem)",
    "28.8 Kbps (Modem)", "33.6 Kbps (Modem)",
    "34.4 Kbps (Modem)", "57.6 Kbps (Modem)",
    "115.2 Kbps (ISDN)", "262.2 Kbps (Cable/DSL)",
    "393.2 Kbps (Cable/DSL)","524.3 Kbps (Cable/DSL)",
    "1.5 Mbps (T1)", "10.5 Mbps (LAN)", NULL};
		


static DB_FILE *
mms_open (const char *fname) {
    MMS_FILE *fp = malloc (sizeof (MMS_FILE));
    memset (fp, 0, sizeof (MMS_FILE));
    fp->io = mms_get_default_io_impl();
    fp->fname = strdup (fname);
    fp->vfs = &plugin;
    return (DB_FILE*)fp;
}

static void
mms_close (DB_FILE *stream) {
    //fprintf (stderr, "\033[0;32mmms_close was called\033[37;0m\n");
    assert (stream);
    MMS_FILE *fp = (MMS_FILE *)stream;
    if (fp->stream) {
        mmsx_close (fp->stream);
    }
    if (fp->fname) {
        free (fp->fname);
    }
    free (stream);
}

static int
mms_ensure_connected (MMS_FILE *fp) {
    if (!fp->stream) {
        fp->stream = mmsx_connect ((mms_io_t *)fp->io, fp, fp->fname, 1544000, &fp->need_abort);
        if (!fp->stream) {
            return -1;
        }
    }
    return 0;
}

static size_t
mms_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
    int connect_err = mms_ensure_connected ((MMS_FILE *)stream);
    if (connect_err < 0) {
        return connect_err;
    }
    MMS_FILE *fp = (MMS_FILE *)stream;
    int res = mmsx_read ((mms_io_t *)fp->io, fp->stream, ptr, (int)(size * nmemb));
    fp->pos += res;
    if (fp->need_abort) {
        return -1;
    }
    return res;
}

static int
mms_seek (DB_FILE *stream, int64_t offset, int whence) {
    assert (0);
    assert (stream);
    int connect_err = mms_ensure_connected ((MMS_FILE *)stream);
    if (connect_err < 0) {
        return connect_err;
    }
    MMS_FILE *fp = (MMS_FILE *)stream;
    return mmsx_seek ((mms_io_t *)fp->io, fp->stream, offset, whence);
}

static int64_t
mms_tell (DB_FILE *stream) {
    assert (stream);
    return ((MMS_FILE *)stream)->pos;
//    int connect_err = mms_ensure_connected ((MMS_FILE *)stream);
//    if (connect_err < 0) {
//        return connect_err;
//    }
//    return mmsx_get_current_pos (((MMS_FILE *)stream)->stream);
}

static void
mms_rewind (DB_FILE *stream) {
    assert (stream);
    // FIXME
    return;
}

static int64_t
mms_getlength (DB_FILE *stream) {
    assert (stream);
    int connect_err = mms_ensure_connected ((MMS_FILE *)stream);
    if (connect_err < 0) {
        return connect_err;
    }
    MMS_FILE *f = (MMS_FILE *)stream;
    return mmsx_get_length (f->stream);
}

const char *
mms_get_content_type (DB_FILE *stream) {
    return "audio/wma";
}

static const char *scheme_names[] = { "mms://", "mmsh://", NULL };

const char **
mms_get_schemes (void) {
    return scheme_names;
}

int
mms_is_streaming (void) {
    return 1;
}

static void
mms_abort (DB_FILE *fp) {
    //fprintf (stderr, "\033[0;35mabort called\033[37;0m\n");
    ((MMS_FILE *)fp)->need_abort = 1;
}

static DB_vfs_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.name = "mms vfs",
    .plugin.descr = "MMS streaming plugin based on libmms",
    .plugin.copyright = 
        "MMS transport VFS plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "Uses modified libmms-0.6.1\n"
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
        "libmms - MMS over TCP protocol\n"
        "Copyright (C) 2002-2003 the xine project\n"
        "Copyright (C) 2004-2012 the libmms project\n"
        "Modifications for DeaDBeeF (C) 2009-2016 Oleksiy Yakovenko\n"
        "\n"
        "Libmms is free software; you can redistribute it and/or modify it\n"
        "under the terms of the GNU Library General Public License as\n"
        "published by the Free Software Foundation; either version 2 of the\n"
        "License, or (at your option) any later version.\n"
        "\n"
        "Libmms is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU Library General Public\n"
        "License along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA\n"
        "02111-1307, USA\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .open = mms_open,
    .close = mms_close,
    .read = mms_read,
    .seek = mms_seek,
    .tell = mms_tell,
    .rewind = mms_rewind,
    .getlength = mms_getlength,
    .get_content_type = mms_get_content_type,
    .get_schemes = mms_get_schemes,
    .is_streaming = mms_is_streaming,
    .abort = mms_abort,
};

DB_plugin_t *
mms_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

