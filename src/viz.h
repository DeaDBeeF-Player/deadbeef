/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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
#ifndef viz_h
#define viz_h

#include <deadbeef/deadbeef.h>

void
viz_process (char * restrict bytes, int bytes_size, DB_output_t *output, int fft_size, int wave_size);

void
viz_init (void);

void
viz_free (void);

void
viz_reset (void);

void
viz_waveform_listen (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data));

void
viz_waveform_unlisten (void *ctx);

void
viz_spectrum_listen (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data));

void
viz_spectrum_unlisten (void *ctx);

#endif /* viz_h */
