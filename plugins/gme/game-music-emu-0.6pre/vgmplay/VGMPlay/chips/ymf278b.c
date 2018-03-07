/*

   YMF278B  FM + Wave table Synthesizer (OPL4)

   Timer and PCM YMF278B.  The FM is shared with the ymf262.

   This chip roughly splits the difference between the Sega 315-5560 MultiPCM
   (Multi32, Model 1/2) and YMF 292-F SCSP (later Model 2, STV, Saturn, Model 3).

   Features as listed in LSI-4MF2782 data sheet:
    FM Synthesis (same as YMF262)
     1. Sound generation mode
         Two-operater mode
          Generates eighteen voices or fifteen voices plus five rhythm sounds simultaneously
         Four-operator mode
          Generates six voices in four-operator mode plus six voices in two-operator mode simultaneously,
          or generates six voices in four-operator mode plus three voices in two-operator mode plus five
          rhythm sounds simultaneously
     2. Eight selectable waveforms
     3. Stereo output
    Wave Table Synthesis
     1. Generates twenty-four voices simultaneously
     2. 44.1kHz sampling rate for output sound data
     3. Selectable from 8-bit, 12-bit and 16-bit word lengths for wave data
     4. Stereo output (16-stage panpot for each voice)
    Wave Data
     1. Accepts 32M bit external memory at maximum
     2. Up to 512 wave tables
     3. External ROM or SRAM can be connected. With SRAM connected, the CPU can download wave data
     4. Outputs chip select signals for 1Mbit, 4Mbit, 8Mbit or 16Mbit memory
     5. Can be directly connected to the Yamaha YRW801 (Wave data ROM)
        Features of YRW801 as listed in LSI 4RW801A2
          Built-in wave data of tones which comply with GM system Level 1
           Melody tone ....... 128 tones
           Percussion tone ...  47 tones
          16Mbit capacity (2,097,152word x 8)

   By R. Belmont and O. Galibert.

   Copyright R. Belmont and O. Galibert.

   This software is dual-licensed: it may be used in MAME and properly licensed
   MAME derivatives under the terms of the MAME license.  For use outside of
   MAME and properly licensed derivatives, it is available under the
   terms of the GNU Lesser General Public License (LGPL), version 2.1.
   You may read the LGPL at http://www.gnu.org/licenses/lgpl.html

   Changelog:
   Sep. 8, 2002 - fixed ymf278b_compute_rate when OCT is negative (RB)
   Dec. 11, 2002 - added ability to set non-standard clock rates (RB)
                   fixed envelope target for release (fixes missing
           instruments in hotdebut).
                   Thanks to Team Japump! for MP3s from a real PCB.
           fixed crash if MAME is run with no sound.
   June 4, 2003 -  Changed to dual-license with LGPL for use in OpenMSX.
                   OpenMSX contributed a bugfix where looped samples were
            not being addressed properly, causing pitch fluctuation.
   August 15, 2010 - Backport to MAME-style C from OpenMSX
*/

#include <math.h>
#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
//#include "cpuintrf.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "ymf262.h"
#include "ymf278b.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

typedef struct
{
	UINT32 startaddr;
	UINT32 loopaddr;
	UINT32 endaddr;
	UINT32 step;	/* fixed-point frequency step */
	UINT32 stepptr;	/* fixed-point pointer into the sample */
	UINT16 pos;
	INT16 sample1, sample2;

	INT32 env_vol;
	
	INT32 lfo_cnt;
	INT32 lfo_step;
	INT32 lfo_max;
	
	INT16 wave;		/* wavetable number */
	INT16 FN;		/* f-number */
	INT8 OCT;		/* octave */
	INT8 PRVB;		/* pseudo-reverb */
	INT8 LD;		/* level direct */
	INT8 TL;		/* total level */
	INT8 pan;		/* panpot */
	INT8 lfo;		/* LFO */
	INT8 vib;		/* vibrato */
	INT8 AM;		/* AM level */

	INT8 AR;
	INT8 D1R;
	INT32 DL;
	INT8 D2R;
	INT8 RC;		/* rate correction */
	INT8 RR;

	INT8 bits;		/* width of the samples */
	INT8 active;		/* slot keyed on */
	
	INT8 state;
	INT8 lfo_active;
	
	UINT8 Muted;
} YMF278BSlot;

typedef struct
{
	YMF278BSlot slots[24];
	
	UINT32 eg_cnt;	/* Global envelope generator counter. */
	
	INT8 wavetblhdr;
	INT8 memmode;
	INT32 memadr;

	INT32 fm_l, fm_r;
	INT32 pcm_l, pcm_r;

	//UINT8 timer_a_count, timer_b_count, enable, current_irq;
	//emu_timer *timer_a, *timer_b;
	//int irq_line;

	UINT8 port_A, port_B, port_C;
	//void (*irq_callback)(const device_config *, int);
	void (*irq_callback)(int);
	//const device_config *device;

	UINT32 ROMSize;
	UINT8 *rom;
	UINT32 RAMSize;
	UINT8 *ram;
	int clock;

	INT32 volume[256*4];			// precalculated attenuation values with some marging for enveloppe and pan levels

	UINT8 regs[256];
	
	void *fmchip;
	UINT8 FMEnabled;	// that saves a whole lot of CPU
	//sound_stream * stream;
} YMF278BChip;

