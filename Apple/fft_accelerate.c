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

static float fftDataImaginary[DDB_FREQ_BANDS*2];

static vDSP_DFT_Setup dftSetup;

void
fft_calculate (const float *data, float *freq) {
    int fftSize = DDB_FREQ_BANDS*2;

    if (dftSetup == NULL) {
        dftSetup = vDSP_DFT_zop_CreateSetup(NULL, fftSize, FFT_FORWARD);
    }

    memset (fftDataImaginary, 0, sizeof((fftDataImaginary)));

    float outputR[fftSize];
    float outputI[fftSize];

    // TODO: apply hamming window
    vDSP_DFT_Execute(dftSetup, data, fftDataImaginary, outputR, outputI);

    DSPSplitComplex splitComplex = {
        .realp = outputR,
        .imagp = outputI
    };
    float sqMagnitudes[fftSize];
    memset (sqMagnitudes, 0, sizeof (sqMagnitudes));
    vDSP_zvmags(&splitComplex, 1, sqMagnitudes, 1, fftSize);

    for (int i = 0; i < fftSize/2; i++) {
        freq[i] = (float)(2 * sqrt(sqMagnitudes[i]) / (fftSize / 2));
    }
}

void
fft_free (void) {
    if (dftSetup != NULL) {
        vDSP_DFT_DestroySetup (dftSetup);
        dftSetup = NULL;
    }
}

