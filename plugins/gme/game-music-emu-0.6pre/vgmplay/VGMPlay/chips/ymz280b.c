/*

 Yamaha YMZ280B driver
  by Aaron Giles

  YMZ280B 8-Channel PCMD8 PCM/ADPCM Decoder

 Features as listed in LSI-4MZ280B3 data sheet:
  Voice data stored in external memory can be played back simultaneously for up to eight voices
  Voice data format can be selected from 4-bit ADPCM, 8-bit PCM and 16-bit PCM
  Control of voice data external memory
   Up to 16M bytes of ROM or SRAM (x 8 bits, access time 150ms max) can be connected
   Continuous access is possible
   Loop playback between selective addresses is possible
  Voice data playback frequency control
   4-bit ADPCM ................ 0.172 to 44.1kHz in 256 steps
   8-bit PCM, 16-bit PCM ...... 0.172 to 88.2kHz in 512 steps
  256 steps total level and 16 steps panpot can be set
  Voice signal is output in stereo 16-bit 2's complement MSB-first format

  TODO:
  - Is memory handling 100% correct? At the moment, Konami firebeat.c is the only
    hardware currently emulated that uses external handlers.
    It also happens to be the only one using 16-bit PCM.

    Some other drivers (eg. bishi.c, bfm_sc4/5.c) also use ROM readback.

*/


#include <math.h>

#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
#ifdef _DEBUG
#include <stdio.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "ymz280b.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

static void update_irq_state_timer_common(void *param, int voicenum);

//unsigned char DISABLE_YMZ_FIX = 0x00;

#define MAX_SAMPLE_CHUNK	0x10000
#define MAKE_WAVS			0
#define MAKE_WAVS_CH		0

#define FRAC_BITS			14
#define FRAC_ONE			(1 << FRAC_BITS)
#define FRAC_MASK			(FRAC_ONE - 1)

#define INTERNAL_BUFFER_SIZE	(1 << 15)
//#define INTERNAL_SAMPLE_RATE	(chip->master_clock * 2.0)
#define INTERNAL_SAMPLE_RATE	chip->rate

#if MAKE_WAVS
#include "wavwrite.h"
#endif
#if MAKE_WAVS_CH
#include <stdio.h>
FILE* hWavFile[8];
signed short int* wavmem[8];
#endif


/* struct describing a single playing ADPCM voice */
struct YMZ280BVoice
{
	UINT8 playing;			/* 1 if we are actively playing */

	UINT8 keyon;			/* 1 if the key is on */
	UINT8 looping;			/* 1 if looping is enabled */
	UINT8 mode;				/* current playback mode */
	UINT16 fnum;			/* frequency */
	UINT8 level;			/* output level */
	UINT8 pan;				/* panning */

	UINT32 start;			/* start address, in nibbles */
	UINT32 stop;			/* stop address, in nibbles */
	UINT32 loop_start;		/* loop start address, in nibbles */
	UINT32 loop_end;		/* loop end address, in nibbles */
	UINT32 position;		/* current position, in nibbles */

	INT32 signal;			/* current ADPCM signal */
	INT32 step;				/* current ADPCM step */

	INT32 loop_signal;		/* signal at loop start */
	INT32 loop_step;		/* step at loop start */
	UINT32 loop_count;		/* number of loops so far */

	INT32 output_left;		/* output volume (left) */
	INT32 output_right;		/* output volume (right) */
	INT32 output_step;		/* step value for frequency conversion */
	INT32 output_pos;		/* current fractional position */
	INT16 last_sample;		/* last sample output */
	INT16 curr_sample;		/* current sample target */
	UINT8 irq_schedule;		/* 1 if the IRQ state is updated by timer */
	UINT8 Muted;			/* used for muting */
};

typedef struct _ymz280b_state ymz280b_state;
struct _ymz280b_state
{
	//sound_stream * stream;			/* which stream are we using */
	UINT8 *region_base;				/* pointer to the base of the region */
	UINT32 region_size;
	UINT8 current_register;			/* currently accessible register */
	UINT8 status_register;			/* current status register */
	UINT8 irq_state;				/* current IRQ state */
	UINT8 irq_mask;					/* current IRQ mask */
	UINT8 irq_enable;				/* current IRQ enable */
	UINT8 keyon_enable;				/* key on enable */
	UINT8 ext_mem_enable;			/* external memory enable */
	UINT8 ext_readlatch;			/* external memory prefetched data */
	UINT32 ext_mem_address_hi;
	UINT32 ext_mem_address_mid;
	UINT32 ext_mem_address;			/* where the CPU can read the ROM */
	
	double master_clock;			/* master clock frequency */
	double rate;
	//void (*irq_callback)(const device_config *, int);		/* IRQ callback */
	void (*irq_callback)(int);		/* IRQ callback */
	struct YMZ280BVoice	voice[8];	/* the 8 voices */
	//devcb_resolved_read8 ext_ram_read;		/* external RAM read handler */
	//devcb_resolved_write8 ext_ram_write;	/* external RAM write handler */

#if MAKE_WAVS
	void *		wavresample;			/* resampled waveform */
#endif

