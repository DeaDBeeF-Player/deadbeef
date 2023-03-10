/*
 DeaDBeeF -- the music player
 Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "trkproperties_shared.h"
#include <deadbeef/deadbeef.h>
#include "../src/utf8.h"

extern DB_functions_t *deadbeef;

// full metadata
const char *trkproperties_types[] = {
    "artist", "Artist Name",
    "title", "Track Title",
    "album", "Album Title",
    "year", "Date",
    "genre", "Genre",
    "composer", "Composer",
    "album artist", "Album Artist",
    "track", "Track Number",
    "numtracks", "Total Tracks",
    "disc", "Disc Number",
    "numdiscs", "Total Discs",
    "comment", "Comment",
    NULL
};

const char *trkproperties_hc_props[] = {
    ":URI", "Location",
    ":TRACKNUM", "Subtrack Index",
    ":DURATION", "Duration",
    ":TAGS", "Tag Type(s)",
    ":HAS_EMBEDDED_CUESHEET", "Embedded Cuesheet",
    ":DECODER", "Codec",
    NULL
};

int
trkproperties_build_key_list (const char ***pkeys, int props, DB_playItem_t **tracks, int numtracks) {
    int sz = 20;
    const char **keys = malloc (sizeof (const char *) * sz);
    if (!keys) {
        fprintf (stderr, "fatal: out of memory allocating key list\n");
        assert (0);
        return 0;
    }

    int n = 0;

    for (int i = 0; i < numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (tracks[i]);
        while (meta) {
            if (meta->key[0] != '!' && ((props && meta->key[0] == ':') || (!props && meta->key[0] != ':'))) {
                int k = 0;
                for (; k < n; k++) {
                    if (meta->key == keys[k]) {
                        break;
                    }
                }
                if (k == n) {
                    if (n >= sz) {
                        sz *= 2;
                        keys = realloc (keys, sizeof (const char *) * sz);
                        if (!keys) {
                            fprintf (stderr, "fatal: out of memory reallocating key list (%d keys)\n", sz);
                            assert (0);
                        }
                    }
                    keys[n++] = meta->key;
                }
            }
            meta = meta->next;
        }
    }

    *pkeys = keys;
    return n;
}

void
trkproperties_free_track_list (DB_playItem_t ***_tracks, int *_numtracks) {
    DB_playItem_t **tracks = *_tracks;
    int numtracks = *_numtracks;

    if (tracks) {
        for (int i = 0; i < numtracks; i++) {
            deadbeef->pl_item_unref (tracks[i]);
        }
        free (tracks);
    }

    *_tracks = NULL;
    *_numtracks = 0;
}

void
trkproperties_build_track_list_for_ctx (ddb_playlist_t *plt, int ctx, DB_playItem_t ***_tracks, int *_numtracks) {
    DB_playItem_t *playing_track = NULL;
    if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        playing_track = deadbeef->streamer_get_playing_track_safe ();
    }

    deadbeef->pl_lock ();

    int num = 0;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        num = deadbeef->plt_getselcount (plt);
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        num = deadbeef->plt_get_item_count (plt, PL_MAIN);
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        num = 1;
    }
    if (num <= 0) {
        deadbeef->pl_unlock ();
        if (playing_track) {
            deadbeef->pl_item_unref (playing_track);
        }
        return;
    }

    DB_playItem_t **tracks = calloc (num, sizeof (DB_playItem_t *));
    if (!tracks) {
        fprintf (stderr, "trkproperties: failed to alloc %d bytes to store selected tracks\n", (int)(num * sizeof (void *)));
        deadbeef->pl_unlock ();
        if (playing_track) {
            deadbeef->pl_item_unref (playing_track);
        }
        return;
    }

    if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        if (!playing_track) {
            free (tracks);
            tracks = NULL;
            deadbeef->pl_unlock ();
            return;
        }
        deadbeef->pl_item_ref (playing_track);
        tracks[0] = playing_track;
    }
    else {
        int n = 0;
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            if (ctx == DDB_ACTION_CTX_PLAYLIST || deadbeef->pl_is_selected (it)) {
                assert (n < num);
                deadbeef->pl_item_ref (it);
                tracks[n++] = it;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    *_numtracks = num;
    *_tracks = tracks;

    deadbeef->pl_unlock ();

    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }
}

void
trkproperties_reload_tags (DB_playItem_t **tracks, int numtracks) {
    for (int i = 0; i < numtracks; i++) {
        DB_playItem_t *it = tracks[i];
        deadbeef->pl_lock ();
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        deadbeef->pl_unlock ();

        if (match) {
            uint32_t f = deadbeef->pl_get_item_flags (it);
            if (!(f & DDB_IS_SUBTRACK)) {
                f &= ~DDB_TAG_MASK;
                deadbeef->pl_set_item_flags (it, f);
                DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
                for (int j = 0; decoders[j]; j++) {
                    if (!strcmp (decoders[j]->plugin.id, decoder_id)) {
                        if (decoders[j]->read_metadata) {
                            decoders[j]->read_metadata (it);
                        }
                        break;
                    }
                }
            }
        }
    }
}

static int
string_append_multivalue (char *out, int size, DB_metaInfo_t *meta, int *clipped) {
    int initsize = size;
    const char *p = meta->value;
    const char *end = p + meta->valuesize;
    while (p < end) {
        int l = (int)strlen (p) + 1;
        if (l > size) {
            l = size-1;
            *clipped = 1;
            u8_strnbcpy (out, p, (int)l);
            out[l] = 0;
            out += l;
            size -= l;
            break;
        }

        memcpy (out, p, (int)l);

        p += l;
        out += l-1;
        size -= l-1;

        if (p != end) {
            if (size < 3) {
                *clipped = 1;
                break;
            }
            memcpy (out, "; ", 3);
            out += 2;
            size -= 2;
        }
    }
    return initsize - size;
}

int
trkproperties_get_field_value (char *out, int size, const char *key, DB_playItem_t **tracks, int numtracks) {
    int multiple = 0;
    char *out_start = out;
    int clipped = 0;
    *out = 0;
    if (numtracks == 0) {
        return 0;
    }
    deadbeef->pl_lock ();
    const char **prev = malloc (sizeof (const char *) * numtracks);
    memset (prev, 0, sizeof (const char *) * numtracks);
    for (int i = 0; i < numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_meta_for_key (tracks[i], key);
        if (meta && meta->valuesize == 1) {
            meta = NULL;
        }

        if (i > 0) {
            int n = 0;
            for (; n < i; n++) {
                if (prev[n] == (meta ? meta->value : NULL)) {
                    break;
                }
            }
            if (n == i) {
                multiple = 1;
                if (meta) {
                    if (out != out_start) {
                        if (size < 3) {
                            clipped = 1;
                            break;
                        }
                        memcpy (out, "; ", 3);
                        out += 2;
                        size -= 2;
                    }
                    int len = string_append_multivalue (out, size, meta, &clipped);
                    out += len;
                    size -= len;
                }
            }
        }
        else if (meta) {
            int n = string_append_multivalue (out, size, meta, &clipped);
            out += n;
            size -= n;
        }
        prev[i] = meta ? meta->value : NULL;
        if (size < 3) {
            break;
        }
    }
    deadbeef->pl_unlock ();
    if (clipped) {
        // FIXME: This is a hack for strings which don't fit in the preallocated 5K buffer
        // When the code is converted to use dynamic buffer - this can be removed
        int idx = (int)(out - 4 - out_start);
        u8_dec (out_start, &idx);
        char *p = out_start + idx;
        strcpy (p, "...");
    }
    free (prev);
    return multiple;
}
