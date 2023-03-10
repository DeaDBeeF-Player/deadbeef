/*
    Converter for DeaDBeeF Player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <deadbeef/deadbeef.h>
#include "converter.h"
#include <deadbeef/strdupa.h>
#include "../../shared/mp4tagutil.h"

static ddb_converter_t plugin;

#ifndef __linux__
#define O_LARGEFILE 0
#endif

#define min(x,y) ((x)<(y)?(x):(y))

#define trace(...) { deadbeef->log_detailed (&plugin.misc.plugin, 0, __VA_ARGS__); }
#define trace_err(...) { deadbeef->log_detailed (&plugin.misc.plugin, DDB_LOG_LAYER_DEFAULT, __VA_ARGS__); }

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

static ddb_converter_t plugin;
DB_functions_t *deadbeef;

static ddb_encoder_preset_t *encoder_presets;
static ddb_dsp_preset_t *dsp_presets;

ddb_encoder_preset_t *
encoder_preset_alloc (void) {
    ddb_encoder_preset_t *p = malloc (sizeof (ddb_encoder_preset_t));
    if (!p) {
        fprintf (stderr, "failed to alloc ddb_encoder_preset_t\n");
        return NULL;
    }
    memset (p, 0, sizeof (ddb_encoder_preset_t));
    return p;
}

void
encoder_preset_free (ddb_encoder_preset_t *p) {
    if (p) {
        if (p->title) {
            free (p->title);
        }
        if (p->ext) {
            free (p->ext);
        }
        if (p->encoder) {
            free (p->encoder);
        }
        free (p);
    }
}

ddb_encoder_preset_t *
encoder_preset_load (const char *fname) {
    int err = 1;
    FILE *fp = fopen (fname, "rt");
    if (!fp) {
        return NULL;
    }
    ddb_encoder_preset_t *p = encoder_preset_alloc ();

    char str[1024];
    while (fgets (str, sizeof (str), fp)) {
        // chomp
        char *cr = str + strlen (str) - 1;
        while (*cr == '\n') {
            cr--;
        }
        cr++;
        *cr = 0;

        char *sp = strchr (str, ' ');
        if (!sp) {
            continue;
        }

        *sp = 0;
        char *item = sp + 1;

        if (!strcmp (str, "title")) {
            p->title = strdup (item);
        }
        else if (!strcmp (str, "ext")) {
            p->ext = strdup (item);
        }
        else if (!strcmp (str, "encoder")) {
            p->encoder = strdup (item);
        }
        else if (!strcmp (str, "method")) {
            p->method = atoi (item);
        }
        else if (!strcmp (str, "id3v2_version")) {
            p->id3v2_version = atoi (item);
        }
        else if (!strcmp (str, "tag_id3v2")) {
            p->tag_id3v2 = atoi (item);
        }
        else if (!strcmp (str, "tag_id3v1")) {
            p->tag_id3v1 = atoi (item);
        }
        else if (!strcmp (str, "tag_apev2")) {
            p->tag_apev2 = atoi (item);
        }
        else if (!strcmp (str, "tag_flac")) {
            p->tag_flac = atoi (item);
        }
        else if (!strcmp (str, "tag_oggvorbis")) {
            p->tag_oggvorbis = atoi (item);
        }
        else if (!strcmp (str, "tag_mp4")) {
            p->tag_mp4 = atoi (item);
        }
    }

    if (!p->title) {
        p->title = strdup ("Untitled");
    }
    if (!p->ext) {
        p->ext = strdup ("");
    }
    if (!p->encoder) {
        p->encoder = strdup ("");
    }

    err = 0;

    if (err) {
        encoder_preset_free (p);
        p = NULL;
    }
    if (fp) {
        fclose (fp);
    }
    return p;
}

// @return -1 on path/write error, -2 if file already exists
int
encoder_preset_save (ddb_encoder_preset_t *p, int overwrite) {
    if (!p->title || !p->title[0]) {
        trace ("encoder_preset_save: empty title\n");
        return -1;
    }
    const char *confdir = deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG);
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/presets", confdir) < 0) {
        return -1;
    }
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/encoders", confdir) < 0) {
        return -1;
    }
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", confdir, p->title) < 0) {
        return -1;
    }

    if (!overwrite) {
        FILE *fp = fopen (path, "rb");
        if (fp) {
            fclose (fp);
            return -2;
        }
    }

    FILE *fp = fopen (path, "w+b");
    if (!fp) {
        return -1;
    }

    fprintf (fp, "title %s\n", p->title);
    fprintf (fp, "ext %s\n", p->ext);
    fprintf (fp, "encoder %s\n", p->encoder);
    fprintf (fp, "method %d\n", p->method);
    fprintf (fp, "id3v2_version %d\n", p->id3v2_version);
    fprintf (fp, "tag_id3v2 %d\n", p->tag_id3v2);
    fprintf (fp, "tag_id3v1 %d\n", p->tag_id3v1);
    fprintf (fp, "tag_apev2 %d\n", p->tag_apev2);
    fprintf (fp, "tag_flac %d\n", p->tag_flac);
    fprintf (fp, "tag_oggvorbis %d\n", p->tag_oggvorbis);
    fprintf (fp, "tag_mp4 %d\n", p->tag_mp4);

    fclose (fp);
    return 0;
}

void
encoder_preset_copy (ddb_encoder_preset_t *to, ddb_encoder_preset_t *from) {
    to->title = strdup (from->title);
    to->ext = strdup (from->ext);
    to->encoder = strdup (from->encoder);
    to->method = from->method;
    to->tag_id3v2 = from->tag_id3v2;
    to->tag_id3v1 = from->tag_id3v1;
    to->tag_apev2 = from->tag_apev2;
    to->tag_flac = from->tag_flac;
    to->tag_oggvorbis = from->tag_oggvorbis;
    to->tag_mp4 = from->tag_mp4;
    to->tag_mp3xing = from->tag_mp3xing;
    to->id3v2_version = from->id3v2_version;
}

ddb_encoder_preset_t *
encoder_preset_get_list (void) {
    return encoder_presets;
}

ddb_encoder_preset_t *
encoder_preset_get_for_idx (int idx) {
    ddb_encoder_preset_t *p = encoder_presets;
    while (p && idx--) {
        p = p->next;
    }
    return p;
}

void
encoder_preset_append (ddb_encoder_preset_t *p) {
    // append
    ddb_encoder_preset_t *tail = encoder_presets;
    while (tail && tail->next) {
        tail = tail->next;
    }
    if (tail) {
        tail->next = p;
    }
    else {
        encoder_presets = p;
    }
}

void
encoder_preset_remove (ddb_encoder_preset_t *p) {
    ddb_encoder_preset_t *prev = encoder_presets;
    while (prev && prev->next != p) {
        prev = prev->next;
    }
    if (prev) {
        prev->next = p->next;
    }
    else {
        encoder_presets = p->next;
    }
}

void
encoder_preset_replace (ddb_encoder_preset_t *from, ddb_encoder_preset_t *to) {
    ddb_encoder_preset_t *prev = encoder_presets;
    while (prev && prev->next != from) {
        prev = prev->next;
    }
    if (prev) {
        prev->next = to;
    }
    else {
        encoder_presets = to;
    }
    to->next = from->next;
}

ddb_dsp_preset_t *
dsp_preset_alloc (void) {
    ddb_dsp_preset_t *p = malloc (sizeof (ddb_dsp_preset_t));
    if (!p) {
        fprintf (stderr, "failed to alloc ddb_dsp_preset_t\n");
        return NULL;
    }
    memset (p, 0, sizeof (ddb_dsp_preset_t));
    return p;
}

void
dsp_preset_free (ddb_dsp_preset_t *p) {
    if (p) {
        if (p->title) {
            free (p->title);
        }
        deadbeef->dsp_preset_free (p->chain);
        free (p);
    }
}

void
dsp_preset_copy (ddb_dsp_preset_t *to, ddb_dsp_preset_t *from) {
    to->title = strdup (from->title);
    ddb_dsp_context_t *tail = NULL;
    ddb_dsp_context_t *dsp = from->chain;
    while (dsp) {
        ddb_dsp_context_t *i = dsp->plugin->open ();
        if (dsp->plugin->num_params) {
            int n = dsp->plugin->num_params ();
            for (int j = 0; j < n; j++) {
                char s[1000] = "";
                dsp->plugin->get_param (dsp, j, s, sizeof (s));
                i->plugin->set_param (i, j, s);
            }
        }
        if (tail) {
            tail->next = i;
            tail = i;
        }
        else {
            to->chain = tail = i;
        }
        dsp = dsp->next;
    }
}

ddb_dsp_preset_t *
dsp_preset_get_list (void) {
    return dsp_presets;
}

ddb_dsp_preset_t *
dsp_preset_load (const char *fname) {
    ddb_dsp_preset_t *p = dsp_preset_alloc ();
    if (!p) {
        return NULL;
    }
    memset (p, 0, sizeof (ddb_dsp_preset_t));
    const char *end = strrchr (fname, '.');
    if (!end) {
        end = fname + strlen (fname);
    }
    const char *start = strrchr (fname, '/');
    if (!start) {
        start = fname;
    }
    else {
        start++;
    }

    p->title = malloc (end-start+1);
    memcpy (p->title, start, end-start);
    p->title[end-start] = 0;
    int err = deadbeef->dsp_preset_load (fname, &p->chain);
    if (err != 0) {
        dsp_preset_free (p);
        return NULL;
    }
    return p;
}

int
dsp_preset_save (ddb_dsp_preset_t *p, int overwrite) {
    if (!p->title || !p->title[0]) {
        trace ("dsp_preset_save: empty title\n");
        return -1;
    }
    const char *confdir = deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG);
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/presets", confdir) < 0) {
        return -1;
    }
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/dsp", confdir) < 0) {
        return -1;
    }
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/dsp/%s.txt", confdir, p->title) < 0) {
        return -1;
    }

    if (!overwrite) {
        FILE *fp = fopen (path, "rb");
        if (fp) {
            fclose (fp);
            return -2;
        }
    }

    return deadbeef->dsp_preset_save (path, p->chain);
}

static int dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

int
scandir_preset_filter (const struct dirent *ent) {
    char *ext = strrchr (ent->d_name, '.');
    if (ext && !strcasecmp (ext, ".txt")) {
        return 1;
    }
    return 0;
}

int
load_encoder_presets (void) {
    // check if we need to install presets
    char ppath[PATH_MAX];
    char epath[PATH_MAX];
    snprintf (ppath, sizeof (ppath), "%s/presets", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG));
    snprintf (epath, sizeof (epath), "%s/encoders", ppath);

    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/presets/encoders", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG)) < 0) {
        return -1;
    }

    char syspath[PATH_MAX];
    if (snprintf (syspath, sizeof (syspath), "%s/convpresets", deadbeef->get_system_dir (DDB_SYS_DIR_PLUGIN_RESOURCES)) < 0) {
        return -1;
    }

    const char *preset_dirs[] = {
        syspath, path, NULL
    };

    ddb_encoder_preset_t *tail = NULL;

    for (int di = 0; preset_dirs[di]; di++) {
        const char *path = preset_dirs[di];
        struct dirent **namelist = NULL;
        int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
        int i;
        for (i = 0; i < n; i++) {
            char s[PATH_MAX];
            if (snprintf (s, sizeof (s), "%s/%s", path, namelist[i]->d_name) > 0){
                ddb_encoder_preset_t *p = encoder_preset_load (s);
                if (p) {
                    if (path == syspath) {
                        // don't allow editing stock presets
                        p->readonly = 1;
                    }
                    else {
                        // check if the same RO preset exists
                        for (ddb_encoder_preset_t *pr = encoder_presets; pr; pr = pr->next) {
                            if (pr->readonly && !strcmp (pr->title, p->title)) {
                                encoder_preset_free (p);
                                p = NULL;
                                break;
                            }
                        }
                        if (!p) {
                            // NOTE: we don't delete duplicate presets in $HOME
                            // for compat with <=0.6.1
                            encoder_preset_free (p);
                            p = NULL;
                            continue;
                        }
                    }
                    if (tail) {
                        tail->next = p;
                        tail = p;
                    }
                    else {
                        encoder_presets = tail = p;
                    }
                }
            }
        }
        for (i = 0; i < n; i++) {
            free (namelist[i]);
        }
        free (namelist);
        namelist = NULL;
    }
    return 0;
}

void
free_encoder_presets (void) {
    ddb_encoder_preset_t *p = encoder_presets;
    while (p) {
        ddb_encoder_preset_t *next = p->next;
        if (p->title) {
            free (p->title);
        }
        if (p->ext) {
            free (p->ext);
        }
        if (p->encoder) {
            free (p->encoder);
        }
        free (p);
        p = next;
    }
    encoder_presets = NULL;
}

int
load_dsp_presets (void) {
    ddb_dsp_preset_t *tail = NULL;
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/presets/dsp", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG)) < 0) {
        return -1;
    }
    struct dirent **namelist = NULL;
    int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
    int i;
    for (i = 0; i < n; i++) {
        char s[PATH_MAX];
        if (snprintf (s, sizeof (s), "%s/%s", path, namelist[i]->d_name) > 0){
            ddb_dsp_preset_t *p = dsp_preset_load (s);
            if (p) {
                if (tail) {
                    tail->next = p;
                    tail = p;
                }
                else {
                    dsp_presets = tail = p;
                }
            }
        }
        free (namelist[i]);
    }
    free (namelist);
    return 0;
}

void
free_dsp_presets (void) {
    ddb_dsp_preset_t *p = dsp_presets;
    while (p) {
        ddb_dsp_preset_t *next = p->next;
        if (p->title) {
            free (p->title);
        }
        if (p->chain) {
            deadbeef->dsp_preset_free (p->chain);
        }
        free (p);
        p = next;
    }
    dsp_presets = NULL;
}

ddb_dsp_preset_t *
dsp_preset_get_for_idx (int idx) {
    ddb_dsp_preset_t *p = dsp_presets;
    while (p && idx--) {
        p = p->next;
    }
    return p;
}

void
dsp_preset_append (ddb_dsp_preset_t *p) {
    // append
    ddb_dsp_preset_t *tail = dsp_presets;
    while (tail && tail->next) {
        tail = tail->next;
    }
    if (tail) {
        tail->next = p;
    }
    else {
        dsp_presets = p;
    }
}

void
dsp_preset_remove (ddb_dsp_preset_t *p) {
    ddb_dsp_preset_t *prev = dsp_presets;
    while (prev && prev->next != p) {
        prev = prev->next;
    }
    if (prev) {
        prev->next = p->next;
    }
    else {
        dsp_presets = p->next;
    }
}

void
dsp_preset_replace (ddb_dsp_preset_t *from, ddb_dsp_preset_t *to) {
    ddb_dsp_preset_t *prev = dsp_presets;
    while (prev && prev->next != from) {
        prev = prev->next;
    }
    if (prev) {
        prev->next = to;
    }
    else {
        dsp_presets = to;
    }
    to->next = from->next;
}

static void
escape_filepath (const char *path, char *out, int sz) {
    // escape special chars
    char invalid[] = "$\"`\\";
    char *p = out;
    const char *t = path;
    int n = sz;
    while (*t && n > 1) {
        if (strchr (invalid, *t)) {
            *p++ = '\\';
            n--;
            *p++ = *t;
            n--;
        }
        else {
            *p++ = *t;
            n--;
        }
        t++;
    }
    *p = 0;
}

static void
get_output_field (DB_playItem_t *it, const char *field, char *out, int sz)
{
    int idx = deadbeef->pl_get_idx_of (it);
    char temp[PATH_MAX];
    char fmt[strlen(field)+3];
    snprintf (fmt, sizeof (fmt), "%%/%s", field);
    deadbeef->pl_format_title (it, idx, temp, sizeof (temp), -1, fmt);

    strncpy (out, temp, sz);
    out[sz-1] = 0;
    //trace ("field '%s' expanded to '%s'\n", field, out);
}

static void
get_output_field2 (DB_playItem_t *it, ddb_playlist_t *plt, const char *field, char *out, int sz)
{
    int idx = deadbeef->pl_get_idx_of (it);
    char temp[PATH_MAX];

    char *tf = deadbeef->tf_compile (field);
    if (!tf) {
        *out = 0;
        return;
    }

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .flags = DDB_TF_CONTEXT_HAS_INDEX,
        .it = it,
        .idx = idx,
        .iter = PL_MAIN,
        .plt = plt,
    };

    deadbeef->tf_eval (&ctx, tf, temp, sizeof (temp));
    deadbeef->tf_free (tf);

    char *o = out;
    for (char *p = temp; *p && sz > 0; p++) {
        if (*p == '/') {
            *o++ = '-';
        }
        else
        {
            *o++ = *p;
        }
        sz--;
    }

    *o = 0;

    //trace ("field '%s' expanded to '%s'\n", field, out);
}


static void
get_output_path_int (DB_playItem_t *it, ddb_playlist_t *plt, const char *outfolder_user, const char *outfile, ddb_encoder_preset_t *encoder_preset, int preserve_folder_structure, const char *root_folder, int write_to_source_folder, char *out, int sz, int use_new_tf) {
//    trace ("get_output_path: %s %s %s\n", outfolder_user, outfile, root_folder);

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();

    char outfolder_preserve[PATH_MAX];
    if (preserve_folder_structure) {
        // generate new outfolder
        size_t rootlen = strlen (root_folder);
        const char *e = strrchr (uri, '/');
        if (e) {
            const char *s = uri + rootlen;
            char subpath[e-s+1];
            memcpy (subpath, s, e-s);
            subpath[e-s] = 0;
            snprintf (outfolder_preserve, sizeof (outfolder_preserve), "%s%s", outfolder_user[0] ? outfolder_user : getenv("HOME"), subpath);
        }
    }

    const char *outfolder;

    if (write_to_source_folder) {
        char *path = strdupa (uri);
        char *sep = strrchr (path, '/');
        if (sep) {
            *sep = 0;
        }
        outfolder = path;
    }
    else {
        outfolder = preserve_folder_structure ? outfolder_preserve : outfolder_user;
    }

    size_t l;
    char fname[PATH_MAX];
    char *pattern = strdupa (outfile);

    snprintf (out, sz, "%s/", outfolder);

    // split path, and expand each path component using get_output_field
    char *field = pattern;
    char *s = pattern;
    while (*s) {
        if ((*s == '/') || (*s == '\\')) {
            *s = '\0';
            if (use_new_tf) {
                get_output_field2(it, plt, field, fname, sizeof (fname));
            }
            else {
                get_output_field (it, field, fname, sizeof (fname));
            }

            l = strlen (out);
            snprintf (out+l, sz-l, "%s/", fname);

            field = s+1;
        }
        s++;
    }

    // last part of outfile is the filename
    if (use_new_tf) {
        get_output_field2(it, plt, field, fname, sizeof (fname));
    }
    else {
        get_output_field (it, field, fname, sizeof(fname));
    }

    l = strlen (out);
    if (encoder_preset->ext && encoder_preset->ext[0]) {
        snprintf (out+l, sz-l, "%s.%s", fname, encoder_preset->ext);
    }
    else {
        // get original file ext
        const char *ext = strrchr (uri, '.');
        if (!ext) {
            ext = "";
        }
        snprintf (out+l, sz-l, "%s%s", fname, ext);
    }
    //trace ("converter output file is '%s'\n", out);
}

void
get_output_path (DB_playItem_t *it, const char *outfolder_user, const char *outfile, ddb_encoder_preset_t *encoder_preset, int preserve_folder_structure, const char *root_folder, int write_to_source_folder, char *out, int sz) {
    get_output_path_int (it, NULL, outfolder_user, outfile, encoder_preset, preserve_folder_structure, root_folder, write_to_source_folder, out, sz, 0);
}

void
get_output_path2 (DB_playItem_t *it, ddb_playlist_t *plt, const char *outfolder_user, const char *outfile, ddb_encoder_preset_t *encoder_preset, int preserve_folder_structure, const char *root_folder, int write_to_source_folder, char *out, int sz) {
    get_output_path_int (it, plt, outfolder_user, outfile, encoder_preset, preserve_folder_structure, root_folder, write_to_source_folder, out, sz, 1);
}

static void
get_output_path_1_0 (DB_playItem_t *it, const char *outfolder, const char *outfile, ddb_encoder_preset_t *encoder_preset, char *out, int sz) {
    trace ("warning: old version of \"get_output_path\" has been called, please update your plugins which depend on converter 1.1\n");
    *out = 0;
    sz = 0;
}

static int
check_dir (const char *dir, mode_t mode)
{
    char *tmp = strdup (dir);
    char *slash = tmp;
    struct stat stat_buf;
    do
    {
        slash = strstr (slash+1, "/");
        if (slash)
            *slash = 0;
        if (0 != mkdir (tmp, mode))
        {
            if ( (errno == EEXIST && (-1 == stat (tmp, &stat_buf))) || errno != EEXIST)
            {
                trace ("Failed to create %s\n", tmp);
                free (tmp);
                return 0;
            }
        }
        if (slash)
            *slash = '/';
    } while (slash);
    free (tmp);
    return 1;
}

static inline void
write_int32_le (char *p, uint32_t value) {
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
    p[2] = (value >> 16) & 0xff;
    p[3] = (value >> 24) & 0xff;
}

static inline void
write_int16_le (char *p, uint16_t value) {
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
}

static const unsigned char format_id_float32[] = {
    0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0xaa,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x38, 0x9b, 0x71
};

static const unsigned char format_id_pcm[] = {
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
};

static int64_t
_write_wav (DB_playItem_t *it, DB_decoder_t *dec, DB_fileinfo_t *fileinfo, ddb_dsp_preset_t *dsp_preset, ddb_encoder_preset_t *encoder_preset, int *abort, int fd, int output_bps, int output_is_float) {
    int64_t res = -1;
    char *buffer = NULL;
    char *dspbuffer = NULL;

    // write wave header
    int exheader = output_bps > 16 && !output_is_float;

    int32_t wavehdr_size = 0;
    char wavehdr[0x50];
    int header_written = 0;
    int64_t outsize = 0;
    uint32_t outsr = fileinfo->fmt.samplerate;
    uint16_t outch = fileinfo->fmt.channels;

    int samplesize = fileinfo->fmt.channels * fileinfo->fmt.bps / 8;

    // block size
    int bs = 2000 * samplesize;
    // expected buffer size after worst-case dsp
    int dspsize = bs/samplesize*sizeof(float)*8*48;
    buffer = malloc (dspsize);
    // account for up to float32 7.1 resampled to 48x ratio
    dspbuffer = malloc (dspsize);
    int eof = 0;
    for (;;) {
        if (eof) {
            break;
        }
        if (abort && *abort) {
            break;
        }
        int sz = dec->read (fileinfo, buffer, bs);

        if (sz != bs) {
            eof = 1;
        }
        if (dsp_preset) {
            ddb_waveformat_t fmt;
            ddb_waveformat_t outfmt;
            memcpy (&fmt, &fileinfo->fmt, sizeof (fmt));
            memcpy (&outfmt, &fileinfo->fmt, sizeof (fmt));
            fmt.bps = 32;
            fmt.is_float = 1;
            deadbeef->pcm_convert (&fileinfo->fmt, buffer, &fmt, dspbuffer, sz);

            ddb_dsp_context_t *dsp = dsp_preset->chain;
            int frames = sz / samplesize;
            while (dsp) {
                frames = dsp->plugin->process (dsp, (float *)dspbuffer, frames, dspsize / (fmt.channels * 4), &fmt, NULL);
                if (frames <= 0) {
                    break;
                }
                dsp = dsp->next;
            }
            if (frames <= 0) {
                trace ("DSP error, please check you dsp preset\n");
                goto error;
            }

            outsr = fmt.samplerate;
            outch = fmt.channels;

            outfmt.bps = output_bps;
            outfmt.is_float = output_is_float;
            outfmt.channels = outch;
            outfmt.samplerate = outsr;

            int n = deadbeef->pcm_convert (&fmt, dspbuffer, &outfmt, buffer, frames * sizeof (float) * fmt.channels);
            sz = n;
        }
        else if (fileinfo->fmt.bps != output_bps || fileinfo->fmt.is_float != output_is_float) {
            ddb_waveformat_t outfmt;
            memcpy (&outfmt, &fileinfo->fmt, sizeof (outfmt));
            outfmt.bps = output_bps;
            outfmt.is_float = output_is_float;
            outfmt.channels = outch;
            outfmt.samplerate = outsr;

            int frames = sz / samplesize;
            int n = deadbeef->pcm_convert (&fileinfo->fmt, buffer, &outfmt, dspbuffer, frames * samplesize);
            memcpy (buffer, dspbuffer, n);
            sz = n;
        }
        outsize += sz;

        if (!header_written) {
            int64_t startsample = deadbeef->pl_item_get_startsample (it);
            int64_t endsample = deadbeef->pl_item_get_endsample (it);
            uint64_t size = (int64_t)(endsample-startsample) * outch * output_bps / 8;
            if (!size) {
                size = (double)deadbeef->pl_get_item_duration (it) * fileinfo->fmt.samplerate * outch * output_bps / 8;

            }

            if (outsr != fileinfo->fmt.samplerate) {
                uint64_t temp = size;
                temp *= outsr;
                temp /= fileinfo->fmt.samplerate;
                size  = temp;
            }

            uint64_t chunksize;
            chunksize = size + 40;

            // for exheader, add 36 more
            if (exheader) {
                chunksize += 36;
            }

            uint32_t size32 = 0xffffffff;
            if (chunksize <= 0xffffffff) {
                size32 = (uint32_t)chunksize;
            }

            memcpy (wavehdr, "RIFF", 4); // RIFFxxxxWAVEfmt_
            write_int32_le (wavehdr+4, size32);
            memcpy (wavehdr+8, "WAVE", 4);
            memcpy (wavehdr+12, "fmt ", 4);
            int32_t wavefmtsize = exheader ? 0x28 : 0x10;
            write_int32_le (wavehdr+16, wavefmtsize); // chunk size; fe ff; num chan ; samples_per_sec; avg_bytes_per_sec
            int16_t fmt = exheader ? 0xfffe : (output_is_float ? 3 : 1);
            write_int16_le (wavehdr+20, fmt);
            write_int16_le (wavehdr+22, outch);
            write_int32_le (wavehdr+24, outsr);
            int32_t bytes_per_sec = outsr * output_bps / 8 * outch;
            write_int32_le (wavehdr+28, bytes_per_sec);
            uint16_t blockalign = outch * output_bps / 8; // block_align; bits_per_sample; cbSize; validBPS
            write_int16_le (wavehdr+32, blockalign);
            write_int16_le (wavehdr+34, output_bps);
            if (exheader) {
                int16_t cbSize = 0x16;
                write_int16_le (wavehdr+36, cbSize); // cbSize (validBPS + channelmask + codec ID = 22 bytes)
                write_int16_le (wavehdr+38, output_bps); // validBPS
                int32_t chMask = 3;
                write_int32_le (wavehdr+40, chMask); // channelMask

                memcpy (wavehdr + 44, output_is_float ? format_id_float32 : format_id_pcm, 16); // 16 bytes format ID
                memcpy (wavehdr + 60, "data", 4);
                wavehdr_size = 64;
            }
            else {
                memcpy (wavehdr + 36, "data", 4);
                wavehdr_size = 40;
            }

            size32 = 0xffffffff;
            if (size <= 0xffffffff) {
                size32 = (uint32_t)size;
            }

            if (wavehdr_size != write (fd, wavehdr, wavehdr_size)) {
                trace ("Wave header write error\n");
                goto error;
            }
            if (encoder_preset->method == DDB_ENCODER_METHOD_PIPE) {
                size32 = 0;
            }
            if (write (fd, &size32, sizeof (size32)) != sizeof (size32)) {
                trace ("Wave header size write error\n");
                goto error;
            }
            header_written = 1;
        }

        if (output_bps == 8) {
            // convert to unsigned
            for (char *p = buffer; p < buffer + sz; p++) {
                uint8_t sample = (uint8_t)(((int)*p) + 128);
                *((uint8_t *)p) = sample;
            }
        }

        int64_t res = write (fd, buffer, sz);
        if (sz != res) {
            trace ("Write error (%"PRId64" bytes written out of %d)\n", res, sz);
            goto error;
        }
    }

    res = outsize;

    // rewrite wave data size
    if (encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
        uint32_t writesize;

        // RIFF chunk size
        lseek (fd, 4, SEEK_SET);
        int64_t riffsize = wavehdr_size + outsize - 4;
        if (riffsize <= 0xffffffff) {
            writesize = (uint32_t)riffsize;
        }
        else {
            writesize = 0xffffffff;
        }
        if (4 != write (fd, &writesize, 4)) {
            trace_err ("converter: riff size write error\n");
            goto error;
        }

        // data size
        lseek (fd, wavehdr_size, SEEK_SET);
        if (outsize <= 0xffffffff) {
            writesize = (uint32_t)outsize;
        }
        else {
            writesize = 0xffffffff;
        }
        if (4 != write (fd, &writesize, 4)) {
            trace_err ("converter: data size write error\n");
            goto error;
        }
    }

error:

    if (buffer) {
        free (buffer);
        buffer = NULL;
    }
    if (dspbuffer) {
        free (dspbuffer);
        dspbuffer = NULL;
    }

    return res;
}

static int
_get_encoder_cmdline (ddb_encoder_preset_t *encoder_preset, char *enc, int len, const char *escaped_out, const char *input_file_name) {
    // formatting: %o = outfile, %i = infile
    char *e = encoder_preset->encoder;
    char *o = enc;
    *o = 0;

    while (e && *e) {
        if (len <= 0) {
            trace ("Failed to assemble encoder command line - buffer is not big enough, try to shorten your parameters. max allowed length is %u characters\n", (unsigned)sizeof (enc));
            return -1;
        }
        if (e[0] == '%' && e[1]) {
            if (e[1] == 'o') {
                int l = snprintf (o, len, "\"%s\"", escaped_out);
                o += l;
                len -= l;
            }
            else if (e[1] == 'i') {
                int l = snprintf (o, len, "\"%s\"", input_file_name);
                o += l;
                len -= l;
            }
            else {
                strncpy (o, e, 2);
                o += 2;
                len -= 2;
            }
            e += 2;
        }
        else {
            *o++ = *e++;
            *o = 0;
            len--;
        }
    }

    return 0;
}

#define BUFFER_SIZE 4096
static int
_copy_file (const char *in, const char *out) {
    char *final_path = strdupa (out);
    char *sep = strrchr (final_path, '/');
    if (sep) {
        *sep = 0;
        if (!check_dir (final_path, 0755)) {
            trace ("Failed to create output folder: %s\n", final_path);
            return -1;
        }
    }

    DB_FILE *infile = deadbeef->fopen (in);
    if (!infile) {
        trace ("Failed to open file %s for reading\n", in);
        return -1;
    }

    char tmp_out[PATH_MAX];
    snprintf (tmp_out, PATH_MAX, "%s.part", out);
    FILE *fout = fopen (tmp_out, "w+b");
    if (!fout) {
        trace ("Failed to open file %s for writing\n", tmp_out);
        deadbeef->fclose (infile);
        return -1;
    }

    int err = 0;
    int64_t bytes_read;
    size_t file_bytes = 0;
    do {
        char buffer[BUFFER_SIZE];
        bytes_read = deadbeef->fread (buffer, 1, BUFFER_SIZE, infile);
        if (bytes_read > 0 && fwrite (buffer, bytes_read, 1, fout) != 1) {
            trace ("Failed to write file %s: %s\n", tmp_out, strerror (errno));
            err = -1;
        }
        file_bytes += bytes_read;
    } while (!err && bytes_read == BUFFER_SIZE);

    deadbeef->fclose (infile);

    if (fclose (fout)) {
        trace ("Failed to write file %s: %s\n", tmp_out, strerror (errno));
        unlink (tmp_out);
        return -1;
    }

    if (file_bytes > 0 && !err) {
        err = rename (tmp_out, out);
        if (err) {
            trace ("Failed to move %s to %s: %s\n", tmp_out, out, strerror (errno));
        }
    }

    unlink (tmp_out);
    return err;
}

// replaygain key names in deadbeef internal metadata
static const char *ddb_internal_rg_keys[] = {
    ":REPLAYGAIN_ALBUMGAIN",
    ":REPLAYGAIN_ALBUMPEAK",
    ":REPLAYGAIN_TRACKGAIN",
    ":REPLAYGAIN_TRACKPEAK",
    NULL
};

static int
_converter_write_tags (ddb_encoder_preset_t *encoder_preset, DB_playItem_t *it, const char *out) {
    int err = 0;

    DB_playItem_t *out_it = NULL;

    if (!(encoder_preset->tag_id3v2
        || encoder_preset->tag_id3v1
        || encoder_preset->tag_apev2
        || encoder_preset->tag_flac
        || encoder_preset->tag_oggvorbis
        || encoder_preset->tag_mp4)) {
        // encoder doesn't specify tagging format
        return 0;
    }

    out_it = deadbeef->pl_item_init (out);

    if (!out_it) {
        // can't initialize the converted file, just copy metadata from source
        out_it = deadbeef->pl_item_alloc ();
        deadbeef->pl_item_copy (out_it, it);
        deadbeef->pl_set_item_flags (out_it, 0);
    }
    else {
        // merge metadata
        deadbeef->pl_lock ();

        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (it);
        while (meta) {
            if (strchr (":!_", meta->key[0])) {
                break;
            }
            if (!deadbeef->pl_meta_exists (out_it, meta->key)) {
                deadbeef->pl_append_meta (out_it, meta->key, meta->value);
            }
            meta = meta->next;
        }

        deadbeef->pl_unlock ();
    }

    DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (out_it);
    while (m) {
        DB_metaInfo_t *next = m->next;
        if (m->key[0] == ':' || m->key[0] == '!' || !strcasecmp (m->key, "cuesheet")) {
            int i;
            for (i = 0; ddb_internal_rg_keys[i]; i++) {
                if (!strcasecmp (m->key, ddb_internal_rg_keys[i])) {
                    break;
                }
            }
            if (!ddb_internal_rg_keys[i]) {
                deadbeef->pl_delete_metadata (out_it, m);
            }
        }
        m = next;
    }
    deadbeef->pl_replace_meta (out_it, ":URI", out);

    uint32_t tagflags = 0;
    if (encoder_preset->tag_id3v2) {
        tagflags |= JUNK_WRITE_ID3V2;
    }
    if (encoder_preset->tag_id3v1) {
        tagflags |= JUNK_WRITE_ID3V1;
    }
    if (encoder_preset->tag_apev2) {
        tagflags |= JUNK_WRITE_APEV2;
    }

    if (tagflags) {
        tagflags |= JUNK_STRIP_ID3V2 | JUNK_STRIP_APEV2 | JUNK_STRIP_ID3V1;
        deadbeef->junk_rewrite_tags (out_it, tagflags, encoder_preset->id3v2_version + 3, "iso8859-1");
    }

    // write flac tags
    if (encoder_preset->tag_flac) {
        // find flac decoder plugin
        DB_decoder_t **plugs = deadbeef->plug_get_decoder_list ();
        int res = -1;
        for (int i = 0; plugs[i]; i++) {
            if (!strcmp (plugs[i]->plugin.id, "stdflac")) {
                res = plugs[i]->write_metadata (out_it);
                if (!res) {
                    break;
                }
                break;
            }
        }
        if (res) {
            trace ("converter: Failed to write FLAC metadata to %s\n", out);
        }
    }

    // write vorbis tags
    if (encoder_preset->tag_oggvorbis) {
        // find plugin to write vorbis comment tags
        DB_decoder_t **plugs = deadbeef->plug_get_decoder_list ();
        int res = -1;
        for (int i = 0; plugs[i]; i++) {
            if (!strcmp (plugs[i]->plugin.id, "stdogg")
                || !strcmp (plugs[i]->plugin.id, "opus")
                || !strcmp (plugs[i]->plugin.id, "stdopus")) {
                res = plugs[i]->write_metadata (out_it);
                if (!res) {
                    break;
                }
            }
        }
        if (res) {
            trace ("converter: Failed to write ogg metadata to %s\n", out);
        }
    }

    if (encoder_preset->tag_mp4) {
        int res = mp4_write_metadata(out_it);
        if (res) {
            trace ("converter: Failed to write mp4 metadata to %s\n", out);
        }
    }

    if (out_it) {
        deadbeef->pl_item_unref (out_it);
    }

    return err;
}

static int
ddb_mktemp(char *template, size_t template_size, char *suffix) {
#if PORTABLE || defined(_WIN32) || !HAVE_MKSTEMPS
    // This codepath should only be used when building with older LIBC
    // which doesn't support mkstemps,
    // such as glibc < 2.19 or MSVCRT.
    (void)mktemp (template);
    strncat(template, suffix, template_size);
    return open(template,
                O_LARGEFILE | O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#else
    // Here, we would like to use mkstemp for better compatibility,
    // but we want to have the file extension,
    // to avoid confusing external command line encoders.
    strncat(template, suffix, template_size);
    return mkstemps(template, (int)strlen (suffix));
#endif
}

static int
convert2 (ddb_converter_settings_t *settings, DB_playItem_t *it, const char *out, int *pabort) {
    int output_bps = settings->output_bps;
    int output_is_float = settings->output_is_float;
    ddb_encoder_preset_t *encoder_preset = settings->encoder_preset;
    ddb_dsp_preset_t *dsp_preset = settings->dsp_preset;
    int bypass_conversion_on_same_format = settings->bypass_conversion_on_same_format;
    int rewrite_tags_after_copy = settings->rewrite_tags_after_copy;

    char enc[2000];
    memset (enc, 0, sizeof (enc));

    const char *fname;
    deadbeef->pl_lock ();
    fname = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();

    if (deadbeef->pl_get_item_duration (it) <= 0) {
        trace ("stream %s doesn't have finite length, skipped\n", fname);
        return -1;
    }

    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
    }
    else {
        ext = "";
    }

    if (bypass_conversion_on_same_format && !strcasecmp (ext, encoder_preset->ext)) {
        int res = _copy_file (fname, out);
        if (res) {
            return res;
        }
        if (rewrite_tags_after_copy) {
            (void)_converter_write_tags (encoder_preset, it, out);
        }
        return res;
    }

    int err = -1;
    FILE *enc_pipe = NULL;
    int temp_file = -1;
    DB_decoder_t *dec = NULL;
    DB_fileinfo_t *fileinfo = NULL;
    char input_file_name[PATH_MAX] = "";

    char *final_path = strdupa (out);
    char *sep = strrchr (final_path, '/');
    if (sep) {
        *sep = 0;
        if (!check_dir (final_path, 0755)) {
            trace ("Failed to create output folder: %s\n", final_path);
            goto error;
        }
    }
    char escaped_out[PATH_MAX];
    escape_filepath (out, escaped_out, sizeof (escaped_out));

    // only need to decode / process the file if not passing the source filename
    if (encoder_preset->method == DDB_ENCODER_METHOD_FILE || encoder_preset->method == DDB_ENCODER_METHOD_PIPE) {
        deadbeef->pl_lock ();
        dec = (DB_decoder_t *)deadbeef->plug_get_for_id (deadbeef->pl_find_meta (it, ":DECODER"));
        deadbeef->pl_unlock ();

        if (dec) {
            fileinfo = dec->open (DDB_DECODER_HINT_RAW_SIGNAL);
            if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (it)) != 0) {
                trace ("Failed to decode file %s\n", fname);
                goto error;
            }
            if (fileinfo) {
                if (output_bps == -1) {
                    output_bps = fileinfo->fmt.bps;
                    output_is_float = fileinfo->fmt.is_float;
                }

                switch (encoder_preset->method) {
                    case DDB_ENCODER_METHOD_FILE:
                    {
                        const char *tmp = getenv ("TMPDIR");
                        if (!tmp) {
                            tmp = "/tmp";
                        }
                        snprintf (input_file_name, sizeof (input_file_name), "%s/ddbconvXXXXXX", tmp);
                        temp_file = ddb_mktemp(input_file_name, sizeof(input_file_name), ".wav");
                        if (temp_file == -1) {
                            trace ("Failed to open temp file %s\n", input_file_name);
                            goto error;
                        }
                    }
                        break;
                    case DDB_ENCODER_METHOD_PIPE:
                        strcpy (input_file_name, "-");
                        break;
                    default:
                        trace ("Invalid encoder method: %d, check your encoder preset\n", encoder_preset->method);
                        goto error;
                }

                if (_get_encoder_cmdline (encoder_preset, enc, sizeof (enc), escaped_out, input_file_name) < 0) {
                    goto error;
                }

                mode_t wrmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

                if (!encoder_preset->encoder[0]) {
                    // write to wave file
                    temp_file = open (out, O_LARGEFILE | O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY, wrmode);
                    if (temp_file == -1) {
                        trace ("Failed to open output wave file %s\n", out);
                        goto error;
                    }
                }
                else if (encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                    // File should have been opened already
                    if (temp_file == -1) {
                        temp_file = open (input_file_name, O_LARGEFILE | O_WRONLY | O_CREAT | O_TRUNC | _O_BINARY, wrmode);
                    }
                    if (temp_file == -1) {
                        trace ("Failed to open temp file %s\n", input_file_name);
                        goto error;
                    }
                }
                else {
                    enc_pipe = popen (enc, "w");
                    if (!enc_pipe) {
                        trace ("Failed to execute the encoder, command used:\n%s\n", enc[0] ? enc : "internal RIFF WAVE writer");
                        goto error;
                    }
                }

                if (encoder_preset->method == DDB_ENCODER_METHOD_FILE || encoder_preset->method == DDB_ENCODER_METHOD_PIPE) {
                    if (temp_file == -1 && enc_pipe) {
                        temp_file = fileno (enc_pipe);
                    }
                }

                if (temp_file > 0) {
                    int64_t outsize = _write_wav (it, dec, fileinfo, dsp_preset, encoder_preset, pabort, temp_file, output_bps, output_is_float);

                    if (outsize < 0) {
                        goto error;
                    }

                    if (pabort && *pabort) {
                        goto error;
                    }

                    if (encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                        if (temp_file != -1 && (!enc_pipe || temp_file != fileno (enc_pipe))) {
                            close (temp_file);
                            temp_file = -1;
                        }
                    }
                }
            }
        }
    }
    else if (encoder_preset->method == DDB_ENCODER_METHOD_FILENAME) {
        if (_get_encoder_cmdline (encoder_preset, enc, sizeof (enc), escaped_out, fname) < 0) {
            goto error;
        }
    }

    if (enc[0] && (encoder_preset->method == DDB_ENCODER_METHOD_FILE || encoder_preset->method == DDB_ENCODER_METHOD_FILENAME)) {
        enc_pipe = popen (enc, "w");
    }

    err = 0;
error:
    if (temp_file != -1 && (!enc_pipe || temp_file != fileno (enc_pipe))) {
        close (temp_file);
        temp_file = -1;
    }
    if (enc_pipe) {
        err = pclose (enc_pipe);
        err = WEXITSTATUS(err);
        if (err) {
            trace ("Failed to execute the encoder, command used:\n%s\n", enc[0] ? enc : "internal RIFF WAVE writer");
            err = -1;
        }
        enc_pipe = NULL;
    }
    if (dec && fileinfo) {
        dec->free (fileinfo);
        fileinfo = NULL;
    }
    if (pabort && *pabort && out[0]) {
        unlink (out);
    }
    if (input_file_name[0] && strcmp (input_file_name, "-")) {
        unlink (input_file_name);
    }
    if (err != 0) {
        return err;
    }

    (void)_converter_write_tags (encoder_preset, it, out);

    return err;
}

static int
convert (DB_playItem_t *it, const char *out, int output_bps, int output_is_float, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort) {
    ddb_converter_settings_t settings = {
        .output_bps = output_bps,
        .output_is_float = output_is_float,
        .encoder_preset = encoder_preset,
        .dsp_preset = dsp_preset,
    };

    return convert2(&settings, it, out, abort);
}


static int
convert_1_0 (DB_playItem_t *it, const char *outfolder, const char *outfile, int output_bps, int output_is_float, int preserve_folder_structure, const char *root_folder, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort) {
    trace ("An old version of \"convert\" has been called, please update your plugins which depend on converter 1.1\n");
    return -1;
}

int
converter_cmd (int cmd, ...) {
    return -1;
}

int
converter_start (void) {
    load_encoder_presets ();
    load_dsp_presets ();

    return 0;
}

int
converter_stop (void) {
    free_encoder_presets ();
    free_dsp_presets ();
    return 0;
}

// define plugin interface
static ddb_converter_t plugin = {
    .misc.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .misc.plugin.api_vminor = DB_API_VERSION_MINOR,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 5,
    .misc.plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.name = "Converter",
    .misc.plugin.id = "converter",
    .misc.plugin.descr = "Converts any supported formats to other formats.\n"
        "Requires separate GUI plugin, e.g. Converter GTK UI\n",
    .misc.plugin.copyright =
        "Converter for DeaDBeeF Player\n"
        "Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors\n"
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
    .misc.plugin.website = "http://deadbeef.sf.net",
    .misc.plugin.start = converter_start,
    .misc.plugin.stop = converter_stop,
    .misc.plugin.command = converter_cmd,
    .encoder_preset_alloc = encoder_preset_alloc,
    .encoder_preset_free = encoder_preset_free,
    .encoder_preset_load = encoder_preset_load,
    .encoder_preset_save = encoder_preset_save,
    .encoder_preset_copy = encoder_preset_copy,
    .encoder_preset_get_list = encoder_preset_get_list,
    .encoder_preset_get_for_idx = encoder_preset_get_for_idx,
    .encoder_preset_append = encoder_preset_append,
    .encoder_preset_remove = encoder_preset_remove,
    .encoder_preset_replace = encoder_preset_replace,
    .dsp_preset_alloc = dsp_preset_alloc,
    .dsp_preset_free = dsp_preset_free,
    .dsp_preset_load = dsp_preset_load,
    .dsp_preset_save = dsp_preset_save,
    .dsp_preset_copy = dsp_preset_copy,
    .dsp_preset_get_list = dsp_preset_get_list,
    .dsp_preset_get_for_idx = dsp_preset_get_for_idx,
    .dsp_preset_append = dsp_preset_append,
    .dsp_preset_remove = dsp_preset_remove,
    .dsp_preset_replace = dsp_preset_replace,
    .get_output_path_1_0 = get_output_path_1_0,
    .convert_1_0 = convert_1_0,
    // 1.1 entry points
    .load_encoder_presets = load_encoder_presets,
    .load_dsp_presets = load_dsp_presets,
    .free_encoder_presets = free_encoder_presets,
    .free_dsp_presets = free_dsp_presets,
    // 1.2 entry points
    .convert = convert,
    .get_output_path = get_output_path,
    // 1.4 entry points
    .get_output_path2 = get_output_path2,
    // 1.5 entry points
    .convert2 = convert2,
};

DB_plugin_t *
converter_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
