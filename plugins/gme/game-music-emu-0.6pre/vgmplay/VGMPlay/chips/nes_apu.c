/*****************************************************************************

  MAME/MESS NES APU CORE

  Based on the Nofrendo/Nosefart NES N2A03 sound emulation core written by
  Matthew Conte (matt@conte.com) and redesigned for use in MAME/MESS by
  Who Wants to Know? (wwtk@mail.com)

  This core is written with the advise and consent of Matthew Conte and is
  released under the GNU Public License.  This core is freely avaiable for
  use in any freeware project, subject to the following terms:

  Any modifications to this code must be duly noted in the source and
  approved by Matthew Conte and myself prior to public submission.

  timing notes:
  master = 21477270
  2A03 clock = master/12
  sequencer = master/89490 or CPU/7457

 *****************************************************************************

   NES_APU.C

   Actual NES APU interface.

   LAST MODIFIED 02/29/2004

   - Based on Matthew Conte's Nofrendo/Nosefart core and redesigned to
     use MAME system calls and to enable multiple APUs.  Sound at this
     point should be just about 100% accurate, though I cannot tell for
     certain as yet.

     A queue interface is also available for additional speed.  However,
     the implementation is not yet 100% (DPCM sounds are inaccurate),
     so it is disabled by default.

 *****************************************************************************

   BUGFIXES:

   - Various bugs concerning the DPCM channel fixed. (Oliver Achten)
   - Fixed $4015 read behaviour. (Oliver Achten)

 *****************************************************************************/

#include "mamedef.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>	// for NULL
//#include "emu.h"
//#include "streams.h"
#include "nes_apu.h"
//#include "cpu/m6502/m6502.h"

#include "nes_defs.h"

/* GLOBAL CONSTANTS */
#define  SYNCS_MAX1     0x20
#define  SYNCS_MAX2     0x80

/* GLOBAL VARIABLES */
typedef struct _nesapu_state nesapu_state;
struct _nesapu_state
{
	apu_t   APU;			       /* Actual APUs */
	float   apu_incsize;           /* Adjustment increment */
	uint32  samps_per_sync;        /* Number of samples per vsync */
	uint32  buffer_size;           /* Actual buffer size in bytes */
	uint32  real_rate;             /* Actual playback rate */
	uint8   noise_lut[NOISE_LONG]; /* Noise sample lookup table */
	uint32  vbl_times[0x20];       /* VBL durations in samples */
	uint32  sync_times1[SYNCS_MAX1]; /* Samples per sync table */
	uint32  sync_times2[SYNCS_MAX2]; /* Samples per sync table */
	//sound_stream *stream;
};

static UINT8 DPCMBase0 = 0x01;

//#define MAX_CHIPS	0x02
//static nesapu_state NESAPUData[MAX_CHIPS];

/*INLINE nesapu_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == NES);
	return (nesapu_state *)downcast<legacy_device_base *>(device)->token();
}*/

/* INTERNAL FUNCTIONS */

/* INITIALIZE WAVE TIMES RELATIVE TO SAMPLE RATE */
static void create_vbltimes(uint32 * table,const uint8 *vbl,unsigned int rate)
{
	int i;

	for (i = 0; i < 0x20; i++)
		table[i] = vbl[i] * rate;
}

/* INITIALIZE SAMPLE TIMES IN TERMS OF VSYNCS */
static void create_syncs(nesapu_state *info, unsigned long sps)
{
	int i;
	unsigned long val = sps;

	for (i = 0; i < SYNCS_MAX1; i++)
	{
		info->sync_times1[i] = val;
		val += sps;
	}

	val = 0;
	for (i = 0; i < SYNCS_MAX2; i++)
	{
		info->sync_times2[i] = val;
		info->sync_times2[i] >>= 2;
		val += sps;
	}
}

