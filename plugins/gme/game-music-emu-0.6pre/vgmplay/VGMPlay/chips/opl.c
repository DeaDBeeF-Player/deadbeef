// IMPORTANT: This file is not meant to be compiled. It's included in adlibemu_opl?.c.

/*
 *  Copyright (C) 2002-2010  The DOSBox Team
 *  OPL2/OPL3 emulation library
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


/*
 * Originally based on ADLIBEMU.C, an AdLib/OPL2 emulation library by Ken Silverman
 * Copyright (C) 1998-2001 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 */


#include <math.h>
#include <stdlib.h> // rand()
#include <memory.h>	// for memset()
//#include "dosbox.h"
#include "../stdbool.h"
#include "opl.h"


//static fltype recipsamp;	// inverse of sampling rate		// moved to OPL_DATA
static Bit16s wavtable[WAVEPREC*3];	// wave form table

// vibrato/tremolo tables
static Bit32s vib_table[VIBTAB_SIZE];
static Bit32s trem_table[TREMTAB_SIZE*2];

static Bit32s vibval_const[BLOCKBUF_SIZE];
static Bit32s tremval_const[BLOCKBUF_SIZE];

// vibrato value tables (used per-operator)
static Bit32s vibval_var1[BLOCKBUF_SIZE];
static Bit32s vibval_var2[BLOCKBUF_SIZE];
//static Bit32s vibval_var3[BLOCKBUF_SIZE];
//static Bit32s vibval_var4[BLOCKBUF_SIZE];

// vibrato/trmolo value table pointers
//static Bit32s *vibval1, *vibval2, *vibval3, *vibval4;
//static Bit32s *tremval1, *tremval2, *tremval3, *tremval4;
// moved to adlib_getsample


// key scale level lookup table
static const fltype kslmul[4] = {
	0.0, 0.5, 0.25, 1.0		// -> 0, 3, 1.5, 6 dB/oct
};

// frequency multiplicator lookup table
static const fltype frqmul_tab[16] = {
	0.5,1,2,3,4,5,6,7,8,9,10,10,12,12,15,15
};
// calculated frequency multiplication values (depend on sampling rate)
//static fltype frqmul[16];	// moved to OPL_DATA

// key scale levels
static Bit8u kslev[8][16];

// map a channel number to the register offset of the modulator (=register base)
static const Bit8u modulatorbase[9]	= {
	0,1,2,
	8,9,10,
	16,17,18
};

// map a register base to a modulator operator number or operator number
#if defined(OPLTYPE_IS_OPL3)
static const Bit8u regbase2modop[44] = {
	0,1,2,0,1,2,0,0,3,4,5,3,4,5,0,0,6,7,8,6,7,8,					// first set
	18,19,20,18,19,20,0,0,21,22,23,21,22,23,0,0,24,25,26,24,25,26	// second set
};
static const Bit8u regbase2op[44] = {
	0,1,2,9,10,11,0,0,3,4,5,12,13,14,0,0,6,7,8,15,16,17,			// first set
	18,19,20,27,28,29,0,0,21,22,23,30,31,32,0,0,24,25,26,33,34,35	// second set
};
#else
static const Bit8u regbase2modop[22] = {
	0,1,2,0,1,2,0,0,3,4,5,3,4,5,0,0,6,7,8,6,7,8
};
static const Bit8u regbase2op[22] = {
	0,1,2,9,10,11,0,0,3,4,5,12,13,14,0,0,6,7,8,15,16,17
};
#endif


// start of the waveform
static Bit32u waveform[8] = {
	WAVEPREC,
	WAVEPREC>>1,
	WAVEPREC,
	(WAVEPREC*3)>>2,
	0,
	0,
	(WAVEPREC*5)>>2,
	WAVEPREC<<1
};

// length of the waveform as mask
static Bit32u wavemask[8] = {
	WAVEPREC-1,
	WAVEPREC-1,
	(WAVEPREC>>1)-1,
	(WAVEPREC>>1)-1,
	WAVEPREC-1,
	((WAVEPREC*3)>>2)-1,
	WAVEPREC>>1,
	WAVEPREC-1
};

// where the first entry resides
static Bit32u wavestart[8] = {
	0,
	WAVEPREC>>1,
	0,
	WAVEPREC>>2,
	0,
	0,
	0,
	WAVEPREC>>3
};

// envelope generator function constants
static fltype attackconst[4] = {
	(fltype)(1/2.82624),
	(fltype)(1/2.25280),
	(fltype)(1/1.88416),
	(fltype)(1/1.59744)
};
static fltype decrelconst[4] = {
	(fltype)(1/39.28064),
	(fltype)(1/31.41608),
	(fltype)(1/26.17344),
	(fltype)(1/22.44608)
};


INLINE void operator_advance(OPL_DATA* chip, op_type* op_pt, Bit32s vib) {
	op_pt->wfpos = op_pt->tcount;						// waveform position
	
	// advance waveform time
	op_pt->tcount += op_pt->tinc;
	op_pt->tcount += (Bit32s)(op_pt->tinc)*vib/FIXEDPT;

	op_pt->generator_pos += chip->generator_add;
}

INLINE void operator_advance_drums(OPL_DATA* chip, op_type* op_pt1, Bit32s vib1, op_type* op_pt2, Bit32s vib2, op_type* op_pt3, Bit32s vib3)
{
	Bit32u c1 = op_pt1->tcount/FIXEDPT;
	Bit32u c3 = op_pt3->tcount/FIXEDPT;
	Bit32u phasebit = (((c1 & 0x88) ^ ((c1<<5) & 0x80)) | ((c3 ^ (c3<<2)) & 0x20)) ? 0x02 : 0x00;

	Bit32u noisebit = rand()&1;

	Bit32u snare_phase_bit = (((Bitu)((op_pt1->tcount/FIXEDPT) / 0x100))&1);

	//Hihat
	Bit32u inttm = (phasebit<<8) | (0x34<<(phasebit ^ (noisebit<<1)));
	op_pt1->wfpos = inttm*FIXEDPT;				// waveform position
	// advance waveform time
	op_pt1->tcount += op_pt1->tinc;
	op_pt1->tcount += (Bit32s)(op_pt1->tinc)*vib1/FIXEDPT;
	op_pt1->generator_pos += chip->generator_add;

	//Snare
	inttm = ((1+snare_phase_bit) ^ noisebit)<<8;
	op_pt2->wfpos = inttm*FIXEDPT;				// waveform position
	// advance waveform time
	op_pt2->tcount += op_pt2->tinc;
	op_pt2->tcount += (Bit32s)(op_pt2->tinc)*vib2/FIXEDPT;
	op_pt2->generator_pos += chip->generator_add;

	//Cymbal
	inttm = (1+phasebit)<<8;
	op_pt3->wfpos = inttm*FIXEDPT;				// waveform position
	// advance waveform time
	op_pt3->tcount += op_pt3->tinc;
	op_pt3->tcount += (Bit32s)(op_pt3->tinc)*vib3/FIXEDPT;
	op_pt3->generator_pos += chip->generator_add;
}


// output level is sustained, mode changes only when operator is turned off (->release)
// or when the keep-sustained bit is turned off (->sustain_nokeep)
INLINE void operator_output(op_type* op_pt, Bit32s modulator, Bit32s trem)
{
	if (op_pt->op_state != OF_TYPE_OFF)
	{
		Bit32u i;
		op_pt->lastcval = op_pt->cval;
		i = (Bit32u)((op_pt->wfpos+modulator)/FIXEDPT);

		// wform: -16384 to 16383 (0x4000)
		// trem :  32768 to 65535 (0x10000)
		// step_amp: 0.0 to 1.0
		// vol  : 1/2^14 to 1/2^29 (/0x4000; /1../0x8000)

		op_pt->cval = (Bit32s)(op_pt->step_amp*op_pt->vol*op_pt->cur_wform[i&op_pt->cur_wmask]*trem/16.0);
	}
}


// no action, operator is off
static void operator_off(op_type* op_pt) {
}

// output level is sustained, mode changes only when operator is turned off (->release)
// or when the keep-sustained bit is turned off (->sustain_nokeep)
static void operator_sustain(op_type* op_pt)
{
	Bit32u num_steps_add = op_pt->generator_pos/FIXEDPT;	// number of (standardized) samples
	Bit32u ct;
	for (ct=0; ct<num_steps_add; ct++)
	{
		op_pt->cur_env_step++;
	}
	op_pt->generator_pos -= num_steps_add*FIXEDPT;
}

// operator in release mode, if output level reaches zero the operator is turned off
static void operator_release(op_type* op_pt)
{
	Bit32u num_steps_add;
	Bit32u ct;
	
	// ??? boundary?
	if (op_pt->amp > 0.00000001)
	{
		// release phase
		op_pt->amp *= op_pt->releasemul;
	}

	num_steps_add = op_pt->generator_pos/FIXEDPT;	// number of (standardized) samples
	for (ct=0; ct<num_steps_add; ct++)
	{
		op_pt->cur_env_step++;						// sample counter
		if ((op_pt->cur_env_step & op_pt->env_step_r)==0)
		{
			if (op_pt->amp <= 0.00000001)
			{
				// release phase finished, turn off this operator
				op_pt->amp = 0.0;
				if (op_pt->op_state == OF_TYPE_REL)
				{
					op_pt->op_state = OF_TYPE_OFF;
				}
			}
			op_pt->step_amp = op_pt->amp;
		}
	}
	op_pt->generator_pos -= num_steps_add*FIXEDPT;
}

