/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
gettoken (const char *p, char *tok) {
    const char *c;
    assert (p);
    assert (tok);
    int n = MAX_TOKEN-1;
    char specialchars[] = "{}();";
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
gettoken_warn_eof (const char *p, char *tok) {
    p = gettoken (p, tok);
    if (!p) {
        fprintf (stderr, "parser: unexpected eof at line %d", parser_line);
    }
    return p;
}

const char *
gettoken_err_eof (const char *p, char *tok) {
    p = gettoken (p, tok);
    if (!p) {
        fprintf (stderr, "parser: unexpected eof at line %d", parser_line);
        exit (-1);
    }
    return p;
}


