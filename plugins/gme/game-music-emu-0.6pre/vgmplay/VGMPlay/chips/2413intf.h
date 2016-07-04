#pragma once

/*WRITE8_DEVICE_HANDLER( ym2413_w );

WRITE8_DEVICE_HANDLER( ym2413_register_port_w );
WRITE8_DEVICE_HANDLER( ym2413_data_port_w );

DEVICE_GET_INFO( ym2413 );
#define SOUND_YM2413 DEVICE_GET_INFO_NAME( ym2413 )*/

void ym2413_stream_update(void *chip, stream_sample_t **outputs, int samples);

int device_start_ym2413(void **chip, int core, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_ym2413(void *chip);
void device_reset_ym2413(void *chip);

void ym2413_w(void *chip, offs_t offset, UINT8 data);
void ym2413_register_port_w(void *chip, offs_t offset, UINT8 data);
void ym2413_data_port_w(void *chip, offs_t offset, UINT8 data);

void ym2413_set_mute_mask(void *chip, UINT32 MuteMask);
void ym2413_set_panning(void *chip, INT16* PanVals);
