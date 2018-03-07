/***************************************************************************

  262intf.c

  MAME interface for YMF262 (OPL3) emulator

***************************************************************************/
#include "mamedef.h"
#include <stdlib.h>
//#include "attotime.h"
//#include "sndintrf.h"
//#include "streams.h"
#include "262intf.h"
#ifdef ENABLE_ALL_CORES
#include "ymf262.h"
#endif

#define OPLTYPE_IS_OPL3
#include "adlibemu.h"


#define EC_DBOPL	0x00	// DosBox OPL (AdLibEmu)
#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01	// YMF262 core from MAME
#endif

typedef struct _ymf262_state ymf262_state;
struct _ymf262_state
{
	//sound_stream *	stream;
	//emu_timer *		timer[2];
	void *			chip;
	int			EMU_CORE;
	//const ymf262_interface *intf;
	//const device_config *device;
};


/*INLINE ymf262_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_YMF262);
	return (ymf262_state *)device->token;
}*/




#ifdef ENABLE_ALL_CORES
static void IRQHandler_262(void *param,int irq)
{
	//ymf262_state *info = (ymf262_state *)param;
	//if (info->intf->handler) (info->intf->handler)(info->device, irq);
}
#endif

/*static TIMER_CALLBACK( timer_callback_262_0 )
{
	ymf262_state *info = (ymf262_state *)ptr;
	ymf262_timer_over(info->chip, 0);
}

static TIMER_CALLBACK( timer_callback_262_1 )
{
	ymf262_state *info = (ymf262_state *)ptr;
	ymf262_timer_over(info->chip, 1);
}*/

//static void timer_handler_262(void *param,int timer, attotime period)
#ifdef ENABLE_ALL_CORES
static void timer_handler_262(void *param,int timer, int period)
{
	//ymf262_state *info = (ymf262_state *)param;
	if( period == 0 )
	{	/* Reset FM Timer */
		//timer_enable(info->timer[timer], 0);
	}
	else
	{	/* Start FM Timer */
		//timer_adjust_oneshot(info->timer[timer], period, 0);
	}
}
#endif

//static STREAM_UPDATE( ymf262_stream_update )
void ymf262_stream_update(void *param, stream_sample_t **outputs, int samples)
{
	ymf262_state *info = (ymf262_state *)param;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ymf262_update_one(info->chip, outputs, samples);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL3_getsample(info->chip, outputs, samples);
		break;
	}
}

static stream_sample_t* DUMMYBUF[0x02] = {NULL, NULL};

static void _stream_update(void *param/*, int interval*/)
{
	ymf262_state *info = (ymf262_state *)param;
	//stream_update(info->stream);
	
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ymf262_update_one(info->chip, DUMMYBUF, 0);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL3_getsample(info->chip, DUMMYBUF, 0);
		break;
	}
}


//static DEVICE_START( ymf262 )
int device_start_ymf262(void **_info, int EMU_CORE, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	//static const ymf262_interface dummy = { 0 };
	//ymf262_state *info = get_safe_token(device);
	ymf262_state *info;
	int rate;

#ifdef ENABLE_ALL_CORES
	if (EMU_CORE >= 0x02)
		EMU_CORE = EC_DBOPL;
#else
	EMU_CORE = EC_DBOPL;
#endif
	
	info = (ymf262_state *) calloc(1, sizeof(ymf262_state));
	*_info = (void *) info;
	
	info->EMU_CORE = EMU_CORE;
	rate = clock/288;
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;

	//info->intf = device->static_config ? (const ymf262_interface *)device->static_config : &dummy;
	//info->intf = &dummy;
	//info->device = device;

	/* stream system initialize */
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		info->chip = ymf262_init(clock,rate);
		//assert_always(info->chip != NULL, "Error creating YMF262 chip");

		//info->stream = stream_create(device,0,4,rate,info,ymf262_stream_update);

		/* YMF262 setup */
		ymf262_set_timer_handler (info->chip, timer_handler_262, info);
		ymf262_set_irq_handler   (info->chip, IRQHandler_262, info);
		ymf262_set_update_handler(info->chip, _stream_update, info);

		//info->timer[0] = timer_alloc(device->machine, timer_callback_262_0, info);
		//info->timer[1] = timer_alloc(device->machine, timer_callback_262_1, info);
		break;
#endif
	case EC_DBOPL:
		info->chip = adlib_OPL3_init(clock, rate, _stream_update, info);
		break;
	}

	return rate;
}

//static DEVICE_STOP( ymf262 )
void device_stop_ymf262(void *_info)
{
	//ymf262_state *info = get_safe_token(device);
	ymf262_state *info = (ymf262_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ymf262_shutdown(info->chip);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL3_stop(info->chip);
		break;
	}
	free(info);
}

/* reset */
//static DEVICE_RESET( ymf262 )
void device_reset_ymf262(void *_info)
{
	//ymf262_state *info = get_safe_token(device);
	ymf262_state *info = (ymf262_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ymf262_reset_chip(info->chip);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL3_reset(info->chip);
		break;
	}
}


//READ8_DEVICE_HANDLER( ymf262_r )
UINT8 ymf262_r(void *_info, offs_t offset)
{
	//ymf262_state *info = get_safe_token(device);
	ymf262_state *info = (ymf262_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		return ymf262_read(info->chip, offset & 3);
#endif
	case EC_DBOPL:
		return adlib_OPL3_reg_read(info->chip, offset & 0x03);
	default:
		return 0x00;
	}
}

//WRITE8_DEVICE_HANDLER( ymf262_w )
void ymf262_w(void *_info, offs_t offset, UINT8 data)
{
	//ymf262_state *info = get_safe_token(device);
	ymf262_state *info = (ymf262_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ymf262_write(info->chip, offset & 3, data);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL3_writeIO(info->chip, offset & 3, data);
		break;
	}
}

//READ8_DEVICE_HANDLER ( ymf262_status_r )
UINT8 ymf262_status_r(void *info, offs_t offset)
{
	return ymf262_r(info, 0);
}
//WRITE8_DEVICE_HANDLER( ymf262_register_a_w )
void ymf262_register_a_w(void *info, offs_t offset, UINT8 data)
{
	ymf262_w(info, 0, data);
}
//WRITE8_DEVICE_HANDLER( ymf262_register_b_w )
void ymf262_register_b_w(void *info, offs_t offset, UINT8 data)
{
	ymf262_w(info, 2, data);
}
//WRITE8_DEVICE_HANDLER( ymf262_data_a_w )
void ymf262_data_a_w(void *info, offs_t offset, UINT8 data)
{
	ymf262_w(info, 1, data);
}
//WRITE8_DEVICE_HANDLER( ymf262_data_b_w )
void ymf262_data_b_w(void *info, offs_t offset, UINT8 data)
{
	ymf262_w(info, 3, data);
}


void ymf262_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ymf262_state *info = (ymf262_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ymf262_set_mutemask(info->chip, MuteMask);
		break;
#endif
	case EC_DBOPL:
		adlib_OPL3_set_mute_mask(info->chip, MuteMask);
		break;
	}
	
	return;
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( ymf262 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ymf262_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ymf262 );				break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( ymf262 );				break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( ymf262 );				break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YMF262");							break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");						break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/

