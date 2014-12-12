/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <libgen.h>
#include "../../deadbeef.h"
#include "converter.h"

#ifndef P_tmpdir
    #define P_tmpdir "/tmp"
#endif

#if WORDS_BIGENDIAN
    #define LE16(x) (((uint16_t)x & 0xff00) >> 8 | ((uint16_t)x & 0x00ff) << 8)
    #define LE32(x) (((uint32_t)x & 0xff000000) >> 24 | ((uint32_t)x & 0x00ff0000 >> 8) | ((uint32_t)x & 0x0000ff00) << 8 | ((uint32_t)x & 0x000000ff) << 24)
#else
    #define LE16(x) (x)
    #define LE32(x) (x)
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define CONVERT_SAMPLES 4096

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static ddb_converter_t plugin;
static DB_functions_t *deadbeef;

struct encoder_preset_link {
    ddb_encoder_preset_t preset;
    struct encoder_preset_link *next;
};
static struct encoder_preset_link *encoder_presets;

struct dsp_preset_link {
    ddb_dsp_preset_t preset;
    struct dsp_preset_link *next;
};
static struct dsp_preset_link *dsp_presets;

static int
check_dir(const char *path)
{
    struct stat stat_struct;
    if (!stat(path, &stat_struct)) {
        return S_ISDIR(stat_struct.st_mode);
    }
    if (errno != ENOENT) {
        return 0;
    }

    char* dir = strdup(path);
    if (!dir) {
        return 0;
    }

    const int good_dir = check_dir(dirname(dir));
    free(dir);
    return good_dir && !mkdir(path, 0755);
}

static int
ensure_dir(const char *path)
{
    char dir[strlen(path) + 1];
    strcpy(dir, path);
    dirname(dir);
    trace("converter: ensure folder %s exists\n", dir);
    return check_dir(dir);
}

static char *
preset_path(const char *dir, const char *subdir, const char *title)
{
    char *path = malloc(strlen(dir) + strlen(subdir) + strlen(title) + sizeof("%s/%s/%s.txt"));
    if (!path) {
        return NULL;
    }

    if (sprintf(path, "%s/%s/%s.txt", dir, subdir, title) < 0 || !ensure_dir(path)) {
        free(path);
        return NULL;
    }

    return path;
}

static ddb_encoder_preset_t *
encoder_preset_alloc (void)
{
    return (ddb_encoder_preset_t *)calloc(1, sizeof(struct encoder_preset_link));
}

static void
encoder_preset_free_strings(ddb_encoder_preset_t *p)
{
    if (p->title) {
        free(p->title);
    }
    if (p->extension) {
        free(p->extension);
    }
    if (p->encoder) {
        free(p->encoder);
    }
}

static void
encoder_preset_free (ddb_encoder_preset_t *p)
{
    encoder_preset_free_strings(p);
    free(p);
}

static ddb_encoder_preset_t *
encoder_preset_copy (ddb_encoder_preset_t *to, ddb_encoder_preset_t *from)
{
    char *p_title = strdup(from->title);
    char *p_extension = strdup(from->extension);
    char *p_encoder = strdup(from->encoder);
    if (!p_title || !p_extension || !p_encoder) {
        free(p_title);
        free(p_extension);
        free(p_encoder);
        return NULL;
    }

    encoder_preset_free_strings(to);
    to->title = p_title;
    to->extension = p_extension;
    to->encoder = p_encoder;
    to->method = from->method;
    to->tag_id3v2 = from->tag_id3v2;
    to->tag_id3v1 = from->tag_id3v1;
    to->tag_apev2 = from->tag_apev2;
    to->tag_flac = from->tag_flac;
    to->tag_oggvorbis = from->tag_oggvorbis;
    to->tag_mp3xing = from->tag_mp3xing;
    to->id3v2_version = from->id3v2_version;
    to->builtin = from->builtin;

    return to;
}

static ddb_encoder_preset_t *
encoder_preset_duplicate (ddb_encoder_preset_t *old)
{
    ddb_encoder_preset_t *new = encoder_preset_alloc();
    if (!new) {
        return NULL;
    }

    if (!encoder_preset_copy(new, old)) {
        encoder_preset_free(new);
        return NULL;
    }

    return new;
}

static ddb_encoder_preset_t *
encoder_preset_get(const char *title)
{
    for (struct encoder_preset_link *l = encoder_presets; l; l = l->next) {
        if (!strcoll(l->preset.title, title)) {
            return &l->preset;
        }
    }
    return NULL;
}

static ddb_encoder_preset_t *
encoder_preset_get_for_idx (int idx)
{
    for (struct encoder_preset_link *l = encoder_presets; l; l = l->next) {
        if (!idx--) {
            return &l->preset;
        }
    }
    return NULL;
}

static ddb_encoder_preset_t *
encoder_preset_get_next(const ddb_encoder_preset_t *p)
{
    struct encoder_preset_link *l = (struct encoder_preset_link *)p;
    return &l->next->preset;
}

static int
encoder_preset_get_idx(const char *title)
{
    struct encoder_preset_link *l = encoder_presets;
    for (size_t i = 0; l; l = l->next, i++) {
        if (!strcoll(l->preset.title, title)) {
            return i;
        }
    }
    return -1;
}

static char *
encoder_preset_path (const char *title)
{
    return preset_path(deadbeef->get_config_dir(), "presets/encoders", title);
}