#define EG_SH	16	// 16.16 fixed point (EG timing)
#define EG_TIMER_OVERFLOW	(1 << EG_SH)

// envelope output entries
#define ENV_BITS	10
#define ENV_LEN		(1 << ENV_BITS)
#define ENV_STEP	(128.0 / ENV_LEN)
#define MAX_ATT_INDEX	((1 << (ENV_BITS - 1)) - 1)	// 511
#define MIN_ATT_INDEX	0

// Envelope Generator phases
#define EG_ATT	4
#define EG_DEC	3
#define EG_SUS	2
#define EG_REL	1
#define EG_OFF	0

#define EG_REV	5	// pseudo reverb
#define EG_DMP	6	// damp

// Pan values, units are -3dB, i.e. 8.
const INT32 pan_left[16]  = {
	0, 8, 16, 24, 32, 40, 48, 256, 256,   0,  0,  0,  0,  0,  0, 0
};
const INT32 pan_right[16] = {
	0, 0,  0,  0,  0,  0,  0,   0, 256, 256, 48, 40, 32, 24, 16, 8
};

// Mixing levels, units are -3dB, and add some marging to avoid clipping
const INT32 mix_level[8] = {
	8, 16, 24, 32, 40, 48, 56, 256
};

// decay level table (3dB per step)
// 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)
#define SC(db) (db * (2.0 / ENV_STEP))
const UINT32 dl_tab[16] = {
 SC( 0), SC( 1), SC( 2), SC(3 ), SC(4 ), SC(5 ), SC(6 ), SC( 7),
 SC( 8), SC( 9), SC(10), SC(11), SC(12), SC(13), SC(14), SC(31)
};
#undef SC

#define RATE_STEPS	8
const UINT8 eg_inc[15 * RATE_STEPS] = {
//cycle:0  1   2  3   4  5   6  7
	0, 1,  0, 1,  0, 1,  0, 1, //  0  rates 00..12 0 (increment by 0 or 1)
	0, 1,  0, 1,  1, 1,  0, 1, //  1  rates 00..12 1
	0, 1,  1, 1,  0, 1,  1, 1, //  2  rates 00..12 2
	0, 1,  1, 1,  1, 1,  1, 1, //  3  rates 00..12 3

	1, 1,  1, 1,  1, 1,  1, 1, //  4  rate 13 0 (increment by 1)
	1, 1,  1, 2,  1, 1,  1, 2, //  5  rate 13 1
	1, 2,  1, 2,  1, 2,  1, 2, //  6  rate 13 2
	1, 2,  2, 2,  1, 2,  2, 2, //  7  rate 13 3

	2, 2,  2, 2,  2, 2,  2, 2, //  8  rate 14 0 (increment by 2)
	2, 2,  2, 4,  2, 2,  2, 4, //  9  rate 14 1
	2, 4,  2, 4,  2, 4,  2, 4, // 10  rate 14 2
	2, 4,  4, 4,  2, 4,  4, 4, // 11  rate 14 3

	4, 4,  4, 4,  4, 4,  4, 4, // 12  rates 15 0, 15 1, 15 2, 15 3 for decay
	8, 8,  8, 8,  8, 8,  8, 8, // 13  rates 15 0, 15 1, 15 2, 15 3 for attack (zero time)
	0, 0,  0, 0,  0, 0,  0, 0, // 14  infinity rates for attack and decay(s)
};

#define O(a) (a * RATE_STEPS)
const UINT8 eg_rate_select[64] = {
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 4),O( 5),O( 6),O( 7),
	O( 8),O( 9),O(10),O(11),
	O(12),O(12),O(12),O(12),
};
#undef O

// rate  0,    1,    2,    3,   4,   5,   6,  7,  8,  9,  10, 11, 12, 13, 14, 15
// shift 12,   11,   10,   9,   8,   7,   6,  5,  4,  3,  2,  1,  0,  0,  0,  0
// mask  4095, 2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3,  1,  0,  0,  0,  0
#define O(a) (a)
const UINT8 eg_rate_shift[64] = {
	O(12),O(12),O(12),O(12),
	O(11),O(11),O(11),O(11),
	O(10),O(10),O(10),O(10),
	O( 9),O( 9),O( 9),O( 9),
	O( 8),O( 8),O( 8),O( 8),
	O( 7),O( 7),O( 7),O( 7),
	O( 6),O( 6),O( 6),O( 6),
	O( 5),O( 5),O( 5),O( 5),
	O( 4),O( 4),O( 4),O( 4),
	O( 3),O( 3),O( 3),O( 3),
	O( 2),O( 2),O( 2),O( 2),
	O( 1),O( 1),O( 1),O( 1),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
};
#undef O


