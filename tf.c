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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
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
#include "playqueue.h"
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

static int
tf_eval_int (ddb_tf_context_t *ctx, char *code, int size, char *out, int outlen, int *bool_out, int fail_on_undef);

#define TF_EVAL_CHECK(res, ctx, arg, arg_len, out, outlen, fail_on_undef)\
res = tf_eval_int (ctx, arg, arg_len, out, outlen, &bool_out, fail_on_undef);\
if (res < 0) { *out = 0; return -1; }

int
tf_eval (ddb_tf_context_t *ctx, char *code, char *out, int outlen) {
    if (!code) {
        *out = 0;
        return 0;
    }
    int32_t codelen = *((int32_t *)code);
    code += 4;
    memset (out, 0, outlen);
    int l = 0;

    int bool_out = 0;
    int id = -1;
    if (ctx->flags & DDB_TF_CONTEXT_HAS_ID) {
        id = ctx->id;
    }
    switch (id) {
    case DB_COLUMN_FILENUMBER:
        if (ctx->flags & DDB_TF_CONTEXT_HAS_INDEX) {
            l = snprintf (out, sizeof (outlen), "%d", ctx->idx+1);
        }
        else if (ctx->plt) {
            int idx = plt_get_item_idx ((playlist_t *)ctx->plt, (playItem_t *)ctx->it, PL_MAIN);
            l = snprintf (out, sizeof (outlen), "%d", idx+1);
        }
        break;
    case DB_COLUMN_PLAYING:
        l = pl_format_item_queue ((playItem_t *)ctx->it, out, outlen);
        break;
    default:
        // tf_eval_int expects outlen to not include the terminating zero
        l = tf_eval_int (ctx, code, codelen, out, outlen-1, &bool_out, 0);
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

    int bool_out = 0;

    char a[10];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], a, sizeof (a), fail_on_undef);

    int aa = atoi (a);

    arg += arglens[0];
    char b[10];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], b, sizeof (b), fail_on_undef);
    int bb = atoi (b);

    return aa > bb;
}

// $strcmp(s1,s2) compares s1 and s2, returns true if equal, otherwise false
int
tf_func_strcmp (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    char *arg = args;

    int bool_out = 0;

    char s1[1000];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], s1, sizeof (s1), fail_on_undef);

    arg += arglens[0];
    char s2[1000];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], s2, sizeof (s2), fail_on_undef);

    int res = strcmp (s1, s2);
    return !res;
}

// $left(text,n) returns the first n characters of text
int
tf_func_left (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    char *arg = args;

    int bool_out = 0;

    // get number of characters
    char num_chars_str[10];
    arg += arglens[0];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], num_chars_str, sizeof (num_chars_str), fail_on_undef);
    int num_chars = atoi (num_chars_str);
    if (num_chars <= 0 || num_chars > outlen) {
        *out = 0;
        return -1;
    }

    // get text
    char text[1000];
    arg = args;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], text, sizeof (text), fail_on_undef);

    int res = u8_strncpy (out, text, num_chars);
    trace ("left: (%s,%d) -> (%s), res: %d\n", text, num_chars, out, res);
    return res;
}

int
tf_func_add (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    int outval = 0;
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        outval += atoi (out);
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", outval);
    return res;
}

int
tf_func_div (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    float outval = 0;
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (i == 0) {
            outval = atoi (out);
        }
        else {
            int divider = atoi (out);
            if (divider == 0) {
                out[0] = 0;
                return -1;
            }
            outval /= divider;
        }
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", (int)round (outval));
    return res;
}

int
tf_func_max (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc == 0) {
        return -1;
    }

    int nmax = -1;
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        int n = atoi (out);
        if (n > nmax) {
            nmax = n;
        }
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", nmax);
    return res;
}

int
tf_func_min (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc == 0) {
        return -1;
    }

    int nmin = 0x7fffffff;
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        int n = atoi (out);
        if (n < nmin) {
            nmin = n;
        }
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", nmin);
    return res;
}

int
tf_func_mod (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    int outval = 0;
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (i == 0) {
            outval = atoi (out);
        }
        else {
            int divider = atoi (out);
            if (divider == 0) {
                *out = 0;
                return -1;
            }
            outval %= divider;
        }
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", outval);
    return res;
}

int
tf_func_mul (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    int outval = 0;
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (i == 0) {
            outval = atoi (out);
        }
        else {
            outval *= atoi (out);
        }
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", outval);
    return res;
}

