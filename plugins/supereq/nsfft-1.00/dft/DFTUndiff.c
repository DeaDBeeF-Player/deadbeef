#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "SIMDBase.h"
#include "SIMDBaseUndiff.h"
#include "DFT.h"
#include "DFTUndiff.h"

//

#define SIN(x) sin(x)
#define COS(x) cos(x)

#define SQRT2_2 .7071067811865475244008443621048490392848359376884740365883398689953L

#ifndef M_PIl
#define M_PIl 3.141592653589793238462643383279502884197169399375105820974944592307L
#endif

//

static inline void srBut2(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  int32_t o = p->offset1;
  SIMDBase_VECT t0, t1;

  t0 = SIMDBase_ADDm(&s[o  ], &s[o+2]); t1 = SIMDBase_SUBm(&s[o  ], &s[o+2]);
  SIMDBase_STOR(&s[o  ], t0); SIMDBase_STOR(&s[o+2], t1);
  t0 = SIMDBase_ADDm(&s[o+1], &s[o+3]); t1 = SIMDBase_SUBm(&s[o+1], &s[o+3]);
  SIMDBase_STOR(&s[o+1], t0); SIMDBase_STOR(&s[o+3], t1);
}

static inline void srButForward4(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  int32_t o = p->offset1;
  SIMDBase_VECT t0r, t0i, t1r, t1i, t2r, t2i, t3r, t3i;

  t0r = SIMDBase_ADDm(&s[o+0], &s[o+4]); t2r = SIMDBase_SUBm(&s[o+0], &s[o+4]);
  t0i = SIMDBase_ADDm(&s[o+1], &s[o+5]); t2i = SIMDBase_SUBm(&s[o+1], &s[o+5]);
  t1r = SIMDBase_ADDm(&s[o+2], &s[o+6]); t3i = SIMDBase_SUBm(&s[o+2], &s[o+6]);
  t1i = SIMDBase_ADDm(&s[o+7], &s[o+3]); t3r = SIMDBase_SUBm(&s[o+7], &s[o+3]);

  SIMDBase_STOR(&s[o+0], SIMDBase_ADDi(t0r, t1r)); SIMDBase_STOR(&s[o+1], SIMDBase_ADDi(t0i, t1i));
  SIMDBase_STOR(&s[o+2], SIMDBase_SUBi(t0r, t1r)); SIMDBase_STOR(&s[o+3], SIMDBase_SUBi(t0i, t1i));
  SIMDBase_STOR(&s[o+4], SIMDBase_SUBi(t2r, t3r)); SIMDBase_STOR(&s[o+5], SIMDBase_SUBi(t2i, t3i));
  SIMDBase_STOR(&s[o+6], SIMDBase_ADDi(t2r, t3r)); SIMDBase_STOR(&s[o+7], SIMDBase_ADDi(t2i, t3i));
}

static inline void srButBackward4(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  int32_t o = p->offset1;

  SIMDBase_VECT t0r, t0i, t1r, t1i;
  SIMDBase_VECT s0 = SIMDBase_LOAD(&s[o+0]), s1 = SIMDBase_LOAD(&s[o+1]), s2 = SIMDBase_LOAD(&s[o+2]), s3 = SIMDBase_LOAD(&s[o+3]);

  t0r = SIMDBase_ADDi(s0, s2); t0i = SIMDBase_SUBi(s0, s2); s0 = t0r; s2 = t0i;
  t0r = SIMDBase_ADDi(s1, s3); t0i = SIMDBase_SUBi(s1, s3); s1 = t0r; s3 = t0i;
  t0r = SIMDBase_ADDm(&s[o+4], &s[o+6]); t1i = SIMDBase_SUBm(&s[o+4], &s[o+6]);
  t0i = SIMDBase_ADDm(&s[o+7], &s[o+5]); t1r = SIMDBase_SUBm(&s[o+7], &s[o+5]);

  SIMDBase_STOR(&s[o+4], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[o+5], SIMDBase_SUBi(s1, t0i));
  SIMDBase_STOR(&s[o+6], SIMDBase_SUBi(s2, t1r)); SIMDBase_STOR(&s[o+7], SIMDBase_SUBi(s3, t1i));
  SIMDBase_STOR(&s[o+0], SIMDBase_ADDi(s0, t0r)); SIMDBase_STOR(&s[o+1], SIMDBase_ADDi(s1, t0i));
  SIMDBase_STOR(&s[o+2], SIMDBase_ADDi(s2, t1r)); SIMDBase_STOR(&s[o+3], SIMDBase_ADDi(s3, t1i));
}

static inline void srButForward8(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  int32_t o = p->offset1;
  SIMDBase_VECT t0r, t0i, t1r, t1i, t2r, t2i, t3r, t3i;

  SIMDBase_VECT s0 = SIMDBase_LOAD(&s[o+ 0]), s1 = SIMDBase_LOAD(&s[o+ 1]), s2 = SIMDBase_LOAD(&s[o+ 2]), s3 = SIMDBase_LOAD(&s[o+ 3]);
  SIMDBase_VECT s4 = SIMDBase_LOAD(&s[o+ 4]), s5 = SIMDBase_LOAD(&s[o+ 5]), s6 = SIMDBase_LOAD(&s[o+ 6]), s7 = SIMDBase_LOAD(&s[o+ 7]);
  SIMDBase_VECT s8 = SIMDBase_LOAD(&s[o+ 8]), s9 = SIMDBase_LOAD(&s[o+ 9]), sa = SIMDBase_LOAD(&s[o+10]) ,sb = SIMDBase_LOAD(&s[o+11]);
  SIMDBase_VECT sc = SIMDBase_LOAD(&s[o+12]), sd = SIMDBase_LOAD(&s[o+13]), se = SIMDBase_LOAD(&s[o+14]), sf = SIMDBase_LOAD(&s[o+15]);

  t2r = SIMDBase_SUBi(s0, s8); t2i = SIMDBase_SUBi(s1, s9);
  t3r = SIMDBase_SUBi(sd, s5); t3i = SIMDBase_SUBi(s4, sc); 
  
  s0 = SIMDBase_ADDi(s0, s8); s1 = SIMDBase_ADDi(s1, s9);
  s4 = SIMDBase_ADDi(s4, sc); s5 = SIMDBase_ADDi(s5, sd);

  s8 = SIMDBase_SUBi(t2r, t3r); s9 = SIMDBase_SUBi(t2i, t3i);
  sc = SIMDBase_ADDi(t2r, t3r); sd = SIMDBase_ADDi(t2i, t3i);

  t2r = SIMDBase_SUBi(s2, sa); t2i = SIMDBase_SUBi(s3, sb);
  t3r = SIMDBase_SUBi(sf, s7); t3i = SIMDBase_SUBi(s6, se);

  s2 = SIMDBase_ADDi(s2, sa); s3 = SIMDBase_ADDi(s3, sb);
  s6 = SIMDBase_ADDi(s6, se); s7 = SIMDBase_ADDi(s7, sf);

  t0r = SIMDBase_SUBi(t2r, t3r); t1r = SIMDBase_ADDi(t2r, t3r);
  t0i = SIMDBase_SUBi(t2i, t3i); t1i = SIMDBase_ADDi(t2i, t3i);

  sa = SIMDBase_MULi(SIMDBase_ADDi(t0r, t0i), SIMDBase_SET1( SQRT2_2));
  sb = SIMDBase_MULi(SIMDBase_SUBi(t0i, t0r), SIMDBase_SET1( SQRT2_2));
  se = SIMDBase_MULi(SIMDBase_SUBi(t1i, t1r), SIMDBase_SET1( SQRT2_2));
  sf = SIMDBase_MULi(SIMDBase_ADDi(t1r, t1i), SIMDBase_SET1(-SQRT2_2));

  SIMDBase_STOR(&s[o+ 8], SIMDBase_ADDi(s8, sa)); SIMDBase_STOR(&s[o+ 9], SIMDBase_ADDi(s9, sb));
  SIMDBase_STOR(&s[o+10], SIMDBase_SUBi(s8, sa)); SIMDBase_STOR(&s[o+11], SIMDBase_SUBi(s9, sb));

  SIMDBase_STOR(&s[o+12], SIMDBase_ADDi(sc, se)); SIMDBase_STOR(&s[o+13], SIMDBase_ADDi(sd, sf));
  SIMDBase_STOR(&s[o+14], SIMDBase_SUBi(sc, se)); SIMDBase_STOR(&s[o+15], SIMDBase_SUBi(sd, sf));

  t0r = SIMDBase_ADDi(s0, s4); t2r = SIMDBase_SUBi(s0, s4);
  t0i = SIMDBase_ADDi(s1, s5); t2i = SIMDBase_SUBi(s1, s5);

  t1r = SIMDBase_ADDi(s2, s6); t3i = SIMDBase_SUBi(s2, s6);
  t1i = SIMDBase_ADDi(s3, s7); t3r = SIMDBase_SUBi(s7, s3);

  SIMDBase_STOR(&s[o+0], SIMDBase_ADDi(t0r, t1r)); SIMDBase_STOR(&s[o+1], SIMDBase_ADDi(t0i, t1i));
  SIMDBase_STOR(&s[o+2], SIMDBase_SUBi(t0r, t1r)); SIMDBase_STOR(&s[o+3], SIMDBase_SUBi(t0i, t1i));
  SIMDBase_STOR(&s[o+4], SIMDBase_SUBi(t2r, t3r)); SIMDBase_STOR(&s[o+5], SIMDBase_SUBi(t2i, t3i));
  SIMDBase_STOR(&s[o+6], SIMDBase_ADDi(t2r, t3r)); SIMDBase_STOR(&s[o+7], SIMDBase_ADDi(t2i, t3i));
}

