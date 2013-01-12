/*
 * Fixed Point IMDCT 
 * Copyright (c) 2002 The FFmpeg Project.
 * Copyright (c) 2010 Dave Hooper, Mohamed Tarek, Michael Giacomelli
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

#include "mdct.h"
#include "codeclib_misc.h"
#include "mdct_lookup.h"

/* Use to give gcc hints on which branch is most likely taken */
#if defined(__GNUC__) && __GNUC__ >= 3
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#ifndef ICODE_ATTR_TREMOR_MDCT
#define ICODE_ATTR_TREMOR_MDCT ICODE_ATTR
#endif

/**
 * Compute the middle half of the inverse MDCT of size N = 2^nbits
 * thus excluding the parts that can be derived by symmetry
 * @param output N/2 samples
 * @param input N/2 samples
 *
 * NOTE - CANNOT CURRENTLY OPERATE IN PLACE (input and output must
 *                                          not overlap or intersect at all)
 */
#define ICODE_ATTR
void ff_imdct_half(unsigned int nbits, fixed32 *output, const fixed32 *input) ICODE_ATTR_TREMOR_MDCT;
void ff_imdct_half(unsigned int nbits, fixed32 *output, const fixed32 *input)
{
    int n8, n4, n2, n, j;
    const fixed32 *in1, *in2;
    (void)j;
    n = 1 << nbits;

    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    FFTComplex *z = (FFTComplex *)output;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
    
    /* revtab comes from the fft; revtab table is sized for N=4096 size fft = 2^12.
       The fft is size N/4 so s->nbits-2, so our shift needs to be (12-(nbits-2)) */
    const int revtab_shift = (14- nbits);
    
    /* bitreverse reorder the input and rotate;   result here is in OUTPUT ... */
    /* (note that when using the current split radix, the bitreverse ordering is
        complex, meaning that this reordering cannot easily be done in-place) */
    /* Using the following pdf, you can see that it is possible to rearrange
       the 'classic' pre/post rotate with an alternative one that enables
       us to use fewer distinct twiddle factors.
       http://www.eurasip.org/Proceedings/Eusipco/Eusipco2006/papers/1568980508.pdf
       
       For prerotation, the factors are just sin,cos(2PI*i/N)
       For postrotation, the factors are sin,cos(2PI*(i+1/4)/N)
       
       Therefore, prerotation can immediately reuse the same twiddles as fft
       (for postrotation it's still a bit complex, we reuse the fft trig tables
        where we can, or a special table for N=2048, or interpolate between
        trig tables for N>2048)
       */
    const int32_t *T = sincos_lookup0;
    const int step = 2<<(12-nbits);
    const uint16_t * p_revtab=revtab;
    {
        const uint16_t * const p_revtab_end = p_revtab + n8;
#ifdef CPU_COLDFIRE
        asm volatile ("move.l (%[in2]), %%d0\n\t"
                      "move.l (%[in1]), %%d1\n\t"
                      "bra.s 1f\n\t"
                      "0:\n\t"
                      "movem.l (%[T]), %%d2-%%d3\n\t"

                      "addq.l #8, %[in1]\n\t"
                      "subq.l #8, %[in2]\n\t"

                      "lea (%[step]*4, %[T]), %[T]\n\t"

                      "mac.l %%d0, %%d3, (%[T]), %%d4, %%acc0;"
                      "msac.l %%d1, %%d2, (4, %[T]), %%d5, %%acc0;"
                      "mac.l %%d1, %%d3, (%[in1]), %%d1, %%acc1;"
                      "mac.l %%d0, %%d2, (%[in2]), %%d0, %%acc1;"

                      "addq.l #8, %[in1]\n\t"
                      "subq.l #8, %[in2]\n\t"

                      "mac.l %%d0, %%d5, %%acc2;"
                      "msac.l %%d1, %%d4, (%[p_revtab])+, %%d2, %%acc2;"
                      "mac.l %%d1, %%d5, (%[in1]), %%d1, %%acc3;"
                      "mac.l %%d0, %%d4, (%[in2]), %%d0, %%acc3;"

                      "clr.l %%d3\n\t"
                      "move.w %%d2, %%d3\n\t"
                      "eor.l %%d3, %%d2\n\t"
                      "swap %%d2\n\t"
                      "lsr.l %[revtab_shift], %%d2\n\t"

                      "movclr.l %%acc0, %%d4;"
                      "movclr.l %%acc1, %%d5;"
                      "lsl.l #3, %%d2\n\t"
                      "lea (%%d2, %[z]), %%a1\n\t"
                      "movem.l %%d4-%%d5, (%%a1)\n\t"

                      "lsr.l %[revtab_shift], %%d3\n\t"

                      "movclr.l %%acc2, %%d4;"
                      "movclr.l %%acc3, %%d5;"
                      "lsl.l #3, %%d3\n\t"
                      "lea (%%d3, %[z]), %%a1\n\t"
                      "movem.l %%d4-%%d5, (%%a1)\n\t"
                          
                      "lea (%[step]*4, %[T]), %[T]\n\t"

                      "1:\n\t"
                      "cmp.l %[p_revtab_end], %[p_revtab]\n\t"
                      "bcs.s 0b\n\t"
                      : [in1] "+a" (in1), [in2] "+a" (in2), [T] "+a" (T),
                        [p_revtab] "+a" (p_revtab)
                      : [z] "a" (z), [step] "d" (step), [revtab_shift] "d" (revtab_shift),
                        [p_revtab_end] "r" (p_revtab_end)
                      : "d0", "d1", "d2", "d3", "d4", "d5", "a1", "cc", "memory");
#else
        while(LIKELY(p_revtab < p_revtab_end))
        {
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[1], T[0], &z[j].re, &z[j].im );
            T += step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[1], T[0], &z[j].re, &z[j].im );
            T += step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
        }
