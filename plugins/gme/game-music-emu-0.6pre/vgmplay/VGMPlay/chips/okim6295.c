/**********************************************************************************************
 *
 *   streaming ADPCM driver
 *   by Aaron Giles
 *
 *   Library to transcode from an ADPCM source to raw PCM.
 *   Written by Buffoni Mirko in 08/06/97
 *   References: various sources and documents.
 *
 *   HJB 08/31/98
 *   modified to use an automatically selected oversampling factor
 *   for the current sample rate
 *
 *   Mish 21/7/99
 *   Updated to allow multiple OKI chips with different sample rates
 *
 *   R. Belmont 31/10/2003
 *   Updated to allow a driver to use both MSM6295s and "raw" ADPCM voices (gcpinbal)
 *   Also added some error trapping for MAME_DEBUG builds
 *
 **********************************************************************************************/


#include "mamedef.h"
//#include "emu.h"
//#include "streams.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "okim6295.h"

#define FALSE	0
#define TRUE	1

//#define MAX_SAMPLE_CHUNK	10000
#define MAX_SAMPLE_CHUNK	0x10	// that's enough for VGMPlay's update rate


/* struct describing a single playing ADPCM voice */
struct ADPCMVoice
{
	UINT8 playing;			/* 1 if we are actively playing */

	UINT32 base_offset;		/* pointer to the base memory location */
	UINT32 sample;			/* current sample number */
	UINT32 count;			/* total samples to play */

	struct adpcm_state adpcm;/* current ADPCM state */
	UINT32 volume;			/* output volume */
	UINT8 Muted;
};

typedef struct _okim6295_state okim6295_state;
struct _okim6295_state
{
	#define OKIM6295_VOICES		4
	struct ADPCMVoice voice[OKIM6295_VOICES];
	//running_device *device;
	INT16 command;
	//UINT8 bank_installed;
	INT32 bank_offs;
	UINT8 pin7_state;
	UINT8 nmk_mode;
	UINT8 nmk_bank[4];
	//sound_stream *stream;	/* which stream are we playing on? */
	UINT32 master_clock;	/* master clock frequency */
	UINT32 initial_clock;
	
	UINT32	ROMSize;
	UINT8*	ROM;
	
	SRATE_CALLBACK SmpRateFunc;
	void* SmpRateData;
};

/* step size index shift table */
static const int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];

/* volume lookup table. The manual lists only 9 steps, ~3dB per step. Given the dB values,
   that seems to map to a 5-bit volume control. Any volume parameter beyond the 9th index
   results in silent playback. */
static const int volume_table[16] =
{
	0x20,	//   0 dB
	0x16,	//  -3.2 dB
	0x10,	//  -6.0 dB
	0x0b,	//  -9.2 dB
	0x08,	// -12.0 dB
	0x06,	// -14.5 dB
	0x04,	// -18.0 dB
	0x03,	// -20.5 dB
	0x02,	// -24.0 dB
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
};

/* tables computed? */
static int tables_computed = 0;

/* useful interfaces */
//const okim6295_interface okim6295_interface_pin7high = { 1 };
//const okim6295_interface okim6295_interface_pin7low = { 0 };

/* default address map */
/*static ADDRESS_MAP_START( okim6295, 0, 8 )
	AM_RANGE(0x00000, 0x3ffff) AM_ROM
ADDRESS_MAP_END*/


/*INLINE okim6295_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_OKIM6295);
	return (okim6295_state *)device->token;
}*/


/**********************************************************************************************

     compute_tables -- compute the difference tables

***********************************************************************************************/

