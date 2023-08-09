/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Oleksiy Yakovenko and other contributors

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
//   func_idx:byte, num_args:byte, arg1_len:uint16[,arg2_len:byte[,...]]
//  2: meta field
//   len:byte, data
//  3: if_defined block
//   len:int32, data
//  4: pre-interpreted text
//   len:int32, data
//  5: text dimming block
//   dim_amount:int8, len:int32, data
// !0: plain text

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>
#include <sys/stat.h>
#include "streamer.h"
#include "utf8.h"
#include "playlist.h"
#include "plmeta.h"
#include "playqueue.h"
#include "tf.h"
#include "gettext.h"
#include "plugins.h"
#include "junklib.h"
#include "external/wcwidth/wcwidth.h"

#define min(x,y) ((x)<(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define TEMP_BUFFER_SIZE 1000
#define TF_INTERNAL_FLAG_LOCKED (1<<16)

typedef struct {
    ddb_tf_context_t _ctx;

    /// indicates that current code is evaluated as the argument of $itematindex
    unsigned getting_item_at_index;
    /// the index parameter of $itematindex
    int item_at_index;
} ddb_tf_context_int_t;

typedef struct {
    const char *i;
    uint8_t *o;
    int eol;
} tf_compiler_t;

/*
 * String functions: Returns the number of bytes in the output buffer,
 *                   not including a null terminator, which is not written.
 * Integer functions: As with string functions. Returns a number in string format.
 * Boolean functions: Returns a positive value to indicate truthiness,
 *                    but with an empty output string (*out == 0).
 *
 * In any context, -1 indicates an error.
 * The output string is sometimes, but not always, set to the empty string (*out == 0) in such cases.
 * Use of TF_EVAL_CHECK will always ensure that the output string is always
 * null terminated in such cases, once reached.
 *
 * @param outlen Available bytes in `out`, not including space for any terminating null.
 */
typedef int (*tf_func_ptr_t)(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef);

#define TF_MAX_FUNCS 0xff

typedef struct {
    const char *name;
    tf_func_ptr_t func;
} tf_func_def;


/*
 * @param out output buffer to write to
 * @param outlen Available bytes in the buffer `out`, not including the terminating null byte.
 */
static int
tf_eval_int (ddb_tf_context_t *ctx, const char *code, int size, char *out, int outlen, int *bool_out, int fail_on_undef);

static const char *
_tf_get_combined_value (playItem_t *it, const char *key, int *needs_free, int item_index);


#define TF_EVAL_CHECK(res, ctx, arg, arg_len, out, outlen, fail_on_undef)\
res = tf_eval_int (ctx, arg, arg_len, out, outlen, &bool_out, fail_on_undef);\
if (res < 0) { *out = 0; return -1; }

// empty track is used when ctx.it is null
static playItem_t empty_track;
// empty playlist is used when ctx.plt is null
static playlist_t empty_playlist;
// empty code is used when "code" argument is null
static char empty_code[4] = {0};

static int
snprintf_clip (char *buf, size_t len, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, len, fmt, ap);
    va_end(ap);
    if (n < 0) {
        *buf = 0;
        return 0;
    }
    return (int)min (n, len-1);
}

/*
 * @param outlen bytes available in the buffer `out`, including the terminating null byte
 */
int
tf_eval (ddb_tf_context_t *_ctx, const char *code, char *out, int outlen) {
    // ensure the size is valid
    if (_ctx->_size < (char *)&_ctx->dimmed - (char *)_ctx) {
        *out = 0;
        return -1;
    }

    // normalize the context
    ddb_tf_context_int_t ctx = {0};
    ctx._ctx._size = sizeof (ddb_tf_context_t);
    ctx._ctx.flags = _ctx->flags;
    ctx._ctx.it = _ctx->it;
    ctx._ctx.plt = _ctx->plt;
    ctx._ctx.idx = _ctx->idx;
    ctx._ctx.id = _ctx->id;
    ctx._ctx.iter = _ctx->iter;
    ctx._ctx.update = _ctx->update;
    if (_ctx->_size >= (char *)&_ctx->metadata_transformer - (char *)_ctx + sizeof(_ctx->metadata_transformer)) {
        ctx._ctx.metadata_transformer = _ctx->metadata_transformer;
    }


    if (!code) {
        code = empty_code;
    }

    if (!ctx._ctx.it) {
        ctx._ctx.it = (ddb_playItem_t *)&empty_track;
    }

    if (!ctx._ctx.plt) {
        ctx._ctx.plt = (ddb_playlist_t *)&empty_playlist;
    }

    int32_t codelen = *((int32_t *)code);
    code += 4;
    memset (out, 0, outlen);
    int l = 0;

    int bool_out = 0;
    int id = -1;
    if (_ctx->flags & DDB_TF_CONTEXT_HAS_ID) {
        id = _ctx->id;
    }

    switch (id) {
    case DB_COLUMN_FILENUMBER:
        if (ctx._ctx.flags & DDB_TF_CONTEXT_HAS_INDEX) {
            l = snprintf_clip (out, outlen, "%d", ctx._ctx.idx+1);
        }
        else if (ctx._ctx.plt) {
            int idx = plt_get_item_idx ((playlist_t *)ctx._ctx.plt, (playItem_t *)ctx._ctx.it, PL_MAIN);
            l = snprintf_clip (out, outlen, "%d", idx+1);
        }
        break;
    case DB_COLUMN_PLAYING:
        l = pl_format_item_queue ((playItem_t *)ctx._ctx.it, out, outlen);
        break;
    default:
        // tf_eval_int expects outlen to not include the terminating zero
        TF_EVAL_CHECK(l, &ctx._ctx, code, codelen, out, outlen - 1, 0);
        break;
    }

    if (!(ctx._ctx.flags & DDB_TF_CONTEXT_MULTILINE)) {
        // replace any unprintable char with '_'
        for (; *out; out++) {
            if ((uint8_t)(*out) < ' ') {
                if (*out == '\033' && (ctx._ctx.flags & DDB_TF_CONTEXT_TEXT_DIM)) {
                    continue;
                }
                *out = '_';
            }
        }
    }

    _ctx->update = ctx._ctx.update;
    if (_ctx->_size >= (char *)&_ctx->dimmed - (char *)_ctx + sizeof(_ctx->dimmed)) {
        _ctx->dimmed = ctx._ctx.dimmed;
    }

    return l;
}

// $greater(a,b) returns true if a is greater than b, otherwise false
int
tf_func_greater (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    const char *arg = args;

    int bool_out = 0;

    char a[TEMP_BUFFER_SIZE];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], a, sizeof (a) - 1, fail_on_undef);

    int aa = atoi (a);

    arg += arglens[0];
    char b[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], b, sizeof (b) - 1, fail_on_undef);
    int bb = atoi (b);

    *out = 0;
    return aa > bb;
}

// $strcmp(s1,s2) compares s1 and s2, returns true if equal, otherwise false
int
tf_func_strcmp (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    const char *arg = args;

    int bool_out = 0;

    char s1[TEMP_BUFFER_SIZE];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], s1, sizeof (s1) - 1, fail_on_undef);

    arg += arglens[0];
    char s2[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], s2, sizeof (s2) - 1, fail_on_undef);

    int res = strcmp (s1, s2);
    *out = 0;
    return !res;
}

int
tf_func_stricmp (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    const char *arg = args;

    int bool_out = 0;

    char s1[TEMP_BUFFER_SIZE];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], s1, sizeof (s1) - 1, fail_on_undef);

    arg += arglens[0];
    char s2[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], s2, sizeof (s2) - 1, fail_on_undef);

    int res = u8_strcasecmp (s1, s2);
    *out = 0;
    return !res;
}

static int
tf_prefix_helper (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef, int swap) {
    if (argc == 0) {
        return -1;
    }

    const char *arg = args;
    int bool_out = 0;
    int i;

    char str[TEMP_BUFFER_SIZE];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], str, sizeof (str) - 1, fail_on_undef);
    arg += arglens[0];

    int prefix_count;
    size_t buffer_size;
    char *buf = NULL;

    if (argc == 1) {
        prefix_count = 2;
        buffer_size = 0;
    }
    else {
        prefix_count = argc - 1;
        buffer_size = 2000;
        buf = alloca(buffer_size);
    }

    const char *prefixes[prefix_count];
    int prefix_lengths[prefix_count];

    if (argc == 1) {
        prefixes[0] = "A";
        prefix_lengths[0] = 1;
        prefixes[1] = "The";
        prefix_lengths[1] = 3;
    }
    else {
        char *ptr = buf;
        for (i = 0; i < prefix_count; ++i) {
            prefixes[i] = ptr;
            TF_EVAL_CHECK (prefix_lengths[i], ctx, arg, arglens[i+1], ptr, (int)(buf+buffer_size-ptr-1), fail_on_undef);
            ptr += prefix_lengths[i]+1;
            arg += arglens[i+1];
        }
    }

    for (i = 0; i < prefix_count; i++) {
        if (prefix_lengths[i] + 1 > len) {
            continue;
        }

        if (!strncasecmp(str, prefixes[i], prefix_lengths[i]) && str[prefix_lengths[i]] == ' ') {
            int stripped_length = len - prefix_lengths[i] - 1;
            int new_len;
            if (swap) {
                new_len = len + 1;
            }
            else {
                new_len = stripped_length;
            }

            if (new_len > outlen) {
                return -1;
            }

            memcpy(out, str + prefix_lengths[i] + 1, stripped_length);

            if (swap) {
                out[stripped_length] = ',';
                out[stripped_length+1] = ' ';
                memcpy(out+stripped_length+2, str, prefix_lengths[i]);
            }

            return new_len;
        }
    }

    if (len > outlen) {
        return -1;
    }

    memcpy(out, str, len);

    return len;
}

// $stripprefix(str,...) strips a list of prefixes from a string. If no prefixes are supplied, 'A' and 'The' are used.
int
tf_func_stripprefix (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_prefix_helper (ctx, argc, arglens, args, out, outlen, fail_on_undef, 0);
}

// $swapprefix(str,...) moves a list of prefixes to the end of a string. If no prefixes are supplied, 'A' and 'The' are used.
int
tf_func_swapprefix (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_prefix_helper (ctx, argc, arglens, args, out, outlen, fail_on_undef, 1);
}

