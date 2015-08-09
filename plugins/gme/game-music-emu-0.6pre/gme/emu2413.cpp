// Permission is granted to anyone to use this software for any purpose,
// including commercial applications. To alter this software and redistribute it freely,
// if the origin of this software is not misrepresented.

// written by Mitsutaka Okazaki 2001
// Modified by xodnizel to remove code not needed for the VRC7, among other things.
// Optimized performance and code size - Shay Green

// References: 
// fmopl.c        -- 1999,2000 written by Tatsuyuki Satoh (MAME development).
// fmopl.c(fixed) -- (C) 2002 Jarek Burczynski.
// s_opl.c        -- 2001 written by Mamiya (NEZplug development).
// fmgen.cpp      -- 1999,2000 written by cisc.
// fmpac.ill      -- 2000 created by NARUTO.
// MSX-Datapack
// YMU757 data sheet
// YM2143 data sheet

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "emu2413.h"

#undef PI
#define PI 3.14159265358979323846

static const unsigned char default_inst[15][8] =
{
// VRC7 instruments, January 17, 2004 update -Xodnizel
    {0x03, 0x21, 0x04, 0x06, 0x8D, 0xF2, 0x42, 0x17}, 
    {0x13, 0x41, 0x05, 0x0E, 0x99, 0x96, 0x63, 0x12}, 
    {0x31, 0x11, 0x10, 0x0A, 0xF0, 0x9C, 0x32, 0x02}, 
    {0x21, 0x61, 0x1D, 0x07, 0x9F, 0x64, 0x20, 0x27}, 
    {0x22, 0x21, 0x1E, 0x06, 0xF0, 0x76, 0x08, 0x28}, 
    {0x02, 0x01, 0x06, 0x00, 0xF0, 0xF2, 0x03, 0x95}, 
    {0x21, 0x61, 0x1C, 0x07, 0x82, 0x81, 0x16, 0x07}, 
    {0x23, 0x21, 0x1A, 0x17, 0xEF, 0x82, 0x25, 0x15}, 
    {0x25, 0x11, 0x1F, 0x00, 0x86, 0x41, 0x20, 0x11}, 
    {0x85, 0x01, 0x1F, 0x0F, 0xE4, 0xA2, 0x11, 0x12}, 
    {0x07, 0xC1, 0x2B, 0x45, 0xB4, 0xF1, 0x24, 0xF4},
    {0x61, 0x23, 0x11, 0x06, 0x96, 0x96, 0x13, 0x16}, 
    {0x01, 0x02, 0xD3, 0x05, 0x82, 0xA2, 0x31, 0x51}, 
    {0x61, 0x22, 0x0D, 0x02, 0xC3, 0x7F, 0x24, 0x05},
    {0x21, 0x62, 0x0E, 0x00, 0xA1, 0xA0, 0x44, 0x17},

};

#define EG2DB(d) ((d)*(e_int32)(EG_STEP/DB_STEP))
#define TL2EG(d) ((d)*(e_int32)(TL_STEP/EG_STEP))
#define SL2EG(d) ((d)*(e_int32)(SL_STEP/EG_STEP))

// Bits for liner value
#define DB2LIN_AMP_BITS 11
#define SLOT_AMP_BITS (DB2LIN_AMP_BITS)

// Bits for envelope phase incremental counter
#define EG_DP_BITS 22
#define EG_DP_WIDTH (1<<EG_DP_BITS)

// PM table is calcurated by PM_AMP * pow(2,PM_DEPTH*sin(x)/1200)
#define PM_AMP_BITS 8
#define PM_AMP (1<<PM_AMP_BITS)

// PM speed(Hz) and depth(cent)
#define PM_SPEED 6.4
#define PM_DEPTH 13.75

// AM speed(Hz) and depth(dB)
#define AM_SPEED 3.7
//#define AM_DEPTH 4.8
#define AM_DEPTH 2.4

// Cut the lower b bit(s) off.
#define HIGHBITS(c,b) ((c)>>(b))

