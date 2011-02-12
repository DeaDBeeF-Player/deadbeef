#ifndef _SIMDBaseUndiff_H_
#define _SIMDBaseUndiff_H_

#if defined(ENABLE_PUREC_FLOAT) ////////////////////////////////////////////

typedef float SIMDBase_REAL;
typedef float SIMDBase_VECT;

#define SIMDBase_MODE 1
#define SIMDBase_TYPE SIMDBase_TYPE_FLOAT
#define SIMDBase_VECTLEN 1
#define SIMDBase_NAME "Pure C float"
#define SIMDBaseUndiff_DETECT detect_purec_float
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_purec_float
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_purec_float

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return *p; }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { *p = u; }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return f; }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return *p; }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return u + v; }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return u - v; }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return u * v; }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return -u; }

#elif defined(ENABLE_PUREC_DOUBLE) ////////////////////////////////////////////

typedef double SIMDBase_REAL;
typedef double SIMDBase_VECT;

#define SIMDBase_MODE 2
#define SIMDBase_TYPE SIMDBase_TYPE_DOUBLE
#define SIMDBase_VECTLEN 1
#define SIMDBase_NAME "Pure C double"
#define SIMDBaseUndiff_DETECT detect_purec_double
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_purec_double
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_purec_double

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return *p; }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { *p = u; }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return f; }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return *p; }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return u + v; }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return u - v; }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return u * v; }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return -u; }

#elif defined(ENABLE_PUREC_LONGDOUBLE) ////////////////////////////////////////////

typedef long double SIMDBase_REAL;
typedef long double SIMDBase_VECT;

#define SIMDBase_MODE 3
#define SIMDBase_TYPE SIMDBase_TYPE_LONGDOUBLE
#define SIMDBase_VECTLEN 1
#define SIMDBase_NAME "Pure C long double"
#define SIMDBaseUndiff_DETECT detect_purec_longdouble
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_purec_longdouble
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_purec_longdouble

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return *p; }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { *p = u; }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return f; }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return *p; }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return u + v; }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return u - v; }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return u * v; }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return -u; }

#elif defined(ENABLE_SSE_FLOAT) ////////////////////////////////////////////

#include <xmmintrin.h>

typedef float SIMDBase_REAL;
typedef __m128 SIMDBase_VECT;

#define SIMDBase_MODE 4
#define SIMDBase_TYPE SIMDBase_TYPE_FLOAT
#define SIMDBase_VECTLEN 4
#define SIMDBase_NAME "x86 SSE float"
#define SIMDBaseUndiff_DETECT detect_sse_float
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_sse_float
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_sse_float

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return _mm_load_ps((float *)p); }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { _mm_store_ps((float *)p, u); }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return _mm_set1_ps(f); }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return _mm_load1_ps(p); }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm_add_ps(u, v); }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm_sub_ps(u, v); }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm_mul_ps(u, v); }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return _mm_xor_ps(u, _mm_set_ps(-0.0f, -0.0f, -0.0f, -0.0f)); }

#elif defined(ENABLE_SSE2_DOUBLE) ////////////////////////////////////////////

#include <emmintrin.h>

typedef double SIMDBase_REAL;
typedef __m128d SIMDBase_VECT;

#define SIMDBase_MODE 5
#define SIMDBase_TYPE SIMDBase_TYPE_DOUBLE
#define SIMDBase_VECTLEN 2
#define SIMDBase_NAME "x86 SSE2 double"
#define SIMDBaseUndiff_DETECT detect_sse2_double
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_sse2_double
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_sse2_double

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return _mm_load_pd((double *)p); }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { _mm_store_pd((double *)p, u); }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return _mm_set1_pd(f); }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return _mm_load1_pd(p); }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm_add_pd(u, v); }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm_sub_pd(u, v); }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm_mul_pd(u, v); }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return _mm_xor_pd(u, _mm_set_pd(-0.0, -0.0)); }

#elif defined(ENABLE_NEON_FLOAT) ////////////////////////////////////////////

#include <arm_neon.h>

typedef float32_t SIMDBase_REAL;
typedef float32x4_t SIMDBase_VECT;

#define SIMDBase_MODE 6
#define SIMDBase_TYPE SIMDBase_TYPE_FLOAT
#define SIMDBase_VECTLEN 4
#define SIMDBase_NAME "ARM NEON float"
#define SIMDBaseUndiff_DETECT detect_neon_float
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_neon_float
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_neon_float

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return vld1q_f32((float32_t *)p); }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { vst1q_f32((float32_t *)p, u); }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return vdupq_n_f32(f); }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return vdupq_n_f32(*p); }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return vaddq_f32(u, v); }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return vsubq_f32(u, v); }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return vmulq_f32(u, v); }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { 
  return vreinterpretq_f32_u32( veorq_u32(vreinterpretq_u32_f32(u), vdupq_n_u32(0x80000000U)));
}

#define SIMDBase_FMADD_AVAILABLE