static void srButBackward8(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  int32_t o = p->offset1;
  SIMDBase_VECT t0r, t0i, t1r, t1i;

  SIMDBase_VECT s0 = SIMDBase_LOAD(&s[o+ 0]), s1 = SIMDBase_LOAD(&s[o+ 1]), s2 = SIMDBase_LOAD(&s[o+ 2]), s3 = SIMDBase_LOAD(&s[o+ 3]);
  SIMDBase_VECT s4 = SIMDBase_LOAD(&s[o+ 4]), s5 = SIMDBase_LOAD(&s[o+ 5]), s6 = SIMDBase_LOAD(&s[o+ 6]), s7 = SIMDBase_LOAD(&s[o+ 7]);
  SIMDBase_VECT s8 = SIMDBase_LOAD(&s[o+ 8]), s9 = SIMDBase_LOAD(&s[o+ 9]), sa = SIMDBase_LOAD(&s[o+10]) ,sb = SIMDBase_LOAD(&s[o+11]);
  SIMDBase_VECT sc = SIMDBase_LOAD(&s[o+12]), sd = SIMDBase_LOAD(&s[o+13]), se = SIMDBase_LOAD(&s[o+14]), sf = SIMDBase_LOAD(&s[o+15]);

  t0r = SIMDBase_ADDi(s8, sa); t0i = SIMDBase_SUBi(s8, sa); s8 = t0r; sa = t0i;
  t0r = SIMDBase_ADDi(s9, sb); t0i = SIMDBase_SUBi(s9, sb); s9 = t0r; sb = t0i;
  t0r = SIMDBase_ADDi(sc, se); t0i = SIMDBase_SUBi(sc, se); sc = t0r; se = t0i;
  t0r = SIMDBase_ADDi(sd, sf); t0i = SIMDBase_SUBi(sd, sf); sd = t0r; sf = t0i;
  t0r = SIMDBase_ADDi(s0, s2); t0i = SIMDBase_SUBi(s0, s2); s0 = t0r; s2 = t0i;
  t0r = SIMDBase_ADDi(s1, s3); t0i = SIMDBase_SUBi(s1, s3); s1 = t0r; s3 = t0i;

  t0r = SIMDBase_ADDi(s4, s6); t0i = SIMDBase_ADDi(s7, s5);
  t1r = SIMDBase_SUBi(s7, s5); t1i = SIMDBase_SUBi(s4, s6);

  s4 = SIMDBase_SUBi(s0, t0r); s5 = SIMDBase_SUBi(s1, t0i);
  s6 = SIMDBase_SUBi(s2, t1r); s7 = SIMDBase_SUBi(s3, t1i);
  s0 = SIMDBase_ADDi(s0, t0r); s1 = SIMDBase_ADDi(s1, t0i);
  s2 = SIMDBase_ADDi(s2, t1r); s3 = SIMDBase_ADDi(s3, t1i);

  t0r = SIMDBase_ADDi(s8, sc); t0i = SIMDBase_ADDi(s9, sd);
  t1r = SIMDBase_SUBi(sd, s9); t1i = SIMDBase_SUBi(s8, sc);

  s8 = SIMDBase_SUBi(s0, t0r); s9 = SIMDBase_SUBi(s1, t0i);
  sc = SIMDBase_SUBi(s4, t1r); sd = SIMDBase_SUBi(s5, t1i);
  s0 = SIMDBase_ADDi(s0, t0r); s1 = SIMDBase_ADDi(s1, t0i);
  s4 = SIMDBase_ADDi(s4, t1r); s5 = SIMDBase_ADDi(s5, t1i);

  t0r = SIMDBase_MULi(SIMDBase_SUBi(sa, sb), SIMDBase_SET1( SQRT2_2));
  t0i = SIMDBase_MULi(SIMDBase_ADDi(sa, sb), SIMDBase_SET1( SQRT2_2));
  t1r = SIMDBase_MULi(SIMDBase_ADDi(se, sf), SIMDBase_SET1(-SQRT2_2));
  t1i = SIMDBase_MULi(SIMDBase_SUBi(se, sf), SIMDBase_SET1( SQRT2_2));

  sa = t0r; sb = t0i; se = t1r; sf = t1i;

  t0r = SIMDBase_ADDi(sa, se); t0i = SIMDBase_ADDi(sb, sf);
  t1r = SIMDBase_SUBi(sf, sb); t1i = SIMDBase_SUBi(sa, se);

  sa = SIMDBase_SUBi(s2, t0r); sb = SIMDBase_SUBi(s3, t0i);
  se = SIMDBase_SUBi(s6, t1r); sf = SIMDBase_SUBi(s7, t1i);
  s2 = SIMDBase_ADDi(s2, t0r); s3 = SIMDBase_ADDi(s3, t0i);
  s6 = SIMDBase_ADDi(s6, t1r); s7 = SIMDBase_ADDi(s7, t1i);

  SIMDBase_STOR(&s[o+ 0], s0); SIMDBase_STOR(&s[o+ 1], s1); SIMDBase_STOR(&s[o+ 2], s2); SIMDBase_STOR(&s[o+ 3], s3);
  SIMDBase_STOR(&s[o+ 4], s4); SIMDBase_STOR(&s[o+ 5], s5); SIMDBase_STOR(&s[o+ 6], s6); SIMDBase_STOR(&s[o+ 7], s7);
  SIMDBase_STOR(&s[o+ 8], s8); SIMDBase_STOR(&s[o+ 9], s9); SIMDBase_STOR(&s[o+10], sa); SIMDBase_STOR(&s[o+11], sb);
  SIMDBase_STOR(&s[o+12], sc); SIMDBase_STOR(&s[o+13], sd); SIMDBase_STOR(&s[o+14], se); SIMDBase_STOR(&s[o+15], sf);
}

#if 0
static inline void srButForwardSub(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  SIMDBase_REAL *tbl = p->ptTable[p->log2butlen];

  int32_t i0 = p->offset1;
  int32_t i1 = i0 + p->stride;
  int32_t i2 = i1 + p->stride;
  int32_t i3 = i2 + p->stride;
  int32_t im = i1;

  int32_t p0 = p->offset2 & (p->butlen*4-1);

  while(i0 < im) {
    SIMDBase_VECT t0r, t0i, t1r, t1i;
    SIMDBase_VECT s00, s01, s10, s11, s20, s21, s30, s31;
    SIMDBase_VECT a0, a1, a2, a3;

    s00 = SIMDBase_LOAD(&s[i0+0]), s01 = SIMDBase_LOAD(&s[i0+1]);
    s10 = SIMDBase_LOAD(&s[i1+0]), s11 = SIMDBase_LOAD(&s[i1+1]);
    s20 = SIMDBase_LOAD(&s[i2+0]), s21 = SIMDBase_LOAD(&s[i2+1]);
    s30 = SIMDBase_LOAD(&s[i3+0]), s31 = SIMDBase_LOAD(&s[i3+1]);

    t0r = SIMDBase_SUBi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t0i = SIMDBase_SUBi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));

    t1r = SIMDBase_ADDi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t1i = SIMDBase_ADDi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));
    
    a0 = SIMDBase_LOAD1(&tbl[p0+0]); a1 = SIMDBase_LOAD1(&tbl[p0+1]);
    a2 = SIMDBase_LOAD1(&tbl[p0+2]); a3 = SIMDBase_LOAD1(&tbl[p0+3]);

    SIMDBase_STOR(&s[i0  ], SIMDBase_ADDi(s00, s20)); SIMDBase_STOR(&s[i0+1], SIMDBase_ADDi(s01, s21));
    SIMDBase_STOR(&s[i1  ], SIMDBase_ADDi(s10, s30)); SIMDBase_STOR(&s[i1+1], SIMDBase_ADDi(s11, s31));

#ifndef SIMDBase_FMADD_AVAILABLE
    SIMDBase_STOR(&s[i2  ], SIMDBase_SUBi(SIMDBase_MULi(t0r, a0), SIMDBase_MULi(t0i, a1)));
    SIMDBase_STOR(&s[i2+1], SIMDBase_ADDi(SIMDBase_MULi(t0r, a1), SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3  ], SIMDBase_SUBi(SIMDBase_MULi(t1r, a2), SIMDBase_MULi(t1i, a3)));
    SIMDBase_STOR(&s[i3+1], SIMDBase_ADDi(SIMDBase_MULi(t1r, a3), SIMDBase_MULi(t1i, a2)));
#else
    SIMDBase_STOR(&s[i2  ], SIMDBase_FMSUBi(t0i, a1, SIMDBase_MULi(t0r, a0)));
    SIMDBase_STOR(&s[i2+1], SIMDBase_FMADDi(t0r, a1, SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3  ], SIMDBase_FMSUBi(t1i, a3, SIMDBase_MULi(t1r, a2)));
    SIMDBase_STOR(&s[i3+1], SIMDBase_FMADDi(t1r, a3, SIMDBase_MULi(t1i, a2)));
#endif

    i0 += 2; i1 += 2; i2 += 2; i3 += 2;
    p0 += 4;
  }
}
#endif

#if 0
static inline void srButBackwardSub(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  SIMDBase_REAL *tbl = p->ptTable[p->log2butlen];

  int32_t i0 = p->offset1;
  int32_t i1 = i0 + p->stride;
  int32_t i2 = i1 + p->stride;
  int32_t i3 = i2 + p->stride;
  int32_t im = i1;

  int32_t p0 = p->offset2 & (p->butlen*4-1);

  while(i0 < im) {
    SIMDBase_VECT t0r, t0i, t1r, t1i, u, v;
    SIMDBase_VECT s00, s01, s10, s11, s20, s21, s30, s31;
    SIMDBase_VECT a0, a1, a2, a3;

    s20 = SIMDBase_LOAD(&s[i2+0]); s21 = SIMDBase_LOAD(&s[i2+1]);
    a0 = SIMDBase_LOAD1(&tbl[p0+0]); a1 = SIMDBase_LOAD1(&tbl[p0+1]);
    u = SIMDBase_ADDi(SIMDBase_MULi(s20, a0), SIMDBase_MULi(s21, a1));

    s30 = SIMDBase_LOAD(&s[i3+0]); s31 = SIMDBase_LOAD(&s[i3+1]);
    a2 = SIMDBase_LOAD1(&tbl[p0+2]); a3 = SIMDBase_LOAD1(&tbl[p0+3]);
    v = SIMDBase_ADDi(SIMDBase_MULi(s30, a2), SIMDBase_MULi(s31, a3));

    t0r = SIMDBase_ADDi(u, v); t1i = SIMDBase_SUBi(u, v);
    u = SIMDBase_SUBi(SIMDBase_MULi(s31, a2), SIMDBase_MULi(s30, a3));
    v = SIMDBase_SUBi(SIMDBase_MULi(s21, a0), SIMDBase_MULi(s20, a1));
    t0i = SIMDBase_ADDi(u, v); t1r = SIMDBase_SUBi(u, v);

    s00 = SIMDBase_LOAD(&s[i0+0]); s01 = SIMDBase_LOAD(&s[i0+1]);
    s10 = SIMDBase_LOAD(&s[i1+0]); s11 = SIMDBase_LOAD(&s[i1+1]);

    SIMDBase_STOR(&s[i2+0], SIMDBase_SUBi(s00, t0r)); SIMDBase_STOR(&s[i0+0], SIMDBase_ADDi(s00, t0r));
    SIMDBase_STOR(&s[i2+1], SIMDBase_SUBi(s01, t0i)); SIMDBase_STOR(&s[i0+1], SIMDBase_ADDi(s01, t0i));
    SIMDBase_STOR(&s[i3+0], SIMDBase_SUBi(s10, t1r)); SIMDBase_STOR(&s[i1+0], SIMDBase_ADDi(s10, t1r));
    SIMDBase_STOR(&s[i3+1], SIMDBase_SUBi(s11, t1i)); SIMDBase_STOR(&s[i1+1], SIMDBase_ADDi(s11, t1i));

    i0 += 2; i1 += 2; i2 += 2; i3 += 2;
    p0 += 4;
  }
}

static void srButBackwardSubUnrolled(DFTUndiff *p) {
  srButBackwardSub(p);
}
#endif

static inline void srButForwardSubUnrolled(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  SIMDBase_REAL *tbl = p->ptTable[p->log2butlen];

  int32_t i0 = p->offset1;
  int32_t i1 = i0 + p->stride;
  int32_t i2 = i1 + p->stride;
  int32_t i3 = i2 + p->stride;
  int32_t im = i1;

  int32_t p0 = p->offset2 & (p->butlen*4-1);

  while(i0 < im) {
    SIMDBase_VECT t0r, t0i, t1r, t1i;
    SIMDBase_VECT s00, s01, s10, s11, s20, s21, s30, s31;
    SIMDBase_VECT a0, a1, a2, a3;

    //

    s00 = SIMDBase_LOAD(&s[i0+0]); s01 = SIMDBase_LOAD(&s[i0+1]);
    s10 = SIMDBase_LOAD(&s[i1+0]); s11 = SIMDBase_LOAD(&s[i1+1]);
    s20 = SIMDBase_LOAD(&s[i2+0]); s21 = SIMDBase_LOAD(&s[i2+1]);
    s30 = SIMDBase_LOAD(&s[i3+0]); s31 = SIMDBase_LOAD(&s[i3+1]);

    t0r = SIMDBase_SUBi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t0i = SIMDBase_SUBi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));

    t1r = SIMDBase_ADDi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t1i = SIMDBase_ADDi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));
    
    a0 = SIMDBase_LOAD1(&tbl[p0+0]); a1 = SIMDBase_LOAD1(&tbl[p0+1]);
    a2 = SIMDBase_LOAD1(&tbl[p0+2]); a3 = SIMDBase_LOAD1(&tbl[p0+3]);

    SIMDBase_STOR(&s[i0  ], SIMDBase_ADDi(s00, s20)); SIMDBase_STOR(&s[i0+1], SIMDBase_ADDi(s01, s21));
    SIMDBase_STOR(&s[i1  ], SIMDBase_ADDi(s10, s30)); SIMDBase_STOR(&s[i1+1], SIMDBase_ADDi(s11, s31));

