/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  high-level vfs access interface

  Copyright (C) 2009-2013 Alexey Yakovenko

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

  Alexey Yakovenko waker@users.sourceforge.net
*/
#include <string.h>
#include <stdio.h>
#include "vfs.h"
#include "plugins.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

DB_FILE *
vfs_fopen (const char *fname) {
    trace ("vfs_open %s\n", fname);
    DB_vfs_t **plugs = plug_get_vfs_list ();
    DB_vfs_t *fallback = NULL;
    int i;
    for (i = 0; plugs[i]; i++) {
        DB_vfs_t *p = plugs[i];
        if (!p->get_schemes) {
            fallback = p;
            continue;
        }
        int n;
        const char **scheme_names = p->get_schemes ();
        for (n = 0; scheme_names[n]; n++) {
            size_t l = strlen (scheme_names[n]);
            if (!strncasecmp (scheme_names[n], fname, l)) {
                return p->open (fname);
            }
        }
    }
    if (fallback) {
        return fallback->open (fname);
    }
    return NULL;
}

void vfs_set_track (DB_FILE *stream, DB_playItem_t *it) {
    if (stream->vfs->set_track) {
        stream->vfs->set_track (stream, it);
    }
}

void
vfs_fclose (DB_FILE *stream) {
    return stream->vfs->close (stream);
}

size_t
vfs_fread (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    return stream->vfs->read (ptr, size, nmemb, stream);
}

int
vfs_fseek (DB_FILE *stream, int64_t offset, int whence) {
    return stream->vfs->seek (stream, offset, whence);
}

int64_t
vfs_ftell (DB_FILE *stream) {
    return stream->vfs->tell (stream);
}

void
vfs_rewind (DB_FILE *stream) {
    stream->vfs->rewind (stream);
}

int64_t
vfs_fgetlength (DB_FILE *stream) {
    return stream->vfs->getlength (stream);
}

const char *
vfs_get_content_type (DB_FILE *stream) {
    return stream->vfs->get_content_type (stream);
}

void
vfs_fabort (DB_FILE *stream) {
    if (stream->vfs->abort) {
        stream->vfs->abort (stream);
    }
}
