#ifndef _SIMDBase_H_
#define _SIMDBase_H_

#include <stdint.h>

#define SIMDBase_TYPE_FLOAT ( 1 | ( 1 << 24 ))
#define SIMDBase_TYPE_DOUBLE ( 2 | ( 1 << 24 ))
#define SIMDBase_TYPE_LONGDOUBLE ( 3 | ( 1 << 24 ))
#define SIMDBase_TYPE_EXTENDED ( 4 | ( 1 << 24 ))
#define SIMDBase_TYPE_QUAD ( 5 | ( 1 << 24 ))
#define SIMDBase_TYPE_HALF ( 6 | ( 1 << 24 ))

#define SIMDBase_MODE_NONE 0
#define SIMDBase_MODE_PUREC_FLOAT 1
#define SIMDBase_MODE_PUREC_DOUBLE 2
#define SIMDBase_MODE_PUREC_LONGDOUBLE 3
#define SIMDBase_MODE_SSE_FLOAT 4
#define SIMDBase_MODE_SSE2_DOUBLE 5
#define SIMDBase_MODE_NEON_FLOAT 6
#define SIMDBase_MODE_AVX_FLOAT 7
#define SIMDBase_MODE_AVX_DOUBLE 8
#define SIMDBase_MODE_ALTIVEC_FLOAT 9

#define SIMDBase_LAST_MODE SIMDBase_MODE_ALTIVEC_FLOAT

#define SIMDBase_PARAMID_MODE_MAX ( 1 | ( 2 << 24 ))
#define SIMDBase_PARAMID_TYPE_AVAILABILITY ( 2 | ( 2 << 24 ))
#define SIMDBase_PARAMID_SIZE_OF_REAL ( 3 | ( 2 << 24 ))
#define SIMDBase_PARAMID_SIZE_OF_VECT ( 4 | ( 2 << 24 ))
#define SIMDBase_PARAMID_VECTOR_LEN ( 5 | ( 2 << 24 ))
#define SIMDBase_PARAMID_MODE_AVAILABILITY ( 6 | ( 2 << 24 ))
#define SIMDBase_PARAMID_MODE_NAME ( 7 | ( 2 << 24 ))

//

typedef struct {
  uint32_t linesize;
  uint32_t size[8], assoc[8];
} CacheParam;

void *SIMDBase_alignedMalloc(uint64_t size);
void SIMDBase_alignedFree(void *ptr);
int32_t SIMDBase_sizeOfCachelineInByte();
int32_t SIMDBase_sizeOfDataCacheInByte();
int32_t SIMDBase_chooseBestMode(int32_t typeId);
char *SIMDBase_getProcessorNameString();
int32_t SIMDBase_detect(int32_t paramId);
int32_t SIMDBase_getParamInt(int32_t paramId);
int32_t SIMDBase_getTypeParamInt(int32_t paramId, int32_t typeId);
int32_t SIMDBase_getModeParamInt(int32_t paramId, int32_t mode);
char *SIMDBase_getModeParamString(int32_t paramId, int32_t mode);

#endif