#ifndef SIMDBase_FMADD_AVAILABLE
    SIMDBase_STOR(&s[i2  ], SIMDBase_SUBi(SIMDBase_MULi(t0r, a0), SIMDBase_MULi(t0i, a1)));
    SIMDBase_STOR(&s[i2+1], SIMDBase_ADDi(SIMDBase_MULi(t0r, a1), SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3  ], SIMDBase_SUBi(SIMDBase_MULi(t1r, a2), SIMDBase_MULi(t1i, a3)));
    SIMDBase_STOR(&s[i3+1], SIMDBase_ADDi(SIMDBase_MULi(t1r, a3), SIMDBase_MULi(t1i, a2)));
#else
    SIMDBase_STOR(&s[i2  ], SIMDBase_FMSUBi(t0i, a1, SIMDBase_MULi(t0r, a0)));
    SIMDBase_STOR(&s[i2+1], SIMDBase_FMADDi(t0r, a1, SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3  ], SIMDBase_FMSUBi(t1i, a3, SIMDBase_MULi(t1r, a2)));
    SIMDBase_STOR(&s[i3+1], SIMDBase_FMADDi(t1r, a3, SIMDBase_MULi(t1i, a2)));
#endif

    //

    s00 = SIMDBase_LOAD(&s[i0+2]); s01 = SIMDBase_LOAD(&s[i0+3]);
    s10 = SIMDBase_LOAD(&s[i1+2]); s11 = SIMDBase_LOAD(&s[i1+3]);
    s20 = SIMDBase_LOAD(&s[i2+2]); s21 = SIMDBase_LOAD(&s[i2+3]);
    s30 = SIMDBase_LOAD(&s[i3+2]); s31 = SIMDBase_LOAD(&s[i3+3]);

    t0r = SIMDBase_SUBi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t0i = SIMDBase_SUBi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));

    t1r = SIMDBase_ADDi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t1i = SIMDBase_ADDi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));
    
    a0 = SIMDBase_LOAD1(&tbl[p0+4]); a1 = SIMDBase_LOAD1(&tbl[p0+5]);
    a2 = SIMDBase_LOAD1(&tbl[p0+6]); a3 = SIMDBase_LOAD1(&tbl[p0+7]);

    SIMDBase_STOR(&s[i0+2], SIMDBase_ADDi(s00, s20)); SIMDBase_STOR(&s[i0+3], SIMDBase_ADDi(s01, s21));
    SIMDBase_STOR(&s[i1+2], SIMDBase_ADDi(s10, s30)); SIMDBase_STOR(&s[i1+3], SIMDBase_ADDi(s11, s31));

#ifndef SIMDBase_FMADD_AVAILABLE
    SIMDBase_STOR(&s[i2+2], SIMDBase_SUBi(SIMDBase_MULi(t0r, a0), SIMDBase_MULi(t0i, a1)));
    SIMDBase_STOR(&s[i2+3], SIMDBase_ADDi(SIMDBase_MULi(t0r, a1), SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3+2], SIMDBase_SUBi(SIMDBase_MULi(t1r, a2), SIMDBase_MULi(t1i, a3)));
    SIMDBase_STOR(&s[i3+3], SIMDBase_ADDi(SIMDBase_MULi(t1r, a3), SIMDBase_MULi(t1i, a2)));
#else
    SIMDBase_STOR(&s[i2+2], SIMDBase_FMSUBi(t0i, a1, SIMDBase_MULi(t0r, a0)));
    SIMDBase_STOR(&s[i2+3], SIMDBase_FMADDi(t0r, a1, SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3+2], SIMDBase_FMSUBi(t1i, a3, SIMDBase_MULi(t1r, a2)));
    SIMDBase_STOR(&s[i3+3], SIMDBase_FMADDi(t1r, a3, SIMDBase_MULi(t1i, a2)));
#endif

    //

    s00 = SIMDBase_LOAD(&s[i0+4]); s01 = SIMDBase_LOAD(&s[i0+5]);
    s10 = SIMDBase_LOAD(&s[i1+4]); s11 = SIMDBase_LOAD(&s[i1+5]);
    s20 = SIMDBase_LOAD(&s[i2+4]); s21 = SIMDBase_LOAD(&s[i2+5]);
    s30 = SIMDBase_LOAD(&s[i3+4]); s31 = SIMDBase_LOAD(&s[i3+5]);

    t0r = SIMDBase_SUBi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t0i = SIMDBase_SUBi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));

    t1r = SIMDBase_ADDi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t1i = SIMDBase_ADDi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));
    
    a0 = SIMDBase_LOAD1(&tbl[p0+ 8]); a1 = SIMDBase_LOAD1(&tbl[p0+ 9]);
    a2 = SIMDBase_LOAD1(&tbl[p0+10]); a3 = SIMDBase_LOAD1(&tbl[p0+11]);

    SIMDBase_STOR(&s[i0+4], SIMDBase_ADDi(s00, s20)); SIMDBase_STOR(&s[i0+5], SIMDBase_ADDi(s01, s21));
    SIMDBase_STOR(&s[i1+4], SIMDBase_ADDi(s10, s30)); SIMDBase_STOR(&s[i1+5], SIMDBase_ADDi(s11, s31));

#ifndef SIMDBase_FMADD_AVAILABLE
    SIMDBase_STOR(&s[i2+4], SIMDBase_SUBi(SIMDBase_MULi(t0r, a0), SIMDBase_MULi(t0i, a1)));
    SIMDBase_STOR(&s[i2+5], SIMDBase_ADDi(SIMDBase_MULi(t0r, a1), SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3+4], SIMDBase_SUBi(SIMDBase_MULi(t1r, a2), SIMDBase_MULi(t1i, a3)));
    SIMDBase_STOR(&s[i3+5], SIMDBase_ADDi(SIMDBase_MULi(t1r, a3), SIMDBase_MULi(t1i, a2)));
#else
    SIMDBase_STOR(&s[i2+4], SIMDBase_FMSUBi(t0i, a1, SIMDBase_MULi(t0r, a0)));
    SIMDBase_STOR(&s[i2+5], SIMDBase_FMADDi(t0r, a1, SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3+4], SIMDBase_FMSUBi(t1i, a3, SIMDBase_MULi(t1r, a2)));
    SIMDBase_STOR(&s[i3+5], SIMDBase_FMADDi(t1r, a3, SIMDBase_MULi(t1i, a2)));
#endif

    //

    s00 = SIMDBase_LOAD(&s[i0+6]); s01 = SIMDBase_LOAD(&s[i0+7]);
    s10 = SIMDBase_LOAD(&s[i1+6]); s11 = SIMDBase_LOAD(&s[i1+7]);
    s20 = SIMDBase_LOAD(&s[i2+6]); s21 = SIMDBase_LOAD(&s[i2+7]);
    s30 = SIMDBase_LOAD(&s[i3+6]); s31 = SIMDBase_LOAD(&s[i3+7]);

    t0r = SIMDBase_SUBi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t0i = SIMDBase_SUBi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));

    t1r = SIMDBase_ADDi(SIMDBase_SUBi(s00, s20), SIMDBase_SUBi(s31, s11));
    t1i = SIMDBase_ADDi(SIMDBase_SUBi(s01, s21), SIMDBase_SUBi(s10, s30));
    
    a0 = SIMDBase_LOAD1(&tbl[p0+12]); a1 = SIMDBase_LOAD1(&tbl[p0+13]);
    a2 = SIMDBase_LOAD1(&tbl[p0+14]); a3 = SIMDBase_LOAD1(&tbl[p0+15]);

    SIMDBase_STOR(&s[i0+6], SIMDBase_ADDi(s00, s20)); SIMDBase_STOR(&s[i0+7], SIMDBase_ADDi(s01, s21));
    SIMDBase_STOR(&s[i1+6], SIMDBase_ADDi(s10, s30)); SIMDBase_STOR(&s[i1+7], SIMDBase_ADDi(s11, s31));

#ifndef SIMDBase_FMADD_AVAILABLE
    SIMDBase_STOR(&s[i2+6], SIMDBase_SUBi(SIMDBase_MULi(t0r, a0), SIMDBase_MULi(t0i, a1)));
    SIMDBase_STOR(&s[i2+7], SIMDBase_ADDi(SIMDBase_MULi(t0r, a1), SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3+6], SIMDBase_SUBi(SIMDBase_MULi(t1r, a2), SIMDBase_MULi(t1i, a3)));
    SIMDBase_STOR(&s[i3+7], SIMDBase_ADDi(SIMDBase_MULi(t1r, a3), SIMDBase_MULi(t1i, a2)));
#else
    SIMDBase_STOR(&s[i2+6], SIMDBase_FMSUBi(t0i, a1, SIMDBase_MULi(t0r, a0)));
    SIMDBase_STOR(&s[i2+7], SIMDBase_FMADDi(t0r, a1, SIMDBase_MULi(t0i, a0)));
    SIMDBase_STOR(&s[i3+6], SIMDBase_FMSUBi(t1i, a3, SIMDBase_MULi(t1r, a2)));
    SIMDBase_STOR(&s[i3+7], SIMDBase_FMADDi(t1r, a3, SIMDBase_MULi(t1i, a2)));
#endif

    //

    i0 += 8; i1 += 8; i2 += 8; i3 += 8;
    p0 += 16;
  }
}

