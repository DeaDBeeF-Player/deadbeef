#pragma once

//#include "devlegcy.h"

void MultiPCM_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_multipcm(void **chip, int clock);
void device_stop_multipcm(void *chip);
void device_reset_multipcm(void *chip);

//WRITE8_DEVICE_HANDLER( multipcm_w );
//READ8_DEVICE_HANDLER( multipcm_r );
void multipcm_w(void *chip, offs_t offset, UINT8 data);
UINT8 multipcm_r(void *chip, offs_t offset);

void multipcm_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
						const UINT8* ROMData);
//void multipcm_set_bank(running_device *device, UINT32 leftoffs, UINT32 rightoffs);
void multipcm_set_bank(void *chip, UINT32 leftoffs, UINT32 rightoffs);
void multipcm_bank_write(void *chip, UINT8 offset, UINT16 data);

void multipcm_set_mute_mask(void *chip, UINT32 MuteMask);
//DECLARE_LEGACY_SOUND_DEVICE(MULTIPCM, multipcm);
