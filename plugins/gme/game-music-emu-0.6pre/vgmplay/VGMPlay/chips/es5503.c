/*

  ES5503 - Ensoniq ES5503 "DOC" emulator v2.1.1
  By R. Belmont.

  Copyright R. Belmont.

  This software is dual-licensed: it may be used in MAME and properly licensed
  MAME derivatives under the terms of the MAME license.  For use outside of
  MAME and properly licensed derivatives, it is available under the
  terms of the GNU Lesser General Public License (LGPL), version 2.1.
  You may read the LGPL at http://www.gnu.org/licenses/lgpl.html

  History: the ES5503 was the next design after the famous C64 "SID" by Bob Yannes.
  It powered the legendary Mirage sampler (the first affordable pro sampler) as well
  as the ESQ-1 synth/sequencer.  The ES5505 (used in Taito's F3 System) and 5506
  (used in the "Soundscape" series of ISA PC sound cards) followed on a fundamentally
  similar architecture.

  Bugs: On the real silicon, oscillators 30 and 31 have random volume fluctuations and are
  unusable for playback.  We don't attempt to emulate that. :-)

  Additionally, in "swap" mode, there's one cycle when the switch takes place where the
  oscillator's output is 0x80 (centerline) regardless of the sample data.  This can
  cause audible clicks and a general degradation of audio quality if the correct sample
  data at that point isn't 0x80 or very near it.

  Changes:
  0.2 (RB) - improved behavior for volumes > 127, fixes missing notes in Nucleus & missing voices in Thexder
  0.3 (RB) - fixed extraneous clicking, improved timing behavior for e.g. Music Construction Set & Music Studio
  0.4 (RB) - major fixes to IRQ semantics and end-of-sample handling.
  0.5 (RB) - more flexible wave memory hookup (incl. banking) and save state support.
  1.0 (RB) - properly respects the input clock
  2.0 (RB) - C++ conversion, more accurate oscillator IRQ timing
  2.1 (RB) - Corrected phase when looping; synthLAB, Arkanoid, and Arkanoid II no longer go out of tune
  2.1.1 (RB) - Fixed issue introduced in 2.0 where IRQs were delayed
*/

//#include "emu.h"
//#include "streams.h"
#include <stdlib.h>
#include <string.h>
#include "mamedef.h"
#include "es5503.h"

typedef struct
{
	UINT16 freq;
	UINT16 wtsize;
	UINT8  control;
	UINT8  vol;
	UINT8  data;
	UINT32 wavetblpointer;
	UINT8  wavetblsize;
	UINT8  resolution;

	UINT32 accumulator;
	UINT8  irqpend;
	
	UINT8  Muted;
} ES5503Osc;

typedef struct
{
	ES5503Osc oscillators[32];

	UINT32 dramsize;
	UINT8 *docram;

	//sound_stream * stream;

	//void (*irq_callback)(running_device *, int);	// IRQ callback

	//read8_device_func adc_read;		// callback for the 5503's built-in analog to digital converter

	INT8  oscsenabled;		// # of oscillators enabled
	int   rege0;			// contents of register 0xe0

	UINT8 channel_strobe;

	UINT32 clock;
	int output_channels;
	int outchn_mask;
	UINT32 output_rate;
	//running_device *device;
	
	SRATE_CALLBACK SmpRateFunc;
	void* SmpRateData;
} ES5503Chip;

/*INLINE ES5503Chip *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == ES5503);
	return (ES5503Chip *)downcast<legacy_device_base *>(device)->token();
}*/

static const UINT16 wavesizes[8] = { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };
static const UINT32 wavemasks[8] = { 0x1ff00, 0x1fe00, 0x1fc00, 0x1f800, 0x1f000, 0x1e000, 0x1c000, 0x18000 };
static const UINT32 accmasks[8]  = { 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff };
static const int    resshifts[8] = { 9, 10, 11, 12, 13, 14, 15, 16 };

