#ifndef __ARTWORK_H
#define __ARTWORK_H

#include "../../deadbeef.h"

typedef void (*artwork_callback) (const char *artist, const char *album);

char*
fetch (const char *url);

int
fetch_to_file (const char *url, const char *filename);

int
fetch_to_stream (const char *url, FILE *stream);

typedef struct {
    DB_misc_t plugin;
    // returns filename of cached image, or NULL
    char* (*get_album_art) (DB_playItem_t *track, artwork_callback callback);
} DB_artwork_plugin_t;

#endif /*__ARTWORK_H*/

