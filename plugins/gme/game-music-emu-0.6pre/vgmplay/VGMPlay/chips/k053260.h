/*********************************************************

    Konami 053260 PCM/ADPCM Sound Chip

*********************************************************/

#pragma once

//#include "devlegcy.h"

/*typedef struct _k053260_interface k053260_interface;
struct _k053260_interface {
	const char *rgnoverride;
	timer_expired_func irq;			// called on SH1 complete cycle ( clock / 32 ) //
};*/


void k053260_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_k053260(void **chip, int clock);
void device_stop_k053260(void *chip);
void device_reset_k053260(void *chip);

//WRITE8_DEVICE_HANDLER( k053260_w );
//READ8_DEVICE_HANDLER( k053260_r );
void k053260_w(void *chip, offs_t offset, UINT8 data);
UINT8 k053260_r(void *chip, offs_t offset);

void k053260_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					   const UINT8* ROMData);
void k053260_set_mute_mask(void *chip, UINT32 MuteMask);

//DECLARE_LEGACY_SOUND_DEVICE(K053260, k053260);
