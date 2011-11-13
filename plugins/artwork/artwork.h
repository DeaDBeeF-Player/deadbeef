#ifndef __ARTWORK_H
#define __ARTWORK_H

#include "../../deadbeef.h"

extern DB_FILE *current_file;

typedef void (*artwork_callback) (const char *fname, const char *artist, const char *album, void *user_data);

typedef struct {
    DB_misc_t plugin;
    // returns filename of cached image, or NULL
    char* (*get_album_art) (const char *fname, const char *artist, const char *album, int size, artwork_callback callback, void *user_data);
    // syncronizely get filename
    char* (*get_album_art_sync) (const char *fname, const char *artist, const char *album, int size);

    // this has to be called to clear queue on exit, before caller terminates
    // `fast=1' means "don't wait, just flush queue"
    void (*reset) (int fast);
    const char *(*get_default_cover) (void);
} DB_artwork_plugin_t;

#endif /*__ARTWORK_H*/

