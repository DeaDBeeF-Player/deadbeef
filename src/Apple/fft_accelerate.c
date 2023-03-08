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

#include <Accelerate/Accelerate.h>
#include <deadbeef/deadbeef.h>
#include "../fft.h"

static int _fft_size;
static float *_input_real;
static float *_input_imaginary;
static float *_output_real;
static float *_output_imaginary;
static float *_hamming;
static float *_sq_mags;

static vDSP_DFT_Setup _dft_setup;

static void
_init_buffers (int fft_size) {
    if (fft_size != _fft_size) {
        fft_free ();

        _input_real = calloc (fft_size * 2, sizeof (float));
        _input_imaginary = calloc (fft_size * 2, sizeof (float));
        _hamming = calloc (fft_size * 2, sizeof (float));
        _sq_mags = calloc (fft_size, sizeof (float));
        _output_real = calloc (fft_size * 2, sizeof (float));
        _output_imaginary = calloc (fft_size * 2, sizeof (float));

        _dft_setup = vDSP_DFT_zop_CreateSetup(NULL, fft_size * 2, FFT_FORWARD);
        vDSP_hamm_window(_hamming, fft_size * 2, 0);

        _fft_size = fft_size;
    }
}

void
fft_calculate (const float *data, float *freq, int fft_size) {
    int dft_size = fft_size * 2;

    _init_buffers (fft_size);

    vDSP_vmul(data, 1, _hamming, 1, _input_real, 1, dft_size);

    vDSP_DFT_Execute(_dft_setup, _input_real, _input_imaginary, _output_real, _output_imaginary);

    DSPSplitComplex split_complex = {
        .realp = _output_real,
        .imagp = _output_imaginary
    };
    vDSP_zvmags(&split_complex, 1, _sq_mags, 1, fft_size);

    int sq_count = fft_size;
    vvsqrtf(_sq_mags, _sq_mags, &sq_count);

    float mult = 2.f / fft_size;
    vDSP_vsmul(_sq_mags, 1, &mult, freq, 1, fft_size);
}

void
fft_free (void) {
    free (_input_real);
    free (_input_imaginary);
    free (_hamming);
    free (_sq_mags);
    free (_output_real);
    free (_output_imaginary);
    if (_dft_setup != NULL) {
        vDSP_DFT_DestroySetup (_dft_setup);
    }
    _input_real = NULL;
    _input_imaginary = NULL;
    _hamming = NULL;
    _sq_mags = NULL;
    _dft_setup = NULL;
    _output_real = NULL;
    _output_imaginary = NULL;
}

