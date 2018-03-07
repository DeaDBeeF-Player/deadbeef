#include <math.h>
#include <string.h>	// for memset
#include <stdlib.h>	// for free
#include <stddef.h>	// for NULL
#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
#include "2203intf.h"
#include "fm.h"


#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01	// AY8910 core from MAME
#endif
#define EC_EMU2149	0x00

typedef struct _ym2203_state ym2203_state;
struct _ym2203_state
{
	//sound_stream *	stream;
	//emu_timer *		timer[2];
	void *			chip;
	void *			psg;
	ym2203_interface intf;
	int			AY_EMU_CORE;
	//const device_config *device;
};

#define CHTYPE_YM2203	0x20


/*INLINE ym2203_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_YM2203);
	return (ym2203_state *)device->token;
}*/


static void psg_set_clock(void *param, int clock)
{
	ym2203_state *info = (ym2203_state *)param;
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_set_clock_ym(info->psg, clock);
			break;
#endif
		case EC_EMU2149:
			PSG_set_clock((PSG*)info->psg, clock);
			break;
		}
	}
}

static void psg_write(void *param, int address, int data)
{
	ym2203_state *info = (ym2203_state *)param;
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_write_ym(info->psg, address, data);
			break;
#endif
		case EC_EMU2149:
			PSG_writeIO((PSG*)info->psg, address, data);
			break;
		}
	}
}

static int psg_read(void *param)
{
	ym2203_state *info = (ym2203_state *)param;
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			return ay8910_read_ym(info->psg);
#endif
		case EC_EMU2149:
			return PSG_readIO((PSG*)info->psg);
		}
	}
	return 0x00;
}

static void psg_reset(void *param)
{
	ym2203_state *info = (ym2203_state *)param;
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_reset_ym(info->psg);
			break;
#endif
		case EC_EMU2149:
			PSG_reset((PSG*)info->psg);
			break;
		}
	}
}

static const ssg_callbacks psgintf =
{
	psg_set_clock,
	psg_write,
	psg_read,
	psg_reset
};

/* IRQ Handler */
/*static void IRQHandler(void *param,int irq)
{
	ym2203_state *info = (ym2203_state *)param;
	if (info->intf.handler != NULL)
		//(*info->intf->handler)(info->device, irq);
		(*info->intf.handler)(irq);
}*/

/* Timer overflow callback from timer.c */
/*static TIMER_CALLBACK( timer_callback_2203_0 )
{
	ym2203_state *info = (ym2203_state *)ptr;
	ym2203_timer_over(info->chip,0);
}

static TIMER_CALLBACK( timer_callback_2203_1 )
{
	ym2203_state *info = (ym2203_state *)ptr;
	ym2203_timer_over(info->chip,1);
}*/

static stream_sample_t* DUMMYBUF[0x02] = {NULL, NULL};

/* update request from fm.c */
void ym2203_update_request(void *param)
{
	ym2203_state *info = (ym2203_state *)param;
	//stream_update(info->stream);
	
	ym2203_update_one(info->chip, DUMMYBUF, 0);
	// We really don't need this.
	/*if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_update_one(info->psg, DUMMYBUF, 0);
			break;
#endif
		case EC_EMU2149:
			PSG_calc_stereo((PSG*)info->psg, DUMMYBUF, 0);
			break;
		}
	}*/
}


/*static void timer_handler(void *param,int c,int count,int clock)
{
	ym2203_state *info = (ym2203_state *)param;
	if( count == 0 )
	{	// Reset FM Timer
		//timer_enable(info->timer[c], 0);
	}
	else
	{	// Start FM Timer
		//attotime period = attotime_mul(ATTOTIME_IN_HZ(clock), count);
		//if (!timer_enable(info->timer[c], 1))
		//	timer_adjust_oneshot(info->timer[c], period, 0);
	}
}*/

//static STREAM_UPDATE( ym2203_stream_update )
void ym2203_stream_update(void *_info, stream_sample_t **outputs, int samples)
{
	//ym2203_state *info = (ym2203_state *)param;
	ym2203_state *info = (ym2203_state *)_info;
	ym2203_update_one(info->chip, outputs, samples);
}

void ym2203_stream_update_ay(void *_info, stream_sample_t **outputs, int samples)
{
	//ym2203_state *info = (ym2203_state *)param;
	ym2203_state *info = (ym2203_state *)_info;
	
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_update_one(info->psg, outputs, samples);
			break;
#endif
		case EC_EMU2149:
			PSG_calc_stereo((PSG*)info->psg, outputs, samples);
			break;
		}
	}
	else
	{
		memset(outputs[0], 0x00, samples * sizeof(stream_sample_t));
		memset(outputs[1], 0x00, samples * sizeof(stream_sample_t));
	}
}


//static STATE_POSTLOAD( ym2203_intf_postload )
/*static void ym2203_intf_postload(UINT8 ChipID)
{
	//ym2203_state *info = (ym2203_state *)param;
	ym2203_state *info = &YM2203Data[ChipID];
	ym2203_postload(info->chip);
}*/


