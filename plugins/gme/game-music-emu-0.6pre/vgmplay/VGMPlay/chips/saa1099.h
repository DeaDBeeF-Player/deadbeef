#pragma once

#ifndef __SAA1099_H__
#define __SAA1099_H__

/**********************************************
    Philips SAA1099 Sound driver
**********************************************/

//WRITE8_DEVICE_HANDLER( saa1099_control_w );
void saa1099_control_w(void *chip, offs_t offset, UINT8 data);
//WRITE8_DEVICE_HANDLER( saa1099_data_w );
void saa1099_data_w(void *chip, offs_t offset, UINT8 data);

//DECLARE_LEGACY_SOUND_DEVICE(SAA1099, saa1099);
void saa1099_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_saa1099(void **chip, int clock);
void device_stop_saa1099(void *chip);
void device_reset_saa1099(void *chip);

void saa1099_set_mute_mask(void *chip, UINT32 MuteMask);

#endif /* __SAA1099_H__ */
