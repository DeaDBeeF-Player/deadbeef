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
#include "playlist.h"
#include "tf.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

typedef struct {
    const char *i;
    char *o;
} tf_compiler_t;

typedef int (*tf_func_ptr_t)(tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef);

#define TF_MAX_FUNCS 0xff

typedef struct {
    const char *name;
    tf_func_ptr_t func;
} tf_func_def;

int
tf_eval_int (tf_context_t *ctx, char *code, int size, char *out, int outlen, int fail_on_undef);

int
tf_eval (tf_context_t *ctx, char *code, int codelen, char *out, int outlen) {
    return tf_eval_int (ctx, code, codelen, out, outlen, 0);
}

int
tf_func_add (tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
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
        arg += arglens[i];
    }
    int res = snprintf (out, outlen, "%d", outval);
    trace ("and of add (%d), res: %d\n", outval, res);
    return res;
}

int
tf_func_if (tf_context_t *ctx, int argc, char *arglens, char *args, char *out, int outlen, int fail_on_undef) {
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
    { "add", tf_func_add },
    { "if", tf_func_if },
    { NULL, NULL }
};

int
tf_eval_int (tf_context_t *ctx, char *code, int size, char *out, int outlen, int fail_on_undef) {
    char *init_out = out;
    while (size) {
        if (*code) {
            trace ("free char: %c\n", *code);
            *out++ = *code++;
            size--;
            outlen++;
        }
        else {
            *code++;
            size--;
            trace ("special: %d\n", (int)(*code));
            if (*code == 1) {
                *code++;
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

                const char *val = pl_find_meta (ctx->it, name);
                if (val) {
                    int len = strlen (val);
                    memcpy (out, val, len);
                    out[len] = 0;
                    out += len;
                    outlen -= len;
                }
                else if (fail_on_undef) {
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
                code += len;
                size -= len;
            }
            else {
                trace ("invalid special block: %d\n", (int)(*code));
                return -1;
            }
        }
    }
    return out-init_out;
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
        int l = strlen (tf_funcs[i].name);
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
    char *prev_arg;
    while (*(c->i)) {
        if (*(c->i) == '\\') {
            c->i++;
            if (*(c->i) != 0) {
                *(c->o++) = *(c->i++);
            }
        }
        else if (*(c->i) == ',' || *(c->i) == ')') {
            // next arg
            int len = c->o - argstart;
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

    int32_t len = c->o - plen - 1;
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

    int32_t len = c->o - plen - 4;
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
    return c.o - code;
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

    tf_context_t ctx;
    memset (&ctx, 0, sizeof (ctx));
    ctx._size = sizeof (ctx);
    ctx.idx = -1;

    char out[1000] = "";
    int res = tf_eval (&ctx, code, len, out, sizeof (out));
    trace ("output (%d): %s\n", res, out);
}
