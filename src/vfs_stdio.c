/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  standard file vfs implementation

  Copyright (C) 2009-2015 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#include <deadbeef/deadbeef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef __linux__
#define O_LARGEFILE 0
#endif

//#define USE_STDIO
#define USE_BUFFERING

#ifndef USE_STDIO
#define BUFSIZE 1024
#endif

static DB_functions_t *deadbeef;
typedef struct {
    DB_vfs_t *vfs;
#ifdef USE_STDIO
    FILE *stream;
#else
    int stream;
    int64_t offs;
#ifdef USE_BUFFERING
    uint8_t buffer[BUFSIZE];
    uint8_t *bufptr;
    int bufremaining;
#endif
    int have_size;
    size_t size;
#endif
} STDIO_FILE;

static DB_vfs_t plugin;

static DB_FILE *
stdio_open (const char *fname) {
    if (!memcmp (fname, "file://", 7)) {
        fname += 7;
    }
#ifdef USE_STDIO
    FILE *file = fopen (fname, "rb");
    if (!file) {
        return NULL;
    }
#else
    int file = open (fname, O_LARGEFILE);
    if (file == -1) {
        return NULL;
    }
#endif
    STDIO_FILE *fp = malloc (sizeof (STDIO_FILE));
    memset (fp, 0, sizeof (STDIO_FILE));
    fp->vfs = &plugin;
    fp->stream = file;
    return (DB_FILE*)fp;
}

static void
stdio_close (DB_FILE *stream) {
    assert (stream);
#ifdef USE_STDIO
    fclose (((STDIO_FILE *)stream)->stream);
#else
    close (((STDIO_FILE *)stream)->stream);
#endif
    free (stream);
}

#ifndef USE_STDIO
#ifdef USE_BUFFERING
static int
fillbuffer (STDIO_FILE *f) {
    assert (f->bufremaining >= 0);
    if (f->bufremaining == 0) {
        f->bufremaining = (int)read (f->stream, f->buffer, BUFSIZE);
        if (f->bufremaining < 0) {
            f->bufremaining = 0;
            return -1;
        }
        f->bufptr = f->buffer;
    }
    return f->bufremaining;
}
#endif
#endif

static size_t
stdio_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
#ifdef USE_STDIO
    return fread (ptr, size, nmemb, ((STDIO_FILE*)stream)->stream);
#else
    STDIO_FILE *f = (STDIO_FILE*)stream;

    size_t nb = size * nmemb;
#ifdef USE_BUFFERING
    while (nb > 0) {
        if (fillbuffer (f) <= 0) {
            break;
        }
        int r = f->bufremaining;
        if (r > nb) {
            r = (int)nb;
        }
        memcpy (ptr, f->bufptr, r);
        f->bufremaining -= r;
        f->bufptr += r;
        ptr += r;
        f->offs += r;
        nb -= r;
    }
    size_t ret = ((size * nmemb) - nb) / size;
#else
    ssize_t ret = read (f->stream, ptr, nb);
    if (ret < 0) {
        return -1;
    }
    f->offs += ret;
    ret = ret / size;
#endif
    return ret;
#endif
}

static int
stdio_seek (DB_FILE *stream, int64_t offset, int whence) {
    assert (stream);
#ifdef USE_STDIO
    return fseek (((STDIO_FILE *)stream)->stream, offset, whence);
#else
    // convert offset to absolute
    if (whence == SEEK_CUR) {
        whence = SEEK_SET;
        offset = ((STDIO_FILE*)stream)->offs + offset;
    }
    off_t res = lseek (((STDIO_FILE *)stream)->stream, offset, whence);
    if (res == -1) {
        return -1;
    }
//    printf ("lseek res: %lld (%lld, %d, prev=%lld)\n", res, offset, whence,  ((STDIO_FILE*)stream)->offs);
    ((STDIO_FILE*)stream)->offs = res;
#ifdef USE_BUFFERING
    ((STDIO_FILE*)stream)->bufremaining = 0;
#endif
#endif
    return 0;
}

static int64_t
stdio_tell (DB_FILE *stream) {
    assert (stream);
#ifdef USE_STDIO
    return ftell (((STDIO_FILE*)stream)->stream);
#else
    return ((STDIO_FILE*)stream)->offs;
#endif
}

static void
stdio_rewind (DB_FILE *stream) {
    assert (stream);
#ifdef USE_STDIO
    rewind (((STDIO_FILE*)stream)->stream);
#else
    stdio_seek (stream, 0, SEEK_SET);
#endif
}

static int64_t
stdio_getlength (DB_FILE *stream) {
    assert (stream);
    STDIO_FILE *f = (STDIO_FILE *)stream;
#ifdef USE_STDIO
    size_t offs = ftell (f->stream);
    fseek (f->stream, 0, SEEK_END);
    size_t l = ftell (f->stream);
    fseek (f->stream, offs, SEEK_SET);
    return l;
#else
    if (!f->have_size) {
        off_t size = lseek (f->stream, 0, SEEK_END);
        lseek (f->stream, f->offs, SEEK_SET);
#ifdef USE_BUFFERING
        f->bufremaining = 0;
#endif
        f->have_size = 1;
        f->size = size;
    }
    return f->size;
#endif
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
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.name = "stdio vfs",
    .plugin.id = "vfs_stdio",
    .plugin.descr = "Standard IO plugin\nUsed for reading normal local files\nIt is statically linked, so you can't delete it.",
    .plugin.copyright = 
        "standard file vfs implementation\n"
        "\n"
        "Copyright (C) 2009-2015 Oleksiy Yakovenko\n"
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
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
        "\n"
        "Oleksiy Yakovenko waker@users.sourceforge.net\n"
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