static void compute_tables(void)
{
	/* nibble to bit map */
	static const int nbl2bit[16][4] =
	{
		{ 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
		{ 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
		{-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
		{-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
	};

	int step, nib;

	/* loop over all possible steps */
	for (step = 0; step <= 48; step++)
	{
		/* compute the step value */
		int stepval = (int)floor(16.0 * pow(11.0 / 10.0, (double)step));

		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval   * nbl2bit[nib][1] +
				 stepval/2 * nbl2bit[nib][2] +
				 stepval/4 * nbl2bit[nib][3] +
				 stepval/8);
		}
	}

	tables_computed = 1;
}



/**********************************************************************************************

     reset_adpcm -- reset the ADPCM stream

***********************************************************************************************/

void reset_adpcm(struct adpcm_state *state)
{
	/* make sure we have our tables */
	if (!tables_computed)
		compute_tables();

	/* reset the signal/step */
	state->signal = -2;
	state->step = 0;
}



/**********************************************************************************************

     clock_adpcm -- clock the next ADPCM byte

***********************************************************************************************/

INT16 clock_adpcm(struct adpcm_state *state, UINT8 nibble)
{
	state->signal += diff_lookup[state->step * 16 + (nibble & 15)];

	/* clamp to the maximum */
	if (state->signal > 2047)
		state->signal = 2047;
	else if (state->signal < -2048)
		state->signal = -2048;

	/* adjust the step size and clamp */
	state->step += index_shift[nibble & 7];
	if (state->step > 48)
		state->step = 48;
	else if (state->step < 0)
		state->step = 0;

	/* return the signal */
	return state->signal;
}



/**********************************************************************************************

     generate_adpcm -- general ADPCM decoding routine

***********************************************************************************************/

#define NMK_BNKTBLBITS	8
#define NMK_BNKTBLSIZE	(1 << NMK_BNKTBLBITS)	// 0x100
#define NMK_TABLESIZE	(4 * NMK_BNKTBLSIZE)	// 0x400
#define NMK_TABLEMASK	(NMK_TABLESIZE - 1)		// 0x3FF

#define NMK_BANKBITS	16
#define NMK_BANKSIZE	(1 << NMK_BANKBITS)		// 0x10000
#define NMK_BANKMASK	(NMK_BANKSIZE - 1)		// 0xFFFF
#define NMK_ROMBASE		(4 * NMK_BANKSIZE)		// 0x40000

static UINT8 memory_raw_read_byte(okim6295_state *chip, offs_t offset)
{
	offs_t CurOfs;
	
	if (! chip->nmk_mode)
	{
		CurOfs = chip->bank_offs | offset;
	}
	else
	{
		UINT8 BankID;
		
		if (offset < NMK_TABLESIZE && (chip->nmk_mode & 0x80))
		{
			// pages sample table
			BankID = offset >> NMK_BNKTBLBITS;
			CurOfs = offset & NMK_TABLEMASK;	// 0x3FF, not 0xFF
		}
		else
		{
			BankID = offset >> NMK_BANKBITS;
			CurOfs = offset & NMK_BANKMASK;
		}
		CurOfs |= (chip->nmk_bank[BankID & 0x03] << NMK_BANKBITS);
		// I modified MAME to write a clean sample ROM.
		// (Usually it moves the data by NMK_ROMBASE.)
		//CurOfs += NMK_ROMBASE;
	}
	if (CurOfs < chip->ROMSize)
		return chip->ROM[CurOfs];
	else
		return 0x00;
}

static void generate_adpcm(okim6295_state *chip, struct ADPCMVoice *voice, INT16 *buffer, int samples)
{
	/* if this voice is active */
	if (voice->playing)
	{
		offs_t base = voice->base_offset;
		int sample = voice->sample;
		int count = voice->count;

		/* loop while we still have samples to generate */
		while (samples)
		{
			/* compute the new amplitude and update the current step */
			//int nibble = memory_raw_read_byte(chip->device->space(), base + sample / 2) >> (((sample & 1) << 2) ^ 4);
			UINT8 nibble = memory_raw_read_byte(chip, base + sample / 2) >> (((sample & 1) << 2) ^ 4);

			/* output to the buffer, scaling by the volume */
			/* signal in range -2048..2047, volume in range 2..32 => signal * volume / 2 in range -32768..32767 */
			*buffer++ = clock_adpcm(&voice->adpcm, nibble) * voice->volume / 2;
			samples--;

			/* next! */
			if (++sample >= count)
			{
				voice->playing = 0;
				break;
			}
		}

		/* update the parameters */
		voice->sample = sample;
	}

	/* fill the rest with silence */
	while (samples--)
		*buffer++ = 0;
}



/**********************************************************************************************
 *
 *  OKIM 6295 ADPCM chip:
 *
 *  Command bytes are sent:
 *
 *      1xxx xxxx = start of 2-byte command sequence, xxxxxxx is the sample number to trigger
 *      abcd vvvv = second half of command; one of the abcd bits is set to indicate which voice
 *                  the v bits seem to be volumed
 *
 *      0abc d000 = stop playing; one or more of the abcd bits is set to indicate which voice(s)
 *
 *  Status is read:
 *
 *      ???? abcd = one bit per voice, set to 0 if nothing is playing, or 1 if it is active
 *
***********************************************************************************************/


/**********************************************************************************************

     okim6295_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

//static STREAM_UPDATE( okim6295_update )
void okim6295_update(void *param, stream_sample_t **outputs, int samples)
{
	okim6295_state *chip = (okim6295_state *)param;
	int i;

	memset(outputs[0], 0, samples * sizeof(*outputs[0]));

	for (i = 0; i < OKIM6295_VOICES; i++)
	{
		struct ADPCMVoice *voice = &chip->voice[i];
		if (! voice->Muted)
		{
			stream_sample_t *buffer = outputs[0];
			INT16 sample_data[MAX_SAMPLE_CHUNK];
			int remaining = samples;

			/* loop while we have samples remaining */
			while (remaining)
			{
				int samples = (remaining > MAX_SAMPLE_CHUNK) ? MAX_SAMPLE_CHUNK : remaining;
				int samp;

				generate_adpcm(chip, voice, sample_data, samples);
				for (samp = 0; samp < samples; samp++)
					*buffer++ += sample_data[samp];

				remaining -= samples;
			}
		}
	}
	
	memcpy(outputs[1], outputs[0], samples * sizeof(*outputs[0]));
}