// $upper(str) converts a string to uppercase
int
tf_func_upper (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;

    int len;
    char temp_str[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, args, arglens[0], temp_str, sizeof (temp_str) - 1, fail_on_undef);

    char *pout = out;
    char *p = temp_str;
    while (*p && outlen > 0) {
        uint32_t i = 0;
        u8_nextchar (p, &i);
        if (i > outlen) {
            break;
        }
        int l = u8_toupper (p, i, pout);
        p += i;
        pout += l;
        outlen -= l;
    }

    return (int)(pout - out);
}

// $lower(str) converts a string to lowercase
int
tf_func_lower (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;

    int len;
    char temp_str[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, args, arglens[0], temp_str, sizeof (temp_str) - 1, fail_on_undef);

    char *pout = out;
    char *p = temp_str;
    while (*p && outlen > 0) {
        uint32_t i = 0;
        u8_nextchar (p, &i);
        if (i > outlen) {
            break;
        }
        int l = u8_tolower (p, i, pout);
        p += i;
        pout += l;
        outlen -= l;
    }

    return (int)(pout - out);
}

// $num(n,len) Formats the integer number n in decimal notation with len characters. Pads with zeros
// from the left if necessary. len includes the dash when the number is negative. If n is not numeric, it is treated as zero.
int
tf_func_num (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    const char *arg = args;
    int bool_out = 0;
    int len;

    if (argc != 2) {
        return -1;
    }

    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);
    arg += arglens[0];
    int n = atoi (out);

    TF_EVAL_CHECK(len, ctx, arg, arglens[1], out, outlen, fail_on_undef);
    int n_len = atoi (out);

    if (outlen < 1 || outlen < n_len) {
        *out = 0;
        return -1;
    }

    if (n_len < 0) {
        n_len = 0;
    }

    char *out_w = out;
    int is_negative = n < 0;
    int num_len = 0;

    int cnt = n;
    do {
        num_len++;
        cnt /= 10;
    } while (cnt);

    if (is_negative) {
        *out_w++ = '-';
        num_len++;
        n *= -1;
    }

    int num_len_plus_padding = num_len;
    while (num_len_plus_padding < n_len) {
        *out_w++ = '0';
        num_len_plus_padding++;
    }

    out_w += num_len - is_negative;
    do {
        *--out_w = (n % 10) + '0';
        n /= 10;
    } while (n);

    return n_len > num_len ? n_len : num_len;
}

// $replace(subject, search, replace, ...) Replaces all occurrences of `search`
// substring in `subject` with `replace`. Accepts multiple search and replace
// substrings
int
tf_func_replace (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 3 || argc % 2 == 0) {
        return -1;
    }

    int bool_out = 0;
    int i;

    const char *arg = args;
    char buf[2000];
    char *lines[argc];
    int lens[argc];
    char *ptr = buf;

    for (i = 0; i < argc; ++i) {
        lines[i] = ptr;
        TF_EVAL_CHECK (lens[i], ctx, arg, arglens[i], ptr, (int)(buf+sizeof(buf)-ptr-1), fail_on_undef);
        ptr += lens[i]+1;
        arg += arglens[i];
    }

    char *optr = out;
    const char *iptr = lines[0];

    for (;;) {
        int chunklen = (int)(lines[0] + lens[0] - iptr); //chunk is a substring before the found needle
        int idx = -1; //index of the found needle

        for (i = 0; i < (argc - 1) / 2; ++i) {
            // Check for empty string -- can't replace it with anything
            if (*lines[i*2+1] == 0) {
                break;
            }
            char *found = strstr (iptr, lines[i*2+1]);
            if (found && found - iptr < chunklen) {
                chunklen = (int)(found - iptr);
                idx = i;
            }
        }

        if (chunklen > out + outlen - optr)
            return -1;
        memcpy (optr, iptr, chunklen);
        optr += chunklen;

        if (idx == -1) //nothing found
            break;

        if (lens[idx*2+2] > out + outlen - optr)
            return -1;

        memcpy (optr, lines[idx*2+2], lens[idx*2+2]);
        optr += lens[idx*2+2];

        iptr += chunklen + lens[idx*2+1];
    }
    *optr = 0;
    return (int)(optr - out);
}

int
tf_func_abbr (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1 && argc != 2) {
        return -1;
    }

    const char *arg = args;

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);

    if (argc == 2) {
        char num_chars_str[TEMP_BUFFER_SIZE];
        arg += arglens[0];
        int l;
        TF_EVAL_CHECK(l, ctx, arg, arglens[1], num_chars_str, sizeof (num_chars_str) - 1, fail_on_undef);
        int num_chars = atoi (num_chars_str);
        if (len <= num_chars) {
            return len;
        }
    }

    char *p = out;
    char *pout = out;
    const char skipchars[] = "() ,/\\|";
    while (*p) {
        // skip whitespace/paren
        while (*p && strchr (skipchars, *p)) {
            p++;
        }
        if (!*p) {
            break;
        }

        // take the first letter for abbrev
        int is_bracket = *p == '[' || *p == ']';
        int32_t size = 0;
        u8_nextchar(p, &size);
        memmove (pout, p, size);
        pout += size;
        p += size;

        // skip to the end of word
        while (*p && !strchr (skipchars, *p)) {
            if (!is_bracket) {
                p++;
            }
            else {
                size = 0;
                u8_nextchar(p, &size);
                memmove (pout, p, size);
                pout += size;
                p += size;
            }
        }
    }

    *pout = 0;
    return (int)(pout - out);
}

int
tf_func_ansi (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);
    return len;
}

int
tf_func_ascii (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;

    int len;
    char temp_str[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, args, arglens[0], temp_str, sizeof (temp_str) - 1, fail_on_undef);

    len = junk_iconv (temp_str, len, out, outlen, "utf-8", "ascii");

    return len;
}

int
tf_caps_impl (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef, int do_lowercasing) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    char *p = out;
    char *end = p + len;
    const char skipchars[] = "() ,/\\|";
    while (*p) {
        // skip whitespace/paren
        while (*p && strchr (skipchars, *p)) {
            p++;
        }
        if (!*p) {
            break;
        }

        int is_bracket = *p == '[' || *p == ']';

        char temp[5];

        // uppercase the first letter
        int32_t size = 0;
        u8_nextchar (p, &size);
        int32_t uppersize = u8_toupper ((const signed char *)p, size, temp);
        if (uppersize != size) {
            memmove (p+uppersize, p+size, end-(p+size));
            end += uppersize - size;
            *end = 0;
        }
        memcpy (p, temp, uppersize);

        p += uppersize;

        // lowercase to the end of word
        while (*p && !strchr (skipchars, *p)) {
            if (is_bracket) {
                p++;
            }
            else {
                size = 0;
                u8_nextchar ((const char *)p, &size);
                if (do_lowercasing) {
                    int32_t lowersize = u8_tolower ((const signed char *)p, size, temp);
                    if (lowersize != size) {
                        memmove (p+lowersize, p+size, end-(p+size));
                        end += lowersize - size;
                        *end = 0;
                    }
                    memcpy (p, temp, lowersize);
                    p += lowersize;
                }
                else {
                    p += size;
                }
            }
        }
    }

    return (int)(end - out);
}

int
tf_func_caps (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_caps_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 1);
}

int
tf_func_caps2 (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_caps_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 0);
}

int
tf_func_char (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    int n = atoi (out);
    *out = 0;

    if (outlen < 5) {
        return -1;
    }
    len = u8_wc_toutf8 (out, n);
    return len;
}

int
tf_func_crc32 (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    static const uint32_t tab[256] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    uint32_t crc = 0xffffffff;

    for (int i = 0; i < len; i++) {
        crc = (crc >> 8) ^ tab[(crc ^ (uint8_t)out[i]) & 0xff];
    }

    crc ^= 0xffffffff;

    return snprintf_clip (out, outlen, "%u", crc);
}

int
tf_func_crlf (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 0 || outlen < 2) {
        return -1;
    }
    out[0] = '\n';
    out[1] = 0;
    return 1;
}

// $left(text,n) returns the first n characters of text
int
tf_func_left (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    const char *arg = args;

    int bool_out = 0;

    // get number of characters
    char num_chars_str[TEMP_BUFFER_SIZE];
    arg += arglens[0];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], num_chars_str, sizeof (num_chars_str) - 1, fail_on_undef);
    int num_chars = atoi (num_chars_str);
    if (num_chars <= 0 || num_chars > outlen) {
        *out = 0;
        return -1;
    }

    // get text
    char text[TEMP_BUFFER_SIZE];
    arg = args;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], text, sizeof (text) - 1, fail_on_undef);

    // convert num_chars to num_bytes
    int num_bytes = u8_offset(text, num_chars);

    int res = u8_strnbcpy (out, text, min(num_bytes, outlen));
    return res;
}

// $repeat(expr,count): repeat `count` copies of `expr`
int
tf_func_repeat (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    const char *arg = args;

    int bool_out = 0;

    // get repeat count
    char num_chars_str[TEMP_BUFFER_SIZE];
    arg += arglens[0];
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], num_chars_str, sizeof (num_chars_str) - 1, fail_on_undef);
    int repeat_count = atoi (num_chars_str);
    if (repeat_count < 0) {
        *out = 0;
        return -1;
    }
    else if (repeat_count == 0) {
        // early out on zero repeat count
        *out = 0;
        return 0;
    }

    // get expr
    char text[TEMP_BUFFER_SIZE];
    arg = args;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], text, sizeof (text) - 1, fail_on_undef);

    int res=0;
    for (int i = 0; i < repeat_count; i++) {
        if (res + len > outlen) {
            break;
        }
        res += u8_strnbcpy (out + len * i, text, len);
    }

    return res;
}

// $insert(str,insert,n): Inserts `insert` into `str` after `n` characters.
int
tf_func_insert (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 3) {
        return -1;
    }
    const char *arg = args;

    int bool_out = 0;

    int len, str_len, insert_len;

    // get str
    char str[TEMP_BUFFER_SIZE];
    arg = args;
    TF_EVAL_CHECK(str_len, ctx, arg, arglens[0], str, sizeof (str) - 1, fail_on_undef);
    int str_chars = u8_strlen(str);

    // get insert
    char insert[TEMP_BUFFER_SIZE];
    arg += arglens[0];
    TF_EVAL_CHECK(insert_len, ctx, arg, arglens[1], insert, sizeof (insert) - 1, fail_on_undef);

    // get insertion point
    char num_chars_str[TEMP_BUFFER_SIZE];
    arg += arglens[1];
    TF_EVAL_CHECK(len, ctx, arg, arglens[2], num_chars_str, sizeof (num_chars_str) - 1, fail_on_undef);
    int insertion_point = atoi (num_chars_str);
    if (insertion_point < 0) {
        *out = 0;
        return -1;
    }
    if (insertion_point > str_chars) {
        insertion_point = str_chars;
    }

    // convert num_chars to num_bytes
    int nb_before = u8_offset (str, insertion_point);
    int nb_after = u8_offset (str + nb_before, str_chars - insertion_point);

    int l;
    int res = 0;

    l = u8_strnbcpy(out, str, min (nb_before, outlen));
    outlen -= l;
    out += l;
    res += l;

    l = u8_strnbcpy(out, insert, min (insert_len, outlen));
    outlen -= l;
    out += l;
    res += l;

    l = u8_strnbcpy(out, str + nb_before, min (nb_after, outlen));
    outlen -= l;
    out += l;
    res += l;

    return res;
}