	INT16 *scratch;
	//const device_config *device;
};

static void write_to_register(ymz280b_state *chip, int data);


/* step size index shift table */
static const int index_scale[8] = { 0x0e6, 0x0e6, 0x0e6, 0x0e6, 0x133, 0x199, 0x200, 0x266 };

/* lookup table for the precomputed difference */
static int diff_lookup[16];
static unsigned char lookup_init = 0x00;	/* lookup-table is initialized */

/* timer callback */
/*static TIMER_CALLBACK( update_irq_state_timer_0 );
static TIMER_CALLBACK( update_irq_state_timer_1 );
static TIMER_CALLBACK( update_irq_state_timer_2 );
static TIMER_CALLBACK( update_irq_state_timer_3 );
static TIMER_CALLBACK( update_irq_state_timer_4 );
static TIMER_CALLBACK( update_irq_state_timer_5 );
static TIMER_CALLBACK( update_irq_state_timer_6 );
static TIMER_CALLBACK( update_irq_state_timer_7 );

static const timer_fired_func update_irq_state_cb[] =
{
	update_irq_state_timer_0,
	update_irq_state_timer_1,
	update_irq_state_timer_2,
	update_irq_state_timer_3,
	update_irq_state_timer_4,
	update_irq_state_timer_5,
	update_irq_state_timer_6,
	update_irq_state_timer_7
};*/


/*INLINE ymz280b_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_YMZ280B);
	return (ymz280b_state *)device->token;
}*/

INLINE void update_irq_state(ymz280b_state *chip)
{
	int irq_bits = chip->status_register & chip->irq_mask;

	/* always off if the enable is off */
	if (!chip->irq_enable)
		irq_bits = 0;

	/* update the state if changed */
	if (irq_bits && !chip->irq_state)
	{
		chip->irq_state = 1;
		if (chip->irq_callback)
			//(*chip->irq_callback)(chip->device, 1);
			(*chip->irq_callback)(1);
		//else logerror("YMZ280B: IRQ generated, but no callback specified!");
	}
	else if (!irq_bits && chip->irq_state)
	{
		chip->irq_state = 0;
		if (chip->irq_callback)
			//(*chip->irq_callback)(chip->device, 0);
			(*chip->irq_callback)(0);
		//else logerror("YMZ280B: IRQ generated, but no callback specified!");
	}
}


INLINE void update_step(ymz280b_state *chip, struct YMZ280BVoice *voice)
{
	double frequency;

	/* compute the frequency */
	if (voice->mode == 1)
		frequency = chip->master_clock * (double)((voice->fnum & 0x0ff) + 1) * (1.0 / 256.0);
	else
		frequency = chip->master_clock * (double)((voice->fnum & 0x1ff) + 1) * (1.0 / 256.0);
	voice->output_step = (UINT32)(frequency * (double)FRAC_ONE / INTERNAL_SAMPLE_RATE);
}


INLINE void update_volumes(struct YMZ280BVoice *voice)
{
	if (voice->pan == 8)
	{
		voice->output_left = voice->level;
		voice->output_right = voice->level;
	}
	else if (voice->pan < 8)
	{
		voice->output_left = voice->level;

		/* pan 1 is hard-left, what's pan 0? for now assume same as pan 1 */
		voice->output_right = (voice->pan == 0) ? 0 : voice->level * (voice->pan - 1) / 7;
	}
	else
	{
		voice->output_left = voice->level * (15 - voice->pan) / 7;
		voice->output_right = voice->level;
	}
}


//static STATE_POSTLOAD( YMZ280B_state_save_update_step )
/*void YMZ280B_state_save_update_step(void *param)
{
	ymz280b_state *chip = (ymz280b_state *)param;
	int j;
	for (j = 0; j < 8; j++)
	{
		struct YMZ280BVoice *voice = &chip->voice[j];
		update_step(chip, voice);
		if(voice->irq_schedule)
		//	timer_set(machine, attotime_zero, chip, 0, update_irq_state_cb[j]);
			update_irq_state_timer_common(param, j);
	}
}*/


INLINE UINT8 ymz280b_read_memory(UINT8 *base, UINT32 size, UINT32 offset)
{
	offset &= 0xFFFFFF;
	if (offset < size)
		return base[offset];

	/* 16MB chip limit (shouldn't happen) */
	//else if (offset > 0xffffff)
	//	return base[offset & 0xffffff];

	else
		return 0;
}


static void update_irq_state_timer_common(void *param, int voicenum)
{
	ymz280b_state *chip = (ymz280b_state *)param;
	struct YMZ280BVoice *voice = &chip->voice[voicenum];

	if(!voice->irq_schedule) return;

	voice->playing = 0;
	chip->status_register |= 1 << voicenum;
	update_irq_state(chip);
	voice->irq_schedule = 0;
}

