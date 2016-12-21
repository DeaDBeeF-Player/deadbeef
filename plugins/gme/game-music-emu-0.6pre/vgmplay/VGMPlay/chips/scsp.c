//#include "emu.h"
#include "mamedef.h"
#include <stdlib.h>	// for malloc/free
#include <string.h>	// for memset

#include "scsp.h"

#include "yam.h"

enum { SCSPRAM_LENGTH = 0x80000 };

#undef YAMSTATE
#define SCSPRAM ((unsigned char *)info)
#define YAMSTATE ((void*)(SCSPRAM+SCSPRAM_LENGTH))

//static STREAM_UPDATE( SCSP_Update )
void SCSP_Update(void *info, stream_sample_t **outputs, int samples)
{
	sint16 buffer[400];
	stream_sample_t *bufferleft = outputs[0];
	stream_sample_t *bufferright = outputs[1];
	while (samples) {
		int i;
		int samplesnow = samples>200 ? 200 : samples;
		yam_beginbuffer(YAMSTATE, buffer);
		yam_advance(YAMSTATE, samplesnow);
        yam_flush(YAMSTATE);
		for (i = 0; i < samplesnow; ++i) {
			*bufferleft++ = buffer[i * 2] << 8;
			*bufferright++ = buffer[i * 2 + 1] << 8;
		}
		samples -= samplesnow;
	}
}

//static DEVICE_START( scsp )

int device_start_scsp(void **_info, int clock, int Flags)
{
	void * info = malloc(SCSPRAM_LENGTH + yam_get_state_size(1));
	if (info) {
		memset(SCSPRAM, 0, SCSPRAM_LENGTH);
        device_reset_scsp(info);
		*_info = info;
	}
	return 44100;
}

void device_stop_scsp(void *info)
{
	free(info);	
}

void device_reset_scsp(void *info)
{
    yam_clear_state(YAMSTATE, 1);
    yam_setram(YAMSTATE, (uint32*)info, SCSPRAM_LENGTH, 0, EMU_ENDIAN_XOR(1) ^ 1);
    yam_enable_dry(YAMSTATE, 1);
    yam_enable_dsp(YAMSTATE, 1);
    yam_enable_dsp_dynarec(YAMSTATE, 0);
}


/*void scsp_set_ram_base(device_t *device, void *base)
{
	scsp_state *scsp = get_safe_token(device);
	if (scsp)
	{
		scsp->SCSPRAM = (unsigned char *)base;
		scsp->DSP.SCSPRAM = (UINT16 *)base;
		scsp->SCSPRAM_LENGTH = 0x80000;
		scsp->DSP.SCSPRAM_LENGTH = 0x80000/2;
	}
}*/


//READ16_DEVICE_HANDLER( scsp_r )
UINT16 scsp_r(void *info, offs_t offset)
{
	return yam_scsp_load_reg(YAMSTATE, offset*2, 0xFFFF);
}

//WRITE16_DEVICE_HANDLER( scsp_w )
void scsp_w(void *info, offs_t offset, UINT8 data)
{
	UINT8 tmp8;
	UINT16 tmp;

	tmp = yam_scsp_load_reg(YAMSTATE, offset & 0xFFFE, 0xFFFF);
	//COMBINE_DATA(&tmp);
	if (offset & 1)
		tmp = (tmp & 0xFF00) | (data << 0);
	else
		tmp = (tmp & 0x00FF) | (data << 8);
	yam_scsp_store_reg(YAMSTATE, offset & 0xFFFE, tmp, 0xFFFF, &tmp8);
}

/*WRITE16_DEVICE_HANDLER( scsp_midi_in )
{
	scsp_state *scsp = get_safe_token(device);

//    printf("scsp_midi_in: %02x\n", data);

	scsp->MidiStack[scsp->MidiW++]=data;
	scsp->MidiW &= 31;

	//CheckPendingIRQ(scsp);
}

READ16_DEVICE_HANDLER( scsp_midi_out_r )
{
	scsp_state *scsp = get_safe_token(device);
	unsigned char val;

	val=scsp->MidiStack[scsp->MidiR++];
	scsp->MidiR&=31;
	return val;
}*/


/*void scsp_write_rom(UINT8 ChipID, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					const UINT8* ROMData)
{
	scsp_state *scsp = &SCSPData[ChipID];
	
	if (scsp->SCSPRAM_LENGTH != ROMSize)
	{
		scsp->SCSPRAM = (unsigned char*)realloc(scsp->SCSPRAM, ROMSize);
		scsp->SCSPRAM_LENGTH = ROMSize;
		scsp->DSP.SCSPRAM = (UINT16*)scsp->SCSPRAM;
		scsp->DSP.SCSPRAM_LENGTH = scsp->SCSPRAM_LENGTH / 2;
		memset(scsp->SCSPRAM, 0x00, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(scsp->SCSPRAM + DataStart, ROMData, DataLength);
	
	return;
}*/

void scsp_write_ram(void *info, offs_t DataStart, offs_t DataLength, const UINT8* RAMData)
{
	if (DataStart >= SCSPRAM_LENGTH)
		return;
	if (DataStart + DataLength > SCSPRAM_LENGTH)
		DataLength = SCSPRAM_LENGTH - DataStart;
	
	memcpy(SCSPRAM + DataStart, RAMData, DataLength);
	
	return;
}


void scsp_set_mute_mask(void *info, UINT32 MuteMask)
{
	int CurChn;
	for (CurChn = 0; CurChn < 32; CurChn ++)
		yam_set_mute(YAMSTATE, CurChn, (MuteMask >> CurChn) & 0x01);
}

/*UINT8 scsp_get_channels(void *_info, UINT32* ChannelMask)
{
	scsp_state *scsp = (scsp_state *)_info;
	UINT8 CurChn;
	UINT8 UsedChns;
	UINT32 ChnMask;
	
	ChnMask = 0x00000000;
	UsedChns = 0x00;
	for (CurChn = 0; CurChn < 32; CurChn ++)
	{
		if (scsp->Slots[CurChn].active)
		{
			ChnMask |= (1 << CurChn);
			UsedChns ++;
		}
	}
	if (ChannelMask != NULL)
		*ChannelMask = ChnMask;
	
	return UsedChns;
}*/



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( scsp )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(scsp_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( scsp );		break;
		case DEVINFO_FCT_STOP:							// Nothing //								break;
		case DEVINFO_FCT_RESET:							// Nothing //								break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "SCSP");					break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Sega/Yamaha custom");		break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "2.1.1");					break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/


//DEFINE_LEGACY_SOUND_DEVICE(SCSP, scsp);
