/*

libdemac - A Monkey's Audio decoder

$Id: predictor.c 19375 2008-12-09 23:20:59Z amiconn $

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#include <inttypes.h>
#include <string.h>

#include "parser.h"
#include "predictor.h"
#include "demac_config.h"

/* Return 0 if x is zero, -1 if x is positive, 1 if x is negative */
#define SIGN(x) (x) ? (((x) > 0) ? -1 : 1) : 0

static const int32_t initial_coeffs[4] = {
  360, 317, -109, 98
};

#define YDELAYA (18 + PREDICTOR_ORDER*4)
#define YDELAYB (18 + PREDICTOR_ORDER*3)
#define XDELAYA (18 + PREDICTOR_ORDER*2)
#define XDELAYB (18 + PREDICTOR_ORDER)

#define YADAPTCOEFFSA (18)
#define XADAPTCOEFFSA (14)
#define YADAPTCOEFFSB (10)
#define XADAPTCOEFFSB (5)

void init_predictor_decoder(struct predictor_t* p)
{
    /* Zero the history buffers */
    memset(p->historybuffer, 0, PREDICTOR_SIZE * sizeof(int32_t));
    p->buf = p->historybuffer;

    /* Initialise and zero the co-efficients */
    memcpy(p->YcoeffsA, initial_coeffs, sizeof(initial_coeffs));
    memcpy(p->XcoeffsA, initial_coeffs, sizeof(initial_coeffs));
    memset(p->YcoeffsB, 0, sizeof(p->YcoeffsB));
    memset(p->XcoeffsB, 0, sizeof(p->XcoeffsB));

    p->YfilterA = 0;
    p->YfilterB = 0;
    p->YlastA = 0;

    p->XfilterA = 0;
    p->XfilterB = 0;
    p->XlastA = 0;
}

