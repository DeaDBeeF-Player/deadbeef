// most of the code below was taken from libvorbis/vorbis/lib/os.h
// under the conditions below
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2009             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

contents of the libvorbis COPYING:

Copyright (c) 2002-2008 Xiph.org Foundation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

- Neither the name of the Xiph.org Foundation nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __OPTMATH_H
#define __OPTMATH_H

#include <math.h>
#include <stdint.h>

#ifdef __SSE2__ // that comes from -msse2
#define __FORCE_SSE2__
#endif

#ifdef __SSE3__ // that comes from -msse3
#define __FORCE_SSE3__
#endif

/* Special i386 GCC implementation */
#if defined(__i386__) && defined(__GNUC__) && !defined(__BEOS__) && !defined(__FORCE_SSE2__)
#  define FPU_CONTROL
/* both GCC and MSVC are kinda stupid about rounding/casting to int.
   Because of encapsulation constraints (GCC can't see inside the asm
   block and so we end up doing stupid things like a store/load that
   is collectively a noop), we do it this way */

/* we must set up the fpu before this works!! */

typedef int16_t fpu_control;

static inline void fpu_setround(fpu_control *fpu){
  int16_t ret;
  int16_t temp;
  __asm__ __volatile__("fnstcw %0\n\t"
	  "movw %0,%%dx\n\t"
	  "orw $62463,%%dx\n\t"
	  "movw %%dx,%1\n\t"
	  "fldcw %1\n\t":"=m"(ret):"m"(temp): "dx");
  *fpu=ret;
}

static inline void fpu_restore(fpu_control fpu){
  __asm__ __volatile__("fldcw %0":: "m"(fpu));
}

/* assumes the FPU is in round mode! */
static inline int ftoi(double f){  /* yes, double!  Otherwise,
                                             we get extra fst/fld to
                                             truncate precision */
  int i;
  __asm__("fistl %0": "=m"(i) : "t"(f));
  return(i);
}
#endif /* Special i386 GCC implementation */


/* MSVC inline assembly. 32 bit only; inline ASM isn't implemented in the
 * 64 bit compiler */
#if defined(_MSC_VER) && !defined(_WIN64)
#  define FPU_CONTROL

typedef int16_t fpu_control;

static __inline int ftoi(double f){
	int i;
	__asm{
		fld f
		fistp i
	}
	return i;
}

static __inline void fpu_setround(fpu_control *fpu){
}

static __inline void fpu_restore(fpu_control fpu){
}

#endif /* Special MSVC 32 bit implementation */


#if (defined(__FORCE_SSE3__)) || (defined(_MSC_VER) && defined(_WIN64)) || (defined(__GNUC__) && defined (__x86_64__))
#  define FPU_CONTROL

typedef int16_t fpu_control;

#include <emmintrin.h>
static __inline int ftoi(float f){
    return _mm_cvtt_ss2si(_mm_load_ss(&f));
}

static __inline void fpu_setround(fpu_control *fpu){
}

static __inline void fpu_restore(fpu_control fpu){
}


/* Optimized code path for x86_64 builds. Uses SSE2 intrinsics. This can be
   done safely because all x86_64 CPUs supports SSE2. */
#elif (defined(__FORCE_SSE2__)) || (defined(_MSC_VER) && defined(_WIN64)) || (defined(__GNUC__) && defined (__x86_64__))
#  define FPU_CONTROL

typedef int16_t fpu_control;

#include <emmintrin.h>
static __inline int ftoi(float f){
        return _mm_cvtt_ss2si(_mm_load_ss(&f));
}

static __inline void fpu_setround(fpu_control *fpu){
}

static __inline void fpu_restore(fpu_control fpu){
}

#endif /* Special MSVC x64 implementation */


/* If no special implementation was found for the current compiler / platform,
   use the default implementation here: */
#ifndef FPU_CONTROL

typedef int fpu_control;

static int ftoi(double f){
        /* Note: MSVC and GCC (at least on some systems) round towards zero, thus,
           the floor() call is required to ensure correct roudning of
           negative numbers */
        return (int)floor(f+.5);
}

/* We don't have special code for this compiler/arch, so do it the slow way */
#  define fpu_setround(fpu_control) {}
#  define fpu_restore(fpu_control) {}

#endif /* default implementation */

#endif // __OPTMATH_H
