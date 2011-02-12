#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <sys/time.h>

#include "SIMDBase.h"
#include "DFT.h"
#include "DFTUndiff.h"

int32_t getModeParamInt_purec_float(int32_t paramId);
int32_t getModeParamInt_purec_double(int32_t paramId);
int32_t getModeParamInt_purec_longdouble(int32_t paramId);
int32_t getModeParamInt_sse_float(int32_t paramId);
int32_t getModeParamInt_sse2_double(int32_t paramId);
int32_t getModeParamInt_neon_float(int32_t paramId);
int32_t getModeParamInt_avx_float(int32_t paramId);
int32_t getModeParamInt_avx_double(int32_t paramId);
int32_t getModeParamInt_altivec_float(int32_t paramId);

char * getModeParamString_purec_float(int32_t paramId);
char * getModeParamString_purec_double(int32_t paramId);
char * getModeParamString_purec_longdouble(int32_t paramId);
char * getModeParamString_sse_float(int32_t paramId);
char * getModeParamString_sse2_double(int32_t paramId);
char * getModeParamString_neon_float(int32_t paramId);
char * getModeParamString_avx_float(int32_t paramId);
char * getModeParamString_avx_double(int32_t paramId);
char * getModeParamString_altivec_float(int32_t paramId);

void *makePlan_purec_float(uint64_t n, uint64_t flags);
void *makePlan_purec_double(uint64_t n, uint64_t flags);
void *makePlan_purec_longdouble(uint64_t n, uint64_t flags);
void *makePlan_sse_float(uint64_t n, uint64_t flags);
void *makePlan_sse2_double(uint64_t n, uint64_t flags);
void *makePlan_neon_float(uint64_t n, uint64_t flags);
void *makePlan_avx_float(uint64_t n, uint64_t flags);
void *makePlan_avx_double(uint64_t n, uint64_t flags);
void *makePlan_altivec_float(uint64_t n, uint64_t flags);

