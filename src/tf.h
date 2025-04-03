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

#ifndef __TF_H
#define __TF_H

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

void
tf_init (void);

void
tf_deinit (void);

// compile the input title formatting string into bytecode
// script: freeform string with title formatting special characters in it
// returns the pointer to compiled bytecode, which must be tf_free'd by the caller.
char *
tf_compile (const char *script);

void
tf_free (char *code);

// evaluate the titleformatting script in a given context
// ctx: a pointer to ddb_tf_context_t structure initialized by the caller
// code: the bytecode data created by tf_compile
// out: buffer allocated by the caller, must be big enough to fit the output string
// outlen: the size of out buffer
// returns -1 on fail, output size on success
int
tf_eval (ddb_tf_context_t *ctx, const char *code, char *out, int outlen);

// convert legacy title formatting to the new format, usable with tf_compile
void
tf_import_legacy (const char *fmt, char *out, int outsize);

#ifdef __cplusplus
}
#endif

#endif // __TF_H