/*static TIMER_CALLBACK( update_irq_state_timer_0 ) { update_irq_state_timer_common(ptr, 0); }
static TIMER_CALLBACK( update_irq_state_timer_1 ) { update_irq_state_timer_common(ptr, 1); }
static TIMER_CALLBACK( update_irq_state_timer_2 ) { update_irq_state_timer_common(ptr, 2); }
static TIMER_CALLBACK( update_irq_state_timer_3 ) { update_irq_state_timer_common(ptr, 3); }
static TIMER_CALLBACK( update_irq_state_timer_4 ) { update_irq_state_timer_common(ptr, 4); }
static TIMER_CALLBACK( update_irq_state_timer_5 ) { update_irq_state_timer_common(ptr, 5); }
static TIMER_CALLBACK( update_irq_state_timer_6 ) { update_irq_state_timer_common(ptr, 6); }
static TIMER_CALLBACK( update_irq_state_timer_7 ) { update_irq_state_timer_common(ptr, 7); }*/


/**********************************************************************************************

     compute_tables -- compute the difference tables

***********************************************************************************************/

static void compute_tables(void)
{
	int nib;

	if (lookup_init)
		return;

	/* loop over all nibbles and compute the difference */
	for (nib = 0; nib < 16; nib++)
	{
		int value = (nib & 0x07) * 2 + 1;
		diff_lookup[nib] = (nib & 0x08) ? -value : value;
	}
	
	lookup_init = 0x01;
}



/**********************************************************************************************

     generate_adpcm -- general ADPCM decoding routine

***********************************************************************************************/

static int generate_adpcm(struct YMZ280BVoice *voice, UINT8 *base, UINT32 size, INT16 *buffer, int samples)
{
	int position = voice->position;
	int signal = voice->signal;
	int step = voice->step;
	int val;

	/*if (! DISABLE_YMZ_FIX)
	{
		if (position >= voice->stop)
		{
			voice->playing = 0;
			return samples;
		}
	}*/

	/* two cases: first cases is non-looping */
	if (!voice->looping)
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* compute the new amplitude and update the current step */
			//val = base[position / 2] >> ((~position & 1) << 2);
			val = ymz280b_read_memory(base, size, position / 2) >> ((~position & 1) << 2);
			signal += (step * diff_lookup[val & 15]) / 8;

			/* clamp to the maximum */
			if (signal > 32767)
				signal = 32767;
			else if (signal < -32768)
				signal = -32768;

			/* adjust the step size and clamp */
			step = (step * index_scale[val & 7]) >> 8;
			if (step > 0x6000)
				step = 0x6000;
			else if (step < 0x7f)
				step = 0x7f;

			/* output to the buffer, scaling by the volume */
			*buffer++ = signal;
			samples--;

			/* next! */
			position++;
			if (position >= voice->stop)
			{
				if (!samples)
					samples |= 0x10000;

				break;
			}
		}
	}

	/* second case: looping */
	else
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* compute the new amplitude and update the current step */
			//val = base[position / 2] >> ((~position & 1) << 2);
			val = ymz280b_read_memory(base, size, position / 2) >> ((~position & 1) << 2);
			signal += (step * diff_lookup[val & 15]) / 8;

			/* clamp to the maximum */
			if (signal > 32767)
				signal = 32767;
			else if (signal < -32768)
				signal = -32768;

			/* adjust the step size and clamp */
			step = (step * index_scale[val & 7]) >> 8;
			if (step > 0x6000)
				step = 0x6000;
			else if (step < 0x7f)
				step = 0x7f;

			/* output to the buffer, scaling by the volume */
			*buffer++ = signal;
			samples--;

			/* next! */
			position++;
			if (position == voice->loop_start && voice->loop_count == 0)
			{
				voice->loop_signal = signal;
				voice->loop_step = step;
			}
			if (position >= voice->loop_end)
			{
				if (voice->keyon)
				{
					position = voice->loop_start;
					signal = voice->loop_signal;
					step = voice->loop_step;
					voice->loop_count++;
				}
			}
			if (position >= voice->stop)
			{
				if (!samples)
					samples |= 0x10000;

				break;
			}
		}
	}

	/* update the parameters */
	voice->position = position;
	voice->signal = signal;
	voice->step = step;

	return samples;
}



/**********************************************************************************************

     generate_pcm8 -- general 8-bit PCM decoding routine

***********************************************************************************************/

static int generate_pcm8(struct YMZ280BVoice *voice, UINT8 *base, UINT32 size, INT16 *buffer, int samples)
{
	int position = voice->position;
	int val;

	/*if (! DISABLE_YMZ_FIX)
	{
		if (position >= voice->stop)
		{
			voice->playing = 0;
			return samples;
		}
	}*/

	/* two cases: first cases is non-looping */
	if (!voice->looping)
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			//val = base[position / 2];
			val = ymz280b_read_memory(base, size, position / 2);

			/* output to the buffer, scaling by the volume */
			*buffer++ = (INT8)val * 256;
			samples--;

			/* next! */
			position += 2;
			if (position >= voice->stop)
			{
				if (!samples)
					samples |= 0x10000;

				break;
			}
		}
	}

	/* second case: looping */
	else
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			//val = base[position / 2];
			val = ymz280b_read_memory(base, size, position / 2);

			/* output to the buffer, scaling by the volume */
			*buffer++ = (INT8)val * 256;
			samples--;

			/* next! */
			position += 2;
			if (position >= voice->loop_end)
			{
				if (voice->keyon)
					position = voice->loop_start;
			}
			if (position >= voice->stop)
			{
				if (!samples)
					samples |= 0x10000;

				break;
			}
		}
	}

	/* update the parameters */
	voice->position = position;

	return samples;
}