#if 1
static void srButBackwardSubUnrolled(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  SIMDBase_REAL *tbl = p->ptTable[p->log2butlen];

  int32_t i0 = p->offset1;
  int32_t i1 = i0 + p->stride;
  int32_t i2 = i1 + p->stride;
  int32_t i3 = i2 + p->stride;
  int32_t im = i1;

  int32_t p0 = p->offset2 & (p->butlen*4-1);

  while(i0 < im) {
    SIMDBase_VECT t0r, t0i, t1r, t1i, u, v;
    SIMDBase_VECT s00, s01, s10, s11, s20, s21, s30, s31;
    SIMDBase_VECT a0, a1, a2, a3;

    //

    s20 = SIMDBase_LOAD(&s[i2+ 0]); s21 = SIMDBase_LOAD(&s[i2+ 1]);
    a0 = SIMDBase_LOAD1(&tbl[p0+ 0]); a1 = SIMDBase_LOAD1(&tbl[p0+ 1]);
#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_ADDi(SIMDBase_MULi(s20, a0), SIMDBase_MULi(s21, a1));
#else
    u = SIMDBase_FMADDi(s20, a0, SIMDBase_MULi(s21, a1));
#endif

    s30 = SIMDBase_LOAD(&s[i3+ 0]); s31 = SIMDBase_LOAD(&s[i3+ 1]);
    a2 = SIMDBase_LOAD1(&tbl[p0+ 2]); a3 = SIMDBase_LOAD1(&tbl[p0+ 3]);
#ifndef SIMDBase_FMADD_AVAILABLE
    v = SIMDBase_ADDi(SIMDBase_MULi(s30, a2), SIMDBase_MULi(s31, a3));
#else
    v = SIMDBase_FMADDi(s30, a2, SIMDBase_MULi(s31, a3));
#endif

    t0r = SIMDBase_ADDi(u, v); t1i = SIMDBase_SUBi(u, v);

#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_SUBi(SIMDBase_MULi(s31, a2), SIMDBase_MULi(s30, a3));
    v = SIMDBase_SUBi(SIMDBase_MULi(s21, a0), SIMDBase_MULi(s20, a1));
#else
    u = SIMDBase_FMSUBi(s30, a3, SIMDBase_MULi(s31, a2));
    v = SIMDBase_FMSUBi(s20, a1, SIMDBase_MULi(s21, a0));
#endif
    t0i = SIMDBase_ADDi(u, v); t1r = SIMDBase_SUBi(u, v);

    s00 = SIMDBase_LOAD(&s[i0+ 0]); s01 = SIMDBase_LOAD(&s[i0+ 1]);
    s10 = SIMDBase_LOAD(&s[i1+ 0]); s11 = SIMDBase_LOAD(&s[i1+ 1]);

    SIMDBase_STOR(&s[i2+ 0], SIMDBase_SUBi(s00, t0r)); SIMDBase_STOR(&s[i0+ 0], SIMDBase_ADDi(s00, t0r));
    SIMDBase_STOR(&s[i2+ 1], SIMDBase_SUBi(s01, t0i)); SIMDBase_STOR(&s[i0+ 1], SIMDBase_ADDi(s01, t0i));
    SIMDBase_STOR(&s[i3+ 0], SIMDBase_SUBi(s10, t1r)); SIMDBase_STOR(&s[i1+ 0], SIMDBase_ADDi(s10, t1r));
    SIMDBase_STOR(&s[i3+ 1], SIMDBase_SUBi(s11, t1i)); SIMDBase_STOR(&s[i1+ 1], SIMDBase_ADDi(s11, t1i));

    //

    s20 = SIMDBase_LOAD(&s[i2+ 2]); s21 = SIMDBase_LOAD(&s[i2+ 3]);
    a0 = SIMDBase_LOAD1(&tbl[p0+ 4]); a1 = SIMDBase_LOAD1(&tbl[p0+ 5]);
#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_ADDi(SIMDBase_MULi(s20, a0), SIMDBase_MULi(s21, a1));
#else
    u = SIMDBase_FMADDi(s20, a0, SIMDBase_MULi(s21, a1));
#endif

    s30 = SIMDBase_LOAD(&s[i3+ 2]); s31 = SIMDBase_LOAD(&s[i3+ 3]);
    a2 = SIMDBase_LOAD1(&tbl[p0+ 6]); a3 = SIMDBase_LOAD1(&tbl[p0+ 7]);
#ifndef SIMDBase_FMADD_AVAILABLE
    v = SIMDBase_ADDi(SIMDBase_MULi(s30, a2), SIMDBase_MULi(s31, a3));
#else
    v = SIMDBase_FMADDi(s30, a2, SIMDBase_MULi(s31, a3));
#endif

    t0r = SIMDBase_ADDi(u, v); t1i = SIMDBase_SUBi(u, v);

#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_SUBi(SIMDBase_MULi(s31, a2), SIMDBase_MULi(s30, a3));
    v = SIMDBase_SUBi(SIMDBase_MULi(s21, a0), SIMDBase_MULi(s20, a1));
#else
    u = SIMDBase_FMSUBi(s30, a3, SIMDBase_MULi(s31, a2));
    v = SIMDBase_FMSUBi(s20, a1, SIMDBase_MULi(s21, a0));
#endif
    t0i = SIMDBase_ADDi(u, v); t1r = SIMDBase_SUBi(u, v);

    s00 = SIMDBase_LOAD(&s[i0+ 2]); s01 = SIMDBase_LOAD(&s[i0+ 3]);
    s10 = SIMDBase_LOAD(&s[i1+ 2]); s11 = SIMDBase_LOAD(&s[i1+ 3]);

    SIMDBase_STOR(&s[i2+ 2], SIMDBase_SUBi(s00, t0r)); SIMDBase_STOR(&s[i0+ 2], SIMDBase_ADDi(s00, t0r));
    SIMDBase_STOR(&s[i2+ 3], SIMDBase_SUBi(s01, t0i)); SIMDBase_STOR(&s[i0+ 3], SIMDBase_ADDi(s01, t0i));
    SIMDBase_STOR(&s[i3+ 2], SIMDBase_SUBi(s10, t1r)); SIMDBase_STOR(&s[i1+ 2], SIMDBase_ADDi(s10, t1r));
    SIMDBase_STOR(&s[i3+ 3], SIMDBase_SUBi(s11, t1i)); SIMDBase_STOR(&s[i1+ 3], SIMDBase_ADDi(s11, t1i));

    //

    s20 = SIMDBase_LOAD(&s[i2+ 4]); s21 = SIMDBase_LOAD(&s[i2+ 5]);
    a0 = SIMDBase_LOAD1(&tbl[p0+ 8]); a1 = SIMDBase_LOAD1(&tbl[p0+ 9]);
#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_ADDi(SIMDBase_MULi(s20, a0), SIMDBase_MULi(s21, a1));
#else
    u = SIMDBase_FMADDi(s20, a0, SIMDBase_MULi(s21, a1));
#endif

    s30 = SIMDBase_LOAD(&s[i3+ 4]); s31 = SIMDBase_LOAD(&s[i3+ 5]);
    a2 = SIMDBase_LOAD1(&tbl[p0+10]); a3 = SIMDBase_LOAD1(&tbl[p0+11]);
#ifndef SIMDBase_FMADD_AVAILABLE
    v = SIMDBase_ADDi(SIMDBase_MULi(s30, a2), SIMDBase_MULi(s31, a3));
#else
    v = SIMDBase_FMADDi(s30, a2, SIMDBase_MULi(s31, a3));
#endif

    t0r = SIMDBase_ADDi(u, v); t1i = SIMDBase_SUBi(u, v);

#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_SUBi(SIMDBase_MULi(s31, a2), SIMDBase_MULi(s30, a3));
    v = SIMDBase_SUBi(SIMDBase_MULi(s21, a0), SIMDBase_MULi(s20, a1));
#else
    u = SIMDBase_FMSUBi(s30, a3, SIMDBase_MULi(s31, a2));
    v = SIMDBase_FMSUBi(s20, a1, SIMDBase_MULi(s21, a0));
#endif
    t0i = SIMDBase_ADDi(u, v); t1r = SIMDBase_SUBi(u, v);

    s00 = SIMDBase_LOAD(&s[i0+ 4]); s01 = SIMDBase_LOAD(&s[i0+ 5]);
    s10 = SIMDBase_LOAD(&s[i1+ 4]); s11 = SIMDBase_LOAD(&s[i1+ 5]);

    SIMDBase_STOR(&s[i2+ 4], SIMDBase_SUBi(s00, t0r)); SIMDBase_STOR(&s[i0+ 4], SIMDBase_ADDi(s00, t0r));
    SIMDBase_STOR(&s[i2+ 5], SIMDBase_SUBi(s01, t0i)); SIMDBase_STOR(&s[i0+ 5], SIMDBase_ADDi(s01, t0i));
    SIMDBase_STOR(&s[i3+ 4], SIMDBase_SUBi(s10, t1r)); SIMDBase_STOR(&s[i1+ 4], SIMDBase_ADDi(s10, t1r));
    SIMDBase_STOR(&s[i3+ 5], SIMDBase_SUBi(s11, t1i)); SIMDBase_STOR(&s[i1+ 5], SIMDBase_ADDi(s11, t1i));

    //

    s20 = SIMDBase_LOAD(&s[i2+ 6]); s21 = SIMDBase_LOAD(&s[i2+ 7]);
    a0 = SIMDBase_LOAD1(&tbl[p0+12]); a1 = SIMDBase_LOAD1(&tbl[p0+13]);
#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_ADDi(SIMDBase_MULi(s20, a0), SIMDBase_MULi(s21, a1));
#else
    u = SIMDBase_FMADDi(s20, a0, SIMDBase_MULi(s21, a1));
#endif

    s30 = SIMDBase_LOAD(&s[i3+ 6]); s31 = SIMDBase_LOAD(&s[i3+ 7]);
    a2 = SIMDBase_LOAD1(&tbl[p0+14]); a3 = SIMDBase_LOAD1(&tbl[p0+15]);
#ifndef SIMDBase_FMADD_AVAILABLE
    v = SIMDBase_ADDi(SIMDBase_MULi(s30, a2), SIMDBase_MULi(s31, a3));
#else
    v = SIMDBase_FMADDi(s30, a2, SIMDBase_MULi(s31, a3));
#endif

    t0r = SIMDBase_ADDi(u, v); t1i = SIMDBase_SUBi(u, v);

#ifndef SIMDBase_FMADD_AVAILABLE
    u = SIMDBase_SUBi(SIMDBase_MULi(s31, a2), SIMDBase_MULi(s30, a3));
    v = SIMDBase_SUBi(SIMDBase_MULi(s21, a0), SIMDBase_MULi(s20, a1));
#else
    u = SIMDBase_FMSUBi(s30, a3, SIMDBase_MULi(s31, a2));
    v = SIMDBase_FMSUBi(s20, a1, SIMDBase_MULi(s21, a0));
#endif
    t0i = SIMDBase_ADDi(u, v); t1r = SIMDBase_SUBi(u, v);

    s00 = SIMDBase_LOAD(&s[i0+ 6]); s01 = SIMDBase_LOAD(&s[i0+ 7]);
    s10 = SIMDBase_LOAD(&s[i1+ 6]); s11 = SIMDBase_LOAD(&s[i1+ 7]);

    SIMDBase_STOR(&s[i2+ 6], SIMDBase_SUBi(s00, t0r)); SIMDBase_STOR(&s[i0+ 6], SIMDBase_ADDi(s00, t0r));
    SIMDBase_STOR(&s[i2+ 7], SIMDBase_SUBi(s01, t0i)); SIMDBase_STOR(&s[i0+ 7], SIMDBase_ADDi(s01, t0i));
    SIMDBase_STOR(&s[i3+ 6], SIMDBase_SUBi(s10, t1r)); SIMDBase_STOR(&s[i1+ 6], SIMDBase_ADDi(s10, t1r));
    SIMDBase_STOR(&s[i3+ 7], SIMDBase_SUBi(s11, t1i)); SIMDBase_STOR(&s[i1+ 7], SIMDBase_ADDi(s11, t1i));

    //

    i0 += 8; i1 += 8; i2 += 8; i3 += 8;
    p0 += 16;
  }
}
#endif