/* INITIALIZE NOISE LOOKUP TABLE */
static void create_noise(uint8 *buf, const int bits, int size)
{
	int m = 0x0011;
	int xor_val, i;

	for (i = 0; i < size; i++)
	{
		xor_val = m & 1;
		m >>= 1;
		xor_val ^= (m & 1);
		m |= xor_val << (bits - 1);

		buf[i] = m;
	}
}

/* TODO: sound channels should *ALL* have DC volume decay */

/* OUTPUT SQUARE WAVE SAMPLE (VALUES FROM -16 to +15) */
static int8 apu_square(nesapu_state *info, square_t *chan)
{
	int env_delay;
	int sweep_delay;
	int8 output;
	uint8 freq_index;

	/* reg0: 0-3=volume, 4=envelope, 5=hold, 6-7=duty cycle
    ** reg1: 0-2=sweep shifts, 3=sweep inc/dec, 4-6=sweep length, 7=sweep on
    ** reg2: 8 bits of freq
    ** reg3: 0-2=high freq, 7-4=vbl length counter
    */

	if (FALSE == chan->enabled || chan->Muted)
		return 0;

	/* enveloping */
	env_delay = info->sync_times1[chan->regs[0] & 0x0F];

	/* decay is at a rate of (env_regs + 1) / 240 secs */
	chan->env_phase -= 4;
	while (chan->env_phase < 0)
	{
		chan->env_phase += env_delay;
		if (chan->regs[0] & 0x20)
			chan->env_vol = (chan->env_vol + 1) & 15;
		else if (chan->env_vol < 15)
			chan->env_vol++;
	}

	/* vbl length counter */
	if (chan->vbl_length > 0 && 0 == (chan->regs [0] & 0x20))
		chan->vbl_length--;

	if (0 == chan->vbl_length)
		return 0;

	/* freqsweeps */
	if ((chan->regs[1] & 0x80) && (chan->regs[1] & 7))
	{
		sweep_delay = info->sync_times1[(chan->regs[1] >> 4) & 7];
		chan->sweep_phase -= 2;
		while (chan->sweep_phase < 0)
		{
			chan->sweep_phase += sweep_delay;
			if (chan->regs[1] & 8)
				chan->freq -= chan->freq >> (chan->regs[1] & 7);
			else
				chan->freq += chan->freq >> (chan->regs[1] & 7);
		}
	}

//	if ((0 == (chan->regs[1] & 8) && (chan->freq >> 16) > freq_limit[chan->regs[1] & 7])
//		 || (chan->freq >> 16) < 4)
//		return 0;
	
	// Thanks to Delek for the fix
	if (chan->regs[1] & 0x80)
		freq_index = chan->regs[1] & 7;	//If sweeping is enabled, I choose it as normal.
	else
		freq_index = 7;	//If sweeping is disabled, I choose the lower limit.
	
	if ((0 == (chan->regs[1] & 8) && (chan->freq >> 16) > freq_limit[freq_index])
		 || (chan->freq >> 16) < 4)
		return 0;

	chan->phaseacc -= (float) info->apu_incsize; /* # of cycles per sample */

	while (chan->phaseacc < 0)
	{
		chan->phaseacc += (chan->freq >> 16);
		chan->adder = (chan->adder + 1) & 0x0F;
	}

	if (chan->regs[0] & 0x10) /* fixed volume */
		output = chan->regs[0] & 0x0F;
	else
		output = 0x0F - chan->env_vol;

	if (chan->adder < (duty_lut[chan->regs[0] >> 6]))
		output = -output;

	return (int8) output;
}

