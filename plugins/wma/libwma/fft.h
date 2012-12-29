/*
 * WMA compatible decoder
 * Copyright (c) 2002 The FFmpeg Project.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef CODECLIB_FFT_H_INCLUDED
#define CODECLIB_FFT_H_INCLUDED
 
#include <inttypes.h>
typedef int32_t fixed32; 
typedef int64_t fixed64;

#define FFT_FIXED

#ifdef FFT_FIXED
typedef fixed32 FFTSample;
#else /* FFT_FIXED */
typedef float   FFTSample;
#endif /* FFT_FIXED */

typedef struct FFTComplex {
    FFTSample re, im;
} FFTComplex;

typedef struct FFTContext {
    int nbits;
    int inverse;
    uint16_t *revtab;
    int mdct_size; /* size of MDCT (i.e. number of input data * 2) */
    int mdct_bits; /* n = 2^nbits */
    /* pre/post rotation tables */
    FFTSample *tcos;
    FFTSample *tsin;
    void (*fft_permute)(struct FFTContext *s, FFTComplex *z);
    void (*fft_calc)(struct FFTContext *s, FFTComplex *z);
    void (*imdct_calc)(struct FFTContext *s, FFTSample *output, const FFTSample *input);
    void (*imdct_half)(struct FFTContext *s, FFTSample *output, const FFTSample *input);
    void (*mdct_calc)(struct FFTContext *s, FFTSample *output, const FFTSample *input);
    int split_radix;
    int permutation;
#define FF_MDCT_PERM_NONE       0
#define FF_MDCT_PERM_INTERLEAVE 1
} FFTContext;

// internal api  (fft<->mdct)
//int fft_calc_unscaled(FFTContext *s, FFTComplex *z);
//void ff_fft_permute_c(FFTContext *s, FFTComplex *z); // internal only?
void ff_fft_calc_c(int nbits, FFTComplex *z);

#endif // CODECLIB_FFT_H_INCLUDED