//static DEVICE_START( ym2203 )
int device_start_ym2203(void **_info, int AY_EMU_CORE, int clock, UINT8 AYDisable, UINT8 AYFlags, int* AYrate, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	static const ym2203_interface generic_2203 =
	{
		{
			AY8910_LEGACY_OUTPUT,
			AY8910_DEFAULT_LOADS
			//DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL
		},
		NULL
	};
	//const ym2203_interface *intf = device->static_config ? (const ym2203_interface *)device->static_config : &generic_2203;
	ym2203_interface* intf;
	//ym2203_state *info = get_safe_token(device);
	ym2203_state *info;
	int rate;
	int ay_clock;

#ifdef ENABLE_ALL_CORES
	if (AY_EMU_CORE >= 0x02)
		AY_EMU_CORE = EC_EMU2149;
#else
        AY_EMU_CORE = EC_EMU2149;
#endif

	info = (ym2203_state *) calloc(1, sizeof(ym2203_state));
	*_info = (void *)info;
	
	info->AY_EMU_CORE = AY_EMU_CORE;
	rate = clock/72; /* ??? */
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;

	info->intf = generic_2203;
	intf = &info->intf;
	if (AYFlags)
		intf->ay8910_intf.flags = AYFlags;
	//info->device = device;
	//info->psg = ay8910_start_ym(NULL, SOUND_YM2203, device, device->clock, &intf->ay8910_intf);
	if (! AYDisable)
	{
		ay_clock = clock / 2;
		*AYrate = ay_clock / 8;
		if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
			CHIP_SAMPLING_MODE == 0x02)
			*AYrate = CHIP_SAMPLE_RATE;
		
		switch(AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			// fits in the most common cases
			// TODO: remove after being able to change the resampler's sampling rate
			info->psg = ay8910_start_ym(NULL, CHTYPE_YM2203, ay_clock, &intf->ay8910_intf);
			break;
#endif
		case EC_EMU2149:
			info->psg = PSG_new(ay_clock, *AYrate);
			if (info->psg == NULL)
				return 0;
			PSG_setVolumeMode((PSG*)info->psg, 1);	// YM2149 volume mode
			break;
		}
	}
	else
	{
		info->psg = NULL;
		*AYrate = 0;
	}
	//assert_always(info->psg != NULL, "Error creating YM2203/AY8910 chip");

	/* Timer Handler set */
	//info->timer[0] = timer_alloc(device->machine, timer_callback_2203_0, info);
	//info->timer[1] = timer_alloc(device->machine, timer_callback_2203_1, info);

	/* stream system initialize */
	//info->stream = stream_create(device,0,1,rate,info,ym2203_stream_update);

	/* Initialize FM emurator */
	//info->chip = ym2203_init(info,clock,rate,timer_handler,IRQHandler,&psgintf);
	info->chip = ym2203_init(info,clock,rate,NULL,NULL,&psgintf);
	//assert_always(info->chip != NULL, "Error creating YM2203 chip");

	//state_save_register_postload(device->machine, ym2203_intf_postload, info);

	return rate;
}

//static DEVICE_STOP( ym2203 )
void device_stop_ym2203(void *_info)
{
	//ym2203_state *info = get_safe_token(device);
	ym2203_state *info = (ym2203_state *)_info;
	ym2203_shutdown(info->chip);
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_stop_ym(info->psg);
			break;
#endif
		case EC_EMU2149:
			PSG_delete((PSG*)info->psg);
			break;
		}
		info->psg = NULL;
	}
	free(info);
}

//static DEVICE_RESET( ym2203 )
void device_reset_ym2203(void *_info)
{
	//ym2203_state *info = get_safe_token(device);
	ym2203_state *info = (ym2203_state *)_info;
	ym2203_reset_chip(info->chip);	// also resets the AY clock
	//psg_reset(info);	// already done as a callback in ym2203_reset_chip
}



//READ8_DEVICE_HANDLER( ym2203_r )
UINT8 ym2203_r(void *_info, offs_t offset)
{
	//ym2203_state *info = get_safe_token(device);
	ym2203_state *info = (ym2203_state *)_info;
	return ym2203_read(info->chip, offset & 1);
}

//WRITE8_DEVICE_HANDLER( ym2203_w )
void ym2203_w(void *_info, offs_t offset, UINT8 data)
{
	//ym2203_state *info = get_safe_token(device);
	ym2203_state *info = (ym2203_state *)_info;
	ym2203_write(info->chip, offset & 1, data);
}


//READ8_DEVICE_HANDLER( ym2203_status_port_r )
UINT8 ym2203_status_port_r(void *info, offs_t offset)
{
	return ym2203_r(info, 0);
}
//READ8_DEVICE_HANDLER( ym2203_read_port_r )
UINT8 ym2203_read_port_r(void *info, offs_t offset)
{
	return ym2203_r(info, 1);
}
//WRITE8_DEVICE_HANDLER( ym2203_control_port_w )
void ym2203_control_port_w(void *info, offs_t offset, UINT8 data)
{
	ym2203_w(info, 0, data);
}
//WRITE8_DEVICE_HANDLER( ym2203_write_port_w )
void ym2203_write_port_w(void *info, offs_t offset, UINT8 data)
{
	ym2203_w(info, 1, data);
}


void ym2203_set_mute_mask(void *_info, UINT32 MuteMaskFM, UINT32 MuteMaskAY)
{
	ym2203_state *info = (ym2203_state *)_info;
	ym2203_set_mutemask(info->chip, MuteMaskFM);
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_set_mute_mask_ym(info->psg, MuteMaskAY);
			break;
#endif
		case EC_EMU2149:
			PSG_setMask((PSG*)info->psg, MuteMaskAY);
			break;
		}
	}
}

void ym2203_set_srchg_cb(void *_info, SRATE_CALLBACK CallbackFunc, void* DataPtr, void* AYDataPtr)
{
	ym2203_state *info = (ym2203_state *)_info;
	
	if (info->psg != NULL)
	{
		switch(info->AY_EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			ay8910_set_srchg_cb_ym(info->psg, CallbackFunc, AYDataPtr);
			break;
#endif
		case EC_EMU2149:
			break;
		}
	}
	
	return;
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( ym2203 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ym2203_state);					break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ym2203 );		break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( ym2203 );		break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( ym2203 );		break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YM2203");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/