// number of steps to take in quarter of lfo frequency
// TODO check if frequency matches real chip
#define O(a) ((EG_TIMER_OVERFLOW / a) / 6)
const INT32 lfo_period[8] = {
	O(0.168), O(2.019), O(3.196), O(4.206),
	O(5.215), O(5.888), O(6.224), O(7.066)
};
#undef O


#define O(a) (a * 65536)
const INT32 vib_depth[8] = {
	O(0),	   O(3.378),  O(5.065),  O(6.750),
	O(10.114), O(20.170), O(40.106), O(79.307)
};
#undef O


#define SC(db) (INT32)(db * (2.0 / ENV_STEP))
const INT32 am_depth[8] = {
	SC(0),	   SC(1.781), SC(2.906), SC(3.656),
	SC(4.406), SC(5.906), SC(7.406), SC(11.91)
};
#undef SC

void ymf278b_slot_reset(YMF278BSlot* slot)
{
	slot->wave = slot->FN = slot->OCT = slot->PRVB = slot->LD = slot->TL = slot->pan =
		slot->lfo = slot->vib = slot->AM = 0;
	slot->AR = slot->D1R = slot->DL = slot->D2R = slot->RC = slot->RR = 0;
	slot->step = slot->stepptr = 0;
	slot->bits = slot->startaddr = slot->loopaddr = slot->endaddr = 0;
	slot->env_vol = MAX_ATT_INDEX;

	slot->lfo_active = 0;
	slot->lfo_cnt = slot->lfo_step = 0;
	slot->lfo_max = lfo_period[0];

	slot->state = EG_OFF;
	slot->active = 0;

	// not strictly needed, but avoid UMR on savestate
	slot->pos = slot->sample1 = slot->sample2 = 0;
}

INLINE int ymf278b_slot_compute_rate(YMF278BSlot* slot, int val)
{
	int res;
	int oct;
	
	if (val == 0)
		return 0;
	else if (val == 15)
		return 63;
	
	if (slot->RC != 15)
	{
		oct = slot->OCT;
		
		if (oct & 8)
		{
			oct |= -8;
		}
		res = (oct + slot->RC) * 2 + (slot->FN & 0x200 ? 1 : 0) + val * 4;
	}
	else
	{
		res = val * 4;
	}
	
	if (res < 0)
		res = 0;
	else if (res > 63)
		res = 63;
	
	return res;
}

INLINE int ymf278b_slot_compute_vib(YMF278BSlot* slot)
{
	return (((slot->lfo_step << 8) / slot->lfo_max) * vib_depth[slot->vib]) >> 24;
}


INLINE int ymf278b_slot_compute_am(YMF278BSlot* slot)
{
	if (slot->lfo_active && slot->AM)
		return (((slot->lfo_step << 8) / slot->lfo_max) * am_depth[slot->AM]) >> 12;
	else
		return 0;
}

INLINE void ymf278b_slot_set_lfo(YMF278BSlot* slot, int newlfo)
{
	slot->lfo_step = (((slot->lfo_step << 8) / slot->lfo_max) * newlfo) >> 8;
	slot->lfo_cnt  = (((slot->lfo_cnt  << 8) / slot->lfo_max) * newlfo) >> 8;

	slot->lfo = newlfo;
	slot->lfo_max = lfo_period[slot->lfo];
}

