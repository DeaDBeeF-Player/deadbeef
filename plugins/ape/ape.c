#include "../../deadbeef.h"

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

int
ape_init (DB_playItem_t *it) {
    return 0;
}

void
ape_free (void) {
}

int
ape_read (char *buffer, int size) {
    return 0;
}

int
ape_seek (float seconds) {
}

DB_playItem_t *
ape_insert (DB_playItem_t *after, const char *fname) {
}

static const char * exts[] = { "ape", NULL };
static const char *filetypes[] = { "APE", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "Monkey's Audio decoder",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = ape_init,
    .free = ape_free,
    .read_int16 = ape_read,
    .seek = ape_seek,
    .insert = ape_insert,
    .exts = exts,
    .id = "stdape",
    .filetypes = filetypes
};

DB_plugin_t *
oggvorbis_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