static void r2ButForwardSub(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;

  SIMDBase_REAL *tbl = p->ptTable[p->log2butlen];

  int32_t i0 = p->offset1;
  int32_t i2 = i0 + p->stride*2;
  int32_t cp = 0, sp = p->butlen/4;

  do {
    SIMDBase_VECT t0r, t0i, s0, s1, s2, s3, t0, t1;

    s0 = SIMDBase_LOAD(&s[i0+0]); s2 = SIMDBase_LOAD(&s[i0+1]);
    s1 = SIMDBase_LOAD(&s[i2+0]); s3 = SIMDBase_LOAD(&s[i2+1]);
    t0 = SIMDBase_LOAD1(&tbl[cp+0]); t1 = SIMDBase_LOAD1(&tbl[sp-0]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+0], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+1], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+0], SIMDBase_ADDi(SIMDBase_MULi(t0r, t0), SIMDBase_MULi(t0i, t1)));
    SIMDBase_STOR(&s[i2+1], SIMDBase_SUBi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1)));

    s0 = SIMDBase_LOAD(&s[i0+2]); s2 = SIMDBase_LOAD(&s[i0+3]);
    s1 = SIMDBase_LOAD(&s[i2+2]); s3 = SIMDBase_LOAD(&s[i2+3]);
    t0 = SIMDBase_LOAD1(&tbl[cp+1]); t1 = SIMDBase_LOAD1(&tbl[sp-1]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+2], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+3], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+2], SIMDBase_ADDi(SIMDBase_MULi(t0r, t0), SIMDBase_MULi(t0i, t1)));
    SIMDBase_STOR(&s[i2+3], SIMDBase_SUBi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1)));

    s0 = SIMDBase_LOAD(&s[i0+4]); s2 = SIMDBase_LOAD(&s[i0+5]);
    s1 = SIMDBase_LOAD(&s[i2+4]); s3 = SIMDBase_LOAD(&s[i2+5]);
    t0 = SIMDBase_LOAD1(&tbl[cp+2]); t1 = SIMDBase_LOAD1(&tbl[sp-2]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+4], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+5], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+4], SIMDBase_ADDi(SIMDBase_MULi(t0r, t0), SIMDBase_MULi(t0i, t1)));
    SIMDBase_STOR(&s[i2+5], SIMDBase_SUBi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1)));

    s0 = SIMDBase_LOAD(&s[i0+6]); s2 = SIMDBase_LOAD(&s[i0+7]);
    s1 = SIMDBase_LOAD(&s[i2+6]); s3 = SIMDBase_LOAD(&s[i2+7]);
    t0 = SIMDBase_LOAD1(&tbl[cp+3]); t1 = SIMDBase_LOAD1(&tbl[sp-3]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+6], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+7], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+6], SIMDBase_ADDi(SIMDBase_MULi(t0r, t0), SIMDBase_MULi(t0i, t1)));
    SIMDBase_STOR(&s[i2+7], SIMDBase_SUBi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1)));

    //

    i0 += 8; i2 += 8; cp += 4; sp -= 4;
  } while(sp > 0);

  do {
    SIMDBase_VECT t0r, t0i, s0, s1, s2, s3, t0, t1;

    s0 = SIMDBase_LOAD(&s[i0+0]); s2 = SIMDBase_LOAD(&s[i0+1]);
    s1 = SIMDBase_LOAD(&s[i2+0]); s3 = SIMDBase_LOAD(&s[i2+1]);
    t0 = SIMDBase_LOAD1(&tbl[cp-0]); t1 = SIMDBase_LOAD1(&tbl[sp+0]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+0], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+1], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+0], SIMDBase_SUBi(SIMDBase_MULi(t0i, t1), SIMDBase_MULi(t0r, t0)));
    SIMDBase_STOR(&s[i2+1], SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1))));

    s0 = SIMDBase_LOAD(&s[i0+2]); s2 = SIMDBase_LOAD(&s[i0+3]);
    s1 = SIMDBase_LOAD(&s[i2+2]); s3 = SIMDBase_LOAD(&s[i2+3]);
    t0 = SIMDBase_LOAD1(&tbl[cp-1]); t1 = SIMDBase_LOAD1(&tbl[sp+1]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+2], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+3], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+2], SIMDBase_SUBi(SIMDBase_MULi(t0i, t1), SIMDBase_MULi(t0r, t0)));
    SIMDBase_STOR(&s[i2+3], SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1))));

    s0 = SIMDBase_LOAD(&s[i0+4]); s2 = SIMDBase_LOAD(&s[i0+5]);
    s1 = SIMDBase_LOAD(&s[i2+4]); s3 = SIMDBase_LOAD(&s[i2+5]);
    t0 = SIMDBase_LOAD1(&tbl[cp-2]); t1 = SIMDBase_LOAD1(&tbl[sp+2]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+4], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+5], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+4], SIMDBase_SUBi(SIMDBase_MULi(t0i, t1), SIMDBase_MULi(t0r, t0)));
    SIMDBase_STOR(&s[i2+5], SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1))));

    s0 = SIMDBase_LOAD(&s[i0+6]); s2 = SIMDBase_LOAD(&s[i0+7]);
    s1 = SIMDBase_LOAD(&s[i2+6]); s3 = SIMDBase_LOAD(&s[i2+7]);
    t0 = SIMDBase_LOAD1(&tbl[cp-3]); t1 = SIMDBase_LOAD1(&tbl[sp+3]);
    t0r = SIMDBase_SUBi(s0, s1); SIMDBase_STOR(&s[i0+6], SIMDBase_ADDi(s0, s1));
    t0i = SIMDBase_SUBi(s2, s3); SIMDBase_STOR(&s[i0+7], SIMDBase_ADDi(s2, s3));
    SIMDBase_STOR(&s[i2+6], SIMDBase_SUBi(SIMDBase_MULi(t0i, t1), SIMDBase_MULi(t0r, t0)));
    SIMDBase_STOR(&s[i2+7], SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(t0i, t0), SIMDBase_MULi(t0r, t1))));

    //

    i0 += 8; i2 += 8; cp -= 4; sp += 4;
  } while(cp > 0);
}

static void r2ButBackwardSub(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;

  SIMDBase_REAL *tbl = p->ptTable[p->log2butlen];

  int i0 = p->offset1;
  int i2 = i0 + p->stride*2;

  int cp = 0, sp = p->butlen/4;

  do {
    SIMDBase_VECT t0r, t0i, s0, s1, s2, s3, t0, t1;

    s0 = SIMDBase_LOAD(&s[i0+0]); s2 = SIMDBase_LOAD(&s[i0+1]);
    s1 = SIMDBase_LOAD(&s[i2+0]); s3 = SIMDBase_LOAD(&s[i2+1]);
    t0 = SIMDBase_LOAD1(&tbl[cp+0]); t1 = SIMDBase_LOAD1(&tbl[sp-0]);
    t0r = SIMDBase_SUBi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1));
    t0i = SIMDBase_ADDi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+0], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+0], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+1], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+1], SIMDBase_ADDi(s2, t0i));

    s0 = SIMDBase_LOAD(&s[i0+2]); s2 = SIMDBase_LOAD(&s[i0+3]);
    s1 = SIMDBase_LOAD(&s[i2+2]); s3 = SIMDBase_LOAD(&s[i2+3]);
    t0 = SIMDBase_LOAD1(&tbl[cp+1]); t1 = SIMDBase_LOAD1(&tbl[sp-1]);
    t0r = SIMDBase_SUBi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1));
    t0i = SIMDBase_ADDi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+2], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+2], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+3], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+3], SIMDBase_ADDi(s2, t0i));

    s0 = SIMDBase_LOAD(&s[i0+4]); s2 = SIMDBase_LOAD(&s[i0+5]);
    s1 = SIMDBase_LOAD(&s[i2+4]); s3 = SIMDBase_LOAD(&s[i2+5]);
    t0 = SIMDBase_LOAD1(&tbl[cp+2]); t1 = SIMDBase_LOAD1(&tbl[sp-2]);
    t0r = SIMDBase_SUBi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1));
    t0i = SIMDBase_ADDi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+4], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+4], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+5], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+5], SIMDBase_ADDi(s2, t0i));

    s0 = SIMDBase_LOAD(&s[i0+6]); s2 = SIMDBase_LOAD(&s[i0+7]);
    s1 = SIMDBase_LOAD(&s[i2+6]); s3 = SIMDBase_LOAD(&s[i2+7]);
    t0 = SIMDBase_LOAD1(&tbl[cp+3]); t1 = SIMDBase_LOAD1(&tbl[sp-3]);
    t0r = SIMDBase_SUBi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1));
    t0i = SIMDBase_ADDi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+6], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+6], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+7], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+7], SIMDBase_ADDi(s2, t0i));

    i0 += 8; i2 += 8; cp += 4; sp -= 4;
  } while(sp > 0);

  do {
    SIMDBase_VECT t0r, t0i, s0, s1, s2, s3, t0, t1;

    s0 = SIMDBase_LOAD(&s[i0+0]); s2 = SIMDBase_LOAD(&s[i0+1]);
    s1 = SIMDBase_LOAD(&s[i2+0]); s3 = SIMDBase_LOAD(&s[i2+1]);
    t0 = SIMDBase_LOAD1(&tbl[cp-0]); t1 = SIMDBase_LOAD1(&tbl[sp+0]);
    t0r = SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1)));
    t0i = SIMDBase_SUBi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+0], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+0], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+1], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+1], SIMDBase_ADDi(s2, t0i));

    s0 = SIMDBase_LOAD(&s[i0+2]); s2 = SIMDBase_LOAD(&s[i0+3]);
    s1 = SIMDBase_LOAD(&s[i2+2]); s3 = SIMDBase_LOAD(&s[i2+3]);
    t0 = SIMDBase_LOAD1(&tbl[cp-1]); t1 = SIMDBase_LOAD1(&tbl[sp+1]);
    t0r = SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1)));
    t0i = SIMDBase_SUBi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+2], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+2], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+3], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+3], SIMDBase_ADDi(s2, t0i));

    s0 = SIMDBase_LOAD(&s[i0+4]); s2 = SIMDBase_LOAD(&s[i0+5]);
    s1 = SIMDBase_LOAD(&s[i2+4]); s3 = SIMDBase_LOAD(&s[i2+5]);
    t0 = SIMDBase_LOAD1(&tbl[cp-2]); t1 = SIMDBase_LOAD1(&tbl[sp+2]);
    t0r = SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1)));
    t0i = SIMDBase_SUBi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+4], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+4], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+5], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+5], SIMDBase_ADDi(s2, t0i));

    s0 = SIMDBase_LOAD(&s[i0+6]); s2 = SIMDBase_LOAD(&s[i0+7]);
    s1 = SIMDBase_LOAD(&s[i2+6]); s3 = SIMDBase_LOAD(&s[i2+7]);
    t0 = SIMDBase_LOAD1(&tbl[cp-3]); t1 = SIMDBase_LOAD1(&tbl[sp+3]);
    t0r = SIMDBase_NEGi(SIMDBase_ADDi(SIMDBase_MULi(s1, t0), SIMDBase_MULi(s3, t1)));
    t0i = SIMDBase_SUBi(SIMDBase_MULi(s1, t1), SIMDBase_MULi(s3, t0));
    SIMDBase_STOR(&s[i2+6], SIMDBase_SUBi(s0, t0r)); SIMDBase_STOR(&s[i0+6], SIMDBase_ADDi(s0, t0r));
    SIMDBase_STOR(&s[i2+7], SIMDBase_SUBi(s2, t0i)); SIMDBase_STOR(&s[i0+7], SIMDBase_ADDi(s2, t0i));

    i0 += 8; i2 += 8; cp -= 4; sp += 4;
  } while(cp > 0);
}

static void srButForward16(DFTUndiff *p) {
  int32_t o = p->offset1;

  p->butlen = 16; p->log2butlen = 4; p->stride = p->butlen/2;
  srButForwardSubUnrolled(p);

  p->offset1 = o + 16*6/4;
  srButForward4(p);

  p->offset1 = o + 16*4/4;
  srButForward4(p);

  p->offset1 = o;
  srButForward8(p);
}

static void srButBackward16(DFTUndiff *p) {
  int32_t o = p->offset1;

  p->offset1 = o + 16*6/4;
  srButBackward4(p);

  p->offset1 = o + 16*4/4;
  srButBackward4(p);

  p->offset1 = o;
  srButBackward8(p);

  p->butlen = 16; p->log2butlen = 4; p->stride = p->butlen/2;
  srButBackwardSubUnrolled(p);
}