int
tf_func_muldiv (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc != 3) {
        return -1;
    }

    int vals[3];
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        vals[i] = atoi (out);
        arg += arglens[i];
    }

    if (vals[2] == 0) {
        *out = 0;
        return -1;
    }

    int outval = (int)round(vals[0] * vals[1] / (float)vals[2]);

    int res = snprintf (out, outlen, "%d", outval);
    return res;
}

int
tf_func_rand (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 0) {
        return -1;
    }

    int outval = rand ();

    int res = snprintf (out, outlen, "%d", outval);
    return res;
}

int
tf_func_sub (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    int outval = 0;
    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (i == 0) {
            outval = atoi (out);
        }
        else {
            outval -= atoi (out);
        }
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", outval);
    return res;
}

int
tf_func_if (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 2 || argc > 3) {
        return -1;
    }
    int bool_out = 0;

    char *arg = args;
    int res;
    TF_EVAL_CHECK(res, ctx, arg, arglens[0], out, outlen, fail_on_undef);
    arg += arglens[0];
    if (res > 0 && bool_out) {
        trace ("condition true, eval then block\n");
        TF_EVAL_CHECK(res, ctx, arg, arglens[1], out, outlen, fail_on_undef);
    }
    else if (argc == 3) {
        trace ("condition false, eval else block\n");
        arg += arglens[1];
        TF_EVAL_CHECK(res, ctx, arg, arglens[2], out, outlen, fail_on_undef);
    }

    return res;
}

int
tf_func_if2 (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    int bool_out = 0;

    char *arg = args;
    int res;
    TF_EVAL_CHECK(res, ctx, arg, arglens[0], out, outlen, fail_on_undef);
    arg += arglens[0];
    if (res > 0 && bool_out) {
        return res;
    }
    else {
        TF_EVAL_CHECK(res, ctx, arg, arglens[1], out, outlen, fail_on_undef);
    }

    return res;
}

int
tf_func_if3 (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 2) {
        return -1;
    }
    int bool_out = 0;

    char *arg = args;
    for (int i = 0; i < argc; i++) {
        int res;
        TF_EVAL_CHECK(res, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        arg += arglens[i];
        if ((res > 0 && bool_out) || i == argc-1) {
            return res;
        }
    }
    *out = 0;
    return -1;
}

int
tf_func_ifequal (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 4) {
        return -1;
    }

    int bool_out = 0;

    char *arg = args;
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);

    int arg1 = atoi (out);

    arg += arglens[0];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], out, outlen, fail_on_undef);

    int arg2 = atoi (out);

    arg += arglens[1];

    int idx = 2;
    if (arg1 != arg2) {
        arg += arglens[2];
        idx = 3;
    }

    TF_EVAL_CHECK(len, ctx, arg, arglens[idx], out, outlen, fail_on_undef);
    return len;
}

int
tf_func_ifgreater (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 4) {
        return -1;
    }

    int bool_out = 0;

    char *arg = args;
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);

    int arg1 = atoi (out);

    arg += arglens[0];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], out, outlen, fail_on_undef);

    int arg2 = atoi (out);

    arg += arglens[1];

    int idx = 2;
    if (arg1 <= arg2) {
        arg += arglens[2];
        idx = 3;
    }

    TF_EVAL_CHECK(len, ctx, arg, arglens[idx], out, outlen, fail_on_undef);
    return len;
}

int
tf_func_iflonger (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 4) {
        return -1;
    }

    int bool_out = 0;

    char *arg = args;
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);
    int l1 = (int)strlen (out);

    arg += arglens[0];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], out, outlen, fail_on_undef);
    int l2 = (int)strlen (out);

    arg += arglens[1];
    int idx = 2;
    if (l1 <= l2) {
        arg += arglens[2];
        idx = 3;
    }

    TF_EVAL_CHECK(len, ctx, arg, arglens[idx], out, outlen, fail_on_undef);
    return len;
}

int
tf_func_select (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 3) {
        return -1;
    }

    char *arg = args;

    int bool_out = 0;

    int res;
    TF_EVAL_CHECK(res, ctx, arg, arglens[0], out, outlen, fail_on_undef);

    int n = atoi (out);
    if (n < 1 || n >= argc) {
        return 0;
    }

    arg += arglens[0];

    for (int i = 1; i < n; i++) {
        arg += arglens[i];
    }
    TF_EVAL_CHECK(res, ctx, arg, arglens[n], out, outlen, fail_on_undef);
    return res;
}