/**********************************************************************************************

     state save support for MAME

***********************************************************************************************/

/*static void adpcm_state_save_register(struct ADPCMVoice *voice, running_device *device, int index)
{
	state_save_register_device_item(device, index, voice->playing);
	state_save_register_device_item(device, index, voice->sample);
	state_save_register_device_item(device, index, voice->count);
	state_save_register_device_item(device, index, voice->adpcm.signal);
	state_save_register_device_item(device, index, voice->adpcm.step);
	state_save_register_device_item(device, index, voice->volume);
	state_save_register_device_item(device, index, voice->base_offset);
}

static STATE_POSTLOAD( okim6295_postload )
{
	running_device *device = (running_device *)param;
	okim6295_state *info = get_safe_token(device);
	okim6295_set_bank_base(device, info->bank_offs);
}

static void okim6295_state_save_register(okim6295_state *info, running_device *device)
{
	int j;

	state_save_register_device_item(device, 0, info->command);
	state_save_register_device_item(device, 0, info->bank_offs);
	for (j = 0; j < OKIM6295_VOICES; j++)
		adpcm_state_save_register(&info->voice[j], device, j);

	state_save_register_postload(device->machine, okim6295_postload, (void *)device);
}*/



/**********************************************************************************************

     DEVICE_START( okim6295 ) -- start emulation of an OKIM6295-compatible chip

***********************************************************************************************/

