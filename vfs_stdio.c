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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static DB_functions_t *deadbeef;
typedef struct {
    DB_vfs_t *vfs;
    int stream;
    int64_t offs;
} STDIO_FILE;

static DB_vfs_t plugin;

static DB_FILE *
stdio_open (const char *fname) {
    if (!memcmp (fname, "file://", 7)) {
        fname += 7;
    }
    int file = open (fname, O_LARGEFILE);
    if (file == -1) {
        return NULL;
    }
    STDIO_FILE *fp = malloc (sizeof (STDIO_FILE));
    fp->vfs = &plugin;
    fp->stream = file;
    fp->offs = 0;
    return (DB_FILE*)fp;
}

static void
stdio_close (DB_FILE *stream) {
    assert (stream);
    close (((STDIO_FILE *)stream)->stream);
    free (stream);
}

static size_t
stdio_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
    int res = read (((STDIO_FILE*)stream)->stream, ptr, size*nmemb);
    if (res == -1) {
        return 0;
    }
    ((STDIO_FILE*)stream)->offs += res;
    return res / size;
}

static int
stdio_seek (DB_FILE *stream, int64_t offset, int whence) {
    assert (stream);
    off64_t res = lseek64 (((STDIO_FILE *)stream)->stream, offset, whence);
    if (res == -1) {
        return -1;
    }
    ((STDIO_FILE*)stream)->offs = res; 
    return 0;
}

static int64_t
stdio_tell (DB_FILE *stream) {
    assert (stream);
    return ((STDIO_FILE*)stream)->offs;
}

static void
stdio_rewind (DB_FILE *stream) {
    assert (stream);
    stdio_seek (stream, 0, SEEK_SET);
}

static int64_t
stdio_getlength (DB_FILE *stream) {
    assert (stream);
    STDIO_FILE *f = (STDIO_FILE *)stream;
    int64_t size = lseek64 (f->stream, 0, SEEK_END);
    lseek64 (f->stream, f->offs, SEEK_SET);
    return size;
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
    .plugin.descr = "Standard IO plugin\nUsed for reading normal local files\nIt is statically linked, so you can't delete it.",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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

