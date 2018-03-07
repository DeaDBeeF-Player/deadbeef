/****************************************************************

    MAME / MESS functions

****************************************************************/

#include <stdlib.h>
#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
#ifdef ENABLE_ALL_CORES
#include "ym2413.h"
#endif
#include "emu2413.h"
#include "2413intf.h"

#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01	// YM2413 core from MAME
#endif
#define EC_EMU2413	0x00	// EMU2413 core from in_vgm, value 0 because it's better than MAME

#ifndef NULL
#define NULL	((void *)0)
#endif

/* for stream system */
typedef struct _ym2413_state ym2413_state;
struct _ym2413_state
{
	//sound_stream *	stream;
	void *			chip;
	int			EMU_CORE;
	UINT8			Mode;
};

static unsigned char vrc7_inst[(16 + 3) * 8] =
{
#include "vrc7tone.h"
};

/*INLINE ym2413_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_YM2413);
	return (ym2413_state *)device->token;
}*/

#ifdef UNUSED_FUNCTION
void YM2413DAC_update(int chip,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
    INT16 *buffer = _buffer[0];
    static int out = 0;

    if ( ym2413[chip].reg[0x0F] & 0x01 )
    {
        out = ((ym2413[chip].reg[0x10] & 0xF0) << 7);
    }
    while (length--) *(buffer++) = out;
}
#endif

//static STREAM_UPDATE( ym2413_stream_update )
void ym2413_stream_update(void *_info, stream_sample_t **outputs, int samples)
{
	ym2413_state *info = (ym2413_state*)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym2413_update_one(info->chip, outputs, samples);
		break;
#endif
	case EC_EMU2413:
		OPLL_calc_stereo(info->chip, outputs, samples, -1);
		break;
	}
}

#ifdef ENABLE_ALL_CORES
static stream_sample_t* DUMMYBUF[0x02] = {NULL, NULL};

static void _stream_update(void *param, int interval)
{
	ym2413_state *info = (ym2413_state *)param;
	/*stream_update(info->stream);*/

	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym2413_update_one(info->chip, DUMMYBUF, 0);
		break;
#endif
	case EC_EMU2413:
		OPLL_calc_stereo(info->chip, DUMMYBUF, 0, -1);
		break;
	}
}
#endif

//static DEVICE_START( ym2413 )
int device_start_ym2413(void **_info, int EMU_CORE, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	//ym2413_state *info = get_safe_token(device);
	ym2413_state *info;
	int rate;

#ifdef ENABLE_ALL_CORES
	if (EMU_CORE >= 0x02)
		EMU_CORE = EC_EMU2413;
#else
	EMU_CORE = EC_EMU2413;
#endif

	info = (ym2413_state *) calloc(1, sizeof(ym2413_state));
	*_info = (void*) info;

	info->EMU_CORE = EMU_CORE;
	info->Mode = (clock & 0x80000000) >> 31;
	clock &= 0x7FFFFFFF;

	rate = clock/72;
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	/* emulator create */
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		info->chip = ym2413_init(clock, rate);
		if (info->chip == NULL)
			return 0;
		//assert_always(info->chip != NULL, "Error creating YM2413 chip");
		ym2413_set_chip_mode(info->chip, info->Mode);

		/* stream system initialize */
		//info->stream = stream_create(device,0,2,rate,info,ym2413_stream_update);

		ym2413_set_update_handler(info->chip, _stream_update, info);
		break;
#endif
	case EC_EMU2413:
		info->chip = OPLL_new(clock, rate);
		if (info->chip == NULL)
			return 0;

		OPLL_SetChipMode(info->chip, info->Mode);
		if (info->Mode)
			OPLL_setPatch(info->chip, vrc7_inst);
		break;
	}
	// Note: VRC7 instruments are set in device_reset if necessary.



/*#if 0
	int i, tst;
	char name[40];

	num = intf->num;

	tst = YM3812_sh_start (msound);
	if (tst)
		return 1;

	for (i=0;i<num;i++)
	{
		ym2413_reset (i);

		ym2413[i].DAC_stream = stream_create(device, 0, 1, device->clock/72, i, YM2413DAC_update);

		if (ym2413[i].DAC_stream == -1)
			return 1;
	}
	return 0;
#endif*/

	return rate;
}

//static DEVICE_STOP( ym2413 )
void device_stop_ym2413(void *_info)
{
	//ym2413_state *info = get_safe_token(device);
	ym2413_state *info = (ym2413_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym2413_shutdown(info->chip);
		break;
#endif
	case EC_EMU2413:
		OPLL_delete(info->chip);
		break;
	}

	free(info);
}

//static DEVICE_RESET( ym2413 )
void device_reset_ym2413(void *_info)
{
	//ym2413_state *info = get_safe_token(device);
	ym2413_state *info = (ym2413_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym2413_reset_chip(info->chip);
		if (info->Mode)
			ym2413_override_patches(info->chip, vrc7_inst);
		break;
#endif
	case EC_EMU2413:
		OPLL_reset(info->chip);
		// EMU2413 doesn't reset the patch data in OPLL_reset
		//if (info->Mode)
		//	OPLL_setPatch(info->chip, vrc7_inst);
		break;
	}
}


//WRITE8_DEVICE_HANDLER( ym2413_w )
void ym2413_w(void *_info, offs_t offset, UINT8 data)
{
	//ym2413_state *info = get_safe_token(device);
	ym2413_state *info = (ym2413_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym2413_write(info->chip, offset & 1, data);
		break;
#endif
	case EC_EMU2413:
		OPLL_writeIO(info->chip, offset & 1, data);
		break;
	}
}

//WRITE8_DEVICE_HANDLER( ym2413_register_port_w )
void ym2413_register_port_w(void *_info, offs_t offset, UINT8 data)
{
	ym2413_w(_info, 0, data);
}
//WRITE8_DEVICE_HANDLER( ym2413_data_port_w )
void ym2413_data_port_w(void *_info, offs_t offset, UINT8 data)
{
	ym2413_w(_info, 1, data);
}


void ym2413_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ym2413_state *info = (ym2413_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ym2413_set_mutemask(info->chip, MuteMask);
		break;
#endif
	case EC_EMU2413:
		OPLL_SetMuteMask(info->chip, MuteMask);
		break;
	}

	return;
}

void ym2413_set_panning(void *_info, INT16* PanVals)
{
	ym2413_state *info = (ym2413_state *)_info;
	UINT8 CurChn;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		break;
#endif
	case EC_EMU2413:
		for (CurChn = 0x00; CurChn < 0x0E; CurChn ++)
			OPLL_set_pan(info->chip, CurChn, PanVals[CurChn]);
		break;
	}

	return;
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( ym2413 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ym2413_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ym2413 );				break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( ym2413 );				break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( ym2413 );				break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YM2413");							break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");						break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/
