/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Alexey Yakovenko and other contributors

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

// basic syntax:
// function call: $function([arg1[,arg2[,...]]])
// meta fields, with spaces allowed: %field name%
// if_defined block: [text$func()%field%more text]
// plain text: anywhere outside of the above
// escaping: $, %, [, ], \ must be escaped

// bytecode format
// 0: indicates start of special block
//  1: function call
//   func_idx:byte, num_args:byte, arg1_len:byte[,arg2_len:byte[,...]]
//  2: meta field
//   len:byte, data
//  3: if_defined block
//   len:int32, data
// !0: plain text

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include "streamer.h"
#include "utf8.h"
#include "playlist.h"
#include "tf.h"
#include "gettext.h"
#include "plugins.h"

#define min(x,y) ((x)<(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

typedef struct {
    const char *i;
    char *o;
} tf_compiler_t;

typedef int (*tf_func_ptr_t)(ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef);

#define TF_MAX_FUNCS 0xff

typedef struct {
    const char *name;
    tf_func_ptr_t func;
} tf_func_def;

int
tf_eval_int (ddb_tf_context_t *ctx, char *code, int size, char *out, int outlen, int fail_on_undef);

int
tf_eval (ddb_tf_context_t *ctx, char *code, int codelen, char *out, int outlen) {
    memset (out, 0, outlen);
    int l = 0;
    switch (ctx->id) {
    case DB_COLUMN_FILENUMBER:
        if (ctx->idx == -1 && ctx->plt) {
            ctx->idx = plt_get_item_idx ((playlist_t *)ctx->plt, (playItem_t *)ctx->it, PL_MAIN);
        }
        l = snprintf (out, sizeof (outlen), "%d", ctx->idx+1);
        break;
    case DB_COLUMN_PLAYING:
        l = pl_format_item_queue ((playItem_t *)ctx->it, out, outlen);
        break;
    default:
        l = tf_eval_int (ctx, code, codelen, out, outlen, 0);
        break;
    }

    for (; *out; out++) {
        if (*out == '\n') {
            *out = ';';
        }
    }
    return l;
}

// $greater(a,b) returns true if a is greater than b, otherwise false
int
tf_func_greater (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    char *arg = args;

    char a[10];
    int len = tf_eval_int (ctx, arg, arglens[0], a, sizeof (a), fail_on_undef);
    if (len < 0) {
        goto out;
    }
    else if (len == 0) {
        return 0;
    }
    int aa = atoi (a);

    arg += arglens[0];
    char b[10];
    len = tf_eval_int (ctx, arg, arglens[1], b, sizeof (b), fail_on_undef);
    if (len < 0) {
        goto out;
    }
    else if (len == 0) {
        return 0;
    }
    int bb = atoi (b);

    trace ("greater: (%s,%s)\n", a, b);
    if (aa > bb) {
        return 1;
    }
    else {
        return 0;
    }
out:
    *out = 0;
    return -1;
}

// $strcmp(s1,s2) compares s1 and s2, returns true if equal, otherwise false
int
tf_func_strcmp (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    char *arg = args;

    char s1[1000];
    int len = tf_eval_int (ctx, arg, arglens[0], s1, sizeof (s1), fail_on_undef);
    if (len < 0) {
        goto out;
    }
    else if (len == 0) {
        return 0;
    }

    arg += arglens[0];
    char s2[1000];
    len = tf_eval_int (ctx, arg, arglens[1], s2, sizeof (s2), fail_on_undef);
    if (len < 0) {
        goto out;
    }
    else if (len == 0) {
        return 0;
    }

    int res = strcmp (s1, s2);
    trace ("strcmp: (%s,%s), res: %d\n", s1, s2, res);
    if (res == 0) {
        return 1;
    }
    else {
        return 0;
    }
out:
    *out = 0;
    return -1;
}

