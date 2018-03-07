/****************************************************************

    MAME / MESS functions

****************************************************************/

#include "mamedef.h"
#include <string.h>	// for memset
#include <stdlib.h>	// for free
#include <stddef.h>	// for NULL
#include "../stdbool.h"
//#include "sndintrf.h"
//#include "streams.h"
#include "nes_apu.h"
#include "np_nes_apu.h"
#include "np_nes_dmc.h"
#include "np_nes_fds.h"
#include "nes_intf.h"


#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01	// NES core from MAME
#endif
#define EC_NSFPLAY	0x00	// NES core from NSFPlay
// Note: FDS core from NSFPlay is always used

/* for stream system */
typedef struct _nes_state nes_state;
struct _nes_state
{
	void* chip_apu;
	void* chip_dmc;
	void* chip_fds;
	UINT8* Memory;
	int EMU_CORE;
};

void nes_stream_update(void *_info, stream_sample_t **outputs, int samples)
{
	nes_state *info = (nes_state *)_info;
	int CurSmpl;
	INT32 Buffer[4];
	
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		nes_psg_update_sound(info->chip_apu, outputs, samples);
		break;
#endif
	case EC_NSFPLAY:
		for (CurSmpl = 0x00; CurSmpl < samples; CurSmpl ++)
		{
			NES_APU_np_Render(info->chip_apu, &Buffer[0]);
			NES_DMC_np_Render(info->chip_dmc, &Buffer[2]);
			outputs[0][CurSmpl] = Buffer[0] + Buffer[2];
			outputs[1][CurSmpl] = Buffer[1] + Buffer[3];
		}
		break;
	}
	
	if (info->chip_fds != NULL)
	{
		for (CurSmpl = 0x00; CurSmpl < samples; CurSmpl ++)
		{
			NES_FDS_Render(info->chip_fds, &Buffer[0]);
			outputs[0][CurSmpl] += Buffer[0];
			outputs[1][CurSmpl] += Buffer[1];
		}
	}
	
	return;
}