static inline SIMDBase_VECT SIMDBase_FMADDi(SIMDBase_VECT u, SIMDBase_VECT v, SIMDBase_VECT w) { return vmlaq_f32(w, u, v); } // w + u * v
static inline SIMDBase_VECT SIMDBase_FMSUBi(SIMDBase_VECT u, SIMDBase_VECT v, SIMDBase_VECT w) { return vmlsq_f32(w, u, v); } // w - u * v

#elif defined(ENABLE_AVX_FLOAT) ////////////////////////////////////////////

#include <immintrin.h>

typedef float SIMDBase_REAL;
typedef __m256 SIMDBase_VECT;

#define SIMDBase_MODE 7
#define SIMDBase_TYPE SIMDBase_TYPE_FLOAT
#define SIMDBase_VECTLEN 8
#define SIMDBase_NAME "x86 AVX float"
#define SIMDBaseUndiff_DETECT detect_avx_float
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_avx_float
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_avx_float

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return _mm256_load_ps((float *)p); }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { _mm256_store_ps((float *)p, u); }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return _mm256_set1_ps(f); }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return _mm256_set1_ps(*p); }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm256_add_ps(u, v); }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm256_sub_ps(u, v); }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm256_mul_ps(u, v); }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return _mm256_xor_ps(u, _mm256_set1_ps(-0.0f)); }

#elif defined(ENABLE_AVX_DOUBLE) ////////////////////////////////////////////

#include <immintrin.h>

typedef double SIMDBase_REAL;
typedef __m256d SIMDBase_VECT;

#define SIMDBase_MODE 8
#define SIMDBase_TYPE SIMDBase_TYPE_DOUBLE
#define SIMDBase_VECTLEN 4
#define SIMDBase_NAME "x86 AVX double"
#define SIMDBaseUndiff_DETECT detect_avx_double
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_avx_double
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_avx_double

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return _mm256_load_pd((double *)p); }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { _mm256_store_pd((double *)p, u); }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return _mm256_set1_pd(f); }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return _mm256_set1_pd(*p); }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm256_add_pd(u, v); }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm256_sub_pd(u, v); }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return _mm256_mul_pd(u, v); }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return _mm256_xor_pd(u, _mm256_set1_pd(-0.0)); }

#elif defined(ENABLE_ALTIVEC_FLOAT) ////////////////////////////////////////////

#include <altivec.h>

typedef float SIMDBase_REAL;
typedef vector float SIMDBase_VECT;

#define SIMDBase_MODE 9
#define SIMDBase_TYPE SIMDBase_TYPE_FLOAT
#define SIMDBase_VECTLEN 4
#define SIMDBase_NAME "PowerPC AltiVec float"
#define SIMDBaseUndiff_DETECT detect_altivec_float
#define SIMDBaseUndiff_GETMODEPARAMINT getModeParamInt_altivec_float
#define SIMDBaseUndiff_GETMODEPARAMSTRING getModeParamString_altivec_float

static inline SIMDBase_VECT SIMDBase_LOAD(SIMDBase_VECT *p) { return vec_ld(0, p); }
static inline void SIMDBase_STOR(SIMDBase_VECT *p, SIMDBase_VECT u) { vec_st(u, 0, p); }
static inline SIMDBase_VECT SIMDBase_SET1(SIMDBase_REAL f) { return (vector float){f, f, f, f}; }
static inline SIMDBase_VECT SIMDBase_LOAD1(SIMDBase_REAL *p) { return (vector float){*p, *p, *p, *p}; }
static inline SIMDBase_VECT SIMDBase_ADDi(SIMDBase_VECT u, SIMDBase_VECT v) { return vec_add(u, v); }
static inline SIMDBase_VECT SIMDBase_SUBi(SIMDBase_VECT u, SIMDBase_VECT v) { return vec_sub(u, v); }
static inline SIMDBase_VECT SIMDBase_MULi(SIMDBase_VECT u, SIMDBase_VECT v) { return vec_madd(u, v, (vector float){0, 0, 0, 0}); }
static inline SIMDBase_VECT SIMDBase_NEGi(SIMDBase_VECT u) { return vec_xor(u, (vector float){-0.0f, -0.0f, -0.0f, -0.0f}); }

#define SIMDBase_FMADD_AVAILABLE

static inline SIMDBase_VECT SIMDBase_FMADDi(SIMDBase_VECT u, SIMDBase_VECT v, SIMDBase_VECT w) { return vec_madd(u, v, w); } // w + u * v
static inline SIMDBase_VECT SIMDBase_FMSUBi(SIMDBase_VECT u, SIMDBase_VECT v, SIMDBase_VECT w) { return vec_nmsub(u, v, w); } // w - u * v

#endif ////////////////////////////////////////////////////////////////////

static inline SIMDBase_VECT SIMDBase_ADDm(SIMDBase_VECT *p, SIMDBase_VECT *q) { return SIMDBase_ADDi(SIMDBase_LOAD(p), SIMDBase_LOAD(q)); }
static inline SIMDBase_VECT SIMDBase_SUBm(SIMDBase_VECT *p, SIMDBase_VECT *q) { return SIMDBase_SUBi(SIMDBase_LOAD(p), SIMDBase_LOAD(q)); }

#endif
