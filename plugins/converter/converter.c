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
#include <deadbeef.h>

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

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
        if (p->fname) {
            free (p->fname);
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
        else if (!strcmp (str, "fname")) {
            p->fname = strdup (item);
        }
        else if (!strcmp (str, "encoder")) {
            p->encoder = strdup (item);
        }
        else if (!strcmp (str, "method")) {
            p->method = atoi (item);
        }
        else if (!strcmp (str, "formats")) {
            sscanf (item, "%X", &p->formats);
        }
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
    fprintf (fp, "fname %s\n", p->fname);
    fprintf (fp, "encoder %s\n", p->encoder);
    fprintf (fp, "method %d\n", p->method);
    fprintf (fp, "formats %08X\n", p->formats);

    fclose (fp);
    return 0;
}

void
encoder_preset_copy (ddb_encoder_preset_t *to, ddb_encoder_preset_t *from) {
    to->title = strdup (from->title);
    to->fname = strdup (from->fname);
    to->encoder = strdup (from->encoder);
    to->method = from->method;
    to->formats = from->formats;
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
        while (p->chain) {
            ddb_dsp_context_t *next = p->chain->next;
            p->chain->plugin->close (p->chain);
            p->chain = next;
        }
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
    int err = 1;
    FILE *fp = fopen (fname, "rt");
    if (!fp) {
        return NULL;
    }
    ddb_dsp_preset_t *p = dsp_preset_alloc ();
    if (!p) {
        goto error;
    }

    // title
    char temp[100];
    if (1 != fscanf (fp, "title %100[^\n]\n", temp)) {
        goto error;
    }
    p->title = strdup (temp);
    ddb_dsp_context_t *tail = NULL;

    for (;;) {
        // plugin {
        int err = fscanf (fp, "%100s {\n", temp);
        if (err == EOF) {
            break;
        }
        else if (1 != err) {
            fprintf (stderr, "error plugin name\n");
            goto error;
        }

        DB_dsp_t *plug = (DB_dsp_t *)deadbeef->plug_get_for_id (temp);
        if (!plug) {
            fprintf (stderr, "ddb_dsp_preset_load: plugin %s not found. preset will not be loaded\n", temp);
            goto error;
        }
        ddb_dsp_context_t *ctx = plug->open ();
        if (!ctx) {
            fprintf (stderr, "ddb_dsp_preset_load: failed to open ctxance of plugin %s\n", temp);
            goto error;
        }

        if (tail) {
            tail->next = ctx;
            tail = ctx;
        }
        else {
            tail = p->chain = ctx;
        }

        int n = 0;
        for (;;) {
            char value[1000];
            if (!fgets (temp, sizeof (temp), fp)) {
                fprintf (stderr, "unexpected eof while reading plugin params\n");
                goto error;
            }
            if (!strcmp (temp, "}\n")) {
                break;
            }
            else if (1 != sscanf (temp, "\t%1000[^\n]\n", value)) {
                fprintf (stderr, "error loading param %d\n", n);
                goto error;
            }
            if (plug->num_params) {
                plug->set_param (ctx, n, value);
            }
            n++;
        }
    }

    err = 0;
error:
    if (err) {
        fprintf (stderr, "error loading %s\n", fname);
    }
    if (fp) {
        fclose (fp);
    }
    if (err && p) {
        dsp_preset_free (p);
        p = NULL;
    }
    return p;
}

