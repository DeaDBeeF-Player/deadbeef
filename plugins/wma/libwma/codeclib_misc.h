/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: miscellaneous math and prototypes

 ********************************************************************/

#ifndef _CODECLIB_MISC_H_
#define _CODECLIB_MISC_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include "asm_arm.h"

#ifndef  _LOW_ACCURACY_
/* 64 bit multiply */

#if WORDS_BIGENDIAN
#define ROCKBOX_BIG_ENDIAN 1
#else
#define ROCKBOX_LITTLE_ENDIAN 1
#endif

#ifdef ROCKBOX_LITTLE_ENDIAN
union magic {
  struct {
    int32_t lo;
    int32_t hi;
  } halves;
  int64_t whole;
};
#elif defined(ROCKBOX_BIG_ENDIAN)
union magic {
  struct {
    int32_t hi;
    int32_t lo;
  } halves;
  int64_t whole;
};
#endif

#ifndef INCL_OPTIMIZED_MULT32
#define INCL_OPTIMIZED_MULT32
static inline int32_t MULT32(int32_t x, int32_t y) {
  union magic magic;
  magic.whole = (int64_t)x * y;
  return magic.halves.hi;
}
#endif

#ifndef INCL_OPTIMIZED_MULT31
#define INCL_OPTIMIZED_MULT31
static inline int32_t MULT31(int32_t x, int32_t y) {
  return MULT32(x,y)<<1;
}
#endif

#ifndef INCL_OPTIMIZED_MULT31_SHIFT15
#define INCL_OPTIMIZED_MULT31_SHIFT15
static inline int32_t MULT31_SHIFT15(int32_t x, int32_t y) {
  union magic magic;
  magic.whole  = (int64_t)x * y;
  return ((uint32_t)(magic.halves.lo)>>15) | ((magic.halves.hi)<<17);
}
#endif

#ifndef INCL_OPTIMIZED_MULT31_SHIFT16
#define INCL_OPTIMIZED_MULT31_SHIFT16
static inline int32_t MULT31_SHIFT16(int32_t x, int32_t y) {
  union magic magic;
  magic.whole  = (int64_t)x * y;
  return ((uint32_t)(magic.halves.lo)>>16) | ((magic.halves.hi)<<16);
}
#endif

#else
/* Rockbox: unused */
#if 0
/* 32 bit multiply, more portable but less accurate */

/*
 * Note: Precision is biased towards the first argument therefore ordering
 * is important.  Shift values were chosen for the best sound quality after
 * many listening tests.
 */

/*
 * For MULT32 and MULT31: The second argument is always a lookup table
 * value already preshifted from 31 to 8 bits.  We therefore take the
 * opportunity to save on text space and use unsigned char for those
 * tables in this case.
 */

static inline int32_t MULT32(int32_t x, int32_t y) {
  return (x >> 9) * y;  /* y preshifted >>23 */
}

static inline int32_t MULT31(int32_t x, int32_t y) {
  return (x >> 8) * y;  /* y preshifted >>23 */
}

static inline int32_t MULT31_SHIFT15(int32_t x, int32_t y) {
  return (x >> 6) * y;  /* y preshifted >>9 */
}
#endif
#endif

/*
 * The XPROD functions are meant to optimize the cross products found all
 * over the place in mdct.c by forcing memory operation ordering to avoid
 * unnecessary register reloads as soon as memory is being written to.
 * However this is only beneficial on CPUs with a sane number of general
 * purpose registers which exclude the Intel x86.  On Intel, better let the
 * compiler actually reload registers directly from original memory by using
 * macros.
 */

#ifndef INCL_OPTIMIZED_XPROD32
#define INCL_OPTIMIZED_XPROD32
/* replaced XPROD32 with a macro to avoid memory reference
   _x, _y are the results (must be l-values) */
#define XPROD32(_a, _b, _t, _v, _x, _y)     \
  { (_x)=MULT32(_a,_t)+MULT32(_b,_v);       \
    (_y)=MULT32(_b,_t)-MULT32(_a,_v); }
#endif

/* Rockbox: Unused */
/*
#ifdef __i386__

#define XPROD31(_a, _b, _t, _v, _x, _y)     \
  { *(_x)=MULT31(_a,_t)+MULT31(_b,_v);      \
    *(_y)=MULT31(_b,_t)-MULT31(_a,_v); }
#define XNPROD31(_a, _b, _t, _v, _x, _y)    \
  { *(_x)=MULT31(_a,_t)-MULT31(_b,_v);      \
    *(_y)=MULT31(_b,_t)+MULT31(_a,_v); }

#else
*/