// Expand x which is s bits to d bits.
#define EXPAND_BITS(x,s,d) ((x)<<((d)-(s)))

#define CAR_OFFSET 1
#define MOD_CAR( o, x, sel ) (&((o)->slot + (sel)) [(x) << 1])
#define MOD(o,x) MOD_CAR( o, x, 0 )
#define CAR(o,x) MOD_CAR( o, x, 1 )

// Definition of envelope mode
enum { SUSHOLD, SETTLE, ATTACK, DECAY, SUSTINE, RELEASE, FINISH };

inline static void update_eg_dphase_inl ( OPLL* opll, OPLL_SLOT* slot, e_int32 slot_eg_mode )
{
	e_uint32 result = 0;
	int index = slot->patch.RR;
	switch ( slot_eg_mode )
	{
	case ATTACK:
		result = opll->dphaseARTable[slot->patch.AR][slot->rks];
		break;

	case DECAY:
		index = slot->patch.DR;
		goto common;

	case RELEASE:
		if (slot->sustine)
			index = 5;
		else if (!slot->patch.EG)
			index = 7;
	case SUSTINE:
	common:
		result = opll->dphaseDRTable [index] [slot->rks];
	case SUSHOLD:
	case FINISH:
	default:
		break;
	}
	
	slot->eg_dphase = result;
}

// out = in, then verify that value wasn't truncated
#define ASSIGN( out, in )\
	((out = in), assert( out == in ))