static void srButForward32(DFTUndiff *p) {
  int32_t o = p->offset1;

  p->butlen = 32; p->log2butlen = 5; p->stride = p->butlen/2;
  srButForwardSubUnrolled(p);

  p->offset1 = o + 32*6/4;
  srButForward8 (p);

  p->offset1 = o + 32*4/4;
  srButForward8 (p);

  p->offset1 = o;
  srButForward16(p);
}

static void srButBackward32(DFTUndiff *p) {
  int32_t o = p->offset1;

  p->offset1 = o + 32*6/4;
  srButBackward8 (p);

  p->offset1 = o + 32*4/4;
  srButBackward8 (p);

  p->offset1 = o;
  srButBackward16(p);

  p->butlen = 32; p->log2butlen = 5; p->stride = p->butlen/2;
  srButBackwardSubUnrolled(p);
}

//

#if 1
static inline void bitReversalUnit(SIMDBase_VECT *p, SIMDBase_VECT *q) {
  SIMDBase_VECT w, x, y, z;

  w = SIMDBase_LOAD(p); x = SIMDBase_LOAD(p+1);
  y = SIMDBase_LOAD(q); z = SIMDBase_LOAD(q+1);

  SIMDBase_STOR(q, w); SIMDBase_STOR(q+1, x);
  SIMDBase_STOR(p, y); SIMDBase_STOR(p+1, z);
}
#else
#define bitReversalUnit(p0, q0) {                    \
  SIMDBase_VECT *px = (p0), *qx = (q0);              \
  SIMDBase_VECT wx, xx, yx, zx;                      \
                                                     \
  wx = SIMDBase_LOAD(px); xx = SIMDBase_LOAD(px+1);  \
  yx = SIMDBase_LOAD(qx); zx = SIMDBase_LOAD(qx+1);  \
                                                     \
  SIMDBase_STOR(qx, wx); SIMDBase_STOR(qx+1, xx);    \
  SIMDBase_STOR(px, yx); SIMDBase_STOR(px+1, zx);    \
}
#endif

static inline void bitReversal4s(SIMDBase_VECT *s, int32_t sc, int32_t o1, int32_t o2) {
  SIMDBase_VECT *p = &s[o1*2], *q = &s[o2*2];
  int b1 = sc*2*1, b2 = b1*2;
  p += b1; q += b2;
  bitReversalUnit(p, q);
}

static inline void bitReversal8s(SIMDBase_VECT *s, int32_t sc, int32_t o1, int32_t o2) {
  SIMDBase_VECT *p = &s[o1*2], *q = &s[o2*2];
  int b1 = sc*2*1, b2 = b1*2, b4 = b2*2;
  p += b1; q += b4;
  bitReversalUnit(p, q); p += b2; q += b2;
  bitReversalUnit(p, q);
}

static inline void bitReversal8d(SIMDBase_VECT *s, int32_t sc, int32_t o1, int32_t o2) {
  SIMDBase_VECT *p = &s[o1*2], *q = &s[o2*2];
  int32_t b1 = sc*2*1, b2 = b1*2, b4 = b2*2;
  bitReversalUnit(p, q); p += b1; q += b4;
  bitReversalUnit(p, q); p += b2; q += b2;
  bitReversalUnit(p, q); p -= b1; q -= b4;
  bitReversalUnit(p, q); p += b4; q += b1;
  bitReversalUnit(p, q); p += b1; q += b4;
  bitReversalUnit(p, q); p -= b2; q -= b2;
  bitReversalUnit(p, q); p -= b1; q -= b4;
  bitReversalUnit(p, q);
}

static inline void bitReversal16s(SIMDBase_VECT *s, int32_t sc, int32_t o1, int32_t o2) {
  SIMDBase_VECT *p = &s[o1*2], *q = &s[o2*2];
  int32_t b1 = sc*2*1, b2 = b1*2, b4 = b2*2, b8 = b4*2;
  p += b1; q += b8;
  bitReversalUnit(p, q); p += b2; q += b4;
  bitReversalUnit(p, q); p -= b1; q -= b8;
  bitReversalUnit(p, q); p += b1 + b4; q += b2 + b8;
  bitReversalUnit(p, q); p -= b2; q -= b4;
  bitReversalUnit(p, q); p += b2 + b4; q += b1 + b2;
  bitReversalUnit(p, q);
}

static inline void bitReversal16d(SIMDBase_VECT *s, int32_t sc, int32_t o1, int32_t o2) {
  SIMDBase_VECT *p = &s[o1*2], *q = &s[o2*2];
  int32_t b1 = sc*2*1, b2 = b1*2, b4 = b2*2, b8 = b4*2;
  bitReversalUnit(p, q); p += b1; q += b8;
  bitReversalUnit(p, q); p += b2; q += b4;
  bitReversalUnit(p, q); p -= b1; q -= b8;
  bitReversalUnit(p, q); p += b4; q += b2;
  bitReversalUnit(p, q); p += b1; q += b8;
  bitReversalUnit(p, q); p -= b2; q -= b4;
  bitReversalUnit(p, q); p -= b1; q -= b8;
  bitReversalUnit(p, q); p += b8; q += b1;
  bitReversalUnit(p, q); p += b1; q += b8;
  bitReversalUnit(p, q); p += b2; q += b4;
  bitReversalUnit(p, q); p -= b1; q -= b8;
  bitReversalUnit(p, q); p -= b4; q -= b2;
  bitReversalUnit(p, q); p += b1; q += b8;
  bitReversalUnit(p, q); p -= b2; q -= b4;
  bitReversalUnit(p, q); p -= b1; q -= b8;
  bitReversalUnit(p, q);
}

static inline void bitReversal32s(SIMDBase_VECT *s, int32_t sc, int32_t o1, int32_t o2) {
  SIMDBase_VECT *p = &s[o1*2], *q = &s[o2*2];
  int32_t b1 = sc*2*1, b2 = b1*2, b4 = b2*2, b8 = b4*2, b16 = b8*2;
  p += b1; q += b16;
  bitReversalUnit(p, q); p += b2; q += b8;
  bitReversalUnit(p, q); p -= b1; q -= b16;
  bitReversalUnit(p, q); p += b4; q += b4;
  bitReversalUnit(p, q); p += b1; q += b16;
  bitReversalUnit(p, q); p -= b2; q -= b8;
  bitReversalUnit(p, q); p += b8; q += b2;
  bitReversalUnit(p, q); p += b2; q += b8;
  bitReversalUnit(p, q); p -= b4; q -= b4;
  bitReversalUnit(p, q); p -= b2; q -= b8;
  bitReversalUnit(p, q); p += b16 - b2; q += b1 + b2 + b8;
  bitReversalUnit(p, q); p -= b4; q -= b4;
  bitReversalUnit(p, q);
}

static void bitReversal32d(SIMDBase_VECT *s, int32_t sc, int32_t o1, int32_t o2) {
  const int32_t k = 32;

  bitReversal8d(s,2*sc, sc*(k/2  )+o1, sc*     1 +o2);
  bitReversal8d(s,2*sc, sc*     0 +o1, sc*     0 +o2);
  bitReversal8d(s,2*sc, sc*     1 +o1, sc*(k/2  )+o2);
  bitReversal8d(s,2*sc, sc*(k/2+1)+o1, sc*(k/2+1)+o2);
}

static void bitReversalRecursive(SIMDBase_VECT *s, int32_t n, int32_t sc, int32_t o1, int32_t o2) {
  if (n >= 64) {
    if (o1 != o2) bitReversalRecursive(s, n/4, 2*sc, sc*(n/2)+o1, sc*1+o2);

    bitReversalRecursive(s, n/4, 2*sc, sc*     0 +o1, sc*     0 +o2);
    bitReversalRecursive(s, n/4, 2*sc, sc*     1 +o1, sc*(n/2  )+o2);
    bitReversalRecursive(s, n/4, 2*sc, sc*(n/2+1)+o1, sc*(n/2+1)+o2);
  } else {
    if (o1 == o2) {
      switch(n) {
      case  4: bitReversal4s (s,sc,o1,o2); return;
      case  8: bitReversal8s (s,sc,o1,o2); return;
      case 16: bitReversal16s(s,sc,o1,o2); return;
      case 32: bitReversal32s(s,sc,o1,o2); return;
      }
    } else {
      switch(n) {
      case  8: bitReversal8d (s,sc,o1,o2); return;
      case 16: bitReversal16d(s,sc,o1,o2); return;
      case 32: bitReversal32d(s,sc,o1,o2); return;
      }
    }
  }
}

//

static int bitR(int a, int logN) {
  int ret = 0;
  int i,j,k;
  for(i=0,j=1,k=1<<(logN-1);i<logN;i++,j=j<<1,k=k>>1) {
    if ((a & j) != 0) ret |= k;
  }
  return ret;
}

static void bitReversalCobraInplace(DFTUndiff *p) {
  SIMDBase_VECT *s = p->s;
  int cobraQ = p->cobraQ;
  SIMDBase_VECT *cobraT = p->cobraT;
  int *cobraR = p->cobraR;
  int logN = p->log2len;

  int b;

  for(b=0;b<(1 << (logN-2*cobraQ));b++) {
    int a,c;
    int b2 = bitR(b, logN-2*cobraQ);

    if (b2 < b) continue;

    if (b2 == b) {
      for(a=0;a<(1 << cobraQ);a++) {
	int abc = ((a << (logN-2*cobraQ)) | b) << (cobraQ + 1);

	int a2c = (cobraR[a] << cobraQ) << 1, a2cm = a2c+(1 << cobraQ)*2;

	while(a2c < a2cm) {
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	}
      }

      for(c=0;c<(1 << cobraQ);c++) {
	int c2 = cobraR[c];
	int c2b2a2 = ((c2 << (logN-2*cobraQ)) | b2) << (cobraQ+1);

	int a2c = c << 1;
	int a2ci = 1 << (cobraQ+1);
	int c2b2a2m = c2b2a2 + (1 << cobraQ)*2;

	while(c2b2a2 < c2b2a2m) {
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); a2c += a2ci;
	}
      }
    } else {
      for(a=0;a<(1 << cobraQ);a++) {
	int a2c = (cobraR[a] << cobraQ) << 1, a2cm = a2c+(1 << cobraQ)*2;
	int abc = ((a << (logN-2*cobraQ)) | b) << (cobraQ + 1);

	while(a2c < a2cm) {
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	  SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++])); SIMDBase_STOR(&cobraT[a2c++], SIMDBase_LOAD(&s[abc++]));
	}
      }

      for(c=0;c<(1 << cobraQ);c++) {
	int c2 = cobraR[c];
	int c2b2a2 = ((c2 << (logN-2*cobraQ)) | b2) << (cobraQ+1);

	int a2c = c << 1;
	int a2ci = 1 << (cobraQ+1);
	int c2b2a2m = c2b2a2 + (1 << cobraQ)*2;

	while(c2b2a2 < c2b2a2m) {
	  SIMDBase_VECT t0, t1, t2, t3, t4, t5, t6, t7;

	  t0 = SIMDBase_LOAD(&s[c2b2a2+0]); t1 = SIMDBase_LOAD(&s[c2b2a2+1]);
	  t2 = SIMDBase_LOAD(&s[c2b2a2+2]); t3 = SIMDBase_LOAD(&s[c2b2a2+3]);
	  t4 = SIMDBase_LOAD(&s[c2b2a2+4]); t5 = SIMDBase_LOAD(&s[c2b2a2+5]);
	  t6 = SIMDBase_LOAD(&s[c2b2a2+6]); t7 = SIMDBase_LOAD(&s[c2b2a2+7]);

	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t0);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t1); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t2);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t3); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t4);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t5); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t6);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t7); a2c += a2ci;

	  t0 = SIMDBase_LOAD(&s[c2b2a2+0]); t1 = SIMDBase_LOAD(&s[c2b2a2+1]);
	  t2 = SIMDBase_LOAD(&s[c2b2a2+2]); t3 = SIMDBase_LOAD(&s[c2b2a2+3]);
	  t4 = SIMDBase_LOAD(&s[c2b2a2+4]); t5 = SIMDBase_LOAD(&s[c2b2a2+5]);
	  t6 = SIMDBase_LOAD(&s[c2b2a2+6]); t7 = SIMDBase_LOAD(&s[c2b2a2+7]);

	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t0);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t1); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t2);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t3); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t4);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t5); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t6);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t7); a2c += a2ci;

	  t0 = SIMDBase_LOAD(&s[c2b2a2+0]); t1 = SIMDBase_LOAD(&s[c2b2a2+1]);
	  t2 = SIMDBase_LOAD(&s[c2b2a2+2]); t3 = SIMDBase_LOAD(&s[c2b2a2+3]);
	  t4 = SIMDBase_LOAD(&s[c2b2a2+4]); t5 = SIMDBase_LOAD(&s[c2b2a2+5]);
	  t6 = SIMDBase_LOAD(&s[c2b2a2+6]); t7 = SIMDBase_LOAD(&s[c2b2a2+7]);

	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t0);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t1); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t2);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t3); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t4);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t5); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t6);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t7); a2c += a2ci;

	  t0 = SIMDBase_LOAD(&s[c2b2a2+0]); t1 = SIMDBase_LOAD(&s[c2b2a2+1]);
	  t2 = SIMDBase_LOAD(&s[c2b2a2+2]); t3 = SIMDBase_LOAD(&s[c2b2a2+3]);
	  t4 = SIMDBase_LOAD(&s[c2b2a2+4]); t5 = SIMDBase_LOAD(&s[c2b2a2+5]);
	  t6 = SIMDBase_LOAD(&s[c2b2a2+6]); t7 = SIMDBase_LOAD(&s[c2b2a2+7]);

	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t0);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t1); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t2);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t3); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t4);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t5); a2c += a2ci;
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c  ])); SIMDBase_STOR(&cobraT[a2c  ], t6);
	  SIMDBase_STOR(&s[c2b2a2++], SIMDBase_LOAD(&cobraT[a2c+1])); SIMDBase_STOR(&cobraT[a2c+1], t7); a2c += a2ci;
	}
      }

      for(a=0;a<(1 << cobraQ);a++) {
	int a2c = (cobraR[a] << cobraQ) << 1, a2cm = a2c+(1 << cobraQ)*2;
	int abc = ((a << (logN-2*cobraQ)) | b) << (cobraQ + 1);

	while(a2c < a2cm) {
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	  SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++])); SIMDBase_STOR(&s[abc++], SIMDBase_LOAD(&cobraT[a2c++]));
	}
      }
    }
  }
}