#if !defined(CPU_ARM) && !defined(CPU_COLDFIRE)
void ICODE_ATTR_DEMAC predictor_decode_stereo(struct predictor_t* p,
                                              int32_t* decoded0,
                                              int32_t* decoded1,
                                              int count)
{
    int32_t predictionA, predictionB; 

    while (LIKELY(count--))
    {
        /* Predictor Y */
        p->buf[YDELAYA] = p->YlastA;
        p->buf[YADAPTCOEFFSA] = SIGN(p->buf[YDELAYA]);

        p->buf[YDELAYA-1] = p->buf[YDELAYA] - p->buf[YDELAYA-1];
        p->buf[YADAPTCOEFFSA-1] = SIGN(p->buf[YDELAYA-1]);

        predictionA = (p->buf[YDELAYA] * p->YcoeffsA[0]) + 
                      (p->buf[YDELAYA-1] * p->YcoeffsA[1]) + 
                      (p->buf[YDELAYA-2] * p->YcoeffsA[2]) + 
                      (p->buf[YDELAYA-3] * p->YcoeffsA[3]);

        /*  Apply a scaled first-order filter compression */
        p->buf[YDELAYB] = p->XfilterA - ((p->YfilterB * 31) >> 5);
        p->buf[YADAPTCOEFFSB] = SIGN(p->buf[YDELAYB]);
        p->YfilterB = p->XfilterA;

        p->buf[YDELAYB-1] = p->buf[YDELAYB] - p->buf[YDELAYB-1];
        p->buf[YADAPTCOEFFSB-1] = SIGN(p->buf[YDELAYB-1]);

        predictionB = (p->buf[YDELAYB] * p->YcoeffsB[0]) + 
                      (p->buf[YDELAYB-1] * p->YcoeffsB[1]) + 
                      (p->buf[YDELAYB-2] * p->YcoeffsB[2]) + 
                      (p->buf[YDELAYB-3] * p->YcoeffsB[3]) + 
                      (p->buf[YDELAYB-4] * p->YcoeffsB[4]);

        p->YlastA = *decoded0 + ((predictionA + (predictionB >> 1)) >> 10);
        p->YfilterA =  p->YlastA + ((p->YfilterA * 31) >> 5);

        /* Predictor X */

        p->buf[XDELAYA] = p->XlastA;
        p->buf[XADAPTCOEFFSA] = SIGN(p->buf[XDELAYA]);
        p->buf[XDELAYA-1] = p->buf[XDELAYA] - p->buf[XDELAYA-1];
        p->buf[XADAPTCOEFFSA-1] = SIGN(p->buf[XDELAYA-1]);

        predictionA = (p->buf[XDELAYA] * p->XcoeffsA[0]) + 
                      (p->buf[XDELAYA-1] * p->XcoeffsA[1]) + 
                      (p->buf[XDELAYA-2] * p->XcoeffsA[2]) + 
                      (p->buf[XDELAYA-3] * p->XcoeffsA[3]);

        /*  Apply a scaled first-order filter compression */
        p->buf[XDELAYB] = p->YfilterA - ((p->XfilterB * 31) >> 5);
        p->buf[XADAPTCOEFFSB] = SIGN(p->buf[XDELAYB]);
        p->XfilterB = p->YfilterA;
        p->buf[XDELAYB-1] = p->buf[XDELAYB] - p->buf[XDELAYB-1];
        p->buf[XADAPTCOEFFSB-1] = SIGN(p->buf[XDELAYB-1]);

        predictionB = (p->buf[XDELAYB] * p->XcoeffsB[0]) + 
                      (p->buf[XDELAYB-1] * p->XcoeffsB[1]) + 
                      (p->buf[XDELAYB-2] * p->XcoeffsB[2]) + 
                      (p->buf[XDELAYB-3] * p->XcoeffsB[3]) + 
                      (p->buf[XDELAYB-4] * p->XcoeffsB[4]);

        p->XlastA = *decoded1 + ((predictionA + (predictionB >> 1)) >> 10); 
        p->XfilterA =  p->XlastA + ((p->XfilterA * 31) >> 5);

        if (LIKELY(*decoded0 != 0))
        {
            if (*decoded0 > 0)
            {
                p->YcoeffsA[0] -= p->buf[YADAPTCOEFFSA];
                p->YcoeffsA[1] -= p->buf[YADAPTCOEFFSA-1];
                p->YcoeffsA[2] -= p->buf[YADAPTCOEFFSA-2];
                p->YcoeffsA[3] -= p->buf[YADAPTCOEFFSA-3];

                p->YcoeffsB[0] -= p->buf[YADAPTCOEFFSB];
                p->YcoeffsB[1] -= p->buf[YADAPTCOEFFSB-1];
                p->YcoeffsB[2] -= p->buf[YADAPTCOEFFSB-2];
                p->YcoeffsB[3] -= p->buf[YADAPTCOEFFSB-3];
                p->YcoeffsB[4] -= p->buf[YADAPTCOEFFSB-4];
            }
            else
            {
                p->YcoeffsA[0] += p->buf[YADAPTCOEFFSA];
                p->YcoeffsA[1] += p->buf[YADAPTCOEFFSA-1];
                p->YcoeffsA[2] += p->buf[YADAPTCOEFFSA-2];
                p->YcoeffsA[3] += p->buf[YADAPTCOEFFSA-3];

                p->YcoeffsB[0] += p->buf[YADAPTCOEFFSB];
                p->YcoeffsB[1] += p->buf[YADAPTCOEFFSB-1];
                p->YcoeffsB[2] += p->buf[YADAPTCOEFFSB-2];
                p->YcoeffsB[3] += p->buf[YADAPTCOEFFSB-3];
                p->YcoeffsB[4] += p->buf[YADAPTCOEFFSB-4];
            }
        }

        *(decoded0++) = p->YfilterA;

        if (LIKELY(*decoded1 != 0))
        {
            if (*decoded1 > 0)
            {
                p->XcoeffsA[0] -= p->buf[XADAPTCOEFFSA];
                p->XcoeffsA[1] -= p->buf[XADAPTCOEFFSA-1];
                p->XcoeffsA[2] -= p->buf[XADAPTCOEFFSA-2];
                p->XcoeffsA[3] -= p->buf[XADAPTCOEFFSA-3];

                p->XcoeffsB[0] -= p->buf[XADAPTCOEFFSB];
                p->XcoeffsB[1] -= p->buf[XADAPTCOEFFSB-1];
                p->XcoeffsB[2] -= p->buf[XADAPTCOEFFSB-2];
                p->XcoeffsB[3] -= p->buf[XADAPTCOEFFSB-3];
                p->XcoeffsB[4] -= p->buf[XADAPTCOEFFSB-4];
            }
            else
            {
                p->XcoeffsA[0] += p->buf[XADAPTCOEFFSA];
                p->XcoeffsA[1] += p->buf[XADAPTCOEFFSA-1];
                p->XcoeffsA[2] += p->buf[XADAPTCOEFFSA-2];
                p->XcoeffsA[3] += p->buf[XADAPTCOEFFSA-3];

                p->XcoeffsB[0] += p->buf[XADAPTCOEFFSB];
                p->XcoeffsB[1] += p->buf[XADAPTCOEFFSB-1];
                p->XcoeffsB[2] += p->buf[XADAPTCOEFFSB-2];
                p->XcoeffsB[3] += p->buf[XADAPTCOEFFSB-3];
                p->XcoeffsB[4] += p->buf[XADAPTCOEFFSB-4];
            }
        }

        *(decoded1++) = p->XfilterA;

        /* Combined */
        p->buf++;

        /* Have we filled the history buffer? */
        if (UNLIKELY(p->buf == p->historybuffer + PREDICTOR_HISTORY_SIZE)) {
            memmove(p->historybuffer, p->buf, 
                    PREDICTOR_SIZE * sizeof(int32_t));
            p->buf = p->historybuffer;
        }
    }
}

