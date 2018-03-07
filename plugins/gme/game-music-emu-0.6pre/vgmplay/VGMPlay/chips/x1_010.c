/***************************************************************************

                            -= Seta Hardware =-

                    driver by   Luca Elia (l.elia@tin.it)

                    rewrite by Manbow-J(manbowj@hamal.freemail.ne.jp)

                    X1-010 Seta Custom Sound Chip (80 Pin PQFP)

 Custom programmed Mitsubishi M60016 Gate Array, 3608 gates, 148 Max I/O ports

    The X1-010 is 16 Voices sound generator, each channel gets it's
    waveform from RAM (128 bytes per waveform, 8 bit unsigned data)
    or sampling PCM(8bit unsigned data).

Registers:
    8 registers per channel (mapped to the lower bytes of 16 words on the 68K)

    Reg:    Bits:       Meaning:

    0       7654 3---
            ---- -2--   PCM/Waveform repeat flag (0:Ones 1:Repeat) (*1)
            ---- --1-   Sound out select (0:PCM 1:Waveform)
            ---- ---0   Key on / off

    1       7654 ----   PCM Volume 1 (L?)
            ---- 3210   PCM Volume 2 (R?)
                        Waveform No.

    2                   PCM Frequency
                        Waveform Pitch Lo

    3                   Waveform Pitch Hi

    4                   PCM Sample Start / 0x1000           [Start/End in bytes]
                        Waveform Envelope Time

    5                   PCM Sample End 0x100 - (Sample End / 0x1000)    [PCM ROM is Max 1MB?]
                        Waveform Envelope No.
    6                   Reserved
    7                   Reserved

    offset 0x0000 - 0x0fff  Wave form data
    offset 0x1000 - 0x1fff  Envelope data

    *1 : when 0 is specified, hardware interrupt is caused(allways return soon)

***************************************************************************/

//#include "emu.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>	// for NULL
#include "mamedef.h"
#include "x1_010.h"


#define VERBOSE_SOUND 0
#define VERBOSE_REGISTER_WRITE 0
#define VERBOSE_REGISTER_READ 0

#define LOG_SOUND(x) do { if (VERBOSE_SOUND) logerror x; } while (0)
#define LOG_REGISTER_WRITE(x) do { if (VERBOSE_REGISTER_WRITE) logerror x; } while (0)
#define LOG_REGISTER_READ(x) do { if (VERBOSE_REGISTER_READ) logerror x; } while (0)

#define SETA_NUM_CHANNELS 16

//#define FREQ_BASE_BITS		  8					// Frequency fixed decimal shift bits
#define FREQ_BASE_BITS		  14					// Frequency fixed decimal shift bits
#define ENV_BASE_BITS		 16					// wave form envelope fixed decimal shift bits
#define	VOL_BASE	(2*32*256/30)					// Volume base

/* this structure defines the parameters for a channel */
typedef struct {
	unsigned char	status;
	unsigned char	volume;						//        volume / wave form no.
	unsigned char	frequency;					//     frequency / pitch lo
	unsigned char	pitch_hi;					//      reserved / pitch hi
	unsigned char	start;						// start address / envelope time
	unsigned char	end;						//   end address / envelope no.
	unsigned char	reserve[2];
} X1_010_CHANNEL;

typedef struct _x1_010_state x1_010_state;
struct _x1_010_state
{
	/* Variables only used here */
	int	rate;								// Output sampling rate (Hz)
	//sound_stream *	stream;					// Stream handle
	//int	address;							// address eor data
	//const UINT8 *region;					// region name
	UINT32 ROMSize;
	UINT8* rom;
	int	sound_enable;						// sound output enable/disable
	UINT8	reg[0x2000];				// X1-010 Register & wave form area
//	UINT8	HI_WORD_BUF[0x2000];			// X1-010 16bit access ram check avoidance work
	UINT32	smp_offset[SETA_NUM_CHANNELS];
	UINT32	env_offset[SETA_NUM_CHANNELS];

	UINT32 base_clock;
	
	UINT8 Muted[SETA_NUM_CHANNELS];
};

/* mixer tables and internal buffers */
//static short  *mixer_buffer = NULL;

/*INLINE x1_010_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == X1_010);
	return (x1_010_state *)downcast<legacy_device_base *>(device)->token();
}*/