static char *
encoder_preset_builtin_path (const char *title)
{
    char title_copy[strlen(title)+1];
    strcpy(title_copy, title);
    char *p = title_copy;
    while ((p = strchr(p, ' '))) {
        *p = '_';
    }
    return preset_path(deadbeef->get_plugin_dir(), "convpresets", title_copy);
}

static struct encoder_preset_link *
encoder_preset_create (const char *fname)
{
    FILE *fp = fopen(fname, "rt");
    if (!fp) {
        return NULL;
    }

    ddb_encoder_preset_t *p = encoder_preset_alloc();
    if (!p) {
        fclose (fp);
        return NULL;
    }

    char str[1024];
    while (fgets(str, sizeof(str), fp)) {
        char *cr = str + strlen(str) - 1;
        while (*cr == '\n') {
            cr--;
        }
        cr++;
        *cr = '\0';

        char *sp = strchr(str, ' ');
        if (sp) {
            *sp = '\0';
            const char *item = sp + 1;

            if (!strcmp(str, "title")) {
                p->title = strdup(item);
            }
            else if (!strcmp(str, "ext")) {
                p->extension = strdup(item);
            }
            else if (!strcmp(str, "encoder")) {
                p->encoder = strdup(item);
            }
            else if (!strcmp(str, "method")) {
                p->method = atoi(item);
            }
            else if (!strcmp(str, "id3v2_version")) {
                p->id3v2_version = atoi(item);
            }
            else if (!strcmp(str, "tag_id3v2")) {
                p->tag_id3v2 = atoi(item);
            }
            else if (!strcmp(str, "tag_id3v1")) {
                p->tag_id3v1 = atoi(item);
            }
            else if (!strcmp(str, "tag_apev2")) {
                p->tag_apev2 = atoi(item);
            }
            else if (!strcmp(str, "tag_flac")) {
                p->tag_flac = atoi(item);
            }
            else if (!strcmp(str, "tag_oggvorbis")) {
                p->tag_oggvorbis = atoi(item);
            }
        }
    }

    fclose (fp);

    if (!p->title || !p->extension || !p->encoder) {
        encoder_preset_free(p);
        return NULL;
    }

    return (struct encoder_preset_link *)p;
}

ddb_encoder_preset_t *
encoder_preset_load_builtin(const char *title)
{
    char *path = encoder_preset_builtin_path(title);
    if (!path) {
        return NULL;
    }

    struct encoder_preset_link *l = encoder_preset_create(path);
    free(path);

    return l ? &l->preset : NULL;
}

static void
encoder_preset_delete(ddb_encoder_preset_t *p)
{
    char *path = encoder_preset_path(p->title);
    if (path) {
        unlink(path);
        free(path);
    }
}

static ddb_encoder_preset_t *
encoder_preset_save (ddb_encoder_preset_t *p)
{
    if (!p->title || !p->title[0]) {
        trace(stderr, "encoder_preset_save: empty title\n");
        return NULL;
    }

    if (p->builtin == DDB_PRESET_BUILTIN) {
        encoder_preset_delete(p);
    }
    else {
        char *path = encoder_preset_path(p->title);
        if (!path) {
            return NULL;
        }

        FILE *fp = fopen(path, "w+b");
        free(path);
        if (!fp) {
            return NULL;
        }

        if (fprintf(fp, "title %s\n", p->title) < 0 ||
            fprintf(fp, "ext %s\n", p->extension) < 0 ||
            fprintf(fp, "encoder %s\n", p->encoder) < 0 ||
            fprintf(fp, "method %d\n", p->method) < 0 ||
            fprintf(fp, "id3v2_version %d\n", p->id3v2_version) < 0 ||
            fprintf(fp, "tag_id3v2 %d\n", p->tag_id3v2) < 0 ||
            fprintf(fp, "tag_id3v1 %d\n", p->tag_id3v1) < 0 ||
            fprintf(fp, "tag_apev2 %d\n", p->tag_apev2) < 0 ||
            fprintf(fp, "tag_flac %d\n", p->tag_flac) < 0 ||
            fprintf(fp, "tag_oggvorbis %d\n", p->tag_oggvorbis) < 0 ||
            fclose(fp)) {
            return NULL;
        }
    }

    ddb_encoder_preset_t *old = encoder_preset_get(p->title);
    if (old) {
        encoder_preset_copy(old, p);
        encoder_preset_free(p);
        return old;
    }
    else {
        p->builtin = DDB_PRESET_CUSTOM;
        struct encoder_preset_link *l = encoder_presets;
        struct encoder_preset_link *prev = NULL;
        while (l && l->preset.builtin == DDB_PRESET_CUSTOM && strcoll(l->preset.title, p->title) < 0) {
            prev = l;
            l = l->next;
        }

        struct encoder_preset_link *new_link = (struct encoder_preset_link *)p;
        new_link->next = l;
        if (prev) {
            prev->next = new_link;
        }
        else {
            encoder_presets = new_link;
        }
        return p;
    }
}

static void
encoder_preset_remove (ddb_encoder_preset_t *p)
{
    encoder_preset_delete(p);

    struct encoder_preset_link *old_link = (struct encoder_preset_link *)p;
    if (encoder_presets == old_link) {
        encoder_presets = old_link->next;
    }
    else {
        struct encoder_preset_link *l = encoder_presets;
        while (l->next != old_link) {
            l = l->next;
        }
        l->next = old_link->next;
    }

    encoder_preset_free(p);
}

