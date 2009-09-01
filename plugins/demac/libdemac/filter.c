/*

libdemac - A Monkey's Audio decoder

$Id: filter.c 19556 2008-12-22 08:33:49Z amiconn $

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

#include <string.h>
#include <inttypes.h>

#include "demac.h"
#include "filter.h"
#include "demac_config.h"
     
#if FILTER_BITS == 32

#if defined(CPU_ARM) && (ARM_ARCH == 4)
#include "vector_math32_armv4.h"
#else
#include "vector_math_generic.h"
#endif

#else /* FILTER_BITS == 16 */

#ifdef CPU_COLDFIRE
#include "vector_math16_cf.h"
#elif defined(CPU_ARM) && (ARM_ARCH >= 6)
#include "vector_math16_armv6.h"
#elif defined(CPU_ARM) && (ARM_ARCH >= 5)
/* Assume all our ARMv5 targets are ARMv5te(j) */
#include "vector_math16_armv5te.h"
#else
#include "vector_math_generic.h"
#endif

#endif /* FILTER_BITS */

struct filter_t {
    filter_int* coeffs; /* ORDER entries */

    /* We store all the filter delays in a single buffer */
    filter_int* history_end;

    filter_int* delay;
    filter_int* adaptcoeffs;

    int avg;
};

/* We name the functions according to the ORDER and FRACBITS
   pre-processor symbols and build multiple .o files from this .c file
   - this increases code-size but gives the compiler more scope for
   optimising the individual functions, as well as replacing a lot of
   variables with constants.
*/

#if FRACBITS == 11
  #if ORDER == 16
     #define INIT_FILTER   init_filter_16_11
     #define APPLY_FILTER apply_filter_16_11
  #elif ORDER == 64
     #define INIT_FILTER  init_filter_64_11
     #define APPLY_FILTER apply_filter_64_11
  #endif
#elif FRACBITS == 13
  #define INIT_FILTER  init_filter_256_13
  #define APPLY_FILTER apply_filter_256_13
#elif FRACBITS == 10
  #define INIT_FILTER  init_filter_32_10
  #define APPLY_FILTER apply_filter_32_10
#elif FRACBITS == 15
  #define INIT_FILTER  init_filter_1280_15
  #define APPLY_FILTER apply_filter_1280_15
#endif

/* Some macros to handle the fixed-point stuff */

/* Convert from (32-FRACBITS).FRACBITS fixed-point format to an
   integer (rounding to nearest). */
#define FP_HALF  (1 << (FRACBITS - 1))   /* 0.5 in fixed-point format. */
#define FP_TO_INT(x) ((x + FP_HALF) >> FRACBITS)  /* round(x) */

#if defined(CPU_ARM) && (ARM_ARCH >= 6)
#define SATURATE(x) ({int __res; asm("ssat %0, #16, %1" : "=r"(__res) : "r"(x)); __res; })
#else
#define SATURATE(x) (LIKELY((x) == (int16_t)(x)) ? (x) : ((x) >> 31) ^ 0x7FFF)
#endif

/* Apply the filter with state f to count entries in data[] */