// operator in decay mode, if sustain level is reached the output level is either
// kept (sustain level keep enabled) or the operator is switched into release mode
static void operator_decay(op_type* op_pt)
{
	Bit32u num_steps_add;
	Bit32u ct;
	
	if (op_pt->amp > op_pt->sustain_level)
	{
		// decay phase
		op_pt->amp *= op_pt->decaymul;
	}

	num_steps_add = op_pt->generator_pos/FIXEDPT;	// number of (standardized) samples
	for (ct=0; ct<num_steps_add; ct++)
	{
		op_pt->cur_env_step++;
		if ((op_pt->cur_env_step & op_pt->env_step_d)==0)
		{
			if (op_pt->amp <= op_pt->sustain_level)
			{
				// decay phase finished, sustain level reached
				if (op_pt->sus_keep)
				{
					// keep sustain level (until turned off)
					op_pt->op_state = OF_TYPE_SUS;
					op_pt->amp = op_pt->sustain_level;
				}
				else
				{
					// next: release phase
					op_pt->op_state = OF_TYPE_SUS_NOKEEP;
				}
			}
			op_pt->step_amp = op_pt->amp;
		}
	}
	op_pt->generator_pos -= num_steps_add*FIXEDPT;
}

// operator in attack mode, if full output level is reached,
// the operator is switched into decay mode
static void operator_attack(op_type* op_pt)
{
	Bit32u num_steps_add;
	Bit32u ct;
	
	op_pt->amp = ((op_pt->a3*op_pt->amp + op_pt->a2)*op_pt->amp + op_pt->a1)*op_pt->amp + op_pt->a0;

	num_steps_add = op_pt->generator_pos/FIXEDPT;		// number of (standardized) samples
	for (ct=0; ct<num_steps_add; ct++)
	{
		op_pt->cur_env_step++;	// next sample
		if ((op_pt->cur_env_step & op_pt->env_step_a)==0)
		{		// check if next step already reached
			if (op_pt->amp > 1.0)
			{
				// attack phase finished, next: decay
				op_pt->op_state = OF_TYPE_DEC;
				op_pt->amp = 1.0;
				op_pt->step_amp = 1.0;
			}
			op_pt->step_skip_pos_a <<= 1;
			if (op_pt->step_skip_pos_a==0) op_pt->step_skip_pos_a = 1;
			if (op_pt->step_skip_pos_a & op_pt->env_step_skip_a)
			{	// check if required to skip next step
				op_pt->step_amp = op_pt->amp;
			}
		}
	}
	op_pt->generator_pos -= num_steps_add*FIXEDPT;
}

static void operator_eg_attack_check(op_type* op_pt)
{
	if (((op_pt->cur_env_step + 1) & op_pt->env_step_a)==0)
	{
		// check if next step already reached
		if (op_pt->a0 >= 1.0)
		{
			// attack phase finished, next: decay
			op_pt->op_state = OF_TYPE_DEC;
			op_pt->amp = 1.0;
			op_pt->step_amp = 1.0;
		}
	}
}


typedef void (*optype_fptr)(op_type*);

static optype_fptr opfuncs[6] = {
	operator_attack,
	operator_decay,
	operator_release,
	operator_sustain,	// sustain phase (keeping level)
	operator_release,	// sustain_nokeep phase (release-style)
	operator_off
};

static void change_attackrate(OPL_DATA* chip, Bitu regbase, op_type* op_pt)
{
	Bits attackrate = chip->adlibreg[ARC_ATTR_DECR+regbase]>>4;
	if (attackrate)
	{
		static Bit8u step_skip_mask[5] = {0xff, 0xfe, 0xee, 0xba, 0xaa}; 
		Bits step_skip;
		Bits steps;
		Bits step_num;
		
		fltype f = (fltype)(pow(FL2,(fltype)attackrate+(op_pt->toff>>2)-1)*attackconst[op_pt->toff&3]*chip->recipsamp);
		// attack rate coefficients
		op_pt->a0 = (fltype)(0.0377*f);
		op_pt->a1 = (fltype)(10.73*f+1);
		op_pt->a2 = (fltype)(-17.57*f);
		op_pt->a3 = (fltype)(7.42*f);

		step_skip = attackrate*4 + op_pt->toff;
		steps = step_skip >> 2;
		op_pt->env_step_a = (1<<(steps<=12?12-steps:0))-1;

		step_num = (step_skip<=48)?(4-(step_skip&3)):0;
		op_pt->env_step_skip_a = step_skip_mask[step_num];

#if defined(OPLTYPE_IS_OPL3)
		if (step_skip>=60)
#else
		if (step_skip>=62)
#endif
		{
			op_pt->a0 = (fltype)(2.0);	// something that triggers an immediate transition to amp:=1.0
			op_pt->a1 = (fltype)(0.0);
			op_pt->a2 = (fltype)(0.0);
			op_pt->a3 = (fltype)(0.0);
		}
	}
	else
	{
		// attack disabled
		op_pt->a0 = 0.0;
		op_pt->a1 = 1.0;
		op_pt->a2 = 0.0;
		op_pt->a3 = 0.0;
		op_pt->env_step_a = 0;
		op_pt->env_step_skip_a = 0;
	}
}

static void change_decayrate(OPL_DATA* chip, Bitu regbase, op_type* op_pt)
{
	Bits decayrate = chip->adlibreg[ARC_ATTR_DECR+regbase]&15;
	// decaymul should be 1.0 when decayrate==0
	if (decayrate) {
		Bits steps;
		
		fltype f = (fltype)(-7.4493*decrelconst[op_pt->toff&3]*chip->recipsamp);
		op_pt->decaymul = (fltype)(pow(FL2,f*pow(FL2,(fltype)(decayrate+(op_pt->toff>>2)))));
		steps = (decayrate*4 + op_pt->toff) >> 2;
		op_pt->env_step_d = (1<<(steps<=12?12-steps:0))-1;
	}
	else
	{
		op_pt->decaymul = 1.0;
		op_pt->env_step_d = 0;
	}
}

static void change_releaserate(OPL_DATA* chip, Bitu regbase, op_type* op_pt)
{
	Bits releaserate = chip->adlibreg[ARC_SUSL_RELR+regbase]&15;
	// releasemul should be 1.0 when releaserate==0
	if (releaserate)
	{
		Bits steps;
		
		fltype f = (fltype)(-7.4493*decrelconst[op_pt->toff&3]*chip->recipsamp);
		op_pt->releasemul = (fltype)(pow(FL2,f*pow(FL2,(fltype)(releaserate+(op_pt->toff>>2)))));
		steps = (releaserate*4 + op_pt->toff) >> 2;
		op_pt->env_step_r = (1<<(steps<=12?12-steps:0))-1;
	}
	else
	{
		op_pt->releasemul = 1.0;
		op_pt->env_step_r = 0;
	}
}

static void change_sustainlevel(OPL_DATA* chip, Bitu regbase, op_type* op_pt)
{
	Bits sustainlevel = chip->adlibreg[ARC_SUSL_RELR+regbase]>>4;
	// sustainlevel should be 0.0 when sustainlevel==15 (max)
	if (sustainlevel<15)
	{
		op_pt->sustain_level = (fltype)(pow(FL2,(fltype)sustainlevel * (-FL05)));
	}
	else
	{
		op_pt->sustain_level = 0.0;
	}
}

static void change_waveform(OPL_DATA* chip, Bitu regbase, op_type* op_pt)
{
#if defined(OPLTYPE_IS_OPL3)
	if (regbase>=ARC_SECONDSET) regbase -= (ARC_SECONDSET-22);	// second set starts at 22
#endif
	// waveform selection
	op_pt->cur_wmask = wavemask[chip->wave_sel[regbase]];
	op_pt->cur_wform = &wavtable[waveform[chip->wave_sel[regbase]]];
	// (might need to be adapted to waveform type here...)
}

static void change_keepsustain(OPL_DATA* chip, Bitu regbase, op_type* op_pt)
{
	op_pt->sus_keep = (chip->adlibreg[ARC_TVS_KSR_MUL+regbase]&0x20)>0;
	if (op_pt->op_state==OF_TYPE_SUS)
	{
		if (!op_pt->sus_keep)
			op_pt->op_state = OF_TYPE_SUS_NOKEEP;
	}
	else if (op_pt->op_state==OF_TYPE_SUS_NOKEEP)
	{
		if (op_pt->sus_keep)
			op_pt->op_state = OF_TYPE_SUS;
	}
}