//static DEVICE_START( okim6295 )
int device_start_okim6295(void **_info, int clock)
{
	//const okim6295_interface *intf = (const okim6295_interface *)device->baseconfig().static_config;
	//okim6295_state *info = get_safe_token(device);
	okim6295_state *info;
	//int divisor = intf->pin7 ? 132 : 165;
	int divisor;
	//int voice;

	info = (okim6295_state *) calloc(1, sizeof(okim6295_state));
	*_info = (void *) info;
	
	compute_tables();

	info->command = -1;
	//info->bank_installed = FALSE;
	info->bank_offs = 0;
	info->nmk_mode = 0x00;
	memset(info->nmk_bank, 0x00, 4 * sizeof(UINT8));
	//info->device = device;

	//info->master_clock = device->clock;
	info->initial_clock = clock;
	info->master_clock = clock & 0x7FFFFFFF;
	info->pin7_state = (clock & 0x80000000) >> 31;
	info->SmpRateFunc = NULL;

	/* generate the name and create the stream */
	divisor = info->pin7_state ? 132 : 165;
	//info->stream = stream_create(device, 0, 1, device->clock/divisor, info, okim6295_update);

	// moved to device_reset
	/*// initialize the voices //
	for (voice = 0; voice < OKIM6295_VOICES; voice++)
	{
		// initialize the rest of the structure //
		info->voice[voice].volume = 0;
		reset_adpcm(&info->voice[voice].adpcm);
	}*/

	//okim6295_state_save_register(info, device);

	return info->master_clock / divisor;
}



void device_stop_okim6295(void *_info)
{
	okim6295_state* chip = (okim6295_state *)_info;
	
	free(chip->ROM);	chip->ROM = NULL;
	chip->ROMSize = 0x00;

	free(chip);
	
	return;
}

/**********************************************************************************************

     DEVICE_RESET( okim6295 ) -- stop emulation of an OKIM6295-compatible chip

***********************************************************************************************/

//static DEVICE_RESET( okim6295 )
void device_reset_okim6295(void *_info)
{
	//okim6295_state *info = get_safe_token(device);
	okim6295_state *info = (okim6295_state *)_info;
	int voice;

	//stream_update(info->stream);
	
	info->command = -1;
	info->bank_offs = 0;
	info->nmk_mode = 0x00;
	memset(info->nmk_bank, 0x00, 4 * sizeof(UINT8));
	info->master_clock = info->initial_clock & 0x7FFFFFFF;
	info->pin7_state = (info->initial_clock & 0x80000000) >> 31;
	
	for (voice = 0; voice < OKIM6295_VOICES; voice++)
	{
		info->voice[voice].volume = 0;
		reset_adpcm(&info->voice[voice].adpcm);
		
		info->voice[voice].playing = 0;
	}
}



/**********************************************************************************************

     okim6295_set_bank_base -- set the base of the bank for a given voice on a given chip

***********************************************************************************************/

//void okim6295_set_bank_base(running_device *device, int base)
void okim6295_set_bank_base(okim6295_state *info, int base)
{
	//okim6295_state *info = get_safe_token(device);
	//stream_update(info->stream);

	// if we are setting a non-zero base, and we have no bank, allocate one
	/*if (!info->bank_installed && base != 0)
	{
		// override our memory map with a bank
		//memory_install_read_bank(device->space(), 0x00000, 0x3ffff, 0, 0, device->tag());
		info->bank_installed = TRUE;
	}

	// if we have a bank number, set the base pointer
	if (info->bank_installed)
	{
		info->bank_offs = base;
		//memory_set_bankptr(device->machine, device->tag(), device->region->base.u8 + base);
	}*/
	info->bank_offs = base;
}



/**********************************************************************************************

     okim6295_set_pin7 -- adjust pin 7, which controls the internal clock division

***********************************************************************************************/

static void okim6295_clock_changed(okim6295_state *info)
{
	int divisor;
	divisor = info->pin7_state ? 132 : 165;
	//stream_set_sample_rate(info->stream, info->master_clock/divisor);
	if (info->SmpRateFunc != NULL)
		info->SmpRateFunc(info->SmpRateData, info->master_clock / divisor);
}