/* OUTPUT TRIANGLE WAVE SAMPLE (VALUES FROM -16 to +15) */
static int8 apu_triangle(nesapu_state *info, triangle_t *chan)
{
	int freq;
	int8 output;
	/* reg0: 7=holdnote, 6-0=linear length counter
    ** reg2: low 8 bits of frequency
    ** reg3: 7-3=length counter, 2-0=high 3 bits of frequency
    */

	if (FALSE == chan->enabled || chan->Muted)
		return 0;

	if (FALSE == chan->counter_started && 0 == (chan->regs[0] & 0x80))
	{
		if (chan->write_latency)
			chan->write_latency--;
		if (0 == chan->write_latency)
			chan->counter_started = TRUE;
	}

	if (chan->counter_started)
	{
		if (chan->linear_length > 0)
			chan->linear_length--;
		if (chan->vbl_length && 0 == (chan->regs[0] & 0x80))
				chan->vbl_length--;

		if (0 == chan->vbl_length)
			return 0;
	}

	if (0 == chan->linear_length)
		return 0;

	freq = (((chan->regs[3] & 7) << 8) + chan->regs[2]) + 1;

	if (freq < 4) /* inaudible */
		return 0;

	chan->phaseacc -= (float) info->apu_incsize; /* # of cycles per sample */
	while (chan->phaseacc < 0)
	{
		chan->phaseacc += freq;
		chan->adder = (chan->adder + 1) & 0x1F;

		output = (chan->adder & 7) << 1;
		if (chan->adder & 8)
			output = 0x10 - output;
		if (chan->adder & 0x10)
			output = -output;

		chan->output_vol = output;
	}

	return (int8) chan->output_vol;
}

/* OUTPUT NOISE WAVE SAMPLE (VALUES FROM -16 to +15) */
static int8 apu_noise(nesapu_state *info, noise_t *chan)
{
	int freq, env_delay;
	uint8 outvol;
	uint8 output;

	/* reg0: 0-3=volume, 4=envelope, 5=hold
    ** reg2: 7=small(93 byte) sample,3-0=freq lookup
    ** reg3: 7-4=vbl length counter
    */

	if (FALSE == chan->enabled || chan->Muted)
		return 0;

	/* enveloping */
	env_delay = info->sync_times1[chan->regs[0] & 0x0F];

	/* decay is at a rate of (env_regs + 1) / 240 secs */
	chan->env_phase -= 4;
	while (chan->env_phase < 0)
	{
		chan->env_phase += env_delay;
		if (chan->regs[0] & 0x20)
			chan->env_vol = (chan->env_vol + 1) & 15;
		else if (chan->env_vol < 15)
			chan->env_vol++;
	}

	/* length counter */
	if (0 == (chan->regs[0] & 0x20))
	{
		if (chan->vbl_length > 0)
			chan->vbl_length--;
	}

	if (0 == chan->vbl_length)
		return 0;

	freq = noise_freq[chan->regs[2] & 0x0F];
	chan->phaseacc -= (float) info->apu_incsize; /* # of cycles per sample */
	while (chan->phaseacc < 0)
	{
		chan->phaseacc += freq;

		chan->cur_pos++;
		if (NOISE_SHORT == chan->cur_pos && (chan->regs[2] & 0x80))
			chan->cur_pos = 0;
		else if (NOISE_LONG == chan->cur_pos)
			chan->cur_pos = 0;
	}

	if (chan->regs[0] & 0x10) /* fixed volume */
		outvol = chan->regs[0] & 0x0F;
	else
		outvol = 0x0F - chan->env_vol;

	output = info->noise_lut[chan->cur_pos];
	if (output > outvol)
		output = outvol;

	if (info->noise_lut[chan->cur_pos] & 0x80) /* make it negative */
		output = -output;

	return (int8) output;
}

/* RESET DPCM PARAMETERS */
INLINE void apu_dpcmreset(dpcm_t *chan)
{
	chan->address = 0xC000 + (uint16) (chan->regs[2] << 6);
	chan->length = (uint16) (chan->regs[3] << 4) + 1;
	chan->bits_left = chan->length << 3;
	chan->irq_occurred = FALSE;
	chan->enabled = TRUE; /* Fixed * Proper DPCM channel ENABLE/DISABLE flag behaviour*/
	// Note: according to NSFPlay, it does NOT do that
	chan->vol = 0; /* Fixed * DPCM DAC resets itself when restarted */
}

