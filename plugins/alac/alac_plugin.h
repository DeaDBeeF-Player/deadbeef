#ifndef __ALAC_PLUGIN_H
#define __ALAC_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

DB_fileinfo_t *
alacplug_open (uint32_t hints);

int
alacplug_init (DB_fileinfo_t *_info, DB_playItem_t *it);

void
alacplug_free (DB_fileinfo_t *_info);

int
alacplug_read (DB_fileinfo_t *_info, char *bytes, int size);

int
alacplug_seek_sample (DB_fileinfo_t *_info, int sample);

int
alacplug_seek (DB_fileinfo_t *_info, float t);

DB_playItem_t *
alacplug_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname);

#ifdef __cplusplus
}
#endif

#endif
