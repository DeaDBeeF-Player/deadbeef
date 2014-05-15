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

#ifndef __TF_H
#define __TF_H

// returns the output bytecode size.
// the resulting bytecode is stored in the *out_code.
// must be free'd by the caller.
int
tf_compile (const char *script, char **out_code);

void
tf_free (char *code);

// returns -1 on fail, size on success
int
tf_eval (ddb_tf_context_t *ctx, char *code, int codelen, char *out, int outlen);

#endif // __TF_H
