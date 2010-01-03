/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __OPTMATH_H
#define __OPTMATH_H

#ifdef __SSE2__ // that comes from -msse2
#define __FORCE_SSE2__
#endif
// some maths
// taken from vorbis/lib/os.h, (C) 1994-2007 Xiph.Org Foundation http://www.xiph.org/

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


/* Optimized code path for x86_64 builds. Uses SSE2 intrinsics. This can be
   done safely because all x86_64 CPUs supports SSE2. */
#if (defined(__FORCE_SSE2__)) || (defined(_MSC_VER) && defined(_WIN64)) || (defined(__GNUC__) && defined (__x86_64__))
#pragma warning using sse2 for ftoi
#  define FPU_CONTROL

typedef int16_t fpu_control;

#include <emmintrin.h>
static __inline int ftoi(double f){
        return _mm_cvtsd_si32(_mm_load_sd(&f));
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
