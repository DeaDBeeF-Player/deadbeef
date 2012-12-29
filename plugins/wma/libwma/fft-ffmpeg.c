/*
 * FFT/IFFT transforms converted to integer precision
 * Copyright (c) 2010 Dave Hooper, Mohamed Tarek, Michael Giacomelli
 * Copyright (c) 2008 Loren Merritt
 * Copyright (c) 2002 Fabrice Bellard
 * Partly based on libdjbfft by D. J. Bernstein
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/fft.c
 * FFT/IFFT transforms.
 */


#ifdef CPU_ARM
// we definitely want CONFIG_SMALL undefined for ipod
// so we get the inlined version of fft16 (which is measurably faster)
#undef CONFIG_SMALL
#else
#undef CONFIG_SMALL 
#endif
 
#include "fft.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#include "codeclib_misc.h"
#include "mdct_lookup.h"

/* constants for fft_16 (same constants as in mdct_arm.S ... ) */
#define cPI1_8 (0x7641af3d) /* cos(pi/8) s.31 */
#define cPI2_8 (0x5a82799a) /* cos(2pi/8) = 1/sqrt(2) s.31 */
#define cPI3_8 (0x30fbc54d) /* cos(3pi/8) s.31 */

/* asm-optimised functions and/or macros */
#include "fft-ffmpeg_arm.h"

#ifndef ICODE_ATTR_TREMOR_MDCT
#define ICODE_ATTR_TREMOR_MDCT ICODE_ATTR
#endif

/* Use to give gcc hints on which branch is most likely taken */
#if defined(__GNUC__) && __GNUC__ >= 3
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#if 0
static int split_radix_permutation(int i, int n, int inverse)
{
    int m;
    if(n <= 2) return i&1;
    m = n >> 1;
    if(!(i&m))            return split_radix_permutation(i, m, inverse)*2;
    m >>= 1;
    if(inverse == !(i&m)) return split_radix_permutation(i, m, inverse)*4 + 1;
    else                  return split_radix_permutation(i, m, inverse)*4 - 1;
}

static void ff_fft_permute_c(FFTContext *s, FFTComplex *z)
{
    int j, k, np;
    FFTComplex tmp;
    //const uint16_t *revtab = s->revtab;
    np = 1 << s->nbits;
    
    const int revtab_shift = (12 - s->nbits);

    /* reverse */
    for(j=0;j<np;j++) {
        k = revtab[j]>>revtab_shift;
        if (k < j) {
            tmp = z[k];
            z[k] = z[j];
            z[j] = tmp;
        }
    }
}
#endif

#define BF(x,y,a,b) {\
    x = a - b;\
    y = a + b;\
}

#define BF_REV(x,y,a,b) {\
    x = a + b;\
    y = a - b;\
}

#ifndef FFT_FFMPEG_INCL_OPTIMISED_BUTTERFLIES
#define BUTTERFLIES(a0,a1,a2,a3) {\
    {\
        FFTSample temp1,temp2;\
        BF(temp1, temp2, t5, t1);\
        BF(a2.re, a0.re, a0.re, temp2);\
        BF(a3.im, a1.im, a1.im, temp1);\
    }\
    {\
        FFTSample temp1,temp2;\
        BF(temp1, temp2, t2, t6);\
        BF(a3.re, a1.re, a1.re, temp1);\
        BF(a2.im, a0.im, a0.im, temp2);\
    }\
}

// force loading all the inputs before storing any.
// this is slightly slower for small data, but avoids store->load aliasing
// for addresses separated by large powers of 2.
#define BUTTERFLIES_BIG(a0,a1,a2,a3) {\
    FFTSample r0=a0.re, i0=a0.im, r1=a1.re, i1=a1.im;\
    {\
        FFTSample temp1, temp2;\
        BF(temp1, temp2, t5, t1);\
        BF(a2.re, a0.re, r0, temp2);\
        BF(a3.im, a1.im, i1, temp1);\
    }\
    {\
        FFTSample temp1, temp2;\
        BF(temp1, temp2, t2, t6);\
        BF(a3.re, a1.re, r1, temp1);\
        BF(a2.im, a0.im, i0, temp2);\
    }\
}
#endif