#endif
    }
    {
        const uint16_t * const p_revtab_end = p_revtab + n8;
#ifdef CPU_COLDFIRE
        asm volatile ("move.l (%[in2]), %%d0\n\t"
                      "move.l (%[in1]), %%d1\n\t"
                      "bra.s 1f\n\t"
                      "0:\n\t"
                      "movem.l (%[T]), %%d2-%%d3\n\t"

                      "addq.l #8, %[in1]\n\t"
                      "subq.l #8, %[in2]\n\t"

                      "lea (%[step]*4, %[T]), %[T]\n\t"

                      "mac.l %%d0, %%d2, (%[T]), %%d4, %%acc0;"
                      "msac.l %%d1, %%d3, (4, %[T]), %%d5, %%acc0;"
                      "mac.l %%d1, %%d2, (%[in1]), %%d1, %%acc1;"
                      "mac.l %%d0, %%d3, (%[in2]), %%d0, %%acc1;"

                      "addq.l #8, %[in1]\n\t"
                      "subq.l #8, %[in2]\n\t"

                      "mac.l %%d0, %%d4, %%acc2;"
                      "msac.l %%d1, %%d5, (%[p_revtab])+, %%d2, %%acc2;"
                      "mac.l %%d1, %%d4, (%[in1]), %%d1, %%acc3;"
                      "mac.l %%d0, %%d5, (%[in2]), %%d0, %%acc3;"

                      "clr.l %%d3\n\t"
                      "move.w %%d2, %%d3\n\t"
                      "eor.l %%d3, %%d2\n\t"
                      "swap %%d2\n\t"
                      "lsr.l %[revtab_shift], %%d2\n\t"

                      "movclr.l %%acc0, %%d4;"
                      "movclr.l %%acc1, %%d5;"
                      "lsl.l #3, %%d2\n\t"
                      "lea (%%d2, %[z]), %%a1\n\t"
                      "movem.l %%d4-%%d5, (%%a1)\n\t"

                      "lsr.l %[revtab_shift], %%d3\n\t"

                      "movclr.l %%acc2, %%d4;"
                      "movclr.l %%acc3, %%d5;"
                      "lsl.l #3, %%d3\n\t"
                      "lea (%%d3, %[z]), %%a1\n\t"
                      "movem.l %%d4-%%d5, (%%a1)\n\t"
                          
                      "lea (%[step]*4, %[T]), %[T]\n\t"

                      "1:\n\t"
                      "cmp.l %[p_revtab_end], %[p_revtab]\n\t"
                      "bcs.s 0b\n\t"
                      : [in1] "+a" (in1), [in2] "+a" (in2), [T] "+a" (T),
                        [p_revtab] "+a" (p_revtab)
                      : [z] "a" (z), [step] "d" (-step), [revtab_shift] "d" (revtab_shift),
                        [p_revtab_end] "r" (p_revtab_end)
                      : "d0", "d1", "d2", "d3", "d4", "d5", "a1", "cc", "memory");
#else
        while(LIKELY(p_revtab < p_revtab_end))
        {
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[0], T[1], &z[j].re, &z[j].im);
            T -= step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
            j = (*p_revtab)>>revtab_shift;
            XNPROD31(*in2, *in1, T[0], T[1], &z[j].re, &z[j].im);
            T -= step;
            in1 += 2;
            in2 -= 2;
            p_revtab++;
        }