INLINE void ymf278b_advance(YMF278BChip* chip)
{
	YMF278BSlot* op;
	int i;
	UINT8 rate;
	UINT8 shift;
	UINT8 select;
	
	chip->eg_cnt ++;
	for (i = 0; i < 24; i ++)
	{
		op = &chip->slots[i];

		if (op->lfo_active)
		{
			op->lfo_cnt ++;
			if (op->lfo_cnt < op->lfo_max)
			{
				op->lfo_step ++;
			}
			else if (op->lfo_cnt < (op->lfo_max * 3))
			{
				op->lfo_step --;
			}
			else
			{
				op->lfo_step ++;
				if (op->lfo_cnt == (op->lfo_max * 4))
					op->lfo_cnt = 0;
			}
		}

		// Envelope Generator
		switch(op->state)
		{
		case EG_ATT:	// attack phase
			rate = ymf278b_slot_compute_rate(op, op->AR);
			if (rate < 4)
				break;
			
			shift = eg_rate_shift[rate];
			if (! (chip->eg_cnt & ((1 << shift) - 1)))
			{
				select = eg_rate_select[rate];
				op->env_vol += (~op->env_vol * eg_inc[select + ((chip->eg_cnt >> shift) & 7)]) >> 3;
				if (op->env_vol <= MIN_ATT_INDEX)
				{
					op->env_vol = MIN_ATT_INDEX;
					if (op->DL)
						op->state = EG_DEC;
					else
						op->state = EG_SUS;
				}
			}
			break;
		case EG_DEC:	// decay phase
			rate = ymf278b_slot_compute_rate(op, op->D1R);
			if (rate < 4)
				break;
			
			shift = eg_rate_shift[rate];
			if (! (chip->eg_cnt & ((1 << shift) - 1)))
			{
				select = eg_rate_select[rate];
				op->env_vol += eg_inc[select + ((chip->eg_cnt >> shift) & 7)];

				if ((op->env_vol > dl_tab[6]) && op->PRVB)
					op->state = EG_REV;
				else
				{
					if (op->env_vol >= op->DL)
						op->state = EG_SUS;
				}
			}
			break;
		case EG_SUS:	// sustain phase
			rate = ymf278b_slot_compute_rate(op, op->D2R);
			if (rate < 4)
				break;
			
			shift = eg_rate_shift[rate];
			if (! (chip->eg_cnt & ((1 << shift) - 1)))
			{
				select = eg_rate_select[rate];
				op->env_vol += eg_inc[select + ((chip->eg_cnt >> shift) & 7)];

				if ((op->env_vol > dl_tab[6]) && op->PRVB)
					op->state = EG_REV;
				else
				{
					if (op->env_vol >= MAX_ATT_INDEX)
					{
						op->env_vol = MAX_ATT_INDEX;
						op->active = 0;
					}
				}
			}
			break;
		case EG_REL:	// release phase
			rate = ymf278b_slot_compute_rate(op, op->RR);
			if (rate < 4)
				break;
			
			shift = eg_rate_shift[rate];
			if (! (chip->eg_cnt & ((1 << shift) - 1)))
			{
				select = eg_rate_select[rate];
				op->env_vol += eg_inc[select + ((chip->eg_cnt >> shift) & 7)];

				if ((op->env_vol > dl_tab[6]) && op->PRVB)
					op->state = EG_REV;
				else
				{
					if (op->env_vol >= MAX_ATT_INDEX)
					{
						op->env_vol = MAX_ATT_INDEX;
						op->active = 0;
					}
				}
			}
			break;
		case EG_REV:	// pseudo reverb
			// TODO improve env_vol update
			rate = ymf278b_slot_compute_rate(op, 5);
			//if (rate < 4)
			//	break;
			
			shift = eg_rate_shift[rate];
			if (! (chip->eg_cnt & ((1 << shift) - 1)))
			{
				select = eg_rate_select[rate];
				op->env_vol += eg_inc[select + ((chip->eg_cnt >> shift) & 7)];

				if (op->env_vol >= MAX_ATT_INDEX)
				{
					op->env_vol = MAX_ATT_INDEX;
					op->active = 0;
				}
			}
			break;
		case EG_DMP:	// damping
			// TODO improve env_vol update, damp is just fastest decay now
			rate = 56;
			shift = eg_rate_shift[rate];
			if (! (chip->eg_cnt & ((1 << shift) - 1)))
			{
				select = eg_rate_select[rate];
				op->env_vol += eg_inc[select + ((chip->eg_cnt >> shift) & 7)];

				if (op->env_vol >= MAX_ATT_INDEX)
				{
					op->env_vol = MAX_ATT_INDEX;
					op->active = 0;
				}
			}
			break;
		case EG_OFF:
			// nothing
			break;

		default:
#ifdef _DEBUG
			//logerror(...);
#endif
			break;
		}
	}
}

INLINE UINT8 ymf278b_readMem(YMF278BChip* chip, offs_t address)
{
	if (address < chip->ROMSize)
		return chip->rom[address&0x3fffff];
	else if (address < chip->ROMSize + chip->RAMSize)
		return chip->ram[address - chip->ROMSize&0x3fffff];
	else
		return 255; // TODO check
}

INLINE UINT8* ymf278b_readMemAddr(YMF278BChip* chip, offs_t address)
{
	if (address < chip->ROMSize)
		return &chip->rom[address&0x3fffff];
	else if (address < chip->ROMSize + chip->RAMSize)
		return &chip->ram[address - chip->ROMSize&0x3fffff];
	else
		return NULL; // TODO check
}

INLINE void ymf278b_writeMem(YMF278BChip* chip, offs_t address, UINT8 value)
{
	if (address < chip->ROMSize)
		return; // can't write to ROM
	else if (address < chip->ROMSize + chip->RAMSize)
		chip->ram[address - chip->ROMSize] = value;
	else
		return;	// can't write to unmapped memory
	
	return;
}

