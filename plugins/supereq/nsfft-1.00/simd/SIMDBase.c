#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>

#include "SIMDBase.h"

void detect_purec_float(void);
void detect_purec_double(void);
void detect_purec_longdouble(void);
void detect_sse_float(void);
void detect_sse2_double(void);
void detect_neon_float(void);
void detect_avx_float(void);
void detect_avx_double(void);
void detect_altivec_float(void);

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

uint8_t detectBuffer[256];
char SIMDBase_processorNameString[256];

static char *startsWith(char *str1, char *str2) {
  if (strncmp(str1, str2, strlen(str2)) == 0) {
    return str1 + strlen(str2);
  }

  return NULL;
}

#if defined(__linux__)
static char *tryReadingProcCpuinfo(char *entry) {
  int i;

  FILE *fp = fopen("/proc/cpuinfo", "r");
  if (fp == NULL) return NULL;

  for(i=0;i<100;i++) {
    char *q;
    bzero(SIMDBase_processorNameString, 256);
    if (fgets(SIMDBase_processorNameString, 255, fp) == NULL) break;

    if ((q = startsWith(SIMDBase_processorNameString, entry)) != NULL) {
      int j;
      fclose(fp);

      for(j=0;j<256;j++) {
	if (SIMDBase_processorNameString[j] == '\n') SIMDBase_processorNameString[j] = ' ';
      }
      while(*q != '\0' && *q != ':' && q - SIMDBase_processorNameString < 200) q++;
      if (q - SIMDBase_processorNameString >= 200) return NULL;
      if (*q == ':' && *(q+1) == ' ') return q + 2;
      return NULL;
    }
  }

  fclose(fp);
  return NULL;
}
#else
static char *tryReadingProcCpuinfo(char *entry) { return NULL; }
#endif

#if defined(__i386__)
static void SIMDBase_x86cpuid(uint32_t out[4], uint32_t eax, uint32_t ecx) {
  uint32_t a, b, c, d;
  __asm__ __volatile__("pushl %%eax;      \n\t"
		       "pushl %%ebx;      \n\t"
		       "pushl %%ecx;      \n\t"
		       "pushl %%edx;      \n\t"
		       "cpuid;            \n\t"
		       "movl %%eax, %0;   \n\t"
		       "movl %%ebx, %1;   \n\t"
		       "movl %%ecx, %2;   \n\t"
		       "movl %%edx, %3;   \n\t"
		       "popl %%edx;       \n\t"
		       "popl %%ecx;       \n\t"
		       "popl %%ebx;       \n\t"
		       "popl %%eax;       \n\t"
		       : "=m"(a), "=m"(b), "=m"(c), "=m"(d)
		       : "a"(eax), "c"(ecx)
		       : "cc");
  out[0] = a; out[1] = b; out[2] = c; out[3] = d;
}
#endif

