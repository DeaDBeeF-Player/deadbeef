#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <complex.h>

#include "SIMDBase.h"
#include "DFT.h"

#if 1
typedef float REAL;
#define TYPE SIMDBase_TYPE_FLOAT
#else
typedef double REAL;
#define TYPE SIMDBase_TYPE_DOUBLE
#endif

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

void backward(double complex *fs, double complex *ts, int len) {
  int k, n;

  for(k=0;k<len;k++) {
    ts[k] = 0;

    for(n=0;n<len;n++) {
      ts[k] += fs[n] * omega(-len, n*k);
    }
  }
}

// complex forward
int check_cf(int n, int mode, int veclen, int sizeOfVect) {
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

  //

  SIMDBase_alignedFree(sx);
  DFT_dispose(p, mode);

  //

  return success;
}

// complex backward
int check_cb(int n, int mode, int veclen, int sizeOfVect) {
  int i,j;

  DFT *p = DFT_init(mode, n, 0);
  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);

  //

  double complex fs[veclen][n], ts[veclen][n];

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      fs[j][i] = (random() / (double)RAND_MAX) + (random() / (double)RAND_MAX) * _Complex_I;

      sx[(i*2+0)*veclen+j] = creal(fs[j][i]);
      sx[(i*2+1)*veclen+j] = cimag(fs[j][i]);
    }
  }

  //

  DFT_execute(p, mode, sx, 1);

  for(j=0;j<veclen;j++) {
    backward(fs[j], ts[j], n);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if ((fabs(sx[(i*2+0)*veclen+j] - creal(ts[j][i])) > THRES) ||
	  (fabs(sx[(i*2+1)*veclen+j] - cimag(ts[j][i])) > THRES)) {
	success = 0;
      }
    }
  }

  //

  SIMDBase_alignedFree(sx);
  DFT_dispose(p, mode);

  //

  return success;
}

// real forward
int check_rf(int n, int mode, int veclen, int sizeOfVect) {
  int i,j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_REAL);
  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n);

  //

  double complex ts[veclen][n], fs[veclen][n];

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      ts[j][i] = (random() / (double)RAND_MAX);
      sx[i*veclen+j] = creal(ts[j][i]);
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
    for(i=0;i<n/2;i++) {
      if (i == 0) {
	if (fabs(sx[(2*0+0) * veclen + j] - creal(fs[j][0  ])) > THRES) success = 0;
	if (fabs(sx[(2*0+1) * veclen + j] - creal(fs[j][n/2])) > THRES) success = 0;
      } else {
	if (fabs(sx[(2*i+0) * veclen + j] - creal(fs[j][i])) > THRES) success = 0;
	if (fabs(sx[(2*i+1) * veclen + j] - cimag(fs[j][i])) > THRES) success = 0;
      }
    }
  }

  //

  SIMDBase_alignedFree(sx);
  DFT_dispose(p, mode);

  //

  return success;
}

// real backward
int check_rb(int n, int mode, int veclen, int sizeOfVect) {
  int i,j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_REAL);
  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n);

  //

  double complex fs[veclen][n], ts[veclen][n];

  for(j=0;j<veclen;j++) {
    for(i=0;i<n/2;i++) {
      if (i == 0) {
	fs[j][0  ] = (random() / (double)RAND_MAX);
	fs[j][n/2] = (random() / (double)RAND_MAX);
      } else {
	fs[j][i  ] = (random() / (double)RAND_MAX) + (random() / (double)RAND_MAX) * _Complex_I;
	fs[j][n-i] = conj(fs[j][i]);
      }
    }
  }

  for(j=0;j<veclen;j++) {
    for(i=0;i<n/2;i++) {
      if (i == 0) {
	sx[(2*0+0) * veclen + j] = creal(fs[j][0  ]);
	sx[(2*0+1) * veclen + j] = creal(fs[j][n/2]);
      } else {
	sx[(2*i+0) * veclen + j] = creal(fs[j][i]);
	sx[(2*i+1) * veclen + j] = cimag(fs[j][i]);
      }
    }
  }

  //

  for(j=0;j<veclen;j++) {
    backward(fs[j], ts[j], n);
  }

  DFT_execute(p, mode, sx, 1);

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if (fabs(cimag(ts[j][i])) > THRES) {
	success = 0;
      }

      if ((fabs(sx[i * veclen + j]*2 - creal(ts[j][i])) > THRES)) {
	success = 0;
      }
    }
  }

  //

  SIMDBase_alignedFree(sx);
  DFT_dispose(p, mode);

  //

  return success;
}

// alt real forward
int check_arf(int n, int mode, int veclen, int sizeOfVect) {
  int i,j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_ALT_REAL);
  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n);

  //

  double complex ts[veclen][n], fs[veclen][n];

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      ts[j][i] = (random() / (double)RAND_MAX);
      sx[i*veclen+j] = creal(ts[j][i]);
    }
  }

  //

  DFT_execute(p, mode, sx, 1);

  for(j=0;j<veclen;j++) {
    backward(ts[j], fs[j], n);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n/2;i++) {
      if (i == 0) {
	if (fabs(sx[(2*0+0) * veclen + j] - creal(fs[j][0  ])) > THRES) success = 0;
	if (fabs(sx[(2*0+1) * veclen + j] - creal(fs[j][n/2])) > THRES) success = 0;
      } else {
	if (fabs(sx[(2*i+0) * veclen + j] - creal(fs[j][i])) > THRES) success = 0;
	if (fabs(sx[(2*i+1) * veclen + j] - cimag(fs[j][i])) > THRES) success = 0;
      }
    }
  }

  //

  SIMDBase_alignedFree(sx);
  DFT_dispose(p, mode);

  //

  return success;
}

// alt real backward
int check_arb(int n, int mode, int veclen, int sizeOfVect) {
  int i,j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_ALT_REAL);
  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n);

  //

  double complex fs[veclen][n], ts[veclen][n];

  for(j=0;j<veclen;j++) {
    for(i=0;i<n/2;i++) {
      if (i == 0) {
	fs[j][0  ] = (random() / (double)RAND_MAX);
	fs[j][n/2] = (random() / (double)RAND_MAX);
      } else {
	fs[j][i  ] = (random() / (double)RAND_MAX) + (random() / (double)RAND_MAX) * _Complex_I;
	fs[j][n-i] = conj(fs[j][i]);
      }
    }
  }

  for(j=0;j<veclen;j++) {
    for(i=0;i<n/2;i++) {
      if (i == 0) {
	sx[(2*0+0) * veclen + j] = creal(fs[j][0  ]);
	sx[(2*0+1) * veclen + j] = creal(fs[j][n/2]);
      } else {
	sx[(2*i+0) * veclen + j] = creal(fs[j][i]);
	sx[(2*i+1) * veclen + j] = cimag(fs[j][i]);
      }
    }
  }

  //

  for(j=0;j<veclen;j++) {
    forward(fs[j], ts[j], n);
  }

  DFT_execute(p, mode, sx, -1);

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if (fabs(cimag(ts[j][i])) > THRES) {
	success = 0;
      }

      if ((fabs(sx[i * veclen + j]*2 - creal(ts[j][i])) > THRES)) {
	success = 0;
      }
    }
  }

  //

  SIMDBase_alignedFree(sx);
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
  printf("alt real forward  : %s\n", check_arf(n, mode, veclen, sizeOfVect) ? "OK" : "NG");
  printf("alt real backward : %s\n", check_arb(n, mode, veclen, sizeOfVect) ? "OK" : "NG");

  exit(0);
}