// enable/disable vibrato/tremolo LFO effects
static void change_vibrato(OPL_DATA* chip, Bitu regbase, op_type* op_pt)
{
	op_pt->vibrato = (chip->adlibreg[ARC_TVS_KSR_MUL+regbase]&0x40)!=0;
	op_pt->tremolo = (chip->adlibreg[ARC_TVS_KSR_MUL+regbase]&0x80)!=0;
}

// change amount of self-feedback
static void change_feedback(OPL_DATA* chip, Bitu chanbase, op_type* op_pt)
{
	Bits feedback = chip->adlibreg[ARC_FEEDBACK+chanbase]&14;
	if (feedback)
		op_pt->mfbi = (Bit32s)(pow(FL2,(fltype)((feedback>>1)+8)));
	else
		op_pt->mfbi = 0;
}

static void change_frequency(OPL_DATA* chip, Bitu chanbase, Bitu regbase, op_type* op_pt)
{
	Bit32u frn;
	Bit32u oct;
	Bit32u note_sel;
	fltype vol_in;
	
	// frequency
	frn = ((((Bit32u)chip->adlibreg[ARC_KON_BNUM+chanbase])&3)<<8) + (Bit32u)chip->adlibreg[ARC_FREQ_NUM+chanbase];
	// block number/octave
	oct = ((((Bit32u)chip->adlibreg[ARC_KON_BNUM+chanbase])>>2)&7);
	op_pt->freq_high = (Bit32s)((frn>>7)&7);

	// keysplit
	note_sel = (chip->adlibreg[8]>>6)&1;
	op_pt->toff = ((frn>>9)&(note_sel^1)) | ((frn>>8)&note_sel);
	op_pt->toff += (oct<<1);

	// envelope scaling (KSR)
	if (!(chip->adlibreg[ARC_TVS_KSR_MUL+regbase]&0x10)) op_pt->toff >>= 2;

	// 20+a0+b0:
	op_pt->tinc = (Bit32u)((((fltype)(frn<<oct))*chip->frqmul[chip->adlibreg[ARC_TVS_KSR_MUL+regbase]&15]));
	// 40+a0+b0:
	vol_in = (fltype)((fltype)(chip->adlibreg[ARC_KSL_OUTLEV+regbase]&63) +
							kslmul[chip->adlibreg[ARC_KSL_OUTLEV+regbase]>>6]*kslev[oct][frn>>6]);
	op_pt->vol = (fltype)(pow(FL2,(fltype)(vol_in * -0.125 - 14)));

	// operator frequency changed, care about features that depend on it
	change_attackrate(chip, regbase,op_pt);
	change_decayrate(chip, regbase,op_pt);
	change_releaserate(chip, regbase,op_pt);
}

static void enable_operator(OPL_DATA* chip, Bitu regbase, op_type* op_pt, Bit32u act_type)
{
	// check if this is really an off-on transition
	if (op_pt->act_state == OP_ACT_OFF)
	{
		Bits wselbase = regbase;
		if (wselbase>=ARC_SECONDSET)
			wselbase -= (ARC_SECONDSET-22);	// second set starts at 22

		op_pt->tcount = wavestart[chip->wave_sel[wselbase]]*FIXEDPT;

		// start with attack mode
		op_pt->op_state = OF_TYPE_ATT;
		op_pt->act_state |= act_type;
	}
}

static void disable_operator(op_type* op_pt, Bit32u act_type)
{
	// check if this is really an on-off transition
	if (op_pt->act_state != OP_ACT_OFF)
	{
		op_pt->act_state &= (~act_type);
		if (op_pt->act_state == OP_ACT_OFF)
		{
			if (op_pt->op_state != OF_TYPE_OFF)
				op_pt->op_state = OF_TYPE_REL;
		}
	}
}

//void adlib_init(Bit32u samplerate)
void* ADLIBEMU(init)(UINT32 clock, UINT32 samplerate,
					 ADL_UPDATEHANDLER UpdateHandler, void* param)
{
	OPL_DATA* OPL;
	//op_type* op;

	Bits i, j, oct;
	//Bit32s trem_table_int[TREMTAB_SIZE];
	static Bitu initfirstime = 0;

	OPL = (OPL_DATA*)malloc(sizeof(OPL_DATA));
	OPL->chip_clock = clock;
	OPL->int_samplerate = samplerate;
	OPL->UpdateHandler = UpdateHandler;
	OPL->UpdateParam = param;

	OPL->generator_add = (Bit32u)(INTFREQU*FIXEDPT/OPL->int_samplerate);


	/*memset(OPL->adlibreg,0,sizeof(OPL->adlibreg));
	memset(OPL->op,0,sizeof(op_type)*MAXOPERATORS);
	memset(OPL->wave_sel,0,sizeof(OPL->wave_sel));

	for (i=0;i<MAXOPERATORS;i++)
	{
		op = &OPL->op[i];
		
		op->op_state = OF_TYPE_OFF;
		op->act_state = OP_ACT_OFF;
		op->amp = 0.0;
		op->step_amp = 0.0;
		op->vol = 0.0;
		op->tcount = 0;
		op->tinc = 0;
		op->toff = 0;
		op->cur_wmask = wavemask[0];
		op->cur_wform = &wavtable[waveform[0]];
		op->freq_high = 0;

		op->generator_pos = 0;
		op->cur_env_step = 0;
		op->env_step_a = 0;
		op->env_step_d = 0;
		op->env_step_r = 0;
		op->step_skip_pos_a = 0;
		op->env_step_skip_a = 0;

#if defined(OPLTYPE_IS_OPL3)
		op->is_4op = false;
		op->is_4op_attached = false;
		op->left_pan = 1;
		op->right_pan = 1;
#endif
	}*/

	OPL->recipsamp = 1.0 / (fltype)OPL->int_samplerate;
	for (i=15;i>=0;i--)
	{
		OPL->frqmul[i] = (fltype)(frqmul_tab[i]*INTFREQU/(fltype)WAVEPREC*(fltype)FIXEDPT*OPL->recipsamp);
	}

	//OPL->status = 0;
	//OPL->opl_index = 0;


	if (!initfirstime)
	{
		// create vibrato table
		vib_table[0] = 8;
		vib_table[1] = 4;
		vib_table[2] = 0;
		vib_table[3] = -4;
		for (i=4; i<VIBTAB_SIZE; i++) vib_table[i] = vib_table[i-4]*-1;
	}

	// vibrato at ~6.1 ?? (opl3 docs say 6.1, opl4 docs say 6.0, y8950 docs say 6.4)
	OPL->vibtab_add = (Bit32u)(VIBTAB_SIZE*FIXEDPT_LFO/8192*INTFREQU/OPL->int_samplerate);
	OPL->vibtab_pos = 0;

	if (!initfirstime)
	{
		Bit32s trem_table_int[TREMTAB_SIZE];
		
		for (i=0; i<BLOCKBUF_SIZE; i++) vibval_const[i] = 0;


		// create tremolo table
		for (i=0; i<14; i++)	trem_table_int[i] = i-13;		// upwards (13 to 26 -> -0.5/6 to 0)
		for (i=14; i<41; i++)	trem_table_int[i] = -i+14;		// downwards (26 to 0 -> 0 to -1/6)
		for (i=41; i<53; i++)	trem_table_int[i] = i-40-26;	// upwards (1 to 12 -> -1/6 to -0.5/6)

		for (i=0; i<TREMTAB_SIZE; i++)
		{
			// 0.0 .. -26/26*4.8/6 == [0.0 .. -0.8], 4/53 steps == [1 .. 0.57]
			fltype trem_val1=(fltype)(((fltype)trem_table_int[i])*4.8/26.0/6.0);				// 4.8db
			fltype trem_val2=(fltype)((fltype)((Bit32s)(trem_table_int[i]/4))*1.2/6.0/6.0);		// 1.2db (larger stepping)

			trem_table[i] = (Bit32s)(pow(FL2,trem_val1)*FIXEDPT);
			trem_table[TREMTAB_SIZE+i] = (Bit32s)(pow(FL2,trem_val2)*FIXEDPT);
		}
	}

	// tremolo at 3.7hz
	OPL->tremtab_add = (Bit32u)((fltype)TREMTAB_SIZE * TREM_FREQ * FIXEDPT_LFO / (fltype)OPL->int_samplerate);
	OPL->tremtab_pos = 0;

	if (!initfirstime)
	{
		initfirstime = 1;

		for (i=0; i<BLOCKBUF_SIZE; i++) tremval_const[i] = FIXEDPT;


		// create waveform tables
		for (i=0;i<(WAVEPREC>>1);i++)
		{
			wavtable[(i<<1)  +WAVEPREC]	= (Bit16s)(16384*sin((fltype)((i<<1)  )*PI*2/WAVEPREC));
			wavtable[(i<<1)+1+WAVEPREC]	= (Bit16s)(16384*sin((fltype)((i<<1)+1)*PI*2/WAVEPREC));
			wavtable[i]					= wavtable[(i<<1)  +WAVEPREC];
			// alternative: (zero-less)
/*			wavtable[(i<<1)  +WAVEPREC]	= (Bit16s)(16384*sin((fltype)((i<<2)+1)*PI/WAVEPREC));
			wavtable[(i<<1)+1+WAVEPREC]	= (Bit16s)(16384*sin((fltype)((i<<2)+3)*PI/WAVEPREC));
			wavtable[i]					= wavtable[(i<<1)-1+WAVEPREC]; */
		}
		for (i=0;i<(WAVEPREC>>3);i++)
		{
			wavtable[i+(WAVEPREC<<1)]		= wavtable[i+(WAVEPREC>>3)]-16384;
			wavtable[i+((WAVEPREC*17)>>3)]	= wavtable[i+(WAVEPREC>>2)]+16384;
		}

		// key scale level table verified ([table in book]*8/3)
		kslev[7][0] = 0;	kslev[7][1] = 24;	kslev[7][2] = 32;	kslev[7][3] = 37;
		kslev[7][4] = 40;	kslev[7][5] = 43;	kslev[7][6] = 45;	kslev[7][7] = 47;
		kslev[7][8] = 48;
		for (i=9;i<16;i++) kslev[7][i] = (Bit8u)(i+41);
		for (j=6;j>=0;j--)
		{
			for (i=0;i<16;i++)
			{
				oct = (Bits)kslev[j+1][i]-8;
				if (oct < 0) oct = 0;
				kslev[j][i] = (Bit8u)oct;
			}
		}
	}
	
	return OPL;
}

