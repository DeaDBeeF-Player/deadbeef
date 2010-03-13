#ifndef __ARTWORK_H
#define __ARTWORK_H

#include "../../deadbeef.h"

typedef void (*artwork_callback) (const char *fname, const char *artist, const char *album, void *user_data);

char*
fetch (const char *url);

int
fetch_to_file (const char *url, const char *filename);

int
fetch_to_stream (const char *url, FILE *stream);

typedef struct {
    DB_misc_t plugin;
    // returns filename of cached image, or NULL
    char* (*get_album_art) (const char *fname, const char *artist, const char *album, artwork_callback callback, void *user_data);
    // this has to be called to clear queue on exit, before caller terminates
    // `fast=1' means "don't wait, just flush queue"
    void (*reset) (int fast);
} DB_artwork_plugin_t;

#endif /*__ARTWORK_H*/

