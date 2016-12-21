/////////////////////////////////////////////////////////////////////////////
//
// yam - Emulates Yamaha SCSP and AICA
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SEGA_YAM_H__
#define __SEGA_YAM_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////

// version = 1 for SCSP, 2 for AICA
// ramsize must be a power of 2

sint32 EMU_CALL yam_init(void);
uint32 EMU_CALL yam_get_state_size(uint8 version);
void   EMU_CALL yam_clear_state(void *state, uint8 version);

void   EMU_CALL yam_enable_dry(void *state, uint8 enable);
void   EMU_CALL yam_enable_dsp(void *state, uint8 enable);
void   EMU_CALL yam_enable_dsp_dynarec(void *state, uint8 enable);

void   EMU_CALL yam_setram(void *state, uint32 *ram, uint32 size, uint8 mbx, uint8 mwx);
void   EMU_CALL yam_beginbuffer(void *state, sint16 *buf);
void   EMU_CALL yam_advance(void *state, uint32 samples);
void   EMU_CALL yam_flush(void *state);

uint32 EMU_CALL yam_aica_load_reg(void *state, uint32 a, uint32 mask);
void   EMU_CALL yam_aica_store_reg(void *state, uint32 a, uint32 d, uint32 mask, uint8 *breakcpu);

uint32 EMU_CALL yam_scsp_load_reg(void *state, uint32 a, uint32 mask);
void   EMU_CALL yam_scsp_store_reg(void *state, uint32 a, uint32 d, uint32 mask, uint8 *breakcpu);

uint8* EMU_CALL yam_get_interrupt_pending_ptr(void *state);
uint32 EMU_CALL yam_get_min_samples_until_interrupt(void *state);

void   EMU_CALL yam_prepare_dynacode(void *state);
void   EMU_CALL yam_unprepare_dynacode(void *state);

void   EMU_CALL yam_set_mute(void *state, uint32 channel, uint32 enable);

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif
