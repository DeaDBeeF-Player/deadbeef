void daccontrol_update(void *chip, UINT32 samples);
UINT8 device_start_daccontrol(void **chip, void *param, int samplerate);
void device_stop_daccontrol(void *chip);
void device_reset_daccontrol(void *chip);
void daccontrol_setup_chip(void *chip, UINT8 ChType, UINT8 ChNum, UINT16 Command);
void daccontrol_set_data(void *chip, UINT8* Data, UINT32 DataLen, UINT8 StepSize, UINT8 StepBase);
void daccontrol_refresh_data(void *chip, UINT8* Data, UINT32 DataLen);
void daccontrol_set_frequency(void *chip, UINT32 Frequency);
void daccontrol_start(void *chip, UINT32 DataPos, UINT8 LenMode, UINT32 Length);
void daccontrol_stop(void *chip);

#define DCTRL_LMODE_IGNORE	0x00
#define DCTRL_LMODE_CMDS	0x01
#define DCTRL_LMODE_MSEC	0x02
#define DCTRL_LMODE_TOEND	0x03
#define DCTRL_LMODE_BYTES	0x0F
