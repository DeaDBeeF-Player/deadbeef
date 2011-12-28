/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "converter.h"
#include "../../deadbeef.h"

#ifndef PATH_MAX
#define PATH_MAX    1024    /* max # of characters in a path name */
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
    char path[1024];
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
    char path[1024];
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
    char ppath[1024];
    char epath[1024];
    char fpath[1024];
    snprintf (ppath, sizeof (ppath), "%s/presets", deadbeef->get_config_dir ());
    snprintf (epath, sizeof (epath), "%s/encoders", ppath);
    snprintf (fpath, sizeof (fpath), "%s/.installed", epath);
    struct stat stat_buf;
    if (0 != stat (fpath, &stat_buf)) {
        // file not found, install all presets from plugin_dir/convpresets/
        mkdir (ppath, 0755);
        mkdir (epath, 0755);
        char preset_src_dir[1024];
        snprintf (preset_src_dir, sizeof (preset_src_dir), "%s/convpresets", deadbeef->get_plugin_dir ());
        struct dirent **namelist = NULL;
        int n = scandir (preset_src_dir, &namelist, NULL, dirent_alphasort);
        for (int i = 0; i < n; i++) {
            // replace _ with spaces
            char new_name[1024];
            char *o = new_name;
            char *in = namelist[i]->d_name;
            while (*in) {
                if (*in == '_') {
                    *o++ = ' ';
                    in++;
                }
                else {
                    *o++ = *in++;
                }
            }
            *o = 0;
            char in_name[1024];
            char out_name[1024];
            snprintf (in_name, sizeof (in_name), "%s/%s", preset_src_dir, namelist[i]->d_name);
            snprintf (out_name, sizeof (out_name), "%s/%s", epath, new_name);
            copy_file (in_name, out_name);
            free (namelist[i]);
        }
        if (namelist) {
            free (namelist);
        }
        FILE *fp = fopen (fpath, "w+b");
        if (fp) {
            fclose (fp);
        }
    }

    ddb_encoder_preset_t *tail = NULL;
    char path[1024];
    if (snprintf (path, sizeof (path), "%s/presets/encoders", deadbeef->get_config_dir ()) < 0) {
        return -1;
    }
    struct dirent **namelist = NULL;
    int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
    int i;
    for (i = 0; i < n; i++) {
        char s[1024];
        if (snprintf (s, sizeof (s), "%s/%s", path, namelist[i]->d_name) > 0){
            ddb_encoder_preset_t *p = encoder_preset_load (s);
            if (p) {
                if (tail) {
                    tail->next = p;
                    tail = p;
                }
                else {
                    encoder_presets = tail = p;
                }
            }
        }
        free (namelist[i]);
    }
    free (namelist);
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
    char path[1024];
    if (snprintf (path, sizeof (path), "%s/presets/dsp", deadbeef->get_config_dir ()) < 0) {
        return -1;
    }
    struct dirent **namelist = NULL;
    int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
    int i;
    for (i = 0; i < n; i++) {
        char s[1024];
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
get_output_field (DB_playItem_t *it, const char *field, char *out, int sz)
{
    int idx = deadbeef->pl_get_idx_of (it);
    deadbeef->pl_format_title (it, idx, out, sz, -1, field);

    // replace invalid chars
    char invalid[] = "/\\?%*:|\"<>";
    char *p = out;
    while (*p) {
        if (strchr (invalid, *p)) {
            *p = '_';
        }
        p++;
    }
    trace ("field '%s' expanded to '%s'\n", field, out);
}

static void
get_output_path (DB_playItem_t *it, const char *outfolder, const char *outfile, ddb_encoder_preset_t *encoder_preset, char *out, int sz) {
    int l;
    char fname[PATH_MAX];
    char *path = outfolder[0] ? strdupa (outfolder) : strdupa (getenv("HOME"));
    char *pattern = strdupa (outfile);

    // replace invalid chars
    char invalid[] = "?%*:|\"<>";
    char *p = path;
    while (*p) {
        if (strchr (invalid, *p)) {
            *p = '_';
        }
        p++;
    }
    snprintf (out, sz, "%s/", path);

    // split path and create directories
    char *field = pattern;
    char *s = pattern;
    while (*s) {
        if ((*s == '/') || (*s == '\\')) {
            *s = '\0';
            get_output_field (it, field, fname, sizeof(fname));

            l = strlen (out);
            snprintf (out+l, sz-l, "%s/", fname);
            mkdir (out, 0755);

            field = s+1;
        }
        s++;
    }

    // last part of outfile is the filename
    get_output_field (it, field, fname, sizeof(fname));

    l = strlen (out);
    snprintf (out+l, sz-l, "%s.%s", fname, encoder_preset->ext);
    trace ("converter output file is '%s'\n", out);
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
                trace ("Failed to create %s (%d)\n", tmp, errno);
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

int
convert (DB_playItem_t *it, const char *outfolder, const char *outfile, int output_bps, int output_is_float, int preserve_folder_structure, const char *root_folder, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort) {
    if (deadbeef->pl_get_item_duration (it) <= 0) {
        deadbeef->pl_lock ();
        const char *fname = deadbeef->pl_find_meta (it, ":URI");
        fprintf (stderr, "converter: stream %s doesn't have finite length, skipped\n", fname);
        deadbeef->pl_unlock ();
        return -1;
    }

    char *path = outfolder[0] ? strdupa (outfolder) : strdupa (getenv("HOME"));
    if (!check_dir (path, 0755)) {
        fprintf (stderr, "converter: failed to create output folder: %s\n", outfolder);
        return -1;
    }

    int err = -1;
    FILE *enc_pipe = NULL;
    FILE *temp_file = NULL;
    DB_decoder_t *dec = NULL;
    DB_fileinfo_t *fileinfo = NULL;
    char out[PATH_MAX] = ""; // full path to output file
    char input_file_name[PATH_MAX] = "";
    dec = (DB_decoder_t *)deadbeef->plug_get_for_id (deadbeef->pl_find_meta (it, ":DECODER"));

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

            get_output_path (it, outfolder, outfile, encoder_preset, out, sizeof (out));
            if (encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                const char *tmp = getenv ("TMPDIR");
                if (!tmp) {
                    tmp = "/tmp";
                }
                snprintf (input_file_name, sizeof (input_file_name), "%s/ddbconvXXXXXX", tmp);
                mktemp (input_file_name);
                strcat (input_file_name, ".wav");
            }
            else {
                strcpy (input_file_name, "-");
            }

            char enc[2000];
            memset (enc, 0, sizeof (enc));

            // formatting: %o = outfile, %i = infile
            char *e = encoder_preset->encoder;
            char *o = enc;
            *o = 0;
            int len = sizeof (enc);
            while (e && *e) {
                if (len <= 0) {
                    fprintf (stderr, "converter: failed to assemble encoder command line - buffer is not big enough, try to shorten your parameters. max allowed length is %u characters\n", (unsigned)sizeof (enc));
                    goto error;
                }
                if (e[0] == '%' && e[1]) {
                    if (e[1] == 'o') {
                        int l = snprintf (o, len, "\"%s\"", out);
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

            if (!encoder_preset->encoder[0]) {
                // write to wave file
                temp_file = fopen (out, "w+b");
                if (!temp_file) {
                    fprintf (stderr, "converter: failed to open output wave file %s\n", out);
                    goto error;
                }
            }
            else if (encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                temp_file = fopen (input_file_name, "w+b");
                if (!temp_file) {
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

            if (!temp_file) {
                temp_file = enc_pipe;
            }

            // write wave header
            char wavehdr_int[] = {
                0x52, 0x49, 0x46, 0x46, 0x24, 0x70, 0x0d, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xac, 0x00, 0x00, 0x10, 0xb1, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61
            };
            char wavehdr_float[] = {
                0x52, 0x49, 0x46, 0x46, 0x2a, 0xdf, 0x02, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x28, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x02, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x08, 0x00, 0x20, 0x00, 0x16, 0x00, 0x20, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71, 0x66, 0x61, 0x63, 0x74, 0x04, 0x00, 0x00, 0x00, 0xc5, 0x5b, 0x00, 0x00, 0x64, 0x61, 0x74, 0x61
            };
            char *wavehdr = output_is_float ? wavehdr_float : wavehdr_int;
            int wavehdr_size = output_is_float ? sizeof (wavehdr_float) : sizeof (wavehdr_int);
            int header_written = 0;
            uint32_t outsize = 0;
            uint32_t outsr = fileinfo->fmt.samplerate;
            uint16_t outch = fileinfo->fmt.channels;

            int samplesize = fileinfo->fmt.channels * fileinfo->fmt.bps / 8;
            int bs = 10250 * samplesize;
            char buffer[bs * 4];
            int dspsize = bs / samplesize * sizeof (float) * fileinfo->fmt.channels;
            char dspbuffer[dspsize * 4];
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
                        frames = dsp->plugin->process (dsp, (float *)dspbuffer, frames, sizeof (dspbuffer) / (fmt.channels * 4), &fmt, NULL);
                        dsp = dsp->next;
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
                    uint32_t size = (it->endsample-it->startsample) * outch * output_bps / 8;
                    if (!size) {
                        size = deadbeef->pl_get_item_duration (it) * fileinfo->fmt.samplerate * outch * output_bps / 8;

                    }

                    if (outsr != fileinfo->fmt.samplerate) {
                        uint64_t temp = size;
                        temp *= outsr;
                        temp /= fileinfo->fmt.samplerate;
                        size  = temp;
                    }

                    memcpy (&wavehdr[22], &outch, 2);
                    memcpy (&wavehdr[24], &outsr, 4);
                    uint16_t blockalign = outch * output_bps / 8;
                    memcpy (&wavehdr[32], &blockalign, 2);
                    memcpy (&wavehdr[34], &output_bps, 2);

                    fwrite (wavehdr, 1, wavehdr_size, temp_file);
                    if (encoder_preset->method == DDB_ENCODER_METHOD_PIPE) {
                        size = 0;
                    }
                    fwrite (&size, 1, sizeof (size), temp_file);
                    header_written = 1;
                }

                int64_t res = fwrite (buffer, 1, sz, temp_file);
                if (sz != res) {
                    fprintf (stderr, "converter: write error (%lld bytes written out of %d)\n", res, sz);
                    goto error;
                }
            }
            if (abort && *abort) {
                goto error;
            }
            if (temp_file && temp_file != enc_pipe) {
                fseek (temp_file, wavehdr_size, SEEK_SET);
                fwrite (&outsize, 1, 4, temp_file);

                fclose (temp_file);
                temp_file = NULL;
            }

            if (encoder_preset->encoder[0] && encoder_preset->method == DDB_ENCODER_METHOD_FILE) {
                enc_pipe = popen (enc, "w");
            }
        }
    }
    err = 0;
error:
    if (temp_file && temp_file != enc_pipe) {
        fclose (temp_file);
        temp_file = NULL;
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

    // write junklib tags
    uint32_t tagflags = JUNK_STRIP_ID3V2 | JUNK_STRIP_APEV2 | JUNK_STRIP_ID3V1;
    if (encoder_preset->tag_id3v2) {
        tagflags |= JUNK_WRITE_ID3V2;
    }
    if (encoder_preset->tag_id3v1) {
        tagflags |= JUNK_WRITE_ID3V1;
    }
    if (encoder_preset->tag_apev2) {
        tagflags |= JUNK_WRITE_APEV2;
    }
    DB_playItem_t *out_it = deadbeef->pl_item_alloc ();
    deadbeef->pl_item_copy (out_it, it);
    deadbeef->pl_replace_meta (out_it, ":URI", out);
    deadbeef->pl_delete_meta (out_it, "cuesheet");

    deadbeef->junk_rewrite_tags (out_it, tagflags, encoder_preset->id3v2_version + 3, "iso8859-1");

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
        DB_decoder_t *ogg = NULL;
        for (int i = 0; plugs[i]; i++) {
            if (!strcmp (plugs[i]->plugin.id, "stdogg")) {
                ogg = plugs[i];
                break;
            }
        }
        if (!ogg) {
            fprintf (stderr, "converter: ogg plugin not found, cannot write ogg metadata\n");
        }
        else {
            if (0 != ogg->write_metadata (out_it)) {
                fprintf (stderr, "converter: failed to write ogg metadata, not an ogg file?\n");
            }
        }
    }

    deadbeef->pl_item_unref (out_it);


    return err;
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
    .misc.plugin.version_minor = 1,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.name = "Converter",
    .misc.plugin.id = "converter",
    .misc.plugin.descr = "Converts any supported formats to other formats.\n"
        "Requires separate GUI plugin, e.g. Converter GTK UI\n",
    .misc.plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .get_output_path = get_output_path,
    .convert = convert,
    // 1.1 entry points
    .load_encoder_presets = load_encoder_presets,
    .load_dsp_presets = load_dsp_presets,
    .free_encoder_presets = free_encoder_presets,
    .free_dsp_presets = free_dsp_presets,
};

DB_plugin_t *
converter_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
