/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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

#include "deadbeef.h"
#include "../fft.h"
#include <Accelerate/Accelerate.h>

static int _fft_size;
static float *fftDataReal;
static float *fftDataImaginary;
static float *hamming;
static float *sqMagnitudes;

static vDSP_DFT_Setup dftSetup;

static void
_init_buffers (int fft_size) {
    if (fft_size != _fft_size) {
        fft_free ();

        fftDataReal = calloc (fft_size * 2, sizeof (float));
        fftDataImaginary = calloc (fft_size * 2, sizeof (float));
        hamming = calloc (fft_size * 2, sizeof (float));
        sqMagnitudes = calloc (fft_size, sizeof (float));

        dftSetup = vDSP_DFT_zop_CreateSetup(NULL, fft_size * 2, FFT_FORWARD);
        vDSP_hamm_window(hamming, fft_size * 2, 0);

        _fft_size = fft_size;
    }
}

void
fft_calculate (const float *data, float *freq, int fft_size) {
    int dft_size = fft_size * 2;

    _init_buffers (fft_size);

    vDSP_vmul(data, 1, hamming, 1, fftDataReal, 1, dft_size);

    float outputR[dft_size];
    float outputI[dft_size];

    vDSP_DFT_Execute(dftSetup, fftDataReal, fftDataImaginary, outputR, outputI);

    DSPSplitComplex splitComplex = {
        .realp = outputR,
        .imagp = outputI
    };
    vDSP_zvmags(&splitComplex, 1, sqMagnitudes, 1, fft_size);

    for (int i = 0; i < fft_size; i++) {
        freq[i] = (float)(2 * sqrt(sqMagnitudes[i]) / fft_size);
    }
}

void
fft_free (void) {
    free (fftDataReal);
    free (fftDataImaginary);
    free (hamming);
    free (sqMagnitudes);
    if (dftSetup != NULL) {
        vDSP_DFT_DestroySetup (dftSetup);
    }
    fftDataReal = NULL;
    fftDataImaginary = NULL;
    hamming = NULL;
    sqMagnitudes = NULL;
    dftSetup = NULL;
}