void ICODE_ATTR_DEMAC predictor_decode_mono(struct predictor_t* p,
                                            int32_t* decoded0,
                                            int count)
{
    int32_t predictionA, currentA, A;

    currentA = p->YlastA;

    while (LIKELY(count--))
    {
        A = *decoded0;

        p->buf[YDELAYA] = currentA;
        p->buf[YDELAYA-1] = p->buf[YDELAYA] - p->buf[YDELAYA-1];

        predictionA = (p->buf[YDELAYA] * p->YcoeffsA[0]) + 
                      (p->buf[YDELAYA-1] * p->YcoeffsA[1]) + 
                      (p->buf[YDELAYA-2] * p->YcoeffsA[2]) + 
                      (p->buf[YDELAYA-3] * p->YcoeffsA[3]);

        currentA = A + (predictionA >> 10);

        p->buf[YADAPTCOEFFSA] = SIGN(p->buf[YDELAYA]);
        p->buf[YADAPTCOEFFSA-1] = SIGN(p->buf[YDELAYA-1]);
        
        if (LIKELY(A != 0))
        {
            if (A > 0)
            {
                p->YcoeffsA[0] -= p->buf[YADAPTCOEFFSA];
                p->YcoeffsA[1] -= p->buf[YADAPTCOEFFSA-1];
                p->YcoeffsA[2] -= p->buf[YADAPTCOEFFSA-2];
                p->YcoeffsA[3] -= p->buf[YADAPTCOEFFSA-3];
            }
            else
            {
                p->YcoeffsA[0] += p->buf[YADAPTCOEFFSA];
                p->YcoeffsA[1] += p->buf[YADAPTCOEFFSA-1];
                p->YcoeffsA[2] += p->buf[YADAPTCOEFFSA-2];
                p->YcoeffsA[3] += p->buf[YADAPTCOEFFSA-3];
            }
        }

        p->buf++;

        /* Have we filled the history buffer? */
        if (UNLIKELY(p->buf == p->historybuffer + PREDICTOR_HISTORY_SIZE)) {
            memmove(p->historybuffer, p->buf, 
                    PREDICTOR_SIZE * sizeof(int32_t));
            p->buf = p->historybuffer;
        }

        p->YfilterA =  currentA + ((p->YfilterA * 31) >> 5);
        *(decoded0++) = p->YfilterA;
    }

    p->YlastA = currentA;
}
#endif