/*
  see conjugate pair description in
  http://www.fftw.org/newsplit.pdf

  a0 = z[k]
  a1 = z[k+N/4]
  a2 = z[k+2N/4]
  a3 = z[k+3N/4]
  
  result:
  y[k]      = z[k]+w(z[k+2N/4])+w'(z[k+3N/4])
  y[k+N/4]  = z[k+N/4]-iw(z[k+2N/4])+iw'(z[k+3N/4])
  y[k+2N/4] = z[k]-w(z[k+2N/4])-w'(z[k+3N/4])
  y[k+3N/4] = z[k+N/4]+iw(z[k+2N/4])-iw'(z[k+3N/4])
  
  i.e.
  
  a0        = a0 +  (w.a2 + w'.a3)
  a1        = a1 - i(w.a2 - w'.a3)
  a2        = a0 -  (w.a2 + w'.a3)
  a3        = a1 + i(w.a2 - w'.a3)
  
  note re(w') = re(w) and im(w') = -im(w)
  
  so therefore
  
  re(a0)   = re(a0) + re(w.a2) + re(w.a3)
  im(a0)   = im(a0) + im(w.a2) - im(w.a3) etc

  and remember also that  
  Re([s+it][u+iv]) = su-tv
  Im([s+it][u+iv]) = sv+tu
  
  so
  Re(w'.(s+it)) = Re(w').s - Im(w').t = Re(w).s + Im(w).t
  Im(w'.(s+it)) = Re(w').t + Im(w').s = Re(w).t - Im(w).s

  For inverse dft we take the complex conjugate of all twiddle factors.
  Hence 
  
  a0        = a0 +  (w'.a2 + w.a3)
  a1        = a1 - i(w'.a2 - w.a3)
  a2        = a0 -  (w'.a2 + w.a3)
  a3        = a1 + i(w'.a2 - w.a3)
  
  Define t1 = Re(w'.a2)  =  Re(w)*Re(a2) + Im(w)*Im(a2)
         t2 = Im(w'.a2)  =  Re(w)*Im(a2) - Im(w)*Re(a2)
         t5 = Re(w.a3)   =  Re(w)*Re(a3) - Im(w)*Im(a3)
         t6 = Im(w.a3)   =  Re(w)*Im(a3) + Im(w)*Re(a3)
         
  Then we just output:
  a0.re = a0.re + ( t1 + t5 )
  a0.im = a0.im + ( t2 + t6 )
  a1.re = a1.re + ( t2 - t6 )   // since we multiply by -i and i(-i) = 1
  a1.im = a1.im - ( t1 - t5 )   // since we multiply by -i and 1(-i) = -i
  a2.re = a0.re - ( t1 + t5 )
  a2.im = a0.im - ( t1 + t5 )
  a3.re = a1.re - ( t2 - t6 )   // since we multiply by +i and i(+i) = -1
  a3.im = a1.im + ( t1 - t5 )   // since we multiply by +i and 1(+i) = i
    
    
*/

#ifndef FFT_FFMPEG_INCL_OPTIMISED_TRANSFORM
static inline FFTComplex* TRANSFORM(FFTComplex * z, unsigned int n, FFTSample wre, FFTSample wim)
{
    register FFTSample t1,t2,t5,t6,r_re,r_im;
    r_re = z[n*2].re;
    r_im = z[n*2].im;
    XPROD31_R(r_re, r_im, wre, wim, t1,t2);
    r_re = z[n*3].re;
    r_im = z[n*3].im;
    XNPROD31_R(r_re, r_im, wre, wim, t5,t6);
    BUTTERFLIES(z[0],z[n],z[n*2],z[n*3]);
    return z+1;
}

static inline FFTComplex* TRANSFORM_W01(FFTComplex * z, unsigned int n, const FFTSample * w)
{
    register const FFTSample wre=w[0],wim=w[1];
    register FFTSample t1,t2,t5,t6,r_re,r_im;
    r_re = z[n*2].re;
    r_im = z[n*2].im;
    XPROD31_R(r_re, r_im, wre, wim, t1,t2);
    r_re = z[n*3].re;
    r_im = z[n*3].im;
    XNPROD31_R(r_re, r_im, wre, wim, t5,t6);
    BUTTERFLIES(z[0],z[n],z[n*2],z[n*3]);
    return z+1;
}

static inline FFTComplex* TRANSFORM_W10(FFTComplex * z, unsigned int n, const FFTSample * w)
{
    register const FFTSample wim=w[0],wre=w[1];
    register FFTSample t1,t2,t5,t6,r_re,r_im;
    r_re = z[n*2].re;
    r_im = z[n*2].im;
    XPROD31_R(r_re, r_im, wre, wim, t1,t2);
    r_re = z[n*3].re;
    r_im = z[n*3].im;
    XNPROD31_R(r_re, r_im, wre, wim, t5,t6);
    BUTTERFLIES(z[0],z[n],z[n*2],z[n*3]);
    return z+1;
}

