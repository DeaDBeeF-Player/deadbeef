/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Dave Hooper
 *
 * ARM optimisations for ffmpeg's fft (used in fft-ffmpeg.c)
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

#ifdef CPU_ARM

/* Start off with optimised variants of the butterflies that work
   nicely on arm */
/* 1.  where y and a share the same variable/register */
#define BF_OPT(x,y,a,b) {\
    y = a + b;\
    x = y - (b<<1);\
}

/* 2.  where y and b share the same variable/register */
#define BF_OPT2(x,y,a,b) {\
    x = a - b;\
    y = x + (b<<1);\
}

/* 3.  where y and b share the same variable/register (but y=(-b)) */
#define BF_OPT2_REV(x,y,a,b) {\
    x = a + b;\
    y = x - (b<<1);\
}


/* standard BUTTERFLIES package.  Note, we actually manually inline this
   in all the TRANSFORM macros below anyway */
#define FFT_FFMPEG_INCL_OPTIMISED_BUTTERFLIES
#define BUTTERFLIES(a0,a1,a2,a3) {\
    {\
        BF_OPT(t1, t5, t5, t1);\
        BF_OPT(t6, t2, t2, t6);\
        BF_OPT(a2.re, a0.re, a0.re, t5);\
        BF_OPT(a2.im, a0.im, a0.im, t2);\
        BF_OPT(a3.re, a1.re, a1.re, t6);\
        BF_OPT(a3.im, a1.im, a1.im, t1);\
    }\
}

#define FFT_FFMPEG_INCL_OPTIMISED_TRANSFORM

static inline FFTComplex* TRANSFORM( FFTComplex* z, int n, FFTSample wre, FFTSample wim )
{
    register FFTSample t1,t2 asm("r5"),t5 asm("r6"),t6 asm("r7"),r_re asm("r8"),r_im asm("r9");
    z += n*2; /* z[o2] */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
    XPROD31_R(r_re, r_im, wre, wim, t1,t2);
    
    z += n; /* z[o3] */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
    XNPROD31_R(r_re, r_im, wre, wim, t5,t6);
    
    BF_OPT(t1, t5, t5, t1);
    BF_OPT(t6, t2, t2, t6);

    {    
        register FFTSample rt0temp asm("r4");
        /*{*/
        /*   BF_OPT(t1, t5, t5, t1);*/
        /*    BF_OPT(t6, t2, t2, t6);*/
        /*    BF_OPT(a2.re, a0.re, a0.re, t5);*/
        /*    BF_OPT(a2.im, a0.im, a0.im, t2);*/
        /*    BF_OPT(a3.re, a1.re, a1.re, t6);*/
        /*    BF_OPT(a3.im, a1.im, a1.im, t1);*/
        /*}*/
        z -= n*3;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(rt0temp, r_re, r_re, t5);
        BF_OPT(t2,      r_im, r_im, t2);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory" );
        z += n;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(t5, r_re, r_re, t6);
        BF_OPT(t6, r_im, r_im, t1);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* my_z[0] = rt0temp; my_z[1] = t2; */
        asm volatile( "stmia %[my_z], {%[rt0temp],%[t2]}\n\t"::[my_z] "r" (z), [rt0temp] "r" (rt0temp), [t2] "r" (t2):"memory");
    }
    z += n;
   
    /* my_z[0] = t5; my_z[1] = t6; */
    asm volatile( "stmia %[my_z]!, {%[t5],%[t6]}\n\t":[my_z] "+r" (z) : [t5] "r" (t5), [t6] "r" (t6):"memory");
    z -= n*3;
    return(z);
}