static void maketables ( OPLL* opll )
{
	e_int32 i;
	
	// Table for Pitch Modulator
	for (i = 0; i < PM_PG_WIDTH; i++)
		ASSIGN( opll->pmtable[i],
				(e_int32) ((double) PM_AMP * pow (2, (double) PM_DEPTH * sin (2.0 * PI * i / PM_PG_WIDTH) / 1200)) );
	
	// Table for Amp Modulator
	for ( i = 0; i < AM_PG_WIDTH; i++)
		ASSIGN( opll->amtable[i],
				(e_int32) ((double) AM_DEPTH / 2 / DB_STEP * (1.0 + sin (2.0 * PI * i / PM_PG_WIDTH))) );
	
	// Table for dB(0 -- (1<<DB_BITS)-1) to Liner(0 -- DB2LIN_AMP_WIDTH)
	for (i = 0; i < DB_MUTE; i++)
		ASSIGN( opll->DB2LIN_TABLE[i],
				(e_int16) ((double) ((1 << DB2LIN_AMP_BITS) - 1) * pow (10, -(double) i * DB_STEP / 20)) );
	
	// TODO: remove (calloc already ensures zero fill)
	//for (i = DB_MUTE; i < 2 * DB_MUTE; i++)
	//	opll->DB2LIN_TABLE[i] = 0;
	
	for (i = 0; i < 2 * DB_MUTE; i++)
		ASSIGN( opll->DB2LIN_TABLE[i + DB_MUTE + DB_MUTE],
				(e_int16) (-opll->DB2LIN_TABLE[i]) );
	
	// Table for AR to LogCurve.
	{
		ASSIGN( opll->AR_ADJUST_TABLE[0], (1 << EG_BITS) );
		for (int i = 1; i < 128; i++)
			ASSIGN( opll->AR_ADJUST_TABLE[i],
					(e_uint16) ((double) (1 << EG_BITS) - 1 - (1 << EG_BITS) * log ((double)i) / log (128.)) );
	}
	
	{
		#define dB2(x) ((x)*2)
		static const double kltable[16] = {
			dB2 (0.000), dB2 (9.000), dB2 (12.000), dB2 (13.875), dB2 (15.000), dB2 (16.125), dB2 (16.875), dB2 (17.625),
			dB2 (18.000), dB2 (18.750), dB2 (19.125), dB2 (19.500), dB2 (19.875), dB2 (20.250), dB2 (20.625), dB2 (21.000)
		};

		for (int fnum = 0; fnum < 16; fnum++)
		{
			for (int block = 0; block < 8; block++)
			{
				for (int TL = 0; TL < 64; TL++)
				{
					e_uint32 eg = TL2EG( TL );
					ASSIGN( opll->tllTable[fnum][block][TL][0], eg );
					for (int KL = 1; KL < 4; KL++)
					{
						e_int32 tmp = (e_int32) (kltable[fnum] - dB2 (3.000) * (7 - block));
						e_uint32 n = eg;
						if ( tmp > 0 )
							n += (e_uint32) ((tmp >> (3 - KL)) / EG_STEP);
						ASSIGN( opll->tllTable[fnum][block][TL][KL], n );
					}
				}
			}
		}
	}

	{
		for (int fnum8 = 0; fnum8 < 2; fnum8++)
		{
			for (int block = 0; block < 8; block++)
			{
				ASSIGN( opll->rksTable[fnum8][block][0], block >> 1 );
				for (int KR = 1; KR < 2; KR++)
					ASSIGN( opll->rksTable[fnum8][block][KR], (block << 1) + fnum8 );
			}
		}
	}

	// Sin Table
	for (i = 0; i < PG_WIDTH / 4; i++)
	{
		double d = sin (2.0 * PI / PG_WIDTH * i);
		
		// Liner(+0.0 - +1.0) to dB((1<<DB_BITS) - 1 -- 0)
		e_int32 x = DB_MUTE - 1;
		if ( d )
		{
			e_int32 y = -(e_int32) (20.0 / DB_STEP * log10 (d));
			if ( x > y )
				x = y;
		}

		ASSIGN( opll->sintable [0] [i], (e_uint32) x );
	}

	for (i = 0; i < PG_WIDTH / 4; i++)
		ASSIGN( opll->sintable [0] [PG_WIDTH / 2 - 1 - i], opll->sintable [0] [i] );

	for (i = 0; i < PG_WIDTH / 2; i++)
		ASSIGN( opll->sintable [0] [PG_WIDTH / 2 + i], (e_uint32) (DB_MUTE + DB_MUTE + opll->sintable [0] [i]) );

	for (i = 0; i < PG_WIDTH / 2; i++)
		ASSIGN( opll->sintable [1] [i], opll->sintable [0] [i] );
	
	for (i = PG_WIDTH / 2; i < PG_WIDTH; i++)
		ASSIGN( opll->sintable [1] [i], opll->sintable [0] [0] );

	//makeDefaultPatch ();
	
	// internal refresh
	
	// Phase increment counter table
	{
		static int const mltable[16] = {
			1, 1 * 2, 2 * 2, 3 * 2, 4 * 2, 5 * 2, 6 * 2, 7 * 2, 8 * 2,
			9 * 2, 10 * 2, 10 * 2, 12 * 2, 12 * 2, 15 * 2, 15 * 2
		};
		for (int fnum = 0; fnum < 512; fnum++)
			for (int block = 0; block < 8; block++)
				for (int ML = 0; ML < 16; ML++)
					ASSIGN( opll->dphaseTable[fnum][block][ML],
							(((fnum * mltable[ML]) << block) >> (20 - DP_BITS)) );
	}

	// Rate Table for Attack
	{
		for (int Rks = 0; Rks < 16; Rks++)
		{
			// TODO: remove (calloc already ensures zero fill)
			//opll->dphaseARTable[ 0][Rks] = 0;
			//opll->dphaseARTable[15][Rks] = 0;
			int RL = (Rks & 3) * 3 + 12;
			for (int AR = 1; AR < 15; AR++)
			{
				int RM = AR + (Rks >> 2);
				if (RM > 15)
					RM = 15;
				ASSIGN( opll->dphaseARTable[AR][Rks], RL << (RM + 1) );
			}
		}
	}

	// Rate Table for Decay and Release
	{
		for (int Rks = 0; Rks < 16; Rks++)
		{
			// TODO: remove (calloc already ensures zero fill)
			//opll->dphaseDRTable[0][Rks] = 0;
			int RL = (Rks & 3) + 4;
			for (int DR = 1; DR < 16; DR++)
			{
				int RM = DR + (Rks >> 2);
				if (RM > 15)
					RM = 15;
				ASSIGN( opll->dphaseDRTable[DR][Rks], RL << (RM - 1) );
			}
		}
	}
}

