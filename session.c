/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "session.h"
#include "common.h"
#include "deadbeef.h"
#include "conf.h"

#define SESS_CURRENT_VER 4
// changelog:
// version 5 everything moved to common config
// version 4 column settings moved to common config
// version 3 adds column widths

// NOTE: dont forget to update session_reset when changing that

static const uint8_t sessfile_magic[] = { 0xdb, 0xef, 0x5e, 0x55 }; // dbefsess in hexspeak

void
session_reset (void) {
}

static int
write_i16_be (uint16_t val, FILE *fp) {
    uint8_t b;
    b = (uint8_t)(val&0xff);
    if (fwrite (&b, 1, 1, fp) != 1) {
        return 0;
    }
    b = (uint8_t)((val&0xff00)>>8);
    if (fwrite (&b, 1, 1, fp) != 1) {
        return 1;
    }
    return 2;
}

static int
write_i32_be (uint32_t val, FILE *fp) {
    uint8_t b;
    b = (uint8_t)(val&0xff);
    if (fwrite (&b, 1, 1, fp) != 1) {
        return 0;
    }
    b = (uint8_t)((val&0xff00)>>8);
    if (fwrite (&b, 1, 1, fp) != 1) {
        return 1;
    }
    b = (uint8_t)((val&0xff0000)>>16);
    if (fwrite (&b, 1, 1, fp) != 1) {
        return 2;
    }
    b = (uint8_t)((val&0xff000000)>>24);
    if (fwrite (&b, 1, 1, fp) != 1) {
        return 3;
    }
    return 4;
}

static int
read_i16_be (uint16_t *pval, FILE *fp) {
    uint16_t val = 0;
    uint8_t b;
    if (fread (&b, 1, 1, fp) != 1) {
        return 0;
    }
    val |= (uint16_t)b;
    if (fread (&b, 1, 1, fp) != 1) {
        return 1;
    }
    val |= (((uint16_t)b) << 8);
    *pval = val;
    return 2;
}

static int
read_i32_be (uint32_t *pval, FILE *fp) {
    uint32_t val = 0;
    uint8_t b;
    if (fread (&b, 1, 1, fp) != 1) {
        return 0;
    }
    val |= (uint32_t)b;
    if (fread (&b, 1, 1, fp) != 1) {
        return 1;
    }
    val |= (((uint32_t)b) << 8);
    if (fread (&b, 1, 1, fp) != 1) {
        return 2;
    }
    val |= (((uint32_t)b) << 16);
    if (fread (&b, 1, 1, fp) != 1) {
        return 3;
    }
    val |= (((uint32_t)b) << 24);
    *pval = val;
    return 4;
}

int
session_save (const char *fname) {
    unlink (fname);
#if 0
    FILE *fp = fopen (fname, "w+b");
    if (!fp) {
        fprintf (stderr, "failed to save session, file %s could not be opened\n");
        return -1;
    }
    // magic
    if (fwrite (sessfile_magic, 1, 4, fp) != 4) {
        goto session_save_fail;
    }
    uint8_t version = SESS_CURRENT_VER;
    if (fwrite (&version, 1, 1, fp) != 1) {
        goto session_save_fail;
    }
    uint16_t l = strlen (session_dir);
    if (write_i16_be (l, fp) != 2) {
        goto session_save_fail;
    }
    if (fwrite (session_dir, 1, l, fp) != l) {
        goto session_save_fail;
    }
    if (write_i32_be (*((uint32_t*)&session_volume), fp) != 4) {
        goto session_save_fail;
    }
    if (fwrite (&session_playlist_order, 1, 1, fp) != 1) {
        goto session_save_fail;
    }
    if (fwrite (&session_playlist_looping, 1, 1, fp) != 1) {
        goto session_save_fail;
    }
    if (fwrite (&session_scroll_follows_playback, 1, 1, fp) != 1) {
        goto session_save_fail;
    }
    for (int k = 0; k < 5; k++) {
        if (write_i32_be (session_win_attrs[k], fp) != 4) {
            goto session_save_fail;
        }
    }
    fclose (fp);
    return 0;
session_save_fail:
    fprintf (stderr, "failed to save session, seems to be a disk error\n");
    fclose (fp);
    return -1;
#endif
}