static inline FFTComplex* TRANSFORM_W01( FFTComplex* z, int n, const FFTSample* w )
{
    register FFTSample t1,t2 asm("r5"),t5 asm("r6"),t6 asm("r7"),r_re asm("r8"),r_im asm("r9");
    
    /* load wre,wim into t5,t6 */
    asm volatile( "ldmia %[w], {%[wre], %[wim]}\n\t":[wre] "=r" (t5), [wim] "=r" (t6):[w] "r" (w));
    z += n*2; /* z[o2] -- 2n * 2 since complex numbers */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
    XPROD31_R(r_re, r_im, t5 /*wre*/, t6 /*wim*/, t1,t2);

    z += n; /* z[o3] */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
    XNPROD31_R(r_re, r_im, t5 /*wre*/, t6 /*wim*/, t5,t6);
    
    BF_OPT(t1, t5, t5, t1);
    BF_OPT(t6, t2, t2, t6);
    {
        register FFTSample rt0temp asm("r4");
        /*{*/
        /*   BF_OPT(t1, t5, t5, t1);*/
        /*    BF_OPT(t6, t2, t2, t6);*/
        /*    BF_OPT(a2.re, a0.re, a0.re, t5);*/
        /*    BF_OPT(a2.im, a0.im, a0.im, t2);*/
        /*    BF_OPT(a3.re, a1.re, a1.re, t6);*/
        /*    BF_OPT(a3.im, a1.im, a1.im, t1);*/
        /*}*/
        z -= n*3;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(rt0temp, r_re, r_re, t5);
        BF_OPT(t2,      r_im, r_im, t2);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(t5, r_re, r_re, t6);
        BF_OPT(t6, r_im, r_im, t1);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* my_z[0] = rt0temp; my_z[1] = t2; */
        asm volatile( "stmia %[my_z], {%[rt0temp],%[t2]}\n\t"::[my_z] "r" (z), [rt0temp] "r" (rt0temp), [t2] "r" (t2):"memory");
    }
    z += n;

    /* my_z[0] = t5; my_z[1] = t6; */
    asm volatile( "stmia %[my_z]!, {%[t5],%[t6]}\n\t":[my_z] "+r" (z) : [t5] "r" (t5), [t6] "r" (t6):"memory");
    z -= n*3;
    return(z);
}

static inline FFTComplex* TRANSFORM_W10( FFTComplex* z, int n, const FFTSample* w )
{
    register FFTSample t1,t2 asm("r5"),t5 asm("r6"),t6 asm("r7"),r_re asm("r8"),r_im asm("r9");
    
    /* load wim,wre into t5,t6 */
    asm volatile( "ldmia %[w], {%[wim], %[wre]}\n\t":[wim] "=r" (t5), [wre] "=r" (t6):[w] "r" (w));
    z += n*2; /* z[o2] -- 2n * 2 since complex numbers */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
    XPROD31_R(r_re, r_im, t6 /*wim*/, t5 /*wre*/, t1,t2);

    z += n; /* z[o3] */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
    XNPROD31_R(r_re, r_im, t6 /*wim*/, t5 /*wre*/, t5,t6);
    
    BF_OPT(t1, t5, t5, t1);
    BF_OPT(t6, t2, t2, t6);
    {
        register FFTSample rt0temp asm("r4");
        /*{*/
        /*   BF_OPT(t1, t5, t5, t1);*/
        /*    BF_OPT(t6, t2, t2, t6);*/
        /*    BF_OPT(a2.re, a0.re, a0.re, t5);*/
        /*    BF_OPT(a2.im, a0.im, a0.im, t2);*/
        /*    BF_OPT(a3.re, a1.re, a1.re, t6);*/
        /*    BF_OPT(a3.im, a1.im, a1.im, t1);*/
        /*}*/
        z -= n*3;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(rt0temp, r_re, r_re, t5);
        BF_OPT(t2,      r_im, r_im, t2);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(t5, r_re, r_re, t6);
        BF_OPT(t6, r_im, r_im, t1);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* my_z[0] = rt0temp; my_z[1] = t2; */
        asm volatile( "stmia %[my_z], {%[rt0temp],%[t2]}\n\t"::[my_z] "r" (z), [rt0temp] "r" (rt0temp), [t2] "r" (t2):"memory");
    }
    z += n;

    /* my_z[0] = t5; my_z[1] = t6; */
    asm volatile( "stmia %[my_z]!, {%[t5],%[t6]}\n\t":[my_z] "+r" (z) : [t5] "r" (t5), [t6] "r" (t6):"memory");
    z -= n*3;
    return(z);
}

