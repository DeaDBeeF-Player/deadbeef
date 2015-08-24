/**********************************************************************************************
 *
 *   Ensoniq ES5505/6 driver
 *   by Aaron Giles
 *
 **********************************************************************************************/

#pragma once

#ifndef __ES5506_H__
#define __ES5506_H__

//#include "devlegcy.h"

/*typedef struct _es5505_interface es5505_interface;
struct _es5505_interface
{
	const char * region0;						// memory region where the sample ROM lives
	const char * region1;						// memory region where the sample ROM lives
	void (*irq_callback)(device_t *device, int state);	// irq callback
	UINT16 (*read_port)(device_t *device);			// input port read
};*

READ16_DEVICE_HANDLER( es5505_r );
WRITE16_DEVICE_HANDLER( es5505_w );
void es5505_voice_bank_w(device_t *device, int voice, int bank);
void es5505_set_channel_volume(device_t *device, int channel, int volume);

//DECLARE_LEGACY_SOUND_DEVICE(ES5505, es5505);


typedef struct _es5506_interface es5506_interface;
struct _es5506_interface
{
	const char * region0;						// memory region where the sample ROM lives
	const char * region1;						// memory region where the sample ROM lives
	const char * region2;						// memory region where the sample ROM lives
	const char * region3;						// memory region where the sample ROM lives
	void (*irq_callback)(device_t *device, int state);	// irq callback
	UINT16 (*read_port)(device_t *device);			// input port read
};*/

//READ8_DEVICE_HANDLER( es5506_r );
//WRITE8_DEVICE_HANDLER( es5506_w );
UINT8 es550x_r(void *chip, offs_t offset);
void es550x_w(void *chip, offs_t offset, UINT8 data);
void es550x_w16(void *chip, offs_t offset, UINT16 data);

void es5506_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_es5506(void **chip, int clock, int channels);
void device_stop_es5506(void *chip);
void device_reset_es5506(void *chip);
//void es5506_set_base(running_device *device, UINT8 *wavemem);

void es5506_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					   const UINT8* ROMData);

void es5506_set_mute_mask(void *chip, UINT32 MuteMask);
void es5506_set_srchg_cb(void *chip, SRATE_CALLBACK CallbackFunc, void* DataPtr);

//void es5506_voice_bank_w(device_t *device, int voice, int bank);

//DECLARE_LEGACY_SOUND_DEVICE(ES5506, es5506);

#endif /* __ES5506_H__ */
