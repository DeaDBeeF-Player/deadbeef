/*********************************************************

    Capcom Q-Sound system

*********************************************************/

#pragma once

//#include "devlegcy.h"

#define QSOUND_CLOCK    4000000   /* default 4MHz clock */

void qsound_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_qsound(void **chip, int clock);
void device_stop_qsound(void *chip);
void device_reset_qsound(void *chip);


//WRITE8_DEVICE_HANDLER( qsound_w );
//READ8_DEVICE_HANDLER( qsound_r );
void qsound_w(void *chip, offs_t offset, UINT8 data);
UINT8 qsound_r(void *chip, offs_t offset);


void qsound_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					   const UINT8* ROMData);
void qsound_set_mute_mask(void *chip, UINT32 MuteMask);

//DECLARE_LEGACY_SOUND_DEVICE(QSOUND, qsound);