static int
dirent_alphasort (const struct dirent **a, const struct dirent **b)
{
    const char *a_name = (*a)->d_name;
    const char *b_name = (*b)->d_name;
    char a_title[strlen(a_name)+1];
    char b_title[strlen(b_name)+1];
    strcpy(a_title, a_name);
    strcpy(b_title, b_name);
    *(strrchr(a_title, '.')) = '\0';
    *(strrchr(b_title, '.')) = '\0';
    return strcoll(b_title, a_title);
}

static int
scandir_preset_filter (const struct dirent *ent)
{
    char *ext = strrchr(ent->d_name, '.');
    if (ext && !strcasecmp(ext, ".txt")) {
        return 1;
    }
    return 0;
}

static ddb_encoder_preset_t *
duplicate_encoder_preset(ddb_encoder_preset_t *p)
{
    if (!p->builtin) {
        for (struct encoder_preset_link *l = encoder_presets; l; l = l->next) {
            if (l->preset.builtin && !strcoll(l->preset.title, p->title)) {
                return &l->preset;
            }
        }
    }
    return NULL;
}

static void
load_encoder_preset_dir(const char *path, const int builtin)
{
    struct dirent **namelist = NULL;
    const int n = scandir(path, &namelist, scandir_preset_filter, dirent_alphasort);
    for (int i = 0; i < n; i++) {
        char s[strlen(path) + strlen(namelist[i]->d_name) + 1];
        strcpy(s, path);
        strcat(s, namelist[i]->d_name);
        struct encoder_preset_link *l = encoder_preset_create(s);
        if (l) {
            l->preset.builtin = builtin;
            ddb_encoder_preset_t *duplicate = duplicate_encoder_preset(&l->preset);
            if (duplicate) {
                encoder_preset_copy(duplicate, &l->preset);
                duplicate->builtin = DDB_PRESET_MODIFIED;
                encoder_preset_free(&l->preset);
                l = NULL;
            }
            if (l) {
                l->next = encoder_presets;
                encoder_presets = l;
            }
        }
        free(namelist[i]);
    }
    free(namelist);
}

static int
load_encoder_presets (void)
{
    char syspath[sizeof("/convpresets/") + strlen(deadbeef->get_plugin_dir())];
    strcpy(syspath, deadbeef->get_plugin_dir());
    strcat(syspath, "/convpresets/");
    load_encoder_preset_dir(syspath, DDB_PRESET_BUILTIN);

    char path[sizeof("/presets/encoders/") + strlen(deadbeef->get_config_dir())];
    strcpy(path, deadbeef->get_config_dir());
    strcat(path, "/presets/encoders/");
    load_encoder_preset_dir(path, DDB_PRESET_CUSTOM);

    return 0;
}

static void
free_encoder_presets (void)
{
    struct encoder_preset_link *l = encoder_presets;
    while (l) {
        struct encoder_preset_link *next = l->next;
        encoder_preset_free(&l->preset);
        l = next;
    }
    encoder_presets = NULL;
}

static ddb_dsp_context_t *
dsp_plugin_duplicate(ddb_dsp_context_t *old)
{
    ddb_dsp_context_t *new = old->plugin->open();
    if (!new) {
        return NULL;
    }

    if (old->plugin->num_params) {
        const int n = old->plugin->num_params();
        for (int i = 0; i < n; i++) {
            char s[1000] = "";
            old->plugin->get_param(old, i, s, sizeof(s));
            new->plugin->set_param(new, i, s);
        }
    }

    return new;
}

static ddb_dsp_preset_t *
dsp_preset_alloc (void)
{
    return (ddb_dsp_preset_t *)calloc(1, sizeof(struct dsp_preset_link));
}

static void
dsp_preset_free (ddb_dsp_preset_t *p)
{
    if (p) {
        if (p->title) {
            free(p->title);
        }
        deadbeef->dsp_preset_free(p->chain);
        free(p);
    }
}

static ddb_dsp_preset_t *
dsp_preset_copy (ddb_dsp_preset_t *to, ddb_dsp_preset_t *from)
{
    char *p_title = strdup(from->title);
    if (!p_title) {
        free(p_title);
        return NULL;
    }

    to->title = p_title;
    to->chain = NULL;
    ddb_dsp_context_t *tail = NULL;
    for (ddb_dsp_context_t *dsp = from->chain; dsp; dsp = dsp->next) {
        ddb_dsp_context_t *new = dsp_plugin_duplicate(dsp);
        if (new) {
            if (tail) {
                tail->next = new;
            }
            else {
                to->chain = new;
            }
            tail = new;
        }
    }

    return to;
}

static ddb_dsp_preset_t *
dsp_preset_duplicate (ddb_dsp_preset_t *old)
{
    ddb_dsp_preset_t *new = dsp_preset_alloc();
    if (!new) {
        return NULL;
    }

    new = dsp_preset_copy(new, old);
    if (new) {
        return new;
    }

    dsp_preset_free(new);
    return NULL;
}

static ddb_dsp_preset_t *
dsp_preset_get(const char *title)
{
    for (struct dsp_preset_link *l = dsp_presets; l; l = l->next) {
        if (!strcoll(l->preset.title, title)) {
            return &l->preset;
        }
    }
    return NULL;
}

static int
dsp_preset_get_idx(const char *title)
{
    struct dsp_preset_link *l = dsp_presets;
    for (size_t i = 0; l; l = l->next, i++) {
        if (!strcoll(l->preset.title, title)) {
            return i;
        }
    }
    return -1;
}

