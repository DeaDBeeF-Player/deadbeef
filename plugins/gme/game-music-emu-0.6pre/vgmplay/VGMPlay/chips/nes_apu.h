/*****************************************************************************

  MAME/MESS NES APU CORE

  Based on the Nofrendo/Nosefart NES N2A03 sound emulation core written by
  Matthew Conte (matt@conte.com) and redesigned for use in MAME/MESS by
  Who Wants to Know? (wwtk@mail.com)

  This core is written with the advise and consent of Matthew Conte and is
  released under the GNU Public License.  This core is freely avaiable for
  use in any freeware project, subject to the following terms:

  Any modifications to this code must be duly noted in the source and
  approved by Matthew Conte and myself prior to public submission.

 *****************************************************************************

   NES_APU.H

   NES APU external interface.

 *****************************************************************************/

#pragma once

#ifndef __NES_APU_H__
#define __NES_APU_H__

//#include "devlegcy.h"


/* AN EXPLANATION
 *
 * The NES APU is actually integrated into the Nintendo processor.
 * You must supply the same number of APUs as you do processors.
 * Also make sure to correspond the memory regions to those used in the
 * processor, as each is shared.
 */

/*typedef struct _nes_interface nes_interface;
struct _nes_interface
{
	const char *cpu_tag;  // CPU tag
};

READ8_DEVICE_HANDLER( nes_psg_r );
WRITE8_DEVICE_HANDLER( nes_psg_w );

DECLARE_LEGACY_SOUND_DEVICE(NES, nesapu);*/

UINT8 nes_psg_r(void* chip, offs_t offset);
void nes_psg_w(void* chip, offs_t offset, UINT8 data);

void nes_psg_update_sound(void* chip, stream_sample_t **outputs, int samples);
void* device_start_nesapu(int clock, int rate);
void device_stop_nesapu(void* chip);
void device_reset_nesapu(void* chip);

void nesapu_set_rom(void* chip, const UINT8* ROMData);

void nesapu_set_mute_mask(void* chip, UINT32 MuteMask);

#endif /* __NES_APU_H__ */