//void okim6295_set_pin7(running_device *device, int pin7)
INLINE void okim6295_set_pin7(okim6295_state *info, int pin7)
{
	//okim6295_state *info = get_safe_token(device);

	info->pin7_state = pin7;
	okim6295_clock_changed(info);
}


/**********************************************************************************************

     okim6295_status_r -- read the status port of an OKIM6295-compatible chip

***********************************************************************************************/

//READ8_DEVICE_HANDLER( okim6295_r )
UINT8 okim6295_r(void *_info, offs_t offset)
{
	//okim6295_state *info = get_safe_token(device);
	okim6295_state *info = (okim6295_state *)_info;
	int i, result;

	result = 0xf0;	/* naname expects bits 4-7 to be 1 */

	/* set the bit to 1 if something is playing on a given channel */
	//stream_update(info->stream);
	for (i = 0; i < OKIM6295_VOICES; i++)
	{
		struct ADPCMVoice *voice = &info->voice[i];

		/* set the bit if it's playing */
		if (voice->playing)
			result |= 1 << i;
	}

	return result;
}



/**********************************************************************************************

     okim6295_data_w -- write to the data port of an OKIM6295-compatible chip

***********************************************************************************************/

//WRITE8_DEVICE_HANDLER( okim6295_w )
void okim6295_write_command(okim6295_state *info, UINT8 data)
{
	//okim6295_state *info = get_safe_token(device);

	/* if a command is pending, process the second half */
	if (info->command != -1)
	{
		int temp = data >> 4, i, start, stop;
		offs_t base;

		/* the manual explicitly says that it's not possible to start multiple voices at the same time */
		if (temp != 0 && temp != 1 && temp != 2 && temp != 4 && temp != 8)
			printf("OKI6295 start %x contact MAMEDEV\n", temp);

		/* update the stream */
		//stream_update(info->stream);

		/* determine which voice(s) (voice is set by a 1 bit in the upper 4 bits of the second byte) */
		for (i = 0; i < OKIM6295_VOICES; i++, temp >>= 1)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = &info->voice[i];

				/* determine the start/stop positions */
				base = info->command * 8;

				//start  = memory_raw_read_byte(device->space(), base + 0) << 16;
				start  = memory_raw_read_byte(info, base + 0) << 16;
				start |= memory_raw_read_byte(info, base + 1) << 8;
				start |= memory_raw_read_byte(info, base + 2) << 0;
				start &= 0x3ffff;

				stop  = memory_raw_read_byte(info, base + 3) << 16;
				stop |= memory_raw_read_byte(info, base + 4) << 8;
				stop |= memory_raw_read_byte(info, base + 5) << 0;
				stop &= 0x3ffff;

				/* set up the voice to play this sample */
				if (start < stop)
				{
					if (!voice->playing) /* fixes Got-cha and Steel Force */
					{
						voice->playing = 1;
						voice->base_offset = start;
						voice->sample = 0;
						voice->count = 2 * (stop - start + 1);

						/* also reset the ADPCM parameters */
						reset_adpcm(&voice->adpcm);
						voice->volume = volume_table[data & 0x0f];
					}
					else
					{
						//logerror("OKIM6295:'%s' requested to play sample %02x on non-stopped voice\n",device->tag(),info->command);
						// just displays warnings when seeking
						//logerror("OKIM6295: Voice %u requested to play sample %02x on non-stopped voice\n",i,info->command);
					}
				}
				/* invalid samples go here */
				else
				{
					//logerror("OKIM6295:'%s' requested to play invalid sample %02x\n",device->tag(),info->command);
#ifdef _DEBUG
					logerror("OKIM6295: Voice %u  requested to play invalid sample %02x\n",i,info->command);
#endif
					voice->playing = 0;
				}
			}
		}

		/* reset the command */
		info->command = -1;
	}

	/* if this is the start of a command, remember the sample number for next time */
	else if (data & 0x80)
	{
		info->command = data & 0x7f;
	}

	/* otherwise, see if this is a silence command */
	else
	{
		int temp = data >> 3, i;

		/* update the stream, then turn it off */
		//stream_update(info->stream);

		/* determine which voice(s) (voice is set by a 1 bit in bits 3-6 of the command */
		for (i = 0; i < OKIM6295_VOICES; i++, temp >>= 1)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = &info->voice[i];

				voice->playing = 0;
			}
		}
	}
}

