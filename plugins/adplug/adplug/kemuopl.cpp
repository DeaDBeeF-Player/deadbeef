#include "kemuopl.h"

CKemuopl::CKemuopl(int rate, bool bit16, bool usestereo)
  : use16bit(bit16), stereo(usestereo), sampleerate(rate), mixbufSamples(0)
{
  memset (ctx, 0, sizeof (ctx));
  currType = TYPE_DUAL_OPL2;
  init();
};

CKemuopl::~CKemuopl()
{
  if(mixbufSamples) {
    delete [] mixbuf0;
    delete [] mixbuf1;
    delete [] mixbuf2;
  }
}

void CKemuopl::update(short *buf, int samples)
{
  int i;
  //ensure that our mix buffers are adequately sized
  if(mixbufSamples < samples) {
    if(mixbufSamples) { delete[] mixbuf0; delete[] mixbuf1; delete[] mixbuf2; }
    mixbufSamples = samples;

    //*2 = make room for stereo, if we need it
    mixbuf0 = new short[samples*2];
    mixbuf1 = new short[samples*2];
    mixbuf2 = new short[samples*2];
  }

  short *outbuf = use16bit ? buf : mixbuf2;

  //render each chip to a different tempbuffer
  adlibgetsample(&ctx[0], (unsigned char *)mixbuf0, samples * 2 /* 16-bit */);
  adlibgetsample(&ctx[1], (unsigned char *)mixbuf1, samples * 2 /* 16-bit */);

  //output stereo:
  //then we need to interleave the two buffers
  if(stereo){
    //left channel
    for(i=0;i<samples;i++)
      outbuf[i*2] = mixbuf0[i];
    //right channel
    for(i=0;i<samples;i++)
      outbuf[i*2+1] = mixbuf1[i];
    } else
      //mono, then we need to mix the two buffers into buf
      for(i=0;i<samples;i++)
        outbuf[i] = (mixbuf0[i]>>1) + (mixbuf1[i]>>1);

  //now reduce to 8bit if we need to
  if(!use16bit)
    for(i=0;i<(stereo ? samples*2 : samples);i++)
      ((char *)buf)[i] = (outbuf[i] >> 8) ^ 0x80;
}

void CKemuopl::write(int reg, int val)
{
  adlib0(&ctx[currChip], reg, val);
};

void CKemuopl::init()
{
  adlibinit(&ctx[0], sampleerate, /* mono */ 1, /* 16-bit */ 2);
  adlibinit(&ctx[1], sampleerate, /* mono */ 1, /* 16-bit */ 2);
  currChip = 0;
};