#ifndef INCL_OPTIMIZED_XPROD31
#define INCL_OPTIMIZED_XPROD31
static inline void XPROD31(int32_t a, int32_t b,
                      int32_t t, int32_t v,
                      int32_t *x, int32_t *y)
{
  *x = MULT31(a, t) + MULT31(b, v);
  *y = MULT31(b, t) - MULT31(a, v);
}
#endif

#ifndef INCL_OPTIMIZED_XNPROD31
#define INCL_OPTIMIZED_XNPROD31
static inline void XNPROD31(int32_t a, int32_t b,
                       int32_t  t, int32_t  v,
                       int32_t *x, int32_t *y)
{
  *x = MULT31(a, t) - MULT31(b, v);
  *y = MULT31(b, t) + MULT31(a, v);
}
#endif
/*#endif*/

#ifndef INCL_OPTIMIZED_XPROD31_R
#define INCL_OPTIMIZED_XPROD31_R
#define XPROD31_R(_a, _b, _t, _v, _x, _y)\
{\
  _x = MULT31(_a, _t) + MULT31(_b, _v);\
  _y = MULT31(_b, _t) - MULT31(_a, _v);\
}
#endif

#ifndef INCL_OPTIMIZED_XNPROD31_R
#define INCL_OPTIMIZED_XNPROD31_R
#define XNPROD31_R(_a, _b, _t, _v, _x, _y)\
{\
  _x = MULT31(_a, _t) - MULT31(_b, _v);\
  _y = MULT31(_b, _t) + MULT31(_a, _v);\
}
#endif

#ifndef _V_VECT_OPS
#define _V_VECT_OPS

static inline
void vect_add(int32_t *x, const int32_t *y, int n)
{
  while (n>0) {
    *x++ += *y++;
    n--;
  }
}

static inline
void vect_copy(int32_t *x, const int32_t *y, int n)
{
  while (n>0) {
    *x++ = *y++;
    n--;
  }
}

static inline
void vect_mult_fw(int32_t *data, const int32_t *window, int n)
{
  while(n>0) {
    *data = MULT31(*data, *window);
    data++;
    window++;
    n--;
  }
}

static inline
void vect_mult_bw(int32_t *data, const int32_t *window, int n)
{
  while(n>0) {
    *data = MULT31(*data, *window);
    data++;
    window--;
    n--;
  }
}
#endif

/* not used anymore */
/*
#ifndef _V_CLIP_MATH
#define _V_CLIP_MATH

static inline int32_t CLIP_TO_15(int32_t x) {
  int ret=x;
  ret-= ((x<=32767)-1)&(x-32767);
  ret-= ((x>=-32768)-1)&(x+32768);
  return(ret);
}

#endif
*/
static inline int32_t VFLOAT_MULT(int32_t a,int32_t ap,
                      int32_t b,int32_t bp,
                      int32_t *p){
  if(a && b){
#ifndef _LOW_ACCURACY_
    *p=ap+bp+32;
    return MULT32(a,b);
#else
    *p=ap+bp+31;
    return (a>>15)*(b>>16);
#endif
  }else
    return 0;
}

/*static inline int32_t VFLOAT_MULTI(int32_t a,int32_t ap,
                      int32_t i,
                      int32_t *p){

  int ip=_ilog(abs(i))-31;
  return VFLOAT_MULT(a,ap,i<<-ip,ip,p);
}
*/
static inline int32_t VFLOAT_ADD(int32_t a,int32_t ap,
                      int32_t b,int32_t bp,
                      int32_t *p){

  if(!a){
    *p=bp;
    return b;
  }else if(!b){
    *p=ap;
    return a;
  }

  /* yes, this can leak a bit. */
  if(ap>bp){
    int shift=ap-bp+1;
    *p=ap+1;
    a>>=1;
    if(shift<32){
      b=(b+(1<<(shift-1)))>>shift;
    }else{
      b=0;
    }
  }else{
    int shift=bp-ap+1;
    *p=bp+1;
    b>>=1;
    if(shift<32){
      a=(a+(1<<(shift-1)))>>shift;
    }else{
      a=0;
    }
  }

  a+=b;
  if((a&0xc0000000)==0xc0000000 ||
     (a&0xc0000000)==0){
    a<<=1;
    (*p)--;
  }
  return(a);
}

#endif