void okim6295_w(void *_info, offs_t offset, UINT8 data)
{
	okim6295_state* chip = (okim6295_state *)_info;
	
	switch(offset)
	{
	case 0x00:
		okim6295_write_command(chip, data);
		break;
	case 0x08:
		chip->master_clock &= ~0x000000FF;
		chip->master_clock |= data <<  0;
		break;
	case 0x09:
		chip->master_clock &= ~0x0000FF00;
		chip->master_clock |= data <<  8;
		break;
	case 0x0A:
		chip->master_clock &= ~0x00FF0000;
		chip->master_clock |= data << 16;
		break;
	case 0x0B:
		if ((data >> 7) != chip->pin7_state)
			printf("Pin 7 changed!\n");
		data &= 0x7F;	// fix a bug in MAME VGM logs
		chip->master_clock &= ~0xFF000000;
		chip->master_clock |= data << 24;
		okim6295_clock_changed(chip);
		break;
	case 0x0C:
		okim6295_set_pin7(chip, data);
		break;
	case 0x0E:	// NMK112 bank switch enable
		chip->nmk_mode = data;
		break;
	case 0x0F:
		okim6295_set_bank_base(chip, data << 18);
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		chip->nmk_bank[offset & 0x03] = data;
		break;
	}
	
	return;
}

void okim6295_write_rom(void *_info, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
						const UINT8* ROMData)
{
	okim6295_state *chip = (okim6295_state *)_info;
	
	if (chip->ROMSize != ROMSize)
	{
		chip->ROM = (UINT8*)realloc(chip->ROM, ROMSize);
		chip->ROMSize = ROMSize;
		//printf("OKIM6295: New ROM Size: 0x%05X\n", ROMSize);
		memset(chip->ROM, 0xFF, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(chip->ROM + DataStart, ROMData, DataLength);
	
	return;
}


void okim6295_set_mute_mask(void *_info, UINT32 MuteMask)
{
	okim6295_state *chip = (okim6295_state *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < OKIM6295_VOICES; CurChn ++)
		chip->voice[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}

void okim6295_set_srchg_cb(void *_info, SRATE_CALLBACK CallbackFunc, void* DataPtr)
{
	okim6295_state *info = (okim6295_state *)_info;
	
	// set Sample Rate Change Callback routine
	info->SmpRateFunc = CallbackFunc;
	info->SmpRateData = DataPtr;
	
	return;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( okim6295 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers --- //
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(okim6295_state);				break;
		case DEVINFO_INT_DATABUS_WIDTH_0:			info->i = 8;									break;
		case DEVINFO_INT_ADDRBUS_WIDTH_0:			info->i = 18;									break;
		case DEVINFO_INT_ADDRBUS_SHIFT_0:			info->i = 0;									break;

		// --- the following bits of info are returned as pointers to data --- //
		case DEVINFO_PTR_DEFAULT_MEMORY_MAP_0:		info->default_map8 = ADDRESS_MAP_NAME(okim6295);break;

		// --- the following bits of info are returned as pointers to functions --- //
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME( okim6295 );	break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( okim6295 );	break;

		// --- the following bits of info are returned as NULL-terminated strings --- //
		case DEVINFO_STR_NAME:						strcpy(info->s, "OKI6295");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "OKI ADPCM");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/