/* OUTPUT DPCM WAVE SAMPLE (VALUES FROM -64 to +63) */
/* TODO: centerline naughtiness */
static int8 apu_dpcm(nesapu_state *info, dpcm_t *chan)
{
	int freq, bit_pos;

	/* reg0: 7=irq gen, 6=looping, 3-0=pointer to clock table
    ** reg1: output dc level, 7 bits unsigned
    ** reg2: 8 bits of 64-byte aligned address offset : $C000 + (value * 64)
    ** reg3: length, (value * 16) + 1
    */
	if (chan->Muted)
		return 0;

	if (chan->enabled)
	{
		freq = dpcm_clocks[chan->regs[0] & 0x0F];
		chan->phaseacc -= (float) info->apu_incsize; /* # of cycles per sample */

		while (chan->phaseacc < 0)
		{
			chan->phaseacc += freq;

			if (0 == chan->length)
			{
				chan->enabled = FALSE; /* Fixed * Proper DPCM channel ENABLE/DISABLE flag behaviour*/
				chan->vol=0; /* Fixed * DPCM DAC resets itself when restarted */
				if (chan->regs[0] & 0x40)
					apu_dpcmreset(chan);
				else
				{
					if (chan->regs[0] & 0x80) /* IRQ Generator */
					{
						chan->irq_occurred = TRUE;
						//n2a03_irq(info->APU.dpcm.memory->cpu);
					}
					break;
				}
			}


			chan->bits_left--;
			bit_pos = 7 - (chan->bits_left & 7);
			if (7 == bit_pos)
			{
				//chan->cur_byte = info->APU.dpcm.memory->read_byte(chan->address);
				chan->cur_byte = info->APU.dpcm.memory[chan->address];
				chan->address++;
				// On overflow, the address is set to 8000
				if (chan->address >= 0x10000)
					chan->address -= 0x8000;
				chan->length--;
			}

			if (chan->cur_byte & (1 << bit_pos))
//              chan->regs[1]++;
				chan->vol+=2; /* FIXED * DPCM channel only uses the upper 6 bits of the DAC */
			else
//              chan->regs[1]--;
				chan->vol-=2;
		}
	}

	if (! DPCMBase0)
	{
		if (chan->vol > 63)
			chan->vol = 63;
		else if (chan->vol < -64)
			chan->vol = -64;
	}
	else
	{
		if (chan->vol > 127)
			chan->vol = 127;
		else if (chan->vol < 0)
			chan->vol = 0;
	}

	return (int8) (chan->vol);
	//return (int8) 0;
}