enum
{
	MODE_FREE = 0,
	MODE_ONESHOT = 1,
	MODE_SYNCAM = 2,
	MODE_SWAP = 3
};

// halt_osc: handle halting an oscillator
// chip = chip ptr
// onum = oscillator #
// type = 1 for 0 found in sample data, 0 for hit end of table size
static void es5503_halt_osc(ES5503Chip *chip, int onum, int type, UINT32 *accumulator, int resshift)
{
	ES5503Osc *pOsc = &chip->oscillators[onum];
	ES5503Osc *pPartner = &chip->oscillators[onum^1];
	int mode = (pOsc->control>>1) & 3;
	//int omode = (pPartner->control>>1) & 3;

	// if 0 found in sample data or mode is not free-run, halt this oscillator
	if ((mode != MODE_FREE) || (type != 0))
	{
		pOsc->control |= 1;
	}
	else    // preserve the relative phase of the oscillator when looping
	{
		UINT16 wtsize = pOsc->wtsize - 1;
		UINT32 altram = (*accumulator) >> resshift;

		if (altram > wtsize)
		{
			altram -= wtsize;
		}
		else
		{
			altram = 0;
		}

		*accumulator = altram << resshift;
	}

	// if swap mode, start the partner
	// Note: The swap mode fix breaks Silpheed and other games.
	if (/*(*/mode == MODE_SWAP/*)*/ /*|| (omode == MODE_SWAP)*/)
	{
		pPartner->control &= ~1;	// clear the halt bit
		pPartner->accumulator = 0;  // and make sure it starts from the top (does this also need phase preservation?)
	}

	// IRQ enabled for this voice?
	if (pOsc->control & 0x08)
	{
		pOsc->irqpend = 1;

		/*if (chip->irq_callback)
		{
			chip->irq_callback(chip->device, 1);
		}*/
	}
}

//static STREAM_UPDATE( es5503_pcm_update )
void es5503_pcm_update(void *param, stream_sample_t **outputs, int samples)
{
	// Note: The advantage of NOT using this buffer is not only less RAM usage,
	//       but also a huge speedup. This is, because the array is not marked
	//       as 'static' and thus re-allocated for every single call.
	//INT32 mix[48000*2];
	//INT32 *mixp;
	int osc, snum;
	UINT32 ramptr;
	ES5503Chip *chip = (ES5503Chip *)param;
	int chnsStereo, chan;

	memset(outputs[0], 0x00, samples * sizeof(stream_sample_t));
	memset(outputs[1], 0x00, samples * sizeof(stream_sample_t));
	//memset(mix, 0, sizeof(mix));

	chnsStereo = chip->output_channels & ~1;
	for (osc = 0; osc < chip->oscsenabled; osc++)
	{
		ES5503Osc *pOsc = &chip->oscillators[osc];

		if (!(pOsc->control & 1) && ! pOsc->Muted)
		{
			UINT32 wtptr = pOsc->wavetblpointer & wavemasks[pOsc->wavetblsize], altram;
			UINT32 acc = pOsc->accumulator;
			UINT16 wtsize = pOsc->wtsize - 1;
			//UINT8 ctrl = pOsc->control;
			UINT16 freq = pOsc->freq;
			INT16 vol = pOsc->vol;
			//INT8 data = -128;
			UINT8 chnMask = (pOsc->control >> 4) & 0x0F;
			int resshift = resshifts[pOsc->resolution] - pOsc->wavetblsize;
			UINT32 sizemask = accmasks[pOsc->wavetblsize];
			INT32 outData;
			//mixp = &mix[0] + chan;

			chnMask &= chip->outchn_mask;
			for (snum = 0; snum < samples; snum++)
			{
				altram = acc >> resshift;
				ramptr = altram & sizemask;

				acc += freq;

				// channel strobe is always valid when reading; this allows potentially banking per voice
				//chip->channel_strobe = (pOsc->control>>4) & 0xf;
				//data = (INT32)chip->docram[ramptr + wtptr] ^ 0x80;
				pOsc->data = chip->docram[ramptr + wtptr];

				if (pOsc->data == 0x00)
				{
					es5503_halt_osc(chip, osc, 1, &acc, resshift);
				}
				else
				{
					outData = (pOsc->data - 0x80) * vol;
					//*mixp += outData;
					//mixp += output_channels;
					
					// send groups of 2 channels to L or R
					for (chan = 0; chan < chnsStereo; chan ++)
					{
						if (chan == chnMask)
							outputs[chan & 1][snum] += outData;
					}
					outData = (outData * 181) >> 8;	// outData *= sqrt(2)
					// send remaining channels to L+R
					for (; chan < chip->output_channels; chan ++)
					{
						if (chan == chnMask)
						{
							outputs[0][snum] += outData;
							outputs[1][snum] += outData;
						}
					}

					if (altram >= wtsize)
					{
						es5503_halt_osc(chip, osc, 0, &acc, resshift);
					}
				}

				// if oscillator halted, we've got no more samples to generate
				if (pOsc->control & 1)
				{
					//pOsc->control |= 1;
					break;
				}
			}

			//pOsc->control = ctrl;
			pOsc->accumulator = acc;
			//pOsc->data = data ^ 0x80;
		}
	}

/*	mixp = &mix[0];
	for (i = 0; i < samples; i++)
		for (int chan = 0; chan < output_channels; chan++)
			outputs[chan][i] = (*mixp++)>>1;*/
}