/**********************************************************************************************

     generate_pcm16 -- general 16-bit PCM decoding routine

***********************************************************************************************/

static int generate_pcm16(struct YMZ280BVoice *voice, UINT8 *base, UINT32 size, INT16 *buffer, int samples)
{
	int position = voice->position;
	int val;

	/*if (! DISABLE_YMZ_FIX)
	{
		if (position >= voice->stop)
		{
			voice->playing = 0;
			return samples;
		}
	}*/

	/* is it even used in any MAME game? */
	//popmessage("YMZ280B 16-bit PCM contact MAMEDEV");

	/* two cases: first cases is non-looping */
	if (!voice->looping)
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			//val = (INT16)((base[position / 2 + 1] << 8) + base[position / 2 + 0]);
			val = (INT16)((ymz280b_read_memory(base, size, position / 2 + 0) << 8) + ymz280b_read_memory(base, size, position / 2 + 1));
			// Note: Last MAME updates say it's: ((position / 2 + 1) << 8) + (position / 2 + 0);

			/* output to the buffer, scaling by the volume */
			*buffer++ = val;
			samples--;

			/* next! */
			position += 4;
			if (position >= voice->stop)
			{
				if (!samples)
					samples |= 0x10000;

				break;
			}
		}
	}

	/* second case: looping */
	else
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			//val = (INT16)((base[position / 2 + 1] << 8) + base[position / 2 + 0]);
			val = (INT16)((ymz280b_read_memory(base, size, position / 2 + 0) << 8) + ymz280b_read_memory(base, size, position / 2 + 1));

			/* output to the buffer, scaling by the volume */
			*buffer++ = val;
			samples--;

			/* next! */
			position += 4;
			if (position >= voice->loop_end)
			{
				if (voice->keyon)
					position = voice->loop_start;
			}
			if (position >= voice->stop)
			{
				if (!samples)
					samples |= 0x10000;

				break;
			}
		}
	}

	/* update the parameters */
	voice->position = position;

	return samples;
}