OPLL* VRC7_new( long clock_rate )
{
	OPLL* opll = (OPLL*) calloc( sizeof *opll, 1 );
	if ( opll )
	{
		maketables( opll );
		ASSIGN( opll->pm_dphase, (e_uint32) (PM_SPEED * PM_DP_WIDTH / (clock_rate / 72)) );
		ASSIGN( opll->am_dphase, (e_uint32) (AM_SPEED * AM_DP_WIDTH / (clock_rate / 72)) );
		
		VRC7_reset (opll);
	}
	return opll;
}

void VRC7_delete( OPLL* opll )
{
	free (opll);
}

// Reset whole of OPLL except patch datas.
void VRC7_reset( OPLL* opll )
{
	opll->pm_phase = opll->pm_dphase;
	opll->am_phase = opll->am_dphase;
	
	int i;
	for ( i = 0; i < 12; i++)
	{
		OPLL_SLOT* slot = &opll->slot[i];
		memset( slot, 0, offsetof (OPLL_SLOT,patch) );
		slot->sintbl    = opll->sintable [0];
		slot->eg_mode   = SETTLE;
		slot->eg_phase  = EG_DP_WIDTH;
	}

	for (i = 0; i < 0x40; i++)
		VRC7_writeReg (opll, i, 0);
}

// Force Refresh (When external program changes some parameters).
/*
void VRC7_forceRefresh( OPLL* opll )
{
	for (e_int32 i = 0; i < 12; i++)
		UPDATE_ALL( opll, &opll->slot[i], i & 1 );
}
*/

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 2PI).
#if ( SLOT_AMP_BITS - PG_BITS ) > 0
#define wave2_2pi(e)  ( (e) >> ( SLOT_AMP_BITS - PG_BITS ))
#else
#define wave2_2pi(e)  ( (e) << ( PG_BITS - SLOT_AMP_BITS ))
#endif

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 4PI).
#if ( SLOT_AMP_BITS - PG_BITS - 1 ) == 0
#define wave2_4pi(e)  (e)
#elif ( SLOT_AMP_BITS - PG_BITS - 1 ) > 0
#define wave2_4pi(e)  ( (e) >> ( SLOT_AMP_BITS - PG_BITS - 1 ))
#else
#define wave2_4pi(e)  ( (e) << ( 1 + PG_BITS - SLOT_AMP_BITS ))
#endif

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 8PI).
#if ( SLOT_AMP_BITS - PG_BITS - 2 ) == 0
#define wave2_8pi(e)  (e)
#elif ( SLOT_AMP_BITS - PG_BITS - 2 ) > 0
#define wave2_8pi(e)  ( (e) >> ( SLOT_AMP_BITS - PG_BITS - 2 ))
#else
#define wave2_8pi(e)  ( (e) << ( 2 + PG_BITS - SLOT_AMP_BITS ))
#endif