// $len(expr): returns length of `expr`
int
tf_func_len (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    return snprintf_clip(out, outlen, "%d", u8_strlen(out));
}

int
tf_func_len2 (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    int wcsz = 0;
    int32_t i = 0;
    uint32_t c;
    while (out[i] && (c = u8_nextchar(out, &i)) != 0) {
        wcsz += mk_wcwidth(c);
    }

    return snprintf_clip(out, outlen, "%d", wcsz);
}

int
tf_func_extremest_impl(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef, int shortest) {
    if (argc < 1) {
        return -1;
    }
    int bool_out = 0;
    int this_blen, this_clen;
    int best_blen = arglens[0];
    const char *pos = args;

    char tmp[outlen + 1];

    TF_EVAL_CHECK(best_blen, ctx, pos, arglens[0], out, outlen, fail_on_undef);
    int best_clen = u8_strlen(out);

    for (int i = 1; i != argc; ++i) {
        pos += arglens[i - 1];
        TF_EVAL_CHECK(this_blen, ctx, pos, arglens[i], tmp, outlen, fail_on_undef);
        this_clen = u8_strlen(tmp);
        if (shortest ? this_clen < best_clen : this_clen > best_clen) {
            best_clen = this_clen;
            best_blen = this_blen;
            u8_strnbcpy(out, tmp, min(best_blen, outlen));
       }
    }

    return best_blen;
}

int
tf_func_shortest(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_extremest_impl(ctx, argc, arglens, args, out, outlen, fail_on_undef, 1);
}

int
tf_func_longest(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_extremest_impl(ctx, argc, arglens, args, out, outlen, fail_on_undef, 0);
}

int
tf_func_longer(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    return tf_func_longest(ctx, argc, arglens, args, out, outlen, fail_on_undef);
}

// $pad(expr,len[, char]): If `expr` is shorter than len characters, the function adds `char` characters (if present otherwise
// spaces) to the right of `expr` to make the result `len` characters long. Otherwise the function returns str unchanged.
int
tf_func_pad_impl (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef, int right, int cut) {
    if (argc < 1 || argc > 3) {
        return -1;
    }

    int bool_out = 0;
    int len, str_len;
    const char *arg = args;
    char pad_char_str[TEMP_BUFFER_SIZE] = " ";
    int nb_pad_char=1;

    // get expr
    char str[outlen];
    TF_EVAL_CHECK(str_len, ctx, args, arglens[0], str, outlen, fail_on_undef);

    // get len
    char num_chars_str[TEMP_BUFFER_SIZE];
    arg += arglens[0];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], num_chars_str, sizeof (num_chars_str) - 1, fail_on_undef);
    int padlen_chars = atoi (num_chars_str);
    if (padlen_chars < 0) {
        *out = 0;
        return -1;
    }
    // get char
    if (argc == 3) {
        arg += arglens[1];
        TF_EVAL_CHECK(len, ctx, arg, arglens[2], pad_char_str, sizeof (pad_char_str) - 1, fail_on_undef);
        // only accept first character
        nb_pad_char = u8_offset(pad_char_str, 1);
        pad_char_str[nb_pad_char] = 0;
    }

    int str_chars = u8_strlen(str);

    if (str_chars >= padlen_chars) {
        if (str_chars > padlen_chars && cut) {
            // u8_strncpy has no limiter in byte units available. We rely on str being of size outlen above for safety
            return u8_strncpy(out, str, padlen_chars);
        }
        return u8_strnbcpy(out, str, min (str_len, outlen));
    }

    int res=0,l;
    int repeat_count = padlen_chars-str_chars;


    if (!right) {
        l = u8_strnbcpy(out, str, min (str_len, outlen));
        outlen -= l;
        out += l;
        res += l;
    }

    for (int i = 0; i < repeat_count && outlen; i++) {
        l = u8_charcpy (out, pad_char_str, nb_pad_char);
        outlen -= l;
        out += l;
        res += l;
    }

    if (right) {
        l = u8_strnbcpy(out, str, min (str_len, outlen));
        outlen -= l;
        out += l;
        res += l;
    }

    return res;
}

int
tf_func_pad (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_pad_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 0, 0);
}

// $pad_right(expr,len[,char]): same as $pad but right aligns string
int
tf_func_pad_right (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_pad_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 1, 0);
}

// _cut variants: same as above, but truncates string if too long
int
tf_func_padcut (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_pad_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 0, 1);
}

int
tf_func_padcut_right (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_pad_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 1, 1);
}

int
tf_func_progress_impl(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef, int alt) {
    if (argc != 5) {
        return -1;
    }

    int bool_out = 0;
    int len;

    int progress, range;
    int barsize;
    char *notch;
    char *bar;

    const char *argpos = args;

    TF_EVAL_CHECK(len, ctx, argpos, arglens[0], out, outlen - 1, fail_on_undef);
    progress = atoi(out);
    if (progress < 0) {
        return -1;
    }
    argpos += arglens[0];

    TF_EVAL_CHECK(len, ctx, argpos, arglens[1], out, outlen - 1, fail_on_undef);
    range = atoi(out);
    if (range < 0) {
        return -1;
    }
    if (range == 0) {
        range = 1;
    }
    argpos += arglens[1];

    TF_EVAL_CHECK(len, ctx, argpos, arglens[2], out, outlen - 1, fail_on_undef);
    barsize = atoi(out);
    if (range < 0) {
        return -1;
    }
    argpos += arglens[2];

    TF_EVAL_CHECK(len, ctx, argpos, arglens[3], out, outlen - 1, fail_on_undef);
    notch = strdup (out);
    argpos += arglens[3];

    len = tf_eval_int (ctx, argpos, arglens[4], out, outlen - 1, &bool_out, fail_on_undef);
    if (len < 0) {
        free (notch);
        *out = 0;
        return -1;
    }
    bar = strdup (out);
    argpos += arglens[4];

    int replaced = progress * barsize / range;
    /*
     * Alt mode fills a bar. Normal mode draws a single notch.
     * This adjustment sets what happens once we've reached the end, whether we should:
     *   - fill up completely, or
     *   - replace the last character.
     */
    if (!alt && replaced >= barsize) {
        replaced = barsize - 1;
    }
    char *cur = out;
    int remaining = outlen;
    for (int i = 0; i != barsize; ++i) {
        /*
         * Experimenting in fb2k, it looks like:
         * What would strictly precede the mark's normal mode position gets marked in alt mode,
         * with the exception of a full bar.
         */
        // Try to replicate this.
        char *p = NULL;
        if (alt ? i < replaced : i == replaced) {
            p = notch;
        } else {
            p = bar;
        }
        size_t l = min(strlen (p), remaining);
        l = u8_strncpy (cur, p, (int)l);
        cur += l;
        remaining -= l;
    }

    free (bar);
    free (notch);

    return outlen - remaining;
}

int
tf_func_progress (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_progress_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 0);
}

int
tf_func_progress2 (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    return tf_func_progress_impl (ctx, argc, arglens, args, out, outlen, fail_on_undef, 1);
}

int
tf_func_right (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args + arglens[0], arglens[1], out, outlen, fail_on_undef);
    int desired_chars = atoi(out);
    if (desired_chars < 0) {
        return -1;
    }

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);
    int clen = u8_strlen(out);

    if (clen < desired_chars) {
        return len;
    }

    int32_t index = 0;
    for (int remaining = clen - desired_chars; remaining; --remaining) {
        u8_nextchar(out, &index);
    }

    int newblen = len - index;

    memmove(out, out + index, newblen);
    return newblen;
}

int
tf_func_roman (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;
    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);
    int value = atoi(out);
    // fb2k (presumably, it goes off my screen) formats 100000; but not 100001
    if (value < 0 || value > 100000) {
        return -1;
    }

    int pos = 0;
    while (pos < outlen - 1) {
        if (value <= 0) {
            break;
        }
        if (value >= 1000) {
            out[pos++] = 'M';
            value -= 1000;
        } else if (value >= 900) {
            out[pos++] = 'C';
            value += 100;
        } else if (value >= 500) {
            out[pos++] = 'D';
            value -= 500;
        } else if (value >= 400) {
            out[pos++] = 'C';
            value += 100;
        } else if (value >= 100) {
            out[pos++] = 'C';
            value -= 100;
        } else if (value >= 90) {
            out[pos++] = 'X';
            value += 10;
        } else if (value >= 50) {
            out[pos++] = 'L';
            value -= 50;
        } else if (value >= 40) {
            out[pos++] = 'X';
            value += 10;
        } else if (value >= 10) {
            out[pos++] = 'X';
            value -= 10;
        } else if (value >= 9) {
            out[pos++] = 'I';
            value += 1;
        } else if (value >= 5) {
            out[pos++] = 'V';
            value -= 5;
        } else if (value >= 4) {
            out[pos++] = 'I';
            value += 1;
        } else if (value >= 1) {
            out[pos++] = 'I';
            value -= 1;
        }
    }

    return pos;
}

int
tf_func_rot13 (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    int32_t offset = 0;
    while (offset < outlen && out[offset]) {
        char c = out[offset];
        if (c >= 'A' && c <= 'M') {
            out[offset++] += 13;
        } else if (c >= 'N' && c <= 'Z') {
            out[offset++] -= 13;
        } else if (c >= 'a' && c <= 'm') {
            out[offset++] += 13;
        } else if (c >= 'n' && c <= 'z') {
            out[offset++] -= 13;
        } else {
            u8_nextchar(out, &offset);
        }
    }

    return offset;
}

