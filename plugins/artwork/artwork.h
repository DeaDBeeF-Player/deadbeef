/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __ARTWORK_H
#define __ARTWORK_H

#define DDB_ARTWORK_MAJOR_VERSION 2
#define DDB_ARTWORK_MINOR_VERSION 0

typedef struct ddb_cover_query_s {
    uint32_t _size;

    void *user_data;

    uint32_t flags; // e.g. HAVE_TRACK, HAVE_FILEPATH, HAVE_ALBUM, HAVE_ARTIST
    struct DB_playItem_s *track;
    char *filepath;
    char *album;
    char *artist;
    char *type; // front/back/...
    int imgsize;
} ddb_cover_query_t;

typedef struct {
    // ... the loaded cover info ...
    const char *filename;
} ddb_cover_info_t;

typedef void (*ddb_cover_callback_t) (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover);

typedef struct {
    DB_misc_t plugin;

    void
    (*cover_get) (ddb_cover_query_t *query, ddb_cover_callback_t callback);
} ddb_artwork_plugin_t;

#endif /*__ARTWORK_H*/

