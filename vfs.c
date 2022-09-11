/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  high-level vfs access interface

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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
#include <string.h>
#include <stdio.h>
#include "vfs.h"
#include "plugins.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#if defined(HAVE_XGUI)
extern int android_use_only_wifi;
extern int android_wifi_status;
extern int android_network_access_request;
#endif

static int
can_use_filename (const char *fname) {
#if defined(HAVE_XGUI)
    int res = (plug_is_local_file (fname) || !android_use_only_wifi || android_wifi_status);
    if (!res) {
        android_network_access_request = 1;
    }
    return res;
#else
    return 1;
#endif
}

static int
can_use_file (DB_FILE *fp) {
#if defined(HAVE_XGUI)
    int res = (!fp->vfs->is_streaming () || !android_use_only_wifi || android_wifi_status);
    if (!res) {
        android_network_access_request = 1;
    }
    return res;
#else
    return 1;
#endif
}

DB_FILE *
vfs_fopen (const char *fname) {
    trace ("vfs_open %s\n", fname);

    if (!can_use_filename (fname)) {
        return NULL;
    }

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
    if (!size || !nmemb) {
        return 0;
    }

    if (!can_use_file (stream)) {
        return 0;
    }
    return stream->vfs->read (ptr, size, nmemb, stream);
}

int
vfs_fseek (DB_FILE *stream, int64_t offset, int whence) {
    if (!can_use_file (stream)) {
        return -1;
    }
    return stream->vfs->seek (stream, offset, whence);
}

int64_t
vfs_ftell (DB_FILE *stream) {
    if (!can_use_file (stream)) {
        return -1;
    }
    return stream->vfs->tell (stream);
}

void
vfs_rewind (DB_FILE *stream) {
    stream->vfs->rewind (stream);
}

int64_t
vfs_fgetlength (DB_FILE *stream) {
    if (!can_use_file (stream)) {
        return -1;
    }
    return stream->vfs->getlength (stream);
}

const char *
vfs_get_content_type (DB_FILE *stream) {
    if (!can_use_file (stream)) {
        return NULL;
    }
    return stream->vfs->get_content_type (stream);
}

void
vfs_fabort (DB_FILE *stream) {
}

uint64_t
vfs_get_identifier (DB_FILE *stream) {
    if (stream->vfs->get_identifier) {
        return stream->vfs->get_identifier (stream);
    }
    return 0;
}

void
vfs_abort_with_identifier (DB_vfs_t *vfs, uint64_t identifier) {
    if (vfs->abort_with_identifier) {
        vfs->abort_with_identifier (identifier);
    }
}