int
tf_func_strchr(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 1) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args + arglens[0], arglens[1], out, outlen, fail_on_undef);
    int dummy = 0;
    // fb2k permits strings with many characters as the needle, but uses exactly the first character from them.
    uint32_t needle = u8_nextchar(out, &dummy);

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    int32_t charpos;
    char *pos = u8_strchr(out, needle, &charpos);

    if (!pos) {
        // 0 is used to indicate not found
        charpos = 0;
    } else {
        // Convert from 0- to 1- based indexing.
        charpos += 1;
    }

    return snprintf_clip(out, outlen, "%" PRId32, charpos);
}

int
tf_func_strrchr(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 1) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    char str[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, args + arglens[0], arglens[1], str, sizeof(str) - 1, fail_on_undef);
    int dummy = 0;
    uint32_t needle = u8_nextchar(str, &dummy);

    int32_t acc = 0, charpos;
    char *pos = out;
    do {
        pos = u8_strchr(pos, needle, &charpos);
        if (pos) {
            /*
             * Simultaneously and efficiently:
             * - prevent finding the same character repeatedly
             * - preserve 0 = not found as a return string
             * - convert from 0- to 1- based indexing
             * The second and third points are effected on the first iteration,
             * with the +1 on the accumulator converting indexing and marking
             * found when found.
             * The first point is effected on later iterations, with the +1 on
             * the accumulator covering the increment in character position
             * from the previous iteration.
             */
            acc += charpos + 1;
            pos = pos + 1;
        }
    } while (pos);

    return snprintf_clip(out, outlen, "%" PRId32, acc);
}

int
tf_func_strstr(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    char needle[TEMP_BUFFER_SIZE];

    TF_EVAL_CHECK(len, ctx, args + arglens[0], arglens[1], needle, sizeof(needle) - 1, fail_on_undef);

    char *pos = strstr(out, needle);

    int clen = 0;
    if (pos) {
        /*
         * Terminate the string at the occurrence of the needle, then find the new
         * overall length to find the offset in characters
         */
        *pos = 0;
        clen = u8_strlen(out);
        // Convert from 0-based offset to 1-based indexing
        clen += 1;
    }
    return snprintf_clip(out, outlen, "%d", clen);
}

// fb2k uses 1-based indexing for $substr, with both points inclusive, and all arguments mandatory.
int
tf_func_substr(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 3) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    char str[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, args + arglens[0], arglens[1], str, sizeof(str) - 1, fail_on_undef);
    int from = atoi(str);
    if (from <= 0) {
        // fb2k tolerates negative 'from' values
        from = 1;
    }
    // Convert to a 0-based offset
    from -= 1;

    char *temp = malloc (outlen);
    TF_EVAL_CHECK(len, ctx, args + arglens[0] + arglens[1], arglens[2], temp, outlen, fail_on_undef);
    int to = atoi(temp);
    free (temp);
    if (to <= 0) {
        // If we don't want anything, finish early
        return 0;
    }
    // Convert to a 0-based offset
    to -= 1;

    int bpos = 0;
    int chars;
    for (chars = 0; out[bpos] && chars != from; ++chars) {
        u8_nextchar(out, &bpos);
    }
    int bfrom = bpos;

    for ( ; out[bpos] && chars <= to; ++chars) {
        u8_nextchar(out, &bpos);
    }
    int bto = bpos;

    int bdiff = bto - bfrom;

    memmove(out, out + bfrom, bdiff);

    return bdiff;

}

int
tf_func_tab(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc > 1) {
        return -1;
    }

    volatile int amount = 1;
    if (argc == 1) {
        int bool_out = 0;
        int len;

        char str[TEMP_BUFFER_SIZE];
        TF_EVAL_CHECK(len, ctx, args, arglens[0], str, sizeof(str) - 1, fail_on_undef);
        amount = atoi(str);
        if (amount < 0) {
            return -1;
        }
    }
    if (amount > outlen) {
        amount = outlen;
    }

    memset(out, '\t', amount);
    return amount;
}

int
tf_func_trim(ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc == 0) {
        return 0;
    }
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;
    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    int bfrom = -1, bto = 0;

    for (int offset = 0; offset != len && out[offset]; ) {
        const int last = offset;
        u8_nextchar(out, &offset);
        if (out[last] != ' ') {
            if (bfrom == -1) {
                bfrom = last;
            }
            bto = offset;
        }
    }

    if (bfrom == -1) {
        return 0;
    }

    int bdiff = bto - bfrom;
    memmove(out, out + bfrom, bdiff);
    return bdiff;
}

int
tf_func_directory (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 1 || argc > 2) {
        return -1;
    }

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    int path_len = len;

    int levels = 1;
    if (argc == 2) {
        char temp[20];
        args += arglens[0];
        TF_EVAL_CHECK(len, ctx, args, arglens[1], temp, sizeof (temp) - 1, fail_on_undef);
        levels = atoi (temp);
        if (levels < 0) {
            return -1;
        }
    }

    char *end = out + path_len - 1;
    char *start = end;

    while (levels--) {
        // get to the last delimiter
        while (end >= out && *end != '/') {
            end--;
        }

        if (end < out) {
            *out = 0;
            return -1;
        }

        // skip multiple delimiters
        while (end >= out && *end == '/') {
            end--;
        }
        end++;

        if (end < out) {
            *out = 0;
            return -1;
        }

        // find another delimiter
        start = end - 1;
        while (start > out && *start != '/') {
            start--;
        }

        if (*start == '/') {
            start++;
        }

        if (levels) {
            end = start;
            while (end >= out && *end == '/') {
                end--;
            }
        }
    }

    memmove (out, start, end-start);
    return (int)(end-start);
}

int
tf_func_directory_path (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 1 || argc > 2) {
        return -1;
    }

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    char *p = out + len - 1;

    while (p >= out && *p != '/') {
        p--;
    }
    while (p >= out && *p == '/') {
        p--;
    }
    if (p < out) {
        *out = 0;
        return -1;
    }

    p++;
    return (int)(p-out);
}

int
tf_func_ext (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 1 || argc > 2) {
        return -1;
    }

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);
    
    char *e = out + len;
    char *c = e - 1;
    char *p = NULL;

    while (c >= out && *c != '/') {
        if (*c == '.') {
            p = c+1;
            break;
        }
        c--;
    }

    if (!p) {
        *out = 0;
        return 0;
    }

    memmove (out, p, e-p+1);
    return (int)(e-p);
}

int
tf_func_filename (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 1 || argc > 2) {
        return -1;
    }

    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    char *e = out + len;
    char *p = e - 1;
    while (p >= out && *p != '/') {
        p--;
    }

    p++;

    memmove (out, p, e-p+1);
    return (int)(e-p);
}

int
tf_func_add (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    int outval = 0;
    const char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        outval += atoi (out);
        arg += arglens[i];
    }
    return snprintf_clip (out, outlen, "%d", outval);
}

int
tf_func_div (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    float outval = 0;
    const char *arg = args;
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
    int res = snprintf_clip (out, outlen, "%d", (int)round (outval));
    return res;
}

int
tf_func_max (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc == 0) {
        return -1;
    }

    int nmax = -1;
    const char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        int n = atoi (out);
        if (n > nmax) {
            nmax = n;
        }
        arg += arglens[i];
    }
    int res = snprintf_clip (out, outlen, "%d", nmax);
    return res;
}

int
tf_func_min (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc == 0) {
        return -1;
    }

    int nmin = 0x7fffffff;
    const char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        int n = atoi (out);
        if (n < nmin) {
            nmin = n;
        }
        arg += arglens[i];
    }
    int res = snprintf_clip (out, outlen, "%d", nmin);
    return res;
}

int
tf_func_mod (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    int outval = 0;
    const char *arg = args;
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
    int res = snprintf_clip (out, outlen, "%d", outval);
    return res;
}

int
tf_func_mul (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    int outval = 0;
    const char *arg = args;
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
    int res = snprintf_clip (out, outlen, "%d", outval);
    return res;
}

int
tf_func_muldiv (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc != 3) {
        return -1;
    }

    int vals[3];
    const char *arg = args;
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

    int res = snprintf_clip (out, outlen, "%d", outval);
    return res;
}

int
tf_func_rand (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 0) {
        return -1;
    }

    int outval = rand ();

    int res = snprintf_clip (out, outlen, "%d", outval);
    return res;
}

int
tf_func_sub (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    if (argc < 2) {
        return -1;
    }

    int outval = 0;
    const char *arg = args;
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
    int res = snprintf_clip (out, outlen, "%d", outval);
    return res;
}

int
tf_func_if (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 2 || argc > 3) {
        return -1;
    }
    int bool_out = 0;

    const char *arg = args;
    int res;
    TF_EVAL_CHECK(res, ctx, arg, arglens[0], out, outlen, fail_on_undef);
    arg += arglens[0];
    if (bool_out) {
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
tf_func_if2 (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }
    int bool_out = 0;

    const char *arg = args;
    int res;
    TF_EVAL_CHECK(res, ctx, arg, arglens[0], out, outlen, fail_on_undef);
    arg += arglens[0];
    if (bool_out) {
        return res;
    }
    else {
        TF_EVAL_CHECK(res, ctx, arg, arglens[1], out, outlen, fail_on_undef);
    }

    return res;
}

int
tf_func_if3 (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 2) {
        return -1;
    }
    int bool_out = 0;

    const char *arg = args;
    for (int i = 0; i < argc; i++) {
        int res;
        TF_EVAL_CHECK(res, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        arg += arglens[i];
        if (bool_out || i == argc-1) {
            return res;
        }
    }
    *out = 0;
    return -1;
}

int
tf_func_ifequal (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 4) {
        return -1;
    }

    int bool_out = 0;

    const char *arg = args;
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
tf_func_ifgreater (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 4) {
        return -1;
    }

    int bool_out = 0;

    const char *arg = args;
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
tf_func_iflonger (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 4) {
        return -1;
    }

    int bool_out = 0;

    const char *arg = args;
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);
    int l1 = (int)strlen (out);

    arg += arglens[0];
    TF_EVAL_CHECK(len, ctx, arg, arglens[1], out, outlen, fail_on_undef);
    int l2 = (int)atoi (out);

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
tf_func_select (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc < 3) {
        return -1;
    }

    const char *arg = args;

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

static int
tf_item_index_for_context(ddb_tf_context_t *_ctx) {
    ddb_tf_context_int_t *ctx = (ddb_tf_context_int_t *)_ctx;
    if (ctx->getting_item_at_index) {
        return ctx->item_at_index;
    }
    return -1;
}