void VRC7_run( OPLL* opll )
{
	// PM
	int const opll_lfo_pm = opll->pmtable [HIGHBITS( opll->pm_phase, PM_DP_BITS - PM_PG_BITS )];
	opll->pm_phase = (opll->pm_phase + opll->pm_dphase) & (PM_DP_WIDTH - 1);
	
	{
		int n = 12;
		OPLL_SLOT* slot = opll->slot;
		do
		{
			// phase
			int step = slot->dphase;
			if ( slot->patch.PM ) // 36%
				step = (step * opll_lfo_pm) >> PM_AMP_BITS;
			
			e_uint32 slot_phase = (slot->phase + step) & (DP_WIDTH - 1);
			slot->phase = slot_phase;
			slot->pgout = HIGHBITS( slot_phase, DP_BASE_BITS );
			
			slot++;
		}
		while ( --n );
	}
	
	// AM
	int const opll_lfo_am = opll->amtable [HIGHBITS( opll->am_phase, AM_DP_BITS - AM_PG_BITS )];
	opll->am_phase = (opll->am_phase + opll->am_dphase) & (AM_DP_WIDTH - 1);
	
	int n = 12;
	OPLL_SLOT* slot = opll->slot;
	do
	{
		// envelope
		e_uint32 egout = HIGHBITS( slot->eg_phase, EG_DP_BITS - EG_BITS );
		
		switch ( slot->eg_mode )
		{
		case SUSHOLD: // 54%
			if ( slot->patch.EG ) // 99%
				break;
			slot->eg_mode = SUSTINE;
			update_eg_dphase_inl( opll, slot, SUSTINE );
			break;

		case DECAY:{// 23%
			#define S2E(x) (SL2EG((e_int32)(x/SL_STEP))<<(EG_DP_BITS-EG_BITS))
			static const e_uint32 SL[16] = {
				S2E (0.0), S2E (3.0), S2E (6.0), S2E (9.0), S2E (12.0), S2E (15.0), S2E (18.0), S2E (21.0),
				S2E (24.0), S2E (27.0), S2E (30.0), S2E (33.0), S2E (36.0), S2E (39.0), S2E (42.0), S2E (48.0)
			};
			slot->eg_phase += slot->eg_dphase;
			if ( slot->eg_phase >= SL [slot->patch.SL] )
			{
				slot->eg_phase = SL [slot->patch.SL];
				if ( slot->patch.EG )
				{
					slot->eg_mode = SUSHOLD;
					update_eg_dphase_inl( opll, slot, SUSHOLD );
				}
				else
				{
					slot->eg_mode = SUSTINE;
					update_eg_dphase_inl( opll, slot, SUSTINE );
				}
			}
			break;
		}

		case ATTACK: // 3%
			egout = opll->AR_ADJUST_TABLE[egout];
			slot->eg_phase += slot->eg_dphase;
			if((EG_DP_WIDTH & slot->eg_phase)||(slot->patch.AR==15))
			{
				egout = 0;
				slot->eg_phase = 0;
				slot->eg_mode = DECAY;
				update_eg_dphase_inl( opll, slot, DECAY );
			}
			break;

		case SUSTINE:
		case RELEASE: // 18%
			slot->eg_phase += slot->eg_dphase;
			if ( egout < (1 << EG_BITS) )
				break;
			slot->eg_mode = FINISH;
		case FINISH: // 2%
		default:
			egout = (1 << EG_BITS) - 1;
			break;
		}
		
		egout = EG2DB( egout + slot->tll );
		if ( slot->patch.AM )
			egout += opll_lfo_am;

		if ( egout > DB_MUTE - 1 )
			egout = DB_MUTE;
		
		slot->egout = egout;
		
		slot++;
	}
	while ( --n );
}

e_uint32 VRC7_calcCh( OPLL* opll, e_uint32 ch )
{
	OPLL_SLOT* slot = MOD( opll, ch );
	
	// modulator
	e_int32 feedback;
	{
		e_int32 fm = wave2_4pi( slot->feedback ) >> slot->patch.FB_shift;
		int index = (slot->pgout + fm) & (PG_WIDTH - 1);
		feedback = opll->DB2LIN_TABLE [slot->sintbl [index] + slot->egout];
		assert( slot->egout < DB_MUTE || feedback == 0 ); // was DB_MUTE - 1 in original
		
		e_int32 slot_output_1 = slot->output [0];
		
		if ( slot [CAR_OFFSET].eg_mode == FINISH ) // 3%
			return 0;
		
		slot->output [0] = feedback;
		slot->output [1] = slot_output_1;
		
		feedback = (feedback + slot_output_1) >> 1;
		slot->feedback = feedback;
	}
	slot += CAR_OFFSET;
	
	// carrier
	e_int32 output = opll->DB2LIN_TABLE [
			slot->sintbl [(slot->pgout + wave2_8pi( feedback )) & (PG_WIDTH-1)] + slot->egout];
	assert( slot->egout < DB_MUTE || output == 0 ); // was DB_MUTE - 1 in original
	
	e_int32 slot_output_1 = slot->output [0];
	slot->output [0] = output;
	slot->output [1] = slot_output_1;
	
	return (output + slot_output_1) >> 1;
}

