#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

#include "SIMDBase.h"
#include "DFT.h"

void cdft(int, int, double *, int *, double *);
void rdft(int, int, double *, int *, double *);

#if 1
typedef float REAL;
#define TYPE SIMDBase_TYPE_FLOAT
#else
typedef double REAL;
#define TYPE SIMDBase_TYPE_DOUBLE
#endif

#define THRES 1e-3

// complex forward
int check_cf(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, 0);

  int *ip = calloc(n, sizeof(int));
  double *trigTable = SIMDBase_alignedMalloc(sizeof(double)*n/2);

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);
  double *sy = SIMDBase_alignedMalloc(veclen * sizeof(double) *n*2);

  //

  for(j=0;j<veclen;j++) {
    for(i=0;i<n*2;i++) {
      sx[i*veclen + j] = random() / (double)RAND_MAX;
      sy[j*n*2 + i] = sx[i*veclen + j];
    }
  }

  //

  DFT_execute(p, mode, sx, -1);

  for(j=0;j<veclen;j++) {
    cdft(n*2, -1, &sy[j*n*2], ip, trigTable);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n*2;i++) {
      if (fabs(sx[i*veclen+j] - sy[j*n*2 + i]) > THRES) success = 0;
    }
  }

  //

  SIMDBase_alignedFree(sy);
  SIMDBase_alignedFree(sx);
  SIMDBase_alignedFree(trigTable);
  free(ip);

  DFT_dispose(p, mode);

  //

  return success;
}

// complex backward
int check_cb(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, 0);

  int *ip = calloc(n, sizeof(int));
  double *trigTable = SIMDBase_alignedMalloc(sizeof(double)*n/2);

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);
  double *sy = SIMDBase_alignedMalloc(veclen * sizeof(double) *n*2);

  //

  for(j=0;j<veclen;j++) {
    for(i=0;i<n*2;i++) {
      sx[i*veclen + j] = random() / (double)RAND_MAX;
      sy[j*n*2 + i] = sx[i*veclen + j];
    }
  }

  //

  DFT_execute(p, mode, sx, 1);

  for(j=0;j<veclen;j++) {
    cdft(n*2, 1, &sy[j*n*2], ip, trigTable);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n*2;i++) {
      if (fabs(sx[i*veclen+j] - sy[j*n*2 + i]) > THRES) success = 0;
    }
  }

  //

  SIMDBase_alignedFree(sy);
  SIMDBase_alignedFree(sx);
  SIMDBase_alignedFree(trigTable);
  free(ip);

  DFT_dispose(p, mode);

  //

  return success;
}

// real forward
int check_rf(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_ALT_REAL);

  int *ip = calloc(n, sizeof(int));
  double *trigTable = SIMDBase_alignedMalloc(sizeof(double)*n/2);

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n);
  double *sy = SIMDBase_alignedMalloc(veclen * sizeof(double) *n);

  //

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      sx[i*veclen + j] = random() / (double)RAND_MAX;
      sy[j*n + i] = sx[i*veclen + j];
    }
  }

  //

  DFT_execute(p, mode, sx, -1);

  for(j=0;j<veclen;j++) {
    rdft(n, -1, &sy[j*n], ip, trigTable);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if (fabs(sx[i*veclen+j] - sy[j*n + i]) > THRES) success = 0;
    }
  }

  //

  SIMDBase_alignedFree(sy);
  SIMDBase_alignedFree(sx);
  SIMDBase_alignedFree(trigTable);
  free(ip);

  DFT_dispose(p, mode);

  //

  return success;
}

// real backward
int check_rb(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_ALT_REAL);

  int *ip = calloc(n, sizeof(int));
  double *trigTable = SIMDBase_alignedMalloc(sizeof(double)*n/2);

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n);
  double *sy = SIMDBase_alignedMalloc(veclen * sizeof(double) *n);

  //

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      sx[i*veclen + j] = random() / (double)RAND_MAX;
      sy[j*n + i] = sx[i*veclen + j];
    }
  }

  //

  DFT_execute(p, mode, sx, 1);

  for(j=0;j<veclen;j++) {
    rdft(n, 1, &sy[j*n], ip, trigTable);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if (fabs(sx[i*veclen+j] - sy[j*n + i]) > THRES) success = 0;
    }
  }

  //

  SIMDBase_alignedFree(sy);
  SIMDBase_alignedFree(sx);
  SIMDBase_alignedFree(trigTable);
  free(ip);

  DFT_dispose(p, mode);

  //

  return success;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s <log2n>\n", argv[0]);
    exit(-1);
  }

  const int n = 1 << atoi(argv[1]);

  srandom(time(NULL));

  //

  int mode = SIMDBase_chooseBestMode(TYPE);

  printf("mode : %d, %s\n", mode, SIMDBase_getModeParamString(SIMDBase_PARAMID_MODE_NAME, mode));

  int veclen = SIMDBase_getModeParamInt(SIMDBase_PARAMID_VECTOR_LEN, mode);
  int sizeOfVect = SIMDBase_getModeParamInt(SIMDBase_PARAMID_SIZE_OF_VECT, mode);

  printf("complex forward   : %s\n", check_cf(n, mode, veclen, sizeOfVect) ? "OK" : "NG");
  printf("complex backward  : %s\n", check_cb(n, mode, veclen, sizeOfVect) ? "OK" : "NG");
  printf("real forward      : %s\n", check_rf(n, mode, veclen, sizeOfVect) ? "OK" : "NG");
  printf("real backward     : %s\n", check_rb(n, mode, veclen, sizeOfVect) ? "OK" : "NG");

  exit(0);
}