int
session_load (const char *fname) {
    char session_dir[2048];
    float session_volume;
    int8_t session_playlist_order;
    int8_t session_playlist_looping;
    int8_t session_scroll_follows_playback = 1;
    int session_win_attrs[5] = { 40, 40, 500, 300, 0 };

    session_volume = 0;
    session_dir[0] = 0;
    session_playlist_looping = 0;
    session_playlist_order = 0;
    session_scroll_follows_playback = 1;
    session_win_attrs[0] = 40;
    session_win_attrs[1] = 40;
    session_win_attrs[2] = 500;
    session_win_attrs[3] = 300;
    session_win_attrs[4] = 0;

    FILE *fp = fopen (fname, "r+b");
    if (!fp) {
        return -1;
    }
    // magic
    uint8_t magic[4];
    if (fread (magic, 1, 4, fp) != 4) {
        goto session_load_fail;
    }
    if (memcmp (magic, sessfile_magic, 4)) {
        goto session_load_fail;
    }
    uint8_t version;
    if (fread (&version, 1, 1, fp) != 1) {
        goto session_load_fail;
    }
    if (version < 2 || version > SESS_CURRENT_VER) {
        goto session_load_fail;
    }
    uint16_t l;
    if (read_i16_be (&l, fp) != 2) {
        goto session_load_fail;
    }
    if (l >= 2048) {
        goto session_load_fail;
    }
    if (fread (session_dir, 1, l, fp) != l) {
        goto session_load_fail;
    }
    session_dir[l] = 0;
    if (read_i32_be ((uint32_t*)&session_volume, fp) != 4) {
        goto session_load_fail;
    }
    if (fread (&session_playlist_order, 1, 1, fp) != 1) {
        goto session_load_fail;
    }
    if (session_playlist_order < 0 || session_playlist_order > 2) {
        goto session_load_fail;
    }
    if (fread (&session_playlist_looping, 1, 1, fp) != 1) {
        goto session_load_fail;
    }
    if (session_playlist_looping < 0 || session_playlist_looping > 2) {
        goto session_load_fail;
    }
    if (fread (&session_scroll_follows_playback, 1, 1, fp) != 1) {
        goto session_load_fail;
    }
    for (int k = 0; k < 5; k++) {
        if (read_i32_be (&session_win_attrs[k], fp) != 4) {
            goto session_load_fail;
        }
    }
    if (version == 3) {
        // import playlist and search columns to new common config
#define PL_MAX_COLUMNS 5
        int session_main_colwidths[PL_MAX_COLUMNS] = { 50, 150, 50, 150, 50 };
        int session_main_numcols = 5;
        int session_search_colwidths[PL_MAX_COLUMNS] = { 0, 150, 50, 150, 50 };;
        int session_search_numcols = 5;
        {
            // main column widths
            uint8_t l;
            if (fread (&l, 1, 1, fp) != 1) {
                goto session_load_fail;
            }
            if (l > PL_MAX_COLUMNS) {
                goto session_load_fail;
            }
            session_main_numcols = l;
            for (int i = 0; i < l; i++) {
                int16_t w;
                if (fread (&w, 1, sizeof (w), fp) != sizeof (w)) {
                    goto session_load_fail;
                }
                session_main_colwidths[i] = w;
            }
        }
        {
            // search column widths
            uint8_t l;
            if (fread (&l, 1, 1, fp) != 1) {
                goto session_load_fail;
            }
            if (l > PL_MAX_COLUMNS) {
                goto session_load_fail;
            }
            session_search_numcols = l;
            for (int i = 0; i < l; i++) {
                int16_t w;
                if (fread (&w, 1, sizeof (w), fp) != sizeof (w)) {
                    goto session_load_fail;
                }
                session_search_colwidths[i] = w;
            }
        }
        // convert to common config
        const char *colnames[] = {
            "Playing",
            "Artist / Album",
            "Track №",
            "Title / Track Artist",
            "Duration"
        };
        int colids[] = {
            DB_COLUMN_PLAYING,
            DB_COLUMN_ARTIST_ALBUM,
            DB_COLUMN_TRACK,
            DB_COLUMN_TITLE,
            DB_COLUMN_DURATION
        };
        int i;
        for (i = 0; i < 5; i++) {
            char key[128];
            char value[128];
            snprintf (key, sizeof (key), "playlist.column.%d", i);
            snprintf (value, sizeof (value), "\"%s\" \"%s\" %d %d %d", colnames[i], "", colids[i], session_main_colwidths[i], i == 2 ? 1 : 0);
            conf_set_str (key, value);
        }
        for (i = 1; i < 5; i++) {
            char key[128];
            char value[128];
            snprintf (key, sizeof (key), "search.column.%d", i-1);
            snprintf (value, sizeof (value), "\"%s\" \"%s\" %d %d %d", colnames[i], "", colids[i], session_main_colwidths[i], i == 2 ? 1 : 0);
            conf_set_str (key, value);
        }
    }
//    printf ("dir: %s\n", session_dir);
//    printf ("volume: %f\n", session_volume);
//    printf ("win: %d %d %d %d %d\n", session_win_attrs[0], session_win_attrs[1], session_win_attrs[2], session_win_attrs[3], session_win_attrs[4]);
    fclose (fp);

    if (version <= 4) {
        // move everything to common config
        conf_set_str ("filechooser.lastdir", session_dir);
        conf_set_int ("playback.loop", session_playlist_looping);
        conf_set_int ("playback.order", session_playlist_order);
        conf_set_float ("playback.volume", session_volume);
        conf_set_int ("playlist.scroll.followplayback", session_scroll_follows_playback);
        conf_set_int ("mainwin.geometry.x", session_win_attrs[0]);
        conf_set_int ("mainwin.geometry.y", session_win_attrs[1]);
        conf_set_int ("mainwin.geometry.w", session_win_attrs[2]);
        conf_set_int ("mainwin.geometry.h", session_win_attrs[3]);
    }

    return 0;
session_load_fail:
    fprintf (stderr, "failed to load session, session file is corrupt\n");
    fclose (fp);
//    session_reset ();
    return -1;
}
