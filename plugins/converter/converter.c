/*
    Converter for DeaDBeeF Player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include "converter.h"
#include "../../deadbeef.h"
#include "../../strdupa.h"

#ifndef __linux__
#define O_LARGEFILE 0
#endif

#define min(x,y) ((x)<(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static ddb_converter_t plugin;
static DB_functions_t *deadbeef;

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
        fprintf (stderr, "encoder_preset_save: empty title\n");
        return -1;
    }
    const char *confdir = deadbeef->get_config_dir ();
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
        fprintf (stderr, "dsp_preset_save: empty title\n");
        return -1;
    }
    const char *confdir = deadbeef->get_config_dir ();
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

static int
copy_file (const char *in, const char *out) {
    int BUFFER_SIZE = 1000;
    FILE *fin = fopen (in, "rb");
    if (!fin) {
        fprintf (stderr, "converter: failed to open file %s for reading\n", in);
        return -1;
    }
    FILE *fout = fopen (out, "w+b");
    if (!fout) {
        fclose (fin);
        fprintf (stderr, "converter: failed to open file %s for writing\n", out);
        return -1;
    }
    char *buf = malloc (BUFFER_SIZE);
    if (!buf) {
        fprintf (stderr, "converter: failed to alloc %d bytes\n", BUFFER_SIZE);
        fclose (fin);
        fclose (fout);
        return -1;
    }

    fseek (fin, 0, SEEK_END);
    size_t sz = ftell (fin);
    rewind (fin);

    while (sz > 0) {
        int rs = min (sz, BUFFER_SIZE);
        if (fread (buf, rs, 1, fin) != 1) {
            fprintf (stderr, "converter: failed to read file %s\n", in);
            break;
        }
        if (fwrite (buf, rs, 1, fout) != 1) {
            fprintf (stderr, "converter: failed to write file %s\n", out);
            break;
        }
        sz -= rs;
    }
    free (buf);
    fclose (fin);
    fclose (fout);
    if (sz > 0) {
        unlink (out);
    }
    return 0;
}

int
load_encoder_presets (void) {
    // check if we need to install presets
    char ppath[PATH_MAX];
    char epath[PATH_MAX];
    snprintf (ppath, sizeof (ppath), "%s/presets", deadbeef->get_config_dir ());
    snprintf (epath, sizeof (epath), "%s/encoders", ppath);

    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/presets/encoders", deadbeef->get_config_dir ()) < 0) {
        return -1;
    }

    char syspath[PATH_MAX];
    if (snprintf (syspath, sizeof (syspath), "%s/convpresets", deadbeef->get_plugin_dir ()) < 0) {
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
    if (snprintf (path, sizeof (path), "%s/presets/dsp", deadbeef->get_config_dir ()) < 0) {
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
    trace ("field '%s' expanded to '%s'\n", field, out);
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

    trace ("field '%s' expanded to '%s'\n", field, out);
}


static void
get_output_path_int (DB_playItem_t *it, ddb_playlist_t *plt, const char *outfolder_user, const char *outfile, ddb_encoder_preset_t *encoder_preset, int preserve_folder_structure, const char *root_folder, int write_to_source_folder, char *out, int sz, int use_new_tf) {
    trace ("get_output_path: %s %s %s\n", outfolder_user, outfile, root_folder);
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    char outfolder_preserve[2000];
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

    int l;
    char fname[PATH_MAX];
    size_t pathl = strlen(outfolder)*2+1;
    char path[pathl];
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
    snprintf (out+l, sz-l, "%s.%s", fname, encoder_preset->ext);
    trace ("converter output file is '%s'\n", out);
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
    fprintf (stderr, "converter: warning: old version of \"get_output_path\" has been called, please update your plugins which depend on converter 1.1\n");
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
        if (-1 == stat (tmp, &stat_buf))
        {
            trace ("creating dir %s\n", tmp);
            if (0 != mkdir (tmp, mode))
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


int
convert (DB_playItem_t *it, const char *out, int output_bps, int output_is_float, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort) {
    char *buffer = NULL;
    char *dspbuffer = NULL;
    if (deadbeef->pl_get_item_duration (it) <= 0) {
        deadbeef->pl_lock ();
        fprintf (stderr, "converter: stream %s doesn't have finite length, skipped\n", deadbeef->pl_find_meta (it, ":URI"));
        deadbeef->pl_unlock ();
        return -1;
    }

    int err = -1;
    FILE *enc_pipe = NULL;
    int temp_file = -1;
    DB_decoder_t *dec = NULL;
    DB_fileinfo_t *fileinfo = NULL;
    char input_file_name[PATH_MAX] = "";
    deadbeef->pl_lock ();
    dec = (DB_decoder_t *)deadbeef->plug_get_for_id (deadbeef->pl_find_meta (it, ":DECODER"));
    deadbeef->pl_unlock ();

    if (dec) {
        fileinfo = dec->open (0);
        if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (it)) != 0) {
            deadbeef->pl_lock ();
            fprintf (stderr, "converter: failed to decode file %s\n", deadbeef->pl_find_meta (it, ":URI"));
            deadbeef->pl_unlock ();
            goto error;
        }
        if (fileinfo) {
            if (output_bps == -1) {
                output_bps = fileinfo->fmt.bps;
                output_is_float = fileinfo->fmt.is_float;
            }

            char *final_path = strdupa (out);
            char *sep = strrchr (final_path, '/');
            if (sep) {
                *sep = 0;
                if (!check_dir (final_path, 0755)) {
                    fprintf (stderr, "converter: failed to create output folder: %s\n", final_path);
                    goto error;
                }
            }

            if (encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                const char *tmp = getenv ("TMPDIR");
                if (!tmp) {
                    tmp = "/tmp";
                }
                snprintf (input_file_name, sizeof (input_file_name), "%s/ddbconvXXXXXX", tmp);
                char *res = mktemp (input_file_name);
                strcat (input_file_name, ".wav");
            }
            else {
                strcpy (input_file_name, "-");
            }

            char enc[2000];
            memset (enc, 0, sizeof (enc));

            char escaped_out[PATH_MAX];
            escape_filepath (out, escaped_out, sizeof (escaped_out));

            // formatting: %o = outfile, %i = infile
            char *e = encoder_preset->encoder;
            char *o = enc;
            *o = 0;

#ifdef __APPLE__
            strcpy (o, "/usr/local/bin/");
            o += 15;
#endif

            int len = sizeof (enc);
            while (e && *e) {
                if (len <= 0) {
                    fprintf (stderr, "converter: failed to assemble encoder command line - buffer is not big enough, try to shorten your parameters. max allowed length is %u characters\n", (unsigned)sizeof (enc));
                    goto error;
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

            fprintf (stderr, "converter: will encode using: %s\n", enc[0] ? enc : "internal RIFF WAVE writer");

            mode_t wrmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

            if (!encoder_preset->encoder[0]) {
                // write to wave file
                trace ("opening %s\n", out);
                temp_file = open (out, O_LARGEFILE | O_WRONLY | O_CREAT | O_TRUNC, wrmode);
                if (temp_file == -1) {
                    fprintf (stderr, "converter: failed to open output wave file %s\n", out);
                    goto error;
                }
            }
            else if (encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                temp_file = open (input_file_name, O_LARGEFILE | O_WRONLY | O_CREAT | O_TRUNC, wrmode);
                if (temp_file == -1) {
                    fprintf (stderr, "converter: failed to open temp file %s\n", input_file_name);
                    goto error;
                }
            }
            else {
                enc_pipe = popen (enc, "w");
                if (!enc_pipe) {
                    fprintf (stderr, "converter: failed to open encoder\n");
                    goto error;
                }
            }

            if (temp_file == -1 && enc_pipe) {
                temp_file = fileno (enc_pipe);
            }

            // write wave header
            char wavehdr_int[] = {
                0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xac, 0x00, 0x00, 0x10, 0xb1, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61
            };
            char wavehdr_float[] = {
                0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x28, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x02, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x08, 0x00, 0x20, 0x00, 0x16, 0x00, 0x20, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71, 0x66, 0x61, 0x63, 0x74, 0x04, 0x00, 0x00, 0x00, 0xc5, 0x5b, 0x00, 0x00, 0x64, 0x61, 0x74, 0x61
            };
            char *wavehdr = output_is_float ? wavehdr_float : wavehdr_int;
            int wavehdr_size = output_is_float ? sizeof (wavehdr_float) : sizeof (wavehdr_int);
            int header_written = 0;
            uint32_t outsize = 0;
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
                        fprintf (stderr, "converter: dsp error, please check you dsp preset\n");
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
                    uint64_t size = (int64_t)(it->endsample-it->startsample) * outch * output_bps / 8;
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

                    // for float, add 36 more
                    if (output_is_float) {
                        chunksize += 36;
                    }

                    uint32_t size32 = 0xffffffff;
                    if (chunksize <= 0xffffffff) {
                        size32 = chunksize;
                    }
                    write_int32_le (wavehdr+4, size32);
                    write_int16_le (wavehdr+22, outch);
                    write_int32_le (wavehdr+24, outsr);
                    uint16_t blockalign = outch * output_bps / 8;
                    write_int16_le (wavehdr+32, blockalign);
                    write_int16_le (wavehdr+34, output_bps);

                    size32 = 0xffffffff;
                    if (size <= 0xffffffff) {
                        size32 = size;
                    }

                    if (wavehdr_size != write (temp_file, wavehdr, wavehdr_size)) {
                        fprintf (stderr, "converter: wave header write error\n");
                        goto error;
                    }
                    if (encoder_preset->method == DDB_ENCODER_METHOD_PIPE) {
                        size32 = 0;
                    }
                    if (write (temp_file, &size32, sizeof (size32)) != sizeof (size32)) {
                        fprintf (stderr, "converter: wave header size write error\n");
                        goto error;
                    }
                    header_written = 1;
                }

                int64_t res = write (temp_file, buffer, sz);
                if (sz != res) {
                    fprintf (stderr, "converter: write error (%"PRId64" bytes written out of %d)\n", res, sz);
                    goto error;
                }
            }
            if (abort && *abort) {
                goto error;
            }
            if (temp_file != -1 && (!enc_pipe || temp_file != fileno (enc_pipe))) {
                lseek (temp_file, wavehdr_size, SEEK_SET);
                if (4 != write (temp_file, &outsize, 4)) {
                    fprintf (stderr, "converter: data size write error\n");
                    goto error;
                }

                if (temp_file != -1 && (!enc_pipe || temp_file != fileno (enc_pipe))) {
                    close (temp_file);
                    temp_file = -1;
                }
            }

            if (encoder_preset->encoder[0] && encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                enc_pipe = popen (enc, "w");
            }
        }
    }
    err = 0;
error:
    if (buffer) {
        free (buffer);
        buffer = NULL;
    }
    if (dspbuffer) {
        free (dspbuffer);
        dspbuffer = NULL;
    }
    if (temp_file != -1 && (!enc_pipe || temp_file != fileno (enc_pipe))) {
        close (temp_file);
        temp_file = -1;
    }
    if (enc_pipe) {
        pclose (enc_pipe);
        enc_pipe = NULL;
    }
    if (dec && fileinfo) {
        dec->free (fileinfo);
        fileinfo = NULL;
    }
    if (abort && *abort && out[0]) {
        unlink (out);
    }
    if (input_file_name[0] && strcmp (input_file_name, "-")) {
        unlink (input_file_name);
    }
    if (err != 0) {
        return err;
    }

    // write junklib tags

    DB_playItem_t *out_it = NULL;

    if (encoder_preset->tag_id3v2 || encoder_preset->tag_id3v1 || encoder_preset->tag_apev2 || encoder_preset->tag_flac || encoder_preset->tag_oggvorbis) {
        out_it = deadbeef->pl_item_alloc ();
        deadbeef->pl_item_copy (out_it, it);
        deadbeef->pl_set_item_flags (out_it, 0);
        DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (out_it);
        while (m) {
            DB_metaInfo_t *next = m->next;
            if (m->key[0] == ':' || m->key[0] == '!' || !strcasecmp (m->key, "cuesheet")) {
                if (strcasestr (m->key, ":REPLAYGAIN_")) {
                    deadbeef->pl_delete_metadata (out_it, m);
                }
            }
            m = next;
        }
        deadbeef->pl_replace_meta (out_it, ":URI", out);
    }

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
        DB_decoder_t *flac = NULL;
        for (int i = 0; plugs[i]; i++) {
            if (!strcmp (plugs[i]->plugin.id, "stdflac")) {
                flac = plugs[i];
                break;
            }
        }
        if (!flac) {
            fprintf (stderr, "converter: flac plugin not found, cannot write flac metadata\n");
        }
        else {
            if (0 != flac->write_metadata (out_it)) {
                fprintf (stderr, "converter: failed to write flac metadata, not a flac file?\n");
            }
        }
    }

    // write vorbis tags
    if (encoder_preset->tag_oggvorbis) {
        // find flac decoder plugin
        DB_decoder_t **plugs = deadbeef->plug_get_decoder_list ();
        int res = -1;
        for (int i = 0; plugs[i]; i++) {
            if (!strcmp (plugs[i]->plugin.id, "stdogg")
                    || !strcmp (plugs[i]->plugin.id, "stdopus")) {
                res = plugs[i]->write_metadata (out_it);
                if (!res) {
                    break;
                }
            }
        }
        if (res) {
            fprintf (stderr, "converter: failed to write ogg metadata, not an ogg file?\n");
        }
    }
    if (out_it) {
        deadbeef->pl_item_unref (out_it);
    }

    return err;
}

int
convert_1_0 (DB_playItem_t *it, const char *outfolder, const char *outfile, int output_bps, int output_is_float, int preserve_folder_structure, const char *root_folder, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort) {
    fprintf (stderr, "converter: warning: old version of \"convert\" has been called, please update your plugins which depend on converter 1.1\n");
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
    .misc.plugin.api_vmajor = 1,
    .misc.plugin.api_vminor = 0,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 4,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.name = "Converter",
    .misc.plugin.id = "converter",
    .misc.plugin.descr = "Converts any supported formats to other formats.\n"
        "Requires separate GUI plugin, e.g. Converter GTK UI\n",
    .misc.plugin.copyright = 
        "Converter for DeaDBeeF Player\n"
        "Copyright (C) 2009-2015 Alexey Yakovenko and other contributors\n"
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
};

DB_plugin_t *
converter_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
