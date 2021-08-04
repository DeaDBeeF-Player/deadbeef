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
#include "fft.h"
#include <Accelerate/Accelerate.h>

static float fftDataReal[DDB_FREQ_BANDS*2];
static float fftDataImaginary[DDB_FREQ_BANDS*2];
static float hamming[DDB_FREQ_BANDS*2];

static vDSP_DFT_Setup dftSetup;
static float sqMagnitudes[DDB_FREQ_BANDS];

void
fft_calculate (const float *data, float *freq) {
    int fftSize = DDB_FREQ_BANDS*2;

    if (dftSetup == NULL) {
        dftSetup = vDSP_DFT_zop_CreateSetup(NULL, fftSize, FFT_FORWARD);
        vDSP_hamm_window(hamming, fftSize, 0);
        memset (fftDataImaginary, 0, sizeof (fftDataImaginary));
    }

    vDSP_vmul(data, 1, hamming, 1, fftDataReal, 1, fftSize);

    float outputR[fftSize];
    float outputI[fftSize];

    vDSP_DFT_Execute(dftSetup, fftDataReal, fftDataImaginary, outputR, outputI);

    DSPSplitComplex splitComplex = {
        .realp = outputR,
        .imagp = outputI
    };
    vDSP_zvmags(&splitComplex, 1, sqMagnitudes, 1, DDB_FREQ_BANDS);

    for (int i = 0; i < DDB_FREQ_BANDS; i++) {
        freq[i] = (float)(2 * sqrt(sqMagnitudes[i]) / DDB_FREQ_BANDS);
    }
}

void
fft_free (void) {
    if (dftSetup != NULL) {
        vDSP_DFT_DestroySetup (dftSetup);
        dftSetup = NULL;
    }
}