static inline FFTComplex* TRANSFORM_EQUAL(FFTComplex * z, unsigned int n)
{
    register FFTSample t1,t2,t5,t6,temp1,temp2;
    register FFTSample * my_z = (FFTSample *)(z);
    my_z += n*4;
    t2    = MULT31(my_z[0], cPI2_8);
    temp1 = MULT31(my_z[1], cPI2_8);
    my_z += n*2;
    temp2 = MULT31(my_z[0], cPI2_8);
    t5    = MULT31(my_z[1], cPI2_8);
    t1 = ( temp1 + t2 );
    t2 = ( temp1 - t2 );
    t6 = ( temp2 + t5 );
    t5 = ( temp2 - t5 );
    my_z -= n*6;
    BUTTERFLIES(z[0],z[n],z[n*2],z[n*3]);
    return z+1;
}

static inline FFTComplex* TRANSFORM_ZERO(FFTComplex * z, unsigned int n)
{
    FFTSample t1,t2,t5,t6;
    t1 = z[n*2].re;
    t2 = z[n*2].im;
    t5 = z[n*3].re;
    t6 = z[n*3].im;
    BUTTERFLIES(z[0],z[n],z[n*2],z[n*3]);
    return z+1;
}
#endif

/* z[0...8n-1], w[1...2n-1] */
#define ICODE_ATTR
static void pass(FFTComplex *z_arg, unsigned int STEP_arg, unsigned int n_arg) ICODE_ATTR_TREMOR_MDCT;
static void pass(FFTComplex *z_arg, unsigned int STEP_arg, unsigned int n_arg)
{
    register FFTComplex * z = z_arg;
    register unsigned int STEP = STEP_arg;
    register unsigned int n = n_arg;

    register const FFTSample *w = sincos_lookup0+STEP;
    /* wre = *(wim+1) .  ordering is sin,cos */
    register const FFTSample *w_end = sincos_lookup0+1024;

    /* first two are special (well, first one is special, but we need to do pairs) */
    z = TRANSFORM_ZERO(z,n);
    z = TRANSFORM_W10(z,n,w);
    w += STEP;
    /* first pass forwards through sincos_lookup0*/
    do {
        z = TRANSFORM_W10(z,n,w);
        w += STEP;
        z = TRANSFORM_W10(z,n,w);
        w += STEP;
    } while(LIKELY(w < w_end));
    /* second half: pass backwards through sincos_lookup0*/
    /* wim and wre are now in opposite places so ordering now [0],[1] */
    w_end=sincos_lookup0;
    while(LIKELY(w>w_end))
    {
        z = TRANSFORM_W01(z,n,w);
        w -= STEP;
        z = TRANSFORM_W01(z,n,w);
        w -= STEP;
    }
}

/* what is STEP?
   sincos_lookup0 has sin,cos pairs for 1/4 cycle, in 1024 points
   so half cycle would be 2048 points
   ff_cos_16 has 8 elements corresponding to 4 cos points and 4 sin points
   so each of the 4 points pairs corresponds to a 256*2-byte jump in sincos_lookup0
   8192/16 (from "ff_cos_16") is 512 bytes.
   i.e.  for fft16, STEP = 8192/16 */
#define DECL_FFT(n,n2,n4)\
static void fft##n(FFTComplex *z) ICODE_ATTR_TREMOR_MDCT;\
static void fft##n(FFTComplex *z)\
{\
    fft##n2(z);\
    fft##n4(z+n4*2);\
    fft##n4(z+n4*3);\
    pass(z,8192/n,n4);\
}

#ifndef FFT_FFMPEG_INCL_OPTIMISED_FFT4
static inline void fft4(FFTComplex *z)
{
    FFTSample t1, t2, t3, t4, t5, t6, t7, t8;

    BF(t3, t1, z[0].re, z[1].re); // t3=r1-r3 ; t1 = r1+r3
    BF(t8, t6, z[3].re, z[2].re); // t8=r7-r5 ; t6 = r7+r5

    BF(z[2].re, z[0].re, t1, t6); // r5=t1-t6 ; r1 = t1+t6

    BF(t4, t2, z[0].im, z[1].im); // t4=r2-r4 ; t2 = r2+r4
    BF(t7, t5, z[2].im, z[3].im); // t7=r6-r8 ; t5 = r6+r8

    BF(z[3].im, z[1].im, t4, t8); // r8=t4-t8 ; r4 = t4+t8
    BF(z[3].re, z[1].re, t3, t7); // r7=t3-t7 ; r3 = t3+t7
    BF(z[2].im, z[0].im, t2, t5); // r6=t2-t5 ; r2 = t2+t5
}
#endif

