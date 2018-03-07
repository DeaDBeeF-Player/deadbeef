/******************************************************************************
* FILE
*   Yamaha 3812 emulator interface - MAME VERSION
*
* CREATED BY
*   Ernesto Corvi
*
* UPDATE LOG
*   JB  28-04-2002  Fixed simultaneous usage of all three different chip types.
*                       Used real sample rate when resample filter is active.
*       AAT 12-28-2001  Protected Y8950 from accessing unmapped port and keyboard handlers.
*   CHS 1999-01-09  Fixes new ym3812 emulation interface.
*   CHS 1998-10-23  Mame streaming sound chip update
*   EC  1998        Created Interface
*
* NOTES
*
******************************************************************************/
#include "mamedef.h"
#include <stdlib.h>
//#include "attotime.h"
//#include "sndintrf.h"
//#include "streams.h"
//#include "cpuintrf.h"
#include "8950intf.h"
//#include "fm.h"
#include "fmopl.h"

#include <string.h>

#ifndef NULL
#define NULL	((void *)0)
#endif


typedef struct _y8950_state y8950_state;
struct _y8950_state
{
	//sound_stream *	stream;
	//emu_timer *		timer[2];
	void *			chip;
	//const y8950_interface *intf;
	//const device_config *device;
};


/*INLINE y8950_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_Y8950);
	return (y8950_state *)device->token;
}*/


static void IRQHandler(void *param,int irq)
{
	//y8950_state *info = (y8950_state *)param;
	//if (info->intf->handler) (info->intf->handler)(info->device, irq ? ASSERT_LINE : CLEAR_LINE);
	//if (info->intf->handler) (info->intf->handler)(irq ? ASSERT_LINE : CLEAR_LINE);
}
/*static TIMER_CALLBACK( timer_callback_0 )
{
	y8950_state *info = (y8950_state *)ptr;
	y8950_timer_over(info->chip,0);
}
static TIMER_CALLBACK( timer_callback_1 )
{
	y8950_state *info = (y8950_state *)ptr;
	y8950_timer_over(info->chip,1);
}*/
//static void TimerHandler(void *param,int c,attotime period)
static void TimerHandler(void *param,int c,int period)
{
	//y8950_state *info = (y8950_state *)param;
	//if( attotime_compare(period, attotime_zero) == 0 )
	if( period == 0 )
	{	/* Reset FM Timer */
		//timer_enable(info->timer[c], 0);
	}
	else
	{	/* Start FM Timer */
		//timer_adjust_oneshot(info->timer[c], period, 0);
	}
}


static unsigned char Y8950PortHandler_r(void *param)
{
	//y8950_state *info = (y8950_state *)param;
	/*if (info->intf->portread)
		return info->intf->portread(0);*/
	return 0;
}

static void Y8950PortHandler_w(void *param,unsigned char data)
{
	//y8950_state *info = (y8950_state *)param;
	/*if (info->intf->portwrite)
		info->intf->portwrite(0,data);*/
}

static unsigned char Y8950KeyboardHandler_r(void *param)
{
	//y8950_state *info = (y8950_state *)param;
	/*if (info->intf->keyboardread)
		return info->intf->keyboardread(0);*/
	return 0;
}

static void Y8950KeyboardHandler_w(void *param,unsigned char data)
{
	//y8950_state *info = (y8950_state *)param;
	/*if (info->intf->keyboardwrite)
		info->intf->keyboardwrite(0,data);*/
}

//static STREAM_UPDATE( y8950_stream_update )
void y8950_stream_update(void *param, stream_sample_t **outputs, int samples)
{
	y8950_state *info = (y8950_state *)param;
	y8950_update_one(info->chip, outputs, samples);
}

static stream_sample_t* DUMMYBUF[0x02] = {NULL, NULL};

static void _stream_update(void *param/*, int interval*/)
{
	y8950_state *info = (y8950_state *)param;
	//stream_update(info->stream);
	
	y8950_update_one(info->chip, DUMMYBUF, 0);
}


