/*
    CUE playlist plugin for DeaDBeeF Player
    Copyright (C) 2017 github:wdlkmpx
    Copyright (C) 2017 Alexey Yakovenko

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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#include "../../deadbeef.h"
#include "../../shared/cueutil.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

DB_functions_t *deadbeef;

void
plt_process_cue_track2 (DB_playItem_t *it, const char *fname, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], int extra_tag_index) {
    DB_playItem_t *prev = it;
    if (!cuefields[CUE_FIELD_TRACK][0]) {
        trace ("pl_process_cue_track: invalid track (file=%s, title=%s)\n", fname, cuefields[CUE_FIELD_TITLE]);
        return;
    }
    if (!cuefields[CUE_FIELD_INDEX00][0] && !cuefields[CUE_FIELD_INDEX01][0]) {
        trace ("pl_process_cue_track: invalid index (file=%s, title=%s, track=%s)\n", fname, cuefields[CUE_FIELD_TITLE], cuefields[CUE_FIELD_TRACK]);
        //return;
    }
#if SKIP_BLANK_CUE_TRACKS
    if (!cuefields[CUE_FIELD_TITLE][0]) {
        trace ("pl_process_cue_track: invalid title (file=%s, title=%s, track=%s)\n", fname, cuefields[CUE_FIELD_TITLE], cuefields[CUE_FIELD_TRACK]);
        return;
    }
#endif
    // fix track number
    char *p = cuefields[CUE_FIELD_TRACK];
    while (*p && isdigit (*p)) {
        p++;
    }
    *p = 0;

    if (!cuefields[CUE_FIELD_INDEX01][0]) {
        trace ("pl_process_cue_track: invalid index01 (pregap=%s, index01=%s)\n", cuefields[CUE_FIELD_PREGAP], cuefields[CUE_FIELD_INDEX01]);
    }

    uint32_t iflags = deadbeef->pl_get_item_flags(it);
    iflags |= DDB_TAG_CUESHEET;
    deadbeef->pl_set_item_flags(it, iflags);

    pl_cue_set_track_field_values(it, cuefields, extra_tags, extra_tag_index);
}


static DB_playItem_t *
load_cue (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    const char *slash = strrchr (fname, '/');
    char cue_file_dir[strlen(fname) + 10];
    memset(cue_file_dir, 0, sizeof(cue_file_dir));
    if (slash && slash > fname) {
        strncpy(cue_file_dir, fname, slash - fname);
    }
    trace ("enter pl_insert_cue: %s\n", fname);
    trace ("cue directory: %s \n", cue_file_dir);
    // skip all empty lines and comments
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }

    int sz = deadbeef->fgetlength (fp);
    trace ("loading cue...\n");
    uint8_t *membuffer = malloc (sz);
    if (!membuffer) {
        deadbeef->fclose (fp);
        trace ("failed to allocate %d bytes to read the file %s\n", sz, fname);
        free (membuffer);
        return NULL;
    }
    uint8_t *buffer = membuffer;
    deadbeef->fread (buffer, 1, sz, fp);
    deadbeef->fclose (fp);

    if (sz >= 3 && buffer[0] == 0xef && buffer[1] == 0xbb && buffer[2] == 0xbf) {
        buffer += 3;
        sz -= 3;
    }

    const char *charset = deadbeef->junk_detect_charset (buffer);

    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;

    // determine total tracks/files
    int ncuetracks, ncuefiles;
    pl_cue_get_total_tracks_and_files(p, end, &ncuefiles, &ncuetracks);
    if (!ncuefiles || !ncuetracks) {
        deadbeef->log("invalid cuesheet (%s)\n", fname);
        return NULL;
    }

    if (ncuefiles == 1) {
        deadbeef->pl_lock();
        // Only 1 file, get the filename 
        // - set the internal deadbeef cue_file variable
        // - insert audio_file to playlist
        // - expect decoders to call deadbeef->plt_insert_cue_int(...), or plt_process_cue
        // - unset cue_file
        char audio_file[255] = "";
        p = buffer;
        while (p < end) {
            p = skipspaces (p, end);
            if (p >= end) {
                break;
            }
            if (!strncasecmp (p, "FILE ", 5)) {
                DB_playItem_t *it = NULL;
                pl_get_qvalue_from_cue (p + 5, sizeof (audio_file), audio_file, charset);
                deadbeef->plt_set_cue_file(plt, fname);
                if (audio_file[0] == '/') { //full path
                    trace ("pl_insert_cue: adding file %s\n", audio_file);
                    it = deadbeef->plt_insert_file2 (0, plt, after, audio_file, pabort, cb, user_data);
                }
                else {
                    int l = strlen(cue_file_dir) + strlen (audio_file) + 10;
                    char fullpath[l];
                    trace ("pl_insert_cue: adding file %s\n", fullpath);
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", cue_file_dir, audio_file);
                    it = deadbeef->plt_insert_file2 (0, plt, after, fullpath, pabort, cb, user_data);
                }
                deadbeef->plt_set_cue_file(plt, NULL);
                free(membuffer);
                deadbeef->pl_unlock();
                return it;
            }
            // move pointer to the next line
            while (p < end && *p >= 0x20) {
               p++;
            }
        }
        free(membuffer);
        deadbeef->pl_unlock();
        return NULL;
    } /* end of (ncuefiles == 1) */

    if (ncuefiles > 1 && ncuefiles != ncuetracks) {
        deadbeef->log("%s: the number of TRACKS must be equal to the number of FILES..\n", fname);
        return NULL;
    }

    char cuefields[CUE_MAX_FIELDS][255];
    memset(cuefields, 0, sizeof(cuefields));
    snprintf(cuefields[CUE_FIELD_TOTALTRACKS], sizeof(cuefields[CUE_FIELD_TOTALTRACKS]), "%d", ncuetracks);

    char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255];
    memset (extra_tags, 0, sizeof (extra_tags));
    int extra_tag_index = 0;

    int have_track = 0;

    DB_playItem_t *prev = NULL;
    DB_playItem_t *it = NULL;

    p = buffer;

    deadbeef->pl_lock();
    // __ignore = deadbeef ignores external and internal cue files
    deadbeef->plt_set_cue_file(plt, "__ignore");

    while (p < end) {
        p = skipspaces (p, end);
        if (p >= end) {
            break;
        }

        int field = pl_cue_get_field_value(p, cuefields, extra_tags, charset, have_track, &extra_tag_index);

        if (field == CUE_FIELD_FILE) {
            if (cuefields[CUE_FIELD_FILE][0] == '/') { //full path
                trace ("pl_insert_cue: adding file %s\n", cuefields[CUE_FIELD_FILE]);
                it = deadbeef->plt_insert_file2 (0, plt, after, cuefields[CUE_FIELD_FILE], pabort, cb, user_data);
            }
            else {
                int l = strlen(cue_file_dir) + strlen (cuefields[CUE_FIELD_FILE]) + 10;
                char fullpath[l];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", cue_file_dir, cuefields[CUE_FIELD_FILE]);
                trace ("pl_insert_cue: adding file %s\n", fullpath);
                it = deadbeef->plt_insert_file2 (0, plt, after, fullpath, pabort, cb, user_data);
            }
            if (it) {
                if (prev) {
                    // add previous track
                    plt_process_cue_track2 (prev, fname, cuefields, extra_tags, extra_tag_index);
                }
                after = it;
                prev = it;
            }
            pl_cue_reset_per_track_fields(cuefields);
        }
        else if (field == CUE_FIELD_TRACK) {
            have_track = 1;
            pl_get_value_from_cue (p + 6, sizeof (cuefields[CUE_FIELD_TRACK]), cuefields[CUE_FIELD_TRACK]);
        }

        if (pabort && *pabort) {
            deadbeef->plt_set_cue_file(plt, NULL);
            deadbeef->pl_unlock();
            free (membuffer);
            return after;
        }
        // move pointer to the next line
        while (p < end && *p >= 0x20) {
           p++;
        }
    } /* end of while loop */

    if (it) {
        // handle last track
        plt_process_cue_track2 (it, fname, cuefields, extra_tags, extra_tag_index);
    }

    deadbeef->plt_set_cue_file(plt, NULL);
    deadbeef->pl_unlock();
    trace ("leave pl_insert_cue\n");
    free (membuffer);
    return after;

}

