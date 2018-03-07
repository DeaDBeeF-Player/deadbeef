#pragma once

#define YMF278B_STD_CLOCK (33868800)			/* standard clock for OPL4 */


typedef struct _ymf278b_interface ymf278b_interface;
struct _ymf278b_interface
{
	//void (*irq_callback)(const device_config *device, int state);	/* irq callback */
	void (*irq_callback)(int state);	/* irq callback */
};

/*READ8_DEVICE_HANDLER( ymf278b_r );
WRITE8_DEVICE_HANDLER( ymf278b_w );

DEVICE_GET_INFO( ymf278b );
#define SOUND_YMF278B DEVICE_GET_INFO_NAME( ymf278b )*/

void ymf278b_pcm_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_ymf278b(void **chip, int clock);
void device_stop_ymf278b(void *chip);
void device_reset_ymf278b(void *chip);

UINT8 ymf278b_r(void *chip, offs_t offset);
void ymf278b_w(void *chip, offs_t offset, UINT8 data);
void ymf278b_write_rom(void *chip, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					   const UINT8* ROMData);

void ymf278b_set_mute_mask(void *chip, UINT32 MuteMaskFM, UINT32 MuteMaskWT);