int
tf_func_meta (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    if (!ctx->it) {
        return 0;
    }

    int bool_out = 0;

    const char *arg = args;
    int len;
    TF_EVAL_CHECK(len, ctx, arg, arglens[0], out, outlen, fail_on_undef);

    int needs_free = 0;
    const char *meta = _tf_get_combined_value ((playItem_t *)ctx->it, out, &needs_free, tf_item_index_for_context(ctx));
    if (!meta) {
        return 0;
    }

    int res = u8_strnbcpy(out, meta, outlen);
    if (needs_free) {
        free ((char *)meta);
    }
    return res;
}

const char *
tf_get_channels_string_for_track (playItem_t *it) {
    const char *val = pl_find_meta_raw (it, ":CHANNELS");
    if (val) {
        int ch = atoi (val);
        if (ch == 1) {
            val = _("mono");
        }
        else if (ch == 2) {
            val = _("stereo");
        }
    }
    else {
        val = _("stereo");
    }
    return val;
}

int
tf_func_channels (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 0) {
        return -1;
    }

    if (!ctx->it) {
        return 0;
    }

    const char *val = tf_get_channels_string_for_track ((playItem_t *)ctx->it);
    return u8_strnbcpy(out, val, outlen);
}

// Boolean
int
tf_func_and (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    const char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (!bool_out) {
            return 0;
        }
        arg += arglens[i];
    }
    *out = 0;
    return 1;
}

int
tf_func_or (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;

    const char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (bool_out) {
            *out = 0;
            return 1;
        }
        arg += arglens[i];
    }
    *out = 0;
    return 0;
}

int
tf_func_not (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }
    int bool_out = 0;

    int len;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);
    *out = 0;
    return !bool_out;
}

int
tf_func_xor (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    int bool_out = 0;
    int result = 0;

    const char *arg = args;
    for (int i = 0; i < argc; i++) {
        int len;
        TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
        if (i == 0) {
            result = bool_out;
        }
        else {
            result ^= bool_out;
        }
        arg += arglens[i];
    }
    *out = 0;
    return result;
}

int
tf_func_fix_eol (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1 && argc != 2) {
        return -1;
    }

    int bool_out = 0;

    int len;

    char *p = out;
    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);

    char ind[TEMP_BUFFER_SIZE];
    int indlen = sizeof (ind);
    if (argc == 2) {
        TF_EVAL_CHECK(indlen, ctx, args + arglens[0], arglens[1], ind, indlen - 1, fail_on_undef);
    }
    else {
        strcpy (ind, " (...)");
        indlen = (int)strlen (ind);
    }

    for (int n = 0; n < len; n++, p++) {
        if (*p == '\n') {
            if (outlen-n < indlen) {
                *out = 0;
                return -1;
            }
            memcpy (p, ind, indlen);
            len = n + indlen;
            break;
        }
    }

    return len;
}

int
tf_func_hex (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1 && argc != 2) {
        return -1;
    }

    int bool_out = 0;

    int len;

    TF_EVAL_CHECK(len, ctx, args, arglens[0], out, outlen, fail_on_undef);
    int num = atoi (out);
    int pad = 0;
    *out = 0;

    if (argc == 2) {
        TF_EVAL_CHECK(len, ctx, args + arglens[0], arglens[1], out, outlen, fail_on_undef);
        if (!isdigit (*out)) {
            *out = 0;
            return -1;
        }
        pad = atoi (out);
        *out = 0;
    }

    int n = num;
    int cnt = 0;
    do {
        n >>= 4;
        cnt++;
    } while (n);

    char *p = out;

    if (pad > outlen || cnt > outlen) {
        return -1;
    }

    if (pad > cnt) {
        for (n = 0; n < pad-cnt; n++, p++) {
            *p = '0';
        }
    }
    p += cnt;
    *p-- = 0;

    n = num;

    const char hex[] = "0123456789abcdef";

    do {
        *p-- = hex[n & 0x0f];
        n >>= 4;
    } while (n);

    return (int)strlen (out);
}

int
tf_func_rgb (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 0 && argc != 3) {
        return -1;
    }

    int bool_out = 0;

    const char *arg = args;
    int len;

    int rgb[3];

    if (argc == 3) {
        int i;
        for (i = 0; i < 3; i++) {
            TF_EVAL_CHECK(len, ctx, arg, arglens[i], out, outlen, fail_on_undef);
            rgb[i] = atoi (out);
            *out = 0;

            arg += arglens[i];
        }
    } else {
        // Use -1 as use fg color marker
        rgb[0] = rgb[1] = rgb[2] = -1;
    }

    char rgbseq[20] = "";
    int rgbseqlen = 0;

    if (ctx->flags & DDB_TF_CONTEXT_TEXT_DIM) {
        // rgb color esc sequence
        // `\e2;R;G;Bm`
        snprintf (rgbseq, sizeof(rgbseq), "\033%d;%d;%d;%dm", DDB_TF_ESC_RGB, rgb[0], rgb[1], rgb[2]);
        rgbseqlen = (int)strlen (rgbseq);

        if (rgbseqlen > outlen) {
            return -1;
        }

        memcpy (out, rgbseq, rgbseqlen);
        out += rgbseqlen;
        ctx->dimmed = 1;
    }
    *out = 0;
    return rgbseqlen;
}

int
tf_func_year (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 1) {
        return -1;
    }

    int bool_out = 0;

    int len;
    char temp_str[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, args, arglens[0], temp_str, sizeof (temp_str) - 1, fail_on_undef);

    // Shorter than 4 characters? return nothing
    if (len < 4) {
        return 0;
    }

    // First 4 characters are digits? extract
    for (int i = 0; i < 4; i++) {
        if (!isdigit(temp_str[i])) {
            return 0;
        }
    }

    memcpy (out, temp_str, 4);
    return 4;
}

int
tf_func_itematindex (ddb_tf_context_t *ctx, int argc, const uint16_t *arglens, const char *args, char *out, int outlen, int fail_on_undef) {
    if (argc != 2) {
        return -1;
    }

    int bool_out = 0;

    int len;
    char temp_str[TEMP_BUFFER_SIZE];
    TF_EVAL_CHECK(len, ctx, args, arglens[0], temp_str, sizeof (temp_str) - 1, fail_on_undef);

    ddb_tf_context_int_t *priv = (ddb_tf_context_int_t *)ctx;
    priv->getting_item_at_index = 1;
    priv->item_at_index = atoi(temp_str);

    TF_EVAL_CHECK(len, ctx, args+arglens[0], arglens[1], out, outlen, fail_on_undef);

    priv->getting_item_at_index = 0;

    return len;
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
    // Boolean
    { "and", tf_func_and },
    { "or", tf_func_or },
    { "not", tf_func_not },
    { "xor", tf_func_xor },
    // String
    { "abbr", tf_func_abbr },
    { "ansi", tf_func_ansi },
    { "ascii", tf_func_ascii },
    { "caps", tf_func_caps },
    { "caps2", tf_func_caps2 },
    { "char", tf_func_char },
    { "crc32", tf_func_crc32 },
    { "crlf", tf_func_crlf },
    { "cut", tf_func_left },
    { "directory", tf_func_directory },
    { "directory_path", tf_func_directory_path },
    { "ext", tf_func_ext },
    { "filename", tf_func_filename },
    { "fix_eol", tf_func_fix_eol },
    { "hex", tf_func_hex },
    { "insert", tf_func_insert },
    { "left", tf_func_left }, // alias of 'cut'
    { "len", tf_func_len },
    { "len2", tf_func_len2 },
    { "longer", tf_func_longer },
    { "longest", tf_func_longest },
    { "lower", tf_func_lower },
    { "num", tf_func_num },
    { "pad", tf_func_pad },
    { "pad_right", tf_func_pad_right },
    { "padcut", tf_func_padcut },
    { "padcut_right", tf_func_padcut_right },
    { "progress", tf_func_progress },
    { "progress2", tf_func_progress2 },
    { "repeat", tf_func_repeat },
    { "replace", tf_func_replace },
    { "right", tf_func_right },
    { "roman", tf_func_roman },
    { "rot13", tf_func_rot13 },
    { "shortest", tf_func_shortest },
    { "strchr", tf_func_strchr },
    { "strcmp", tf_func_strcmp },
    { "stricmp", tf_func_stricmp },
    { "stripprefix", tf_func_stripprefix },
    { "strrchr", tf_func_strrchr },
    { "strstr", tf_func_strstr },
    { "substr", tf_func_substr },
    { "swapprefix", tf_func_swapprefix },
    { "tab", tf_func_tab },
    { "trim", tf_func_trim },
    { "upper", tf_func_upper },
    // Track info
    { "meta", tf_func_meta },
    { "channels", tf_func_channels },
    { "rgb", tf_func_rgb },
    { "year", tf_func_year },
    { "itematindex", tf_func_itematindex },
    { NULL, NULL }
};

static const char *
_tf_get_combined_value (playItem_t *it, const char *key, int *needs_free, int item_index) {
    DB_metaInfo_t *meta = pl_meta_for_key_with_override (it, key);

    if (!meta) {
        *needs_free = 0;
        return NULL;
    }

    size_t len = 0;

    const char *value = meta->value;
    const char *end = meta->value +meta->valuesize;

    // calculate size or return single value
    while (value < end) {
        size_t l = strlen (value);

        if (l+1 == meta->valuesize) {
            *needs_free = 0;
            return meta->value;
        }

        len += l;
        len += 2; // ", "
        value += l + 1;
    }

    char *out = malloc (len + 1);

    char *p = out;

    value = meta->value;
    end = meta->value +meta->valuesize;

    int index = 0;

    while (value < end) {
        len = strlen (value);
        if (item_index == -1 || index == item_index) {
            memcpy (p, value, value + len + 1 != end ? len : len + 1);
            p += len;
            if (item_index != -1) {
                break;
            }
        }
        if (item_index == -1 && value + len + 1 != end) {
            memcpy (p, ", ", 2);
            p += 2;
        }
        value += len + 1;
        index += 1;
    }
    *p = 0;
    *needs_free = 1;
    return out;
}

static int
format_playback_time (char *out, int outlen, float t) {
    int daystotal = (int)t / (3600*24);
    int hourtotal = ((int)t / 3600) % 24;
    int mintotal = ((int)t / 60) % 60;
    int sectotal = ((int)t) % 60;

    int len = 0;
    if (daystotal == 0) {
        len = snprintf_clip (out, outlen, "%d:%02d:%02d", hourtotal, mintotal, sectotal);
    }
    else if (daystotal == 1) {
        len = snprintf_clip (out, outlen, _("1 day %d:%02d:%02d"), hourtotal, mintotal, sectotal);
    }
    else {
        len = snprintf_clip (out, outlen, _("%d days %d:%02d:%02d"), daystotal, hourtotal, mintotal, sectotal);
    }

    return len;
}