/* WRITE REGISTER VALUE */
INLINE void apu_regwrite(nesapu_state *info,int address, uint8 value)
{
	int chan = (address & 4) ? 1 : 0;

	switch (address)
	{
	/* squares */
	case APU_WRA0:
	case APU_WRB0:
		info->APU.squ[chan].regs[0] = value;
		break;

	case APU_WRA1:
	case APU_WRB1:
		info->APU.squ[chan].regs[1] = value;
		break;

	case APU_WRA2:
	case APU_WRB2:
		info->APU.squ[chan].regs[2] = value;
		if (info->APU.squ[chan].enabled)
			info->APU.squ[chan].freq = ((((info->APU.squ[chan].regs[3] & 7) << 8) + value) + 1) << 16;
		break;

	case APU_WRA3:
	case APU_WRB3:
		info->APU.squ[chan].regs[3] = value;

		if (info->APU.squ[chan].enabled)
		{
		// TODO: Test, if it sounds better with or without it.
		//	info->APU.squ[chan].adder = 0;	// Thanks to Delek
			info->APU.squ[chan].vbl_length = info->vbl_times[value >> 3];
			info->APU.squ[chan].env_vol = 0;
			info->APU.squ[chan].freq = ((((value & 7) << 8) + info->APU.squ[chan].regs[2]) + 1) << 16;
		}

		break;

	/* triangle */
	case APU_WRC0:
		info->APU.tri.regs[0] = value;

		if (info->APU.tri.enabled)
		{                                          /* ??? */
			if (FALSE == info->APU.tri.counter_started)
				info->APU.tri.linear_length = info->sync_times2[value & 0x7F];
		}

		break;

	//case 0x4009:
	case APU_WRC1:
		/* unused */
		info->APU.tri.regs[1] = value;
		break;

	case APU_WRC2:
		info->APU.tri.regs[2] = value;
		break;

	case APU_WRC3:
		info->APU.tri.regs[3] = value;

		/* this is somewhat of a hack.  there is some latency on the Real
        ** Thing between when trireg0 is written to and when the linear
        ** length counter actually begins its countdown.  we want to prevent
        ** the case where the program writes to the freq regs first, then
        ** to reg 0, and the counter accidentally starts running because of
        ** the sound queue's timestamp processing.
        **
        ** set to a few NES sample -- should be sufficient
        **
        **    3 * (1789772.727 / 44100) = ~122 cycles, just around one scanline
        **
        ** should be plenty of time for the 6502 code to do a couple of table
        ** dereferences and load up the other triregs
        */

	/* used to be 3, but now we run the clock faster, so base it on samples/sync */
		info->APU.tri.write_latency = (info->samps_per_sync + 239) / 240;

		if (info->APU.tri.enabled)
		{
			info->APU.tri.counter_started = FALSE;
			info->APU.tri.vbl_length = info->vbl_times[value >> 3];
			info->APU.tri.linear_length = info->sync_times2[info->APU.tri.regs[0] & 0x7F];
		}

		break;

	/* noise */
	case APU_WRD0:
		info->APU.noi.regs[0] = value;
		break;

	case 0x400D:
		/* unused */
		info->APU.noi.regs[1] = value;
		break;

	case APU_WRD2:
		info->APU.noi.regs[2] = value;
		info->APU.noi.cur_pos = 0;	// Thanks to Delek for this fix.
		break;

	case APU_WRD3:
		info->APU.noi.regs[3] = value;

		if (info->APU.noi.enabled)
		{
			info->APU.noi.vbl_length = info->vbl_times[value >> 3];
			info->APU.noi.env_vol = 0; /* reset envelope */
		}
		break;

	/* DMC */
	case APU_WRE0:
		info->APU.dpcm.regs[0] = value;
		if (0 == (value & 0x80))
			info->APU.dpcm.irq_occurred = FALSE;
		break;

	case APU_WRE1: /* 7-bit DAC */
		//info->APU.dpcm.regs[1] = value - 0x40;
		info->APU.dpcm.regs[1] = value & 0x7F;
		if (! DPCMBase0)
			info->APU.dpcm.vol = (info->APU.dpcm.regs[1]-64);
		else
			info->APU.dpcm.vol = info->APU.dpcm.regs[1];
		break;

	case APU_WRE2:
		info->APU.dpcm.regs[2] = value;
		//apu_dpcmreset(info->APU.dpcm);
		break;

	case APU_WRE3:
		info->APU.dpcm.regs[3] = value;
		break;

	case APU_IRQCTRL:
		if(value & 0x80)
			info->APU.step_mode = 5;
		else
			info->APU.step_mode = 4;
		break;

	case APU_SMASK:
		if (value & 0x01)
			info->APU.squ[0].enabled = TRUE;
		else
		{
			info->APU.squ[0].enabled = FALSE;
			info->APU.squ[0].vbl_length = 0;
		}

		if (value & 0x02)
			info->APU.squ[1].enabled = TRUE;
		else
		{
			info->APU.squ[1].enabled = FALSE;
			info->APU.squ[1].vbl_length = 0;
		}

		if (value & 0x04)
			info->APU.tri.enabled = TRUE;
		else
		{
			info->APU.tri.enabled = FALSE;
			info->APU.tri.vbl_length = 0;
			info->APU.tri.linear_length = 0;
			info->APU.tri.counter_started = FALSE;
			info->APU.tri.write_latency = 0;
		}

		if (value & 0x08)
			info->APU.noi.enabled = TRUE;
		else
		{
			info->APU.noi.enabled = FALSE;
			info->APU.noi.vbl_length = 0;
		}

		if (value & 0x10)
		{
			/* only reset dpcm values if DMA is finished */
			if (FALSE == info->APU.dpcm.enabled)
			{
				info->APU.dpcm.enabled = TRUE;
				apu_dpcmreset(&info->APU.dpcm);
			}
		}
		else
			info->APU.dpcm.enabled = FALSE;

		info->APU.dpcm.irq_occurred = FALSE;

		break;
	default:
#ifdef MAME_DEBUG
logerror("invalid apu write: $%02X at $%04X\n", value, address);
#endif
		break;
	}
}