/*--------------------------------------------------------------
 generate sound to the mix buffer
--------------------------------------------------------------*/
//static STREAM_UPDATE( seta_update )
void seta_update(void *param, stream_sample_t **outputs, int samples)
{
	x1_010_state *info = (x1_010_state *)param;
	X1_010_CHANNEL	*reg;
	int		ch, i, volL, volR, freq, div;
	register INT8	*start, *end, data;
	register UINT8	*env;
	register UINT32	smp_offs, smp_step, env_offs, env_step, delta;

	// mixer buffer zero clear
	memset( outputs[0], 0, samples*sizeof(*outputs[0]) );
	memset( outputs[1], 0, samples*sizeof(*outputs[1]) );

//  if( info->sound_enable == 0 ) return;

	for( ch = 0; ch < SETA_NUM_CHANNELS; ch++ ) {
		reg = (X1_010_CHANNEL *)&(info->reg[ch*sizeof(X1_010_CHANNEL)]);
		if( (reg->status&1) != 0 && ! info->Muted[ch]) {		// Key On
			stream_sample_t *bufL = outputs[0];
			stream_sample_t *bufR = outputs[1];
			div = (reg->status&0x80) ? 1 : 0;
			if( (reg->status&2) == 0 ) {						// PCM sampling
				start    = (INT8 *)(info->rom + reg->start*0x1000);
				end      = (INT8 *)(info->rom + (0x100-reg->end)*0x1000);
				volL     = ((reg->volume>>4)&0xf)*VOL_BASE;
				volR     = ((reg->volume>>0)&0xf)*VOL_BASE;
				smp_offs = info->smp_offset[ch];
				freq     = reg->frequency>>div;
				// Meta Fox does write the frequency register, but this is a hack to make it "work" with the current setup
				// This is broken for Arbalester (it writes 8), but that'll be fixed later.
				if( freq == 0 ) freq = 4;
				smp_step = (UINT32)((float)info->base_clock/8192.0f
							*freq*(1<<FREQ_BASE_BITS)/(float)info->rate+0.5f);
#ifdef _DEBUG
				if( smp_offs == 0 ) {
					LOG_SOUND(( "Play sample %p - %p, channel %X volume %d:%d freq %X step %X offset %X\n",
						start, end, ch, volL, volR, freq, smp_step, smp_offs ));
				}
#endif
				for( i = 0; i < samples; i++ ) {
					delta = smp_offs>>FREQ_BASE_BITS;
					// sample ended?
					if( start+delta >= end ) {
						reg->status &= ~0x01;					// Key off
						break;
					}
					data = *(start+delta);
					*bufL++ += (data*volL/256);
					*bufR++ += (data*volR/256);
					smp_offs += smp_step;
				}
				info->smp_offset[ch] = smp_offs;
			} else {											// Wave form
				start    = (INT8 *)&(info->reg[reg->volume*128+0x1000]);
				smp_offs = info->smp_offset[ch];
				freq     = ((reg->pitch_hi<<8)+reg->frequency)>>div;
				smp_step = (UINT32)((float)info->base_clock/128.0/1024.0/4.0*freq*(1<<FREQ_BASE_BITS)/(float)info->rate+0.5f);

				env      = (UINT8 *)&(info->reg[reg->end*128]);
				env_offs = info->env_offset[ch];
				env_step = (UINT32)((float)info->base_clock/128.0/1024.0/4.0*reg->start*(1<<ENV_BASE_BITS)/(float)info->rate+0.5f);
				/* Print some more debug info */
#ifdef _DEBUG
				if( smp_offs == 0 ) {
					LOG_SOUND(( "Play waveform %X, channel %X volume %X freq %4X step %X offset %X\n",
						reg->volume, ch, reg->end, freq, smp_step, smp_offs ));
				}
#endif
				for( i = 0; i < samples; i++ ) {
					int vol;
					delta = env_offs>>ENV_BASE_BITS;
					// Envelope one shot mode
					if( (reg->status&4) != 0 && delta >= 0x80 ) {
						reg->status &= ~0x01;					// Key off
						break;
					}
					vol = *(env+(delta&0x7f));
					volL = ((vol>>4)&0xf)*VOL_BASE;
					volR = ((vol>>0)&0xf)*VOL_BASE;
					data  = *(start+((smp_offs>>FREQ_BASE_BITS)&0x7f));
					*bufL++ += (data*volL/256);
					*bufR++ += (data*volR/256);
					smp_offs += smp_step;
					env_offs += env_step;
				}
				info->smp_offset[ch] = smp_offs;
				info->env_offset[ch] = env_offs;
			}
		}
	}
}



