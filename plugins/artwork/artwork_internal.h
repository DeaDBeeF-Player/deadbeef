/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#ifndef __ARTWORK_INTERNAL_H
#define __ARTWORK_INTERNAL_H

#include <limits.h>
#include <deadbeef/deadbeef.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define FETCH_CONCURRENT_LIMIT 5

struct ddb_cover_info_priv_s {
    // query info
    time_t timestamp; // Last time when the info was used last time
    char filepath[PATH_MAX];
    char album[1000];
    char artist[1000];
    char title[1000];
    int is_compilation;

    char track_cache_path[PATH_MAX];
    char album_cache_path[PATH_MAX];

    char *blob; // A blob with the image data, or NULL
    uint64_t blob_size; // Size of the blob
    uint64_t blob_image_offset; // offset where the image data starts in the blob
    uint64_t blob_image_size; // size of the image at offset

    int refc; // Reference count, to allow sending the same cover to multiple callbacks

    // prev/next in the list of all alive cover_info_t objects
    struct ddb_cover_info_s *prev;
    struct ddb_cover_info_s *next;
};

size_t artwork_http_request(const char *url, char *buffer, const size_t max_bytes);

void artwork_abort_all_http_requests (void);

int ensure_dir(const char *path);
int copy_file (const char *in, const char *out);
int write_file(const char *out, const char *data, const size_t data_length);

#endif /*__ARTWORK_INTERNAL_H*/
