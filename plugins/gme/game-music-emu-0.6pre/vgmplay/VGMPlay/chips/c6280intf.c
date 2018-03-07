#include <stdlib.h>
#include "mamedef.h"
#ifdef ENABLE_ALL_CORES
#include "c6280.h"
#endif
#include "Ootake_PSG.h"

#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01
#endif
#define EC_OOTAKE	0x00

#ifndef NULL
#define NULL	((void *)0)
#endif

typedef struct _c6280_state
{
	void* chip;
	int EMU_CORE;
} c6280_state;

void c6280_update(void *_info, stream_sample_t **outputs, int samples)
{
	c6280_state* info = (c6280_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		c6280m_update(info->chip, outputs, samples);
		break;
#endif
	case EC_OOTAKE:
		PSG_Mix(info->chip, outputs, samples);
		break;
	}
}

int device_start_c6280(void **_info, int EMU_CORE, int clock, int SampleRate, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	c6280_state* info;
	int rate;

#ifdef ENABLE_ALL_CORES
	if (EMU_CORE >= 0x02)
		EMU_CORE = EC_OOTAKE;
#else
	EMU_CORE = EC_OOTAKE;
#endif
	
	info = (c6280_state *) calloc(1, sizeof(c6280_state));
	*_info = (void *) info;
 
	info->EMU_CORE = EMU_CORE;
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		rate = (clock & 0x7FFFFFFF)/16;
		if (((CHIP_SAMPLING_MODE & 0x01) && rate < CHIP_SAMPLE_RATE) ||
			CHIP_SAMPLING_MODE == 0x02)
			rate = CHIP_SAMPLE_RATE;
		
		info->chip = device_start_c6280m(clock, rate);
		if (info->chip == NULL)
			return 0;
		break;
#endif
	case EC_OOTAKE:
		rate = SampleRate;
		info->chip = PSG_Init(clock, rate);
		if (info->chip == NULL)
			return 0;
		break;
	}

	return rate;
}

void device_stop_c6280(void *_info)
{
	c6280_state* info = (c6280_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		device_stop_c6280m(info->chip);
		break;
#endif
	case EC_OOTAKE:
		PSG_Deinit(info->chip);
		break;
	}
	info->chip = NULL;

	free(info);	

	return;
}

void device_reset_c6280(void *_info)
{
	c6280_state* info = (c6280_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		device_reset_c6280m(info->chip);
		break;
#endif
	case EC_OOTAKE:
		PSG_ResetVolumeReg(info->chip);
		break;
	}
	return;
}

UINT8 c6280_r(void *_info, offs_t offset)
{
	c6280_state* info = (c6280_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		return c6280m_r(info->chip, offset);
#endif
	case EC_OOTAKE:
		return PSG_Read(info->chip, offset);
	default:
		return 0x00;
	}
}

void c6280_w(void *_info, offs_t offset, UINT8 data)
{
	c6280_state* info = (c6280_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		c6280m_w(info->chip, offset, data);
		break;
#endif
	case EC_OOTAKE:
		PSG_Write(info->chip, offset, data);
		break;
	}
	
	return;
}


void c6280_set_mute_mask(void *_info, UINT32 MuteMask)
{
	c6280_state* info = (c6280_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		c6280m_set_mute_mask(info->chip, MuteMask);
		break;
#endif
	case EC_OOTAKE:
		PSG_SetMuteMask(info->chip, MuteMask);
		break;
	}
	
	return;
}
