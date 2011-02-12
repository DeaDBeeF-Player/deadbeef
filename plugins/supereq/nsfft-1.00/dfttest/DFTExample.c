#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <complex.h>

#include "SIMDBase.h"
#include "DFT.h"

typedef float REAL;
#define TYPE SIMDBase_TYPE_FLOAT

#define THRES 1e-3

double complex omega(double n, double kn) {
  return cexp((-2 * M_PI * _Complex_I / n) * kn);
}

void forward(double complex *ts, double complex *fs, int len) {
  int k, n;

  for(k=0;k<len;k++) {
    fs[k] = 0;

    for(n=0;n<len;n++) {
      fs[k] += ts[n] * omega(len, n*k);
    }
  }
}

int main(int argc, char **argv) {
  const int n = 256;

  int mode = SIMDBase_chooseBestMode(TYPE);
  printf("mode : %d, %s\n", mode, SIMDBase_getModeParamString(SIMDBase_PARAMID_MODE_NAME, mode));

  int veclen = SIMDBase_getModeParamInt(SIMDBase_PARAMID_VECTOR_LEN, mode);
  int sizeOfVect = SIMDBase_getModeParamInt(SIMDBase_PARAMID_SIZE_OF_VECT, mode);

  //

  int i, j;

  DFT *p = DFT_init(mode, n, 0);
  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);

  //

  double complex ts[veclen][n], fs[veclen][n];

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      ts[j][i] = (random() / (double)RAND_MAX) + (random() / (double)RAND_MAX) * _Complex_I;
      sx[(i*2+0)*veclen+j] = creal(ts[j][i]);
      sx[(i*2+1)*veclen+j] = cimag(ts[j][i]);
    }
  }

  //

  DFT_execute(p, mode, sx, -1);

  for(j=0;j<veclen;j++) {
    forward(ts[j], fs[j], n);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if ((fabs(sx[(i*2+0)*veclen+j] - creal(fs[j][i])) > THRES) ||
	  (fabs(sx[(i*2+1)*veclen+j] - cimag(fs[j][i])) > THRES)) {
	success = 0;
      }
    }
  }

  printf("%s\n", success ? "OK" : "NG");

  //

  SIMDBase_alignedFree(sx);
  DFT_dispose(p, mode);

  exit(0);
}