static void ICODE_ATTR_DEMAC do_apply_filter_3980(struct filter_t* f,
                                                  int32_t* data, int count)
{
    int res;
    int absres; 

#ifdef PREPARE_SCALARPRODUCT
    PREPARE_SCALARPRODUCT
#endif

    while(LIKELY(count--))
    {
        res = FP_TO_INT(scalarproduct(f->coeffs, f->delay - ORDER));

        if (LIKELY(*data != 0)) {
            if (*data < 0)
                vector_add(f->coeffs, f->adaptcoeffs - ORDER);
            else
                vector_sub(f->coeffs, f->adaptcoeffs - ORDER);
        }

        res += *data;

        *data++ = res;

        /* Update the output history */
        *f->delay++ = SATURATE(res);

        /* Version 3.98 and later files */

        /* Update the adaption coefficients */
        absres = (res < 0 ? -res : res);

        if (UNLIKELY(absres > 3 * f->avg))
            *f->adaptcoeffs = ((res >> 25) & 64) - 32;
        else if (3 * absres > 4 * f->avg)
            *f->adaptcoeffs = ((res >> 26) & 32) - 16;
        else if (LIKELY(absres > 0))
            *f->adaptcoeffs = ((res >> 27) & 16) - 8;
        else
            *f->adaptcoeffs = 0;

        f->avg += (absres - f->avg) / 16;

        f->adaptcoeffs[-1] >>= 1;
        f->adaptcoeffs[-2] >>= 1;
        f->adaptcoeffs[-8] >>= 1;

        f->adaptcoeffs++;

        /* Have we filled the history buffer? */
        if (UNLIKELY(f->delay == f->history_end)) {
            memmove(f->coeffs + ORDER, f->delay - (ORDER*2),
                    (ORDER*2) * sizeof(filter_int));
            f->adaptcoeffs = f->coeffs + ORDER*2;
            f->delay = f->coeffs + ORDER*3;
        }
    }
}

static void ICODE_ATTR_DEMAC do_apply_filter_3970(struct filter_t* f,
                                                  int32_t* data, int count)
{
    int res;
    
#ifdef PREPARE_SCALARPRODUCT
    PREPARE_SCALARPRODUCT
#endif

    while(LIKELY(count--))
    {
        res = FP_TO_INT(scalarproduct(f->coeffs, f->delay - ORDER));

        if (LIKELY(*data != 0)) {
            if (*data < 0)
                vector_add(f->coeffs, f->adaptcoeffs - ORDER);
            else
                vector_sub(f->coeffs, f->adaptcoeffs - ORDER);
        }

        /* Convert res from (32-FRACBITS).FRACBITS fixed-point format to an
           integer (rounding to nearest) and add the input value to
           it */
        res += *data;

        *data++ = res;

        /* Update the output history */
        *f->delay++ = SATURATE(res);

        /* Version ??? to < 3.98 files (untested) */
        f->adaptcoeffs[0] = (res == 0) ? 0 : ((res >> 28) & 8) - 4;
        f->adaptcoeffs[-4] >>= 1;
        f->adaptcoeffs[-8] >>= 1;

        f->adaptcoeffs++;

        /* Have we filled the history buffer? */
        if (UNLIKELY(f->delay == f->history_end)) {
            memmove(f->coeffs + ORDER, f->delay - (ORDER*2),
                    (ORDER*2) * sizeof(filter_int));
            f->adaptcoeffs = f->coeffs + ORDER*2;
            f->delay = f->coeffs + ORDER*3;
        }
    }
}

static struct filter_t filter0 IBSS_ATTR;
static struct filter_t filter1 IBSS_ATTR;

static void do_init_filter(struct filter_t* f, filter_int* buf)
{
    f->coeffs = buf;
    f->history_end = buf + ORDER*3 + FILTER_HISTORY_SIZE;

    /* Init pointers */
    f->adaptcoeffs = f->coeffs + ORDER*2;
    f->delay = f->coeffs + ORDER*3;

    /* Zero coefficients and history buffer */
    memset(f->coeffs, 0, ORDER*3 * sizeof(filter_int));

    /* Zero the running average */
    f->avg = 0;
}

void INIT_FILTER(filter_int* buf)
{
    do_init_filter(&filter0, buf);
    do_init_filter(&filter1, buf + ORDER*3 + FILTER_HISTORY_SIZE);
}

void ICODE_ATTR_DEMAC APPLY_FILTER(int fileversion, int32_t* data0,
                                   int32_t* data1, int count)
{
    if (fileversion >= 3980) {
        do_apply_filter_3980(&filter0, data0, count);
        if (data1 != NULL)
            do_apply_filter_3980(&filter1, data1, count);
    } else {
        do_apply_filter_3970(&filter0, data0, count);
        if (data1 != NULL)
            do_apply_filter_3970(&filter1, data1, count);
    }
}
