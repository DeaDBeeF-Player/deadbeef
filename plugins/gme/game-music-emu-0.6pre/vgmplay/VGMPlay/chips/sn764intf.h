#pragma once

/*WRITE8_DEVICE_HANDLER( ym2413_w );

WRITE8_DEVICE_HANDLER( ym2413_register_port_w );
WRITE8_DEVICE_HANDLER( ym2413_data_port_w );

DEVICE_GET_INFO( ym2413 );
#define SOUND_YM2413 DEVICE_GET_INFO_NAME( ym2413 )*/

void sn764xx_stream_update(void *chip, stream_sample_t **outputs, int samples);

int device_start_sn764xx(void **chip, int core, int clock, int samplerate, int shiftregwidth, int noisetaps,
						 int negate, int stereo, int clockdivider, int freq0);
void device_stop_sn764xx(void *chip);
void device_reset_sn764xx(void *chip);

void sn764xx_w(void *chip, offs_t offset, UINT8 data);

void sn764xx_set_mute_mask(void *chip, UINT32 MuteMask);
void sn764xx_set_panning(void *chip, INT16* PanVals);
