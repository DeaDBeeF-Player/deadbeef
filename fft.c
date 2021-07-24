/*
 * fft.c
 * Copyright 2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

// this version has a few changes compared to the original audacious fft.c
// please find the original file in audacious

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "deadbeef.h"
#include <math.h>
#include <complex.h>

#define N (DDB_FREQ_BANDS * 2)

static float hamming[N];              /* hamming window, scaled to sum to 1 */
static int reversed[N];               /* bit-reversal table */
static float complex roots[N / 2];    /* N-th roots of unity */
static int generated = 0;
static int LOGN; /* log N (base 2) */

/* Reverse the order of the lowest LOGN bits in an integer. */

static int bit_reverse (int x)
{
    int y = 0;

    for (int n = LOGN; n --; )
    {
        y = (y << 1) | (x & 1);
        x >>= 1;
    }

    return y;
}

#ifndef HAVE_LOG2
static inline float log2(float x) {return (float)log(x)/M_LN2;}
#endif

/* Generate lookup tables. */

static void generate_tables (void)
{
    if (generated)
        return;

    LOGN = (int)log2(N);
    for (int n = 0; n < N; n ++)
        hamming[n] = 1 - 0.85f * cosf (2 * (float)M_PI * n / N);
    for (int n = 0; n < N; n ++)
        reversed[n] = bit_reverse (n);
    for (int n = 0; n < N / 2; n ++)
        roots[n] = cexpf (2 * (float)M_PI * I * n / N);

    generated = 1;
}

static void do_fft (float complex a[N])
{
    int half = 1;       /* (2^s)/2 */
    int inv = N / 2;    /* N/(2^s) */

    /* loop through steps */
    while (inv)
    {
        /* loop through groups */
        for (int g = 0; g < N; g += half << 1)
        {
            /* loop through butterflies */
            for (int b = 0, r = 0; b < half; b ++, r += inv)
            {
                float complex even = a[g + b];
                float complex odd = roots[r] * a[g + half + b];
                a[g + b] = even + odd;
                a[g + half + b] = even - odd;
            }
        }

        half <<= 1;
        inv >>= 1;
    }
}

void
fft_calculate (float *data, float *freq) {
    generate_tables ();

    // fft code shamelessly stolen from audacious
    // thanks, John
    float complex a[N];
    for (int n = 0; n < N; n ++) {
        a[reversed[n]] = data[n] * hamming[n];
    }
    do_fft(a);

    for (int n = 0; n < N / 2 - 1; n ++)
        freq[n] = 2 * cabsf (a[1 + n]) / N;
    freq[N / 2 - 1] = cabsf(a[N / 2]) / N;
}

void
fft_free (void) {
}