static inline FFTComplex* TRANSFORM_EQUAL( FFTComplex* z, int n )
{
    register FFTSample t1,t2 asm("r5"),t5 asm("r6"),t6 asm("r7"),r_re asm("r8"),r_im asm("r9");

    z += n*2; /* z[o2] -- 2n * 2 since complex numbers */
    asm volatile( "ldmia %[my_z], {%[t5],%[t6]}\n\t":[t5] "=r" (t5), [t6] "=r" (t6):[my_z] "r" (z));
    z += n; /* z[o3] */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));

/**/
/*t2 = MULT32(cPI2_8, t5);*/
/*t1 = MULT31(cPI2_8, t6);*/
/*t6 = MULT31(cPI2_8, r_re);*/
/*t5 = MULT32(cPI2_8, r_im);*/

/*t1 = ( t1 + (t2<<1) );*/
/*t2 = ( t1 - (t2<<2) );*/
/*t6 = ( t6 + (t5<<1) );*/
/*t5 = ( t6 - (t5<<2) );*/
/**/
    t2   = MULT31(cPI2_8, t5);
    t6   = MULT31(cPI2_8, t6);
    r_re = MULT31(cPI2_8, r_re);
    t5   = MULT31(cPI2_8, r_im);
    
    t1 = ( t6 + t2 );
    t2 = ( t6 - t2 );
    t6 = ( r_re + t5 );
    t5 = ( r_re - t5 );
    
    BF_OPT(t1, t5, t5, t1);
    BF_OPT(t6, t2, t2, t6);
    {
        register FFTSample rt0temp asm("r4");
        /*{*/
        /*   BF_OPT(t1, t5, t5, t1);*/
        /*    BF_OPT(t6, t2, t2, t6);*/
        /*    BF_OPT(a2.re, a0.re, a0.re, t5);*/
        /*    BF_OPT(a2.im, a0.im, a0.im, t2);*/
        /*    BF_OPT(a3.re, a1.re, a1.re, t6);*/
        /*    BF_OPT(a3.im, a1.im, a1.im, t1);*/
        /*}*/
        z -= n*3;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(rt0temp, r_re, r_re, t5);
        BF_OPT(t2,      r_im, r_im, t2);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(t5, r_re, r_re, t6);
        BF_OPT(t6, r_im, r_im, t1);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* my_z[0] = rt0temp; my_z[1] = t2; */
        asm volatile( "stmia %[my_z], {%[rt0temp],%[t2]}\n\t"::[my_z] "r" (z), [rt0temp] "r" (rt0temp), [t2] "r" (t2):"memory");
    }
    z += n;

    /* my_z[0] = t5; my_z[1] = t6; */
    asm volatile( "stmia %[my_z]!, {%[t5],%[t6]}\n\t":[my_z] "+r" (z) : [t5] "r" (t5), [t6] "r" (t6):"memory");
    z -= n*3;
    return(z);
}

