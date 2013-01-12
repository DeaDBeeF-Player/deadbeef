/****************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 Michael Giacomelli
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*  fixed precision code.  We use a combination of Sign 15.16 and Sign.31
     precision here.

    The WMA decoder does not always follow this convention, and occasionally
    renormalizes values to other formats in order to maximize precision.
    However, only the two precisions above are provided in this file.

*/

#include "types.h"
#include "mdct.h"

#define PRECISION       16
#define PRECISION64     16


#define fixtof64(x)       (float)((float)(x) / (float)(1 << PRECISION64))        //does not work on int64_t!
#define ftofix32(x)       ((fixed32)((x) * (float)(1 << PRECISION) + ((x) < 0 ? -0.5 : 0.5)))
#define itofix64(x)       (IntTo64(x))
#define itofix32(x)       ((x) << PRECISION)
#define fixtoi32(x)       ((x) >> PRECISION)
#define fixtoi64(x)       (IntFrom64(x))


/*fixed functions*/

fixed64 IntTo64(int x);
int IntFrom64(fixed64 x);
fixed32 Fixed32From64(fixed64 x);
fixed64 Fixed32To64(fixed32 x);
fixed32 fixdiv32(fixed32 x, fixed32 y);
fixed64 fixdiv64(fixed64 x, fixed64 y);
fixed32 fixsqrt32(fixed32 x);
/* Inverse gain of circular cordic rotation in s0.31 format. */
long fsincos(unsigned long phase, fixed32 *cos);


#ifdef CPU_ARM

/*Sign-15.16 format */
#define fixmul32(x, y)  \
    ({ int32_t __hi;  \
       uint32_t __lo;  \
       int32_t __result;  \
       asm ("smull   %0, %1, %3, %4\n\t"  \
            "movs    %0, %0, lsr %5\n\t"  \
            "adc    %2, %0, %1, lsl %6"  \
            : "=&r" (__lo), "=&r" (__hi), "=r" (__result)  \
            : "%r" (x), "r" (y),  \
              "M" (PRECISION), "M" (32 - PRECISION)  \
            : "cc");  \
       __result;  \
    })

#elif defined(CPU_COLDFIRE)

static inline int32_t fixmul32(int32_t x, int32_t y)
{
#if PRECISION != 16
#warning Coldfire fixmul32() only works for PRECISION == 16
#endif
    int32_t t1;
    asm (
        "mac.l   %[x], %[y], %%acc0  \n" // multiply
        "mulu.l  %[y], %[x]      \n"     // get lower half, avoid emac stall
        "movclr.l %%acc0, %[t1]  \n"     // get higher half
        "lsr.l   #1, %[t1]       \n"
        "move.w  %[t1], %[x]     \n"
        "swap    %[x]            \n"
        : [t1] "=&d" (t1), [x] "+d" (x)
        : [y] "d"  (y)
    );
    return x;
}

#else

static inline fixed32 fixmul32(fixed32 x, fixed32 y)
{
    fixed64 temp;
    temp = x;
    temp *= y;

    temp >>= PRECISION;

    return (fixed32)temp;
}

#endif


/*
 * Helper functions for wma_window.
 *
 *
 */

#ifdef CPU_ARM
static inline void vector_fmul_add_add(fixed32 *dst, const fixed32 *data,
                         const fixed32 *window, int n)
{
    /* Block sizes are always power of two */
    asm volatile (
        "0:"
        "ldmia %[d]!, {r0, r1};"
        "ldmia %[w]!, {r4, r5};"
        /* consume the first data and window value so we can use those
         * registers again */
        "smull r8, r9, r0, r4;"
        "ldmia %[dst], {r0, r4};"
        "add   r0, r0, r9, lsl #1;"  /* *dst=*dst+(r9<<1)*/
        "smull r8, r9, r1, r5;"
        "add   r1, r4, r9, lsl #1;"
        "stmia %[dst]!, {r0, r1};"
        "subs  %[n], %[n], #2;"
        "bne   0b;"
        : [d] "+r" (data), [w] "+r" (window), [dst] "+r" (dst), [n] "+r" (n)
        : : "r0", "r1", "r4", "r5", "r8", "r9", "memory", "cc");
}

