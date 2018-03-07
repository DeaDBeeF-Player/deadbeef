/****************************************************************

    MAME / MESS functions

****************************************************************/

#include <string.h>	// for memset
#include <stdlib.h>	// for free
#include <stddef.h>	// for NULL
#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
#include "ay8910.h"		// must be always included (for YM2149_PIN26_LOW)
#include "emu2149.h"
#include "ay_intf.h"


#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01	// AY8910 core from MAME
#endif
#define EC_EMU2149	0x00	// EMU2149 from NSFPlay


/* for stream system */
typedef struct _ayxx_state ayxx_state;
struct _ayxx_state
{
	void *chip;
	int EMU_CORE;
};

void ayxx_stream_update(void *_info, stream_sample_t **outputs, int samples)
{
	ayxx_state *info = (ayxx_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_update_one(info->chip, outputs, samples);
		break;
#endif
	case EC_EMU2149:
		PSG_calc_stereo((PSG*)info->chip, outputs, samples);
		break;
	}
}

int device_start_ayxx(void **_info, int EMU_CORE, int clock, UINT8 chip_type, UINT8 Flags, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	ayxx_state *info;
	int rate;

#ifdef ENABLE_ALL_CORES
	if (EMU_CORE >= 0x02)
		EMU_CORE = EC_EMU2149;
#else
	EMU_CORE = EC_EMU2149;
#endif
	
	info = (ayxx_state *) calloc(1, sizeof(ayxx_state));
	*_info = (void *) info;
 
	info->EMU_CORE = EMU_CORE;
	if (Flags & YM2149_PIN26_LOW)
		rate = clock / 16;
	else
		rate = clock / 8;
	if (((CHIP_SAMPLING_MODE & 0x01) && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		rate = ay8910_start(&info->chip, clock, chip_type, Flags);
		break;
#endif
	case EC_EMU2149:
		if (Flags & YM2149_PIN26_LOW)
			clock /= 2;
		info->chip = PSG_new(clock, rate);
		if (info->chip == NULL)
			return 0;
		PSG_setVolumeMode((PSG*)info->chip, (chip_type & 0x10) ? 1 : 2);
		PSG_setFlags((PSG*)info->chip, Flags & ~YM2149_PIN26_LOW);
		break;
	}

	return rate;
}

void device_stop_ayxx(void *_info)
{
	ayxx_state *info = (ayxx_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_stop_ym(info->chip);
		break;
#endif
	case EC_EMU2149:
		PSG_delete((PSG*)info->chip);
		break;
	}
	info->chip = NULL;
	free(info);
}

void device_reset_ayxx(void *_info)
{
	ayxx_state *info = (ayxx_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_reset_ym(info->chip);
		break;
#endif
	case EC_EMU2149:
		PSG_reset((PSG*)info->chip);
		break;
	}
}


void ayxx_w(void *_info, offs_t offset, UINT8 data)
{
	ayxx_state *info = (ayxx_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_write_ym(info->chip, offset, data);
		break;
#endif
	case EC_EMU2149:
		PSG_writeIO((PSG*)info->chip, offset, data);
		break;
	}
}

void ayxx_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ayxx_state *info = (ayxx_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_set_mute_mask_ym(info->chip, MuteMask);
		break;
#endif
	case EC_EMU2149:
		PSG_setMask((PSG*)info->chip, MuteMask);
		break;
	}
	
	return;
}