static inline FFTComplex* TRANSFORM_ZERO( FFTComplex* z, int n )
{
    register FFTSample t1,t2 asm("r5"),t5 asm("r6"),t6 asm("r7"), r_re asm("r8"), r_im asm("r9");

    z += n*2; /* z[o2] -- 2n * 2 since complex numbers */
    asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
    z += n; /* z[o3] */
    asm volatile( "ldmia %[my_z], {%[t5],%[t6]}\n\t":[t5] "=r" (t5), [t6] "=r" (t6):[my_z] "r" (z));

    BF_OPT(t1, t5, t5, r_re);
    BF_OPT(t6, t2, r_im, t6);
    {
        register FFTSample rt0temp asm("r4");
        /*{*/
        /*   BF_OPT(t1, t5, t5, t1);*/
        /*    BF_OPT(t6, t2, t2, t6);*/
        /*    BF_OPT(a2.re, a0.re, a0.re, t5);*/
        /*    BF_OPT(a2.im, a0.im, a0.im, t2);*/
        /*    BF_OPT(a3.re, a1.re, a1.re, t6);*/
        /*    BF_OPT(a3.im, a1.im, a1.im, t1);*/
        /*}*/
        z -= n*3;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(rt0temp, r_re, r_re, t5);
        BF_OPT(t2,      r_im, r_im, t2);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* r_re = my_z[0]; r_im = my_z[1]; */
        asm volatile( "ldmia %[my_z], {%[r_re],%[r_im]}\n\t":[r_re] "=r" (r_re), [r_im] "=r" (r_im):[my_z] "r" (z));
        BF_OPT(t5, r_re, r_re, t6);
        BF_OPT(t6, r_im, r_im, t1);
        /* my_z[0] = r_re; my_z[1] = r_im; */
        asm volatile( "stmia %[my_z], {%[r_re],%[r_im]}\n\t"::[my_z] "r" (z), [r_re] "r" (r_re), [r_im] "r" (r_im):"memory");
        z += n;
        /* my_z[0] = rt0temp; my_z[1] = t2; */
        asm volatile( "stmia %[my_z], {%[rt0temp],%[t2]}\n\t"::[my_z] "r" (z), [rt0temp] "r" (rt0temp), [t2] "r" (t2):"memory");
    }
    z += n;

    /* my_z[0] = t5; my_z[1] = t6; */
    asm volatile( "stmia %[my_z]!, {%[t5],%[t6]}\n\t":[my_z] "+r" (z) : [t5] "r" (t5), [t6] "r" (t6):"memory");
    z -= n*3;
    return(z);
}

#define FFT_FFMPEG_INCL_OPTIMISED_FFT4
static inline FFTComplex* fft4(FFTComplex * z)
{
    FFTSample temp;
    
    /* input[0..7] -> output[0..7] */
    /* load r1=z[0],r2=z[1],...,r8=z[7] */
    asm volatile(
      "ldmia %[z], {r1-r8}\n\t"
      "add r1,r1,r3\n\t"         /* r1 :=t1 */
      "sub r3,r1,r3, lsl #1\n\t" /* r3 :=t3 */
      "sub r7,r7,r5\n\t"         /* r10:=t8 */
      "add r5,r7,r5, lsl #1\n\t" /* r5 :=t6 */
      
      "add r1,r1,r5\n\t"                 /* r1 = o[0] */
      "sub r5,r1,r5, lsl #1\n\t"         /* r5 = o[4] */
      
      "add r2,r2,r4\n\t"         /* r2 :=t2 */
      "sub r4,r2,r4, lsl #1\n\t" /* r9 :=t4 */
      
      "add %[temp],r6,r8\n\t"        /* r10:=t5 */
      "sub r6,r6,r8\n\t"         /* r6 :=t7 */
      
      "sub r8,r4,r7\n\t"                 /* r8 = o[7]*/ 
      "add r4,r4,r7\n\t"                 /* r4 = o[3]*/ 
      "sub r7,r3,r6\n\t"                 /* r7 = o[6]*/ 
      "add r3,r3,r6\n\t"                 /* r3 = o[2]*/ 
      "sub r6,r2,%[temp]\n\t"                /* r6 = o[5]*/ 
      "add r2,r2,%[temp]\n\t"                /* r2 = o[1]*/ 
      
      "stmia %[z]!, {r1-r8}\n\t"
      : /* outputs */ [z] "+r" (z), [temp] "=r" (temp)
      : /* inputs */
      : /* clobbers */
      "r1","r2","r3","r4","r5","r6","r7","r8","memory"
   );
   return z;
}