static ddb_dsp_preset_t *
dsp_preset_get_next(const ddb_dsp_preset_t *p)
{
    struct dsp_preset_link *l = (struct dsp_preset_link *)p;
    return &l->next->preset;
}

static ddb_dsp_preset_t *
dsp_preset_get_for_idx (int idx)
{
    for (struct dsp_preset_link *l = dsp_presets; l; l = l->next) {
        if (!idx--) {
            return &l->preset;
        }
    }
    return NULL;
}

static char *
dsp_preset_path (const char *title)
{
    return preset_path(deadbeef->get_config_dir(), "presets/dsp", title);
}

static ddb_dsp_preset_t *
dsp_preset_load (const char *fname)
{
    ddb_dsp_preset_t *p = dsp_preset_alloc();
    if (!p) {
        return NULL;
    }

    const char *slash = strrchr(fname, '/');
    const char *start = slash ? slash+1 : fname;
    const char *end = strrchr(fname, '.');
    if (!end) {
        end = fname + strlen(fname);
    }
    const size_t title_length = end-start;
    p->title = malloc(title_length+1);
    if (!p->title) {
        dsp_preset_free(p);
        return NULL;
    }
    strncpy(p->title, start, title_length);
    p->title[title_length] = '\0';

    int err = deadbeef->dsp_preset_load(fname, &p->chain);
    if (err != 0) {
        dsp_preset_free(p);
        return NULL;
    }
    return p;
}

static ddb_dsp_preset_t *
dsp_preset_save (ddb_dsp_preset_t *p)
{
    if (!p->title || !p->title[0]) {
        trace(stderr, "dsp_preset_save: empty title\n");
        return NULL;
    }

    char *path = dsp_preset_path(p->title);
    if (!path) {
        return NULL;
    }

    const int res = deadbeef->dsp_preset_save(path, p->chain);
    free(path);
    if (res) {
        return NULL;
    }

    ddb_dsp_preset_t *old = dsp_preset_get(p->title);
    if (old) {
        dsp_preset_copy(old, p);
        dsp_preset_free(p);
        return old;
    }
    else {
        struct dsp_preset_link *l = dsp_presets;
        struct dsp_preset_link *prev = NULL;
        while (l && strcoll(l->preset.title, p->title) < 0) {
            prev = l;
            l = l->next;
        }

        struct dsp_preset_link *new_link = (struct dsp_preset_link *)p;
        new_link->next = l;
        if (prev) {
            prev->next = new_link;
        }
        else {
            dsp_presets = new_link;
        }
        return p;
    }
}

static int
load_dsp_presets (void)
{
    struct dsp_preset_link *tail = NULL;
    char path[sizeof("/presets/dsp/") + strlen(deadbeef->get_config_dir())];
    strcpy(path, deadbeef->get_config_dir());
    strcat(path, "/presets/dsp/");
    struct dirent **namelist = NULL;
    int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
    for (int i = 0; i < n; i++) {
        char s[strlen(path) + strlen(namelist[i]->d_name) + 1];
        strcpy(s, path);
        strcat(s, namelist[i]->d_name);
        ddb_dsp_preset_t *p = dsp_preset_load(s);
        struct dsp_preset_link *l = (struct dsp_preset_link *)p;
        if (l) {
            l->next = dsp_presets;
            dsp_presets = l;
        }
        free(namelist[i]);
    }
    free (namelist);
    return 0;
}

static void
free_dsp_presets (void)
{
    struct dsp_preset_link *l = dsp_presets;
    while (l) {
        struct dsp_preset_link *next = l->next;
        dsp_preset_free(&l->preset);
        l = next;
    }
    dsp_presets = NULL;
}

static void
dsp_preset_remove (ddb_dsp_preset_t *p)
{
    char *path = dsp_preset_path(p->title);
    if (path) {
        unlink(path);
        free(path);
    }

    struct dsp_preset_link *old_link = (struct dsp_preset_link *)p;
    if (dsp_presets == old_link) {
        dsp_presets = old_link->next;
    }
    else {
        struct dsp_preset_link *l = dsp_presets;
        while (l->next != old_link) {
            l = l->next;
        }
        l->next = old_link->next;
    }

    dsp_preset_free(p);
}

static void
get_output_path (DB_playItem_t *it, const ddb_encoder_preset_t *encoder_preset, const char *rootfolder, const char *outfolder_user, const char *outfile, const int use_source_folder, char *out, int sz)
{
    trace("get_output_path: %s %s %s\n", outfolder_user, outfile, rootfolder);

    sz -= strlen(encoder_preset->extension) + 1;

    if (use_source_folder) {
        deadbeef->pl_lock();
        const char *uri = deadbeef->pl_find_meta(it, ":URI");
        const char *sep = strrchr(uri, '/');
        if (sep) {
            snprintf(out, sz, "%.*s/", sep-uri, uri);
        }
        deadbeef->pl_unlock();
    }
    else if (rootfolder) {
        deadbeef->pl_lock();
        const char *subpath_start = deadbeef->pl_find_meta(it, ":URI") + strlen(rootfolder);
        const char *subpath_end = strrchr(subpath_start, '/');
        if (subpath_end) {
            const size_t subpath_length = subpath_end - subpath_start;
            const char *outfolder = outfolder_user[0] ? outfolder_user : getenv("HOME");
            snprintf(out, sz, "%s%.*s/", outfolder, subpath_length, subpath_start);
        }
        deadbeef->pl_unlock();
    }
    else {
        snprintf(out, sz, "%s/", outfolder_user);
    }

    const size_t l = strlen(out);
    deadbeef->pl_format_title(it, -1, out+l, sz-l, -1, outfile);
    strcat(out, ".");
    strcat(out, encoder_preset->extension);
    trace("converter output file is '%s'\n", out);
}

