#pragma once

#include "ay8910.h"
#include "emu2149.h"

void ym2203_update_request(void *param);

typedef struct _ym2203_interface ym2203_interface;
struct _ym2203_interface
{
	ay8910_interface ay8910_intf;
	//void (*handler)(const device_config *device, int irq);
	void (*handler)(int irq);
};

/*READ8_DEVICE_HANDLER( ym2203_r );
WRITE8_DEVICE_HANDLER( ym2203_w );

READ8_DEVICE_HANDLER( ym2203_status_port_r );
READ8_DEVICE_HANDLER( ym2203_read_port_r );
WRITE8_DEVICE_HANDLER( ym2203_control_port_w );
WRITE8_DEVICE_HANDLER( ym2203_write_port_w );

DEVICE_GET_INFO( ym2203 );
#define SOUND_YM2203 DEVICE_GET_INFO_NAME( ym2203 )*/

void ym2203_stream_update(void *chip, stream_sample_t **outputs, int samples);
void ym2203_stream_update_ay(void *chip, stream_sample_t **outputs, int samples);

int device_start_ym2203(void **chip, int core, int clock, UINT8 AYDisable, UINT8 AYFlags, int* AYrate, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE);
void device_stop_ym2203(void *chip);
void device_reset_ym2203(void *chip);

UINT8 ym2203_r(void *chip, offs_t offset);
void ym2203_w(void *chip, offs_t offset, UINT8 data);

UINT8 ym2203_status_port_r(void *chip, offs_t offset);
UINT8 ym2203_read_port_r(void *chip, offs_t offset);
void ym2203_control_port_w(void *chip, offs_t offset, UINT8 data);
void ym2203_write_port_w(void *chip, offs_t offset, UINT8 data);

void ym2203_set_mute_mask(void *chip, UINT32 MuteMaskFM, UINT32 MuteMaskAY);
void ym2203_set_srchg_cb(void *chip, SRATE_CALLBACK CallbackFunc, void* DataPtr, void* AYDataPtr);
