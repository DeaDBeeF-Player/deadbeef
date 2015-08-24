#ifndef __VB_VSU_H
#define __VB_VSU_H

void VSU_Write(void *chip, UINT32 A, UINT8 V);

void vsu_stream_update(void *chip, stream_sample_t **outputs, int samples);

int device_start_vsu(void **chip, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_vsu(void *chip);
void device_reset_vsu(void *chip);

//void vsu_set_options(UINT16 Options);
void vsu_set_mute_mask(void *chip, UINT32 MuteMask);

#endif	// __VB_VSU_H