static char *
get_root_folder(DB_playItem_t **items)
{
    // start with the 1st track path
    deadbeef->pl_lock();
    const char *uri = deadbeef->pl_find_meta(items[0], ":URI");
    char *root = uri ? strdup(uri) : NULL;

    if (root) {
        char *sep = strrchr(root, '/');
        if (sep) {
            *sep = '\0';
        }

        // reduce
        size_t rootlen = strlen(root);
        while (*(++items)) {
            const char *path = deadbeef->pl_find_meta(*items, ":URI");
            if (strncmp(path, root, rootlen)) {
                // find where path splits
                char *r = root;
                while (*path && *r) {
                    if (*path != *r) {
                        // find new separator
                        while (r > root && *r != '/') {
                            r--;
                        }
                        *r = '\0';
                        rootlen = r-root;
                        break;
                    }
                    path++;
                    r++;
                }
            }
        }
    }
    deadbeef->pl_unlock();

    return root;
}

static char *
escape_filepath (const char *path, char *escaped_path)
{
    char *out = escaped_path;
    *out++ = '"';
    while (*path) {
        if (strchr("$\"`\\", *path)) {
            *out++ = '\\';
        }
        *out++ = *path++;
    }
    *out++ = '"';
    *out = '\0';
    return escaped_path;
}

static size_t
ensure_string(char **old, const size_t old_size, const size_t old_len, const size_t increment)
{
    if (old_size > old_len + increment) {
        return old_size;
    }

    const size_t new_size = old_len + increment + 32;
    char *new = realloc(*old, new_size);
    if (!new) {
        free(*old);
        return 0;
    }

    *old = new;
    return new_size;
}

static char *
encoder_command(const char *encoder_pattern, const char *in_path, const char *out_path)
{
    char *command = NULL;
    size_t len = 0;
    size_t size = 0;
    const char *e = encoder_pattern;
    do {
        if (e[0] == '%' && (e[1] == 'i' || e[1] == 'o')) {
            char escaped[max(in_path ? strlen(in_path) : 0, strlen(out_path))*2+3];
            const char *sub = e[1] == 'o' ? escape_filepath(out_path, escaped) : in_path ? escape_filepath(in_path, escaped) : "-";
            const size_t inc = strlen(sub);
            if (!(size = ensure_string(&command, size, len, inc))) {
                return NULL;
            }
            strcpy(command+len, sub);
            len += inc;
            e += 2;
        }
        else {
            if (!(size = ensure_string(&command, size, len, 2))) {
                return NULL;
            }
            command[len] = *e++;
            len++;
        }
    } while (*e);
    command[len] = '\0';
    return command;
}

static char *
encoder_temp_path(void)
{
    const char *dir = getenv("TMPDIR");
    if (!dir) {
        dir = P_tmpdir;
    }

    char *path = malloc(sizeof("/ddbconvXXXXXX.wav") + strlen(dir));
    if (path) {
        strcpy(path, dir);
        strcat(path, "/ddbconvXXXXXX");
        mktemp(path);
        strcat(path, ".wav");
    }
    return path;
}

static int
convert_file (DB_playItem_t *it, const ddb_encoder_preset_t *encoder_preset, const char *out, const ddb_dsp_preset_t *dsp_preset, const int output_bps, const int output_is_float,
              enum ddb_convert_api *api, char **message, void (* convert_callback)(const time_t, const time_t, const float, void *), void *user_data)
{
    void *read_buffer = NULL;
    void *write_buffer = NULL;
    void *dsp_buffer = NULL;
    FILE *output = NULL;
    DB_fileinfo_t *fileinfo = NULL;
    ddb_dsp_context_t *dsp_chain = NULL;
    char *command = NULL;
    char *temp_file_path = NULL;

    int err = -1;
    const time_t start_time = time(NULL);

    deadbeef->pl_lock();
    DB_decoder_t *decoder = (DB_decoder_t *)deadbeef->plug_get_for_id(deadbeef->pl_find_meta(it, ":DECODER"));
    deadbeef->pl_unlock ();
    if (!decoder || !(fileinfo = decoder->open(0)) || decoder->init(fileinfo, DB_PLAYITEM(it))) {
        *message = "Cannot decode source file";
        trace("converter: failed to decode file %s\n", deadbeef->pl_find_meta(it, ":URI"));
        goto error;
    }

    ddb_waveformat_t outfmt;
    memcpy(&outfmt, &fileinfo->fmt, sizeof(ddb_waveformat_t));
    if (output_bps > 0) {
        outfmt.bps = output_bps;
        outfmt.is_float = output_is_float;
    }

