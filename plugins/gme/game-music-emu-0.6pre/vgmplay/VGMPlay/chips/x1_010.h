#pragma once

#ifndef __X1_010_H__
#define __X1_010_H__

//#include "devlegcy.h"


/*typedef struct _x1_010_interface x1_010_interface;
struct _x1_010_interface
{
	int adr;	// address
};*/


void seta_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_x1_010(void **chip, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_x1_010(void *chip);
void device_reset_x1_010(void *chip);

//READ8_DEVICE_HANDLER ( seta_sound_r );
//WRITE8_DEVICE_HANDLER( seta_sound_w );
UINT8 seta_sound_r(void *chip, offs_t offset);
void seta_sound_w(void *chip, offs_t offset, UINT8 data);

//READ16_DEVICE_HANDLER ( seta_sound_word_r );
//WRITE16_DEVICE_HANDLER( seta_sound_word_w );

//void seta_sound_enable_w(device_t *device, int data);

void x1_010_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
						const UINT8* ROMData);

void x1_010_set_mute_mask(void *chip, UINT32 MuteMask);

//DECLARE_LEGACY_SOUND_DEVICE(X1_010, x1_010);

#endif /* __X1_010_H__ */
