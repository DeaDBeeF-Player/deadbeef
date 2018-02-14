/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2017 Alexey Yakovenko and other contributors

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
#include <limits.h>
#include <assert.h>
#include "mp4tagutil.h"
#include "mp4parser.h"

#ifndef __linux__
#define off64_t off_t
#define lseek64 lseek
#define O_LARGEFILE 0
#endif

extern DB_functions_t *deadbeef;

#define COPYRIGHT_SYM "\xa9"

static const char *_mp4_atom_map[] = {
    COPYRIGHT_SYM "alb", "album",
    COPYRIGHT_SYM "art", "artist",
    "aART", "band",
    COPYRIGHT_SYM "cmt", "comment",
    COPYRIGHT_SYM "day", "year",
    COPYRIGHT_SYM "nam", "title",
    COPYRIGHT_SYM "gen", "genre",
    "gnre", "genre",
    "trkn", "track",
    "disk", "disc",
    COPYRIGHT_SYM "wrt", "composer",
    COPYRIGHT_SYM "too", "encoder",
    "tmpo", "bpm",
    "cprt", "copyright",
    COPYRIGHT_SYM "grp", "grouping",
    "cpil", "compilation",
    "pcst", "podcast",
    "catg", "category",
    "keyw", "keyword",
    "desc", "description",
    COPYRIGHT_SYM "lyr", "lyrics",
    "purd", "purchase date",
    "MusicBrainz Track Id", "musicbrainz_trackid",

    NULL, NULL
};

/* For writing:
 * Load/get the existing udta atom
 * If present:
 *   Find ilst
 *   Remove all known non-custom fields, keep the rest
 *   Remove all custom fields
 * If not present:
 *   Create new udta/meta/ilst
 * Re-append all new non-custom fields
 * Re-append all new custom fields
 * Generate data block
 * If the new udta block can fit over the old one, with at least 8 bytes extra for the "free" atom:
 *   Overwrite the old block
 *   Pad with "free" atom if necessary
 * If can't fit: the entire moov atom has to be relocated!
 *   Rename the existing moov into "free"
 *   Append the modified moov block to the end of file, after the last atom
 *   IMPORTANT: the entirety of moov atom with all sub atoms needs to be loaded and saved
 * Further work:
 *   Find if there are "free" blocks between ftyp and mdat, and try to fit the moov there; If that works, truncate the file.
 */

static void
_remove_known_fields (mp4p_atom_t *ilst) {
    mp4p_atom_t *meta_atom = ilst->subatoms;
    while (meta_atom) {
        mp4p_atom_t *next = meta_atom->next;
        mp4p_meta_t *meta = meta_atom->data;

        for (int i = 0; _mp4_atom_map[i]; i++) {
            char type[5];
            memcpy (type, meta_atom->type, 4);
            type[4] = 0;
            if (meta->name || !strcasecmp(type, _mp4_atom_map[i])) {
                mp4p_atom_remove_subatom (ilst, meta_atom);
                break;
            }
        }
        meta_atom = next;
    }
}