//static DEVICE_START( y8950 )
int device_start_y8950(void **_info, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	//static const y8950_interface dummy = { 0 };
	//y8950_state *info = get_safe_token(device);
	y8950_state *info;
	int rate;
	
	info = (y8950_state *) calloc(1, sizeof(y8950_state));
	*_info = (void *) info;	

	rate = clock/72;
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	//info->intf = device->static_config ? (const y8950_interface *)device->static_config : &dummy;
	//info->intf = &dummy;
	//info->device = device;

	/* stream system initialize */
	info->chip = y8950_init(clock,rate);
	//assert_always(info->chip != NULL, "Error creating Y8950 chip");

	/* ADPCM ROM data */
	//y8950_set_delta_t_memory(info->chip, device->region, device->regionbytes);
	y8950_set_delta_t_memory(info->chip, NULL, 0x00);

	//info->stream = stream_create(device,0,1,rate,info,y8950_stream_update);

	/* port and keyboard handler */
	y8950_set_port_handler(info->chip, Y8950PortHandler_w, Y8950PortHandler_r, info);
	y8950_set_keyboard_handler(info->chip, Y8950KeyboardHandler_w, Y8950KeyboardHandler_r, info);

	/* Y8950 setup */
	y8950_set_timer_handler (info->chip, TimerHandler, info);
	y8950_set_irq_handler   (info->chip, IRQHandler, info);
	y8950_set_update_handler(info->chip, _stream_update, info);

	//info->timer[0] = timer_alloc(device->machine, timer_callback_0, info);
	//info->timer[1] = timer_alloc(device->machine, timer_callback_1, info);

	return rate;
}

//static DEVICE_STOP( y8950 )
void device_stop_y8950(void *_info)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state *info = (y8950_state *)_info;
	y8950_shutdown(info->chip);
	free(info);
}

//static DEVICE_RESET( y8950 )
void device_reset_y8950(void *_info)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state *info = (y8950_state *)_info;
	y8950_reset_chip(info->chip);
}


//READ8_DEVICE_HANDLER( y8950_r )
UINT8 y8950_r(void *_info, offs_t offset)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state *info = (y8950_state *)_info;
	return y8950_read(info->chip, offset & 1);
}

//WRITE8_DEVICE_HANDLER( y8950_w )
void y8950_w(void *_info, offs_t offset, UINT8 data)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state *info = (y8950_state *)_info;
	y8950_write(info->chip, offset & 1, data);
}

//READ8_DEVICE_HANDLER( y8950_status_port_r )
UINT8 y8950_status_port_r(void *info, offs_t offset)
{
	return y8950_r(info, 0);
}
//READ8_DEVICE_HANDLER( y8950_read_port_r )
UINT8 y8950_read_port_r(void *info, offs_t offset)
{
	return y8950_r(info, 1);
}
//WRITE8_DEVICE_HANDLER( y8950_control_port_w )
void y8950_control_port_w(void *info, offs_t offset, UINT8 data)
{
	y8950_w(info, 0, data);
}
//WRITE8_DEVICE_HANDLER( y8950_write_port_w )
void y8950_write_port_w(void *info, offs_t offset, UINT8 data)
{
	y8950_w(info, 1, data);
}


void y8950_write_data_pcmrom(void *_info, offs_t ROMSize, offs_t DataStart,
							  offs_t DataLength, const UINT8* ROMData)
{
	y8950_state* info = (y8950_state *)_info;
	
	y8950_write_pcmrom(info->chip, ROMSize, DataStart, DataLength, ROMData);
	
	return;
}

void y8950_set_mute_mask(void *_info, UINT32 MuteMask)
{
	y8950_state *info = (y8950_state *)_info;
	opl_set_mute_mask(info->chip, MuteMask);
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( y8950 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(y8950_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( y8950 );				break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( y8950 );				break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( y8950 );				break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "Y8950");							break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");						break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/
