#pragma once

void ayxx_stream_update(void *chip, stream_sample_t **outputs, int samples);

int device_start_ayxx(void **chip, int core, int clock, UINT8 chip_type, UINT8 Flags, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_ayxx(void *chip);
void device_reset_ayxx(void *chip);

void ayxx_w(void *chip, offs_t offset, UINT8 data);

void ayxx_set_mute_mask(void *chip, UINT32 MuteMask);