//static DEVICE_START( es5503 )
int device_start_es5503(void **_info, int clock, int channels)
{
	//const es5503_interface *intf;
	int osc;
	//ES5503Chip *chip = get_safe_token(device);
	ES5503Chip *chip;

	chip = (ES5503Chip *) calloc(1, sizeof(ES5503Chip));
	*_info = (void *) chip;
	
	//intf = (const es5503_interface *)device->baseconfig().static_config();

	//chip->irq_callback = intf->irq_callback;
	//chip->adc_read = intf->adc_read;
	//chip->docram = intf->wave_memory;
	chip->dramsize = 0x20000;	// 128 KB
	chip->docram = (UINT8*)malloc(chip->dramsize);
	//chip->clock = device->clock();
	//chip->device = device;
	chip->clock = clock;

	chip->output_channels = channels;
	chip->outchn_mask = 1;
	while(chip->outchn_mask < chip->output_channels)
		chip->outchn_mask <<= 1;
	chip->outchn_mask --;
	chip->rege0 = 0xff;

	/*for (osc = 0; osc < 32; osc++)
	{
		state_save_register_device_item(device, osc, chip->oscillators[osc].freq);
		state_save_register_device_item(device, osc, chip->oscillators[osc].wtsize);
		state_save_register_device_item(device, osc, chip->oscillators[osc].control);
		state_save_register_device_item(device, osc, chip->oscillators[osc].vol);
		state_save_register_device_item(device, osc, chip->oscillators[osc].data);
		state_save_register_device_item(device, osc, chip->oscillators[osc].wavetblpointer);
		state_save_register_device_item(device, osc, chip->oscillators[osc].wavetblsize);
		state_save_register_device_item(device, osc, chip->oscillators[osc].resolution);
		state_save_register_device_item(device, osc, chip->oscillators[osc].accumulator);
		state_save_register_device_item(device, osc, chip->oscillators[osc].irqpend);
	}*/

	//chip->output_rate = (device->clock()/8)/34;	// (input clock / 8) / # of oscs. enabled + 2
	//chip->stream = stream_create(device, 0, 2, chip->output_rate, chip, es5503_pcm_update);
	chip->output_rate = (chip->clock/8)/34;	// (input clock / 8) / # of oscs. enabled + 2
	
	for (osc = 0; osc < 32; osc ++)
		chip->oscillators[osc].Muted = 0x00;
	
	return chip->output_rate;
}