/**********************************************************************************************

     ymz280b_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

//static STREAM_UPDATE( ymz280b_update )
void ymz280b_update(void *param, stream_sample_t **outputs, int samples)
{
	ymz280b_state *chip = (ymz280b_state *)param;
	stream_sample_t *lacc = outputs[0];
	stream_sample_t *racc = outputs[1];
	int v;

	/* clear out the accumulator */
	memset(lacc, 0, samples * sizeof(lacc[0]));
	memset(racc, 0, samples * sizeof(racc[0]));

	/* loop over voices */
	for (v = 0; v < 8; v++)
	{
		struct YMZ280BVoice *voice = &chip->voice[v];
		INT16 prev = voice->last_sample;
		INT16 curr = voice->curr_sample;
		INT16 *curr_data = chip->scratch;
		INT32 *ldest = lacc;
		INT32 *rdest = racc;
		UINT32 new_samples, samples_left;
		UINT32 final_pos;
		int remaining = samples;
		int lvol = voice->output_left;
		int rvol = voice->output_right;
#if MAKE_WAVS_CH
		signed short int* wavlog;

		memset(wavmem[v], 0x00, samples * 0x02);
		wavlog = wavmem[v];
#endif
		
		/* skip if muted */
		if (voice->Muted)
			continue;

		/* quick out if we're not playing and we're at 0 */
		if (!voice->playing && curr == 0 && prev == 0)
		{
			/* make sure next sound plays immediately */
			voice->output_pos = FRAC_ONE;

			continue;
		}

		/* finish off the current sample */
		/* interpolate */
		while (remaining > 0 && voice->output_pos < FRAC_ONE)
		{
			int interp_sample = (((INT32)prev * (FRAC_ONE - voice->output_pos)) + ((INT32)curr * voice->output_pos)) >> FRAC_BITS;
			*ldest++ += interp_sample * lvol;
			*rdest++ += interp_sample * rvol;
#if MAKE_WAVS_CH
			*(wavlog ++) = (signed short int)interp_sample;
#endif
			voice->output_pos += voice->output_step;
			remaining--;
		}

		/* if we're over, continue; otherwise, we're done */
		if (voice->output_pos >= FRAC_ONE)
			voice->output_pos -= FRAC_ONE;
		else
			continue;

		/* compute how many new samples we need */
		final_pos = voice->output_pos + remaining * voice->output_step;
		new_samples = (final_pos + FRAC_ONE) >> FRAC_BITS;
		if (new_samples > MAX_SAMPLE_CHUNK)
			new_samples = MAX_SAMPLE_CHUNK;
		samples_left = new_samples;

		/* generate them into our buffer */
		switch (voice->playing << 7 | voice->mode)
		{
			case 0x81:	samples_left = generate_adpcm(voice, chip->region_base, chip->region_size, chip->scratch, new_samples);		break;
			case 0x82:	samples_left = generate_pcm8(voice, chip->region_base, chip->region_size, chip->scratch, new_samples);		break;
			case 0x83:	samples_left = generate_pcm16(voice, chip->region_base, chip->region_size, chip->scratch, new_samples);		break;
			default:	samples_left = 0; memset(chip->scratch, 0, new_samples * sizeof(chip->scratch[0]));							break;
		}

		/* if there are leftovers, ramp back to 0 */
		if (samples_left)
		{
			/* note: samples_left bit 16 is set if the voice was finished at the same time the function ended */
			int base;
			int i, t;
			
			samples_left &= 0xffff;
			base = new_samples - samples_left;
			t = (base == 0) ? curr : chip->scratch[base - 1];
			
			for (i = 0; i < samples_left; i++)
			{
				if (t < 0) t = -((-t * 15) >> 4);
				else if (t > 0) t = (t * 15) >> 4;
				chip->scratch[base + i] = t;
			}

			/* if we hit the end and IRQs are enabled, signal it */
			if (base != 0)
			{
				voice->playing = 0;

				/* set update_irq_state_timer. IRQ is signaled on next CPU execution. */
				//timer_set(chip->device->machine, attotime_zero, chip, 0, update_irq_state_cb[v]);
				voice->irq_schedule = 1;
			}
		}

		/* advance forward one sample */
		prev = curr;
		curr = *curr_data++;

		/* then sample-rate convert with linear interpolation */
		while (remaining > 0)
		{
			/* interpolate */
			while (remaining > 0 && voice->output_pos < FRAC_ONE)
			{
				int interp_sample = (((INT32)prev * (FRAC_ONE - voice->output_pos)) + ((INT32)curr * voice->output_pos)) >> FRAC_BITS;
				*ldest++ += interp_sample * lvol;
				*rdest++ += interp_sample * rvol;
#if MAKE_WAVS_CH
				*(wavlog ++) = (signed short int)interp_sample;
#endif
				voice->output_pos += voice->output_step;
				remaining--;
			}

			/* if we're over, grab the next samples */
			if (voice->output_pos >= FRAC_ONE)
			{
				voice->output_pos -= FRAC_ONE;
				prev = curr;
				curr = *curr_data++;
			}
		}

		/* remember the last samples */
		voice->last_sample = prev;
		voice->curr_sample = curr;
	}

	for (v = 0; v < samples; v++)
	{
		outputs[0][v] /= 256;
		outputs[1][v] /= 256;
	}
	
	for (v = 0; v < 8; v++)
		update_irq_state_timer_common(chip, v);
	
#if MAKE_WAVS_CH
	for (v = 0; v < 8; v++)
	{
		fwrite(wavmem[v], 0x02, samples, hWavFile[v]);
	}
#endif
}



/**********************************************************************************************

     DEVICE_START( ymz280b ) -- start emulation of the YMZ280B

***********************************************************************************************/