#define FFT_FFMPEG_INCL_OPTIMISED_FFT8
        /* The chunk of asm below is equivalent to the following:
        
        // first load in z[4].re thru z[7].im into local registers
        // ...
        BF_OPT2_REV(z[4].re, z[5].re, z[4].re, z[5].re); // x=a+b; y=x-(b<<1)
        BF_OPT2_REV(z[4].im, z[5].im, z[4].im, z[5].im);
        BF_REV     (temp, z[7].re, z[6].re, z[7].re);  // x=a+b; y=a-b;
        BF_REV     (z[6].re, z[7].im, z[6].im, z[7].im);
        // save z[7].re and z[7].im as those are complete now
        // z[5].re and z[5].im are also complete now but save these later on
        
        BF(z[6].im, z[4].re, temp, z[4].re);        // x=a-b; y=a+b
        BF_OPT(z[6].re, z[4].im, z[4].im, z[6].re); // y=a+b; x=y-(b<<1)
        // now load z[2].re and z[2].im
        // ...        
        BF_OPT(z[6].re, z[2].re, z[2].re, z[6].re); // y=a+b; x=y-(b<<1)
        BF_OPT(z[6].im, z[2].im, z[2].im, z[6].im); // y=a+b; x=y-(b<<1)
        // Now save z[6].re and z[6].im, along with z[5].re and z[5].im
        // for efficiency.  Also save z[2].re and z[2].im.
        // Now load z[0].re and z[0].im
        // ...
        
        BF_OPT(z[4].re, z[0].re, z[0].re, z[4].re); // y=a+b; x=y-(b<<1)
        BF_OPT(z[4].im, z[0].im, z[0].im, z[4].im); // y=a+b; x=y-(b<<1)
        // Finally save out z[4].re, z[4].im, z[0].re and z[0].im
        // ...
        */
static inline void fft8(FFTComplex * z)
{
    FFTComplex* m4 = fft4(z);
    {
        /* note that we increment z_ptr on the final stmia, which 
           leaves z_ptr pointing to z[1].re ready for the Transform step */
           
        register FFTSample temp;

        asm volatile(
            /* read in z[4].re thru z[7].im */
            "ldmia %[z4_ptr]!, {r1-r8}\n\t"
            /* (now points one word past &z[7].im) */
            "add r1,r1,r3\n\t"
            "sub r3,r1,r3,lsl #1\n\t"
            "add r2,r2,r4\n\t"
            "sub r4,r2,r4,lsl #1\n\t"
            "add %[temp],r5,r7\n\t"
            "sub r7,r5,r7\n\t"
            "add r5,r6,r8\n\t"
            "sub r8,r6,r8\n\t"

            "stmdb %[z4_ptr]!, {r7,r8}\n\t" /* write z[7].re,z[7].im  straight away */
                                            /* Note, registers r7 & r8 now free */

            "sub r6,%[temp],r1\n\t"
            "add r1,%[temp],r1\n\t"
            "add r2,r2,r5\n\t"
            "sub r5,r2,r5,lsl #1\n\t"
            "add %[temp], %[z_ptr], #16\n\t"  /* point to &z[2].re */
            "ldmia %[temp],{r7,r8}\n\t"  /* load z[2].re and z[2].im */
            "add r7,r7,r5\n\t"
            "sub r5,r7,r5,lsl #1\n\t"
            "add r8,r8,r6\n\t"
            "sub r6,r8,r6,lsl #1\n\t"

            /* write out z[5].re, z[5].im, z[6].re, z[6].im in one go*/
            "stmdb %[z4_ptr]!, {r3-r6}\n\t"
            "stmia %[temp],{r7,r8}\n\t" /* write out z[2].re, z[2].im */
            "ldmia %[z_ptr],{r7,r8}\n\t" /* load r[0].re, r[0].im */

            "add r7,r7,r1\n\t"
            "sub r1,r7,r1,lsl #1\n\t"
            "add r8,r8,r2\n\t"
            "sub r2,r8,r2,lsl #1\n\t"

            "stmia %[z_ptr]!,{r7,r8}\n\t" /* write out z[0].re, z[0].im */
            "stmdb %[z4_ptr], {r1,r2}\n\t" /* write out z[4].re, z[4].im */
            : [z4_ptr] "+r" (m4), [temp] "=r" (temp), [z_ptr] "+r" (z)
            :
            : "r1","r2","r3","r4","r5","r6","r7","r8","memory"
        );
    }

    TRANSFORM_EQUAL(z,2);
}

#endif // CPU_ARM