//

static void srForwardMain2(DFTUndiff *p) {
  int32_t o = p->offset1;
  int32_t butlen = p->butlen;
  int32_t log2butlen = p->log2butlen;

  if (butlen >= p->radix2thres) {
    p->stride           = p->butlen/2;
    r2ButForwardSub(p);

    p->offset1          = o + butlen*4/4;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srForwardMain2(p);

    p->offset1          = o;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srForwardMain2(p);

    return;
  }

  if (butlen >= 256) {
    p->stride           = p->butlen/2;
    srButForwardSubUnrolled(p);

    p->offset1          = o + butlen*6/4;
    p->butlen           = butlen/4;
    p->log2butlen       = log2butlen-2;
    srForwardMain2(p);

    p->offset1          = o + butlen*4/4;
    p->butlen           = butlen/4;
    p->log2butlen       = log2butlen-2;
    srForwardMain2(p);

    p->offset1          = o;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srForwardMain2(p);

    return;
  }

  if (butlen == 128) {
    p->stride           = p->butlen/2;
    srButForwardSubUnrolled(p);

    p->offset1 = o + butlen*6/4;
    srButForward32(p);

    p->offset1 = o + butlen*4/4;
    srButForward32(p);

    p->offset1          = o;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srForwardMain2 (p);

    return;
  }

  // butlen == 64

  p->stride = p->butlen/2;
  srButForwardSubUnrolled(p);

  p->offset1 = o + butlen*6/4;
  srButForward16(p);

  p->offset1 = o + butlen*4/4;
  srButForward16(p);

  p->offset1 = o;
  srButForward32(p);
}

static void srBackwardMain2(DFTUndiff *p) {
  int32_t o = p->offset1;
  int32_t butlen = p->butlen;
  int32_t log2butlen = p->log2butlen;

  if (butlen >= p->radix2thres) {
    p->offset1          = o + butlen*4/4;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srBackwardMain2(p);

    p->offset1          = o;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srBackwardMain2(p);

    p->butlen           = butlen;
    p->stride           = p->butlen/2;
    p->log2butlen       = log2butlen;
    r2ButBackwardSub(p);

    return;
  }

  if (butlen >= 256) {
    p->offset1          = o + butlen*6/4;
    p->butlen           = butlen/4;
    p->log2butlen       = log2butlen-2;
    srBackwardMain2(p);

    p->offset1          = o + butlen*4/4;
    p->butlen           = butlen/4;
    p->log2butlen       = log2butlen-2;
    srBackwardMain2(p);

    p->offset1          = o;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srBackwardMain2(p);

    p->butlen           = butlen;
    p->stride           = p->butlen/2;
    p->log2butlen       = log2butlen;
    srButBackwardSubUnrolled(p);

    return;
  }

  if (butlen == 128) {
    p->offset1 = o + butlen*6/4;
    srButBackward32(p);

    p->offset1 = o + butlen*4/4;
    srButBackward32(p);

    p->offset1          = o;
    p->butlen           = butlen/2;
    p->log2butlen       = log2butlen-1;
    srBackwardMain2 (p);

    p->butlen           = butlen;
    p->stride           = p->butlen/2;
    p->log2butlen       = log2butlen;
    srButBackwardSubUnrolled(p);

    return;
  }

  // butlen == 64

  p->offset1 = o + butlen*6/4;
  srButBackward16(p);

  p->offset1 = o + butlen*4/4;
  srButBackward16(p);

  p->offset1 = o;
  srButBackward32(p);

  p->butlen           = butlen;
  p->stride           = p->butlen/2;
  p->log2butlen       = log2butlen;
  srButBackwardSubUnrolled(p);
}

static void srForwardMain(DFTUndiff *p) {
  if (p->length >= 64) {
    p->butlen = p->length;
    p->log2butlen = p->log2len;
    p->offset1 = p->offset2 = 0;

    srForwardMain2(p);
  } else {
    switch(p->length) {
    case 32:
      srButForward32(p);
      break;
    case 16:
      srButForward16(p);
      break;
    case 8:
      srButForward8(p);
      break;
    case 4:
      srButForward4(p);
      break;
    case 2:
      srBut2(p);
      break;
    }
  }
}

static void srBackwardMain(DFTUndiff *p) {
  if (p->length >= 64) {
    p->butlen = p->length;
    p->log2butlen = p->log2len;
    p->offset1 = p->offset2 = 0;

    srBackwardMain2(p);
  } else {
    switch(p->length) {
    case 32:
      srButBackward32(p);
      break;
    case 16:
      srButBackward16(p);
      break;
    case 8:
      srButBackward8(p);
      break;
    case 4:
      srButBackward4(p);
      break;
    case 2:
      srBut2(p);
      break;
    }
  }
}

static void realSub0(DFTUndiff *p, SIMDBase_VECT *s, int32_t ts) {
  SIMDBase_VECT tr, ti, ur, ui, mr, mi;
  int32_t n = p->length*2;
  int32_t k;

  for(k=1;k<n/4;k++) {
    SIMDBase_VECT s00 = SIMDBase_LOAD(&s[k*2+0]), s01 = SIMDBase_LOAD(&s[k*2+1]);
    SIMDBase_VECT s10 = SIMDBase_LOAD(&s[(n/2-k)*2+0]), s11 = SIMDBase_LOAD(&s[(n/2-k)*2+1]);

    tr = SIMDBase_SUBi(s00, s10); ti = SIMDBase_ADDi(s01, s11);
    ur = SIMDBase_LOAD1(&(p->rtTable[ts][k*2+0]));
    ui = SIMDBase_LOAD1(&(p->rtTable[ts][k*2+1]));
    mr = SIMDBase_SUBi(SIMDBase_MULi(tr, ur), SIMDBase_MULi(ti, ui));
    mi = SIMDBase_ADDi(SIMDBase_MULi(tr, ui), SIMDBase_MULi(ti, ur));
    SIMDBase_STOR(&s[k*2+0], SIMDBase_SUBi(s00, mr));
    SIMDBase_STOR(&s[k*2+1], SIMDBase_SUBi(s01, mi));
    SIMDBase_STOR(&s[(n/2-k)*2+0], SIMDBase_ADDi(s10, mr));
    SIMDBase_STOR(&s[(n/2-k)*2+1], SIMDBase_SUBi(s11, mi));
  }

  tr = SIMDBase_LOAD(&s[0]); ti = SIMDBase_LOAD(&s[1]);
  SIMDBase_STOR(&s[0], SIMDBase_ADDi(tr, ti));
  SIMDBase_STOR(&s[1], SIMDBase_SUBi(tr, ti));
}

static void realSub1(DFTUndiff *p, SIMDBase_VECT *s, int32_t ts) {
  SIMDBase_VECT tr, ti, ur, ui, mr, mi;
  int32_t n = p->length*2;
  int32_t k;

  tr = SIMDBase_LOAD(&s[0]); ti = SIMDBase_LOAD(&s[1]);
  SIMDBase_STOR(&s[0], SIMDBase_MULi(SIMDBase_ADDi(tr, ti), SIMDBase_SET1(0.5)));
  SIMDBase_STOR(&s[1], SIMDBase_MULi(SIMDBase_SUBi(tr, ti), SIMDBase_SET1(0.5)));

  for(k=1;k<n/4;k++) {
    SIMDBase_VECT s00 = SIMDBase_LOAD(&s[k*2+0]), s01 = SIMDBase_LOAD(&s[k*2+1]);
    SIMDBase_VECT s10 = SIMDBase_LOAD(&s[(n/2-k)*2+0]), s11 = SIMDBase_LOAD(&s[(n/2-k)*2+1]);

    tr = SIMDBase_SUBi(s00, s10); ti = SIMDBase_ADDi(s01, s11);
    ur = SIMDBase_LOAD1(&(p->rtTable[ts][k*2+0]));
    ui = SIMDBase_LOAD1(&(p->rtTable[ts][k*2+1]));
    mr = SIMDBase_SUBi(SIMDBase_MULi(tr, ur), SIMDBase_MULi(ti, ui));
    mi = SIMDBase_ADDi(SIMDBase_MULi(tr, ui), SIMDBase_MULi(ti, ur));
    tr = SIMDBase_SUBi(s00, mr); ti = SIMDBase_SUBi(mi, s01);
    SIMDBase_STOR(&s[k*2+0], SIMDBase_ADDi(mr, s10));
    SIMDBase_STOR(&s[k*2+1], SIMDBase_SUBi(mi, s11));
    SIMDBase_STOR(&s[(n/2-k)*2+0], tr);
    SIMDBase_STOR(&s[(n/2-k)*2+1], ti);
  }
}