void ADLIBEMU(stop)(void *chip)
{
	free(chip);
	
	return;
}

void ADLIBEMU(reset)(void *chip)
{
	OPL_DATA* OPL = (OPL_DATA*)chip;
	Bits i;
	op_type* op;
	
	memset(OPL->adlibreg, 0x00, sizeof(OPL->adlibreg));
	memset(OPL->op, 0x00, sizeof(op_type) * MAXOPERATORS);
	memset(OPL->wave_sel, 0x00, sizeof(OPL->wave_sel));
	
	for (i=0;i<MAXOPERATORS;i++)
	{
		op = &OPL->op[i];
		
		op->op_state = OF_TYPE_OFF;
		op->act_state = OP_ACT_OFF;
		op->amp = 0.0;
		op->step_amp = 0.0;
		op->vol = 0.0;
		op->tcount = 0;
		op->tinc = 0;
		op->toff = 0;
		op->cur_wmask = wavemask[0];
		op->cur_wform = &wavtable[waveform[0]];
		op->freq_high = 0;

		op->generator_pos = 0;
		op->cur_env_step = 0;
		op->env_step_a = 0;
		op->env_step_d = 0;
		op->env_step_r = 0;
		op->step_skip_pos_a = 0;
		op->env_step_skip_a = 0;

#if defined(OPLTYPE_IS_OPL3)
		op->is_4op = false;
		op->is_4op_attached = false;
		op->left_pan = 1;
		op->right_pan = 1;
#endif
	}
	
	OPL->status = 0;
	OPL->opl_index = 0;
	OPL->opl_addr = 0;
	
	return;
}



void ADLIBEMU(writeIO)(void *chip, UINT32 addr, UINT8 val)
{
	OPL_DATA* OPL = (OPL_DATA*)chip;
	
	if (addr & 1)
		adlib_write(OPL, OPL->opl_addr, val);
	else
#if defined(OPLTYPE_IS_OPL3)
		OPL->opl_addr = val | ((addr & 2) << 7);
#else
		OPL->opl_addr = val;
#endif
}

