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


#define fltype double

/*
	define Bits, Bitu, Bit32s, Bit32u, Bit16s, Bit16u, Bit8s, Bit8u here
*/
/*
#include <stdint.h>
typedef uintptr_t	Bitu;
typedef intptr_t	Bits;
typedef uint32_t	Bit32u;
typedef int32_t		Bit32s;
typedef uint16_t	Bit16u;
typedef int16_t		Bit16s;
typedef uint8_t		Bit8u;
typedef int8_t		Bit8s;
*/

typedef UINT32		Bitu;
typedef INT32		Bits;
typedef UINT32		Bit32u;
typedef INT32		Bit32s;
typedef UINT16		Bit16u;
typedef INT16		Bit16s;
typedef UINT8		Bit8u;
typedef INT8		Bit8s;

/*
	define attribution that inlines/forces inlining of a function (optional)
*/
#define OPL_INLINE INLINE


#undef NUM_CHANNELS
#if defined(OPLTYPE_IS_OPL3)
#define NUM_CHANNELS	18
#else
#define NUM_CHANNELS	9
#endif

#define MAXOPERATORS	(NUM_CHANNELS*2)


#define FL05	((fltype)0.5)
#define FL2		((fltype)2.0)
#define PI		((fltype)3.1415926535897932384626433832795)


#define FIXEDPT			0x10000		// fixed-point calculations using 16+16
#define FIXEDPT_LFO		0x1000000	// fixed-point calculations using 8+24

#define WAVEPREC		1024		// waveform precision (10 bits)

//#define INTFREQU		((fltype)(14318180.0 / 288.0))		// clocking of the chip
#if defined(OPLTYPE_IS_OPL3)
#define INTFREQU		((fltype)(OPL->chip_clock / 288.0))		// clocking of the chip
#else
#define INTFREQU		((fltype)(OPL->chip_clock / 72.0))		// clocking of the chip
#endif


#define OF_TYPE_ATT			0
#define OF_TYPE_DEC			1
#define OF_TYPE_REL			2
#define OF_TYPE_SUS			3
#define OF_TYPE_SUS_NOKEEP	4
#define OF_TYPE_OFF			5

#define ARC_CONTROL			0x00
#define ARC_TVS_KSR_MUL		0x20
#define ARC_KSL_OUTLEV		0x40
#define ARC_ATTR_DECR		0x60
#define ARC_SUSL_RELR		0x80
#define ARC_FREQ_NUM		0xa0
#define ARC_KON_BNUM		0xb0
#define ARC_PERC_MODE		0xbd
#define ARC_FEEDBACK		0xc0
#define ARC_WAVE_SEL		0xe0

#define ARC_SECONDSET		0x100	// second operator set for OPL3


#define OP_ACT_OFF			0x00
#define OP_ACT_NORMAL		0x01	// regular channel activated (bitmasked)
#define OP_ACT_PERC			0x02	// percussion channel activated (bitmasked)

#define BLOCKBUF_SIZE		512


// vibrato constants
#define VIBTAB_SIZE			8
#define VIBFAC				70/50000		// no braces, integer mul/div

// tremolo constants and table
#define TREMTAB_SIZE		53
#define TREM_FREQ			((fltype)(3.7))			// tremolo at 3.7hz