static void
tf_append_out (char **out, int *out_len, const char *in, int in_len) {
    in_len = min (in_len, *out_len);
    in_len = u8_strnbcpy (*out, in, in_len);
    *out_len -= in_len;
    *out += in_len;
}

int
tf_func_meta (ddb_tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    if (!ctx->it) {
        return 0;
    }

    int bool_out = 0;

    char *arg = args;
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);

    const char *meta = pl_find_meta_raw ((playItem_t *)ctx->it, out);
    if (!meta) {
        return 0;
    }

    int nb = (int)strlen (meta);
    nb = min (nb, outlen);
    return u8_strnbcpy(out, meta, nb);
}

tf_func_def tf_funcs[TF_MAX_FUNCS] = {
    // Control flow
    { "if", tf_func_if },
    { "if2", tf_func_if2 },
    { "if3", tf_func_if3 },
    { "ifequal", tf_func_ifequal },
    { "ifgreater", tf_func_ifgreater },
    { "iflonger", tf_func_iflonger },
    { "select", tf_func_select },
    // Arithmetic
    { "add", tf_func_add },
    { "div", tf_func_div },
    { "greater", tf_func_greater },
    { "max", tf_func_max },
    { "min", tf_func_min },
    { "mod", tf_func_mod },
    { "mul", tf_func_mul },
    { "muldiv", tf_func_muldiv },
    { "rand", tf_func_rand },
    { "sub", tf_func_sub },
    // String
    { "cut", tf_func_left },
    { "left", tf_func_left },
    { "strcmp", tf_func_strcmp },
    // Track info
    { "meta", tf_func_meta },
    { NULL, NULL }
};

