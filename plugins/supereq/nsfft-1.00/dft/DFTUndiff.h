#ifndef __DFTIMPL_H__
#define __DFTIMPL_H__

#include "SIMDBaseUndiff.h"

#define MAGIC_DFT 0x18839f6d82bb02b6ULL

typedef struct {
  uint64_t magic;

  SIMDBase_VECT *s;
  uint32_t offset1, offset2;
  uint32_t butlen, log2butlen;
  uint32_t stride;

  SIMDBase_REAL **ptTable;
  uint32_t length, log2len;

  int32_t radix2thres, flagTrans, useCobra;

  int32_t cobraQ;
  SIMDBase_VECT *cobraT;
  int32_t *cobraR;

  SIMDBase_REAL **rtTable;

  uint64_t flags;
  int32_t mode;
} DFTUndiff;

#if defined(ENABLE_PUREC_FLOAT) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_purec_float
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_purec_float
#define DFTUndiff_EXECUTE execute_purec_float
#define DFTUndiff_MAKEPLAN makePlan_purec_float
#define DFTUndiff_MAKEPLANSUB makePlanSub_purec_float
#define DFTUndiff_DESTROYPLAN destroyPlan_purec_float

#elif defined(ENABLE_PUREC_DOUBLE) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_purec_double
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_purec_double
#define DFTUndiff_EXECUTE execute_purec_double
#define DFTUndiff_MAKEPLAN makePlan_purec_double
#define DFTUndiff_MAKEPLANSUB makePlanSub_purec_double
#define DFTUndiff_DESTROYPLAN destroyPlan_purec_double

#elif defined(ENABLE_PUREC_LONGDOUBLE) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_purec_longdouble
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_purec_longdouble
#define DFTUndiff_EXECUTE execute_purec_longdouble
#define DFTUndiff_MAKEPLAN makePlan_purec_longdouble
#define DFTUndiff_MAKEPLANSUB makePlanSub_purec_longdouble
#define DFTUndiff_DESTROYPLAN destroyPlan_purec_longdouble

#elif defined(ENABLE_SSE_FLOAT) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_sse_float
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_sse_float
#define DFTUndiff_EXECUTE execute_sse_float
#define DFTUndiff_MAKEPLAN makePlan_sse_float
#define DFTUndiff_MAKEPLANSUB makePlanSub_sse_float
#define DFTUndiff_DESTROYPLAN destroyPlan_sse_float

#elif defined(ENABLE_SSE2_DOUBLE) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_sse2_double
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_sse2_double
#define DFTUndiff_EXECUTE execute_sse2_double
#define DFTUndiff_MAKEPLAN makePlan_sse2_double
#define DFTUndiff_MAKEPLANSUB makePlanSub_sse2_double
#define DFTUndiff_DESTROYPLAN destroyPlan_sse2_double

#elif defined(ENABLE_NEON_FLOAT) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_neon_float
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_neon_float
#define DFTUndiff_EXECUTE execute_neon_float
#define DFTUndiff_MAKEPLAN makePlan_neon_float
#define DFTUndiff_MAKEPLANSUB makePlanSub_neon_float
#define DFTUndiff_DESTROYPLAN destroyPlan_neon_float

#elif defined(ENABLE_AVX_FLOAT) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_avx_float
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_avx_float
#define DFTUndiff_EXECUTE execute_avx_float
#define DFTUndiff_MAKEPLAN makePlan_avx_float
#define DFTUndiff_MAKEPLANSUB makePlanSub_avx_float
#define DFTUndiff_DESTROYPLAN destroyPlan_avx_float

#elif defined(ENABLE_AVX_DOUBLE) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_avx_double
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_avx_double
#define DFTUndiff_EXECUTE execute_avx_double
#define DFTUndiff_MAKEPLAN makePlan_avx_double
#define DFTUndiff_MAKEPLANSUB makePlanSub_avx_double
#define DFTUndiff_DESTROYPLAN destroyPlan_avx_double

#elif defined(ENABLE_ALTIVEC_FLOAT) ////////////////////////////////////////////

#define DFTUndiff_GETMODEPARAMINT getModeParamInt_altivec_float
#define DFTUndiff_GETMODEPARAMSTRING getModeParamString_altivec_float
#define DFTUndiff_EXECUTE execute_altivec_float
#define DFTUndiff_MAKEPLAN makePlan_altivec_float
#define DFTUndiff_MAKEPLANSUB makePlanSub_altivec_float
#define DFTUndiff_DESTROYPLAN destroyPlan_altivec_float

#endif ////////////////////////////////////////////////////////////////////

#endif
