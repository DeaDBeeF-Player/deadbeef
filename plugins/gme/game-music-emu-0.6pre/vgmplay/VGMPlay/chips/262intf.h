#pragma once


/*typedef struct _ymf262_interface ymf262_interface;
struct _ymf262_interface
{
	//void (*handler)(const device_config *device, int irq);
	void (*handler)(int irq);
};*/


/*READ8_DEVICE_HANDLER( ymf262_r );
WRITE8_DEVICE_HANDLER( ymf262_w );

READ8_DEVICE_HANDLER ( ymf262_status_r );
WRITE8_DEVICE_HANDLER( ymf262_register_a_w );
WRITE8_DEVICE_HANDLER( ymf262_register_b_w );
WRITE8_DEVICE_HANDLER( ymf262_data_a_w );
WRITE8_DEVICE_HANDLER( ymf262_data_b_w );


DEVICE_GET_INFO( ymf262 );
#define SOUND_YMF262 DEVICE_GET_INFO_NAME( ymf262 )*/

void ymf262_stream_update(void *chip, stream_sample_t **outputs, int samples);

int device_start_ymf262(void **chip, int core, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_ymf262(void *chip);
void device_reset_ymf262(void *chip);

UINT8 ymf262_r(void *chip, offs_t offset);
void ymf262_w(void *chip, offs_t offset, UINT8 data);

UINT8 ymf262_status_r(void *chip, offs_t offset);
void ymf262_register_a_w(void *chip, offs_t offset, UINT8 data);
void ymf262_register_b_w(void *chip, offs_t offset, UINT8 data);
void ymf262_data_a_w(void *chip, offs_t offset, UINT8 data);
void ymf262_data_b_w(void *chip, offs_t offset, UINT8 data);

void ymf262_set_mute_mask(void *chip, UINT32 MuteMask);