static void adlib_write(void *chip, Bitu idx, Bit8u val)
{
	OPL_DATA* OPL = (OPL_DATA*)chip;
	
	Bit32u second_set = idx&0x100;
	OPL->adlibreg[idx] = val;

	switch (idx&0xf0)
	{
	case ARC_CONTROL:
		// here we check for the second set registers, too:
		switch (idx)
		{
		case 0x02:	// timer1 counter
		case 0x03:	// timer2 counter
			break;
		case 0x04:
			// IRQ reset, timer mask/start
			if (val&0x80)
			{
				// clear IRQ bits in status register
				OPL->status &= ~0x60;
			}
			else
			{
				OPL->status = 0;
			}
			break;
#if defined(OPLTYPE_IS_OPL3)
		case 0x04|ARC_SECONDSET:
			// 4op enable/disable switches for each possible channel
			OPL->op[0].is_4op = (val&1)>0;
			OPL->op[3].is_4op_attached = OPL->op[0].is_4op;
			OPL->op[1].is_4op = (val&2)>0;
			OPL->op[4].is_4op_attached = OPL->op[1].is_4op;
			OPL->op[2].is_4op = (val&4)>0;
			OPL->op[5].is_4op_attached = OPL->op[2].is_4op;
			OPL->op[18].is_4op = (val&8)>0;
			OPL->op[21].is_4op_attached = OPL->op[18].is_4op;
			OPL->op[19].is_4op = (val&16)>0;
			OPL->op[22].is_4op_attached = OPL->op[19].is_4op;
			OPL->op[20].is_4op = (val&32)>0;
			OPL->op[23].is_4op_attached = OPL->op[20].is_4op;
			break;
		case 0x05|ARC_SECONDSET:
			break;
#endif
		case 0x08:
			// CSW, note select
			break;
		default:
			break;
		}
		break;
	case ARC_TVS_KSR_MUL:
	case ARC_TVS_KSR_MUL+0x10:
		{
		// tremolo/vibrato/sustain keeping enabled; key scale rate; frequency multiplication
		int num = idx&7;
		Bitu base = (idx-ARC_TVS_KSR_MUL)&0xff;
		if ((num<6) && (base<22)) {
			Bitu modop = regbase2modop[second_set?(base+22):base];
			Bitu regbase = base+second_set;
			Bitu chanbase = second_set?(modop-18+ARC_SECONDSET):modop;

			// change tremolo/vibrato and sustain keeping of this operator
			op_type* op_ptr = &OPL->op[modop+((num<3) ? 0 : 9)];
			change_keepsustain(chip, regbase,op_ptr);
			change_vibrato(chip, regbase,op_ptr);

			// change frequency calculations of this operator as
			// key scale rate and frequency multiplicator can be changed
#if defined(OPLTYPE_IS_OPL3)
			if ((OPL->adlibreg[0x105]&1) && (OPL->op[modop].is_4op_attached))
			{
				// operator uses frequency of channel
				change_frequency(chip, chanbase-3,regbase,op_ptr);
			}
			else
			{
				change_frequency(chip, chanbase,regbase,op_ptr);
			}
#else
			change_frequency(chip, chanbase,base,op_ptr);
#endif
		}
		}
		break;
	case ARC_KSL_OUTLEV:
	case ARC_KSL_OUTLEV+0x10:
		{
		// key scale level; output rate
		int num = idx&7;
		Bitu base = (idx-ARC_KSL_OUTLEV)&0xff;
		if ((num<6) && (base<22))
		{
			Bitu modop = regbase2modop[second_set?(base+22):base];
			Bitu chanbase = second_set?(modop-18+ARC_SECONDSET):modop;

			// change frequency calculations of this operator as
			// key scale level and output rate can be changed
			op_type* op_ptr = &OPL->op[modop+((num<3) ? 0 : 9)];
#if defined(OPLTYPE_IS_OPL3)
			Bitu regbase = base+second_set;
			if ((OPL->adlibreg[0x105]&1) && (OPL->op[modop].is_4op_attached))
			{
				// operator uses frequency of channel
				change_frequency(chip, chanbase-3,regbase,op_ptr);
			}
			else
			{
				change_frequency(chip, chanbase,regbase,op_ptr);
			}
#else
			change_frequency(chip, chanbase,base,op_ptr);
#endif
		}
		}
		break;
	case ARC_ATTR_DECR:
	case ARC_ATTR_DECR+0x10:
		{
		// attack/decay rates
		int num = idx&7;
		Bitu base = (idx-ARC_ATTR_DECR)&0xff;
		if ((num<6) && (base<22))
		{
			Bitu regbase = base+second_set;

			// change attack rate and decay rate of this operator
			op_type* op_ptr = &OPL->op[regbase2op[second_set?(base+22):base]];
			change_attackrate(chip, regbase,op_ptr);
			change_decayrate(chip, regbase,op_ptr);
		}
		}
		break;
	case ARC_SUSL_RELR:
	case ARC_SUSL_RELR+0x10:
		{
		// sustain level; release rate
		int num = idx&7;
		Bitu base = (idx-ARC_SUSL_RELR)&0xff;
		if ((num<6) && (base<22))
		{
			Bitu regbase = base+second_set;

			// change sustain level and release rate of this operator
			op_type* op_ptr = &OPL->op[regbase2op[second_set?(base+22):base]];
			change_releaserate(chip, regbase,op_ptr);
			change_sustainlevel(chip, regbase,op_ptr);
		}
		}
		break;
	case ARC_FREQ_NUM:
		{
		// 0xa0-0xa8 low8 frequency
		Bitu base = (idx-ARC_FREQ_NUM)&0xff;
		if (base<9)
		{
			Bits opbase = second_set?(base+18):base;
			Bits modbase;
			Bitu chanbase;
#if defined(OPLTYPE_IS_OPL3)
		if ((OPL->adlibreg[0x105]&1) && OPL->op[opbase].is_4op_attached) break;
#endif
			// regbase of modulator:
			modbase = modulatorbase[base]+second_set;

			chanbase = base+second_set;

			change_frequency(chip, chanbase,modbase,&OPL->op[opbase]);
			change_frequency(chip, chanbase,modbase+3,&OPL->op[opbase+9]);
#if defined(OPLTYPE_IS_OPL3)
			// for 4op channels all four operators are modified to the frequency of the channel
			if ((OPL->adlibreg[0x105]&1) && OPL->op[second_set?(base+18):base].is_4op)
			{
				change_frequency(chip, chanbase,modbase+8,&OPL->op[opbase+3]);
				change_frequency(chip, chanbase,modbase+3+8,&OPL->op[opbase+3+9]);
			}
#endif
		}
		}
		break;
	case ARC_KON_BNUM:
		{
		Bitu base;
		if (OPL->UpdateHandler != NULL)	// hack for DOSBox logs
			OPL->UpdateHandler(OPL->UpdateParam);
		if (idx == ARC_PERC_MODE)
		{
#if defined(OPLTYPE_IS_OPL3)
			if (second_set) return;
#endif

			if ((val&0x30) == 0x30)
			{		// BassDrum active
				enable_operator(chip, 16,&OPL->op[6],OP_ACT_PERC);
				change_frequency(chip, 6,16,&OPL->op[6]);
				enable_operator(chip, 16+3,&OPL->op[6+9],OP_ACT_PERC);
				change_frequency(chip, 6,16+3,&OPL->op[6+9]);
			}
			else
			{
				disable_operator(&OPL->op[6],OP_ACT_PERC);
				disable_operator(&OPL->op[6+9],OP_ACT_PERC);
			}
			if ((val&0x28) == 0x28)
			{		// Snare active
				enable_operator(chip, 17+3,&OPL->op[16],OP_ACT_PERC);
				change_frequency(chip, 7,17+3,&OPL->op[16]);
			}
			else
			{
				disable_operator(&OPL->op[16],OP_ACT_PERC);
			}
			if ((val&0x24) == 0x24)
			{		// TomTom active
				enable_operator(chip, 18,&OPL->op[8],OP_ACT_PERC);
				change_frequency(chip, 8,18,&OPL->op[8]);
			}
			else
			{
				disable_operator(&OPL->op[8],OP_ACT_PERC);
			}
			if ((val&0x22) == 0x22)
			{		// Cymbal active
				enable_operator(chip, 18+3,&OPL->op[8+9],OP_ACT_PERC);
				change_frequency(chip, 8,18+3,&OPL->op[8+9]);
			}
			else
			{
				disable_operator(&OPL->op[8+9],OP_ACT_PERC);
			}
			if ((val&0x21) == 0x21)
			{		// Hihat active
				enable_operator(chip, 17,&OPL->op[7],OP_ACT_PERC);
				change_frequency(chip, 7,17,&OPL->op[7]);
			}
			else
			{
				disable_operator(&OPL->op[7],OP_ACT_PERC);
			}

			break;
		}
		// regular 0xb0-0xb8
		base = (idx-ARC_KON_BNUM)&0xff;
		if (base<9)
		{
			Bits opbase = second_set?(base+18):base;
			// regbase of modulator:
			Bits modbase = modulatorbase[base]+second_set;
			Bitu chanbase;

#if defined(OPLTYPE_IS_OPL3)
			if ((OPL->adlibreg[0x105]&1) && OPL->op[opbase].is_4op_attached) break;
#endif
			if (val&32)
			{
				// operator switched on
				enable_operator(chip, modbase,&OPL->op[opbase],OP_ACT_NORMAL);		// modulator (if 2op)
				enable_operator(chip, modbase+3,&OPL->op[opbase+9],OP_ACT_NORMAL);	// carrier (if 2op)
#if defined(OPLTYPE_IS_OPL3)
				// for 4op channels all four operators are switched on
				if ((OPL->adlibreg[0x105]&1) && OPL->op[opbase].is_4op)
				{
					// turn on chan+3 operators as well
					enable_operator(chip, modbase+8,&OPL->op[opbase+3],OP_ACT_NORMAL);
					enable_operator(chip, modbase+3+8,&OPL->op[opbase+3+9],OP_ACT_NORMAL);
				}
#endif
			}
			else
			{
				// operator switched off
				disable_operator(&OPL->op[opbase],OP_ACT_NORMAL);
				disable_operator(&OPL->op[opbase+9],OP_ACT_NORMAL);
#if defined(OPLTYPE_IS_OPL3)
				// for 4op channels all four operators are switched off
				if ((OPL->adlibreg[0x105]&1) && OPL->op[opbase].is_4op)
				{
					// turn off chan+3 operators as well
					disable_operator(&OPL->op[opbase+3],OP_ACT_NORMAL);
					disable_operator(&OPL->op[opbase+3+9],OP_ACT_NORMAL);
				}
#endif
			}

			chanbase = base+second_set;

			// change frequency calculations of modulator and carrier (2op) as
			// the frequency of the channel has changed
			change_frequency(chip, chanbase,modbase,&OPL->op[opbase]);
			change_frequency(chip, chanbase,modbase+3,&OPL->op[opbase+9]);
#if defined(OPLTYPE_IS_OPL3)
			// for 4op channels all four operators are modified to the frequency of the channel
			if ((OPL->adlibreg[0x105]&1) && OPL->op[second_set?(base+18):base].is_4op)
			{
				// change frequency calculations of chan+3 operators as well
				change_frequency(chip, chanbase,modbase+8,&OPL->op[opbase+3]);
				change_frequency(chip, chanbase,modbase+3+8,&OPL->op[opbase+3+9]);
			}
#endif
		}
		}
		break;
	case ARC_FEEDBACK:
		{
		// 0xc0-0xc8 feedback/modulation type (AM/FM)
		Bitu base = (idx-ARC_FEEDBACK)&0xff;
		if (base<9)
		{
			Bits opbase = second_set?(base+18):base;
			Bitu chanbase = base+second_set;
			change_feedback(chip, chanbase,&OPL->op[opbase]);
#if defined(OPLTYPE_IS_OPL3)
			// OPL3 panning
			OPL->op[opbase].left_pan = ((val&0x10)>>4);
			OPL->op[opbase].right_pan = ((val&0x20)>>5);
			OPL->op[opbase].left_pan += ((val&0x40)>>6);
			OPL->op[opbase].right_pan += ((val&0x80)>>7);
#endif
		}
		}
		break;
	case ARC_WAVE_SEL:
	case ARC_WAVE_SEL+0x10:
		{
		int num = idx&7;
		Bitu base = (idx-ARC_WAVE_SEL)&0xff;
		if ((num<6) && (base<22))
		{
#if defined(OPLTYPE_IS_OPL3)
			Bits wselbase = second_set?(base+22):base;	// for easier mapping onto wave_sel[]
			op_type* op_ptr;
			// change waveform
			if (OPL->adlibreg[0x105]&1) OPL->wave_sel[wselbase] = val&7;	// opl3 mode enabled, all waveforms accessible
			else OPL->wave_sel[wselbase] = val&3;
			op_ptr = &OPL->op[regbase2modop[wselbase]+((num<3) ? 0 : 9)];
			change_waveform(chip, wselbase,op_ptr);
#else
			if (OPL->adlibreg[0x01]&0x20)
			{
				op_type* op_ptr;
				
				// wave selection enabled, change waveform
				OPL->wave_sel[base] = val&3;
				op_ptr = &OPL->op[regbase2modop[base]+((num<3) ? 0 : 9)];
				change_waveform(chip, base,op_ptr);
			}
#endif
		}
		}
		break;
	default:
		break;
	}
}


Bitu ADLIBEMU(reg_read)(void *chip, UINT32 port)
{
	OPL_DATA* OPL = (OPL_DATA*)chip;
	
#if defined(OPLTYPE_IS_OPL3)
	// opl3-detection routines require ret&6 to be zero
	if ((port&1)==0)
	{
		return OPL->status;
	}
	return 0x00;
#else
	// opl2-detection routines require ret&6 to be 6
	if ((port&1)==0)
	{
		return OPL->status|6;
	}
	return 0xff;
#endif
}

void ADLIBEMU(write_index)(void *chip, UINT32 port, UINT8 val)
{
	OPL_DATA* OPL = (OPL_DATA*)chip;
	
	OPL->opl_index = val;
#if defined(OPLTYPE_IS_OPL3)
	if ((port&3)!=0)
	{
		// possibly second set
		if (((OPL->adlibreg[0x105]&1)!=0) || (OPL->opl_index==5)) OPL->opl_index |= ARC_SECONDSET;
	}
#endif
}

/*static void OPL_INLINE clipit16(Bit32s ival, Bit16s* outval)
{
	if (ival<32768)
	{
		if (ival>-32769)
		{
			*outval=(Bit16s)ival;
		}
		else
		{
			*outval = -32768;
		}
	}
	else
	{
		*outval = 32767;
	}
}*/