static void nes_set_chip_option(nes_state *info, int NesOptions);
int device_start_nes(void **_info, int EMU_CORE, int clock, int Options, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	nes_state *info;
	int rate;
	bool EnableFDS;

#ifdef ENABLE_ALL_CORES
	if (EMU_CORE >= 0x02)
		EMU_CORE = EC_NSFPLAY;
#else
	EMU_CORE = EC_NSFPLAY;
#endif
	
	EnableFDS = (clock >> 31) & 0x01;
	clock &= 0x7FFFFFFF;
	
	info = (nes_state *) calloc(1, sizeof(nes_state));
	*_info = (void *) info;
	
	info->EMU_CORE = EMU_CORE;
	rate = clock / 4;
	if (((CHIP_SAMPLING_MODE & 0x01) && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		info->chip_apu = device_start_nesapu(clock, rate);
		if (info->chip_apu == NULL)
			return 0;
		
		info->chip_dmc = NULL;
		info->chip_fds = NULL;
		
		info->Memory = (UINT8*)malloc(0x8000);
		memset(info->Memory, 0x00, 0x8000);
		nesapu_set_rom(info->chip_apu, info->Memory - 0x8000);
		break;
#endif
	case EC_NSFPLAY:
		info->chip_apu = NES_APU_np_Create(clock, rate);
		if (info->chip_apu == NULL)
			return 0;
		
		info->chip_dmc = NES_DMC_np_Create(clock, rate);
		if (info->chip_dmc == NULL)
		{
			NES_APU_np_Destroy(info->chip_apu);
			info->chip_apu = NULL;
			return 0;
		}
		
		NES_DMC_np_SetAPU(info->chip_dmc, info->chip_apu);
		
		info->Memory = (UINT8*)malloc(0x8000);
		memset(info->Memory, 0x00, 0x8000);
		NES_DMC_np_SetMemory(info->chip_dmc, info->Memory - 0x8000);
		break;
	}
	
	if (EnableFDS)
	{
		info->chip_fds = NES_FDS_Create(clock, rate);
		// If it returns NULL, that's okay.
	}
	else
	{
		info->chip_fds = NULL;
	}
	nes_set_chip_option(info, Options);

	return rate;
}

void device_stop_nes(void *_info)
{
	nes_state *info = (nes_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		device_stop_nesapu(info->chip_apu);
		break;
#endif
	case EC_NSFPLAY:
		NES_APU_np_Destroy(info->chip_apu);
		NES_DMC_np_Destroy(info->chip_dmc);
		break;
	}
	if (info->chip_fds != NULL)
		NES_FDS_Destroy(info->chip_fds);
	
	if (info->Memory != NULL)
	{
		free(info->Memory);
		info->Memory = NULL;
	}
	info->chip_apu = NULL;
	info->chip_dmc = NULL;
	info->chip_fds = NULL;

	free(info);	

	return;
}

void device_reset_nes(void *_info)
{
	nes_state *info = (nes_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		device_reset_nesapu(info->chip_apu);
		break;
#endif
	case EC_NSFPLAY:
		NES_APU_np_Reset(info->chip_apu);
		NES_DMC_np_Reset(info->chip_dmc);
		break;
	}
	if (info->chip_fds != NULL)
		NES_FDS_Reset(info->chip_fds);
}


void nes_w(void *_info, offs_t offset, UINT8 data)
{
	nes_state *info = (nes_state *)_info;
	
	switch(offset & 0xE0)
	{
	case 0x00:	// NES APU
		switch(info->EMU_CORE)
		{
#ifdef ENABLE_ALL_CORES
		case EC_MAME:
			nes_psg_w(info->chip_apu, offset, data);
			break;
#endif
		case EC_NSFPLAY:
			// NES_APU handles the sqaure waves, NES_DMC the rest
			NES_APU_np_Write(info->chip_apu, 0x4000 | offset, data);
			NES_DMC_np_Write(info->chip_dmc, 0x4000 | offset, data);
			break;
		}
		break;
	case 0x20:	// FDS register
		if (info->chip_fds == NULL)
			break;
		if (offset == 0x3F)
			NES_FDS_Write(info->chip_fds, 0x4023, data);
		else
			NES_FDS_Write(info->chip_fds, 0x4080 | (offset & 0x1F), data);
		break;
	case 0x40:	// FDS wave RAM
	case 0x60:
		if (info->chip_fds == NULL)
			break;
		NES_FDS_Write(info->chip_fds, 0x4000 | offset, data);
		break;
	}
}

void nes_write_ram(void *_info, offs_t DataStart, offs_t DataLength, const UINT8* RAMData)
{
	nes_state* info = (nes_state *)_info;
	UINT32 RemainBytes;
	
	if (DataStart >= 0x10000)
		return;
	
	if (DataStart < 0x8000)
	{
		if (DataStart + DataLength <= 0x8000)
			return;
		
		RemainBytes = 0x8000 - DataStart;
		DataStart = 0x8000;
		RAMData += RemainBytes;
		DataLength -= RemainBytes;
	}
	
	RemainBytes = 0x00;
	if (DataStart + DataLength > 0x10000)
	{
		RemainBytes = DataLength;
		DataLength = 0x10000 - DataStart;
		RemainBytes -= DataLength;
	}
	memcpy(info->Memory + (DataStart - 0x8000), RAMData, DataLength);
	if (RemainBytes)
	{
		if (RemainBytes > 0x8000)
			RemainBytes = 0x8000;
		memcpy(info->Memory, RAMData + DataLength, RemainBytes);
	}
	
	return;
}


static void nes_set_chip_option(nes_state *info, int NesOptions)
{
	UINT8 CurOpt;
	
	if (NesOptions & 0x8000)
		return;
	
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		// no options for MAME's NES core
		break;
#endif
	case EC_NSFPLAY:
		// shared APU/DMC options
		for (CurOpt = 0; CurOpt < 2; CurOpt ++)
		{
			NES_APU_np_SetOption(info->chip_apu, CurOpt, (NesOptions >> CurOpt) & 0x01);
			NES_DMC_np_SetOption(info->chip_dmc, CurOpt, (NesOptions >> CurOpt) & 0x01);
		}
		// APU-only options
		for (; CurOpt < 4; CurOpt ++)
			NES_APU_np_SetOption(info->chip_apu, CurOpt-2+2, (NesOptions >> CurOpt) & 0x01);
		// DMC-only options
		for (; CurOpt < 10; CurOpt ++)
			NES_DMC_np_SetOption(info->chip_dmc, CurOpt-4+2, (NesOptions >> CurOpt) & 0x01);
		break;
	}
	if (info->chip_fds != NULL)
	{
		// FDS options
		// I skip the Cutoff frequency here, since it's not a boolean value.
		for (CurOpt = 12; CurOpt < 14; CurOpt ++)
			NES_FDS_SetOption(info->chip_fds, CurOpt-12+1, (NesOptions >> CurOpt) & 0x01);
	}
	
	return;
}

void nes_set_mute_mask(void *_info, UINT32 MuteMask)
{
	nes_state *info = (nes_state *)_info;
	switch(info->EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		nesapu_set_mute_mask(info->chip_apu, MuteMask);
		break;
#endif
	case EC_NSFPLAY:
		NES_APU_np_SetMask(info->chip_apu, (MuteMask & 0x03) >> 0);
		NES_DMC_np_SetMask(info->chip_dmc, (MuteMask & 0x1C) >> 2);
		break;
	}
	if (info->chip_fds != NULL)
		NES_FDS_SetMask(info->chip_fds, (MuteMask & 0x20) >> 5);
	
	return;
}