INLINE INT16 ymf278b_getSample(YMF278BChip* chip, YMF278BSlot* op)
{
	INT16 sample;
	UINT32 addr;
	UINT8* addrp;
	
	switch (op->bits)
	{
	case 0:
		// 8 bit
		sample = ymf278b_readMem(chip, op->startaddr + op->pos) << 8;
		break;
	case 1:
		// 12 bit
		addr = op->startaddr + ((op->pos / 2) * 3);
		addrp = ymf278b_readMemAddr(chip, addr);
		if (op->pos & 1)
			sample = (addrp[2] << 8) | ((addrp[1] << 4) & 0xF0);
		else
			sample = (addrp[0] << 8) | (addrp[1] & 0xF0);
		break;
	case 2:
		// 16 bit
		addr = op->startaddr + (op->pos * 2);
		addrp = ymf278b_readMemAddr(chip, addr);
		sample = (addrp[0] << 8) | addrp[1];
		break;
	default:
		// TODO unspecified
		sample = 0;
		break;
	}
	return sample;
}

int ymf278b_anyActive(YMF278BChip* chip)
{
	int i;
	
	for (i = 0; i < 24; i ++)
	{
		if (chip->slots[i].active)
			return 1;
	}
	return 0;
}

void ymf278b_pcm_update(void *_info, stream_sample_t** outputs, int samples)
{
	YMF278BChip* chip = (YMF278BChip *)_info;
	int i;
	unsigned int j;
	INT32 vl;
	INT32 vr;
	
	if (chip->FMEnabled)
	{
		/* memset is done by ymf262_update */
		ymf262_update_one(chip->fmchip, outputs, samples);
	}
	else
	{
		memset(outputs[0], 0x00, samples * sizeof(stream_sample_t));
		memset(outputs[1], 0x00, samples * sizeof(stream_sample_t));
	}
	
	if (! ymf278b_anyActive(chip))
	{
		// TODO update internal state, even if muted
		// TODO also mute individual channels
		return;
	}

	vl = mix_level[chip->pcm_l];
	vr = mix_level[chip->pcm_r];
	for (j = 0; j < samples; j ++)
	{
		for (i = 0; i < 24; i ++)
		{
			YMF278BSlot* sl;
			INT16 sample;
			int vol;
			int volLeft;
			int volRight;
			
			sl = &chip->slots[i];
			if (! sl->active || sl->Muted)
			{
				//outputs[0][j] += 0;
				//outputs[1][j] += 0;
				continue;
			}

			sample = (sl->sample1 * (0x10000 - sl->stepptr) +
						sl->sample2 * sl->stepptr) >> 16;
			vol = sl->TL + (sl->env_vol >> 2) + ymf278b_slot_compute_am(sl);

			volLeft  = vol + pan_left [sl->pan] + vl;
			volRight = vol + pan_right[sl->pan] + vr;
			// TODO prob doesn't happen in real chip
			//volLeft  = std::max(0, volLeft);
			//volRight = std::max(0, volRight);
			volLeft &= 0x3FF;	// catch negative Volume values in a hardware-like way
			volRight &= 0x3FF;	// (anything beyond 0x100 results in *0)

			outputs[0][j] += (sample * chip->volume[volLeft] ) >> 17;
			outputs[1][j] += (sample * chip->volume[volRight]) >> 17;

			if (sl->lfo_active && sl->vib)
			{
				int oct;
				unsigned int step;
				
				oct = sl->OCT;
				if (oct & 8)
					oct |= -8;
				oct += 5;
				step = (sl->FN | 1024) + ymf278b_slot_compute_vib(sl);
				if (oct >= 0)
					step <<= oct;
				else
					step >>= -oct;
				sl->stepptr += step;
			}
			else
				sl->stepptr += sl->step;

			while (sl->stepptr >= 0x10000)
			{
				sl->stepptr -= 0x10000;
				sl->sample1 = sl->sample2;
				
				sl->sample2 = ymf278b_getSample(chip, sl);
				if (sl->pos == sl->endaddr)
					sl->pos = sl->pos - sl->endaddr + sl->loopaddr;
				else
					sl->pos ++;
			}
		}
		ymf278b_advance(chip);
	}
}

INLINE void ymf278b_keyOnHelper(YMF278BChip* chip, YMF278BSlot* slot)
{
	int oct;
	unsigned int step;
	
	slot->active = 1;

	oct = slot->OCT;
	if (oct & 8)
		oct |= -8;
	oct += 5;
	step = slot->FN | 1024;
	if (oct >= 0)
		step <<= oct;
	else
		step >>= -oct;
	slot->step = step;
	slot->state = EG_ATT;
	slot->stepptr = 0;
	slot->pos = 0;
	slot->sample1 = ymf278b_getSample(chip, slot);
	slot->pos = 1;
	slot->sample2 = ymf278b_getSample(chip, slot);
}

