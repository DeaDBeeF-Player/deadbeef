#ifndef __WS_AUDIO_H__
#define __WS_AUDIO_H__

int ws_audio_init(void **chip, int clock, int samplerate, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void ws_audio_reset(void *chip);
void ws_audio_done(void *chip);
void ws_audio_update(void *chip, stream_sample_t** buffer, int length);
void ws_audio_port_write(void *chip, UINT8 port, UINT8 value);
UINT8 ws_audio_port_read(void *chip, UINT8 port);
//void ws_audio_process(void);
//void ws_audio_sounddma(void);
void ws_write_ram(void *chip, UINT16 offset, UINT8 value);
void ws_set_mute_mask(void *chip, UINT32 MuteMask);
//extern int WaveAdrs;

#endif
