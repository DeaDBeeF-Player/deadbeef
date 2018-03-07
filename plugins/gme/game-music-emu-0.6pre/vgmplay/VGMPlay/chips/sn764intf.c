/****************************************************************

    MAME / MESS functions

****************************************************************/

#include <stdlib.h>
#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
#include "sn76496.h"
#include "sn76489.h"
#include "sn764intf.h"


#define EC_MAME		0x00	// SN76496 core from MAME
#ifdef ENABLE_ALL_CORES
#define EC_MAXIM	0x01	// SN76489 core by Maxim (from in_vgm)
#endif

#ifndef NULL
#define NULL	((void *)0)
#endif

/* for stream system */
typedef struct _sn764xx_state sn764xx_state;
struct _sn764xx_state
{
	void *chip;
	int EMU_CORE;
};

void sn764xx_stream_update(void *_info, stream_sample_t **outputs, int samples)
{
	sn764xx_state* info = (sn764xx_state*)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		SN76496Update(info->chip, outputs, samples);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_MAXIM:
		SN76489_Update((SN76489_Context*)info->chip, outputs, samples);
		break;
#endif
	}
}

int device_start_sn764xx(void **_info, int EMU_CORE, int clock, int SampleRate, int shiftregwidth, int noisetaps,
						 int negate, int stereo, int clockdivider, int freq0)
{
	sn764xx_state *info;
	int rate;

#ifdef ENABLE_ALL_CORES
	if (EMU_CORE >= 0x02)
		EMU_CORE = EC_MAME;
#else
	EMU_CORE = EC_MAME;
#endif
	
	info = (sn764xx_state*) calloc(1, sizeof(sn764xx_state));
	*_info = (void *) info;
	/* emulator create */
	info->EMU_CORE = EMU_CORE;
	switch(EMU_CORE)
	{
	case EC_MAME:
		rate = sn76496_start(&info->chip, clock, shiftregwidth, noisetaps,
							negate, stereo, clockdivider, freq0);
		sn76496_freq_limiter(clock & 0x3FFFFFFF, clockdivider, SampleRate);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_MAXIM:
		rate = SampleRate;
		info->chip = SN76489_Init(clock, rate);
		if (info->chip == NULL)
			return 0;
		SN76489_Config((SN76489_Context*)info->chip, noisetaps, shiftregwidth, 0);
		break;
#endif
	}

	return rate;
}

void device_stop_sn764xx(void *_info)
{
	sn764xx_state *info = (sn764xx_state*)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		sn76496_shutdown(info->chip);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_MAXIM:
		SN76489_Shutdown((SN76489_Context*)info->chip);
		break;
#endif
	}
}

void device_reset_sn764xx(void *_info)
{
	sn764xx_state *info = (sn764xx_state*)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		sn76496_reset(info->chip);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_MAXIM:
		SN76489_Reset((SN76489_Context*)info->chip);
		break;
#endif
	}
}


void sn764xx_w(void *_info, offs_t offset, UINT8 data)
{
	sn764xx_state *info = (sn764xx_state*)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		switch(offset)
		{
		case 0x00:
			sn76496_write_reg(info->chip, offset & 1, data);
			break;
		case 0x01:
			sn76496_stereo_w(info->chip, offset, data);
			break;
		}
		break;
#ifdef ENABLE_ALL_CORES
	case EC_MAXIM:
		switch(offset)
		{
		case 0x00:
			SN76489_Write((SN76489_Context*)info->chip, data);
			break;
		case 0x01:
			SN76489_GGStereoWrite((SN76489_Context*)info->chip, data);
			break;
		}
		break;
#endif
	}
}

void sn764xx_set_mute_mask(void *_info, UINT32 MuteMask)
{
	sn764xx_state *info = (sn764xx_state*)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		sn76496_set_mutemask(info->chip, MuteMask);
		break;
#ifdef ENABLE_ALL_CORES
	case EC_MAXIM:
		SN76489_SetMute(info->chip, ~MuteMask & 0x0F);
		break;
#endif
	}
	
	return;
}

void sn764xx_set_panning(void *_info, INT16* PanVals)
{
	sn764xx_state *info = (sn764xx_state*)_info;
	switch(info->EMU_CORE)
	{
	case EC_MAME:
		break;
#ifdef ENABLE_ALL_CORES
	case EC_MAXIM:
		SN76489_SetPanning(info->chip, PanVals[0x00], PanVals[0x01], PanVals[0x02], PanVals[0x03]);
		break;
#endif
	}
	
	return;
}