// $left(text,n) returns the first n characters of text
int
tf_func_left (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    char *arg = args;

    // get number of characters
    char num_chars_str[10];
    arg += arglens[0];
    int len = tf_eval_int (ctx, arg, arglens[1], num_chars_str, sizeof (num_chars_str), fail_on_undef);
    if (len < 0) {
        goto out;
    }
    int num_chars = atoi (num_chars_str);
    if (num_chars <= 0 || num_chars > outlen) {
        goto out;
    }

    // get text
    char text[1000];
    arg = args;
    len = tf_eval_int (ctx, arg, arglens[0], text, sizeof (text), fail_on_undef);
    if (len < 0) {
        goto out;
    }

    int res = u8_strncpy (out, text, num_chars);
    trace ("left: (%s,%d) -> (%s), res: %d\n", text, num_chars, out, res);
    return res;
out:
    *out = 0;
    return -1;
}

int
tf_func_add (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int outval = 0;
    char *arg = args;
    trace ("num args: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        trace ("add: eval arg %d (%s)\n", i, arg);
        int len = tf_eval_int (ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (len < 0) {
            *out = 0;
            return -1;
        }
        outval += atoi (out);
        memset (out, 0, len);
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", outval);
    trace ("and of add (%d), res: %d\n", outval, res);
    return res;
}

int
tf_func_if (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 2 || argc > 3) {
        return -1;
    }
    char *arg = args;
    int res = tf_eval_int (ctx, arg, arglens[0], out, outlen, fail_on_undef);
    arg += arglens[0];
    if (res > 0) {
        trace ("condition true, eval then block\n");
        res = tf_eval_int (ctx, arg, arglens[1], out, outlen, fail_on_undef);
        if (res < 0) {
            *out = 0;
            return -1;
        }
    }
    else if (argc == 3) {
        trace ("condition false, eval else block\n");
        arg += arglens[1];
        res = tf_eval_int (ctx, arg, arglens[2], out, outlen, fail_on_undef);
        if (res < 0) {
            *out = 0;
            return -1;
        }
    }

    return res;
}

tf_func_def tf_funcs[TF_MAX_FUNCS] = {
    { "greater", tf_func_greater },
    { "strcmp", tf_func_strcmp },
    { "left", tf_func_left },
    { "add", tf_func_add },
    { "if", tf_func_if },
    { NULL, NULL }
};

int
tf_eval_int (ddb_tf_context_t *ctx, char *code, int size, char *out, int outlen, int fail_on_undef) {
    playItem_t *it = (playItem_t *)ctx->it;
    char *init_out = out;
    while (size) {
        if (*code) {
            trace ("free char: %c\n", *code);
            *out++ = *code++;
            size--;
            outlen--;
        }
        else {
            code++;
            size--;
            trace ("special: %d\n", (int)(*code));
            if (*code == 1) {
                code++;
                size--;
                trace ("exec func: %d (%s)\n", *code, tf_funcs[*code].name);
                tf_func_ptr_t func = tf_funcs[*code].func;
                code++;
                size--;
                int res = func (ctx, code[0], code+1, code+1+code[0], out, outlen, fail_on_undef);
                if (res == -1) {
                    return -1;
                }
                out += res;
                outlen -= res;

                int blocksize = 1 + code[0];
                for (int i = 0; i < code[0]; i++) {
                    blocksize += code[1+i];
                }
                trace ("blocksize: %d\n", blocksize);
                code += blocksize;
                size -= blocksize;
            }
            else if (*code == 2) {
                code++;
                size--;
                uint8_t len = *code;
                code++;
                size--;

                char name[len+1];
                memcpy (name, code, len);
                name[len] = 0;

                // special cases
                // most if not all of this stuff is to make tf scripts
                // compatible with fb2k syntax
                pl_lock ();
                const char *val = NULL;
                const char *aa_fields[] = { "album artist", "albumartist", "artist", "composer", "performer", NULL };
                const char *a_fields[] = { "artist", "album artist", "albumartist", "composer", "performer", NULL };

                // set to 1 if special case handler successfully wrote the output
                int skip_out = 0;

                // temp vars used for strcmp optimizations
                int tmp_a = 0, tmp_b = 0, tmp_c = 0, tmp_d = 0;

                if (!strcmp (name, aa_fields[0])) {
                    for (int i = 0; !val && aa_fields[i]; i++) {
                        val = pl_find_meta_raw (it, aa_fields[i]);
                    }
                    if (!val) {
                        val = _("Unknown Artist");
                    }
                }
                else if (!strcmp (name, a_fields[0])) {
                    for (int i = 0; !val && a_fields[i]; i++) {
                        val = pl_find_meta_raw (it, a_fields[i]);
                    }
                    if (!val) {
                        val = _("Unknown Artist");
                    }
                }
                else if (!strcmp (name, "track artist")) {
                    const char *aa = NULL;
                    for (int i = 0; !val && aa_fields[i]; i++) {
                        val = pl_find_meta_raw (it, aa_fields[i]);
                    }
                    aa = val ? val : _("Unknown Artist");
                    val = NULL;
                    for (int i = 0; !val && a_fields[i]; i++) {
                        val = pl_find_meta_raw (it, a_fields[i]);
                    }
                    if (!val) {
                        val = _("Unknown Artist");
                    }
                    if (strcmp (val, aa)) {
                        val = NULL;
                    }
                }
                else if (!strcmp (name, "tracknumber")) {
                    val = pl_find_meta_raw (it, "track");
                    if (val) {
                        const char *p = val;
                        while (*p) {
                            if (!isdigit (*p)) {
                                break;
                            }
                            p++;
                        }
                        if (*p) {
                            val = NULL;
                        }
                        else {
                            int len = snprintf (out, outlen, "%02d", atoi(val));
                            out[len] = 0;
                            out += len;
                            outlen -= len;
                            skip_out = 1;
                            val = NULL;
                        }
                    }
                }
                else if (!strcmp (name, "title")) {
                    val = pl_find_meta_raw (it, "title");
                    if (val && !(*val)) {
                        val = NULL;
                    }
                    if (!val) {
                        val = pl_find_meta_raw (it, ":URI");
                        if (val) {
                            const char *start = strrchr (val, '/');
                            if (start) {
                                start++;
                            }
                            else {
                                start = val;
                            }
                            const char *end = strrchr (start, '.');
                            if (end) {
                                int n = (int)(end-start);
                                n = min ((int)(end-start), outlen-1);
                                strncpy (out, start, n);
                                outlen -= n;
                                out += n;
                            }
                            val = NULL;
                        }
                    }
                }
                else if (!strcmp (name, "discnumber")) {
                    val = pl_find_meta_raw (it, "disc");
                    // convert "disc/disctotal" -> "disc"
                    if (val) {
                        const char *end = strrchr (val, '/');
                        if (end) {
                            int n = (int)(end-val);
                            n = min (n, outlen-1);
                            strncpy (out, val, n);
                            outlen -= n;
                            out += n;
                            skip_out = 1;
                            val = NULL;
                        }
                    }
                }
                else if (!strcmp (name, "totaldiscs")) {
                    val = pl_find_meta_raw (it, "disctotal");
                    if (!val) {
                        // try to extract disctotal from disc field
                        val = pl_find_meta_raw (it, "disc");
                        if (val) {
                            const char *start = strrchr (val, '/');
                            if (start) {
                                start++;
                                int n = strlen (start);
                                n = min (n, outlen-1);
                                strncpy (out, start, n);
                                outlen -= n;
                                out += n;
                                skip_out = 1;
                            }
                            val = NULL;
                        }
                    }
                }
                else if (!strcmp (name, "track number")) {
                    val = pl_find_meta_raw (it, "track");
                }
                else if (!strcmp (name, "samplerate")) {
                    val = pl_find_meta_raw (it, ":SAMPLERATE");
                }
                else if (!strcmp (name, "bitrate")) {
                    val = pl_find_meta_raw (it, ":BITRATE");
                }
                else if (!strcmp (name, "filesize")) {
                    val = pl_find_meta_raw (it, ":FILE_SIZE");
                }
                else if (!strcmp (name, "filesize_natural")) {
                    val = pl_find_meta_raw (it, ":FILE_SIZE");
                    if (val) {
                        int64_t bs = atoll (val);
                        int len;
                        if (bs >= 1024*1024*1024) {
                            double gb = (double)bs / (double)(1024*1024*1024);
                            len = snprintf (out, outlen, "%.3lf GB", gb);
                        }
                        else if (bs >= 1024*1024) {
                            double mb = (double)bs / (double)(1024*1024);
                            len = snprintf (out, outlen, "%.3lf MB", mb);
                        }
                        else if (bs >= 1024) {
                            double kb = (double)bs / (double)(1024);
                            len = snprintf (out, outlen, "%.3lf KB", kb);
                        }
                        else {
                            len = snprintf (out, outlen, "%lld B", bs);
                        }
                        out[len] = 0;
                        out += len;
                        outlen -= len;
                        skip_out = 1;
                        val = NULL;
                    }
                }
                else if (!strcmp (name, "channels")) {
                    val = pl_find_meta_raw (it, ":CHANNELS");
                    if (val) {
                        int ch = atoi (val);
                        if (ch == 1) {
                            val = _("mono");
                        }
                        else if (ch == 2) {
                            val = _("stereo");
                        }
                    }
                }
                else if (!strcmp (name, "codec")) {
                    val = pl_find_meta_raw (it, ":FILETYPE");
                }
                else if ((tmp_a = !strcmp (name, "playback_time")) || (tmp_b = !strcmp (name, "playback_time_seconds")) || (tmp_c = !strcmp (name, "playback_time_remaining")) || (tmp_d = !strcmp (name, "playback_time_remaining_seconds"))) {
                    playItem_t *playing = streamer_get_playing_track ();
                    if (it && playing == it) {
                        float t = streamer_get_playpos ();
                        if (tmp_c || tmp_d) {
                            printf ("inverse time %d %d %d %d\n", tmp_a, tmp_b, tmp_c, tmp_d);
                            float dur = pl_get_item_duration (it);
                            t = dur - t;
                        }
                        if (t >= 0) {
                            int len = 0;
                            if (tmp_a || tmp_c) {
                                int hr = t/3600;
                                int mn = (t-hr*3600)/60;
                                int sc = t-hr*3600-mn*60;
                                if (hr) {
                                    len = snprintf (out, outlen, "%02d:%02d:%02d", hr, mn, sc);
                                }
                                else {
                                    len = snprintf (out, outlen, "%02d:%02d", mn, sc);
                                }
                            }
                            else if (tmp_b || tmp_d) {
                                len = snprintf (out, outlen, "%0.2f", t);
                            }
                            out += len;
                            outlen -= len;
                            skip_out = 1;
                            val = NULL;
                            // notify the caller about update interval
                            if (!ctx->update || (ctx->update > 1000)) {
                                ctx->update = 1000;
                            }
                        }
                    }
                    if (playing) {
                        pl_item_unref (playing);
                    }
                }
                else if ((tmp_a = !strcmp (name, "length")) || (tmp_b = !strcmp (name, "length_ex"))) {
                    float t = pl_get_item_duration (it);
                    if (t >= 0) {
                        int hr = t/3600;
                        int mn = (t-hr*3600)/60;
                        int sc = tmp_a ? roundf(t-hr*3600-mn*60) : floor(t-hr*3600-mn*60);
                        int ms = !tmp_b ? roundf ((t-hr*3600-mn*60-sc) * 1000.f) : 0;
                        int len = 0;
                        if (tmp_a) {
                            if (hr) {
                                len = snprintf (out, outlen, "%2d:%02d:%02d", hr, mn, sc);
                            }
                            else {
                                len = snprintf (out, outlen, "%2d:%02d", mn, sc);
                            }
                        }
                        else if (tmp_b) {
                            if (hr) {
                                len = snprintf (out, outlen, "%2d:%02d:%02d.%03d", hr, mn, sc, ms);
                            }
                            else {
                                len = snprintf (out, outlen, "%2d:%02d.%03d", mn, sc, ms);
                            }
                        }
                        out += len;
                        outlen -= len;
                        skip_out = 1;
                        val = NULL;
                    }
                }
                else if ((tmp_a = !strcmp (name, "length_seconds") || (tmp_b = !strcmp (name, "length_seconds_fp")))) {
                    float t = pl_get_item_duration (it);
                    if (t >= 0) {
                        int len;
                        if (tmp_a) {
                            len = snprintf (out, outlen, "%d", (int)roundf(t));
                        }
                        else {
                            len = snprintf (out, outlen, "%0.3f", t);
                        }
                        out += len;
                        outlen -= len;
                        skip_out = 1;
                        val = NULL;
                    }
                }
                // TODO: length_samples: problem is that in deadbeef this is
                // only guaranteed to be precise after init (mp3), and is not
                // stored in the metadata

                else if ((tmp_a = !strcmp (name, "is_playing")) || (tmp_b = !strcmp (name, "is_paused"))) {
                    playItem_t *playing = streamer_get_playing_track ();
                    
                    if (playing && 
                            (
                            (tmp_a && plug_get_output ()->state () == OUTPUT_STATE_PLAYING)
                            || (tmp_b && plug_get_output ()->state () == OUTPUT_STATE_PAUSED)
                            )) {
                        *out++ = '1';
                        *out = 0;
                        outlen--;
                        skip_out = 1;
                        val = NULL;
                    }
                    if (playing) {
                        pl_item_unref (playing);
                    }
                }

                // default case
                else {
                    val = pl_find_meta_raw (it, name);
                }
                if (!skip_out && val) {
                    int len = (int)strlen (val);
                    memcpy (out, val, len);
                    out[len] = 0;
                    out += len;
                    outlen -= len;
                }
                else {
                    *out = 0;
                }
                pl_unlock ();
                if (!skip_out && !val && fail_on_undef) {
                    return -1;
                }

                code += len;
                size -= len;
            }
            else if (*code == 3) {
                code++;
                size--;
                int32_t len;
                memcpy (&len, code, 4);
                code += 4;
                size -= 4;

                int res = tf_eval_int (ctx, code, len, out, outlen, 1);
                if (res > 0) {
                    out += res;
                    outlen -= res;
                }
                else {
                    *out = 0;
                }
                code += len;
                size -= len;
            }
            else {
                trace ("invalid special block: %d\n", (int)(*code));
                return -1;
            }
        }
    }
    return (int)(out-init_out);
}

int
tf_compile_plain (tf_compiler_t *c);

int
tf_compile_func (tf_compiler_t *c) {
    trace ("start func\n");
    c->i++;

    // function marker
    *(c->o++) = 0;
    *(c->o++) = 1;

    const char *name_start = c->i;

    // find opening (
    while (*(c->i) && *(c->i) != '(') {
        c->i++;
    }

    if (!*(c->i)) {
        return -1;
    }

    int i;
    for (i = 0; tf_funcs[i].name; i++) {
        int l = (int)strlen (tf_funcs[i].name);
        if (c->i - name_start == l && !memcmp (tf_funcs[i].name, name_start, l)) {
            *(c->o++) = i;
            break;
        }
    }
    if (!tf_funcs[i].name) {
        return -1;
    }

    char func_name[c->i - name_start + 1];
    memcpy (func_name, name_start, c->i-name_start);
    func_name[c->i-name_start] = 0;
    trace ("func name: %s\n", func_name);

    c->i++;

    trace ("reading args, starting with %c\n", (*c->i));
    
    // remember ptr and start reading args
    char *start = c->o;
    *(c->o++) = 0; // num args
    char *argstart = c->o;

    //parse comma separated args until )
    while (*(c->i)) {
        if (*(c->i) == '\\') {
            c->i++;
            if (*(c->i) != 0) {
                *(c->o++) = *(c->i++);
            }
        }
        else if (*(c->i) == ',' || *(c->i) == ')') {
            // next arg
            int len = (int)(c->o - argstart);
            trace ("end of arg in func %s, len: %d\n", func_name, len);
            trace ("parsed arg: %s\n", start+(*start)+1);
            if (*(c->i) != ')' || len) {
                // expand arg lengths buffer by 1
                memmove (start+(*start)+2, start+(*start)+1, c->o - start - (*start));
                c->o++;
                (*start)++; // num args++
                // store arg length
                start[(*start)] = len;
                argstart = c->o;
                trace ("numargs: %d\n", (int)(*start));
            }

            if (*(c->i) == ')') {
                break;
            }
            c->i++;
        }
        else if (tf_compile_plain (c)) {
            return -1;
        }
    }
    if (*(c->i) != ')') {
        return -1;
    }
    c->i++;

    trace ("$%s num_args: %d\n", func_name, (int)*start);

    return 0;
}

int
tf_compile_field (tf_compiler_t *c) {
    trace ("start field\n");
    c->i++;
    *(c->o++) = 0;
    *(c->o++) = 2;

    const char *fstart = c->i;
    char *plen = c->o;
    c->o += 1;
    while (*(c->i)) {
        if (*(c->i) == '%') {
            break;
        }
        else {
            *(c->o++) = *(c->i++);
        }
    }
    if (*(c->i) != '%') {
        return -1;
    }
    c->i++;

    int32_t len = (int32_t)(c->o - plen - 1);
    if (len > 0xff) {
        trace ("field name to big: %d\n", len);
        return -1;
    }
    *plen = len;

    char field[len+1];
    memcpy (field, fstart, len);
    field[len] = 0;
    trace ("end field, len: %d, value: %s\n", len, field);
    return 0;
}

int
tf_compile_ifdef (tf_compiler_t *c) {
    trace ("start ifdef\n");
    c->i++;
    *(c->o++) = 0;
    *(c->o++) = 3;

    char *plen = c->o;
    c->o += 4;

    char *start = c->o;

    while (*(c->i)) {
        if (*(c->i) == '\\') {
            c->i++;
            if (*(c->i) != 0) {
                *(c->o++) = *(c->i++);
            }
        }
        else if (*(c->i) == ']') {
            break;
        }
        else if (tf_compile_plain (c)) {
            return -1;
        }
    }

    if (*(c->i) != ']') {
        return -1;
    }
    c->i++;

    int32_t len = (int32_t)(c->o - plen - 4);
    memcpy (plen, &len, 4);

    char value[len+1];
    memcpy (value, start, len);
    value[len] = 0;
    trace ("end ifdef, len: %d, value: %s\n", len, value+3);
    return 0;
}

int
tf_compile_plain (tf_compiler_t *c) {
    if (*(c->i) == '$') {
        if (tf_compile_func (c)) {
            return -1;
        }
    }
    else if (*(c->i) == '[') {
        if (tf_compile_ifdef (c)) {
            return -1;
        }
    }
    else if (*(c->i) == '%') {
        if (tf_compile_field (c)) {
            return -1;
        }
    }
    else if (*(c->i) == '\\') {
        c->i++;
        if (*(c->i) != 0) {
            *(c->o++) = *(c->i++);
        }
    }
    else {
        *(c->o++) = *(c->i++);
    }
    return 0;
}

int
tf_compile (const char *script, char **out) {
    tf_compiler_t c;
    memset (&c, 0, sizeof (c));

    c.i = script;

    char code[strlen(script) * 3];
    memset (code, 0, sizeof (code));

    c.o = code;

    while (*(c.i)) {
        if (tf_compile_plain (&c)) {
            return -1;
        }
    }

    trace ("output len: %d\n", (int)(c.o - code));
    trace ("%s\n", code);

    *out = malloc (c.o - code);
    memcpy (*out, code, c.o - code);
    return (int)(c.o - code);
}

void
tf_free (char *code) {
    free (code);
}

void
tf_test (void) {
    int len;
    char *code;
    len = tf_compile ("$add(1,2,3) [hello] [%hello%]", &code);
    trace ("code (%d): %s\n", len, code);

    for (int i = 0; i < len; i++) {
        trace ("%02x ", code[i]);
    }
    trace ("\n");
    for (int i = 0; i < len; i++) {
        trace ("%2c ", code[i] > 32 ? code[i] : 'x');
    }
    trace ("\n");

    ddb_tf_context_t ctx;
    memset (&ctx, 0, sizeof (ctx));
    ctx._size = sizeof (ctx);
    ctx.idx = -1;

    char out[1000] = "";
    int res = tf_eval (&ctx, code, len, out, sizeof (out));
    trace ("output (%d): %s\n", res, out);
}