void device_stop_es5503(void *_info)
{
	ES5503Chip *chip = (ES5503Chip *)_info;
	
	free(chip->docram);	chip->docram = NULL;

	free(chip);	

	return;
}

void device_reset_es5503(void *_info)
{
	ES5503Chip *chip = (ES5503Chip *)_info;
	int osc;
	ES5503Osc* tempOsc;
	
	for (osc = 0; osc < 32; osc++)
	{
		tempOsc = &chip->oscillators[osc];
		tempOsc->freq = 0;
		tempOsc->wtsize = 0;
		tempOsc->control = 0;
		tempOsc->vol = 0;
		tempOsc->data = 0x80;
		tempOsc->wavetblpointer = 0;
		tempOsc->wavetblsize = 0;
		tempOsc->resolution = 0;
		tempOsc->accumulator = 0;
		tempOsc->irqpend = 0;
	}
	
	chip->oscsenabled = 1;
	
	chip->channel_strobe = 0;
	memset(chip->docram, 0x00, chip->dramsize);
	
	chip->output_rate = (chip->clock/8)/(2+chip->oscsenabled);	// (input clock / 8) / # of oscs. enabled + 2
	if (chip->SmpRateFunc != NULL)
		chip->SmpRateFunc(chip->SmpRateData, chip->output_rate);
	
	return;
}


//READ8_DEVICE_HANDLER( es5503_r )
UINT8 es5503_r(void *_info, offs_t offset)
{
	UINT8 retval;
	int i;
	//ES5503Chip *chip = get_safe_token(device);
	ES5503Chip *chip = (ES5503Chip *)_info;

	//stream_update(chip->stream);

	if (offset < 0xe0)
	{
		int osc = offset & 0x1f;

		switch(offset & 0xe0)
		{
			case 0:		// freq lo
				return (chip->oscillators[osc].freq & 0xff);

			case 0x20:  	// freq hi
				return (chip->oscillators[osc].freq >> 8);

			case 0x40:	// volume
				return chip->oscillators[osc].vol;

			case 0x60:	// data
				return chip->oscillators[osc].data;

			case 0x80:	// wavetable pointer
				return (chip->oscillators[osc].wavetblpointer>>8) & 0xff;

			case 0xa0:	// oscillator control
				return chip->oscillators[osc].control;

			case 0xc0:	// bank select / wavetable size / resolution
				retval = 0;
				if (chip->oscillators[osc].wavetblpointer & 0x10000)
				{
					retval |= 0x40;
				}

				retval |= (chip->oscillators[osc].wavetblsize<<3);
				retval |= chip->oscillators[osc].resolution;
				return retval;
		}
	}
	else	 // global registers
	{
		switch (offset)
		{
			case 0xe0:	// interrupt status
				retval = chip->rege0;

				//m_irq_func(0);

				// scan all oscillators
				for (i = 0; i < chip->oscsenabled; i++)
				{
					if (chip->oscillators[i].irqpend)
					{
						// signal this oscillator has an interrupt
						retval = i<<1;

						chip->rege0 = retval | 0x80;

						// and clear its flag
						chip->oscillators[i].irqpend = 0;

						/*if (chip->irq_callback)
						{
							chip->irq_callback(chip->device, 0);
						}*/
						break;
					}
				}

				// if any oscillators still need to be serviced, assert IRQ again immediately
				/*for (i = 0; i < chip->oscsenabled; i++)
				{
					if (chip->oscillators[i].irqpend)
					{
						if (chip->irq_callback)
						{
							chip->irq_callback(chip->device, 1);
						}
						break;
					}
				}*/

				return retval;

			case 0xe1:	// oscillator enable
				return (chip->oscsenabled-1)<<1;

			case 0xe2:	// A/D converter
				/*if (chip->adc_read)
				{
					return chip->adc_read(chip->device, 0);
				}*/
				break;
		}
	}

	return 0;
}