//static DEVICE_START( ymz280b )
int device_start_ymz280b(void **_info, int clock)
{
	static const ymz280b_interface defintrf = { 0 };
	//const ymz280b_interface *intf = (device->static_config != NULL) ? (const ymz280b_interface *)device->static_config : &defintrf;
	const ymz280b_interface *intf = &defintrf;
	//ymz280b_state *chip = get_safe_token(device);
	ymz280b_state *chip;
	int chn;

	chip = (ymz280b_state *) calloc(1, sizeof(ymz280b_state));
	*_info = (void *) chip;
	
	//chip->device = device;
	//devcb_resolve_read8(&chip->ext_ram_read, &intf->ext_read, device);
	//devcb_resolve_write8(&chip->ext_ram_write, &intf->ext_write, device);

	/* compute ADPCM tables */
	compute_tables();

	/* initialize the rest of the structure */
	chip->master_clock = (double)clock / 384.0;
	
	chip->rate = chip->master_clock * 2.0;
	// disabled until the frequency calculation gets fixed
	/*if (((CHIP_SAMPLING_MODE & 0x01) && chip->rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		chip->rate = (double)CHIP_SAMPLE_RATE;*/
	
	//chip->region_base = device->region;
	chip->region_size = 0x00;
	chip->region_base = NULL;
	chip->irq_callback = intf->irq_callback;

	/* create the stream */
	//chip->stream = stream_create(device, 0, 2, INTERNAL_SAMPLE_RATE, chip, ymz280b_update);

	/* allocate memory */
	//chip->scratch = auto_alloc_array(device->machine, INT16, MAX_SAMPLE_CHUNK);
	chip->scratch = malloc(MAX_SAMPLE_CHUNK * sizeof(INT16));
	memset(chip->scratch, 0x00, MAX_SAMPLE_CHUNK * sizeof(INT16));

	/* state save */
	/*{
		int j;
		state_save_register_device_item(device, 0, chip->current_register);
		state_save_register_device_item(device, 0, chip->status_register);
		state_save_register_device_item(device, 0, chip->irq_state);
		state_save_register_device_item(device, 0, chip->irq_mask);
		state_save_register_device_item(device, 0, chip->irq_enable);
		state_save_register_device_item(device, 0, chip->keyon_enable);
		state_save_register_device_item(device, 0, chip->rom_readback_addr);
		for (j = 0; j < 8; j++)
		{
			state_save_register_device_item(device, j, chip->voice[j].playing);
			state_save_register_device_item(device, j, chip->voice[j].keyon);
			state_save_register_device_item(device, j, chip->voice[j].looping);
			state_save_register_device_item(device, j, chip->voice[j].mode);
			state_save_register_device_item(device, j, chip->voice[j].fnum);
			state_save_register_device_item(device, j, chip->voice[j].level);
			state_save_register_device_item(device, j, chip->voice[j].pan);
			state_save_register_device_item(device, j, chip->voice[j].start);
			state_save_register_device_item(device, j, chip->voice[j].stop);
			state_save_register_device_item(device, j, chip->voice[j].loop_start);
			state_save_register_device_item(device, j, chip->voice[j].loop_end);
			state_save_register_device_item(device, j, chip->voice[j].position);
			state_save_register_device_item(device, j, chip->voice[j].signal);
			state_save_register_device_item(device, j, chip->voice[j].step);
			state_save_register_device_item(device, j, chip->voice[j].loop_signal);
			state_save_register_device_item(device, j, chip->voice[j].loop_step);
			state_save_register_device_item(device, j, chip->voice[j].loop_count);
			state_save_register_device_item(device, j, chip->voice[j].output_left);
			state_save_register_device_item(device, j, chip->voice[j].output_right);
			state_save_register_device_item(device, j, chip->voice[j].output_pos);
			state_save_register_device_item(device, j, chip->voice[j].last_sample);
			state_save_register_device_item(device, j, chip->voice[j].curr_sample);
			state_save_register_device_item(device, j, chip->voice[j].irq_schedule);
		}
	}*/
	
	for (chn = 0; chn < 8; chn ++)
		chip->voice[chn].Muted = 0x00;

	//state_save_register_postload(device->machine, YMZ280B_state_save_update_step, chip);

#if MAKE_WAVS
	chip->wavresample = wav_open("resamp.wav", INTERNAL_SAMPLE_RATE, 2);
#endif
#if MAKE_WAVS_CH
	hWavFile[0] = fopen("logwav0.raw", "wb");
	hWavFile[1] = fopen("logwav1.raw", "wb");
	hWavFile[2] = fopen("logwav2.raw", "wb");
	hWavFile[3] = fopen("logwav3.raw", "wb");
	hWavFile[4] = fopen("logwav4.raw", "wb");
	hWavFile[5] = fopen("logwav5.raw", "wb");
	hWavFile[6] = fopen("logwav6.raw", "wb");
	hWavFile[7] = fopen("logwav7.raw", "wb");
	{
		char v;
		for (v = 0; v < 8; v++)
		{
			wavmem[v] = (signed short int*)malloc(0x10 * 0x02);
		}
	}
#endif

	return (int)INTERNAL_SAMPLE_RATE;
}

//static DEVICE_STOP( ymz280b )
void device_stop_ymz280b(void *_info)
{
	//ymz280b_state *chip = get_safe_token(device);
	ymz280b_state *chip = (ymz280b_state *)_info;
	free(chip->region_base);	chip->region_base = NULL;
	free(chip->scratch);
	
#if MAKE_WAVS_CH
	{
		char v;
		for (v = 0; v < 8; v++)
		{
			free(wavmem[v]);
			fclose(hWavFile[v]);
		}
	}
#endif

	free(chip);

	return;
}

//static DEVICE_RESET( ymz280b )
void device_reset_ymz280b(void *_info)
{
	ymz280b_state *chip = (ymz280b_state *)_info;
	/*struct YMZ280BVoice *voice;
	unsigned char curvoc;
	
	chip->current_register = 0x00;
	chip->status_register = 0x00;
	chip->irq_state = 0x00;
	chip->irq_mask = 0x00;
	chip->irq_enable = 0x00;
	chip->keyon_enable = 0x00;
	chip->ext_mem_address = 0x000000;
	for (curvoc = 0; curvoc < 8; curvoc ++)
	{
		voice = &chip->voice[curvoc];
		
		voice->playing = 0;
			
		voice->keyon = 0;
		voice->looping = 0;
		voice->mode = 0;
		voice->fnum = 0;
		voice->level = 0;
		voice->pan = 8;
		
		voice->start = 0x000000;
		voice->stop = 0x000000;
		voice->loop_start = 0x000000;
		voice->loop_end = 0x000000;
		voice->position = 0x000000;
	}*/
	
	// new code from MAME 0.143u4
	int i;

	/* initial clear registers */
	for (i = 0xff; i >= 0; i--)
	{
		if (i == 0x83 || (i >= 88 && i <= 0xFD))
			continue;	// avoid too many debug messages
		chip->current_register = i;
		write_to_register(chip, 0);
	}

	chip->current_register = 0;
	chip->status_register = 0;

	/* clear other voice parameters */
	for (i = 0; i < 8; i++)
	{
		struct YMZ280BVoice *voice = &chip->voice[i];

		voice->curr_sample = 0;
		voice->last_sample = 0;
		voice->output_pos = FRAC_ONE;
		voice->playing = 0;
	}
	
	return;
}


