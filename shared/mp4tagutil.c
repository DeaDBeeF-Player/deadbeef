/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

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

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "mp4tagutil.h"
#include "mp4ff.h"

#ifndef __linux__
#define off64_t off_t
#define lseek64 lseek
#define O_LARGEFILE 0
#endif

extern DB_functions_t *deadbeef;

typedef struct {
    DB_FILE *file;
    int64_t junk;
} file_info_t;

static uint32_t
_fs_read (void *user_data, void *buffer, uint32_t length) {
    file_info_t *info = user_data;
    return (uint32_t)deadbeef->fread (buffer, 1, length, info->file);
}

static uint32_t
_fs_seek (void *user_data, uint64_t position) {
    file_info_t *info = user_data;
    return deadbeef->fseek (info->file, position+info->junk, SEEK_SET);
}

static uint32_t stdio_read (void *user_data, void *buffer, uint32_t length) {
    return (uint32_t)read (*(int *)user_data, buffer, length);
}

static uint32_t stdio_write (void *user_data, void *buffer, uint32_t length) {
    return (uint32_t)write (*(int *)user_data, buffer, length);
}

static uint32_t stdio_seek (void *user_data, uint64_t position) {
    return (uint32_t)lseek64 (*(int *)user_data, position, SEEK_SET);
}

static uint32_t stdio_truncate (void *user_data) {
    off64_t pos = lseek64 (*(int *)user_data, 0, SEEK_CUR);
    return ftruncate(*(int *)user_data, pos);
}

static const char *metainfo[] = {
    "artist", "artist",
    "title", "title",
    "album", "album",
    "track", "track",
    "date", "year",
    "genre", "genre",
    "comment", "comment",
    "performer", "performer",
    "album_artist", "band",
    "writer", "composer",
    "vendor", "vendor",
    "disc", "disc",
    "compilation", "compilation",
    "totaldiscs", "numdiscs",
    "copyright", "copyright",
    "totaltracks", "numtracks",
    "tool", "tool",
    "MusicBrainz Track Id", "musicbrainz_trackid",
    NULL
};

int
mp4_write_metadata (DB_playItem_t *it) {
    deadbeef->pl_lock ();
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();

    if (!fp) {
        return -1;
    }

    file_info_t inf;
    memset (&inf, 0, sizeof (inf));
    inf.file = fp;
    inf.junk = deadbeef->junk_get_leading_size (fp);
    if (inf.junk >= 0) {
        deadbeef->fseek (inf.file, inf.junk, SEEK_SET);
    }
    else {
        inf.junk = 0;
    }

    mp4ff_callback_t read_cb = {
        .read = _fs_read,
        .write = NULL,
        .seek = _fs_seek,
        .truncate = NULL,
        .user_data = &inf
    };

    mp4ff_t *mp4 = mp4ff_open_read (&read_cb);
    deadbeef->fclose (fp);

    if (!mp4) {
        return -1;
    }

    deadbeef->pl_lock ();
    int fd_out = open (deadbeef->pl_find_meta (it, ":URI"), O_LARGEFILE | O_RDWR);
    deadbeef->pl_unlock ();

    mp4ff_callback_t write_cb = {
        .read = stdio_read,
        .write = stdio_write,
        .seek = stdio_seek,
        .truncate = stdio_truncate,
        .user_data = &fd_out
    };

    mp4ff_tag_delete (&mp4->tags);

    deadbeef->pl_lock ();
    DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (it);
    while (m) {
        if (strchr (":!_", m->key[0])) {
            break;
        }
        int i;
        for (i = 0; metainfo[i]; i += 2) {
            if (!strcasecmp (metainfo[i+1], m->key)) {
                break;
            }
        }

        const char *value = m->value;
        const char *end = m->value + m->valuesize;
        while (value < end) {
            mp4ff_tag_add_field (&mp4->tags, metainfo[i] ? metainfo[i] : m->key, value);
            size_t l = strlen (value) + 1;
            value += l;
        }
        m = m->next;
    }

    static const char *tag_rg_names[] = {
        "replaygain_album_gain",
        "replaygain_album_peak",
        "replaygain_track_gain",
        "replaygain_track_peak",
        NULL
    };

    // replaygain key names in deadbeef internal metadata
    static const char *ddb_internal_rg_keys[] = {
        ":REPLAYGAIN_ALBUMGAIN",
        ":REPLAYGAIN_ALBUMPEAK",
        ":REPLAYGAIN_TRACKGAIN",
        ":REPLAYGAIN_TRACKPEAK",
        NULL
    };

    // add replaygain values
    for (int n = 0; ddb_internal_rg_keys[n]; n++) {
        if (deadbeef->pl_find_meta (it, ddb_internal_rg_keys[n])) {
            float value = deadbeef->pl_get_item_replaygain (it, n);
            char s[100];
            snprintf (s, sizeof (s), "%f", value);
            mp4ff_tag_add_field (&mp4->tags, tag_rg_names[n], s);
        }
    }

    deadbeef->pl_unlock ();

    int32_t res = mp4ff_meta_update(&write_cb, &mp4->tags);

    mp4ff_close (mp4);
    close (fd_out);
    
    return !res;
}

