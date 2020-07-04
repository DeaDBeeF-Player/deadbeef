/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

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

#ifndef __AAC_PARSER_H
#define __AAC_PARSER_H

#include <stdint.h>

#define ADTS_HEADER_SIZE 7

// buf size must be at least ADTS_HEADER_SIZE*8
// returns frame size
int
aac_sync(const uint8_t *buf, int *channels, int *sample_rate, int *bit_rate, int *samples);

#endif