static inline void vector_fmul_reverse(fixed32 *dst, const fixed32 *src0, const fixed32 *src1,
                         int len)
{
    /* Block sizes are always power of two */
    asm volatile (
        "add   %[s1], %[s1], %[n], lsl #2;"
        "0:"
        "ldmia %[s0]!, {r0, r1};"
        "ldmdb %[s1]!, {r4, r5};"
        "smull r8, r9, r0, r5;"
        "mov   r0, r9, lsl #1;"
        "smull r8, r9, r1, r4;"
        "mov   r1, r9, lsl #1;"
        "stmia %[dst]!, {r0, r1};"
        "subs  %[n], %[n], #2;"
        "bne   0b;"
        : [s0] "+r" (src0), [s1] "+r" (src1), [dst] "+r" (dst), [n] "+r" (len)
        : : "r0", "r1", "r4", "r5", "r8", "r9", "memory", "cc");
}

#elif defined(CPU_COLDFIRE)

static inline void vector_fmul_add_add(fixed32 *dst, const fixed32 *data,
                         const fixed32 *window, int n)
{
    /* Block sizes are always power of two. Smallest block is always way bigger
     * than four too.*/
    asm volatile (
        "0:"
        "movem.l (%[d]), %%d0-%%d3;"
        "movem.l (%[w]), %%d4-%%d5/%%a0-%%a1;"
        "mac.l %%d0, %%d4, %%acc0;"
        "mac.l %%d1, %%d5, %%acc1;"
        "mac.l %%d2, %%a0, %%acc2;"
        "mac.l %%d3, %%a1, %%acc3;"
        "lea.l (16, %[d]), %[d];"
        "lea.l (16, %[w]), %[w];"
        "movclr.l %%acc0, %%d0;"
        "movclr.l %%acc1, %%d1;"
        "movclr.l %%acc2, %%d2;"
        "movclr.l %%acc3, %%d3;"
        "movem.l (%[dst]), %%d4-%%d5/%%a0-%%a1;"
        "add.l %%d4, %%d0;"
        "add.l %%d5, %%d1;"
        "add.l %%a0, %%d2;"
        "add.l %%a1, %%d3;"
        "movem.l %%d0-%%d3, (%[dst]);"
        "lea.l (16, %[dst]), %[dst];"
        "subq.l #4, %[n];"
        "jne 0b;"
        : [d] "+a" (data), [w] "+a" (window), [dst] "+a" (dst), [n] "+d" (n)
        : : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "memory", "cc");
}

static inline void vector_fmul_reverse(fixed32 *dst, const fixed32 *src0, const fixed32 *src1,
                         int len)
{
    /* Block sizes are always power of two. Smallest block is always way bigger
     * than four too.*/
    asm volatile (
        "lea.l (-16, %[s1], %[n]*4), %[s1];"
        "0:"
        "movem.l (%[s0]), %%d0-%%d3;"
        "movem.l (%[s1]), %%d4-%%d5/%%a0-%%a1;"
        "mac.l %%d0, %%a1, %%acc0;"
        "mac.l %%d1, %%a0, %%acc1;"
        "mac.l %%d2, %%d5, %%acc2;"
        "mac.l %%d3, %%d4, %%acc3;"
        "lea.l (16, %[s0]), %[s0];"
        "lea.l (-16, %[s1]), %[s1];"
        "movclr.l %%acc0, %%d0;"
        "movclr.l %%acc1, %%d1;"
        "movclr.l %%acc2, %%d2;"
        "movclr.l %%acc3, %%d3;"
        "movem.l %%d0-%%d3, (%[dst]);"
        "lea.l (16, %[dst]), %[dst];"
        "subq.l #4, %[n];"
        "jne 0b;"
        : [s0] "+a" (src0), [s1] "+a" (src1), [dst] "+a" (dst), [n] "+d" (len)
        : : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "memory", "cc");
}

#else

static inline void vector_fmul_add_add(fixed32 *dst, const fixed32 *src0, const fixed32 *src1, int len){
    int i;
    for(i=0; i<len; i++)
        dst[i] = fixmul32b(src0[i], src1[i]) + dst[i];
}

static inline void vector_fmul_reverse(fixed32 *dst, const fixed32 *src0, const fixed32 *src1, int len){
    int i;
    src1 += len-1;
    for(i=0; i<len; i++)
        dst[i] = fixmul32b(src0[i], src1[-i]);
}

#endif