//WRITE8_DEVICE_HANDLER( es5503_w )
void es5503_w(void *_info, offs_t offset, UINT8 data)
{
	//ES5503Chip *chip = get_safe_token(device);
	ES5503Chip *chip = (ES5503Chip *)_info;

	//stream_update(chip->stream);

	if (offset < 0xe0)
	{
		int osc = offset & 0x1f;

		switch(offset & 0xe0)
		{
			case 0:		// freq lo
				chip->oscillators[osc].freq &= 0xff00;
				chip->oscillators[osc].freq |= data;
				break;

			case 0x20:  	// freq hi
				chip->oscillators[osc].freq &= 0x00ff;
				chip->oscillators[osc].freq |= (data<<8);
				break;

			case 0x40:	// volume
				chip->oscillators[osc].vol = data;
				break;

			case 0x60:	// data - ignore writes
				break;

			case 0x80:	// wavetable pointer
				chip->oscillators[osc].wavetblpointer = (data<<8);
				break;

			case 0xa0:	// oscillator control
				// if a fresh key-on, reset the ccumulator
				if ((chip->oscillators[osc].control & 1) && (!(data&1)))
				{
					chip->oscillators[osc].accumulator = 0;
				}

				chip->oscillators[osc].control = data;
				break;

			case 0xc0:	// bank select / wavetable size / resolution
				if (data & 0x40)	// bank select - not used on the Apple IIgs
				{
					chip->oscillators[osc].wavetblpointer |= 0x10000;
				}
				else
				{
					chip->oscillators[osc].wavetblpointer &= 0xffff;
				}

				chip->oscillators[osc].wavetblsize = ((data>>3) & 7);
				chip->oscillators[osc].wtsize = wavesizes[chip->oscillators[osc].wavetblsize];
				chip->oscillators[osc].resolution = (data & 7);
				break;
		}
	}
	else	 // global registers
	{
		switch (offset)
		{
			case 0xe0:	// interrupt status
				break;

			case 0xe1:	// oscillator enable
				chip->oscsenabled = 1 + ((data>>1) & 0x1f);

				chip->output_rate = (chip->clock/8)/(2+chip->oscsenabled);
				//stream_set_sample_rate(chip->stream, chip->output_rate);
				if (chip->SmpRateFunc != NULL)
					chip->SmpRateFunc(chip->SmpRateData, chip->output_rate);
				break;

			case 0xe2:	// A/D converter
				break;
		}
	}
}

/*void es5503_set_base(running_device *device, UINT8 *wavemem)
{
	ES5503Chip *chip = get_safe_token(device);

	chip->docram = wavemem;
}*/

void es5503_write_ram(void *_info, offs_t DataStart, offs_t DataLength, const UINT8* RAMData)
{
	ES5503Chip *chip = (ES5503Chip *)_info;
	
	if (DataStart >= chip->dramsize)
		return;
	if (DataStart + DataLength > chip->dramsize)
		DataLength = chip->dramsize - DataStart;
	
	memcpy(chip->docram + DataStart, RAMData, DataLength);
	
	return;
}

void es5503_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ES5503Chip *chip = (ES5503Chip *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 32; CurChn ++)
		chip->oscillators[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}

void es5503_set_srchg_cb(void *_info, SRATE_CALLBACK CallbackFunc, void* DataPtr)
{
	ES5503Chip *chip = (ES5503Chip *)_info;
	
	// set Sample Rate Change Callback routine
	chip->SmpRateFunc = CallbackFunc;
	chip->SmpRateData = DataPtr;
	
	return;
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( es5503 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ES5503Chip);					break;

		// --- the following bits of info are returned as pointers to data or functions --- //
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( es5503 );			break;
		case DEVINFO_FCT_STOP:							// Nothing //									break;
		case DEVINFO_FCT_RESET:							// Nothing //									break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "ES5503");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Ensoniq ES550x");				break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright R. Belmont");		break;
	}
}


DEFINE_LEGACY_SOUND_DEVICE(ES5503, es5503);*/
