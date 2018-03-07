/***************************************************************************

  2151intf.c

  Support interface YM2151(OPM)

***************************************************************************/

#include <stdlib.h>
#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
#include "fm.h"
#include "2151intf.h"
#include "ym2151.h"


#ifndef NULL
#define NULL	((void *)0)
#endif

typedef struct _ym2151_state ym2151_state;
struct _ym2151_state
{
	//sound_stream *			stream;
	//emu_timer *				timer[2];
	void *					chip;
	UINT8					lastreg;
	//const ym2151_interface *intf;
};


/*INLINE ym2151_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_YM2151);
	return (ym2151_state *)device->token;
}*/

//static STREAM_UPDATE( ym2151_update )
void ym2151_update(void *param, stream_sample_t **outputs, int samples)
{
	ym2151_state *info = (ym2151_state *)param;
	ym2151_update_one(info->chip, outputs, samples);
	//YM2151UpdateOne(0x00, outputs, samples);
}


//static STATE_POSTLOAD( ym2151intf_postload )
/*static void ym2151intf_postload(UINT8 ChipID)
{
	//ym2151_state *info = (ym2151_state *)param;
	ym2151_state *info = &YM2151Data[ChipID];
	ym2151_postload(info->chip);
}*/


//static DEVICE_START( ym2151 )
int device_start_ym2151(void **_info, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	//static const ym2151_interface dummy = { 0 };
	
	//ym2151_state *info = get_safe_token(device);
	ym2151_state *info;
	int rate;

	info = (ym2151_state *) calloc(1, sizeof(ym2151_state));
	*_info = (void *) info;	

	rate = clock/64;
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	//info->intf = device->static_config ? (const ym2151_interface *)device->static_config : &dummy;
	//info->intf = &dummy;

	/* stream setup */
	//info->stream = stream_create(device,0,2,rate,info,ym2151_update);

	info->chip = ym2151_init(clock,rate);
	//assert_always(info->chip != NULL, "Error creating YM2151 chip");

	//state_save_register_postload(device->machine, ym2151intf_postload, info);

	//ym2151_set_irq_handler(info->chip,info->intf->irqhandler);
	//ym2151_set_port_write_handler(info->chip,info->intf->portwritehandler);

	return rate;
}


//static DEVICE_STOP( ym2151 )
void device_stop_ym2151(void *_info)
{
	//ym2151_state *info = get_safe_token(device);
	ym2151_state *info = (ym2151_state *)_info;
	ym2151_shutdown(info->chip);
	//YM2151Shutdown();
	free(info);
}

//static DEVICE_RESET( ym2151 )
void device_reset_ym2151(void *_info)
{
	//ym2151_state *info = get_safe_token(device);
	ym2151_state *info = (ym2151_state *)_info;
	ym2151_reset_chip(info->chip);
	//YM2151ResetChip(0x00);
}


//READ8_DEVICE_HANDLER( ym2151_r )
UINT8 ym2151_r(void *_info, offs_t offset)
{
	//ym2151_state *token = get_safe_token(device);
	ym2151_state *token = (ym2151_state *)_info;

	if (offset & 1)
	{
		//stream_update(token->stream);
		return ym2151_read_status(token->chip);
		//return YM2151ReadStatus(0x00);
	}
	else
		return 0xff;	/* confirmed on a real YM2151 */
}

//WRITE8_DEVICE_HANDLER( ym2151_w )
void ym2151_w(void *_info, offs_t offset, UINT8 data)
{
	//ym2151_state *token = get_safe_token(device);
	ym2151_state *token = (ym2151_state *)_info;

	if (offset & 1)
	{
		//stream_update(token->stream);
		ym2151_write_reg(token->chip, token->lastreg, data);
		//YM2151WriteReg(0x00, token->lastreg, data);
	}
	else
		token->lastreg = data;
}


/*READ8_DEVICE_HANDLER( ym2151_status_port_r ) { return ym2151_r(device, 1); }

WRITE8_DEVICE_HANDLER( ym2151_register_port_w ) { ym2151_w(device, 0, data); }
WRITE8_DEVICE_HANDLER( ym2151_data_port_w ) { ym2151_w(device, 1, data); }*/
UINT8 ym2151_status_port_r(void *info, offs_t offset)
{
	return ym2151_r(info, 1);
}

void ym2151_register_port_w(void *info, offs_t offset, UINT8 data)
{
	ym2151_w(info, 0, data);
}
void ym2151_data_port_w(void *info, offs_t offset, UINT8 data)
{
	ym2151_w(info, 1, data);
}


void ym2151_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ym2151_state *info = (ym2151_state *)_info;
	ym2151_set_mutemask(info->chip, MuteMask);
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( ym2151 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ym2151_state);					break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ym2151 );		break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( ym2151 );		break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( ym2151 );		break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YM2151");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/
