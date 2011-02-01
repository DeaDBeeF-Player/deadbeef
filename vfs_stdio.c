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
#include "deadbeef.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static DB_functions_t *deadbeef;
typedef struct {
    DB_vfs_t *vfs;
    FILE *stream;
} STDIO_FILE;

static DB_vfs_t plugin;

static DB_FILE *
stdio_open (const char *fname) {
    if (!memcmp (fname, "file://", 7)) {
        fname += 7;
    }
    FILE *file = fopen (fname, "rb");
    if (!file) {
        return NULL;
    }
    STDIO_FILE *fp = malloc (sizeof (STDIO_FILE));
    fp->vfs = &plugin;
    fp->stream = file;
    return (DB_FILE*)fp;
}

static void
stdio_close (DB_FILE *stream) {
    assert (stream);
    fclose (((STDIO_FILE *)stream)->stream);
    free (stream);
}

static size_t
stdio_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
    return fread (ptr, size, nmemb, ((STDIO_FILE *)stream)->stream);
}

static int
stdio_seek (DB_FILE *stream, int64_t offset, int whence) {
    assert (stream);
    return fseek (((STDIO_FILE *)stream)->stream, offset, whence);
}

static int64_t
stdio_tell (DB_FILE *stream) {
    assert (stream);
    return ftell (((STDIO_FILE *)stream)->stream);
}

static void
stdio_rewind (DB_FILE *stream) {
    assert (stream);
    rewind (((STDIO_FILE *)stream)->stream);
}

static int64_t
stdio_getlength (DB_FILE *stream) {
    assert (stream);
    STDIO_FILE *f = (STDIO_FILE *)stream;
    size_t pos = ftell (f->stream);
    fseek (f->stream, 0, SEEK_END);
    size_t sz = ftell (f->stream);
    fseek (f->stream, pos, SEEK_SET);
    return sz;
}

const char *
stdio_get_content_type (DB_FILE *stream) {
    return NULL;
}

int
stdio_is_streaming (void) {
    return 0;
}

// standard stdio vfs
static DB_vfs_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.name = "stdio vfs",
    .plugin.descr = "Standard IO plugin",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = stdio_open,
    .close = stdio_close,
    .read = stdio_read,
    .seek = stdio_seek,
    .tell = stdio_tell,
    .rewind = stdio_rewind,
    .getlength = stdio_getlength,
    .get_content_type = stdio_get_content_type,
    .is_streaming = stdio_is_streaming
};

DB_plugin_t *
stdio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