//static DEVICE_START( x1_010 )
int device_start_x1_010(void **_info, int clock, int CHIP_SAMPLING_MODE, int CHIP_SAMPLE_RATE)
{
	int i;
	//const x1_010_interface *intf = (const x1_010_interface *)device->static_config();
	//x1_010_state *info = get_safe_token(device);
	x1_010_state *info;

	info = (x1_010_state *) calloc(1, sizeof(x1_010_state));
	*_info = (void *) info;
	
	//info->region		= *device->region();
	//info->base_clock	= device->clock();
	//info->rate			= device->clock() / 1024;
	//info->address		= intf->adr;
	info->ROMSize		= 0x00;
	info->rom			= NULL;
	info->base_clock	= clock;
	info->rate			= clock / 1024;
	if (((CHIP_SAMPLING_MODE & 0x01) && info->rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		info->rate = CHIP_SAMPLE_RATE;

	for( i = 0; i < SETA_NUM_CHANNELS; i++ ) {
		info->smp_offset[i] = 0;
		info->env_offset[i] = 0;
	}
	/* Print some more debug info */
	//LOG_SOUND(("masterclock = %d rate = %d\n", device->clock(), info->rate ));

	/* get stream channels */
	//info->stream = device->machine().sound().stream_alloc(*device,0,2,info->rate,info,seta_update);
	return info->rate;
}

void device_stop_x1_010(void *_info)
{
	x1_010_state *info = (x1_010_state *)_info;
	
	free(info->rom);	info->rom = NULL;

	free(info);
	
	return;
}

void device_reset_x1_010(void *_info)
{
	x1_010_state *info = (x1_010_state *)_info;
	
	memset(info->reg, 0, 0x2000);
	//memset(info->HI_WORD_BUF, 0, 0x2000);
	memset(info->smp_offset, 0, SETA_NUM_CHANNELS * sizeof(UINT32));
	memset(info->env_offset, 0, SETA_NUM_CHANNELS * sizeof(UINT32));
	
	return;
}


/*void seta_sound_enable_w(device_t *device, int data)
{
	x1_010_state *info = get_safe_token(device);
	info->sound_enable = data;
}*/



/* Use these for 8 bit CPUs */


//READ8_DEVICE_HANDLER( seta_sound_r )
UINT8 seta_sound_r(void *_info, offs_t offset)
{
	//x1_010_state *info = get_safe_token(device);
	x1_010_state *info = (x1_010_state *)_info;
	//offset ^= info->address;
	return info->reg[offset];
}




//WRITE8_DEVICE_HANDLER( seta_sound_w )
void seta_sound_w(void *_info, offs_t offset, UINT8 data)
{
	//x1_010_state *info = get_safe_token(device);
	x1_010_state *info = (x1_010_state *)_info;
	int channel, reg;
	//offset ^= info->address;

	channel	= offset/sizeof(X1_010_CHANNEL);
	reg		= offset%sizeof(X1_010_CHANNEL);

	if( channel < SETA_NUM_CHANNELS && reg == 0
	 && (info->reg[offset]&1) == 0 && (data&1) != 0 ) {
		info->smp_offset[channel] = 0;
		info->env_offset[channel] = 0;
	}
	//LOG_REGISTER_WRITE(("%s: offset %6X : data %2X\n", device->machine().describe_context(), offset, data ));
	info->reg[offset] = data;
}

void x1_010_write_rom(void *_info, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
						const UINT8* ROMData)
{
	x1_010_state *info = (x1_010_state *)_info;
	
	if (info->ROMSize != ROMSize)
	{
		info->rom = (UINT8*)realloc(info->rom, ROMSize);
		info->ROMSize = ROMSize;
		memset(info->rom, 0xFF, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(info->rom + DataStart, ROMData, DataLength);
	
	return;
}


void x1_010_set_mute_mask(void *_info, UINT32 MuteMask)
{
	x1_010_state *info = (x1_010_state *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < SETA_NUM_CHANNELS; CurChn ++)
		info->Muted[CurChn] = (MuteMask >> CurChn) & 0x01;
	
	return;
}




/* Use these for 16 bit CPUs */

/*READ16_DEVICE_HANDLER( seta_sound_word_r )
{
	//x1_010_state *info = get_safe_token(device);
	x1_010_state *info = &X1010Data[ChipID];
	UINT16	ret;

	ret = info->HI_WORD_BUF[offset]<<8;
	ret += (seta_sound_r( device, offset )&0xff);
	LOG_REGISTER_READ(( "%s: Read X1-010 Offset:%04X Data:%04X\n", device->machine().describe_context(), offset, ret ));
	return ret;
}

WRITE16_DEVICE_HANDLER( seta_sound_word_w )
{
	//x1_010_state *info = get_safe_token(device);
	x1_010_state *info = &X1010Data[ChipID];
	info->HI_WORD_BUF[offset] = (data>>8)&0xff;
	seta_sound_w( device, offset, data&0xff );
	LOG_REGISTER_WRITE(( "%s: Write X1-010 Offset:%04X Data:%04X\n", device->machine().describe_context(), offset, data ));
}*/



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( x1_010 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(x1_010_state); 			break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( x1_010 );			break;
		case DEVINFO_FCT_STOP:							// Nothing //									break;
		case DEVINFO_FCT_RESET:							// Nothing //									break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "X1-010");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Seta custom");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}


DEFINE_LEGACY_SOUND_DEVICE(X1_010, x1_010);*/