#endif
    }


    /* ... and so fft runs in OUTPUT buffer */
    ff_fft_calc_c(nbits-2, z);

    /* post rotation + reordering.  now keeps the result within the OUTPUT buffer */
    switch( nbits )
    {
        default:
        {
            fixed32 * z1 = (fixed32 *)(&z[0]);
            int magic_step = step>>2;
            int newstep;
            if(n<=1024)
            {
                T = sincos_lookup0 + magic_step;
                newstep = step>>1;
            }
            else
            {   
                T = sincos_lookup1;
                newstep = 2;
            }

#ifdef CPU_COLDFIRE
            fixed32 * z2 = (fixed32 *)(&z[n4]);
            int c = n4;
            if (newstep == 2)
            {
                asm volatile ("movem.l (%[z1]), %%d0-%%d1\n\t"
                              "addq.l #8, %[z1]\n\t"
                              "movem.l (%[T]), %%d2-%%d3\n\t"
                              "addq.l #8, %[T]\n\t"
                              "bra.s 1f\n\t"
                              "0:\n\t"
                              "msac.l %%d1, %%d2, (%[T])+, %%a3, %%acc0\n\t"
                              "mac.l  %%d0, %%d3, (%[T])+, %%a4, %%acc0\n\t"
                              
                              "msac.l %%d1, %%d3, -(%[z2]), %%d1, %%acc1\n\t"
                              "msac.l %%d0, %%d2, -(%[z2]), %%d0, %%acc1\n\t"

                              "msac.l %%d1, %%a4, (%[T])+, %%d2, %%acc2\n\t"
                              "mac.l  %%d0, %%a3, (%[T])+, %%d3, %%acc2\n\t"
                              "msac.l %%d0, %%a4, (%[z1])+, %%d0, %%acc3\n\t"
                              "msac.l %%d1, %%a3, (%[z1])+, %%d1, %%acc3\n\t"

                              "movclr.l %%acc0, %%a3\n\t"
                              "movclr.l %%acc3, %%a4\n\t"
                              "movem.l %%a3-%%a4, (-16, %[z1])\n\t"

                              "movclr.l %%acc1, %%a4\n\t"
                              "movclr.l %%acc2, %%a3\n\t"
                              "movem.l %%a3-%%a4, (%[z2])\n\t"

                              "subq.l #2, %[n]\n\t"
                              "1:\n\t"
                              "bhi.s 0b\n\t"
                              : [z1] "+a" (z1), [z2] "+a" (z2), [T] "+a" (T), [n] "+d" (c)
                              :
                              : "d0", "d1", "d2", "d3", "a3", "a4", "cc", "memory");
            }
            else
            {
                asm volatile ("movem.l (%[z1]), %%d0-%%d1\n\t"
                              "addq.l #8, %[z1]\n\t"
                              "movem.l (%[T]), %%d2-%%d3\n\t"
                              "lea (%[newstep]*4, %[T]), %[T]\n\t"
                              "bra.s 1f\n\t"
                              "0:\n\t"
                              "msac.l %%d1, %%d2, (%[T]), %%a3, %%acc0\n\t"
                              "mac.l  %%d0, %%d3, (4, %[T]), %%a4, %%acc0\n\t"
                              "msac.l %%d1, %%d3, -(%[z2]), %%d1, %%acc1\n\t"
                              "msac.l %%d0, %%d2, -(%[z2]), %%d0, %%acc1\n\t"

                              "lea (%[newstep]*4, %[T]), %[T]\n\t"
                              "msac.l %%d1, %%a4, (%[T]), %%d2, %%acc2\n\t"
                              "mac.l  %%d0, %%a3, (4, %[T]), %%d3, %%acc2\n\t"
                              "msac.l %%d0, %%a4, (%[z1])+, %%d0, %%acc3\n\t"
                              "msac.l %%d1, %%a3, (%[z1])+, %%d1, %%acc3\n\t"

                              "lea (%[newstep]*4, %[T]), %[T]\n\t"

                              "movclr.l %%acc0, %%a3\n\t"
                              "movclr.l %%acc3, %%a4\n\t"
                              "movem.l %%a3-%%a4, (-16, %[z1])\n\t"

                              "movclr.l %%acc1, %%a4\n\t"
                              "movclr.l %%acc2, %%a3\n\t"
                              "movem.l %%a3-%%a4, (%[z2])\n\t"

                              "subq.l #2, %[n]\n\t"
                              "1:\n\t"
                              "bhi.s 0b\n\t"
                              : [z1] "+a" (z1), [z2] "+a" (z2), [T] "+a" (T), [n] "+d" (c)
                              : [newstep] "d" (newstep)
                              : "d0", "d1", "d2", "d3", "a3", "a4", "cc", "memory");
            }
#else
            fixed32 * z2 = (fixed32 *)(&z[n4-1]);
            while(z1<z2)
            {
                fixed32 r0,i0,r1,i1;
                XNPROD31_R(z1[1], z1[0], T[0], T[1], r0, i1 ); T+=newstep;
                XNPROD31_R(z2[1], z2[0], T[1], T[0], r1, i0 ); T+=newstep;
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
            }
#endif 
            break;
        }

        case 12: /* n=4096 */
        {
            /* linear interpolation (50:50) between sincos_lookup0 and sincos_lookup1 */
            const int32_t * V = sincos_lookup1;
            T = sincos_lookup0;
            int32_t t0,t1,v0,v1;
            fixed32 * z1 = (fixed32 *)(&z[0]);
            fixed32 * z2 = (fixed32 *)(&z[n4-1]);

            t0 = T[0]>>1; t1=T[1]>>1;
        
            while(z1<z2)
            {
                fixed32 r0,i0,r1,i1;
                t0 += (v0 = (V[0]>>1));
                t1 += (v1 = (V[1]>>1));
                XNPROD31_R(z1[1], z1[0], t0, t1, r0, i1 );
                T+=2;
                v0 += (t0 = (T[0]>>1));
                v1 += (t1 = (T[1]>>1));
                XNPROD31_R(z2[1], z2[0], v1, v0, r1, i0 );
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
                V+=2;
            }
            
            break;
        }
        
        case 13: /* n = 8192 */
        {
            /* weight linear interpolation between sincos_lookup0 and sincos_lookup1
               specifically: 25:75 for first twiddle and 75:25 for second twiddle */
            const int32_t * V = sincos_lookup1;
            T = sincos_lookup0;
            int32_t t0,t1,v0,v1,q0,q1;
            fixed32 * z1 = (fixed32 *)(&z[0]);
            fixed32 * z2 = (fixed32 *)(&z[n4-1]);

            t0 = T[0]; t1=T[1];
        
            while(z1<z2)
            {
                fixed32 r0,i0,r1,i1;
                v0 = V[0]; v1 = V[1];
                t0 += (q0 = (v0-t0)>>1);
                t1 += (q1 = (v1-t1)>>1);
                XNPROD31_R(z1[1], z1[0], t0, t1, r0, i1 );
                t0 = v0-q0;
                t1 = v1-q1;
                XNPROD31_R(z2[1], z2[0], t1, t0, r1, i0 );
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
                T+=2;
                
                t0 = T[0]; t1 = T[1];
                v0 += (q0 = (t0-v0)>>1);
                v1 += (q1 = (t1-v1)>>1);
                XNPROD31_R(z1[1], z1[0], v0, v1, r0, i1 );
                v0 = t0-q0;
                v1 = t1-q1;
                XNPROD31_R(z2[1], z2[0], v1, v0, r1, i0 );
                z1[0] = -r0;
                z1[1] = -i0;
                z2[0] = -r1;
                z2[1] = -i1;
                z1+=2;
                z2-=2;
                V+=2;
            }
               
            break;
        }
    }
} 

