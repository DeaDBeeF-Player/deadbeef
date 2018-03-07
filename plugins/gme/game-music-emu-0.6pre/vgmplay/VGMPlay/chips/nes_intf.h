#pragma once

void nes_stream_update(void *chip, stream_sample_t **outputs, int samples);

int device_start_nes(void **chip, int core, int clock, int options, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_nes(void *chip);
void device_reset_nes(void *chip);

void nes_w(void *chip, offs_t offset, UINT8 data);

void nes_write_ram(void *chip, offs_t DataStart, offs_t DataLength, const UINT8* RAMData);

void nes_set_mute_mask(void *chip, UINT32 MuteMask);
