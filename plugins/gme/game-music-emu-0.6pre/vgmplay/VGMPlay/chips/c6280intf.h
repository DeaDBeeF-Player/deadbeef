#pragma once

void c6280_w(void *chip, offs_t offset, UINT8 data);
UINT8 c6280_r(void *chip, offs_t offset);

void c6280_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_c6280(void **chip, int core, int clock, int samplerate, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_c6280(void *chip);
void device_reset_c6280(void *chip);

void c6280_set_mute_mask(void *chip, UINT32 MuteMask);