void *makePlanSub_purec_float(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_purec_double(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_purec_longdouble(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_sse_float(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_sse2_double(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_neon_float(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_avx_float(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_avx_double(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);
void *makePlanSub_altivec_float(uint64_t n, int32_t radix2thres, int32_t useCobra, uint64_t flags);

void destroyPlan_purec_float(void *p);
void destroyPlan_purec_double(void *p);
void destroyPlan_purec_longdouble(void *p);
void destroyPlan_sse_float(void *p);
void destroyPlan_sse2_double(void *p);
void destroyPlan_neon_float(void *p);
void destroyPlan_avx_float(void *p);
void destroyPlan_avx_double(void *p);
void destroyPlan_altivec_float(void *p);

void execute_purec_float(void *p, void *s, int32_t dir);
void execute_purec_double(void *p, void *s, int32_t dir);
void execute_purec_longdouble(void *p, void *s, int32_t dir);
void execute_sse_float(void *p, void *s, int32_t dir);
void execute_sse2_double(void *p, void *s, int32_t dir);
void execute_neon_float(void *p, void *s, int32_t dir);
void execute_avx_float(void *p, void *s, int32_t dir);
void execute_avx_double(void *p, void *s, int32_t dir);
void execute_altivec_float(void *p, void *s, int32_t dir);

void *DFT_init(int32_t mode, uint64_t n, uint64_t flags) {
  switch(mode) {
#if defined(ENABLE_PUREC_FLOAT)
  case 1: return makePlan_purec_float(n, flags); break;
#endif
#if defined(ENABLE_PUREC_DOUBLE)
  case 2: return makePlan_purec_double(n, flags); break;
#endif
#if defined(ENABLE_PUREC_LONGDOUBLE)
  case 3: return makePlan_purec_longdouble(n, flags); break;
#endif
#if defined(ENABLE_SSE_FLOAT)
  case 4: return makePlan_sse_float(n, flags); break;
#endif
#if defined(ENABLE_SSE2_DOUBLE)
  case 5: return makePlan_sse2_double(n, flags); break;
#endif
#if defined(ENABLE_NEON_FLOAT)
  case 6: return makePlan_neon_float(n, flags); break;
#endif
#if defined(ENABLE_AVX_FLOAT)
  case 7: return makePlan_avx_float(n, flags); break;
#endif
#if defined(ENABLE_AVX_DOUBLE)
  case 8: return makePlan_avx_double(n, flags); break;
#endif
#if defined(ENABLE_ALTIVEC_FLOAT)
  case 9: return makePlan_altivec_float(n, flags); break;
#endif
  default: break;
  }

  return NULL;
}

void DFT_dispose(void *p, int32_t mode) {
  switch(mode) {
#if defined(ENABLE_PUREC_FLOAT)
  case 1: destroyPlan_purec_float(p); break;
#endif
#if defined(ENABLE_PUREC_DOUBLE)
  case 2: destroyPlan_purec_double(p); break;
#endif
#if defined(ENABLE_PUREC_LONGDOUBLE)
  case 3: destroyPlan_purec_longdouble(p); break;
#endif
#if defined(ENABLE_SSE_FLOAT)
  case 4: destroyPlan_sse_float(p); break;
#endif
#if defined(ENABLE_SSE2_DOUBLE)
  case 5: destroyPlan_sse2_double(p); break;
#endif
#if defined(ENABLE_NEON_FLOAT)
  case 6: destroyPlan_neon_float(p); break;
#endif
#if defined(ENABLE_AVX_FLOAT)
  case 7: destroyPlan_avx_float(p); break;
#endif
#if defined(ENABLE_AVX_DOUBLE)
  case 8: destroyPlan_avx_double(p); break;
#endif
#if defined(ENABLE_ALTIVEC_FLOAT)
  case 9: destroyPlan_altivec_float(p); break;
#endif
  default: break;
  }
}

void DFT_execute(void *p, int32_t mode, void *s, int32_t dir) {
  switch(mode) {
#if defined(ENABLE_PUREC_FLOAT)
  case 1: return execute_purec_float(p, s, dir); break;
#endif
#if defined(ENABLE_PUREC_DOUBLE)
  case 2: return execute_purec_double(p, s, dir); break;
#endif
#if defined(ENABLE_PUREC_LONGDOUBLE)
  case 3: return execute_purec_longdouble(p, s, dir); break;
#endif
#if defined(ENABLE_SSE_FLOAT)
  case 4: return execute_sse_float(p, s, dir); break;
#endif
#if defined(ENABLE_SSE2_DOUBLE)
  case 5: return execute_sse2_double(p, s, dir); break;
#endif
#if defined(ENABLE_NEON_FLOAT)
  case 6: return execute_neon_float(p, s, dir); break;
#endif
#if defined(ENABLE_AVX_FLOAT)
  case 7: return execute_avx_float(p, s, dir); break;
#endif
#if defined(ENABLE_AVX_DOUBLE)
  case 8: return execute_avx_double(p, s, dir); break;
#endif
#if defined(ENABLE_ALTIVEC_FLOAT)
  case 9: return execute_altivec_float(p, s, dir); break;
#endif
  default: break;
  }
}

#define FILE_FORMAT_VERSION 0

int32_t DFT_fwrite(void *p2, FILE *fp) {
  DFTUndiff *p = (DFTUndiff *)p2;
  if (p->magic != MAGIC_DFT) abort();

  if (fprintf(fp, "nsfft file format : %d\n", FILE_FORMAT_VERSION) <= 0) return 0;
  if (fprintf(fp, "arch : %s\n", SIMDBase_getProcessorNameString()) <= 0) return 0;
  if (fprintf(fp, "computation mode : %d\n", p->mode) <= 0) return 0;
  if (fprintf(fp, "length : %d\n", ((p->flags & DFT_FLAG_REAL) != 0 || (p->flags & DFT_FLAG_ALT_REAL) != 0)? p->length * 2 : p->length) <= 0) return 0;
  if (fprintf(fp, "radix2 threshold : %d\n", p->radix2thres) <= 0) return 0;
  if (fprintf(fp, "transpose : %d\n", p->flagTrans) <= 0) return 0;
  if (fprintf(fp, "bit reversal : %d\n", p->useCobra) <= 0) return 0;
  if (fprintf(fp, "flags : %llx\n", (unsigned long long int)p->flags) <= 0) return 0;
  if (fprintf(fp, "%s\n", "end :") <= 0) return 0;

  return 1;
}

static char *startsWith(char *str1, char *str2) {
  if (strncmp(str1, str2, strlen(str2)) == 0) {
    return str1 + strlen(str2);
  }

  return NULL;
}

DFT *DFT_fread(FILE *fp, int32_t *errcode) {
  int length = -1, radix2thres = -1, flagTrans = -1, useCobra = -1;
  int mode = -1, formatver = -1;
  unsigned long long int flags = (1ULL << 63);

  if (errcode != NULL) *errcode = DFT_ERROR_NOERROR;

  for(;;) {
    char buf[256], *q;
    if (fgets(buf, 255, fp) == NULL) { if (errcode != NULL) *errcode = DFT_ERROR_UNEXPECTED_EOF; return NULL; }

    if ((q = startsWith(buf, "nsfft file format :")) != NULL) {
      if (1 != sscanf(q, "%d", &formatver)) { if (errcode != NULL) *errcode = DFT_ERROR_FILE_IO; return NULL; }
    } else if ((q = startsWith(buf, "computation mode :")) != NULL) {
      if (1 != sscanf(q, "%d", &mode)) { if (errcode != NULL) *errcode = DFT_ERROR_FILE_IO; return NULL; }
    } else if ((q = startsWith(buf, "length :")) != NULL) {
      if (1 != sscanf(q, "%d", &length)) { if (errcode != NULL) *errcode = DFT_ERROR_FILE_IO; return NULL; }
    } else if ((q = startsWith(buf, "radix2 threshold :")) != NULL) {
      if (1 != sscanf(q, "%d", &radix2thres)) { if (errcode != NULL) *errcode = DFT_ERROR_FILE_IO; return NULL; }
    } else if ((q = startsWith(buf, "transpose :")) != NULL) {
      if (1 != sscanf(q, "%d", &flagTrans)) { if (errcode != NULL) *errcode = DFT_ERROR_FILE_IO; return NULL; }
    } else if ((q = startsWith(buf, "bit reversal :")) != NULL) {
      if (1 != sscanf(q, "%d", &useCobra)) { if (errcode != NULL) *errcode = DFT_ERROR_FILE_IO; return NULL; }
    } else if ((q = startsWith(buf, "flags :")) != NULL) {
      if (1 != sscanf(q, "%llx", &flags)) { if (errcode != NULL) *errcode = DFT_ERROR_FILE_IO; return NULL; }
    } else if ((q = startsWith(buf, "end :")) != NULL) {
      break;
    }
  }

  if (formatver > FILE_FORMAT_VERSION) {
    if (errcode != NULL) *errcode = DFT_ERROR_FILE_VERSION;
    return NULL;
  }

  switch(SIMDBase_detect(mode)) {
  case 1:
    break;
  case 0:
    if (errcode != NULL) *errcode = DFT_ERROR_MODE_NOT_AVAILABLE;
    return NULL;
  case -1:
    if (errcode != NULL) *errcode = DFT_ERROR_MODE_NOT_COMPILED_IN;
    return NULL;
  }

  switch(mode) {
#if defined(ENABLE_PUREC_FLOAT)
  case 1: return makePlanSub_purec_float(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_PUREC_DOUBLE)
  case 2: return makePlanSub_purec_double(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_PUREC_LONGDOUBLE)
  case 3: return makePlanSub_purec_longdouble(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_SSE_FLOAT)
  case 4: return makePlanSub_sse_float(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_SSE2_DOUBLE)
  case 5: return makePlanSub_sse2_double(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_NEON_FLOAT)
  case 6: return makePlanSub_neon_float(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_AVX_FLOAT)
  case 7: return makePlanSub_avx_float(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_AVX_DOUBLE)
  case 8: return makePlanSub_avx_double(length, radix2thres, useCobra, flags);
#endif
#if defined(ENABLE_ALTIVEC_FLOAT)
  case 9: return makePlanSub_altivec_float(length, radix2thres, useCobra, flags);
#endif
  }

  if (errcode != NULL) *errcode = DFT_ERROR_UNKNOWN_MODE;

  return NULL;
}

int32_t DFT_getPlanParamInt(int32_t paramId, void *p2) {
  DFTUndiff *p = (DFTUndiff *)p2;
  if (p->magic != MAGIC_DFT) abort();

  switch(paramId) {
  case DFT_PARAMID_MODE: return p->mode;
  case DFT_PARAMID_FFT_LENGTH:
    if ((p->flags & DFT_FLAG_REAL) != 0) return p->length * 2;
    if ((p->flags & DFT_FLAG_ALT_REAL) != 0) return p->length * 2;
    return p->length;
  case DFT_PARAMID_IS_REAL_TRANSFORM: return (p->flags & DFT_FLAG_REAL) ? 1 : 0;
  case DFT_PARAMID_IS_ALT_REAL_TRANSFORM: return (p->flags & DFT_FLAG_ALT_REAL) ? 1 : 0;
  case DFT_PARAMID_NO_BIT_REVERSAL: return (p->flags & DFT_FLAG_NO_BITREVERSAL) ? 1 : 0;
  case DFT_PARAMID_TEST_RUN: return p->flags & 3;
  }

  return -1;
}

#if 0
char *DFT_getPlanParamString(int32_t paramId, void *p2) {
  dft_t *p = (dft_t *)p2;
  if (p->magic != MAGIC_NSDFT) abort();

  return NULL;
}
#endif

uint32_t DFT_ilog2(uint32_t q) {
  static const uint32_t tab[] = {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
  uint32_t r = 0,qq;

  if (q & 0xffff0000) r = 16;

  q >>= r;
  qq = q | (q >> 1);
  qq |= (qq >> 2);
  qq = ((qq & 0x10) >> 4) | ((qq & 0x100) >> 7) | ((qq & 0x1000) >> 10);

  return r + tab[qq] * 4 + tab[q >> (tab[qq] * 4)] - 1;
}

double DFT_timeofday(void) {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return (double)tp.tv_sec+(1e-6)*tp.tv_usec;
}
