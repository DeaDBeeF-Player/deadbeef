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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libmms/mmsx.h>

static DB_functions_t *deadbeef;
typedef struct {
    DB_vfs_t *vfs;
    mmsx_t *stream;
    const mms_io_t *io;
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
    fp->io = mms_get_default_io_impl();
    fp->stream = mmsx_connect ((mms_io_t *)fp->io, fp, fname, 1544000);
    if (!fp->stream) {
        free (fp);
        return NULL;
    }
    fp->vfs = &plugin;
    return (DB_FILE*)fp;
}

static void
mms_close (DB_FILE *stream) {
    assert (stream);
    mmsx_close (((MMS_FILE *)stream)->stream);
    free (stream);
}

static size_t
mms_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
    MMS_FILE *fp = (MMS_FILE *)stream;
    int res = mmsx_read ((mms_io_t *)fp->io, fp->stream, ptr, size * nmemb);
    return res;
}

static int
mms_seek (DB_FILE *stream, int64_t offset, int whence) {
    assert (stream);
    MMS_FILE *fp = (MMS_FILE *)stream;
    return mmsx_seek ((mms_io_t *)fp->io, fp->stream, offset, whence);
}

static int64_t
mms_tell (DB_FILE *stream) {
    assert (stream);
    return mmsx_get_current_pos (((MMS_FILE *)stream)->stream);
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
    MMS_FILE *f = (MMS_FILE *)stream;
    return mmsx_get_length (f->stream);
}

const char *
mms_get_content_type (DB_FILE *stream) {
    return "audio/wma";
}

static const char *scheme_names[] = { "mms://", "mmsh://", NULL };

static DB_vfs_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.name = "mms vfs",
    .plugin.descr = "MMS streaming plugin based on libmms",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = mms_open,
    .close = mms_close,
    .read = mms_read,
    .seek = mms_seek,
    .tell = mms_tell,
    .rewind = mms_rewind,
    .getlength = mms_getlength,
    .get_content_type = mms_get_content_type,
    .scheme_names = scheme_names,
    .streaming = 1
};

DB_plugin_t *
mms_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