    if (encoder_preset->encoder && *encoder_preset->encoder) {
        if (encoder_preset->method == DDB_ENCODER_METHOD_FILE && !(temp_file_path = encoder_temp_path())) {
            goto error;
        }
        if (!(command = encoder_command(encoder_preset->encoder, temp_file_path, out))) {
            goto error;
        }
        fprintf(stderr, "Converter command: %s\n", command);
        if (temp_file_path) {
            FILE *temp = fopen(temp_file_path, "w+b");
            if (!temp) {
                *message = "Cannot open temporary file";
                trace("converter: failed to open temp file %s\n", temp_file_path);
                goto error;
            }
            output = temp;
        }
        else {
            FILE *enc_pipe = popen(command, "w");
            if (!enc_pipe) {
                *message = "Encoder command failed";
                trace("converter: encoder command failed\n");
                goto error;
            }
            output = enc_pipe;
        }
    }
    else {
        fprintf(stderr, "Converter will encode using internal RIFF WAVE writer\n");
        FILE *wav = fopen(out, "w+b");
        if (!wav) {
            *message = "Cannot open output wave file";
            trace("converter: failed to open output wave file %s\n", out);
            goto error;
        }
        output = wav;
    }
    setbuf(output, NULL);

    uint8_t wavehdr_pcm[] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',0x10,0x00,0x00,0x00,0x01,0x00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'d','a','t','a',0,0,0,0};
    uint8_t wavehdr_ext[] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',0x28,0x00,0x00,0x00,0xfe,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x16,0x00,0,0,0x03,0x00,0x00,0x00,0,0,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71,'f','a','c','t',0x04,0x00,0x00,0x00,0,0,0,0,'d','a','t','a',0,0,0,0};
    uint8_t *wavehdr = outfmt.is_float || outfmt.channels > 2 ? wavehdr_ext : wavehdr_pcm;
    const size_t wavehdr_size = wavehdr == wavehdr_pcm ? sizeof(wavehdr_pcm) : sizeof(wavehdr_ext);

    const int readsize = CONVERT_SAMPLES * fileinfo->fmt.channels * fileinfo->fmt.bps / 8;
    read_buffer = malloc(readsize);
    if (dsp_preset && dsp_preset->chain) {
        dsp_chain = dsp_plugin_duplicate(dsp_preset->chain);
        const size_t dspsize = CONVERT_SAMPLES * ceil((double)384000 / fileinfo->fmt.samplerate) * 8 * 32 / 8;
        write_buffer = malloc(dspsize);
        dsp_buffer = malloc(dspsize);
        if (!dsp_buffer) {
            goto error;
        }
    }
    else if (fileinfo->fmt.bps != outfmt.bps || fileinfo->fmt.is_float != outfmt.is_float) {
        const size_t writesize = CONVERT_SAMPLES * outfmt.channels * outfmt.bps / 8;
        write_buffer = malloc(writesize);
    }
    else {
        write_buffer = read_buffer;
    }
    if (!write_buffer) {
        goto error;
    }

    const size_t callback_interval = deadbeef->pl_get_item_duration(it) * fileinfo->fmt.samplerate / 15;
    size_t samples = 0;
    size_t next_sample = 0;
    time_t next_time = start_time + 1;

    off_t outsize = 0;
    int sz = decoder->read(fileinfo, read_buffer, readsize);
    while (*api == DDB_CONVERT_API_CONTINUE && sz > 0) {
        if (dsp_chain) {
            ddb_waveformat_t dspfmt;
            memcpy(&dspfmt, &fileinfo->fmt, sizeof(ddb_waveformat_t));
            dspfmt.bps = 32;
            dspfmt.is_float = 1;
            sz = deadbeef->pcm_convert(&fileinfo->fmt, read_buffer, &dspfmt, dsp_buffer, sz);

            int frames = CONVERT_SAMPLES;
            for (ddb_dsp_context_t *dsp = dsp_chain; dsp; dsp = dsp->next) {
                frames = dsp->plugin->process(dsp, dsp_buffer, frames, -1, &dspfmt, NULL);
                if (frames <= 0) {
                    *message = "DSP error, please check your preset";
                    trace("converter: DSP error, please check your preset\n");
                    goto error;
                }
            }

            outfmt.channels = dspfmt.channels;
            outfmt.samplerate = dspfmt.samplerate;
            sz = deadbeef->pcm_convert(&dspfmt, dsp_buffer, &outfmt, write_buffer, frames*dspfmt.channels*32/8);
        }
        else if (write_buffer != read_buffer) {
            sz = deadbeef->pcm_convert(&fileinfo->fmt, read_buffer, &outfmt, write_buffer, sz);
        }

        if (!outsize) {
            *(uint16_t *)(wavehdr+22) = LE16(outfmt.channels);
            *(uint32_t *)(wavehdr+24) = LE32(outfmt.samplerate);
            const uint16_t blockalign = outfmt.channels * outfmt.bps / 8;
            *(uint32_t *)(wavehdr+28) = LE32(outfmt.samplerate * blockalign);
            *(uint16_t *)(wavehdr+32) = LE16(blockalign);
            *(uint16_t *)(wavehdr+34) = LE16(outfmt.bps);
            if (wavehdr == wavehdr_ext) {
                *(uint16_t *)(wavehdr+38) = LE16(outfmt.bps);
                *(uint16_t *)(wavehdr+44) = LE16(outfmt.is_float ? 3 : 1);
            }
            if (fwrite(wavehdr, wavehdr_size, 1, output) != 1) {
                *message = "Wave header write error";
                trace("converter: wave header write error\n");
                goto error;
            }
        }

#if WORDS_BIGENDIAN
        uint8_t *swap_buffer = write_buffer;
        const size_t bytes = outfmt.bps / 8;
        for (size_t i = 0; i < readsize; i += bytes) {
            for (size_t j = 0; j < bytes; j++) {
                swap_buffer[i+bytes-j-1] = swap_buffer[i+j];
            }
        }
#endif
        const time_t now = time(NULL);
        if (samples >= next_sample || now >= next_time) {
            convert_callback(start_time, now, temp_file_path ? -15 : (float)samples/fileinfo->fmt.samplerate, user_data);
            next_sample += callback_interval;
            next_time++;
        }
        if (!temp_file_path) {
            samples += CONVERT_SAMPLES;
        }

        const size_t res = fwrite(write_buffer, 1, sz, output);
        if (res != sz) {
            *message = temp_file_path ? "Error writing temporary file" : "Error sending data to encoder";
            trace("converter: write error %d (%zu bytes written out of %d)\n", errno, res, sz);
            goto error;
        }
        outsize += sz;

        sz = decoder->read(fileinfo, read_buffer, readsize);
    }