/* UPDATE SOUND BUFFER USING CURRENT DATA */
//INLINE void apu_update(nesapu_state *info, stream_sample_t *buffer16, int samples)
INLINE void apu_update(nesapu_state *info, stream_sample_t **buffer16, int samples)
{
	int accum;
	stream_sample_t* bufL = buffer16[0];
	stream_sample_t* bufR = buffer16[1];

	while (samples--)
	{
		/*accum = apu_square(info, &info->APU.squ[0]);
		accum += apu_square(info, &info->APU.squ[1]);
		accum += apu_triangle(info, &info->APU.tri);
		accum += apu_noise(info, &info->APU.noi);
		accum += apu_dpcm(info, &info->APU.dpcm);

		// 8-bit clamps
		if (accum > 127)
			accum = 127;
		else if (accum < -128)
			accum = -128;

		*(bufL++)=accum<<8;
		*(bufR++)=accum<<8;*/

		// These volumes should match NSFPlay's NES core better
		accum = apu_square(info, &info->APU.squ[0]) << 8;	// << 8 * 1.0
		accum += apu_square(info, &info->APU.squ[1]) << 8;	// << 8 * 1.0
		accum += apu_triangle(info, &info->APU.tri) * 0xC0;	// << 8 * 0.75
		accum += apu_noise(info, &info->APU.noi) * 0xC0;	// << 8 * 0.75
		accum += apu_dpcm(info, &info->APU.dpcm) * 0xC0;	// << 8 * 0.75

		*(bufL++)=accum;
		*(bufR++)=accum;
	}
}

/* READ VALUES FROM REGISTERS */
INLINE uint8 apu_read(nesapu_state *info,int address)
{
	if (address == 0x15) /*FIXED* Address $4015 has different behaviour*/
	{
		int readval = 0;
		if (info->APU.squ[0].vbl_length > 0)
			readval |= 0x01;

		if (info->APU.squ[1].vbl_length > 0)
			readval |= 0x02;

		if (info->APU.tri.vbl_length > 0)
			readval |= 0x04;

		if (info->APU.noi.vbl_length > 0)
			readval |= 0x08;

		if (info->APU.dpcm.enabled == TRUE)
			readval |= 0x10;

		if (info->APU.dpcm.irq_occurred == TRUE)
			readval |= 0x80;

		return readval;
	}
	else
		return info->APU.regs[address];
}

/* WRITE VALUE TO TEMP REGISTRY AND QUEUE EVENT */
INLINE void apu_write(nesapu_state *info,int address, uint8 value)
{
	info->APU.regs[address]=value;
	//stream_update(info->stream);
	apu_regwrite(info,address,value);
}

/* EXTERNAL INTERFACE FUNCTIONS */

/* REGISTER READ/WRITE FUNCTIONS */
//READ8_DEVICE_HANDLER( nes_psg_r )
UINT8 nes_psg_r(void* chip, offs_t offset)
{
	//return apu_read(get_safe_token(device),offset);
	return apu_read((nesapu_state*)chip, offset);
}
//WRITE8_DEVICE_HANDLER( nes_psg_w )
void nes_psg_w(void* chip, offs_t offset, UINT8 data)
{
	//apu_write(get_safe_token(device),offset,data);
	apu_write((nesapu_state*)chip, offset, data);
}

