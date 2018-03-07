
/* Custom Sound Interface */
UINT8 gb_wave_r(void *chip, offs_t offset);
void gb_wave_w(void *chip, offs_t offset, UINT8 data);
UINT8 gb_sound_r(void *chip, offs_t offset);
void gb_sound_w(void *chip, offs_t offset, UINT8 data);

void gameboy_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_gameboy_sound(void **chip, int clock, int flags, int samplerate);
void device_stop_gameboy_sound(void *chip);
void device_reset_gameboy_sound(void *chip);

void gameboy_sound_set_mute_mask(void *chip, UINT32 MuteMask);