static void setInstrument( OPLL* opll, unsigned i, unsigned inst )
{
	opll->patch_number[i]=inst;
	
	const e_uint8* src = opll->CustInst;
	if(inst)
		src=default_inst[inst-1];

	OPLL_PATCH* modp=&MOD(opll,i)->patch;
	OPLL_PATCH* carp=&CAR(opll,i)->patch;
	
	int src_0 = src [0];
	modp->AM=(src_0>>7)&1;
	modp->PM=(src_0>>6)&1;
	modp->EG=(src_0>>5&1);
	modp->KR=(src_0>>4)&1;
	modp->ML=(src_0&0xF);

	int src_1 = src [1];
	carp->AM=(src_1>>7)&1;
	carp->PM=(src_1>>6)&1;
	carp->EG=(src_1>>5&1);
	carp->KR=(src_1>>4)&1;
	carp->ML=(src_1&0xF);  

	int src_2 = src [2];
	modp->KL=(src_2>>6)&3;
	modp->TL=(src_2&0x3F);

	int src_3 = src [3];
	carp->KL = (src_3 >> 6) & 3; 
	carp->WF = (src_3 >> 4) & 1;

	modp->WF = (src_3 >> 3) & 1;
	
	int FB = (src_3) & 7;
	modp->FB_shift = (FB ? 7 - FB : 31);
	
	int src_4 = src [4];
	modp->AR = (src_4>>4)&0xF;
	modp->DR = (src_4&0xF);

	int src_5 = src [5];
	carp->AR = (src_5>>4)&0xF;
	carp->DR = (src_5&0xF);

	int src_6 = src [6];
	modp->SL = (src_6>>4)&0xF;
	modp->RR = (src_6&0xF);

	int src_7 = src [7];
	carp->SL = (src_7>>4)&0xF;
	carp->RR = (src_7&0xF);
}

static void update_eg_dphase( OPLL* opll, OPLL_SLOT* slot )
{
	update_eg_dphase_inl( opll, slot, slot->eg_mode );
}

#define UPDATE_PG(S)  (S)->dphase = opll->dphaseTable[(S)->fnum][(S)->block][(S)->patch.ML]
#define UPDATE_RKS(S) (S)->rks = opll->rksTable[((S)->fnum)>>8][(S)->block][(S)->patch.KR]
#define UPDATE_WF(S)  (S)->sintbl = opll->sintable [(S)->patch.WF]
#define UPDATE_EG(S)  update_eg_dphase( opll, S )

inline static void UPDATE_TLL( OPLL* opll, OPLL_SLOT* S, int type )
{
	int index = (type ? (S)->volume : (S)->patch.TL);
	(S)->tll = opll->tllTable [(S)->fnum >> 5] [(S)->block] [index] [(S)->patch.KL];
}

static void UPDATE_ALL( OPLL* opll, OPLL_SLOT* S, int type )
{
	UPDATE_PG(S);
	UPDATE_TLL(opll, S, type);
	UPDATE_RKS(S);
	UPDATE_WF(S);
	UPDATE_EG(S); // must be done last
}

static void UPDATE_ALL_BOTH( OPLL* opll, OPLL_SLOT* slot )
{
	UPDATE_ALL( opll, slot, 0 );
	UPDATE_ALL( opll, slot + CAR_OFFSET, 1 );
}

inline static void slotOn( OPLL_SLOT* slot )
{
	slot->eg_mode  = ATTACK;
	slot->eg_phase = 0;
	slot->phase    = 0;
}

inline static void setFnumber( OPLL_SLOT* slot, e_int32 fnum )
{
	slot [         0].fnum = fnum;
	slot [CAR_OFFSET].fnum = fnum;
}

