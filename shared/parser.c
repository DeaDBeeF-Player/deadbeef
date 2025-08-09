/*
    Simple text parsing library
    Copyright (C) 2009-2014 Oleksiy Yakovenko

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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"

// very basic parser, ripped from psynth, optimized, and extended to support
// quoted strings and extra special chars
int parser_line;

void
parser_init (void) {
    parser_line = 1;
}

const char *
skipws (const char *p) {
    while (*p <= ' ' && *p) {
        if (*p == '\n') {
            parser_line++;
        }
        p++;
    }
    if (!*p) {
        return NULL;
    }
    return p;
}

const char *
gettoken_ext (const char *p, char *tok, const char *specialchars) {
    const char *c;
    assert (p);
    assert (tok);
    int n = MAX_TOKEN-1;
    if (!(p = skipws (p))) {
        return NULL;
    }
    if (*p == '"') {
        p++;
        c = p;
        while (n > 0 && *c && *c != '"') {
            if (*c == '\n') {
                parser_line++;
            }
            if (*c == '\\' && (*(c+1) == '"' || *(c+1) == '\\')) {
                c++;
            }
            *tok++ = *c++;
            n--;
        }
        if (*c) {
            c++;
        }
        *tok = 0;
        return c;
    }
    if (strchr (specialchars, *p)) {
        *tok = *p;
        tok[1] = 0;
        return p+1;
    }
    c = p;
    while (n > 0 && *c > ' ' && !strchr (specialchars, *c)) {
        *tok++ = *c++;
        n--;
    }
    *tok = 0;
    return c;
}

const char *
gettoken (const char *p, char *tok) {
    char specialchars[] = "{}();";
    return gettoken_ext (p, tok, specialchars);
}

const char *
gettoken_keyvalue (const char *p, char *key, char *val) {
    char specialchars[] = "{}();=";
    p = gettoken_ext (p, key, specialchars);
    if (!p) {
        return NULL;
    }
    p = gettoken_ext (p, val, specialchars);
    if (!p || *val != '=') {
        return NULL;
    }
    return gettoken_ext (p, val, specialchars);
}

const char *
gettoken_warn_eof (const char *p, char *tok) {
    p = gettoken (p, tok);
    if (!p) {
        fprintf (stderr, "parser: unexpected eof at line %d\n", parser_line);
    }
    return p;
}

const char *
gettoken_err_eof (const char *p, char *tok) {
    p = gettoken (p, tok);
    if (!p) {
        fprintf (stderr, "parser: unexpected eof at line %d\n", parser_line);
        exit (-1);
    }
    return p;
}

char *
parser_escape_string (const char *in) {
    char *output;
    size_t len = 0;
    const char *p;
    for (p = in; *p; p++, len++) {
        if (*p == '"' || *p == '\\') {
            len++;
        }
    }
    output = malloc (len + 1);
    char *out = output;
    for (p = in; *p; p++) {
        if (*p == '"' || *p == '\\') {
            *out++ = '\\';
        }
        *out++ = *p;
    }
    *out = 0;
    return output;
}

void
parser_unescape_quoted_string (char *in) {
    char *out = in;
    char *next = in;
    if (next[0] == '"') {
        next++;
    }
    while (*next && *next != '"') {
        if (*next == '\\' && (*(next+1) == '"' || *(next+1) == '\\')) {
            next++;
        }
        *out++ = *next++;
    }
    *out = '\0';
}
