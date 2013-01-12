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

#ifndef CODECLIB_MDCT_H_INCLUDED
#define CODECLIB_MDCT_H_INCLUDED

//#include "types.h"
#include "fft.h"

void ff_imdct_calc(unsigned int nbits, fixed32 *output, const fixed32 *input);
void ff_imdct_half(unsigned int nbits, fixed32 *output, const fixed32 *input);

#ifdef CPU_ARM

/*Sign-15.16 format */
#define fixmul32b(x, y)  \
    ({ int32_t __hi;  \
       uint32_t __lo;  \
       int32_t __result;  \
       asm ("smull   %0, %1, %3, %4\n\t"  \
            "mov     %2, %1, lsl #1"  \
            : "=&r" (__lo), "=&r" (__hi), "=r" (__result)  \
            : "%r" (x), "r" (y)  \
            : "cc" );  \
       __result;  \
    })

#elif defined(CPU_COLDFIRE)

static inline int32_t fixmul32b(int32_t x, int32_t y)
{
    asm (
        "mac.l   %[x], %[y], %%acc0  \n" /* multiply */
        "movclr.l %%acc0, %[x]  \n"     /* get higher half */
        : [x] "+d" (x)
        : [y] "d"  (y)
    );
    return x;
}

#else

static inline fixed32 fixmul32b(fixed32 x, fixed32 y)
{
    fixed64 temp;

    temp = x;
    temp *= y;

    temp >>= 31;        //16+31-16 = 31 bits

    return (fixed32)temp;
}
#endif


#ifdef CPU_ARM
static inline
void CMUL(fixed32 *x, fixed32 *y,
          fixed32  a, fixed32  b,
          fixed32  t, fixed32  v)
{
    /* This version loses one bit of precision. Could be solved at the cost
     * of 2 extra cycles if it becomes an issue. */
    int x1, y1, l;
    asm(
        "smull    %[l], %[y1], %[b], %[t] \n"
        "smlal    %[l], %[y1], %[a], %[v] \n"
        "rsb      %[b], %[b], #0          \n"
        "smull    %[l], %[x1], %[a], %[t] \n"
        "smlal    %[l], %[x1], %[b], %[v] \n"
        : [l] "=&r" (l), [x1]"=&r" (x1), [y1]"=&r" (y1), [b] "+r" (b)
        : [a] "r" (a),   [t] "r" (t),    [v] "r" (v)
        : "cc"
    );
    *x = x1 << 1;
    *y = y1 << 1;
}
#elif defined CPU_COLDFIRE
static inline
void CMUL(fixed32 *x, fixed32 *y,
          fixed32  a, fixed32  b,
          fixed32  t, fixed32  v)
{
  asm volatile ("mac.l %[a], %[t], %%acc0;"
                "msac.l %[b], %[v], %%acc0;"
                "mac.l %[b], %[t], %%acc1;"
                "mac.l %[a], %[v], %%acc1;"
                "movclr.l %%acc0, %[a];"
                "move.l %[a], (%[x]);"
                "movclr.l %%acc1, %[a];"
                "move.l %[a], (%[y]);"
                : [a] "+&r" (a)
                : [x] "a" (x), [y] "a" (y),
                  [b] "r" (b), [t] "r" (t), [v] "r" (v)
                : "cc", "memory");
}
#else
static inline
void CMUL(fixed32 *pre,
          fixed32 *pim,
          fixed32 are,
          fixed32 aim,
          fixed32 bre,
          fixed32 bim)
{
    //int64_t x,y;
    fixed32 _aref = are;
    fixed32 _aimf = aim;
    fixed32 _bref = bre;
    fixed32 _bimf = bim;
    fixed32 _r1 = fixmul32b(_bref, _aref);
    fixed32 _r2 = fixmul32b(_bimf, _aimf);
    fixed32 _r3 = fixmul32b(_bref, _aimf);
    fixed32 _r4 = fixmul32b(_bimf, _aref);
    *pre = _r1 - _r2;
    *pim = _r3 + _r4;

}
#endif


#endif // CODECLIB_MDCT_H_INCLUDED