/*
 * @param outlen bytes available in the buffer `out`, not including the terminating null byte
 * @returns bytes written to `out`, not including the terminating null byte,
 *          which is always written on nonnegative return
 */
static int
tf_eval_int (ddb_tf_context_t *ctx, const char *code, int size, char *out, int outlen, int *bool_out, int fail_on_undef) {
    playItem_t *it = (playItem_t *)ctx->it;
    char *init_out = out;
    *bool_out = 0;

    int count_true_conditionals = 0;
    int count_false_conditionals = 0;

    while (size) {
        if (*code) {
            // Plain text
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
            // Start of special block
            code++;
            size--;
            if (*code == 1) {
                // Function call
                code++;
                size--;
                tf_func_ptr_t func = tf_funcs[*code].func;
                code++;
                size--;
                uint16_t *arglens = NULL;
                char numargs = *code;
                if (numargs) {
                    // copy arg lengths, to make sure they're aligned
                    arglens = alloca (numargs * sizeof (uint16_t));
                    memcpy (arglens, code+1, numargs * sizeof (uint16_t));
                };
                int res = func (ctx, numargs, arglens, code+1+numargs*sizeof (uint16_t), out, outlen, fail_on_undef);
                if (res == -1) {
                    return -1;
                }
                if (res > 0) {
                    *bool_out = 1;
                    // hack for returning true + empty string result (e.g. tf_func_and)
                    if (*out == 0) {
                        res = 0;
                    }
                }

                out += res;
                outlen -= res;

                int blocksize = 1 + numargs*2;
                for (int i = 0; i < numargs; i++) {
                    blocksize += arglens[i];
                }
                code += blocksize;
                size -= blocksize;
            }
            else if (*code == 2) {
                // Meta field
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

                // locking: necessary because we use fast non-thread-safe APIs to access track metadata
                // warning: calling streamer requires unlocking.

                int pl_locked = 0;

                if (!(ctx->flags&DDB_TF_CONTEXT_NO_MUTEX_LOCK)
                    && !(ctx->flags&TF_INTERNAL_FLAG_LOCKED)) {
                    pl_lock ();
                    ctx->flags |= TF_INTERNAL_FLAG_LOCKED;
                    pl_locked = 1;
                }
                const char *val = NULL;
                int needs_free = 0;
                const char *aa_fields[] = { "album artist", "albumartist", "band", "artist", "composer", "performer", NULL };
                const char *a_fields[] = { "artist", "album artist", "albumartist", "band", "composer", "performer", NULL };
                const char *alb_fields[] = { "album", "venue", NULL };

                // set to 1 if special case handler successfully wrote the output
                int skip_out = 0;

                // temp vars used for strcmp optimizations
                int tmp_a = 0, tmp_b = 0, tmp_c = 0, tmp_d = 0, tmp_e = 0;
                int item_index = tf_item_index_for_context(ctx);
                if (!strcmp (name, aa_fields[0])) {
                    for (int i = 0; !val && aa_fields[i]; i++) {
                        val = _tf_get_combined_value(it, aa_fields[i], &needs_free, item_index);
                    }
                }
                else if (!strcmp (name, a_fields[0])) {
                    for (int i = 0; !val && a_fields[i]; i++) {
                        val = _tf_get_combined_value(it, a_fields[i], &needs_free, item_index);
                    }
                }
                else if (!strcmp (name, "album")) {
                    for (int i = 0; !val && alb_fields[i]; i++) {
                        val = _tf_get_combined_value (it, alb_fields[i], &needs_free, item_index);
                    }
                }
                else if (!strcmp (name, "track artist")) {
                    const char *aa = NULL;
                    for (int i = 0; !val && aa_fields[i]; i++) {
                        val = _tf_get_combined_value (it, aa_fields[i], &needs_free, item_index);
                    }
                    aa = val;
                    val = NULL;
                    for (int i = 0; !val && a_fields[i]; i++) {
                        val = _tf_get_combined_value (it, a_fields[i], &needs_free, item_index);
                    }
                    if (val && aa && !strcmp (val, aa)) {
                        val = NULL;
                    }
                }
                else if (!strcmp (name, "tracknumber")) {
                    const char *v = pl_find_meta_raw (it, "track");
                    if (v) {
                        const char *p = v;
                        while (*p) {
                            if (!isdigit (*p)) {
                                break;
                            }
                            p++;
                        }
                        if (p > v && *p == 0 && p-v == 1) {
                            int l = snprintf_clip (out, outlen, "%02d", atoi(v));
                            out += l;
                            outlen -= l;
                            skip_out = 1;
                        }
                        else {
                            val = v;
                        }
                    }
                }
                else if (!strcmp (name, "title")) {
                    val = _tf_get_combined_value (it, "title", &needs_free, item_index);
                    if (!val) {
                        const char *v = pl_find_meta_raw (it, ":URI");
                        if (v) {
                            const char *start = strrchr (v, '/');
                            if (start) {
                                start++;
                            }
                            else {
                                start = v;
                            }
                            const char *startcol = strrchr (v, ':');
                            if (startcol > start) {
                                start = startcol+1;
                            }
                            const char *end = strrchr (start, '.');
                            if (end) {
                                int n = (int)(end-start);
                                n = min (n, outlen);
                                n = u8_strnbcpy (out, start, n);
                                outlen -= n;
                                out += n;
                            }
                        }
                    }
                }
                else if (!strcmp (name, "discnumber")) {
                    val = pl_find_meta_raw (it, "disc");
                }
                else if (!strcmp (name, "totaldiscs")) {
                    val = pl_find_meta_raw (it, "numdiscs");
                }
                else if (!strcmp (name, "track number")) {
                    const char *v = pl_find_meta_raw (it, "track");
                    if (v) {
                        val = v;
                    }
                }
                else if (!strcmp (name, "date")) {
                    // NOTE: foobar2000 uses "date" instead of "year"
                    // so for %date% we simply return the content of "year"
                    val = pl_find_meta_raw (it, "year");
                }
                else if (!strcmp (name, "samplerate")) {
                    val = pl_find_meta_raw (it, ":SAMPLERATE");
                }
                else if (!strcmp (name, "playback_bitrate")) {
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_unlock();
                    }

                    playItem_t *playing_track = streamer_get_playing_track();

                    if (playing_track) {
                        int br = streamer_get_apx_bitrate();
                        if (br >= 0) {
                            int l = snprintf_clip (out, outlen, "%d", br);
                            out += l;
                            outlen -= l;
                            skip_out = 1;
                        }
                        pl_item_unref (playing_track);
                    }
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_lock ();
                    }
                }
                else if (!strcmp (name, "bitrate")) {
                    val = pl_find_meta_raw (it, ":BITRATE");
                }
                else if (!strcmp (name, "filesize")) {
                    val = pl_find_meta_raw (it, ":FILE_SIZE");
                }
                else if (!strcmp (name, "filesize_natural")) {
                    const char *v = pl_find_meta_raw (it, ":FILE_SIZE");
                    if (v) {
                        int64_t bs = atoll (v);
                        int l;
                        if (bs >= 1024*1024*1024) {
                            double gb = (double)bs / (double)(1024*1024*1024);
                            l = snprintf_clip (out, outlen, "%.3lf GB", gb);
                        }
                        else if (bs >= 1024*1024) {
                            double mb = (double)bs / (double)(1024*1024);
                            l = snprintf_clip (out, outlen, "%.3lf MB", mb);
                        }
                        else if (bs >= 1024) {
                            double kb = (double)bs / (double)(1024);
                            l = snprintf_clip (out, outlen, "%.3lf KB", kb);
                        }
                        else {
                            l = snprintf_clip (out, outlen, "%lld B", bs);
                        }
                        out += l;
                        outlen -= l;
                        skip_out = 1;
                    }
                }
                else if (!strcmp (name, "channels")) {
                    val = tf_get_channels_string_for_track (it);
                }
                else if (!strcmp (name, "codec")) {
                    val = pl_find_meta (it, ":FILETYPE");
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
                else if ((tmp_a = !strcmp (name, "playback_time")) || (tmp_b = !strcmp (name, "playback_time_seconds")) || (tmp_c = !strcmp (name, "playback_time_remaining")) || (tmp_d = !strcmp (name, "playback_time_remaining_seconds")) || (tmp_e = !strcmp (name, "playback_time_ms"))) {
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_unlock();
                    }
                    playItem_t *playing = streamer_get_playing_track ();
                    if (it && playing == it && !(ctx->flags & DDB_TF_CONTEXT_NO_DYNAMIC)) {
                        float t = streamer_get_playpos ();
                        if (tmp_c || tmp_d) {
                            float dur = pl_get_item_duration (it);
                            t = dur - t;
                        }
                        if (t >= 0) {
                            int l = 0;
                            if (tmp_a || tmp_c || tmp_e) {
                                int hr = (int)(t/3600);
                                int mn = (int)((t-hr*3600)/60);
                                int sc = (int)(t-hr*3600-mn*60);
                                if (tmp_e) {
                                    int ms = (int)((t-hr*3600-mn*60-sc)*1000);
                                    if (hr) {
                                        l = snprintf_clip (out, outlen, "%d:%02d:%02d.%03d", hr, mn, sc, ms);
                                    }
                                    else {
                                        l = snprintf_clip (out, outlen, "%d:%02d.%03d", mn, sc, ms);
                                    }
                                } else {
                                    if (hr) {
                                        l = snprintf_clip (out, outlen, "%d:%02d:%02d", hr, mn, sc);
                                    }
                                    else {
                                        l = snprintf_clip (out, outlen, "%d:%02d", mn, sc);
                                    }
                                }
                            }
                            else if (tmp_b || tmp_d) {
                                l = snprintf_clip (out, outlen, "%0.2f", t);
                            }
                            out += l;
                            outlen -= l;
                            skip_out = 1;
                            // notify the caller about update interval
                            if (!ctx->update || (ctx->update > 1000)) {
                                ctx->update = tmp_e ? 100 : 1000;
                            }
                        }
                    }
                    if (playing) {
                        pl_item_unref (playing);
                    }
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_lock();
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
                        int hr = (int)(t/3600);
                        int mn = (int)((t-hr*3600)/60);
                        int sc = (int)(tmp_a ? t-hr*3600-mn*60 : t-hr*3600-mn*60);
                        int ms = (int)(tmp_b ? (t-hr*3600-mn*60-sc) * 1000.f : 0);
                        int l = 0;
                        if (tmp_a) {
                            if (hr) {
                                l = snprintf_clip (out, outlen, "%d:%02d:%02d", hr, mn, sc);
                            }
                            else {
                                l = snprintf_clip (out, outlen, "%d:%02d", mn, sc);
                            }
                        }
                        else if (tmp_b) {
                            if (hr) {
                                l = snprintf_clip (out, outlen, "%d:%02d:%02d.%03d", hr, mn, sc, ms);
                            }
                            else {
                                l = snprintf_clip (out, outlen, "%d:%02d.%03d", mn, sc, ms);
                            }
                        }
                        out += l;
                        outlen -= l;
                        skip_out = 1;
                    }
                }
                else if ((tmp_a = !strcmp (name, "length_seconds") || (tmp_b = !strcmp (name, "length_seconds_fp")))) {
                    float t = pl_get_item_duration (it);
                    if (t >= 0) {
                        int l;
                        if (tmp_a) {
                            l = snprintf_clip (out, outlen, "%d", (int)roundf(t));
                        }
                        else {
                            l = snprintf_clip (out, outlen, "%0.3f", t);
                        }
                        out += l;
                        outlen -= l;
                        skip_out = 1;
                    }
                }
                else if (!strcmp (name, "length_samples")) {
                    int l = snprintf_clip (out, outlen, "%lld", pl_item_get_endsample ((playItem_t *)ctx->it) - pl_item_get_startsample ((playItem_t *)ctx->it));
                    out += l;
                    outlen -= l;
                    skip_out = 1;
                }
                else if (!strcmp (name, "isplaying")) {
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_unlock();
                    }

                    playItem_t *playing = streamer_get_playing_track ();
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_lock();
                    }

                    if (playing != NULL && ctx->it == (ddb_playItem_t *)playing) {
                        *out++ = '1';
                        outlen--;
                        skip_out = 1;
                    }
                    if (playing != NULL) {
                        pl_item_unref (playing);
                    }
                }
                else if (!strcmp (name, "ispaused")) {
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_unlock();
                    }
                    playItem_t *playing = streamer_get_playing_track ();
                    if (ctx->flags & TF_INTERNAL_FLAG_LOCKED) {
                        pl_lock();
                    }

                    if (playing != NULL && ctx->it == (ddb_playItem_t *)playing && plug_get_output ()->state () == DDB_PLAYBACK_STATE_PAUSED) {
                        *out++ = '1';
                        outlen--;
                        skip_out = 1;
                    }
                    if (playing != NULL) {
                        pl_item_unref (playing);
                    }
                }
                else if (!strcmp (name, "filename")) {
                    const char *v = pl_find_meta_raw (it, ":URI");
                    if (v) {
                        const char *start = strrchr (v, '/');
                        if (start) {
                            start++;
                        }
                        else {
                            start = v;
                        }
                        const char *end = strrchr (start, '.');
                        if (end) {
                            tf_append_out(&out, &outlen, start, (int)(end-start));
                            skip_out = 1;
                        }
                    }
                }
                else if (!strcmp (name, "filename_ext")) {
                    const char *v = pl_find_meta_raw (it, ":URI");
                    if (v) {
                        const char *start = strrchr (v, '/');
                        if (start) {
                            start++;
                        }
                        else {
                            start = v;
                        }
                        tf_append_out (&out, &outlen, start, (int)strlen (start));
                        skip_out = 1;
                    }
                }
                else if (!strcmp (name, "directoryname")) {
                    const char *v = pl_find_meta_raw (it, ":URI");
                    if (v) {
                        const char *end = strrchr (v, '/');
                        if (end) {
                            const char *start = end - 1;
                            while (start >= v && *start != '/') {
                                start--;
                            }
                            if (start && start != end) {
                                start++;
                                tf_append_out(&out, &outlen, start, (int)(end-start));
                                skip_out = 1;
                            }
                        }
                    }
                }
                else if (!strcmp (name, "last_modified")) {
                    const char *v = pl_find_meta_raw (it, ":URI");
                    if (v) {
                        if (!strncmp (v, "file://", 7)) {
                            v += 7;
                        }
                        struct stat sb;
                        struct tm localtm;
                        char mtime[20];
                        if (!stat (v, &sb) && localtime_r (&sb.st_mtime, &localtm) != NULL
                                && strftime (mtime, sizeof(mtime), "%F %T", &localtm)) {
                            tf_append_out (&out, &outlen, mtime, (int)strlen (mtime));
                            skip_out = 1;
                        }
                    }
                }
                else if (!strcmp (name, "_path_raw")) {
                    const char *v = pl_find_meta_raw (it, ":URI");

                    if (v) {
                        #ifdef _WIN32
                        int is_absolute = (isalpha (v[0]) && v[1] == ':' && v[2] == '/');
                        #else
                        int is_absolute = (v[0] == '/');
                        #endif

                        if (is_absolute) {
                            // This is an absolute path, just prepend proper prefix
                            #ifdef _WIN32
                            const char prefix[] = "file:///";
                            #else
                            const char prefix[] = "file://";
                            #endif
                            tf_append_out (&out, &outlen, prefix, sizeof (prefix) - 1);
                            tf_append_out (&out, &outlen, v, (int)strlen (v));
                        }
                        else {
                            int is_uri = 1;

                            for (const char *p = v; p; p++) {
                                if (!strncmp (p, "://", 3)) {
                                    break;
                                }

                                if (!isalpha (*p)) {
                                    is_uri = 0;
                                    break;
                                }
                            }

                            if (is_uri) {
                                // This is already a URI, just copy as is
                                tf_append_out (&out, &outlen, v, (int)strlen (v));
                            }
                            else {
                                // Relative paths are considered invalid
                            }
                        }

                        skip_out = 1;
                    }
                }
                else if (!strcmp (name, "path")) {
                    val = pl_find_meta_raw (it, ":URI");

                    // strip file://
                    if (val && !strncmp (val, "file://", 7)) {
                        val += 7;
                    }
#if 0
                    // strip any URI scheme
                    if (isalpha (*val)) {
                        const char *slash = strchr (val, ':');
                        if (slash && strlen (slash) > 3 && slash[1] == '/' && slash[2] == '/') {
                            const char *p = val;
                            while (p < slash) {
                                if (!isalpha (*p) && !isdigit (*p) && !strchr ("+.-", *p)) {
                                    p = NULL;
                                    break;
                                }
                                p++;
                            }
                            if (p) {
                                val = slash + 3;
                            }
                        }
                    }
#endif
                }
                // index of track in playlist (zero-padded)
                else if (!strcmp (name, "list_index")) {
                    if (it) {
                        int total_tracks = plt_get_item_count ((playlist_t *)ctx->plt, ctx->iter);
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
                        int l = snprintf_clip (out, outlen, "%0*d", digits, idx);
                        out += l;
                        outlen -= l;
                        skip_out = 1;
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
                        int l = snprintf_clip (out, outlen, "%d", total_tracks);
                        out += l;
                        outlen -= l;
                        skip_out = 1;
                    }
                }
                // index of track in queue
                else if (!strcmp (name, "queue_index")) {
                    if (it) {
                        int idx = playqueue_test (it) + 1;
                        if (idx >= 1) {
                            int l = snprintf_clip (out, outlen, "%d", idx);
                            out += l;
                            outlen -= l;
                            skip_out = 1;
                        }
                    }
                }
                // indexes of track in queue
                else if (!strcmp (name, "queue_indexes")) {
                    if (it) {
                        int idx = playqueue_test (it) + 1;
                        if (idx >= 1) {
                            int l = snprintf_clip (out, outlen, "%d", idx);
                            out += l;
                            outlen -= l;
                            int count = playqueue_getcount ();
                            for (int i = idx; i < count; i++) {
                                playItem_t *trk = playqueue_get_item (i);
                                if (trk) {
                                    if (it == trk) {
                                        l = snprintf_clip (out, outlen, ",%d", i + 1);
                                        out += l;
                                        outlen -= l;
                                    }
                                    pl_item_unref (trk);
                                }
                            }
                            skip_out = 1;
                        }
                    }
                }
                // total amount of tracks in queue
                else if (!strcmp (name, "queue_total")) {
                    int count = playqueue_getcount ();
                    if (count >= 0) {
                        int l = snprintf_clip (out, outlen, "%d", count);
                        out += l;
                        outlen -= l;
                        skip_out = 1;
                    }
                }
                else if (!strcmp (name, "_deadbeef_version")) {
                    val = VERSION;
                }
                else if (!strcmp (name, "_playlist_name")) {
                    val = ((playlist_t *)ctx->plt)->title;
                }
                else if (!strcmp (name, "selection_playback_time")) {
                    float seltime = plt_get_selection_playback_time((playlist_t *)ctx->plt);

                    int l = format_playback_time (out, outlen, seltime);

                    out += l;
                    outlen -= l;
                    skip_out = 1;
                }
                else {
                    val = _tf_get_combined_value (it, name, &needs_free, item_index);
                }

                if (val || (!val && out > init_out)) {
                    *bool_out = 1;
                }

                // default case
                if (!skip_out && val) {
                    int32_t l = u8_strnbcpy (out, val, outlen);

                    if (ctx->metadata_transformer != NULL && outlen > 0) {
                        ctx->metadata_transformer(ctx, out, l);
                    }

                    out += l;
                    outlen -= l;
                }
                if (pl_locked) {
                    pl_unlock ();
                    ctx->flags &= ~TF_INTERNAL_FLAG_LOCKED;
                }
                if (!skip_out && !val && fail_on_undef) {
                    return -1;
                }

                if (val && needs_free) {
                    free ((char *)val);
                }

                code += len;
                size -= len;
            }
            else if (*code == 3) { // conditional expression
                code++;
                size--;
                int32_t len;
                memcpy (&len, code, 4);
                code += 4;
                size -= 4;

                int ignored_bool_out = 0;
                int res = tf_eval_int (ctx, code, len, out, outlen, &ignored_bool_out, 1);
                if (res >= 0) {
                    out += res;
                    outlen -= res;
                    count_true_conditionals++;
                }
                else if (res < 0) {
                    count_false_conditionals++;
                }

                code += len;
                size -= len;
            }
            else if (*code == 4) { // preformatted text
                code++;
                size--;
                int32_t len;
                memcpy (&len, code, 4);
                code += 4;
                size -= 4;
                int32_t l = u8_strnbcpy(out, code, len);
                out += l;
                outlen -= l;
                code += len;
                size -= len;
            }
            else if (*code == 5) { // dimming of text
                code++;
                size--;

                int8_t amount = *code;

                code++;
                size--;

                char dim[10] = "";
                char undim[10] = "";
                int dimlen = 0;
                int undimlen = 0;

                if (ctx->flags & DDB_TF_CONTEXT_TEXT_DIM) {
                    // text dimming esc sequence
                    // `\eX,Ym` where X and Y are numbers, which can be negative
                    snprintf (dim, sizeof(dim), "\033%d;%dm", DDB_TF_ESC_DIM, (int)amount);
                    snprintf (undim, sizeof(undim), "\033%d;%dm", DDB_TF_ESC_DIM, -(int)amount);
                    dimlen = (int)strlen (dim);
                    undimlen = (int)strlen (undim);

                    if (dimlen + undimlen > outlen) {
                        return -1;
                    }

                    memcpy (out, dim, dimlen);
                    out += dimlen;
                    outlen -= dimlen;
                    ctx->dimmed = 1;
                }

                int32_t len;
                memcpy (&len, code, 4);
                code += 4;
                size -= 4;

                int ignored_bool_out = 0;
                int res = tf_eval_int (ctx, code, len, out, outlen - dimlen, &ignored_bool_out, fail_on_undef);
                if (res >= 0) {
                    out += res;
                    outlen -= res;
                    count_true_conditionals++;
                }
                else if (res < 0) {
                    count_false_conditionals++;
                }
                code += len;
                size -= len;

                if ((ctx->flags & DDB_TF_CONTEXT_TEXT_DIM) && outlen >= undimlen) {
                    memcpy (out, undim, undimlen);
                    out += undimlen;
                    outlen -= undimlen;
                }
            }
            else {
                return -1;
            }
        }
    }
    // Always null terminate when there's no error
    *out = 0;

    if (fail_on_undef && count_false_conditionals > 0 && count_true_conditionals == 0) {
        return -1;
    }

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
            *(c->o++) = (char)i;
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
    uint8_t *start = c->o;
    *(c->o++) = 0; // num args
    uint8_t *argstart = c->o;

    uint16_t *arglens = (uint16_t *)c->o;

    //parse comma separated args until )
    while (*(c->i)) {
        if (*(c->i) == ',' || *(c->i) == ')') {
            // next arg
            int len = (int)(c->o - argstart);

            // special case for empty argument list
            if (len == 0 && *(c->i) == ')' && (*start) == 0) {
                break;
            }
            assert(len<=0xffff);

            // expand arg lengths buffer by 1
            int num_args = *start;

            memmove (arglens+num_args+1, arglens+num_args, c->o - start - num_args*sizeof(uint16_t));
            c->o += 2;
            // store arg length
            // FIXME: `arglens` comes in a byte stream without 16 bit alignment, this may cause unaligned access and crash.
            uint16_t len16 = (uint16_t)len;
            memcpy(&arglens[*start], &len16, sizeof (uint16_t));
            (*start)++; // num args++
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
    uint8_t *plen = c->o;
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
    *plen = (char)len;

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

    uint8_t *plen = c->o;
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
tf_compile_text_dim (tf_compiler_t *c) {
    *(c->o++) = 0;
    *(c->o++) = 5;

    uint8_t *pamount = c->o;
    c->o++;

    uint8_t *plen = c->o;
    c->o += 4;

    char *start = c->o;

    char marker = *(c->i);
    // count number of leading '<'
    int count = 0;
    while (*(c->i) && *(c->i) == marker) {
        c->i++;
        count++;
    }

    marker = marker == '<' ? '>' : '<';

    while (*(c->i)) {
        if (*(c->i) == '\\') {
            c->i++;
            if (*(c->i) != 0) {
                *(c->o++) = *(c->i++);
            }
        }
        else if (*(c->i) == marker) {
            int cnt = 0;
            while (*(c->i) && *(c->i) == marker && cnt < count) {
                cnt ++;
                c->i++;
            }
            if (cnt != count) {
                return -1;
            }
            break;
        }
        else if (tf_compile_plain (c)) {
            return -1;
        }
    }

    if (marker == '>') {
        count = -count;
    }

    *pamount = (int8_t)count;

    int32_t len = (int32_t)(c->o - plen - 4);
    memcpy (plen, &len, 4);

    char value[len+1];
    memcpy (value, start, len);
    value[len] = 0;
    return 0;
}

int
tf_compile_plain (tf_compiler_t *c) {
    int eol = c->eol;
    c->eol = 0;
    char i = *(c->i);
    if (i == '$') {
        if (c->i[1] == '$') {
            c->i++;
            *(c->o++) = *(c->i++);
        }
        else if (tf_compile_func (c)) {
            return -1;
        }
    }
    else if (i == '[') {
        if (tf_compile_ifdef (c)) {
            return -1;
        }
    }
    else if (i == '%') {
        if (c->i[1] == '%') {
            c->i++;
            *(c->o++) = *(c->i++);
            return 0;
        }
        if (tf_compile_field (c)) {
            return -1;
        }
    }
    // FIXME this is not fb2k spec
    else if (*(c->i) == '\\') {
        c->i++;
        if (*(c->i) != 0) {
            *(c->o++) = *(c->i++);
        }
    }
    else if (eol && i == '/' && c->i[1] == '/') {
        // skip to end of line
        while (c->i[0] && c->i[0] != '\n') {
            c->i++;
        }
        c->eol = 1;
    }
    else if (i == '\'') {
        // copy as plain text to next single-quote
        c->i++;

        if (c->i[0] == '\'') {
            *(c->o++) = *(c->i++);
        }
        else {
            while (c->i[0] && c->i[0] != '\'') {
                *(c->o++) = *(c->i++);
            }
            if (c->i[0] == '\'') {
                c->i++;
            }
        }
    }
    else if (i == '\n') {
        c->i++;
        c->eol = 1;
    }
    else if (i == '<' || i == '>') {
        if (tf_compile_text_dim (c)) {
            return -1;
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

    size_t len = strlen(script);
    if (len == 0) {
        return calloc(1,4);
    }
    uint8_t *code = calloc(len * 3, 1);

    c.o = code;

    c.eol = 1;

    while (*(c.i)) {
        if (tf_compile_plain (&c)) {
            trace ("tf: compilation failed <%s>\n", c.i);
            free (code);
            return NULL;
        }
    }

    size_t size = c.o - code;
    char *out = malloc (size + 8);
    memcpy (out + 4, code, size);
    memset (out + 4 + size, 0, 4); // FIXME: this is the padding for possible buffer overflow bug fix
    *((int32_t *)out) = (int32_t)(size);

    free (code);

    return out;
}

void
tf_free (char *code) {
    free (code);
}

void
tf_import_legacy (const char *fmt, char *out, int outsize) {
    while (*fmt && outsize > 1) {
        if (*fmt == '\'' || *fmt == '$') {
            if (outsize < 3) {
                break;
            }
            *out++ = *fmt;
            *out++ = *fmt++;
            outsize -= 2;
        }
        else if (*fmt == '\n') {
            if (outsize < 7) {
                break;
            }
            strcpy (out, "$crlf()");
            out += 7;
            outsize -= 7;
            fmt++;
        }
        else if (*fmt == '[') {
            if (outsize < 3) {
                break;
            }
            strcpy (out, "'['");
            out += 3;
            outsize -= 3;
            fmt++;
        }
        else if (*fmt == ']') {
            if (outsize < 3) {
                break;
            }
            strcpy (out, "']'");
            out += 3;
            outsize -= 3;
            fmt++;
        }
        else if (*fmt != '%') {
            *out++ = *fmt++;
            outsize--;
        }
        else {
            fmt++;
            if (!*fmt) {
                break;
            }
            if (*fmt == '@') {
                const char *e = fmt;
                e++;
                while (*e && *e != '@') {
                    e++;
                }
#define APPEND(x) {size_t size = strlen (x); if (size >= outsize-1) break; memcpy (out, x, size); out += size; outsize -= size;}
                if (*e == '@') {
                    char nm[100];
                    size_t l = e-fmt-1;
                    l = min (l, sizeof (nm)-3);
                    strncpy (nm+1, fmt+1, l);
                    nm[l+2] = 0;
                    nm[0] = '%';
                    nm[l+1] = '%';

                    APPEND (nm);
                    fmt = e+1;
                }
                continue;
            }
            else if (*fmt == 'a') {
                APPEND ("%artist%");
            }
            else if (*fmt == 't') {
                APPEND ("%title%");
            }
            else if (*fmt == 'b') {
                APPEND ("%album%");
            }
            else if (*fmt == 'B') {
                APPEND ("%album artist%");
            }
            else if (*fmt == 'C') {
                APPEND ("%composer%");
            }
            else if (*fmt == 'n') {
                APPEND ("%tracknumber%");
            }
            else if (*fmt == 'N') {
                APPEND ("%numtracks%");
            }
            else if (*fmt == 'y') {
                APPEND ("%date%");
            }
            else if (*fmt == 'Y') {
                APPEND ("%original_release_time%");
            }
            else if (*fmt == 'g') {
                APPEND ("%genre%");
            }
            else if (*fmt == 'c') {
                APPEND ("%comment%");
            }
            else if (*fmt == 'r') {
                APPEND ("%copyright%");
            }
            else if (*fmt == 'l') {
                APPEND ("%length%");
            }
            else if (*fmt == 'e') {
                APPEND ("%playback_time%");
            }
            else if (*fmt == 'f') {
                APPEND ("%filename_ext%");
            }
            else if (*fmt == 'F') {
                APPEND ("%_path_raw%");
            }
            else if (*fmt == 'T') {
                APPEND ("$info(tagtype)");
            }
            else if (*fmt == 'd') {
                APPEND ("%directoryname%");
            }
            else if (*fmt == 'D') {
                APPEND ("$directory_path(%_path_raw%)");
            }
            else if (*fmt == 'L') {
                APPEND ("%list_length%"); // TODO
            }
            else if (*fmt == 'X') {
                APPEND ("%list_selected%"); // TODO
            }
            else if (*fmt == 'Z') {
                APPEND ("$channels()");
            }
            else if (*fmt == 'V') {
                APPEND ("%_deadbeef_version%");
            }
            else if (*fmt == '%') {
                APPEND ("%%");
            }
            else {
                *out++ = *fmt;
                outsize--;
            }
#undef APPEND
            fmt++;
        }
    }
    *out = 0;
}