/**
 * Compute inverse MDCT of size N = 2^nbits
 * @param output N samples
 * @param input N/2 samples
 * "In-place" processing can be achieved provided that:
 *            [0  ..  N/2-1 | N/2  ..  N-1 ]
 *            <----input---->
 *            <-----------output----------->
 *
 * The result of ff_imdct_half is to put the 'half' imdct here
 *
 *                          N/2          N-1
 *                          <--half imdct-->
 *
 * We want it here for the full imdct:
 *                   N/4      3N/4-1
 *                   <-------------->
 *
 * In addition we need to apply two symmetries to get the full imdct:
 *
 *           <AAAAAA>                <DDDDDD>
 *                   <BBBBBB><CCCCCC>
 *
 *           D is a reflection of C
 *           A is a reflection of B (but with sign flipped)
 *
 * We process the symmetries at the same time as we 'move' the half imdct
 * from [N/2,N-1] to [N/4,3N/4-1]
 *
 * TODO: find a way to make ff_imdct_half put the result in [N/4..3N/4-1]
 * This would require being able to use revtab 'inplace' (since the input
 * and output of imdct_half would then overlap somewhat)
 */
void ff_imdct_calc(unsigned int nbits, fixed32 *output, const fixed32 *input) ICODE_ATTR_TREMOR_MDCT;
#ifndef CPU_ARM
void ff_imdct_calc(unsigned int nbits, fixed32 *output, const fixed32 *input)
{
    const int n = (1<<nbits);
    const int n2 = (n>>1);
    const int n4 = (n>>2);
    
    /* tell imdct_half to put the output in [N/2..3N/4-1] i.e. output+n2 */
    ff_imdct_half(nbits,output+n2,input);

    fixed32 * in_r, * in_r2, * out_r, * out_r2;

    /* Copy BBBB to AAAA, reflected and sign-flipped.
       Also copy BBBB to its correct destination (from [N/2..3N/4-1] to [N/4..N/2-1]) */
    out_r = output;
    out_r2 = output+n2-8;
    in_r  = output+n2+n4-8;
    while(out_r<out_r2)
    {
#if defined CPU_COLDFIRE
        asm volatile( 
            "movem.l (%[in_r]), %%d0-%%d7\n\t"
            "movem.l %%d0-%%d7, (%[out_r2])\n\t"
            "neg.l %%d7\n\t"
            "move.l %%d7, (%[out_r])+\n\t"
            "neg.l %%d6\n\t"
            "move.l %%d6, (%[out_r])+\n\t"
            "neg.l %%d5\n\t"
            "move.l %%d5, (%[out_r])+\n\t"
            "neg.l %%d4\n\t"
            "move.l %%d4, (%[out_r])+\n\t"
            "neg.l %%d3\n\t"
            "move.l %%d3, (%[out_r])+\n\t"
            "neg.l %%d2\n\t"
            "move.l %%d2, (%[out_r])+\n\t"
            "lea.l (-8*4, %[in_r]), %[in_r]\n\t"
            "neg.l %%d1\n\t"
            "move.l %%d1, (%[out_r])+\n\t"
            "lea.l (-8*4, %[out_r2]), %[out_r2]\n\t"
            "neg.l %%d0\n\t"
            "move.l %%d0, (%[out_r])+\n\t"
            : [in_r] "+a" (in_r), [out_r] "+a" (out_r), [out_r2] "+a" (out_r2)
            :
            : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory" );
#else
        out_r[0]     = -(out_r2[7] = in_r[7]);
        out_r[1]     = -(out_r2[6] = in_r[6]);
        out_r[2]     = -(out_r2[5] = in_r[5]);
        out_r[3]     = -(out_r2[4] = in_r[4]);
        out_r[4]     = -(out_r2[3] = in_r[3]);
        out_r[5]     = -(out_r2[2] = in_r[2]);
        out_r[6]     = -(out_r2[1] = in_r[1]);
        out_r[7]     = -(out_r2[0] = in_r[0]);
        in_r -= 8;
        out_r += 8;
        out_r2 -= 8;
#endif
    }
    in_r = output + n2+n4;
    in_r2 = output + n-4;
    out_r = output + n2;
    out_r2 = output + n2 + n4 - 4;
    while(in_r<in_r2)
    {
#if defined CPU_COLDFIRE
        asm volatile(
            "movem.l (%[in_r]), %%d0-%%d3\n\t"
            "movem.l %%d0-%%d3, (%[out_r])\n\t"
            "movem.l (%[in_r2]), %%d4-%%d7\n\t"
            "movem.l %%d4-%%d7, (%[out_r2])\n\t"
            "move.l %%d0, %%a3\n\t"
            "move.l %%d3, %%d0\n\t"
            "move.l %%d1, %%d3\n\t"
            "movem.l %%d0/%%d2-%%d3/%%a3, (%[in_r2])\n\t"
            "move.l %%d7, %%d1\n\t"
            "move.l %%d6, %%d2\n\t"
            "move.l %%d5, %%d3\n\t"
            "movem.l %%d1-%%d4, (%[in_r])\n\t"
            "lea.l (4*4, %[in_r]), %[in_r]\n\t"
            "lea.l (-4*4, %[in_r2]), %[in_r2]\n\t"
            "lea.l (4*4, %[out_r]), %[out_r]\n\t"
            "lea.l (-4*4, %[out_r2]), %[out_r2]\n\t"
            : [in_r] "+a" (in_r), [in_r2] "+a" (in_r2),
              [out_r] "+a" (out_r), [out_r2] "+a" (out_r2)
            :
            : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a3", "memory", "cc" );
#else
        register fixed32 t0,t1,t2,t3;
        register fixed32 s0,s1,s2,s3;

        /* Copy and reflect CCCC to DDDD.  Because CCCC is already where
           we actually want to put DDDD this is a bit complicated.
         * So simultaneously do the following things:
         * 1. copy range from [n2+n4 .. n-1] to range[n2 .. n2+n4-1]
         * 2. reflect range from [n2+n4 .. n-1] inplace
         *
         *  [                      |                        ]
         *   ^a ->            <- ^b ^c ->               <- ^d
         *
         *  #1: copy from ^c to ^a
         *  #2: copy from ^d to ^b
         *  #3: swap ^c and ^d in place
         */
        /* #1 pt1 : load 4 words from ^c. */
        t0=in_r[0]; t1=in_r[1]; t2=in_r[2]; t3=in_r[3];
        /* #1 pt2 : write to ^a */
        out_r[0]=t0;out_r[1]=t1;out_r[2]=t2;out_r[3]=t3;
        /* #2 pt1 : load 4 words from ^d */
        s0=in_r2[0];s1=in_r2[1];s2=in_r2[2];s3=in_r2[3];
        /* #2 pt2 : write to ^b */
        out_r2[0]=s0;out_r2[1]=s1;out_r2[2]=s2;out_r2[3]=s3;
        /* #3 pt1 : write words from #2 to ^c */
        in_r[0]=s3;in_r[1]=s2;in_r[2]=s1;in_r[3]=s0;
        /* #3 pt2 : write words from #1 to ^d */
        in_r2[0]=t3;in_r2[1]=t2;in_r2[2]=t1;in_r2[3]=t0;

        in_r += 4;
        in_r2 -= 4;
        out_r += 4;
        out_r2 -= 4;
#endif
    }
}
#else
/* Follows the same structure as the canonical version above */
void ff_imdct_calc(unsigned int nbits, fixed32 *output, const fixed32 *input)
{
    const int n = (1<<nbits);
    const int n2 = (n>>1);
    const int n4 = (n>>2);
    
    ff_imdct_half(nbits,output+n2,input);

    fixed32 * in_r, * in_r2, * out_r, * out_r2;

    out_r = output;
    out_r2 = output+n2;
    in_r  = output+n2+n4;
    while(out_r<out_r2)
    {
        asm volatile( 
            "ldmdb %[in_r]!, {r0-r7}\n\t"
            "stmdb %[out_r2]!, {r0-r7}\n\t"
            "rsb r8,r0,#0\n\t"
            "rsb r0,r7,#0\n\t"
            "rsb r7,r1,#0\n\t"
            "rsb r1,r6,#0\n\t"
            "rsb r6,r2,#0\n\t"
            "rsb r2,r5,#0\n\t"
            "rsb r5,r3,#0\n\t"
            "rsb r3,r4,#0\n\t"
            "stmia %[out_r]!, {r0-r3,r5-r8}\n\t"
            : [in_r] "+r" (in_r), [out_r] "+r" (out_r), [out_r2] "+r" (out_r2)
            :
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "memory" );
    }
    in_r = output + n2+n4;
    in_r2 = output + n;
    out_r = output + n2;
    out_r2 = output + n2 + n4;
    while(in_r<in_r2)
    {
        asm volatile(
            "ldmia %[in_r], {r0-r3}\n\t"
            "stmia %[out_r]!, {r0-r3}\n\t"
            "ldmdb %[in_r2], {r5-r8}\n\t"
            "stmdb %[out_r2]!, {r5-r8}\n\t"
            "mov r4,r0\n\t"
            "mov r0,r3\n\t"
            "mov r3,r1\n\t"
            "stmdb %[in_r2]!, {r0,r2,r3,r4}\n\t"
            "mov r4,r8\n\t"
            "mov r8,r5\n\t"
            "mov r5,r7\n\t"
            "stmia %[in_r]!, {r4,r5,r6,r8}\n\t"
            :
            [in_r] "+r" (in_r), [in_r2] "+r" (in_r2), [out_r] "+r" (out_r), [out_r2] "+r" (out_r2)
            :
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "memory" );
    }
}
#endif