/* operator struct definition
     For OPL2 all 9 channels consist of two operators each, carrier and modulator.
     Channel x has operators x as modulator and operators (9+x) as carrier.
     For OPL3 all 18 channels consist either of two operators (2op mode) or four
     operators (4op mode) which is determined through register4 of the second
     adlib register set.
     Only the channels 0,1,2 (first set) and 9,10,11 (second set) can act as
     4op channels. The two additional operators for a channel y come from the
     2op channel y+3 so the operatorss y, (9+y), y+3, (9+y)+3 make up a 4op
     channel.
*/
typedef struct operator_struct {
	Bit32s cval, lastcval;			// current output/last output (used for feedback)
	Bit32u tcount, wfpos, tinc;		// time (position in waveform) and time increment
	fltype amp, step_amp;			// and amplification (envelope)
	fltype vol;						// volume
	fltype sustain_level;			// sustain level
	Bit32s mfbi;					// feedback amount
	fltype a0, a1, a2, a3;			// attack rate function coefficients
	fltype decaymul, releasemul;	// decay/release rate functions
	Bit32u op_state;				// current state of operator (attack/decay/sustain/release/off)
	Bit32u toff;
	Bit32s freq_high;				// highest three bits of the frequency, used for vibrato calculations
	Bit16s* cur_wform;				// start of selected waveform
	Bit32u cur_wmask;				// mask for selected waveform
	Bit32u act_state;				// activity state (regular, percussion)
	bool sus_keep;					// keep sustain level when decay finished
	bool vibrato,tremolo;			// vibrato/tremolo enable bits
	
	// variables used to provide non-continuous envelopes
	Bit32u generator_pos;			// for non-standard sample rates we need to determine how many samples have passed
	Bits cur_env_step;				// current (standardized) sample position
	Bits env_step_a,env_step_d,env_step_r;	// number of std samples of one step (for attack/decay/release mode)
	Bit8u step_skip_pos_a;			// position of 8-cyclic step skipping (always 2^x to check against mask)
	Bits env_step_skip_a;			// bitmask that determines if a step is skipped (respective bit is zero then)

#if defined(OPLTYPE_IS_OPL3)
	bool is_4op,is_4op_attached;	// base of a 4op channel/part of a 4op channel
	Bit32s left_pan,right_pan;		// opl3 stereo panning amount
#endif
} op_type;

typedef struct opl_chip
{
	// per-chip variables
	//Bitu chip_num;
	op_type op[MAXOPERATORS];
	Bit8u MuteChn[NUM_CHANNELS + 5];
	Bitu chip_clock;
	
	Bits int_samplerate;
	
	Bit8u status;
	Bit32u opl_index;
	Bits opl_addr;
#if defined(OPLTYPE_IS_OPL3)
	Bit8u adlibreg[512];	// adlib register set (including second set)
	Bit8u wave_sel[44];		// waveform selection
#else
	Bit8u adlibreg[256];	// adlib register set
	Bit8u wave_sel[22];		// waveform selection
#endif
	
	
	// vibrato/tremolo increment/counter
	Bit32u vibtab_pos;
	Bit32u vibtab_add;
	Bit32u tremtab_pos;
	Bit32u tremtab_add;
	
	Bit32u generator_add;	// should be a chip parameter
	
	fltype recipsamp;	// inverse of sampling rate
	fltype frqmul[16];
	
	ADL_UPDATEHANDLER UpdateHandler;	// stream update handler
	void* UpdateParam;					// stream update parameter
} OPL_DATA;


// enable an operator
static void enable_operator(OPL_DATA* chip, Bitu regbase, op_type* op_pt, Bit32u act_type);

// functions to change parameters of an operator
static void change_frequency(OPL_DATA* chip, Bitu chanbase, Bitu regbase, op_type* op_pt);

static void change_attackrate(OPL_DATA* chip, Bitu regbase, op_type* op_pt);
static void change_decayrate(OPL_DATA* chip, Bitu regbase, op_type* op_pt);
static void change_releaserate(OPL_DATA* chip, Bitu regbase, op_type* op_pt);
static void change_sustainlevel(OPL_DATA* chip, Bitu regbase, op_type* op_pt);
static void change_waveform(OPL_DATA* chip, Bitu regbase, op_type* op_pt);
static void change_keepsustain(OPL_DATA* chip, Bitu regbase, op_type* op_pt);
static void change_vibrato(OPL_DATA* chip, Bitu regbase, op_type* op_pt);
static void change_feedback(OPL_DATA* chip, Bitu chanbase, op_type* op_pt);

// general functions
/*void* adlib_init(Bitu clock, Bit32u samplerate);
void adlib_writeIO(void *chip, Bitu addr, Bit8u val);
void adlib_write(void *chip, Bitu idx, Bit8u val);
//void adlib_getsample(Bit16s* sndptr, Bits numsamples);
void adlib_getsample(void *chip, Bit32s** sndptr, Bits numsamples);

Bitu adlib_reg_read(void *chip, Bitu port);
void adlib_write_index(void *chip, Bitu port, Bit8u val);*/
static void adlib_write(void *chip, Bitu idx, Bit8u val);