/* UPDATE APU SYSTEM */
//static STREAM_UPDATE( nes_psg_update_sound )
void nes_psg_update_sound(void* chip, stream_sample_t **outputs, int samples)
{
	//nesapu_state *info = (nesapu_state *)param;
	nesapu_state *info = (nesapu_state*)chip;
	//apu_update(info, outputs[0], samples);
	apu_update(info, outputs, samples);
}


/* INITIALIZE APU SYSTEM */
#define SCREEN_HZ	60
//static DEVICE_START( nesapu )
void* device_start_nesapu(int clock, int rate)
{
	//const nes_interface *intf = (const nes_interface *)device->baseconfig().static_config();
	//nesapu_state *info = get_safe_token(device);
	nesapu_state *info;
	//int rate = clock / 4;
	//int i;

//	if (ChipID >= MAX_CHIPS)
//		return 0;
	
	info = (nesapu_state*)malloc(sizeof(nesapu_state));
	if (info == NULL)
		return NULL;
	
	/* Initialize global variables */
	//info->samps_per_sync = rate / ATTOSECONDS_TO_HZ(device->machine->primary_screen->frame_period().attoseconds);
	info->samps_per_sync = rate / SCREEN_HZ;
	info->buffer_size = info->samps_per_sync;
	//info->real_rate = info->samps_per_sync * ATTOSECONDS_TO_HZ(device->machine->primary_screen->frame_period().attoseconds);
	info->real_rate = info->samps_per_sync * SCREEN_HZ;
	//info->apu_incsize = (float) (device->clock() / (float) info->real_rate);
	info->apu_incsize = (float) (clock / (float) info->real_rate);

	/* Use initializer calls */
	create_noise(info->noise_lut, 13, NOISE_LONG);
	create_vbltimes(info->vbl_times,vbl_length,info->samps_per_sync);
	create_syncs(info, info->samps_per_sync);

	/* Adjust buffer size if 16 bits */
	info->buffer_size+=info->samps_per_sync;

	/* Initialize individual chips */
	//(info->APU.dpcm).memory = cputag_get_address_space(device->machine, intf->cpu_tag, ADDRESS_SPACE_PROGRAM);
	// no idea how to obtain this
	info->APU.dpcm.memory = NULL;

	//info->stream = stream_create(device, 0, 1, rate, info, nes_psg_update_sound);

	/* register for save */
	/*for (i = 0; i < 2; i++)
	{
		state_save_register_device_item_array(device, i, info->APU.squ[i].regs);
		state_save_register_device_item(device, i, info->APU.squ[i].vbl_length);
		state_save_register_device_item(device, i, info->APU.squ[i].freq);
		state_save_register_device_item(device, i, info->APU.squ[i].phaseacc);
		state_save_register_device_item(device, i, info->APU.squ[i].output_vol);
		state_save_register_device_item(device, i, info->APU.squ[i].env_phase);
		state_save_register_device_item(device, i, info->APU.squ[i].sweep_phase);
		state_save_register_device_item(device, i, info->APU.squ[i].adder);
		state_save_register_device_item(device, i, info->APU.squ[i].env_vol);
		state_save_register_device_item(device, i, info->APU.squ[i].enabled);
	}

	state_save_register_device_item_array(device, 0, info->APU.tri.regs);
	state_save_register_device_item(device, 0, info->APU.tri.linear_length);
	state_save_register_device_item(device, 0, info->APU.tri.vbl_length);
	state_save_register_device_item(device, 0, info->APU.tri.write_latency);
	state_save_register_device_item(device, 0, info->APU.tri.phaseacc);
	state_save_register_device_item(device, 0, info->APU.tri.output_vol);
	state_save_register_device_item(device, 0, info->APU.tri.adder);
	state_save_register_device_item(device, 0, info->APU.tri.counter_started);
	state_save_register_device_item(device, 0, info->APU.tri.enabled);

	state_save_register_device_item_array(device, 0, info->APU.noi.regs);
	state_save_register_device_item(device, 0, info->APU.noi.cur_pos);
	state_save_register_device_item(device, 0, info->APU.noi.vbl_length);
	state_save_register_device_item(device, 0, info->APU.noi.phaseacc);
	state_save_register_device_item(device, 0, info->APU.noi.output_vol);
	state_save_register_device_item(device, 0, info->APU.noi.env_phase);
	state_save_register_device_item(device, 0, info->APU.noi.env_vol);
	state_save_register_device_item(device, 0, info->APU.noi.enabled);

	state_save_register_device_item_array(device, 0, info->APU.dpcm.regs);
	state_save_register_device_item(device, 0, info->APU.dpcm.address);
	state_save_register_device_item(device, 0, info->APU.dpcm.length);
	state_save_register_device_item(device, 0, info->APU.dpcm.bits_left);
	state_save_register_device_item(device, 0, info->APU.dpcm.phaseacc);
	state_save_register_device_item(device, 0, info->APU.dpcm.output_vol);
	state_save_register_device_item(device, 0, info->APU.dpcm.cur_byte);
	state_save_register_device_item(device, 0, info->APU.dpcm.enabled);
	state_save_register_device_item(device, 0, info->APU.dpcm.irq_occurred);
	state_save_register_device_item(device, 0, info->APU.dpcm.vol);

	state_save_register_device_item_array(device, 0, info->APU.regs);

#ifdef USE_QUEUE
	state_save_register_device_item_array(device, 0, info->APU.queue);
	state_save_register_device_item(device, 0, info->APU.head);
	state_save_register_device_item(device, 0, info->APU.tail);
#else
	state_save_register_device_item(device, 0, info->APU.buf_pos);
	state_save_register_device_item(device, 0, info->APU.step_mode);
#endif
	*/
	
	info->APU.squ[0].Muted = 0x00;
	info->APU.squ[1].Muted = 0x00;
	info->APU.tri.Muted = 0x00;
	info->APU.noi.Muted = 0x00;
	info->APU.dpcm.Muted = 0x00;
	
	return info;
}