void DFTUndiff_EXECUTE(void *p2, void *s2, int32_t dir) {
  DFTUndiff *p = (DFTUndiff *)p2;
  SIMDBase_VECT *s = (SIMDBase_VECT *)s2;

  if (p->magic != MAGIC_DFT) abort();

  p->s = s;

  if (dir == -1) {
    if ((p->flags & DFT_FLAG_ALT_REAL) != 0) {
      realSub1(p, s, 0);
    }

    srForwardMain(p);

    if ((p->flags & DFT_FLAG_NO_BITREVERSAL) == 0) {
      if (p->useCobra) {
	bitReversalCobraInplace(p);
      } else {
	bitReversalRecursive(p->s, p->length, 1, 0, 0);
      }
    }

    if ((p->flags & DFT_FLAG_REAL) != 0) {
      realSub0(p, s, 0);
      s[p->length+1] = SIMDBase_NEGi(s[p->length+1]);
    }
  } else {
    if ((p->flags & DFT_FLAG_REAL) != 0) {
      s[p->length+1] = SIMDBase_NEGi(s[p->length+1]);
      realSub1(p, s, 1);
    }

    if ((p->flags & DFT_FLAG_NO_BITREVERSAL) == 0) {
      if (p->useCobra) {
	bitReversalCobraInplace(p);
      } else {
	bitReversalRecursive(p->s, p->length, 1, 0, 0);
      }
    }

    srBackwardMain(p);

    if ((p->flags & DFT_FLAG_ALT_REAL) != 0) {
      realSub0(p, s, 1);
    }
  }
}

void DFTUndiff_DESTROYPLAN(void *p2) {
  DFTUndiff *plan = (DFTUndiff *)p2;
  if (plan->magic != MAGIC_DFT) abort();

  free(*(plan->ptTable));
  free(plan->ptTable);
  free(plan->cobraT);
  free(plan->cobraR);
  //free(plan->t);
  if (plan->rtTable != NULL) {
    free(plan->rtTable[0]);
    free(plan->rtTable[1]);
    free(plan->rtTable);
  }

  plan->magic = 0;
  free(plan);
}

DFTUndiff *DFTUndiff_MAKEPLANSUB(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags) {
  int32_t i, j, k;

  uint32_t linesize = SIMDBase_sizeOfCachelineInByte();
  uint32_t cachesize = SIMDBase_sizeOfDataCacheInByte();

  //

  if ((flags & DFT_FLAG_REAL) != 0 || (flags & DFT_FLAG_ALT_REAL) != 0) n /= 2;

  DFTUndiff *d = calloc(1, sizeof(DFTUndiff));

  d->magic = MAGIC_DFT;
  d->mode = SIMDBase_MODE;
  d->flags = flags;

  d->radix2thres = radix2thres;
  d->useCobra = useCobra;

  d->length = (uint32_t) n;
  d->log2len = DFT_ilog2((uint32_t) n);

  //

  SIMDBase_REAL *trigTable = SIMDBase_alignedMalloc(sizeof(SIMDBase_REAL)*n*2);
  d->ptTable = malloc(sizeof(SIMDBase_REAL *) * (d->log2len+1));

  SIMDBase_REAL *p = trigTable, **pp = d->ptTable;

  for(j=0;j<(int32_t)d->log2len+1;j++) {
    *pp++ = p;

    if ((1 << j) >= d->radix2thres) {
      for(i=0;i<(1 << j)/4+1;i++) {
	*p++ = (SIMDBase_REAL)COS(-2*M_PIl*i/(1 << j));
      }
      const int32_t step = linesize / sizeof(SIMDBase_REAL);
      p += (step - (p - trigTable) % step) % step;
    } else {
      for(i=0;i<(1 << j)/4;i++) {
	*p++ = (SIMDBase_REAL)COS(-2*M_PIl*i/(1 << j));
	*p++ = (SIMDBase_REAL)SIN(-2*M_PIl*i/(1 << j));
	*p++ = (SIMDBase_REAL)COS(-6*M_PIl*i/(1 << j));
	*p++ = (SIMDBase_REAL)SIN(-6*M_PIl*i/(1 << j));
      }
    }
  }

  //

  int32_t cobraQ;

  cobraQ = linesize / (sizeof(SIMDBase_VECT) * 2);

  for(;;) {
    if (1 << (cobraQ*2) >
	(cachesize / (sizeof(SIMDBase_VECT) * 2)/2))
      break;

    cobraQ++;
  }
  cobraQ--;

  d->cobraQ = cobraQ;

  if (cobraQ >= 4 && d->log2len >= 2*cobraQ) {
    SIMDBase_VECT *cobraT;
    int32_t *cobraR;

    if (d->log2len <= 2*cobraQ) cobraQ = d->log2len / 2;

    cobraT = SIMDBase_alignedMalloc(sizeof(SIMDBase_VECT)*2 * (1 << (cobraQ*2)));
    cobraR = (int32_t *)SIMDBase_alignedMalloc(sizeof(int32_t) * (1 << cobraQ));

    for(i=0;i<(1 << cobraQ);i++) cobraR[i] = bitR(i, cobraQ);

    d->cobraT = cobraT; d->cobraR = cobraR;
  } else {
    d->useCobra = 0;
  }

  //

  if ((d->flags & DFT_FLAG_REAL) != 0 || (d->flags & DFT_FLAG_ALT_REAL) != 0) {
    int32_t m = n*2;

    d->rtTable = malloc(sizeof(SIMDBase_REAL *)*2);
    d->rtTable[0] = SIMDBase_alignedMalloc(sizeof(SIMDBase_REAL)*m/2);
    d->rtTable[1] = SIMDBase_alignedMalloc(sizeof(SIMDBase_REAL)*m/2);

    for(k=0;k<m/4;k++) {
      d->rtTable[0][k*2+0] = 0.5-0.5*SIN(-2*M_PIl*k/m);
      d->rtTable[0][k*2+1] =     0.5*COS(-2*M_PIl*k/m);
      d->rtTable[1][k*2+0] = 0.5-0.5*SIN( 2*M_PIl*k/m);
      d->rtTable[1][k*2+1] =     0.5*COS( 2*M_PIl*k/m);
    }
  }

  //

  return (void *)d;
}

void *DFTUndiff_MAKEPLAN(uint64_t n, uint64_t flags) {
  if (flags & DFT_FLAG_VERBOSE) {
    printf("\n--------------------------------\n");
    printf("Making plan, mode = %s, dft length = %d\n", SIMDBase_NAME, (int)n);
    printf("Processor : %s\n", SIMDBase_getProcessorNameString());
    printf("Cache size (L2 + L3) : %d kbytes / thread\n", SIMDBase_sizeOfDataCacheInByte() / 1024);
    printf("Cache Line Size : %d bytes\n", SIMDBase_sizeOfCachelineInByte());
  }

  if (n <= 256 || (flags & 3) == 0) {
    return DFTUndiff_MAKEPLANSUB(n, n*2, (flags & DFT_FLAG_FORCE_COBRA) != 0, flags);
  }

  SIMDBase_REAL *s1 = SIMDBase_alignedMalloc(sizeof(SIMDBase_VECT)*n*2);

  int32_t i, j, ts, tsbest, useCobra = 0;
  double tick, tickmin;

  if (flags & DFT_FLAG_VERBOSE) {
    printf("\nWarming up before calibration ...");
    fflush(stdout);
  }

  // warming up
  tick = DFT_timeofday();
  while(DFT_timeofday() - tick < 0.5)
    ;

  if (flags & DFT_FLAG_VERBOSE) {
    printf(" done\n");
  }

  int32_t ntimes = 20000000.0 / n / DFT_ilog2(n);
  if (ntimes == 0) ntimes = 1;

  if (flags & DFT_FLAG_VERBOSE) {
    printf("nTimes = %d\n", ntimes);
  }

  //

  DFTUndiff *plan = DFTUndiff_MAKEPLANSUB(n, n*2, 0, flags);

  for(i=0;i<n*2*SIMDBase_VECTLEN;i++) {
    s1[i] = 0;
  }

  plan->s = (SIMDBase_VECT *)s1;

  if (plan->cobraT != NULL) {
    double tcobra = 0, trecur = 0;

    if (flags & DFT_FLAG_VERBOSE) {
      printf("\nChecking which bit-reversal method is faster\n");
    }

    //

    bitReversalCobraInplace(plan);

    tick = DFT_timeofday();

    for(j=0;j<ntimes*4;j++) {
      bitReversalCobraInplace(plan);
    }

    tcobra += DFT_timeofday() - tick;

    //

    bitReversalRecursive(plan->s, plan->length, 1, 0, 0);

    tick = DFT_timeofday();

    for(j=0;j<ntimes*4;j++) {
      bitReversalRecursive(plan->s, plan->length, 1, 0, 0);
    }

    trecur += DFT_timeofday() - tick;

    //

    bitReversalCobraInplace(plan);

    tick = DFT_timeofday();

    for(j=0;j<ntimes*4;j++) {
      bitReversalCobraInplace(plan);
    }

    tcobra += DFT_timeofday() - tick;

    //

    bitReversalRecursive(plan->s, plan->length, 1, 0, 0);

    tick = DFT_timeofday();

    for(j=0;j<ntimes*4;j++) {
      bitReversalRecursive(plan->s, plan->length, 1, 0, 0);
    }

    trecur += DFT_timeofday() - tick;

    //

    useCobra = tcobra < trecur;

    if ((flags & DFT_FLAG_FORCE_RECURSIVE) != 0) useCobra = 0;
    if ((flags & DFT_FLAG_FORCE_COBRA) != 0) useCobra = 1;

    if (flags & DFT_FLAG_VERBOSE) {
      printf("cobra : %g\n", tcobra);
      printf("recur : %g\n", trecur);
      if (useCobra) {
	printf("will use Cobra\n");
      } else {
	printf("will use the recursive reverser\n");
      }
    }
  }

  DFTUndiff_DESTROYPLAN(plan);

  //

  if (flags & DFT_FLAG_VERBOSE) {
    printf("\nDetermining radix 2 threshold\n");
  }

  plan = DFTUndiff_MAKEPLANSUB(n, n*2, useCobra, flags);

  for(j=0;j<ntimes;j++) {
    DFTUndiff_EXECUTE(plan, s1, -1);
    DFTUndiff_EXECUTE(plan, s1,  1);
  }

  DFTUndiff_DESTROYPLAN(plan);

  tsbest = -1;
  tickmin = 0;

  for(ts = 1024;ts <= n*2;ts *= 2) {
    plan = DFTUndiff_MAKEPLANSUB(n, ts, useCobra, flags);

    tick = DFT_timeofday();

    for(j=0;j<ntimes;j++) {
      DFTUndiff_EXECUTE(plan, s1, -1);
      DFTUndiff_EXECUTE(plan, s1,  1);
    }

    tick = DFT_timeofday() - tick;

    DFTUndiff_DESTROYPLAN(plan);

    if (tickmin == 0) tickmin = tick;

    if (flags & DFT_FLAG_VERBOSE) {
      printf("%d : %g\n",ts, (double)tick);
    }

    if (tick < tickmin) {
      tickmin = tick;
      tsbest = ts;
    }
  }

  if (tsbest == -1) tsbest = n*2;;

  if (flags & DFT_FLAG_VERBOSE) {
    //printf("forcing tsbest = 1024\n");
    //tsbest = 1024;
    printf("radix 2 threshold : %d\n\n", tsbest);

    double t = tickmin / ntimes / 2;
    double nf = 5 * n * log(n) / log(2) / (t * 1000000);

    printf("nFlops = %d x %g\n", SIMDBase_VECTLEN, nf);
  }

  plan = DFTUndiff_MAKEPLANSUB(n, tsbest, useCobra, flags);

  if (flags & DFT_FLAG_VERBOSE) {
    printf("\nDone making plan\n--------------------------------\n");
  }

  return plan;
}
