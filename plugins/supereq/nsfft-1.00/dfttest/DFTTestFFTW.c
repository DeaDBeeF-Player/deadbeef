#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <complex.h>

#include <fftw3.h>

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

// complex forward
int check_cf(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, 0);
  fftw_plan w[n];

  fftw_complex *in[sizeOfVect], *out[sizeOfVect];

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);

  //

  for(j=0;j<veclen;j++) {
    in[j] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
    out[j] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
    w[j] = fftw_plan_dft_1d(n, in[j], out[j], FFTW_FORWARD, FFTW_ESTIMATE);

    for(i=0;i<n;i++) {
      double re = random() / (double)RAND_MAX;
      double im = random() / (double)RAND_MAX;
      sx[(i*2+0)*veclen+j] = re;
      sx[(i*2+1)*veclen+j] = im;
      in[j][i] = re + im * _Complex_I;
    }
  }

  //

  DFT_execute(p, mode, sx, -1);

  for(j=0;j<veclen;j++) {
    fftw_execute(w[j]);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if (fabs(sx[(i*2+0)*veclen+j] - creal(out[j][i])) > THRES) success = 0;
      if (fabs(sx[(i*2+1)*veclen+j] - cimag(out[j][i])) > THRES) success = 0;
    }
  }

  //

  for(j=0;j<veclen;j++) {
    fftw_destroy_plan(w[j]);
    fftw_free(in[j]);
    fftw_free(out[j]);
  }

  SIMDBase_alignedFree(sx);

  DFT_dispose(p, mode);

  //

  return success;
}

// complex backward
int check_cb(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, 0);
  fftw_plan w[n];

  fftw_complex *in[sizeOfVect], *out[sizeOfVect];

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);

  //

  for(j=0;j<veclen;j++) {
    in[j] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
    out[j] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
    w[j] = fftw_plan_dft_1d(n, in[j], out[j], FFTW_BACKWARD, FFTW_ESTIMATE);

    for(i=0;i<n;i++) {
      double re = random() / (double)RAND_MAX;
      double im = random() / (double)RAND_MAX;
      sx[(i*2+0)*veclen+j] = re;
      sx[(i*2+1)*veclen+j] = im;
      in[j][i] = re + im * _Complex_I;
    }
  }

  //

  DFT_execute(p, mode, sx, 1);

  for(j=0;j<veclen;j++) {
    fftw_execute(w[j]);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n;i++) {
      if (fabs(sx[(i*2+0)*veclen+j] - creal(out[j][i])) > THRES) success = 0;
      if (fabs(sx[(i*2+1)*veclen+j] - cimag(out[j][i])) > THRES) success = 0;
    }
  }

  //

  for(j=0;j<veclen;j++) {
    fftw_destroy_plan(w[j]);
    fftw_free(in[j]);
    fftw_free(out[j]);
  }

  SIMDBase_alignedFree(sx);

  DFT_dispose(p, mode);

  //

  return success;
}

// real forward
int check_rf(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_REAL);
  fftw_plan w[n];

  double *in[sizeOfVect];
  fftw_complex *out[sizeOfVect];

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);

  //

  for(j=0;j<veclen;j++) {
    in[j] = (double *) fftw_malloc(sizeof(double) * n);
    out[j] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (n/2+1));
    w[j] = fftw_plan_dft_r2c_1d(n, in[j], out[j], FFTW_ESTIMATE);

    for(i=0;i<n;i++) {
      double re = random() / (double)RAND_MAX;
      sx[i*veclen+j] = re;
      in[j][i] = re;
    }
  }

  //

  DFT_execute(p, mode, sx, -1);

  for(j=0;j<veclen;j++) {
    fftw_execute(w[j]);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n/2;i++) {
      if (i == 0) {
	if (fabs(sx[(i*2+0)*veclen+j] - creal(out[j][0])) > THRES) success = 0;
	if (fabs(sx[(i*2+1)*veclen+j] - creal(out[j][n/2])) > THRES) success = 0;
      } else {
	if (fabs(sx[(i*2+0)*veclen+j] - creal(out[j][i])) > THRES) success = 0;
	if (fabs(sx[(i*2+1)*veclen+j] - cimag(out[j][i])) > THRES) success = 0;
      }
    }
  }

  //

  for(j=0;j<veclen;j++) {
    fftw_destroy_plan(w[j]);
    fftw_free(in[j]);
    fftw_free(out[j]);
  }

  SIMDBase_alignedFree(sx);

  DFT_dispose(p, mode);

  //

  return success;
}

// real backward
int check_rb(int n, int mode, int veclen, int sizeOfVect) {
  int i, j;

  DFT *p = DFT_init(mode, n, DFT_FLAG_REAL);
  fftw_plan w[n];

  fftw_complex *in[sizeOfVect];
  double *out[sizeOfVect];

  REAL *sx = SIMDBase_alignedMalloc(sizeOfVect*n*2);

  //

  for(j=0;j<veclen;j++) {
    in[j] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (n/2+1));
    out[j] = (double *) fftw_malloc(sizeof(double) * n);
    w[j] = fftw_plan_dft_c2r_1d(n, in[j], out[j], FFTW_ESTIMATE);

    for(i=0;i<n/2;i++) {
      if (i == 0) {
	in[j][0  ] = (random() / (double)RAND_MAX);
	in[j][n/2] = (random() / (double)RAND_MAX);
      } else {
	in[j][i  ] = (random() / (double)RAND_MAX) + (random() / (double)RAND_MAX) * _Complex_I;
      }
    }

    for(i=0;i<n/2;i++) {
      if (i == 0) {
	sx[(2*0+0) * veclen + j] = creal(in[j][0  ]);
	sx[(2*0+1) * veclen + j] = creal(in[j][n/2]);
      } else {
	sx[(2*i+0) * veclen + j] = creal(in[j][i]);
	sx[(2*i+1) * veclen + j] = cimag(in[j][i]);
      }
    }
  }

  //

  DFT_execute(p, mode, sx, 1);

  for(j=0;j<veclen;j++) {
    fftw_execute(w[j]);
  }

  //

  int success = 1;

  for(j=0;j<veclen;j++) {
    for(i=0;i<n/2;i++) {
      if ((fabs(sx[i * veclen + j]*2 - out[j][i]) > THRES)) {
	success = 0;
      }
    }
  }

  //

  for(j=0;j<veclen;j++) {
    fftw_destroy_plan(w[j]);
    fftw_free(in[j]);
    fftw_free(out[j]);
  }

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

  exit(0);
}