static void fft4_dispatch(FFTComplex *z)
{
    fft4(z);
}

#ifndef FFT_FFMPEG_INCL_OPTIMISED_FFT8
static inline void fft8(FFTComplex *z)
{
    fft4(z);
    FFTSample t1,t2,t3,t4,t7,t8;
    
    BF(t1, z[5].re, z[4].re, -z[5].re);
    BF(t2, z[5].im, z[4].im, -z[5].im);
    BF(t3, z[7].re, z[6].re, -z[7].re);
    BF(t4, z[7].im, z[6].im, -z[7].im);
    BF(t8, t1, t3, t1);
    BF(t7, t2, t2, t4);
    BF(z[4].re, z[0].re, z[0].re, t1);
    BF(z[4].im, z[0].im, z[0].im, t2);
    BF(z[6].re, z[2].re, z[2].re, t7);
    BF(z[6].im, z[2].im, z[2].im, t8);

    z++;
    TRANSFORM_EQUAL(z,2);
}
#endif

static void fft8_dispatch(FFTComplex *z)
{
    fft8(z);
}

#ifndef CONFIG_SMALL
static void fft16(FFTComplex *z) ICODE_ATTR_TREMOR_MDCT;
static void fft16(FFTComplex *z)
{
    fft8(z);
    fft4(z+8);
    fft4(z+12);

    TRANSFORM_ZERO(z,4);
    z+=2;
    TRANSFORM_EQUAL(z,4);
    z-=1;
    TRANSFORM(z,4,cPI1_8,cPI3_8);
    z+=2;
    TRANSFORM(z,4,cPI3_8,cPI1_8);
}
#else
DECL_FFT(16,8,4)
#endif
DECL_FFT(32,16,8)
DECL_FFT(64,32,16)
DECL_FFT(128,64,32)
DECL_FFT(256,128,64)
DECL_FFT(512,256,128)
DECL_FFT(1024,512,256)
DECL_FFT(2048,1024,512)
DECL_FFT(4096,2048,1024)

static void (*fft_dispatch[])(FFTComplex*) = {
    fft4_dispatch, fft8_dispatch, fft16, fft32, fft64, fft128, fft256, fft512, fft1024,
    fft2048, fft4096
};

void ff_fft_calc_c(int nbits, FFTComplex *z)
{
    fft_dispatch[nbits-2](z);
}

#if 0
int main (void)
{
#define PRECISION       16
#define FFT_SIZE 1024
#define ftofix32(x)       ((fixed32)((x) * (float)(1 << PRECISION) + ((x) < 0 ? -0.5 : 0.5)))
#define itofix32(x)       ((x) << PRECISION)
#define fixtoi32(x)       ((x) >> PRECISION)

    int             j;
    const long      N = FFT_SIZE;
    double          r[FFT_SIZE] = {0.0}, i[FFT_SIZE] = {0.0};
    long            n;
    double          t;
    double          amp, phase;
    clock_t         start, end;
    double          exec_time = 0;
    FFTContext      s;
    FFTComplex      z[FFT_SIZE];
    memset(z, 0, 64*sizeof(FFTComplex));

    /* Generate saw-tooth test data */
    for (n = 0; n < FFT_SIZE; n++)
    {
        t = (2 * M_PI * n)/N;
        /*z[n].re =  1.1      + sin(      t) +                
                   0.5      * sin(2.0 * t) +
                  (1.0/3.0) * sin(3.0 * t) +
                   0.25     * sin(4.0 * t) +
                   0.2      * sin(5.0 * t) +
                  (1.0/6.0) * sin(6.0 * t) +
                  (1.0/7.0) * sin(7.0 * t) ;*/
        z[n].re  =  ftofix32(cos(2*M_PI*n/64));
        //printf("z[%d] = %f\n", n, z[n].re);
        //getchar();
    }

    ff_fft_init(&s, 10, 1);
//start = clock();
//for(n = 0; n < 1000000; n++)
    ff_fft_permute_c(&s, z);
    ff_fft_calc_c(&s, z);
//end   = clock();
//exec_time = (((double)end-(double)start)/CLOCKS_PER_SEC);
    for(j = 0; j < FFT_SIZE; j++)
    {
        printf("%8.4f\n", sqrt(pow(fixtof32(z[j].re),2)+ pow(fixtof32(z[j].im), 2)));   
        //getchar();
    }
    printf("muls = %d, adds = %d\n", muls, adds);
//printf(" Time elapsed = %f\n", exec_time);
    //ff_fft_end(&s);

}
#endif