inline static void setBlock( OPLL_SLOT* slot, e_int32 block )
{
	slot [         0].block = block;
	slot [CAR_OFFSET].block = block;
}

void VRC7_writeReg( OPLL* opll, int addr, e_uint32 data )
{
	e_int32 i;
	
	data &= 0xFF;
	addr &= 0x3F;

	switch ( addr )
	{
	case 0x00:
	case 0x01:
		opll->CustInst[addr]=data;
		for (i = 0; i < 6; i++)
		{
			if (opll->patch_number[i] == 0)
			{
				setInstrument(opll, i, 0);
				OPLL_SLOT* slot = MOD_CAR( opll, i, addr & 1 );
				UPDATE_PG ( slot );
				UPDATE_RKS( slot );
				UPDATE_EG ( slot );
			}
		}
		break;

	case 0x02:
		opll->CustInst[2]=data;
		for (i = 0; i < 6; i++)
		{
			if (opll->patch_number[i] == 0)
			{
				setInstrument(opll, i, 0);
				UPDATE_TLL(opll, MOD(opll,i), 0 );
			}
		}
		break;

	case 0x03:
		opll->CustInst[3]=data;
		for (i = 0; i < 6; i++)
		{
			if (opll->patch_number[i] == 0)
			{
				setInstrument(opll, i, 0);
				UPDATE_WF(MOD(opll,i));
				UPDATE_WF(CAR(opll,i));
			}
		}
		break;

	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		opll->CustInst[addr]=data;
		for ( i = 0; i < 6; i++ )
		{
			if ( opll->patch_number [i] == 0 )
			{
				setInstrument( opll, i, 0 );
				UPDATE_EG( MOD_CAR( opll, i, addr & 1 ) );
			}
		}
		break;
	
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15: {
		int ch = addr - 0x10;
		opll->LowFreq [ch] = data;
		OPLL_SLOT* slot = MOD( opll, ch );
		
		setFnumber( slot, data + ((opll->HiFreq[ch] & 1) << 8));
		UPDATE_ALL_BOTH( opll, slot );
		break;
	}
	
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25: {
		int ch = addr - 0x20;
		opll->HiFreq [ch] = data;
		OPLL_SLOT* slot = MOD( opll, ch );
		
		setFnumber( slot, ((data & 1) << 8) + opll->LowFreq[ch]);
		setBlock( slot, (data >> 1) & 7);
		slot [CAR_OFFSET].sustine = (data >> 5) & 1;
		if (data & 0x10)
		{
			if ( !slot->slot_on_flag )
				slotOn( slot );
			
			if ( !slot [CAR_OFFSET].slot_on_flag )
				slotOn( slot + CAR_OFFSET );
		}
		else if ( slot [CAR_OFFSET].slot_on_flag )
		{
			if (slot [CAR_OFFSET].eg_mode == ATTACK)
				slot [CAR_OFFSET].eg_phase = EXPAND_BITS (opll->AR_ADJUST_TABLE[HIGHBITS (slot [CAR_OFFSET].eg_phase, EG_DP_BITS - EG_BITS)], EG_BITS, EG_DP_BITS);
			slot [CAR_OFFSET].eg_mode = RELEASE;
		}
		UPDATE_ALL_BOTH( opll, slot );
		
		//inline static void update_key_status (OPLL * opll)
		for (ch = 0; ch < 6; ch++)
		{
			int flag = (opll->HiFreq [ch]) & 0x10;
			OPLL_SLOT* slot = MOD( opll, ch );
			slot [         0].slot_on_flag = flag;
			slot [CAR_OFFSET].slot_on_flag = flag;
		}
		break;
	}

	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35: {
		int ch = addr - 0x30;
		opll->InstVol [ch] = data;
		
		setInstrument( opll, ch, data >> 4 & 0x0F );
		OPLL_SLOT* slot = MOD( opll, ch );
		slot [CAR_OFFSET].volume = (data & 0x0F) << 2;
		UPDATE_ALL_BOTH( opll, slot );
		break;
	}
	
	default:
		break;
	}
}
