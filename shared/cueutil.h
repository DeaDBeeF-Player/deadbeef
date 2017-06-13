/*
    DeaDBeeF -- the music player
    Copyright (C) 2017 Alexey Yakovenko and other contributors

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

#ifndef cueutil_h
#define cueutil_h

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#include "../deadbeef.h"

#define SKIP_BLANK_CUE_TRACKS 0
#define MAX_CUE_TRACKS 99

const uint8_t *
pl_cue_skipspaces (const uint8_t *p);

void
pl_get_qvalue_from_cue (const uint8_t *p, int sz, char *out, const char *charset);

void
pl_get_value_from_cue (const char *p, int sz, char *out);

float
pl_cue_parse_time (const char *p);

#endif /* cueutil_h */