static DB_playItem_t *
cueplug_load (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    char resolved_fname[PATH_MAX];
    char *res = realpath (fname, resolved_fname);
    if (res) {
        fname = resolved_fname;
    }
    return load_cue (plt, after, fname, pabort, cb, user_data);
}

int
cueplug_save (ddb_playlist_t *plt, const char *fname, DB_playItem_t *first, DB_playItem_t *last) {
    FILE *fp = fopen (fname, "w+t");
    if (!fp) {
        return -1;
    }

    int tcount = 1;

    DB_playItem_t *it = first;
    deadbeef->pl_item_ref (it);
    while (it) {
        uint32_t flags = deadbeef->pl_get_item_flags (it);
        if (flags & DDB_IS_SUBTRACK) {
            // skip subtracks
            if (deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0)) {
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
                continue;
            }
        }

        deadbeef->pl_lock ();
        {
            const char *fname = deadbeef->pl_find_meta (it, ":URI");
            const char *artist = deadbeef->pl_find_meta (it, "artist");
            const char *title = deadbeef->pl_find_meta (it, "title");
            const char *composer = deadbeef->pl_find_meta (it, "composer");
            if (!composer) {
                composer = deadbeef->pl_find_meta (it, "SONGWRITER");
            }
            fprintf (fp, "FILE \"%s\"\n", fname);
            fprintf (fp, "  TRACK %02d AUDIO\n", tcount++);
            if (title) {
                fprintf (fp, "    TITLE \"%s\"\n", title);
            }
            if (artist) {
                fprintf (fp, "    PERFORMER \"%s\"\n", artist);
            }
            if (composer) {
                fprintf (fp, "    SONGWRITER \"%s\"\n", composer);
            }
            fprintf (fp, "    INDEX 01 00:00:00\n");
        }
        deadbeef->pl_unlock ();

        if (it == last) {
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    fclose (fp);

    return 0;
}

static const char * exts[] = { "cue", NULL, };
DB_playlist_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_PLAYLIST,
    .plugin.id = "cue",
    .plugin.name = "CUE playlist support",
    .plugin.descr = "",
    .plugin.copyright = 
        "CUE playlist plugin for DeaDBeeF Player\n"
        "Copyright (C) 2017 Alexey Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .load = cueplug_load,
//    .save = cueplug_save,
    .extensions = exts,
};

DB_plugin_t *
cue_load (DB_functions_t *api) {
    deadbeef = api;
    return &plugin.plugin;
}
