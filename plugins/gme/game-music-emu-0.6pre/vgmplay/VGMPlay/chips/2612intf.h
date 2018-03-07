#pragma once

void ym2612_update_request(void *param);

/*typedef struct _ym2612_interface ym2612_interface;
struct _ym2612_interface
{
	//void (*handler)(const device_config *device, int irq);
	void (*handler)(int irq);
};*/

/*READ8_DEVICE_HANDLER( ym2612_r );
WRITE8_DEVICE_HANDLER( ym2612_w );

READ8_DEVICE_HANDLER( ym2612_status_port_a_r );
READ8_DEVICE_HANDLER( ym2612_status_port_b_r );
READ8_DEVICE_HANDLER( ym2612_data_port_a_r );
READ8_DEVICE_HANDLER( ym2612_data_port_b_r );

WRITE8_DEVICE_HANDLER( ym2612_control_port_a_w );
WRITE8_DEVICE_HANDLER( ym2612_control_port_b_w );
WRITE8_DEVICE_HANDLER( ym2612_data_port_a_w );
WRITE8_DEVICE_HANDLER( ym2612_data_port_b_w );


DEVICE_GET_INFO( ym2612 );
#define SOUND_YM2612 DEVICE_GET_INFO_NAME( ym2612 )*/
void ym2612_stream_update(void *chip, stream_sample_t **outputs, int samples);
int device_start_ym2612(void **chip, int core, int options, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE, UINT8 * IsVGMInit);
void device_stop_ym2612(void *chip);
void device_reset_ym2612(void *chip);

UINT8 ym2612_r(void *chip, offs_t offset);
void ym2612_w(void *chip, offs_t offset, UINT8 data);

UINT8 ym2612_status_port_a_r(void *chip, offs_t offset);
UINT8 ym2612_status_port_b_r(void *chip, offs_t offset);
UINT8 ym2612_data_port_a_r(void *chip, offs_t offset);
UINT8 ym2612_data_port_b_r(void *chip, offs_t offset);

void ym2612_control_port_a_w(void *chip, offs_t offset, UINT8 data);
void ym2612_control_port_b_w(void *chip, offs_t offset, UINT8 data);
void ym2612_data_port_a_w(void *chip, offs_t offset, UINT8 data);
void ym2612_data_port_b_w(void *chip, offs_t offset, UINT8 data);

void ym2612_set_mute_mask(void *chip, UINT32 MuteMask);


/*typedef struct _ym3438_interface ym3438_interface;
struct _ym3438_interface
{
	//void (*handler)(const device_config *device, int irq);
	void (*handler)(int irq);
};


#define ym3438_r				ym2612_r
#define ym3438_w				ym2612_w

#define ym3438_status_port_a_r	ym2612_status_port_a_r
#define ym3438_status_port_b_r	ym2612_status_port_b_r
#define ym3438_data_port_a_r	ym2612_data_port_a_r
#define ym3438_data_port_b_r	ym2612_data_port_b_r

#define ym3438_control_port_a_w	ym2612_control_port_a_w
#define ym3438_control_port_b_w	ym2612_control_port_b_w
#define ym3438_data_port_a_w	ym2612_data_port_a_w
#define ym3438_data_port_b_w	ym2612_data_port_b_w*/


//DEVICE_GET_INFO( ym3438 );
//#define SOUND_YM3438 DEVICE_GET_INFO_NAME( ym3438 )