static void ymf278b_A_w(YMF278BChip *chip, UINT8 reg, UINT8 data)
{
	switch(reg)
	{
		case 0x02:
			//chip->timer_a_count = data;
			//ymf278b_timer_a_reset(chip);
			break;
		case 0x03:
			//chip->timer_b_count = data;
			//ymf278b_timer_b_reset(chip);
			break;
		case 0x04:
			/*if(data & 0x80)
				chip->current_irq = 0;
			else
			{
				UINT8 old_enable = chip->enable;
				chip->enable = data;
				chip->current_irq &= ~data;
				if((old_enable ^ data) & 1)
					ymf278b_timer_a_reset(chip);
				if((old_enable ^ data) & 2)
					ymf278b_timer_b_reset(chip);
			}
			ymf278b_irq_check(chip);*/
			break;
		default:
//#ifdef _DEBUG
//			logerror("YMF278B:  Port A write %02x, %02x\n", reg, data);
//#endif
			ymf262_write(chip->fmchip, 1, data);
			if ((reg & 0xF0) == 0xB0 && (data & 0x20))	// Key On set
				chip->FMEnabled = 0x01;
			else if (reg == 0xBD && (data & 0x1F))	// one of the Rhythm bits set
				chip->FMEnabled = 0x01;
			break;
	}
}

static void ymf278b_B_w(YMF278BChip *chip, UINT8 reg, UINT8 data)
{
	switch(reg)
	{
		case 0x05:	// OPL3/OPL4 Enable
			// actually Bit 1 enables OPL4 WaveTable Synth
			ymf262_write(chip->fmchip, 3, data & ~0x02);
			break;
		default:
			ymf262_write(chip->fmchip, 3, data);
			if ((reg & 0xF0) == 0xB0 && (data & 0x20))
				chip->FMEnabled = 0x01;
			break;
	}
//#ifdef _DEBUG
//	logerror("YMF278B:  Port B write %02x, %02x\n", reg, data);
//#endif
}

void ymf278b_C_w(YMF278BChip* chip, UINT8 reg, UINT8 data)
{
	// Handle slot registers specifically
	if (reg >= 0x08 && reg <= 0xF7)
	{
		int snum = (reg - 8) % 24;
		YMF278BSlot* slot = &chip->slots[snum];
		int base;
		UINT8* buf;
		int oct;
		unsigned int step;
		
		switch((reg - 8) / 24)
		{
		case 0:
			//loadTime = time + LOAD_DELAY;

			slot->wave = (slot->wave & 0x100) | data;
			base = (slot->wave < 384 || ! chip->wavetblhdr) ?
					(slot->wave * 12) :
					(chip->wavetblhdr * 0x80000 + ((slot->wave - 384) * 12));
			buf = ymf278b_readMemAddr(chip, base);
			
			slot->bits = (buf[0] & 0xC0) >> 6;
			ymf278b_slot_set_lfo(slot, (buf[7] >> 3) & 7);
			slot->vib  = buf[7] & 7;
			slot->AR   = buf[8] >> 4;
			slot->D1R  = buf[8] & 0xF;
			slot->DL   = dl_tab[buf[9] >> 4];
			slot->D2R  = buf[9] & 0xF;
			slot->RC   = buf[10] >> 4;
			slot->RR   = buf[10] & 0xF;
			slot->AM   = buf[11] & 7;
			slot->startaddr = buf[2] | (buf[1] << 8) |
								((buf[0] & 0x3F) << 16);
			slot->loopaddr = buf[4] + (buf[3] << 8);
			slot->endaddr  = ((buf[6] + (buf[5] << 8)) ^ 0xFFFF);
			
			if (chip->regs[reg + 4] & 0x080)
				ymf278b_keyOnHelper(chip, slot);
			break;
		case 1:
			slot->wave = (slot->wave & 0xFF) | ((data & 0x1) << 8);
			slot->FN = (slot->FN & 0x380) | (data >> 1);
			
			oct = slot->OCT;
			if (oct & 8)
				oct |= -8;
			oct += 5;
			step = slot->FN | 1024;
			if (oct >= 0)
				step <<= oct;
			else
				step >>= -oct;
			slot->step = step;
			break;
		case 2:
			slot->FN = (slot->FN & 0x07F) | ((data & 0x07) << 7);
			slot->PRVB = ((data & 0x08) >> 3);
			slot->OCT =  ((data & 0xF0) >> 4);
			
			oct = slot->OCT;
			if (oct & 8)
				oct |= -8;
			oct += 5;
			step = slot->FN | 1024;
			if (oct >= 0)
				step <<= oct;
			else
				step >>= -oct;
			slot->step = step;
			break;
		case 3:
			slot->TL = data >> 1;
			slot->LD = data & 0x1;

			// TODO
			if (slot->LD) {
				// directly change volume
			} else {
				// interpolate volume
			}
			break;
		case 4:
			if (data & 0x10)
			{
				// output to DO1 pin:
				// this pin is not used in moonsound
				// we emulate this by muting the sound
				slot->pan = 8; // both left/right -inf dB
			}
			else
				slot->pan = data & 0x0F;

			if (data & 0x020)
			{
				// LFO reset
				slot->lfo_active = 0;
				slot->lfo_cnt = 0;
				slot->lfo_max = lfo_period[slot->vib];
				slot->lfo_step = 0;
			}
			else
			{
				// LFO activate
				slot->lfo_active = 1;
			}

			switch (data >> 6)
			{
			case 0: // tone off, no damp
				if (slot->active && (slot->state != EG_REV))
					slot->state = EG_REL;
				break;
			case 2: // tone on, no damp
				if (! (chip->regs[reg] & 0x080))
					ymf278b_keyOnHelper(chip, slot);
				break;
			case 1: // tone off, damp
			case 3: // tone on,  damp
				slot->state = EG_DMP;
				break;
			}
			break;
		case 5:
			slot->vib = data & 0x7;
			ymf278b_slot_set_lfo(slot, (data >> 3) & 0x7);
			break;
		case 6:
			slot->AR  = data >> 4;
			slot->D1R = data & 0xF;
			break;
		case 7:
			slot->DL  = dl_tab[data >> 4];
			slot->D2R = data & 0xF;
			break;
		case 8:
			slot->RC = data >> 4;
			slot->RR = data & 0xF;
			break;
		case 9:
			slot->AM = data & 0x7;
			break;
		}
	}
	else
	{
		// All non-slot registers
		switch (reg)
		{
		case 0x00: // TEST
		case 0x01:
			break;

		case 0x02:
			chip->wavetblhdr = (data >> 2) & 0x7;
			chip->memmode = data & 1;
			break;

		case 0x03:
			chip->memadr = (chip->memadr & 0x00FFFF) | (data << 16);
			break;

		case 0x04:
			chip->memadr = (chip->memadr & 0xFF00FF) | (data << 8);
			break;

		case 0x05:
			chip->memadr = (chip->memadr & 0xFFFF00) | data;
			break;

		case 0x06:  // memory data
			//busyTime = time + MEM_WRITE_DELAY;
			ymf278b_writeMem(chip, chip->memadr, data);
			chip->memadr = (chip->memadr + 1) & 0xFFFFFF;
			break;

		case 0xF8:
			// TODO use these
			chip->fm_l = data & 0x7;
			chip->fm_r = (data >> 3) & 0x7;
			break;

		case 0xF9:
			chip->pcm_l = data & 0x7;
			chip->pcm_r = (data >> 3) & 0x7;
			break;
		}
	}

	chip->regs[reg] = data;
}

