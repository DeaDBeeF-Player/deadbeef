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

typedef struct {
    int _size; // must be set to sizeof(tf_context_t)
    playItem_t *it; // track to get information from, or NULL
    playlist_t *plt; // playlist in which the track resides, or NULL
    int idx; // index of the track in playlist the track belongs to, or -1
} tf_context_t;

// returns the output bytecode size.
// the resulting bytecode is stored in the *out.
// must be free'd by the caller.
int
tf_compile (const char *script, char **out_code);

void
tf_free (char *code);

// returns -1 on fail, 0 on success
int
tf_eval (tf_context_t *ctx, char *code, int codelen, char *out, int outlen);

#endif // __TF_H