/**********************************************************************************************

     write_to_register -- handle a write to the current register

***********************************************************************************************/

static void write_to_register(ymz280b_state *chip, int data)
{
	struct YMZ280BVoice *voice;
	int i;
	//UINT8 mode_new;

	/* force an update */
	//stream_update(chip->stream);

	/* lower registers follow a pattern */
	if (chip->current_register < 0x80)
	{
		voice = &chip->voice[(chip->current_register >> 2) & 7];

		switch (chip->current_register & 0xe3)
		{
			case 0x00:		/* pitch low 8 bits */
				voice->fnum = (voice->fnum & 0x100) | (data & 0xff);
				update_step(chip, voice);
				break;

			case 0x01:		/* pitch upper 1 bit, loop, key on, mode */
				voice->fnum = (voice->fnum & 0xff) | ((data & 0x01) << 8);
				voice->looping = (data & 0x10) >> 4;
				/*mode_new = (data & 0x60) >> 5;
				if (! DISABLE_YMZ_FIX)
				{
					// that fixes the scratch-bug
					if (voice->mode != mode_new)
					{
						// On-the-fly Mode-Change won't make sense,
						//	so I'm doing: KeyOff + Mode Change -> Instant Stop.
						//	(Deroon DeroDero uses this quite often)
						//	This is done by setting KeyOn to 0.
						//	Instant Stop/Restarting is done below.
						voice->keyon = 0;
						voice->irq_schedule = 0;
					}
					if (! mode_new)
						data &= 0x7F;
				}
				voice->mode = mode_new;*/
				if ((data & 0x60) == 0) data &= 0x7f; /* ignore mode setting and set to same state as KON=0 */
				else voice->mode = (data & 0x60) >> 5;
				
				if (!voice->keyon && (data & 0x80) && chip->keyon_enable)
				{
					voice->playing = 1;
					voice->position = voice->start;
					voice->signal = voice->loop_signal = 0;
					voice->step = voice->loop_step = 0x7f;
					voice->loop_count = 0;

					/* if update_irq_state_timer is set, cancel it. */
					voice->irq_schedule = 0;
				}
				/*else if (voice->keyon && !(data & 0x80) && !voice->looping)
				{
					voice->playing = 0;

					// if update_irq_state_timer is set, cancel it.
					voice->irq_schedule = 0;
				}
				else if (! DISABLE_YMZ_FIX && ! voice->keyon && !(data & 0x80) && voice->playing)
				{
					// 2x KeyOff -> Instant Stop, too (see Deroon DeroDero: Round Start-Tune)
					voice->playing = 0;
					voice->irq_schedule = 0;
				}*/
				// new code from MAME 0.143u4
				else if (voice->keyon && !(data & 0x80))
				{
					voice->playing = 0;

					// if update_irq_state_timer is set, cancel it.
					voice->irq_schedule = 0;
				}
				voice->keyon = (data & 0x80) >> 7;
				update_step(chip, voice);
				break;

			case 0x02:		/* total level */
				voice->level = data;
				update_volumes(voice);
				break;

			case 0x03:		/* pan */
				voice->pan = data & 0x0f;
				update_volumes(voice);
				break;

			case 0x20:		/* start address high */
				voice->start = (voice->start & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x21:		/* loop start address high */
				voice->loop_start = (voice->loop_start & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x22:		/* loop end address high */
				voice->loop_end = (voice->loop_end & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x23:		/* stop address high */
				voice->stop = (voice->stop & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x40:		/* start address middle */
				voice->start = (voice->start & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x41:		/* loop start address middle */
				voice->loop_start = (voice->loop_start & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x42:		/* loop end address middle */
				voice->loop_end = (voice->loop_end & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x43:		/* stop address middle */
				voice->stop = (voice->stop & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x60:		/* start address low */
				voice->start = (voice->start & (0xffff00 << 1)) | (data << 1);
				break;

			case 0x61:		/* loop start address low */
				voice->loop_start = (voice->loop_start & (0xffff00 << 1)) | (data << 1);
				break;

			case 0x62:		/* loop end address low */
				voice->loop_end = (voice->loop_end & (0xffff00 << 1)) | (data << 1);
				break;

			case 0x63:		/* stop address low */
				voice->stop = (voice->stop & (0xffff00 << 1)) | (data << 1);
				break;

			default:
#ifdef _DEBUG
				logerror("YMZ280B: unknown register write %02X = %02X\n", chip->current_register, data);
#endif
				break;
		}
	}

	/* upper registers are special */
	else
	{
		switch (chip->current_register)
		{
			/* DSP related (not implemented yet) */
			case 0x80: // d0-2: DSP Rch, d3: enable Rch (0: yes, 1: no), d4-6: DSP Lch, d7: enable Lch (0: yes, 1: no)
			case 0x81: // d0: enable control of $82 (0: yes, 1: no)
			case 0x82: // DSP data
#ifdef _DEBUG
				logerror("YMZ280B: DSP register write %02X = %02X\n", chip->current_register, data);
#endif
				break;

			case 0x84:		/* ROM readback / RAM write (high) */
				chip->ext_mem_address_hi = data << 16;
				break;

			case 0x85:		/* ROM readback / RAM write (middle) */
				chip->ext_mem_address_mid = data << 8;
				break;

			case 0x86:      /* ROM readback / RAM write (low) -> update latch */
				chip->ext_mem_address = chip->ext_mem_address_hi | chip->ext_mem_address_mid | data;
				if (chip->ext_mem_enable)
					chip->ext_readlatch = ymz280b_read_memory(chip->region_base, chip->region_size, chip->ext_mem_address);
				break;

			case 0x87:		/* RAM write */
				if (chip->ext_mem_enable)
				{
					/*if (!chip->ext_ram_write.isnull())
						chip->ext_ram_write(chip->ext_mem_address, data);
					else
						logerror("YMZ280B attempted RAM write to %X\n", chip->ext_mem_address);*/
					chip->ext_mem_address = (chip->ext_mem_address + 1) & 0xffffff;
				}
				break;

			case 0xfe:		/* IRQ mask */
				chip->irq_mask = data;
				update_irq_state(chip);
				break;

			case 0xff:		/* IRQ enable, test, etc */
				chip->ext_mem_enable = (data & 0x40) >> 6;
				chip->irq_enable = (data & 0x10) >> 4;
				update_irq_state(chip);

				if (chip->keyon_enable && !(data & 0x80))
				{
					for (i = 0; i < 8; i++)
					{
						chip->voice[i].playing = 0;

						/* if update_irq_state_timer is set, cancel it. */
						chip->voice[i].irq_schedule = 0;
					}
				}
				else if (!chip->keyon_enable && (data & 0x80))
				{
					for (i = 0; i < 8; i++)
					{
						if (chip->voice[i].keyon && chip->voice[i].looping)
							chip->voice[i].playing = 1;
					}
				}
				chip->keyon_enable = (data & 0x80) >> 7;
				break;

			default:
#ifdef _DEBUG
				logerror("YMZ280B: unknown register write %02X = %02X\n", chip->current_register, data);
#endif
				break;
		}
	}
}



/**********************************************************************************************

     compute_status -- determine the status bits

***********************************************************************************************/

static int compute_status(ymz280b_state *chip)
{
	UINT8 result;

	/* force an update */
	//stream_update(chip->stream);

	result = chip->status_register;

	/* clear the IRQ state */
	chip->status_register = 0;
	update_irq_state(chip);

	return result;
}



/**********************************************************************************************

     ymz280b_r/ymz280b_w -- handle external accesses

***********************************************************************************************/

//READ8_DEVICE_HANDLER( ymz280b_r )
UINT8 ymz280b_r(void *_info, offs_t offset)
{
	//ymz280b_state *chip = get_safe_token(device);
	ymz280b_state *chip = (ymz280b_state *)_info;

	if ((offset & 1) == 0)
	{
		UINT8 ret;
		
		if (! chip->ext_mem_enable)
			return 0xff;

		/* read from external memory */
		ret = chip->ext_readlatch;
		ret = ymz280b_read_memory(chip->region_base, chip->region_size, chip->ext_mem_address);
		chip->ext_mem_address = (chip->ext_mem_address + 1) & 0xffffff;
		return ret;
	}
	else
	{
		return compute_status(chip);
	}
}


//WRITE8_DEVICE_HANDLER( ymz280b_w )
void ymz280b_w(void *_info, offs_t offset, UINT8 data)
{
	//ymz280b_state *chip = get_safe_token(device);
	ymz280b_state *chip = (ymz280b_state *)_info;

	if ((offset & 1) == 0)
		chip->current_register = data;
	else
	{
		/* force an update */
		//chip->stream->update();

		write_to_register(chip, data);
	}
}

void ymz280b_write_rom(void *_info, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					   const UINT8* ROMData)
{
	ymz280b_state *chip = (ymz280b_state *)_info;
	
	if (chip->region_size != ROMSize)
	{
		chip->region_base = (UINT8*)realloc(chip->region_base, ROMSize);
		chip->region_size = ROMSize;
		memset(chip->region_base, 0xFF, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(chip->region_base + DataStart, ROMData, DataLength);
	
	return;
}


void ymz280b_set_mute_mask(void *_info, UINT32 MuteMask)
{
	ymz280b_state *chip = (ymz280b_state *)_info;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 8; CurChn ++)
		chip->voice[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( ymz280b )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ymz280b_state);			break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ymz280b );		break;
		case DEVINFO_FCT_STOP:							// Nothing									break;
		case DEVINFO_FCT_RESET:							info->start = DEVICE_RESET_NAME( ymz280b );		break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "YMZ280B");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha Wavetable");			break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/

