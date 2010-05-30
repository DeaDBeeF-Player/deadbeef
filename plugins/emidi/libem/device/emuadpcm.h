#ifndef _EMUADPCM_H_
#define _EMUADPCM_H_

#include "emutypes.h"

#ifdef EMU8950_ADPCM_DLL_EXPORTS
  #define EMU8950_ADCPM_API __declspec(dllexport)
#elif  EMU8950_ADPCM_DLL_IMPORTS
  #define EMU8950_ADPCM_API __declspec(dllimport)
#else
  #define EMU8950_ADPCM_API
#endif

typedef struct __ADPCM
{
  e_uint32 clk, rate;

  e_uint8 reg[0x20];

  e_uint8 *wave; /* ADPCM DATA */
  e_uint8 *memory[2]; /* [0] RAM, [1] ROM */

  e_uint8 status; /* STATUS Registar */

  e_uint32 start_addr;
  e_uint32 stop_addr;
  e_uint32 play_addr; /* Current play address * 2 */
  e_uint32 delta_addr; /* 16bit address */
  e_uint32 delta_n;
  e_uint32 play_addr_mask;

  e_uint32 play_start;

  e_int32 output[2];
  e_uint32 diff;

  void *timer1_user_data ;
  void *timer2_user_data ;
  void *(*timer1_func)(void *user) ;
  void *(*timer2_func)(void *user) ;

} ADPCM ;

EMU8950_ADPCM_API ADPCM *ADPCM_new(e_uint32 clk, e_uint32 rate);
EMU8950_ADPCM_API void ADPCM_set_rate(ADPCM *, e_uint32 rate);
EMU8950_ADPCM_API void ADPCM_reset(ADPCM *);
EMU8950_ADPCM_API void ADPCM_delete(ADPCM *);
EMU8950_ADPCM_API void ADPCM_writeReg(ADPCM *, e_uint32 reg, e_uint32 val);
EMU8950_ADPCM_API e_int16 ADPCM_calc(ADPCM *);
EMU8950_ADPCM_API e_uint32 ADPCM_status(ADPCM *);

#endif