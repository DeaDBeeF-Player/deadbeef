/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001-2009  Josh Coalson
 * Copyright (C) 2011-2013  Xiph.Org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLAC__PRIVATE__BITMATH_H
#define FLAC__PRIVATE__BITMATH_H

#include "FLAC/ordinals.h"

/* for CHAR_BIT */
#include <limits.h>
#include "share/compat.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#include <intrin.h> /* for _BitScanReverse* */
#endif

/* Will never be emitted for MSVC, GCC, Intel compilers */
static inline unsigned int FLAC__clz_soft_uint32(unsigned int word)
{
    static const unsigned char byte_to_unary_table[] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    return (word) > 0xffffff ? byte_to_unary_table[(word) >> 24] :
    (word) > 0xffff ? byte_to_unary_table[(word) >> 16] + 8 :
    (word) > 0xff ? byte_to_unary_table[(word) >> 8] + 16 :
    byte_to_unary_table[(word)] + 24;
}

static inline unsigned int FLAC__clz_uint32(FLAC__uint32 v)
{
/* Never used with input 0 */
#if defined(__INTEL_COMPILER)
    return _bit_scan_reverse(v) ^ 31U;
#elif defined(__GNUC__) && (__GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
/* This will translate either to (bsr ^ 31U), clz , ctlz, cntlz, lzcnt depending on
 * -march= setting or to a software rutine in exotic machines. */
    return __builtin_clz(v);
#elif defined(_MSC_VER) && (_MSC_VER >= 1400)
    FLAC__uint32 idx;
    _BitScanReverse(&idx, v);
    return idx ^ 31U;
#else
    return FLAC__clz_soft_uint32(v);
#endif
}

/* This one works with input 0 */
static inline unsigned int FLAC__clz2_uint32(FLAC__uint32 v)
{
    if (!v)
        return 32;
    return FLAC__clz_uint32(v);
}

/* An example of what FLAC__bitmath_ilog2() computes:
 *
 * ilog2( 0) = undefined
 * ilog2( 1) = 0
 * ilog2( 2) = 1
 * ilog2( 3) = 1
 * ilog2( 4) = 2
 * ilog2( 5) = 2
 * ilog2( 6) = 2
 * ilog2( 7) = 2
 * ilog2( 8) = 3
 * ilog2( 9) = 3
 * ilog2(10) = 3
 * ilog2(11) = 3
 * ilog2(12) = 3
 * ilog2(13) = 3
 * ilog2(14) = 3
 * ilog2(15) = 3
 * ilog2(16) = 4
 * ilog2(17) = 4
 * ilog2(18) = 4
 */

static inline unsigned FLAC__bitmath_ilog2(FLAC__uint32 v)
{
    return sizeof(FLAC__uint32) * CHAR_BIT  - 1 - FLAC__clz_uint32(v);
}


#ifdef FLAC__INTEGER_ONLY_LIBRARY /*Unused otherwise */

static inline unsigned FLAC__bitmath_ilog2_wide(FLAC__uint64 v)
{
    if (v == 0)
		return 0;
#if defined(__GNUC__) && (__GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
    return sizeof(FLAC__uint64) * CHAR_BIT - 1 - __builtin_clzll(v);
/* Sorry, only supported in win64/Itanium.. */
#elif (defined(_MSC_VER) && (_MSC_VER >= 1400)) && (defined(_M_IA64) || defined(_WIN64))
    FLAC__uint64 idx;
    _BitScanReverse64(&idx, v);
    return idx ^ 63U;
#else
/* Brain-damaged compilers will use the fastest possible way that is,
    de Bruijn sequences (http://supertech.csail.mit.edu/papers/debruijn.pdf)
    (C) Timothy B. Terriberry (tterribe@xiph.org) 2001-2009 LGPL (v2 or later).
*/
    static const unsigned char DEBRUIJN_IDX64[64]={
        0, 1, 2, 7, 3,13, 8,19, 4,25,14,28, 9,34,20,40,
        5,17,26,38,15,46,29,48,10,31,35,54,21,50,41,57,
        63, 6,12,18,24,27,33,39,16,37,45,47,30,53,49,56,
        62,11,23,32,36,44,52,55,61,22,43,51,60,42,59,58
    };
    int ret;
    ret= v>0;
    v|= v>>1;
    v|= v>>2;
    v|= v>>4;
    v|= v>>8;
    v|= v>>16;
    v|= v>>32;
    v= (v>>1)+1;
    ret+=DEBRUIJN_IDX64[v*0x218A392CD3D5DBF>>58&0x3F];
    return ret;
#endif
}
#endif

unsigned FLAC__bitmath_silog2(int v);
unsigned FLAC__bitmath_silog2_wide(FLAC__int64 v);

#endif