    if (temp_file_path || !command) {
        *(uint32_t *)(wavehdr+4), LE32(min(outsize+wavehdr_size-4, 0xffffffff));
        if (wavehdr == wavehdr_ext) {
            *(uint32_t *)(wavehdr+68) = LE32(outsize / outfmt.channels / outfmt.bps * 8);
        }
        *(uint32_t *)(wavehdr+wavehdr_size-4) = LE32(min(outsize, 0xffffffff));
        fseek(output, 0, SEEK_SET);
        const int res = fwrite(wavehdr, wavehdr_size, 1, output) != 1 || fclose(output);
        output = NULL;
        if (res) {
            *message = "Wave header rewrite error";
            trace("converter: wave header rewrite failed\n");
            goto error;
        }
    }

    if (*api != DDB_CONVERT_API_CONTINUE) {
        goto error;
    }

    if (temp_file_path) {
        int pipefd[2];
        if (pipe(pipefd)) {
            goto error;
        }
        close(pipefd[1]);
        pid_t pid = fork();
        if (!pid) {
            dup2(pipefd[0], STDIN_FILENO);
            setpgid(0, 0);
            execlp("sh", "sh", "-c", command, NULL);
            _exit(127);
        }
        close(pipefd[0]);
        if (pid < 0) {
            goto error;
        }
        int status = 0;
        while (waitpid(pid, &status, WNOHANG) == 0 || !WIFEXITED(status) && !WIFSIGNALED(status)) {
            if (*api == DDB_CONVERT_API_ABORT) {
                kill(pid*-1, SIGTERM);
            }
            convert_callback(start_time, time(NULL), -33, user_data);
            usleep(100000);
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) || WIFSIGNALED(status)) {
            *message = "Encoder command failed";
            goto error;
        }
    }
    else if (output) {
        const int res = pclose(output);
        output = NULL;
        if (res) {
            *message = "Encoder command failed";
            trace("converter: encoder command failed\n");
            goto error;
        }
    }

    convert_callback(start_time, time(NULL), deadbeef->pl_get_item_duration(it), user_data);

    err = 0;
error:
    if (command) {
        free(command);
    }
    if (write_buffer != read_buffer) {
        free(write_buffer);
    }
    if (dsp_buffer) {
        free(dsp_buffer);
    }
    if (output) {
        temp_file_path || !command ? fclose(output) : pclose(output);
    }
    if (fileinfo) {
        decoder->free(fileinfo);
    }
    if (dsp_chain) {
        deadbeef->dsp_preset_free(dsp_chain);
    }
    if (temp_file_path) {
        unlink(temp_file_path);
        free(temp_file_path);
    }

    return err;
}