// FIXME: much of this code should be moved to mp4parser lib when finalized
mp4p_atom_t *
mp4tagutil_modify_meta (mp4p_atom_t *mp4file, DB_playItem_t *it) {
    mp4p_atom_t *moov = mp4p_atom_find(mp4file, "moov");
    mp4p_atom_t *padding = NULL;
    mp4p_atom_t *mdat = NULL;
    if (!moov || !moov->next) {
        return NULL;
    }

    // only [moov, free, mdat] or [moov, mdat] are supported
    if (!mp4p_atom_type_compare(moov->next, "free")) {
        padding = moov->next;
        if (!padding->next) {
            return NULL;
        }
        if (mp4p_atom_type_compare(padding->next, "mdat")) {
            return NULL;
        }
        mdat = padding->next;
    }
    else if (!mp4p_atom_type_compare(moov->next, "mdat")) {
        mdat = moov->next;
    }
    else {
        return NULL;
    }

    mp4p_atom_t *orig = mp4file;
    mp4file = mp4p_atom_clone (orig);

    mp4p_atom_t *hdlr = NULL;
    mp4p_atom_t *meta = NULL;
    mp4p_atom_t *ilst = NULL;

    // find an existing udta with \0\0\0\0 mdir appl handler
    mp4p_atom_t *udta = mp4file->subatoms;
    while (udta) {
        // there can be multiple meta atoms
        mp4p_atom_t *subatom = udta->subatoms;
        while (subatom) {
            // check each meta subatom
            if (mp4p_atom_type_compare (udta, "meta")) {
                continue;
            }
            hdlr = mp4p_atom_find(subatom, "meta/hdlr");
            if (hdlr) {
                mp4p_hdlr_t *hdlr_data = hdlr->data;
                if (mp4p_fourcc_compare (hdlr_data->component_subtype, "mdir")
                    && mp4p_fourcc_compare(hdlr_data->component_manufacturer, "appl")) {
                    ilst = mp4p_atom_find(subatom, "meta/ilst");
                    meta = subatom;
                    break;
                }
            }
            meta = hdlr = ilst = NULL;
            subatom = subatom->next;
        }
        udta = udta->next;
    }

    // FIXME: atoms following udta is unsupported yet
    if (udta && udta->next) {
        mp4p_atom_free_list (mp4file);
        return NULL;
    }

    if (!udta) {
        // udta not found at all -- append to moov, it needs to be the last one.
        udta = mp4p_atom_append (mp4file, mp4p_atom_new ("udta"));
    }

    if (!meta) {
        // append meta/hdlr/ilst if needed
        meta = mp4p_atom_append (udta, mp4p_atom_new ("meta"));
        hdlr = mp4p_atom_append (meta, mp4p_atom_new ("hdlr"));
        mp4p_hdlr_init (hdlr, "\0\0\0\0", "mdir", "appl");
        ilst = mp4p_atom_append(meta, mp4p_atom_new ("ilst"));
    }
    else {
        // cleanup the pre-existing keyvalue list
        _remove_known_fields (ilst);
    }

    deadbeef->pl_lock ();
    DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (it);
    while (m) {
        if (strchr (":!_", m->key[0])) {
            break;
        }

        if (!strcasecmp (m->key, "track")
            || !strcasecmp (m->key, "numtracks")
            || !strcasecmp (m->key, "disc")
            || !strcasecmp (m->key, "numdiscs")
            || !strcasecmp (m->key, "genre")) {
            m = m->next;
            continue;
        }

        int i;
        for (i = 0; _mp4_atom_map[i]; i += 2) {
            if (!strcasecmp (_mp4_atom_map[i+1], m->key)) {
                break;
            }
        }

        const char *value = m->value;
        const char *end = m->value + m->valuesize;
        while (value < end) {
            if (!_mp4_atom_map[i] || strlen (_mp4_atom_map[i]) != 4) {
                mp4p_ilst_append_custom(ilst, _mp4_atom_map[i] ? _mp4_atom_map[i] : m->key, value);
            }
            else {
                mp4p_ilst_append_text(ilst, _mp4_atom_map[i], value);
            }
            size_t l = strlen (value) + 1;
            value += l;
        }
        m = m->next;
    }

    const char *genre = deadbeef->pl_find_meta (it, "genre");
    if (genre) {
        mp4p_ilst_append_genre (ilst, genre);
    }
    const char *track = deadbeef->pl_find_meta (it, "track");
    const char *numtracks = deadbeef->pl_find_meta (it, "numtracks");
    const char *disc = deadbeef->pl_find_meta (it, "disc");
    const char *numdiscs = deadbeef->pl_find_meta (it, "numdiscs");

    uint16_t itrack = 0, inumtracks = 0, idisc = 0, inumdiscs = 0;
    if (track) {
        itrack = atoi (track);
    }
    if (numtracks) {
        inumtracks = atoi (numtracks);
    }
    if (disc) {
        idisc = atoi (disc);
    }
    if (numdiscs) {
        inumdiscs = atoi (numdiscs);
    }
    if (itrack || inumtracks) {
        mp4p_ilst_append_track_disc (ilst, "trck", itrack, inumtracks);
    }
    if (idisc || inumdiscs) {
        mp4p_ilst_append_track_disc (ilst, "disk", itrack, inumtracks);
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
            mp4p_ilst_append_custom(ilst, tag_rg_names[n], s);
        }
    }
    
    deadbeef->pl_unlock ();

    mp4p_atom_t *moov_new = mp4p_atom_find(mp4file, "moov");

    if (padding) {
        // remove padding in the modified version
        mp4p_atom_t *padding_new = moov_new->next;
        moov_new->next = padding_new->next;
        mp4p_atom_free (padding_new);
    }

    mp4p_atom_t *mdat_new = moov_new->next;

    mp4p_atom_calculate_size(moov_new);

    // at this point, we got the mp4file with new tags, without padding
    mp4p_rebuild_positions (mp4file, mp4file->pos);

    // get the distance between new and old mdat
    int64_t offs = mdat->pos - mdat_new->pos;

    // xxxxxxxxxxxxxxx|  <-- original file mdat pos
    // yyyyyy|--offs--   <-- new file mdat pos, and offs

    // enough space for padding atom + headroom?
    if (offs < 8) {
        // nope, add a new one
        offs = 1024;
    }

    // padding block is always inserted
    mp4p_atom_t *padding_new = mp4p_atom_new ("free");
    assert (offs >= 0);
    padding_new->size = (uint32_t)offs;
    padding_new->next = mdat_new;
    moov_new->next = padding_new;

    // rebuild positions with padding
    mp4p_rebuild_positions (mp4file, mp4file->pos);

    return mp4file;
}