UINT8 ymf278b_readReg(YMF278BChip* chip, UINT8 reg)
{
	// no need to call updateStream(time)
	UINT8 result;
	switch(reg)
	{
	case 2: // 3 upper bits are device ID
		result = (chip->regs[2] & 0x1F) | 0x20;
		break;

	case 6: // Memory Data Register
		//busyTime = time + MEM_READ_DELAY;
		result = ymf278b_readMem(chip, chip->memadr);
		chip->memadr = (chip->memadr + 1) & 0xFFFFFF;
		break;

	default:
		result = chip->regs[reg];
		break;
	}
	
	return result;
}

UINT8 ymf278b_peekReg(YMF278BChip* chip, UINT8 reg)
{
	UINT8 result;
	
	switch(reg)
	{
	case 2: // 3 upper bits are device ID
		result = (chip->regs[2] & 0x1F) | 0x20;
		break;

	case 6: // Memory Data Register
		result = ymf278b_readMem(chip, chip->memadr);
		break;

	default:
		result = chip->regs[reg];
		break;
	}
	return result;
}

UINT8 ymf278b_readStatus(YMF278BChip* chip)
{
	UINT8 result = 0;
	//if (time < busyTime)
	//	result |= 0x01;
	//if (time < loadTime)
	//	result |= 0x02;
	return result;
}

//WRITE8_DEVICE_HANDLER( ymf278b_w )
void ymf278b_w(void *_info, offs_t offset, UINT8 data)
{
	//YMF278BChip *chip = get_safe_token(device);
	YMF278BChip *chip = (YMF278BChip *)_info;

	switch (offset)
	{
		case 0:
			chip->port_A = data;
			ymf262_write(chip->fmchip, offset, data);
			break;

		case 1:
			ymf278b_A_w(chip, chip->port_A, data);
			break;

		case 2:
			chip->port_B = data;
			ymf262_write(chip->fmchip, offset, data);
			break;

		case 3:
			ymf278b_B_w(chip, chip->port_B, data);
			break;

		case 4:
			chip->port_C = data;
			break;

		case 5:
			ymf278b_C_w(chip, chip->port_C, data);
			break;

		default:
#ifdef _DEBUG
			logerror("YMF278B: unexpected write at offset %X to ymf278b = %02X\n", offset, data);
#endif
			break;
	}
}