void device_stop_nesapu(void* chip)
{
	nesapu_state *info = (nesapu_state*)chip;
	
	info->APU.dpcm.memory = NULL;
	
	return;
}

void device_reset_nesapu(void* chip)
{
	nesapu_state *info = (nesapu_state*)chip;
	const UINT8* MemPtr;
	UINT8 CurReg;
	
	MemPtr = info->APU.dpcm.memory;
	memset(&info->APU, 0x00, sizeof(apu_t));
	info->APU.dpcm.memory = MemPtr;
	apu_dpcmreset(&info->APU.dpcm);
	
	for (CurReg = 0x00; CurReg < 0x18; CurReg ++)
		apu_write(info, CurReg, 0x00);
	
	apu_write(info, 0x15, 0x00);
	apu_write(info, 0x15, 0x0F);
	
	return;
}

void nesapu_set_rom(void* chip, const UINT8* ROMData)
{
	nesapu_state *info = (nesapu_state*)chip;
	info->APU.dpcm.memory = ROMData;
	
	return;
}

void nesapu_set_mute_mask(void* chip, UINT32 MuteMask)
{
	nesapu_state *info = (nesapu_state*)chip;
	
	info->APU.squ[0].Muted = (MuteMask >> 0) & 0x01;
	info->APU.squ[1].Muted = (MuteMask >> 1) & 0x01;
	info->APU.tri.Muted = (MuteMask >> 2) & 0x01;
	info->APU.noi.Muted = (MuteMask >> 3) & 0x01;
	info->APU.dpcm.Muted = (MuteMask >> 4) & 0x01;
	
	return;
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( nesapu )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(nesapu_state);			break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( nesapu );			break;
		case DEVINFO_FCT_STOP:							// Nothing									break;
		case DEVINFO_FCT_RESET:							// Nothing									break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "N2A03");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Nintendo custom");				break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);      					break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team");  break;
	}
}


DEFINE_LEGACY_SOUND_DEVICE(NES, nesapu);*/