int
mp4_write_metadata (DB_playItem_t *it) {
    char fname[PATH_MAX];
    deadbeef->pl_get_meta (it, ":URI", fname, sizeof (fname));

    mp4p_file_callbacks_t *file = mp4p_open_file_readwrite (fname);

    if (!file) {
        return -1;
    }

    mp4p_atom_t *mp4file = mp4p_open(file);

#if 0 // skipping junk is unsupported yet with file callbacks (junklib can't read using this interface -- a VFS wrapper needed?
    int junk = deadbeef->junk_get_leading_size (fp);
    if (junk >= 0) {
        deadbeef->fseek (fp, junk, SEEK_SET);
    }
    else {
        junk = 0;
    }
#endif

    if (!mp4file) {
        mp4p_file_close(file);
        return -1;
    }

    mp4p_atom_t *mp4file_updated = mp4tagutil_modify_meta(mp4file, it);

    int res = mp4p_update_metadata (file, mp4file, mp4file_updated);

    mp4p_file_close(file);

    mp4p_atom_free_list(mp4file);
    mp4p_atom_free_list(mp4file_updated);

    return res;
}

static void
mp4_load_tags (mp4p_atom_t *mp4file, DB_playItem_t *it) {
    int got_itunes_tags = 0;

    mp4p_atom_t *ilst_atom = mp4p_atom_find (mp4file, "moov/udta/meta/ilst");

    // FIXME: hdlr check missing:
//    if (mp4p_fourcc_compare (hdlr_data->component_subtype, "mdir")
//        && mp4p_fourcc_compare(hdlr_data->component_manufacturer, "appl")) {


    mp4p_atom_t *meta_atom = ilst_atom->subatoms;

    while (meta_atom) {
        got_itunes_tags = 1;

        mp4p_meta_t *meta = meta_atom->data;

        char type[5];
        memcpy (type, meta_atom->type, 4);
        type[4] = 0;
        const char *name = meta->name ? meta->name : type;

        if (!strcasecmp (name, "replaygain_track_gain")) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (meta->text));
            continue;
        }
        else if (!strcasecmp (name, "replaygain_album_gain")) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (meta->text));
            continue;
        }
        else if (!strcasecmp (name, "replaygain_track_peak")) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (meta->text));
            continue;
        }
        else if (!strcasecmp (name, "replaygain_album_peak")) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (meta->text));
            continue;
        }

        for (int i = 0; _mp4_atom_map[i]; i += 2) {
            if (!strcasecmp (name, _mp4_atom_map[i])) {
                if (meta->text) {
                    deadbeef->pl_append_meta (it, _mp4_atom_map[i+1], meta->text);
                }
                else if (meta->values) {
                    if (!memcmp (meta_atom->type, "trkn", 4)) {
                        if (meta->data_size >= 6) { // leading + idx + total
                            uint16_t track = meta->values[1];
                            uint16_t total = meta->values[2];
                            char s[10];
                            if (track) {
                                snprintf (s, sizeof (s), "%d", (int)track);
                                deadbeef->pl_replace_meta (it, "track", s);
                            }
                            if (total) {
                                snprintf (s, sizeof (s), "%d", (int)total);
                                deadbeef->pl_replace_meta (it, "numtracks", s);
                            }
                        }
                    }
                    else if (!memcmp (meta_atom->type, "disk", 4)) {
                        if (meta->data_size >= 6) { // leading + idx + total
                            uint16_t track = meta->values[1];
                            uint16_t total = meta->values[2];
                            char s[10];
                            if (track) {
                                snprintf (s, sizeof (s), "%d", (int)track);
                                deadbeef->pl_replace_meta (it, "disc", s);
                            }
                            if (total) {
                                snprintf (s, sizeof (s), "%d", (int)total);
                                deadbeef->pl_replace_meta (it, "numdiscs", s);
                            }
                        }
                    }
                    else if (!strcmp (_mp4_atom_map[i+1], "genre")) {
                        if (meta->values[0]) {
                            const char *genre = mp4p_genre_name_for_index(meta->values[0]);
                            if (genre) {
                                deadbeef->pl_replace_meta (it, _mp4_atom_map[i+1], genre);
                            }
                        }
                    }
                    else {
                        char s[10];
                        snprintf (s, sizeof (s), "%d", (int)meta->values[0]);
                        deadbeef->pl_replace_meta (it, _mp4_atom_map[i+1], s);
                    }
                }
                break;
            }
        }
        meta_atom = meta_atom->next;
    }
    if (got_itunes_tags) {
        uint32_t f = deadbeef->pl_get_item_flags (it);
        f |= DDB_TAG_ITUNES;
        deadbeef->pl_set_item_flags (it, f);
    }
}



