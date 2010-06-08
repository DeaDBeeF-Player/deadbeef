#ifndef _EMU8950_H_
#define _EMU8950_H_

#include "emuadpcm.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EMU8950_DLL_EXPORTS
  #define EMU8950_API __declspec(dllexport)
#elif defined(EMU8950_DLL_IMPORTS)
  #define EMU8950_API __declspec(dllimport)
#else
  #define EMU8950_API
#endif

#define PI 3.14159265358979323846

#include "emutypes.h"

/* voice data */
typedef struct __OPL_PATCH {
  e_uint32 TL,FB,EG,ML,AR,DR,SL,RR,KR,KL,AM,PM,WF ;
} OPL_PATCH ;

/* slot */
typedef struct __OPL_SLOT {

  e_int32 type ;          /* 0 : modulator 1 : carrier */

  /* OUTPUT */
  e_int32 feedback ;
  e_int32 output[5] ;      /* Output value of slot */

  /* for Phase Generator (PG) */
  e_uint32 *sintbl ;    /* Wavetable */
  e_uint32 phase ;      /* Phase */
  e_uint32 dphase ;     /* Phase increment amount */
  e_uint32 pgout ;      /* output */

  /* for Envelope Generator (EG) */
  e_int32 fnum ;          /* F-Number */
  e_int32 block ;         /* Block */
  e_uint32 tll ;	      /* Total Level + Key scale level*/
  e_uint32 rks ;        /* Key scale offset (Rks) */
  e_int32 eg_mode ;       /* Current state */
  e_uint32 eg_phase ;   /* Phase */
  e_uint32 eg_dphase ;  /* Phase increment amount */
  e_uint32 egout ;      /* output */

  /* LFO (refer to OPL->*) */
  e_int32 *plfo_am ;
  e_int32 *plfo_pm ;

  OPL_PATCH *patch;  

} OPL_SLOT ;

/* Channel */
typedef struct __OPL_CH {

  e_int32 key_status ;
  e_int32 alg ;
  OPL_SLOT *mod, *car ;

} OPL_CH ;

/* OPL */
typedef struct __OPL {

  e_uint32 realstep ;
  e_uint32 opltime ;
  e_uint32 oplstep ;

  ADPCM *adpcm;

  e_uint32 adr ;

  e_int32 out ;

  /* Register */
  unsigned char reg[0xff] ; 
  e_int32 slot_on_flag[18] ;

  /* Rythm Mode : 0 = OFF, 1 = ON */
  e_int32 rhythm_mode ;

  /* Pitch Modulator */
  e_int32 pm_mode ;
  e_uint32 pm_phase ;

  /* Amp Modulator */
  e_int32 am_mode ;
  e_uint32 am_phase ;

  /* Noise Generator */
  e_uint32 noise_seed ;

  /* Channel & Slot */
  OPL_CH *ch[9] ;
  OPL_SLOT *slot[18] ;

  e_uint32 mask ;

} OPL ;

/* Mask */
#define OPL_MASK_CH(x) (1<<(x))
#define OPL_MASK_HH (1<<9)
#define OPL_MASK_CYM (1<<10)
#define OPL_MASK_TOM (1<<11)
#define OPL_MASK_SD (1<<12)
#define OPL_MASK_BD (1<<13)
#define OPL_MASK_RHYTHM ( OPLL_MASK_HH | OPLL_MASK_CYM | OPLL_MASK_TOM | OPLL_MASK_SD | OPLL_MASK_BD )
#define OPL_MASK_PCM (1<<14)

EMU8950_API OPL *OPL_new(e_uint32 clk, e_uint32 rate) ;
EMU8950_API void OPL_set_rate(OPL *opl, e_uint32 rate) ;
EMU8950_API void OPL_reset(OPL *opl) ;
EMU8950_API void OPL_delete(OPL *opl) ;
EMU8950_API void OPL_writeReg(OPL *opl, e_uint32 reg, e_uint32 val) ;
EMU8950_API e_int16 OPL_calc(OPL *opl) ;
EMU8950_API void OPL_writeIO(OPL *opl, e_uint32 adr, e_uint32 val) ;
EMU8950_API e_uint32 OPL_readIO(OPL *opl) ;
EMU8950_API e_uint32 OPL_status(OPL *opl) ;
EMU8950_API e_uint32 OPL_setMask(OPL *opl, e_uint32 mask);
EMU8950_API e_uint32 OPL_toggleMask(OPL *opl, e_uint32 mask);

#ifdef __cplusplus
}
#endif



#endif











