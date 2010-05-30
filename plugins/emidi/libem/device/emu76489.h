#ifndef _EMU76489_H_
#define _EMU76489_H_
#include "emutypes.h"

#ifdef EMU76489_DLL_EXPORTS
  #define EMU76489_API __declspec(dllexport)
#elif defined(EMU76489_DLL_IMPORTS)
  #define EMU76489_API __declspec(dllimport)
#else
  #define EMU76489_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __SNG {

  e_int32 out ;

  e_uint32 clk, rate, base_incr, quality ;

  e_uint32 count[3] ;
  e_uint32 volume[3] ;
  e_uint32 freq[3] ;
  e_uint32 edge[3] ;
  e_uint32 mute[3] ;

  e_uint32 noise_seed ;
  e_uint32 noise_count ;
  e_uint32 noise_freq ;
  e_uint32 noise_volume ;
  e_uint32 noise_mode ;

  e_uint32 base_count ;

	/* rate converter */
  e_uint32 realstep ;
  e_uint32 sngtime ;
  e_uint32 sngstep ;

  e_uint32 adr ;

  e_uint32 stereo;
  e_uint32 out2;

} SNG ;
	
EMU76489_API SNG *SNG_new(e_uint32 clk, e_uint32 rate) ;
EMU76489_API void SNG_delete(SNG *) ;
EMU76489_API void SNG_reset(SNG *) ;
EMU76489_API void SNG_set_rate(SNG *,e_uint32 rate) ;
EMU76489_API void SNG_set_quality(SNG *,e_uint32 q) ;
EMU76489_API void SNG_writeIO(SNG *SNG, e_uint32 val) ;
EMU76489_API e_uint8 SNG_readReg(SNG *, e_uint32 reg) ;
EMU76489_API e_uint8 SNG_readIO(SNG *SNG) ;
EMU76489_API e_int16 SNG_calc(SNG *) ;
EMU76489_API void SNG_setVolumeMode(SNG *SNG, int type) ;
EMU76489_API void SNG_calc_stereo(SNG *, e_int32 out[2]) ;
EMU76489_API void SNG_writeGGIO(SNG *SNG, e_uint32 val) ;

#ifdef __cplusplus
}
#endif

#endif