int
mp4_read_metadata_file (DB_playItem_t *it, mp4p_file_callbacks_t *cb) {
    mp4p_atom_t *mp4file = mp4p_open (cb);

    deadbeef->pl_delete_all_meta (it);

    // convert
    mp4_load_tags (mp4file, it);
    mp4p_atom_free_list (mp4file);
    return 0;
}

static ssize_t
_file_read (mp4p_file_callbacks_t *stream, void *ptr, size_t size) {
    return deadbeef->fread (ptr, 1, size, (DB_FILE *)stream->ptrhandle);
}

static off_t
_file_seek (mp4p_file_callbacks_t *stream, off_t offset, int whence) {
    return deadbeef->fseek ((DB_FILE *)stream->ptrhandle, offset, whence);
}

static int64_t
_file_tell (mp4p_file_callbacks_t *stream) {
    return deadbeef->ftell ((DB_FILE *)stream->ptrhandle);
}

void
mp4_init_ddb_file_callbacks (mp4p_file_callbacks_t *cb) {
    cb->read = _file_read;
    cb->seek = _file_seek;
    cb->tell = _file_tell;
}

int
mp4_read_metadata (DB_playItem_t *it) {
    char fname[PATH_MAX];
    deadbeef->pl_get_meta (it, ":URI", fname, sizeof (fname));
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return -1;
    }

    if (fp->vfs->is_streaming ()) {
        deadbeef->fclose (fp);
        return -1;
    }

    mp4p_file_callbacks_t cb;
    memset (&cb, 0, sizeof (cb));
    cb.ptrhandle = fp;
    mp4_init_ddb_file_callbacks(&cb);

    int res = mp4_read_metadata_file(it, &cb);
    deadbeef->fclose (fp);

    return res;
}

