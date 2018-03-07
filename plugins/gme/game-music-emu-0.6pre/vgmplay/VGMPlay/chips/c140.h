/* C140.h */

#pragma once

void c140_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_c140(void **chip, int clock, int banking_type, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_c140(void *chip);
void device_reset_c140(void *chip);

//READ8_DEVICE_HANDLER( c140_r );
//WRITE8_DEVICE_HANDLER( c140_w );
UINT8 c140_r(void *chip, offs_t offset);
void c140_w(void *chip, offs_t offset, UINT8 data);

//void c140_set_base(device_t *device, void *base);
void c140_set_base(void *chip, void *base);

enum
{
	C140_TYPE_SYSTEM2,
	C140_TYPE_SYSTEM21,
	C140_TYPE_ASIC219
};

/*typedef struct _c140_interface c140_interface;
struct _c140_interface {
    int banking_type;
};*/


void c140_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					const UINT8* ROMData);

void c140_set_mute_mask(void *chip, UINT32 MuteMask);

//DECLARE_LEGACY_SOUND_DEVICE(C140, c140);