static void
convert_tags (DB_playItem_t *it, const ddb_encoder_preset_t *encoder_preset, const char *out, char **message)
{
    if (!(encoder_preset->tag_id3v2 || encoder_preset->tag_id3v1 || encoder_preset->tag_apev2 || encoder_preset->tag_flac || encoder_preset->tag_oggvorbis)) {
        trace("converter: no tag type selected\n");
        return;
    }

    DB_playItem_t *out_it = deadbeef->pl_item_alloc();
    if (!out_it) {
        return;
    }

    uint32_t tagflags = 0;
    deadbeef->pl_add_meta(out_it, ":URI", out);

    DB_FILE *fp = deadbeef->fopen(out);
    if (fp) {
        if (encoder_preset->tag_id3v2) {
            deadbeef->junk_id3v2_read(out_it, fp);
            tagflags |= JUNK_WRITE_ID3V2;
        }
        if (encoder_preset->tag_id3v1) {
            deadbeef->junk_id3v1_read(out_it, fp);
            tagflags |= JUNK_WRITE_ID3V1;
        }
        if (encoder_preset->tag_apev2) {
            deadbeef->junk_apev2_read(out_it, fp);
            tagflags |= JUNK_WRITE_APEV2;
        }
        deadbeef->fclose(fp);
    }

    DB_decoder_t *flac_plugin = NULL;
    if (encoder_preset->tag_flac) {
        for (DB_decoder_t **plug = deadbeef->plug_get_decoder_list(); *plug && !flac_plugin; plug++) {
            if (!strcmp((*plug)->plugin.id, "stdflac")) {
                flac_plugin = *plug;
            }
        }
        if (!flac_plugin || flac_plugin->read_metadata(out_it)) {
            *message = "Cannot read FLAC tags";
            trace("converter: failed to read flac metadata\n");
        }
    }

    DB_decoder_t *ogg_plugin = NULL;
    if (encoder_preset->tag_oggvorbis) {
        for (DB_decoder_t **plug = deadbeef->plug_get_decoder_list(); *plug && !ogg_plugin; plug++) {
            if (!strcmp((*plug)->plugin.id, "stdogg") || !strcmp((*plug)->plugin.id, "stdopus") && !(*plug)->read_metadata(out_it)) {
                ogg_plugin = *plug;
            }
        }
        if (!ogg_plugin) {
            *message = "Cannot read Vorbis comment tags";
            trace("converter: failed to read vorbiscomment metadata\n");
        }
    }

    deadbeef->pl_lock();
    DB_metaInfo_t *m = deadbeef->pl_get_metadata_head(it);
    while (m) {
        DB_metaInfo_t *next = m->next;
        if (m->key[0] != ':' && m->key[0] != '!' && !deadbeef->pl_meta_exists(out_it, m->key)) {
            deadbeef->pl_add_meta(out_it, m->key, m->value);
        }
        m = next;
    }
    deadbeef->pl_unlock();

    if (tagflags && deadbeef->junk_rewrite_tags(out_it, tagflags, encoder_preset->id3v2_version + 3, "iso8859-1")) {
        *message = "Cannot write tags";
        trace("converter: failed to write metadata\n");
    }

    if (flac_plugin) {
        if (flac_plugin->write_metadata(out_it)) {
            *message = "Cannot write FLAC tags";
            trace("converter: failed to write flac metadata, not a flac file?\n");
        }
    }

    if (ogg_plugin) {
        if (ogg_plugin->write_metadata(out_it)) {
            *message = "Cannot write Vorbis comment tags";
            trace("converter: failed to write vorbiscomment metadata, not an ogg file?\n");
        }
    }

    if (out_it) {
        deadbeef->pl_item_unref(out_it);
    }
}

static int
convert (DB_playItem_t *it, const ddb_encoder_preset_t *encoder_preset, const char *out, const ddb_dsp_preset_t *dsp_preset, const int output_bps, const int output_is_float,
         enum ddb_convert_api *api, char **message, void (* convert_callback)(const time_t, const time_t, const float, void *), void *user_data)
{
#ifndef __STDC_IEC_559__
    if (output_is_float) {
        fprintf(stderr, "converter: __STDC_IEC_559__ macro not defined, float32 WAVE output may not be valid\n");
    }
#endif

    if (deadbeef->pl_get_item_duration(it) <= 0) {
        deadbeef->pl_lock();
        *message = "Stream does not have finite length, not currently supported";
        trace("converter: stream %s doesn't have finite length, skipped\n", deadbeef->pl_find_meta(it, ":URI"));
        deadbeef->pl_unlock();
        return -1;
    }

    if (!ensure_dir(out)) {
        *message = "Cannot create output folder";
        trace("converter: failed to create output folder: %s\n", out);
        return -1;
    }

    if (!convert_file(it, encoder_preset, out, dsp_preset, output_bps, output_is_float, api, message, convert_callback, user_data)) {
        convert_tags(it, encoder_preset, out, message);
        if (*api == DDB_CONVERT_API_CONTINUE) {
            return 0;
        }
    }

    unlink(out);
    return -1;
}

static void
load_presets(void)
{
    load_encoder_presets();
    load_dsp_presets();
}

static void
unload_presets(void)
{
    free_encoder_presets();
    free_dsp_presets();
}

static ddb_converter_t plugin =
{
    .misc.plugin.api_vmajor = 1,
    .misc.plugin.api_vminor = 0,
    .misc.plugin.version_major = 2,
    .misc.plugin.version_minor = 0,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.name = "Converter",
    .misc.plugin.id = "converter",
    .misc.plugin.descr = "Converts any supported formats to other formats.\n"
        "Requires separate GUI plugin, e.g. Converter GTK UI\n",
    .misc.plugin.copyright =
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .misc.plugin.website = "http://deadbeef.sf.net",
    .encoder_preset_alloc = encoder_preset_alloc,
    .encoder_preset_free = encoder_preset_free,
    .encoder_preset_duplicate = encoder_preset_duplicate,
    .encoder_preset_get = encoder_preset_get,
    .encoder_preset_get_for_idx = encoder_preset_get_for_idx,
    .encoder_preset_get_next = encoder_preset_get_next,
    .encoder_preset_get_idx = encoder_preset_get_idx,
    .encoder_preset_load_builtin = encoder_preset_load_builtin,
    .encoder_preset_save = encoder_preset_save,
    .encoder_preset_remove = encoder_preset_remove,
    .dsp_plugin_duplicate = dsp_plugin_duplicate,
    .dsp_preset_alloc = dsp_preset_alloc,
    .dsp_preset_free = dsp_preset_free,
    .dsp_preset_duplicate = dsp_preset_duplicate,
    .dsp_preset_get = dsp_preset_get,
    .dsp_preset_get_for_idx = dsp_preset_get_for_idx,
    .dsp_preset_get_next = dsp_preset_get_next,
    .dsp_preset_get_idx = dsp_preset_get_idx,
    .dsp_preset_save = dsp_preset_save,
    .dsp_preset_remove = dsp_preset_remove,
    .convert = convert,
    .get_output_path = get_output_path,
    .get_root_folder = get_root_folder,
    .load = load_presets,
    .unload = unload_presets,
};

DB_plugin_t *
converter_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
