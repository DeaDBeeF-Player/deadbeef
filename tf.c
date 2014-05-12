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
//   field name\0
//  3: if_defined block
//   len: int32
// !0: plain text

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *i;
    char *o;
} tf_compiler_t;

int
tf_compile_func (tf_compiler_t *c) {
    printf ("start func\n");
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

    char func_name[c->i - name_start + 1];
    memcpy (func_name, name_start, c->i-name_start);
    func_name[c->i-name_start] = 0;
    printf ("func name: %s\n", func_name);

    c->i++;

    printf ("reading args, starting with %c\n", (*c->i));
    
    // remember ptr and start reading args
    char *start = c->o;
    *(c->o++) = 0; // num args
    char *argstart = c->o;

    //parse comma separated args until )
    char *prev_arg;
    while (*(c->i)) {
        if (*(c->i) == ',' || *(c->i) == ')') {
            // next arg
            int len = c->o - argstart;
            printf ("end of arg in func %s, len: %d\n", func_name, len);
            printf ("parsed arg: %s\n", start+(*start)+1);
            if (*(c->i) != ')' || len) {
                // expand arg lengths buffer by 1
                memmove (start+(*start)+2, start+(*start)+1, c->o - start - (*start));
                c->o++;
                (*start)++; // num args++
                // store arg length
                start[(*start)] = len;
                argstart = c->o;
                printf ("numargs: %d\n", (int)(*start));
            }

            if (*(c->i) == ')') {
                c->i++;
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

    printf ("$%s num_args: %d\n", func_name, (int)*start);

    return 0;
}

inline int
tf_compile_plain (tf_compiler_t *c) {
    if (*(c->i) == '$') {
        if (tf_compile_func (c)) {
            return -1;
        }
    }
    else if (*(c->i) == '[') {
        //p = tf_compile_ifdef ();
        c->i++;
    }
    else if (*(c->i) == '%') {
        //p = tf_compile_field();
        c->i++;
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

void
tf_compile (const char *script) {
    tf_compiler_t c;
    memset (&c, 0, sizeof (c));

    c.i = script;

    char code[strlen(script) * 3];
    memset (code, 0, sizeof (code));

    c.o = code;

    while (*(c.i)) {
        tf_compile_plain (&c);
    }

    printf ("output len: %d\n", (int)(c.o - code));
    printf ("%s\n", code);
}

int main () {
    tf_compile ("hello$world(abra,kadabra,$add(1,2))");
}
