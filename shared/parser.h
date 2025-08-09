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
#ifndef __PARSER_H
#define __PARSER_H

#define MAX_TOKEN 256
extern int parser_line;

void
parser_init (void);

const char *
skipws (const char *p);

const char *
gettoken (const char *p, char *tok);

const char *
gettoken_ext (const char *p, char *tok, const char *specialchars);

const char *
gettoken_keyvalue (const char *p, char *key, char *val);

const char *
gettoken_warn_eof (const char *p, char *tok);

const char *
gettoken_err_eof (const char *p, char *tok);

// escape '"' and '\\' with '\\'
// returns allocated buffer, must be freed by the caller
char *
parser_escape_string (const char *in);

// reverses parser_escape_string
// modified the input in-place
void
parser_unescape_quoted_string (char *in);

#endif
