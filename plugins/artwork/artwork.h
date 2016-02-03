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

#include "../../deadbeef.h"

#define DDB_ARTWORK_VERSION 3

typedef void (*artwork_callback) (const char *fname, const char *artist, const char *album, void *user_data);

typedef struct {
    DB_misc_t plugin;
    // returns filename of cached image, or NULL
    char* (*get_album_art) (const char *fname, const char *artist, const char *album, int size, artwork_callback callback, void *user_data);

    // this has to be called to clear queue on exit, before caller terminates
    // `fast=1' means "don't wait, just flush queue"
    void (*reset) (int fast);

    // returns path to default album art
    const char *(*get_default_cover) (void);

    // synchronously get filename
    char* (*get_album_art_sync) (const char *fname, const char *artist, const char *album, int size);

    // creates full path string for cache storage
    void (*make_cache_path) (char *path, int size, const char *album, const char *artist, int img_size);

    // creates full path string for cache storage
    int (*make_cache_path2) (char *path, int size, const char *fname, const char *album, const char *artist, int img_size);
} DB_artwork_plugin_t;

#endif /*__ARTWORK_H*/