static int
tf_eval_int (ddb_tf_context_t *ctx, char *code, int size, char *out, int outlen, int *bool_out, int fail_on_undef) {
    playItem_t *it = (playItem_t *)ctx->it;
    char *init_out = out;
    *bool_out = 0;
    while (size) {
        if (*code) {
            int len = u8_charcpy (out, code, outlen);
            if (len == 0) {
                break;
            }
            code += len;
            size -= len;
            out += len;
            outlen -= len;
        }
        else {
            code++;
            size--;
            if (*code == 1) {
                code++;
                size--;
                tf_func_ptr_t func = tf_funcs[*code].func;
                code++;
                size--;
                int res = func (ctx, code[0], code+1, code+1+code[0], out, outlen, fail_on_undef);
                if (res == -1) {
                    return -1;
                }
                if (res > 0) {
                    *bool_out = res;
                }
                out += res;
                outlen -= res;

                int blocksize = 1 + code[0];
                for (int i = 0; i < code[0]; i++) {
                    blocksize += code[1+i];
                }
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
                const char *alb_fields[] = { "album", "venue", NULL };

                // set to 1 if special case handler successfully wrote the output
                int skip_out = 0;

                // temp vars used for strcmp optimizations
                int tmp_a = 0, tmp_b = 0, tmp_c = 0, tmp_d = 0;

                if (!strcmp (name, aa_fields[0])) {
                    for (int i = 0; !val && aa_fields[i]; i++) {
                        val = pl_find_meta_raw (it, aa_fields[i]);
                    }
                }
                else if (!strcmp (name, a_fields[0])) {
                    for (int i = 0; !val && a_fields[i]; i++) {
                        val = pl_find_meta_raw (it, a_fields[i]);
                    }
                }
                else if (!strcmp (name, "album")) {
                    for (int i = 0; !val && alb_fields[i]; i++) {
                        val = pl_find_meta_raw (it, alb_fields[i]);
                    }
                }
                else if (!strcmp (name, "track artist")) {
                    const char *aa = NULL;
                    for (int i = 0; !val && aa_fields[i]; i++) {
                        val = pl_find_meta_raw (it, aa_fields[i]);
                    }
                    aa = val;
                    val = NULL;
                    for (int i = 0; !val && a_fields[i]; i++) {
                        val = pl_find_meta_raw (it, a_fields[i]);
                    }
                    if (val && aa && !strcmp (val, aa)) {
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
                                n = min ((int)(end-start), outlen);
                                n = u8_strnbcpy (out, start, n);
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
                            n = min (n, outlen);
                            n = u8_strnbcpy (out, val, n);
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
                                int n = u8_strnbcpy (out, start, outlen);
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
                        else {
                            val = _("multichannel");
                        }
                    }
                }
                else if (!strcmp (name, "codec")) {
                    val = pl_find_meta_raw (it, ":FILETYPE");
                }
                else if (!strcmp (name, "replaygain_album_gain")) {
                    val = pl_find_meta_raw (it, ":REPLAYGAIN_ALBUMGAIN");
                }
                else if (!strcmp (name, "replaygain_album_peak")) {
                    val = pl_find_meta_raw (it, ":REPLAYGAIN_ALBUMPEAK");
                }
                else if (!strcmp (name, "replaygain_track_gain")) {
                    val = pl_find_meta_raw (it, ":REPLAYGAIN_TRACKGAIN");
                }
                else if (!strcmp (name, "replaygain_track_peak")) {
                    val = pl_find_meta_raw (it, ":REPLAYGAIN_TRACKPEAK");
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
                                    len = snprintf (out, outlen, "%2d:%02d:%02d", hr, mn, sc);
                                }
                                else {
                                    len = snprintf (out, outlen, "%2d:%02d", mn, sc);
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
                    if (tmp_a) {
                        t = roundf (t);
                    }
                    else if (tmp_b) {
                        t = roundf(t * 1000) / 1000.f;
                    }
                    if (t >= 0) {
                        int hr = t/3600;
                        int mn = (t-hr*3600)/60;
                        int sc = tmp_a ? t-hr*3600-mn*60 : t-hr*3600-mn*60;
                        int ms = tmp_b ? (t-hr*3600-mn*60-sc) * 1000.f : 0;
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

                else if ((tmp_a = !strcmp (name, "isplaying")) || (tmp_b = !strcmp (name, "ispaused"))) {
                    playItem_t *playing = streamer_get_playing_track ();
                    
                    if (playing && 
                            (
                            (tmp_a && plug_get_output ()->state () == OUTPUT_STATE_PLAYING)
                            || (tmp_b && plug_get_output ()->state () == OUTPUT_STATE_PAUSED)
                            )) {
                        *out++ = '1';
                        outlen--;
                        skip_out = 1;
                        val = NULL;
                    }
                    if (playing) {
                        pl_item_unref (playing);
                    }
                }
                else if (!strcmp (name, "filename")) {
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
                            tf_append_out(&out, &outlen, start, (int)(end-start));
                            skip_out = 1;
                        }
                        val = NULL;
                    }
                }
                else if (!strcmp (name, "filename_ext")) {
                    val = pl_find_meta_raw (it, ":URI");
                    if (val) {
                        const char *start = strrchr (val, '/');
                        if (start) {
                            tf_append_out (&out, &outlen, start+1, (int)strlen (start+1));
                            skip_out = 1;
                        }
                        val = NULL;
                    }
                }
                else if (!strcmp (name, "directoryname")) {
                    val = pl_find_meta_raw (it, ":URI");
                    if (val) {
                        const char *end = strrchr (val, '/');
                        if (end) {
                            const char *start = end - 1;
                            while (start >= val && *start != '/') {
                                start--;
                            }
                            if (start && start != end) {
                                start++;
                                tf_append_out(&out, &outlen, start, (int)(end-start));
                                skip_out = 1;
                            }
                        }
                        val = NULL;
                    }
                }
                else if (!strcmp (name, "path")) {
                    val = pl_find_meta_raw (it, ":URI");
                }
                // index of track in playlist (zero-padded)
                else if (!strcmp (name, "list_index")) {
                    if (it) {
                        playlist_t *plt = pl_get_playlist (it);
                        if (plt) {
                            int total_tracks = plt_get_item_count (plt, ctx->iter);
                            int digits = 0;
                            do {
                                total_tracks /= 10;
                                digits++;
                            } while (total_tracks);

                            int idx = 0;
                            if (ctx->flags & DDB_TF_CONTEXT_HAS_INDEX) {
                                idx = ctx->idx + 1;
                            }
                            else {
                                idx = pl_get_idx_of_iter (it, ctx->iter) + 1;
                            }
                            int len = snprintf (out, outlen, "%0*d", digits, idx);
                            out += len;
                            outlen -= len;
                            skip_out = 1;
                            val = NULL;
                            plt_unref (plt);
                        }
                    }
                }
                // total number of tracks in playlist
                else if (!strcmp (name, "list_total")) {
                    int total_tracks = -1;
                    if (ctx->plt) {
                        total_tracks = plt_get_item_count ((playlist_t *)ctx->plt, ctx->iter);
                    }
                    else {
                        playlist_t *plt = plt_get_curr ();
                        if (plt) {
                            total_tracks = plt_get_item_count (plt, ctx->iter);
                            plt_unref (plt);
                        }
                    }
                    if (total_tracks >= 0) {
                        int len = snprintf (out, outlen, "%d", total_tracks);
                        out += len;
                        outlen -= len;
                        skip_out = 1;
                        val = NULL;
                    }
                }
                // index of track in queue
                else if (!strcmp (name, "queue_index")) {
                    if (it) {
                        int idx = playqueue_test (it) + 1;
                        if (idx >= 1) {
                            int len = snprintf (out, outlen, "%d", idx);
                            out += len;
                            outlen -= len;
                            skip_out = 1;
                            val = NULL;
                        }
                    }
                }
                // indexes of track in queue
                else if (!strcmp (name, "queue_indexes")) {
                    if (it) {
                        int idx = playqueue_test (it) + 1;
                        if (idx >= 1) {
                            int len = snprintf (out, outlen, "%d", idx);
                            out += len;
                            outlen -= len;
                            int count = playqueue_getcount ();
                            for (int i = idx; i < count; i++) {
                                playItem_t *trk = playqueue_get_item (i);
                                if (trk) {
                                    if (it == trk) {
                                        len = snprintf (out, outlen, ",%d", i + 1);
                                        out += len;
                                        outlen -= len;
                                    }
                                    pl_item_unref (trk);
                                }
                            }
                            skip_out = 1;
                            val = NULL;
                        }
                    }
                }
                // total amount of tracks in queue
                else if (!strcmp (name, "queue_total")) {
                    int count = playqueue_getcount ();
                    if (count >= 0) {
                        int len = snprintf (out, outlen, "%d", count);
                        out += len;
                        outlen -= len;
                        skip_out = 1;
                        val = NULL;
                    }
                }
                else if (!strcmp (name, "_deadbeef_version")) {
                    val = VERSION;
                }

                if (val) {
                    *bool_out = 1;
                }

                // default case
                if (!skip_out && val) {
                    int32_t l = u8_strnbcpy(out, val, outlen);
                    out += l;
                    outlen -= l;
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

                int bool_out = 0;
                int res = tf_eval_int (ctx, code, len, out, outlen, &bool_out, 1);
                if (res > 0) {
                    out += res;
                    outlen -= res;
                }
                code += len;
                size -= len;
            }
            else {
                return -1;
            }
        }
    }
    *out = 0;
    return (int)(out-init_out);
}

int
tf_compile_plain (tf_compiler_t *c);

int
tf_compile_func (tf_compiler_t *c) {
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

    c->i++;

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

            // special case for empty argument list
            if (len == 0 && *(c->i) == ')' && (*start) == 0) {
                break;
            }

            // expand arg lengths buffer by 1
            memmove (start+(*start)+2, start+(*start)+1, c->o - start - (*start));
            c->o++;
            (*start)++; // num args++
            // store arg length
            start[(*start)] = len;
            argstart = c->o;

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

    return 0;
}

int
tf_compile_field (tf_compiler_t *c) {
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
        return -1;
    }
    *plen = len;

    char field[len+1];
    memcpy (field, fstart, len);
    field[len] = 0;
    return 0;
}

int
tf_compile_ifdef (tf_compiler_t *c) {
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

char *
tf_compile (const char *script) {
    tf_compiler_t c;
    memset (&c, 0, sizeof (c));

    c.i = script;

    char code[strlen(script) * 3];
    memset (code, 0, sizeof (code));

    c.o = code;

    while (*(c.i)) {
        if (tf_compile_plain (&c)) {
            return NULL;
        }
    }

    size_t size = c.o - code;
    char *out = malloc (size + 8);
    memcpy (out + 4, code, size);
    memset (out + 4 + size, 0, 4); // FIXME: this is the padding for possible buffer overflow bug fix
    *((int32_t *)out) = (int32_t)(size);
    return out;
}

void
tf_free (char *code) {
    free (code);
}
