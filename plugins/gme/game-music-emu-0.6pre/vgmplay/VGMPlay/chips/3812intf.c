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
#include <string.h>

#include <stdlib.h>
#include "mamedef.h"
//#include "attotime.h"
//#include "sndintrf.h"
//#include "streams.h"
//#include "cpuintrf.h"
#include "3812intf.h"
#ifdef ENABLE_ALL_CORES
#include "fmopl.h"
#endif

#define OPLTYPE_IS_OPL2
#include "adlibemu.h"

#ifndef NULL
#define NULL	((void *)0)
#endif


#define EC_DBOPL	0x00	// DosBox OPL (AdLibEmu)
#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01	// YM3826 core from MAME
#endif

typedef struct _ym3812_state ym3812_state;
struct _ym3812_state
{
	//sound_stream *	stream;
	//emu_timer *		timer[2];
	void *			chip;
	int			EMU_CORE;
	//const ym3812_interface *intf;
	//const device_config *device;
};


/*INLINE ym3812_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_YM3812);
	return (ym3812_state *)device->token;
}*/


#ifdef ENABLE_ALL_CORES
static void IRQHandler(void *param,int irq)
{
	//ym3812_state *info = (ym3812_state *)param;
	//if (info->intf->handler) (info->intf->handler)(info->device, irq ? ASSERT_LINE : CLEAR_LINE);
	//if (info->intf->handler) (info->intf->handler)(irq ? ASSERT_LINE : CLEAR_LINE);
}
#endif

/*static TIMER_CALLBACK( timer_callback_0 )
{
	ym3812_state *info = (ym3812_state *)ptr;
	ym3812_timer_over(info->chip,0);
}

static TIMER_CALLBACK( timer_callback_1 )
{
	ym3812_state *info = (ym3812_state *)ptr;
	ym3812_timer_over(info->chip,1);
}*/

//static void TimerHandler(void *param,int c,attotime period)
#ifdef ENABLE_ALL_CORES
static void TimerHandler(void *param,int c,int period)
{
	//ym3812_state *info = (ym3812_state *)param;
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
#endif


//static STREAM_UPDATE( ym3812_stream_update )
void ym3812_stream_update(void *param, stream_sample_t **outputs, int samples)
{
	ym3812_state *info = (ym3812_state *)param;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym3812_update_one(info->chip, outputs, samples);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL2_getsample(info->chip, outputs, samples);
		break;
	}
}

static stream_sample_t* DUMMYBUF[0x02] = {NULL, NULL};

static void _stream_update(void * param/*, int interval*/)
{
	ym3812_state *info = (ym3812_state *)param;
	//stream_update(info->stream);
	
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym3812_update_one(info->chip, DUMMYBUF, 0);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL2_getsample(info->chip, DUMMYBUF, 0);
		break;
	}
}


//static DEVICE_START( ym3812 )
int device_start_ym3812(void **_info, int EMU_CORE, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	//static const ym3812_interface dummy = { 0 };
	//ym3812_state *info = get_safe_token(device);
	ym3812_state *info;
	int rate;

#ifdef ENABLE_ALL_CORES
	if (EMU_CORE >= 0x02)
		EMU_CORE = EC_DBOPL;
#else
	EMU_CORE = EC_DBOPL;
#endif
	
	info = (ym3812_state *) calloc(1, sizeof(ym3812_state));
	*_info = (void *) info;
	
	info->EMU_CORE = EMU_CORE;
	rate = (clock & 0x7FFFFFFF)/72;
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	//info->intf = device->static_config ? (const ym3812_interface *)device->static_config : &dummy;
	//info->intf = &dummy;
	//info->device = device;

	/* stream system initialize */
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		info->chip = ym3812_init(clock & 0x7FFFFFFF,rate);
		//assert_always(info->chip != NULL, "Error creating YM3812 chip");

		//info->stream = stream_create(device,0,1,rate,info,ym3812_stream_update);

		/* YM3812 setup */
		ym3812_set_timer_handler (info->chip, TimerHandler, info);
		ym3812_set_irq_handler   (info->chip, IRQHandler, info);
		ym3812_set_update_handler(info->chip, _stream_update, info);

		//info->timer[0] = timer_alloc(device->machine, timer_callback_0, info);
		//info->timer[1] = timer_alloc(device->machine, timer_callback_1, info);
		break;
#endif
	case EC_DBOPL:
		info->chip = adlib_OPL2_init(clock & 0x7FFFFFFF, rate, _stream_update, info);
		break;
	}

	return rate;
}

//static DEVICE_STOP( ym3812 )
void device_stop_ym3812(void *_info)
{
	//ym3812_state *info = get_safe_token(device);
	ym3812_state *info = (ym3812_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym3812_shutdown(info->chip);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL2_stop(info->chip);
		break;
	}
	free(info);
}

//static DEVICE_RESET( ym3812 )
void device_reset_ym3812(void *_info)
{
	//ym3812_state *info = get_safe_token(device);
	ym3812_state *info = (ym3812_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym3812_reset_chip(info->chip);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL2_reset(info->chip);
		break;
	}
}


//READ8_DEVICE_HANDLER( ym3812_r )
UINT8 ym3812_r(void *_info, offs_t offset)
{
	//ym3812_state *info = get_safe_token(device);
	ym3812_state *info = (ym3812_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		return ym3812_read(info->chip, offset & 1);
#endif
	case EC_DBOPL:
		return adlib_OPL2_reg_read(info->chip, offset & 0x01);
	default:
		return 0x00;
	}
}

//WRITE8_DEVICE_HANDLER( ym3812_w )
void ym3812_w(void *_info, offs_t offset, UINT8 data)
{
	//ym3812_state *info = get_safe_token(device);
	ym3812_state *info = (ym3812_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym3812_write(info->chip, offset & 1, data);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL2_writeIO(info->chip, offset & 1, data);
		break;
	}
}

//READ8_DEVICE_HANDLER( ym3812_status_port_r )
UINT8 ym3812_status_port_r(void *info, offs_t offset)
{
	return ym3812_r(info, 0);
}
//READ8_DEVICE_HANDLER( ym3812_read_port_r )
UINT8 ym3812_read_port_r(void *info, offs_t offset)
{
	return ym3812_r(info, 1);
}
//WRITE8_DEVICE_HANDLER( ym3812_control_port_w )
void ym3812_control_port_w(void *info, offs_t offset, UINT8 data)
{
	ym3812_w(info, 0, data);
}
//WRITE8_DEVICE_HANDLER( ym3812_write_port_w )
void ym3812_write_port_w(void *info, offs_t offset, UINT8 data)
{
	ym3812_w(info, 1, data);
}


void ym3812_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ym3812_state *info = (ym3812_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		opl_set_mute_mask(info->chip, MuteMask);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL2_set_mute_mask(info->chip, MuteMask);
		break;
	}
	
	return;
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( ym3812 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ym3812_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ym3812 );				break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( ym3812 );				break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( ym3812 );				break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YM3812");							break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");						break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/