// be careful with this
// uses cptr and chanval, outputs into outbufl(/outbufr)
// for opl3 check if opl3-mode is enabled (which uses stereo panning)
// 
// Changes by Valley Bell:
//	- Changed to always output to both channels
//	- added parameter "chn" to fix panning for 4-op channels and the Rhythm Cymbal
#undef CHANVAL_OUT
#if defined(OPLTYPE_IS_OPL3)
#define CHANVAL_OUT(chn)								\
	if (OPL->adlibreg[0x105]&1) {						\
		outbufl[i] += chanval*cptr[chn].left_pan;		\
		outbufr[i] += chanval*cptr[chn].right_pan;	\
	} else {										\
		outbufl[i] += chanval;						\
		outbufr[i] += chanval;						\
	}
#else
#define CHANVAL_OUT(chn)								\
	outbufl[i] += chanval;							\
	outbufr[i] += chanval;
#endif

//void adlib_getsample(Bit16s* sndptr, Bits numsamples)
void ADLIBEMU(getsample)(void *chip, INT32** sndptr, INT32 numsamples)
{
	OPL_DATA* OPL = (OPL_DATA*)chip;
	
	Bits i, endsamples;
	op_type* cptr;

	//Bit32s outbufl[BLOCKBUF_SIZE];
#if defined(OPLTYPE_IS_OPL3)
	// second output buffer (right channel for opl3 stereo)
	//Bit32s outbufr[BLOCKBUF_SIZE];
#endif
	Bit32s* outbufl = sndptr[0];
	Bit32s* outbufr = sndptr[1];

	// vibrato/tremolo lookup tables (global, to possibly be used by all operators)
	Bit32s vib_lut[BLOCKBUF_SIZE];
	Bit32s trem_lut[BLOCKBUF_SIZE];

	Bits samples_to_process = numsamples;

	Bits cursmp;
	Bit32s vib_tshift;
	Bitu max_channel = NUM_CHANNELS;
	Bits cur_ch;
	
	Bit32s *vibval1, *vibval2, *vibval3, *vibval4;
	Bit32s *tremval1, *tremval2, *tremval3, *tremval4;
	
#if defined(OPLTYPE_IS_OPL3)
	if ((OPL->adlibreg[0x105]&1)==0) max_channel = NUM_CHANNELS/2;
#endif
	
	if (! samples_to_process)
	{
		for (cur_ch = 0; cur_ch < max_channel; cur_ch ++)
		{
			if ((OPL->adlibreg[ARC_PERC_MODE] & 0x20) && (cur_ch >= 6 && cur_ch < 9))
				continue;
			
#if defined(OPLTYPE_IS_OPL3)
			if (cur_ch < 9)
				cptr = &OPL->op[cur_ch];
			else
				cptr = &OPL->op[cur_ch+9];	// second set is operator18-operator35
			if (cptr->is_4op_attached)
				continue;
#else
			cptr = &OPL->op[cur_ch];
#endif
			
			if (cptr[0].op_state == OF_TYPE_ATT)
				operator_eg_attack_check(&cptr[0]);
			if (cptr[9].op_state == OF_TYPE_ATT)
				operator_eg_attack_check(&cptr[9]);
		}
		
		return;
	}
	
	for (cursmp=0; cursmp<samples_to_process; cursmp+=endsamples)
	{
		endsamples = samples_to_process-cursmp;
		//if (endsamples>BLOCKBUF_SIZE) endsamples = BLOCKBUF_SIZE;

		memset(outbufl,0,endsamples*sizeof(Bit32s));
//#if defined(OPLTYPE_IS_OPL3)
//		// clear second output buffer (opl3 stereo)
//		//if (adlibreg[0x105]&1)
			memset(outbufr,0,endsamples*sizeof(Bit32s));
//#endif

		// calculate vibrato/tremolo lookup tables
		vib_tshift = ((OPL->adlibreg[ARC_PERC_MODE]&0x40)==0) ? 1 : 0;	// 14cents/7cents switching
		for (i=0;i<endsamples;i++)
		{
			// cycle through vibrato table
			OPL->vibtab_pos += OPL->vibtab_add;
			if (OPL->vibtab_pos/FIXEDPT_LFO>=VIBTAB_SIZE)
				OPL->vibtab_pos-=VIBTAB_SIZE*FIXEDPT_LFO;
			vib_lut[i] = vib_table[OPL->vibtab_pos/FIXEDPT_LFO]>>vib_tshift;		// 14cents (14/100 of a semitone) or 7cents

			// cycle through tremolo table
			OPL->tremtab_pos += OPL->tremtab_add;
			if (OPL->tremtab_pos/FIXEDPT_LFO>=TREMTAB_SIZE)
				OPL->tremtab_pos-=TREMTAB_SIZE*FIXEDPT_LFO;
			if (OPL->adlibreg[ARC_PERC_MODE]&0x80)
				trem_lut[i] = trem_table[OPL->tremtab_pos/FIXEDPT_LFO];
			else
				trem_lut[i] = trem_table[TREMTAB_SIZE+OPL->tremtab_pos/FIXEDPT_LFO];
		}

		if (OPL->adlibreg[ARC_PERC_MODE]&0x20)
		{
			if (! (OPL->MuteChn[NUM_CHANNELS + 0]))
			{
			//BassDrum
			cptr = &OPL->op[6];
			if (OPL->adlibreg[ARC_FEEDBACK+6]&1)
			{
				// additive synthesis
				if (cptr[9].op_state != OF_TYPE_OFF)
				{
					if (cptr[9].vibrato)
					{
						vibval1 = vibval_var1;
						for (i=0;i<endsamples;i++)
							vibval1[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
					}
					else
						vibval1 = vibval_const;
					if (cptr[9].tremolo)
						tremval1 = trem_lut;	// tremolo enabled, use table
					else
						tremval1 = tremval_const;

					// calculate channel output
					for (i=0;i<endsamples;i++)
					{
						Bit32s chanval;
						
						operator_advance(OPL, &cptr[9],vibval1[i]);
						opfuncs[cptr[9].op_state](&cptr[9]);
						operator_output(&cptr[9],0,tremval1[i]);
						
						chanval = cptr[9].cval*2;
						CHANVAL_OUT(0)
					}
				}
			}
			else
			{
				// frequency modulation
				if ((cptr[9].op_state != OF_TYPE_OFF) || (cptr[0].op_state != OF_TYPE_OFF))
				{
					if ((cptr[0].vibrato) && (cptr[0].op_state != OF_TYPE_OFF))
					{
						vibval1 = vibval_var1;
						for (i=0;i<endsamples;i++)
							vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
					}
					else
						vibval1 = vibval_const;
					if ((cptr[9].vibrato) && (cptr[9].op_state != OF_TYPE_OFF))
					{
						vibval2 = vibval_var2;
						for (i=0;i<endsamples;i++)
							vibval2[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
					}
					else
						vibval2 = vibval_const;
					if (cptr[0].tremolo)
						tremval1 = trem_lut;	// tremolo enabled, use table
					else
						tremval1 = tremval_const;
					if (cptr[9].tremolo)
						tremval2 = trem_lut;	// tremolo enabled, use table
					else
						tremval2 = tremval_const;

					// calculate channel output
					for (i=0;i<endsamples;i++)
					{
						Bit32s chanval;
						
						operator_advance(OPL, &cptr[0],vibval1[i]);
						opfuncs[cptr[0].op_state](&cptr[0]);
						operator_output(&cptr[0],(cptr[0].lastcval+cptr[0].cval)*cptr[0].mfbi/2,tremval1[i]);

						operator_advance(OPL, &cptr[9],vibval2[i]);
						opfuncs[cptr[9].op_state](&cptr[9]);
						operator_output(&cptr[9],cptr[0].cval*FIXEDPT,tremval2[i]);
						
						chanval = cptr[9].cval*2;
						CHANVAL_OUT(0)
					}
				}
			}
			}	// end if (! Muted)

			//TomTom (j=8)
			if (! (OPL->MuteChn[NUM_CHANNELS + 2]) && OPL->op[8].op_state != OF_TYPE_OFF)
			{
				cptr = &OPL->op[8];
				if (cptr[0].vibrato)
				{
					vibval3 = vibval_var1;
					for (i=0;i<endsamples;i++)
						vibval3[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval3 = vibval_const;

				if (cptr[0].tremolo)
					tremval3 = trem_lut;	// tremolo enabled, use table
				else
					tremval3 = tremval_const;

				// calculate channel output
				for (i=0;i<endsamples;i++)
				{
					Bit32s chanval;
					
					operator_advance(OPL, &cptr[0],vibval3[i]);
					opfuncs[cptr[0].op_state](&cptr[0]);		//TomTom
					operator_output(&cptr[0],0,tremval3[i]);
					chanval = cptr[0].cval*2;
					CHANVAL_OUT(0)
				}
			}

			//Snare/Hihat (j=7), Cymbal (j=8)
			if ((OPL->op[7].op_state != OF_TYPE_OFF) || (OPL->op[16].op_state != OF_TYPE_OFF) ||
				(OPL->op[17].op_state != OF_TYPE_OFF))
			{
				cptr = &OPL->op[7];
				if ((cptr[0].vibrato) && (cptr[0].op_state != OF_TYPE_OFF))
				{
					vibval1 = vibval_var1;
					for (i=0;i<endsamples;i++)
						vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval1 = vibval_const;
				if ((cptr[9].vibrato) && (cptr[9].op_state == OF_TYPE_OFF))
				{
					vibval2 = vibval_var2;
					for (i=0;i<endsamples;i++)
						vibval2[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval2 = vibval_const;

				if (cptr[0].tremolo)
					tremval1 = trem_lut;	// tremolo enabled, use table
				else
					tremval1 = tremval_const;
				if (cptr[9].tremolo)
					tremval2 = trem_lut;	// tremolo enabled, use table
				else
					tremval2 = tremval_const;

				cptr = &OPL->op[8];
				if ((cptr[9].vibrato) && (cptr[9].op_state == OF_TYPE_OFF))
				{
					vibval4 = vibval_var2;
					for (i=0;i<endsamples;i++)
						vibval4[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval4 = vibval_const;

				if (cptr[9].tremolo) tremval4 = trem_lut;	// tremolo enabled, use table
				else tremval4 = tremval_const;

				// calculate channel output
				cptr = &OPL->op[0];	// set cptr to something useful (else it stays at op[8])
				for (i=0;i<endsamples;i++)
				{
					Bit32s chanval;
					
					operator_advance_drums(OPL, &OPL->op[7],vibval1[i],&OPL->op[7+9],vibval2[i],&OPL->op[8+9],vibval4[i]);

					if (! (OPL->MuteChn[NUM_CHANNELS + 4]))
					{
						opfuncs[OPL->op[7].op_state](&OPL->op[7]);			//Hihat
						operator_output(&OPL->op[7],0,tremval1[i]);
					}
					else
						OPL->op[7].cval = 0;

					if (! (OPL->MuteChn[NUM_CHANNELS + 1]))
					{
						opfuncs[OPL->op[7+9].op_state](&OPL->op[7+9]);		//Snare
						operator_output(&OPL->op[7+9],0,tremval2[i]);
					}
					else
						OPL->op[7+9].cval = 0;

					if (! (OPL->MuteChn[NUM_CHANNELS + 3]))
					{
						opfuncs[OPL->op[8+9].op_state](&OPL->op[8+9]);		//Cymbal
						operator_output(&OPL->op[8+9],0,tremval4[i]);
					}
					else
						OPL->op[8+9].cval = 0;

					//chanval = (OPL->op[7].cval + OPL->op[7+9].cval + OPL->op[8+9].cval)*2;
					//CHANVAL_OUT(0)
					// fix panning of the snare -Valley Bell
					chanval = (OPL->op[7].cval + OPL->op[7+9].cval)*2;
					CHANVAL_OUT(7)
					chanval = OPL->op[8+9].cval*2;
					CHANVAL_OUT(8)
				}
			}
		}

		for (cur_ch=max_channel-1; cur_ch>=0; cur_ch--)
		{
			Bitu k;

			if (OPL->MuteChn[cur_ch])
				continue;

			// skip drum/percussion operators
			if ((OPL->adlibreg[ARC_PERC_MODE]&0x20) && (cur_ch >= 6) && (cur_ch < 9)) continue;

			k = cur_ch;
#if defined(OPLTYPE_IS_OPL3)
			if (cur_ch < 9)
			{
				cptr = &OPL->op[cur_ch];
			}
			else
			{
				cptr = &OPL->op[cur_ch+9];	// second set is operator18-operator35
				k += (-9+256);		// second set uses registers 0x100 onwards
			}
			// check if this operator is part of a 4-op
			//if ((OPL->adlibreg[0x105]&1) && cptr->is_4op_attached) continue;
			if (cptr->is_4op_attached) continue;	// this is more correct
#else
			cptr = &OPL->op[cur_ch];
#endif

			// check for FM/AM
			if (OPL->adlibreg[ARC_FEEDBACK+k]&1)
			{
#if defined(OPLTYPE_IS_OPL3)
				//if ((OPL->adlibreg[0x105]&1) && cptr->is_4op)
				if (cptr->is_4op)	// this is more correct
				{
					if (OPL->adlibreg[ARC_FEEDBACK+k+3]&1)
					{
						// AM-AM-style synthesis (op1[fb] + (op2 * op3) + op4)
						if (cptr[0].op_state != OF_TYPE_OFF)
						{
							if (cptr[0].vibrato)
							{
								vibval1 = vibval_var1;
								for (i=0;i<endsamples;i++)
									vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval1 = vibval_const;
							if (cptr[0].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[0],vibval1[i]);
								opfuncs[cptr[0].op_state](&cptr[0]);
								operator_output(&cptr[0],(cptr[0].lastcval+cptr[0].cval)*cptr[0].mfbi/2,tremval1[i]);

								chanval = cptr[0].cval;
								CHANVAL_OUT(3)	// Note: Op 1 of 4, so it needs to use the panning bits of Op 4 (Ch+3)
							}
						}

						if ((cptr[3].op_state != OF_TYPE_OFF) || (cptr[9].op_state != OF_TYPE_OFF))
						{
							if ((cptr[9].vibrato) && (cptr[9].op_state != OF_TYPE_OFF))
							{
								vibval1 = vibval_var1;
								for (i=0;i<endsamples;i++)
									vibval1[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval1 = vibval_const;
							if (cptr[9].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;
							if (cptr[3].tremolo)
								tremval2 = trem_lut;	// tremolo enabled, use table
							else
								tremval2 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[9],vibval1[i]);
								opfuncs[cptr[9].op_state](&cptr[9]);
								operator_output(&cptr[9],0,tremval1[i]);

								operator_advance(OPL, &cptr[3],0);
								opfuncs[cptr[3].op_state](&cptr[3]);
								operator_output(&cptr[3],cptr[9].cval*FIXEDPT,tremval2[i]);

								chanval = cptr[3].cval;
								CHANVAL_OUT(3)
							}
						}

						if (cptr[3+9].op_state != OF_TYPE_OFF)
						{
							if (cptr[3+9].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[3+9],0);
								opfuncs[cptr[3+9].op_state](&cptr[3+9]);
								operator_output(&cptr[3+9],0,tremval1[i]);

								chanval = cptr[3+9].cval;
								CHANVAL_OUT(3)
							}
						}
					}
					else
					{
						// AM-FM-style synthesis (op1[fb] + (op2 * op3 * op4))
						if (cptr[0].op_state != OF_TYPE_OFF)
						{
							if (cptr[0].vibrato)
							{
								vibval1 = vibval_var1;
								for (i=0;i<endsamples;i++)
									vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval1 = vibval_const;
							if (cptr[0].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[0],vibval1[i]);
								opfuncs[cptr[0].op_state](&cptr[0]);
								operator_output(&cptr[0],(cptr[0].lastcval+cptr[0].cval)*cptr[0].mfbi/2,tremval1[i]);

								chanval = cptr[0].cval;
								CHANVAL_OUT(3)
							}
						}

						if ((cptr[9].op_state != OF_TYPE_OFF) || (cptr[3].op_state != OF_TYPE_OFF) || (cptr[3+9].op_state != OF_TYPE_OFF))
						{
							if ((cptr[9].vibrato) && (cptr[9].op_state != OF_TYPE_OFF)) {
								vibval1 = vibval_var1;
								for (i=0;i<endsamples;i++)
									vibval1[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval1 = vibval_const;
							if (cptr[9].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;
							if (cptr[3].tremolo)
								tremval2 = trem_lut;	// tremolo enabled, use table
							else
								tremval2 = tremval_const;
							if (cptr[3+9].tremolo)
								tremval3 = trem_lut;	// tremolo enabled, use table
							else
								tremval3 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[9],vibval1[i]);
								opfuncs[cptr[9].op_state](&cptr[9]);
								operator_output(&cptr[9],0,tremval1[i]);

								operator_advance(OPL, &cptr[3],0);
								opfuncs[cptr[3].op_state](&cptr[3]);
								operator_output(&cptr[3],cptr[9].cval*FIXEDPT,tremval2[i]);

								operator_advance(OPL, &cptr[3+9],0);
								opfuncs[cptr[3+9].op_state](&cptr[3+9]);
								operator_output(&cptr[3+9],cptr[3].cval*FIXEDPT,tremval3[i]);

								chanval = cptr[3+9].cval;
								CHANVAL_OUT(3)
							}
						}
					}
					continue;
				}
#endif
				// 2op additive synthesis
				if ((cptr[9].op_state == OF_TYPE_OFF) && (cptr[0].op_state == OF_TYPE_OFF)) continue;
				if ((cptr[0].vibrato) && (cptr[0].op_state != OF_TYPE_OFF))
				{
					vibval1 = vibval_var1;
					for (i=0;i<endsamples;i++)
						vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval1 = vibval_const;
				if ((cptr[9].vibrato) && (cptr[9].op_state != OF_TYPE_OFF))
				{
					vibval2 = vibval_var2;
					for (i=0;i<endsamples;i++)
						vibval2[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval2 = vibval_const;
				if (cptr[0].tremolo)
					tremval1 = trem_lut;	// tremolo enabled, use table
				else
					tremval1 = tremval_const;
				if (cptr[9].tremolo)
					tremval2 = trem_lut;	// tremolo enabled, use table
				else
					tremval2 = tremval_const;

				// calculate channel output
				for (i=0;i<endsamples;i++)
				{
					Bit32s chanval;
					
					// carrier1
					operator_advance(OPL, &cptr[0],vibval1[i]);
					opfuncs[cptr[0].op_state](&cptr[0]);
					operator_output(&cptr[0],(cptr[0].lastcval+cptr[0].cval)*cptr[0].mfbi/2,tremval1[i]);

					// carrier2
					operator_advance(OPL, &cptr[9],vibval2[i]);
					opfuncs[cptr[9].op_state](&cptr[9]);
					operator_output(&cptr[9],0,tremval2[i]);

					chanval = cptr[9].cval + cptr[0].cval;
					CHANVAL_OUT(0)
				}
			}
			else
			{
#if defined(OPLTYPE_IS_OPL3)
				//if ((OPL->adlibreg[0x105]&1) && cptr->is_4op)
				if (cptr->is_4op)	// this is more correct
				{
					if (OPL->adlibreg[ARC_FEEDBACK+k+3]&1)
					{
						// FM-AM-style synthesis ((op1[fb] * op2) + (op3 * op4))
						if ((cptr[0].op_state != OF_TYPE_OFF) || (cptr[9].op_state != OF_TYPE_OFF))
						{
							if ((cptr[0].vibrato) && (cptr[0].op_state != OF_TYPE_OFF))
							{
								vibval1 = vibval_var1;
								for (i=0;i<endsamples;i++)
									vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval1 = vibval_const;
							if ((cptr[9].vibrato) && (cptr[9].op_state != OF_TYPE_OFF))
							{
								vibval2 = vibval_var2;
								for (i=0;i<endsamples;i++)
									vibval2[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval2 = vibval_const;
							if (cptr[0].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;
							if (cptr[9].tremolo)
								tremval2 = trem_lut;	// tremolo enabled, use table
							else
								tremval2 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[0],vibval1[i]);
								opfuncs[cptr[0].op_state](&cptr[0]);
								operator_output(&cptr[0],(cptr[0].lastcval+cptr[0].cval)*cptr[0].mfbi/2,tremval1[i]);

								operator_advance(OPL, &cptr[9],vibval2[i]);
								opfuncs[cptr[9].op_state](&cptr[9]);
								operator_output(&cptr[9],cptr[0].cval*FIXEDPT,tremval2[i]);

								chanval = cptr[9].cval;
								CHANVAL_OUT(3)
							}
						}

						if ((cptr[3].op_state != OF_TYPE_OFF) || (cptr[3+9].op_state != OF_TYPE_OFF))
						{
							if (cptr[3].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;
							if (cptr[3+9].tremolo)
								tremval2 = trem_lut;	// tremolo enabled, use table
							else
								tremval2 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[3],0);
								opfuncs[cptr[3].op_state](&cptr[3]);
								operator_output(&cptr[3],0,tremval1[i]);

								operator_advance(OPL, &cptr[3+9],0);
								opfuncs[cptr[3+9].op_state](&cptr[3+9]);
								operator_output(&cptr[3+9],cptr[3].cval*FIXEDPT,tremval2[i]);

								chanval = cptr[3+9].cval;
								CHANVAL_OUT(3)
							}
						}

					}
					else
					{
						// FM-FM-style synthesis (op1[fb] * op2 * op3 * op4)
						if ((cptr[0].op_state != OF_TYPE_OFF) || (cptr[9].op_state != OF_TYPE_OFF) || 
							(cptr[3].op_state != OF_TYPE_OFF) || (cptr[3+9].op_state != OF_TYPE_OFF))
						{
							if ((cptr[0].vibrato) && (cptr[0].op_state != OF_TYPE_OFF))
							{
								vibval1 = vibval_var1;
								for (i=0;i<endsamples;i++)
									vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval1 = vibval_const;
							if ((cptr[9].vibrato) && (cptr[9].op_state != OF_TYPE_OFF))
							{
								vibval2 = vibval_var2;
								for (i=0;i<endsamples;i++)
									vibval2[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
							}
							else
								vibval2 = vibval_const;
							if (cptr[0].tremolo)
								tremval1 = trem_lut;	// tremolo enabled, use table
							else
								tremval1 = tremval_const;
							if (cptr[9].tremolo)
								tremval2 = trem_lut;	// tremolo enabled, use table
							else
								tremval2 = tremval_const;
							if (cptr[3].tremolo)
								tremval3 = trem_lut;	// tremolo enabled, use table
							else
								tremval3 = tremval_const;
							if (cptr[3+9].tremolo)
								tremval4 = trem_lut;	// tremolo enabled, use table
							else
								tremval4 = tremval_const;

							// calculate channel output
							for (i=0;i<endsamples;i++)
							{
								Bit32s chanval;
								
								operator_advance(OPL, &cptr[0],vibval1[i]);
								opfuncs[cptr[0].op_state](&cptr[0]);
								operator_output(&cptr[0],(cptr[0].lastcval+cptr[0].cval)*cptr[0].mfbi/2,tremval1[i]);

								operator_advance(OPL, &cptr[9],vibval2[i]);
								opfuncs[cptr[9].op_state](&cptr[9]);
								operator_output(&cptr[9],cptr[0].cval*FIXEDPT,tremval2[i]);

								operator_advance(OPL, &cptr[3],0);
								opfuncs[cptr[3].op_state](&cptr[3]);
								operator_output(&cptr[3],cptr[9].cval*FIXEDPT,tremval3[i]);

								operator_advance(OPL, &cptr[3+9],0);
								opfuncs[cptr[3+9].op_state](&cptr[3+9]);
								operator_output(&cptr[3+9],cptr[3].cval*FIXEDPT,tremval4[i]);

								chanval = cptr[3+9].cval;
								CHANVAL_OUT(3)
							}
						}
					}
					continue;
				}
#endif
				// 2op frequency modulation
				if ((cptr[9].op_state == OF_TYPE_OFF) && (cptr[0].op_state == OF_TYPE_OFF)) continue;
				if ((cptr[0].vibrato) && (cptr[0].op_state != OF_TYPE_OFF))
				{
					vibval1 = vibval_var1;
					for (i=0;i<endsamples;i++)
						vibval1[i] = (Bit32s)((vib_lut[i]*cptr[0].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval1 = vibval_const;
				if ((cptr[9].vibrato) && (cptr[9].op_state != OF_TYPE_OFF))
				{
					vibval2 = vibval_var2;
					for (i=0;i<endsamples;i++)
						vibval2[i] = (Bit32s)((vib_lut[i]*cptr[9].freq_high/8)*FIXEDPT*VIBFAC);
				}
				else
					vibval2 = vibval_const;
				if (cptr[0].tremolo)
					tremval1 = trem_lut;	// tremolo enabled, use table
				else
					tremval1 = tremval_const;
				if (cptr[9].tremolo)
					tremval2 = trem_lut;	// tremolo enabled, use table
				else
					tremval2 = tremval_const;

				// calculate channel output
				for (i=0;i<endsamples;i++)
				{
					Bit32s chanval;
					
					// modulator
					operator_advance(OPL, &cptr[0],vibval1[i]);
					opfuncs[cptr[0].op_state](&cptr[0]);
					operator_output(&cptr[0],(cptr[0].lastcval+cptr[0].cval)*cptr[0].mfbi/2,tremval1[i]);

					// carrier
					operator_advance(OPL, &cptr[9],vibval2[i]);
					opfuncs[cptr[9].op_state](&cptr[9]);
					operator_output(&cptr[9],cptr[0].cval*FIXEDPT,tremval2[i]);

					chanval = cptr[9].cval;
					CHANVAL_OUT(0)
				}
			}
		}

/*#if defined(OPLTYPE_IS_OPL3)
		if (adlibreg[0x105]&1)
		{
			// convert to 16bit samples (stereo)
			for (i=0;i<endsamples;i++)
			{
				clipit16(outbufl[i],sndptr++);
				clipit16(outbufr[i],sndptr++);
			}
		}
		else
		{
			// convert to 16bit samples (mono)
			for (i=0;i<endsamples;i++)
			{
				clipit16(outbufl[i],sndptr++);
				clipit16(outbufl[i],sndptr++);
			}
		}
#else
		// convert to 16bit samples
		for (i=0;i<endsamples;i++)
			clipit16(outbufl[i],sndptr++);
#endif*/

	}
}

void ADLIBEMU(set_mute_mask)(void *chip, UINT32 MuteMask)
{
	OPL_DATA* OPL = (OPL_DATA*)chip;
	
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < NUM_CHANNELS + 5; CurChn ++)
		OPL->MuteChn[CurChn] = (MuteMask >> CurChn) & 0x01;
	
	return;
}