#if defined(__x86_64__)
static void SIMDBase_x86cpuid(uint32_t out[4], uint32_t eax, uint32_t ecx) {
  uint32_t a, b, c, d;
  __asm__ __volatile__ ("cpuid" : "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (eax), "c"(ecx));
  out[0] = a; out[1] = b; out[2] = c; out[3] = d;
}
#endif

#if defined(__i386__) || defined(__x86_64__)
static void getCacheParam(CacheParam *p) {
  static int l2assoc[] = {0,1,2,0,4,0,8,0,16,0,32,48,64,96,128,-1};
  int32_t i;
  uint32_t out[4];

  for(i=0;i<8;i++) {
    p->size[i] = p->assoc[i] = 0;
  }

  SIMDBase_x86cpuid(out, 4, 0);

  if ((out[0] & 0xf) != 0) {
    p->linesize = ((out[1] >> 0) & 2047)+1;
    for(i=0;i<8;i++) {
      SIMDBase_x86cpuid(out, 4, i);
      if ((out[0] & 0xf) == 0) break;
      int level = (out[0] >> 5) & 0x7;
      int type  = (out[0] >> 0) & 0xf;
      int assoc = ((out[1] >> 22) & 1023)+1;
      int part  = ((out[1] >> 12) & 1023)+1;
      int lsize = ((out[1] >> 0) & 2047)+1;
      int nsets = ((out[2] >> 0))+1;
      int nthre = ((out[0] >> 14) & 1023)+1;

      if (type != 1 && type != 3) continue;
      p->assoc[level-1] = assoc;
      p->size[level-1] = (uint64_t)assoc * part * lsize * nsets / nthre;
    }
  } else {
    SIMDBase_x86cpuid(out, 0x80000008U, 0);
    int ncores = (out[2] & 0xff) + 1;

    SIMDBase_x86cpuid(out, 0x80000005U, 0);
    p->linesize = out[2] & 255;
    p->size[0] = (out[2] >> 24) * 1024 / ncores;
    p->assoc[0] = (out[2] >> 16) & 0xff;

    SIMDBase_x86cpuid(out, 0x80000006U, 0);
    p->size[1] = (out[2] >> 16) * 1024 / ncores;
    p->assoc[1] = l2assoc[(out[2] >> 12) & 0xf];
    p->size[2] = (out[3] >> 18) * 512 * 1024 / ncores;
    p->assoc[2] = l2assoc[(out[3] >> 12) & 0xf];
  }

  if (p->size[0] == 0) {
    p->size[0] = 16 * 1024;
    p->assoc[0] = 4;
  }

  if (p->size[1] == 0) {
    p->size[1] = 256 * 1024;
    p->assoc[1] = 4;
  }
}

char *SIMDBase_getProcessorNameString() {
  union {
    uint32_t info[4];
    uint8_t str[16];
  } u;
  int i,j;
  char *p;

  p = SIMDBase_processorNameString;

  SIMDBase_x86cpuid(u.info, 0, 0);

  for(i=0;i<4;i++) *p++ = u.str[i+4];
  for(i=0;i<4;i++) *p++ = u.str[i+12];
  for(i=0;i<4;i++) *p++ = u.str[i+8];

  *p++ = ' ';

  for(i=0;i<3;i++) {
    SIMDBase_x86cpuid(u.info, i + 0x80000002, 0);

    for(j=0;j<16;j++) {
      *p++ = u.str[j];
    }
  }

  *p++ = '\n';

  return SIMDBase_processorNameString;
}
#else
char *SIMDBase_getProcessorNameString() {
  char *p = "Unknown";
#if defined(__powerpc__)
  if ((p = tryReadingProcCpuinfo("cpu")) == NULL) p = "PowerPC";
#elif defined(__arm__)
  if ((p = tryReadingProcCpuinfo("Processor")) == NULL) p = "ARM";
#endif

  return p;
}
#endif

int32_t SIMDBase_sizeOfCachelineInByte() {
#if defined(__i386__) || defined(__x86_64__)
  CacheParam p;
  getCacheParam(&p);
  return p.linesize;
#else
  return 64;
#endif
}

int32_t SIMDBase_sizeOfDataCacheInByte() {
#if defined(__i386__) || defined(__x86_64__)
  CacheParam p;
  getCacheParam(&p);
  return p.size[1] + p.size[2]; // L2 + L3
#else
  return 256 * 1024;
#endif
}

static jmp_buf sigjmp;

static void sighandler(int signum) {
  longjmp(sigjmp, 1);
}

int32_t SIMDBase_detect(int32_t paramId) {
#if defined(__i386__) || defined(__x86_64__)
  uint32_t reg[4];
#endif

  switch(paramId) {
  case SIMDBase_MODE_PUREC_FLOAT:
#if defined(ENABLE_PUREC_FLOAT)
    return 1;
#else
    return -1;
#endif
  case SIMDBase_MODE_PUREC_DOUBLE:
#if defined(ENABLE_PUREC_DOUBLE)
    return 1;
#else
    return -1;
#endif
  case SIMDBase_MODE_PUREC_LONGDOUBLE:
#if defined(ENABLE_PUREC_LONGDOUBLE)
    return 1;
#else
    return -1;
#endif
  case SIMDBase_MODE_SSE_FLOAT:
#if defined(ENABLE_SSE_FLOAT)
    SIMDBase_x86cpuid(reg, 1, 0);
    return (reg[3] & (1 << 25)) != 0;
#else
    return -1;
#endif
  case SIMDBase_MODE_SSE2_DOUBLE:
#if defined(ENABLE_SSE2_DOUBLE)
    SIMDBase_x86cpuid(reg, 1, 0);
    return (reg[3] & (1 << 26)) != 0;
#else
    return -1;
#endif
  case SIMDBase_MODE_AVX_FLOAT:
#if defined(ENABLE_AVX_FLOAT)
    SIMDBase_x86cpuid(reg, 1, 0);
    return (reg[2] & (1 << 28)) != 0;
#else
    return -1;
#endif
  case SIMDBase_MODE_AVX_DOUBLE:
#if defined(ENABLE_AVX_DOUBLE)
    SIMDBase_x86cpuid(reg, 1, 0);
    return (reg[2] & (1 << 28)) != 0;
#else
    return -1;
#endif
  default:
    break;
  }

  signal(SIGILL, sighandler);

  if (setjmp(sigjmp) == 0) {
    switch(paramId) {
#if defined(ENABLE_NEON_FLOAT)
    case SIMDBase_MODE_NEON_FLOAT:
      detect_neon_float();
      break;
#endif
#if defined(ENABLE_ALTIVEC_FLOAT)
    case SIMDBase_MODE_ALTIVEC_FLOAT:
      detect_altivec_float();
      break;
#endif
    default:
      signal(SIGILL, SIG_DFL);
      return -1;
    }
    signal(SIGILL, SIG_DFL);
    return 1;
  } else {
    signal(SIGILL, SIG_DFL);
    return 0;
  }
}

int32_t SIMDBase_chooseBestMode(int32_t typeId) {
  switch(typeId) {
  case SIMDBase_TYPE_HALF:
    break;
  case SIMDBase_TYPE_FLOAT:
    if (SIMDBase_detect(SIMDBase_MODE_AVX_FLOAT) == 1) return SIMDBase_MODE_AVX_FLOAT;
    if (SIMDBase_detect(SIMDBase_MODE_SSE_FLOAT) == 1) return SIMDBase_MODE_SSE_FLOAT;
    if (SIMDBase_detect(SIMDBase_MODE_NEON_FLOAT) == 1) return SIMDBase_MODE_NEON_FLOAT;
    if (SIMDBase_detect(SIMDBase_MODE_ALTIVEC_FLOAT) == 1) return SIMDBase_MODE_ALTIVEC_FLOAT;
    if (SIMDBase_detect(SIMDBase_MODE_PUREC_FLOAT) == 1) return SIMDBase_MODE_PUREC_FLOAT;
    break;

  case SIMDBase_TYPE_DOUBLE:
    if (SIMDBase_detect(SIMDBase_MODE_AVX_DOUBLE) == 1) return SIMDBase_MODE_AVX_DOUBLE;
    if (SIMDBase_detect(SIMDBase_MODE_SSE2_DOUBLE) == 1) return SIMDBase_MODE_SSE2_DOUBLE;
    if (SIMDBase_detect(SIMDBase_MODE_PUREC_DOUBLE) == 1) return SIMDBase_MODE_PUREC_DOUBLE;
    break;

  case SIMDBase_TYPE_LONGDOUBLE:
    if (SIMDBase_detect(SIMDBase_MODE_PUREC_LONGDOUBLE) == 1) return SIMDBase_MODE_PUREC_LONGDOUBLE;
    break;

  case SIMDBase_TYPE_EXTENDED:
    break;

  case SIMDBase_TYPE_QUAD:
    break;
  }

  return SIMDBase_MODE_NONE;
}

int32_t SIMDBase_getModeParamInt(int32_t paramId, int32_t mode) {
  switch(mode) {
#if defined(ENABLE_PUREC_FLOAT)
  case 1: return getModeParamInt_purec_float(paramId); break;
#endif
#if defined(ENABLE_PUREC_DOUBLE)
  case 2: return getModeParamInt_purec_double(paramId); break;
#endif
#if defined(ENABLE_PUREC_LONGDOUBLE)
  case 3: return getModeParamInt_purec_longdouble(paramId); break;
#endif
#if defined(ENABLE_SSE_FLOAT)
  case 4: return getModeParamInt_sse_float(paramId); break;
#endif
#if defined(ENABLE_SSE2_DOUBLE)
  case 5: return getModeParamInt_sse2_double(paramId); break;
#endif
#if defined(ENABLE_NEON_FLOAT)
  case 6: return getModeParamInt_neon_float(paramId); break;
#endif
#if defined(ENABLE_AVX_FLOAT)
  case 7: return getModeParamInt_avx_float(paramId); break;
#endif
#if defined(ENABLE_AVX_DOUBLE)
  case 8: return getModeParamInt_avx_double(paramId); break;
#endif
#if defined(ENABLE_ALTIVEC_FLOAT)
  case 9: return getModeParamInt_altivec_float(paramId); break;
#endif
  }

  return -1;
}

char *SIMDBase_getModeParamString(int32_t paramId, int32_t mode) {
  switch(mode) {
#if defined(ENABLE_PUREC_FLOAT)
  case 1: return getModeParamString_purec_float(paramId); break;
#endif
#if defined(ENABLE_PUREC_DOUBLE)
  case 2: return getModeParamString_purec_double(paramId); break;
#endif
#if defined(ENABLE_PUREC_LONGDOUBLE)
  case 3: return getModeParamString_purec_longdouble(paramId); break;
#endif
#if defined(ENABLE_SSE_FLOAT)
  case 4: return getModeParamString_sse_float(paramId); break;
#endif
#if defined(ENABLE_SSE2_DOUBLE)
  case 5: return getModeParamString_sse2_double(paramId); break;
#endif
#if defined(ENABLE_NEON_FLOAT)
  case 6: return getModeParamString_neon_float(paramId); break;
#endif
#if defined(ENABLE_AVX_FLOAT)
  case 7: return getModeParamString_avx_float(paramId); break;
#endif
#if defined(ENABLE_AVX_DOUBLE)
  case 8: return getModeParamString_avx_double(paramId); break;
#endif
#if defined(ENABLE_ALTIVEC_FLOAT)
  case 9: return getModeParamString_altivec_float(paramId); break;
#endif
  }

  return NULL;
}

#ifdef ANDROID
int posix_memalign (void **memptr, size_t alignment, size_t size) {
    *memptr = malloc (size);
    return *memptr ? 0 : -1;
}
#endif

void *SIMDBase_alignedMalloc(uint64_t size) {
  void *p;
  if (posix_memalign(&p, SIMDBase_sizeOfCachelineInByte(), size) != 0) abort();
  return p;
}

void SIMDBase_alignedFree(void *ptr) {
  free(ptr);
}

int32_t SIMDBase_getParamInt(int32_t paramId) {
  switch(paramId) {
  case SIMDBase_PARAMID_MODE_MAX:
    return SIMDBase_LAST_MODE + 1;
  }

  return -1;
}

int32_t SIMDBase_getTypeParamInt(int32_t paramId, int32_t typeId) {
  switch(typeId) {
  }

  return -1;
}
