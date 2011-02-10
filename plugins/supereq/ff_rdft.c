#include <stdint.h>
#include <complex.h>
#include "libavcodec/avfft.h"
#include "libavutil/avutil.h"

void rfft(int n,int isign,float *x)
{
  static int wsize=0;
  static float *w = NULL;
  static RDFTContext *s = NULL;
  static RDFTContext *si = NULL;
  int newwsize;

  if (n == 0) {
      if (w) {
          av_free(w);
          w  = NULL;
          wsize  = 0;
      }
      if (s) {
          av_rdft_end (s);
          s = NULL;
      }
      if (si) {
          av_rdft_end (si);
          si = NULL;
      }
    return;
  }

  newwsize = n/2;
  if (newwsize > wsize) {
    wsize = newwsize;
    if (s) {
        av_rdft_end (s);
        s = NULL;
    }
      if (si) {
          av_rdft_end (si);
          si = NULL;
      }
    if (w) {
        av_free (w);
        w = NULL;
    }
    w = (float *)av_malloc(sizeof(float)*wsize);
  }

  if (!s) {
      s = av_rdft_init(n,DFT_R2C);
  }
  if (!si) {
      si = av_rdft_init(n,IDFT_C2R);
  }

  if (isign == 1) {
      av_rdft_calc (s, x);
  }
  else {
      av_rdft_calc (si, x);
  }
}