int
dsp_preset_save (ddb_dsp_preset_t *p, int overwrite) {
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

    FILE *fp = fopen (path, "w+t");
    if (!fp) {
        return -1;
    }

    fprintf (fp, "title %s\n", p->title);

    ddb_dsp_context_t *ctx = p->chain;
    while (ctx) {
        fprintf (fp, "%s {\n", ctx->plugin->plugin.id);
        if (ctx->plugin->num_params) {
            int n = ctx->plugin->num_params ();
            int i;
            for (i = 0; i < n; i++) {
                char v[1000];
                ctx->plugin->get_param (ctx, i, v, sizeof (v));
                fprintf (fp, "\t%s\n", v);
            }
        }
        fprintf (fp, "}\n");
        ctx = ctx->next;
    }

    fclose (fp);
    return 0;
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

int
convert (DB_playItem_t *it, const char *outfolder, int selected_format, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort) {
    int err = -1;
    FILE *enc_pipe = NULL;
    FILE *temp_file = NULL;
    DB_decoder_t *dec = NULL;
    DB_fileinfo_t *fileinfo = NULL;
    char out[PATH_MAX] = ""; // full path to output file
    char input_file_name[PATH_MAX] = "";
    dec = (DB_decoder_t *)deadbeef->plug_get_for_id (it->decoder_id);
    if (dec) {
        fileinfo = dec->open (0);
        if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (it)) != 0) {
            fprintf (stderr, "converter: decoder->init failed\n");
            goto error;
        }
        if (fileinfo) {
            char fname[PATH_MAX];
            int idx = deadbeef->pl_get_idx_of (it);
            deadbeef->pl_format_title (it, idx, fname, sizeof (fname), -1, encoder_preset->fname);
            snprintf (out, sizeof (out), "%s/%s", outfolder, fname);
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

            // formatting: %o = outfile, %i = infile
            char *e = encoder_preset->encoder;
            char *o = enc;
            *o = 0;
            int len = sizeof (enc);
            while (e && *e) {
                if (len <= 0) {
                    fprintf (stderr, "converter: failed to assemble encoder command line - buffer is not big enough, try to shorten your parameters. max allowed length is %d characters\n", sizeof (enc));
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

            fprintf (stderr, "converter: will encode using: %s\n", enc);

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
            char wavehdr[] = {
                0x52, 0x49, 0x46, 0x46, 0x24, 0x70, 0x0d, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xac, 0x00, 0x00, 0x10, 0xb1, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61
            };
            int header_written = 0;
            uint32_t outsize = 0;
            uint32_t outsr = fileinfo->fmt.samplerate;
            uint16_t outch = fileinfo->fmt.channels;
            uint16_t outbps = fileinfo->fmt.bps;
            if (selected_format != 0) {
                switch (selected_format) {
                case 1 ... 4:
                    outbps = selected_format * 8;
                    break;
                case 5:
                    outbps = 32;
                    break;
                }
            }

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

                    outfmt.bps = outbps;
                    outfmt.channels = outch;
                    outfmt.samplerate = outsr;

                    int n = deadbeef->pcm_convert (&fmt, dspbuffer, &outfmt, buffer, frames * sizeof (float) * fmt.channels);
                    sz = n;
                }
                else if (fileinfo->fmt.bps != outbps) {
                    ddb_waveformat_t outfmt;
                    memcpy (&outfmt, &fileinfo->fmt, sizeof (outfmt));
                    outfmt.bps = outbps;
                    outfmt.channels = outch;
                    outfmt.samplerate = outsr;

                    int frames = sz / samplesize;
                    int n = deadbeef->pcm_convert (&fileinfo->fmt, buffer, &outfmt, dspbuffer, frames * samplesize);
                    memcpy (buffer, dspbuffer, n);
                    sz = n;
                }
                outsize += sz;

                if (!header_written) {
                    uint32_t size = (it->endsample-it->startsample) * outch * outbps / 8;
                    if (!size) {
                        size = deadbeef->pl_get_item_duration (it) * fileinfo->fmt.samplerate * outch * outbps / 8;

                    }

                    if (outsr != fileinfo->fmt.samplerate) {
                        uint64_t temp = size;
                        temp *= outsr;
                        temp /= fileinfo->fmt.samplerate;
                        size  = temp;
                    }

                    memcpy (&wavehdr[22], &outch, 2);
                    memcpy (&wavehdr[24], &outsr, 4);
                    uint16_t blockalign = outch * outbps / 8;
                    memcpy (&wavehdr[32], &blockalign, 2);
                    memcpy (&wavehdr[34], &outbps, 2);

                    fwrite (wavehdr, 1, sizeof (wavehdr), temp_file);
                    fwrite (&size, 1, sizeof (size), temp_file);
                    header_written = 1;
                }

                if (sz != fwrite (buffer, 1, sz, temp_file)) {
                    fprintf (stderr, "converter: write error\n");
                    goto error;
                }
            }
            if (abort && *abort) {
                goto error;
            }
            if (temp_file && temp_file != enc_pipe) {
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
    return 0;
}

// define plugin interface
static ddb_converter_t plugin = {
    .misc.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .misc.plugin.api_vminor = DB_API_VERSION_MINOR,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 0,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.name = "Converter",
    .misc.plugin.id = "converter",
    .misc.plugin.descr = "Converts any supported formats to other formats",
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
    .convert = convert,
};

DB_plugin_t *
converter_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