static void
mp4_load_tags (DB_playItem_t *it, mp4ff_t *mp4) {
    int got_itunes_tags = 0;

    int n = mp4ff_meta_get_num_items (mp4);
    for (int t = 0; t < n; t++)  {
        char *key = NULL;
        char *value = NULL;
        int res = mp4ff_meta_get_by_index(mp4, t, &key, &value);
        if (res) {
            got_itunes_tags = 1;
            if (strcasecmp (key, "cover")) {
                if (!strcasecmp (key, "replaygain_track_gain")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (value));
                }
                else if (!strcasecmp (key, "replaygain_album_gain")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (value));
                }
                else if (!strcasecmp (key, "replaygain_track_peak")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (value));
                }
                else if (!strcasecmp (key, "replaygain_album_peak")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (value));
                }
                else {
                    int i;
                    for (i = 0; metainfo[i]; i += 2) {
                        if (!strcasecmp (metainfo[i], key)) {
                            deadbeef->pl_append_meta (it, metainfo[i+1], value);
                            break;
                        }
                    }
                    if (!metainfo[i]) {
                        deadbeef->pl_append_meta (it, key, value);
                    }
                }
            }
        }
        if (key) {
            free (key);
        }
        if (value) {
            free (value);
        }
    }

    if (got_itunes_tags) {
        uint32_t f = deadbeef->pl_get_item_flags (it);
        f |= DDB_TAG_ITUNES;
        deadbeef->pl_set_item_flags (it, f);
    }
}

int
mp4_read_metadata_file (DB_playItem_t *it, DB_FILE *fp) {
    file_info_t inf;
    memset (&inf, 0, sizeof (inf));
    inf.file = fp;
    inf.junk = deadbeef->junk_get_leading_size (fp);
    if (inf.junk >= 0) {
        deadbeef->fseek (inf.file, inf.junk, SEEK_SET);
    }
    else {
        inf.junk = 0;
    }

    mp4ff_callback_t cb = {
        .read = _fs_read,
        .write = NULL,
        .seek = _fs_seek,
        .truncate = NULL,
        .user_data = &inf
    };

    deadbeef->pl_delete_all_meta (it);

    mp4ff_t *mp4 = mp4ff_open_read (&cb);
    if (mp4) {
        mp4_load_tags (it, mp4);
        mp4ff_close (mp4);
    }
    (void)deadbeef->junk_apev2_read (it, fp);
    (void)deadbeef->junk_id3v2_read (it, fp);
    (void)deadbeef->junk_id3v1_read (it, fp);
    return 0;
}

int
mp4_read_metadata (DB_playItem_t *it) {
    deadbeef->pl_lock ();
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!fp) {
        return -1;
    }

    if (fp->vfs->is_streaming ()) {
        deadbeef->fclose (fp);
        return -1;
    }

    int res = mp4_read_metadata_file(it, fp);
    deadbeef->fclose (fp);

    return res;
}