void ymf278b_clearRam(YMF278BChip* chip)
{
	memset(chip->ram, 0, chip->RAMSize);
}

static void ymf278b_load_rom(YMF278BChip *chip)
{
	chip->ROMSize = 0x00200000;
	chip->rom = (UINT8*)malloc(chip->ROMSize);
	memset(chip->rom, 0xFF, chip->ROMSize);
	
	return;
}

static int ymf278b_init(YMF278BChip *chip, int clock, void (*cb)(int))
{
	int rate;
	
	rate = clock / 768;
	//if (((CHIP_SAMPLING_MODE & 0x01) && rate < CHIP_SAMPLE_RATE) ||
	//	CHIP_SAMPLING_MODE == 0x02)
	//	rate = CHIP_SAMPLE_RATE;
	chip->fmchip = ymf262_init(clock * 8 / 19, rate);
	chip->FMEnabled = 0x00;
	
	chip->rom = NULL;
	chip->irq_callback = cb;
	//chip->timer_a = timer_alloc(device->machine, ymf278b_timer_a_tick, chip);
	//chip->timer_b = timer_alloc(device->machine, ymf278b_timer_b_tick, chip);
	chip->clock = clock;

	ymf278b_load_rom(chip);
	chip->RAMSize = 0x00080000;
	chip->ram = (UINT8*)malloc(chip->RAMSize);
	ymf278b_clearRam(chip);

	return rate;
}

//static DEVICE_START( ymf278b )
int device_start_ymf278b(void **_info, int clock)
{
	static const ymf278b_interface defintrf = { 0 };
	const ymf278b_interface *intf;
	int i;
	YMF278BChip *chip;
	int rate;

	chip = (YMF278BChip *) calloc(1, sizeof(YMF278BChip));
	*_info = (void *) chip;
	
	//chip->device = device;
	//intf = (device->static_config != NULL) ? (const ymf278b_interface *)device->static_config : &defintrf;
	intf = &defintrf;

	rate = ymf278b_init(chip, clock, intf->irq_callback);
	//chip->stream = stream_create(device, 0, 2, device->clock/768, chip, ymf278b_pcm_update);

	chip->memadr = 0; // avoid UMR

	// Volume table, 1 = -0.375dB, 8 = -3dB, 256 = -96dB
	for (i = 0; i < 256; i ++)
		chip->volume[i] = 32768 * pow(2.0, (-0.375 / 6) * i);
	for (i = 256; i < 256 * 4; i ++)
		chip->volume[i] = 0;
	for (i = 0; i < 24; i ++)
		chip->slots[i].Muted = 0x00;;

	return rate;
}

//static DEVICE_STOP( ymf278 )
void device_stop_ymf278b(void *_info)
{
	YMF278BChip* chip = (YMF278BChip *)_info;
	
	ymf262_shutdown(chip->fmchip);
	free(chip->rom);	chip->rom = NULL;

	free(chip);	

	return;
}

void device_reset_ymf278b(void *_info)
{
	YMF278BChip* chip = (YMF278BChip *)_info;
	int i;
	
	ymf262_reset_chip(chip->fmchip);
	chip->FMEnabled = 0x00;
	
	chip->eg_cnt = 0;

	for (i = 0; i < 24; i ++)
		ymf278b_slot_reset(&chip->slots[i]);
	for (i = 255; i >= 0; i --)	// reverse order to avoid UMR
		ymf278b_C_w(chip, i, 0);
	
	chip->wavetblhdr = chip->memmode = chip->memadr = 0;
	chip->fm_l = chip->fm_r = chip->pcm_l = chip->pcm_r = 0;
	//busyTime = time;
	//loadTime = time;
}

void ymf278b_write_rom(void *_info, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
					  const UINT8* ROMData)
{
	YMF278BChip *chip = (YMF278BChip *)_info;
	
	if (chip->ROMSize != ROMSize)
	{
		chip->rom = (UINT8*)realloc(chip->rom, ROMSize);
		chip->ROMSize = ROMSize;
		memset(chip->rom, 0xFF, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(chip->rom + DataStart, ROMData, DataLength);
	
	return;
}


void ymf278b_set_mute_mask(void *_info, UINT32 MuteMaskFM, UINT32 MuteMaskWT)
{
	YMF278BChip *chip = (YMF278BChip *)_info;
	UINT8 CurChn;
	
	ymf262_set_mutemask(chip->fmchip, MuteMaskFM);
	for (CurChn = 0; CurChn < 24; CurChn ++)
		chip->slots[CurChn].Muted = (MuteMaskWT >> CurChn) & 0x01;
	
	return;
}
