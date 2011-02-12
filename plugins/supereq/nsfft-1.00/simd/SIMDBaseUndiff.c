#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "SIMDBase.h"
#include "SIMDBaseUndiff.h"

void SIMDBaseUndiff_DETECT() {
  extern uint8_t detectBuffer[256];
  SIMDBase_VECT a = SIMDBase_LOAD((SIMDBase_VECT *)&detectBuffer[0]);
  SIMDBase_VECT b = SIMDBase_LOAD((SIMDBase_VECT *)&detectBuffer[64]);
  SIMDBase_VECT c = SIMDBase_ADDi(a, b);
  SIMDBase_STOR((SIMDBase_VECT *)&detectBuffer[128], c);
}

int32_t SIMDBaseUndiff_GETMODEPARAMINT(int32_t paramId) {
  switch(paramId) {
  case SIMDBase_PARAMID_SIZE_OF_REAL:
    return sizeof(SIMDBase_REAL);
  case SIMDBase_PARAMID_SIZE_OF_VECT:
    return sizeof(SIMDBase_VECT);
  case SIMDBase_PARAMID_VECTOR_LEN:
    return SIMDBase_VECTLEN;
  case SIMDBase_PARAMID_MODE_AVAILABILITY:
    return SIMDBase_detect(paramId);
  }

  return -1;
}

char * SIMDBaseUndiff_GETMODEPARAMSTRING(int32_t paramId) {
  switch(paramId) {
  case SIMDBase_PARAMID_MODE_NAME:
    return SIMDBase_NAME;
  }

  return NULL;
}
