#ifndef EMU2413_H
#define EMU2413_H

typedef unsigned char  e_uint8;
typedef signed short   e_int16;
typedef unsigned short e_uint16;
typedef signed int     e_int32;
typedef unsigned int   e_uint32;

// Size of Sintable ( 8 -- 18 can be used. 9 recommended.)
#define PG_BITS 9              
#define PG_WIDTH (1<<PG_BITS)

// Phase increment counter
#define DP_BITS 18
#define DP_WIDTH (1<<DP_BITS)
#define DP_BASE_BITS (DP_BITS - PG_BITS)
														
// Dynamic range (Accuracy of sin table)
#define DB_BITS 8
#define DB_STEP (48.0/(1<<DB_BITS))
#define DB_MUTE (1<<DB_BITS)

// Dynamic range of envelope
#define EG_STEP 0.375
#define EG_BITS 7
#define EG_MUTE (1<<EG_BITS)

// Dynamic range of total level
#define TL_STEP 0.75
#define TL_BITS 6   
#define TL_MUTE (1<<TL_BITS)

// Dynamic range of sustine level
#define SL_STEP 3.0
#define SL_BITS 4  
#define SL_MUTE (1<<SL_BITS)

// Bits for Pitch and Amp modulator
#define PM_PG_BITS 8
#define PM_PG_WIDTH (1<<PM_PG_BITS)
#define PM_DP_BITS 16
#define PM_DP_WIDTH (1<<PM_DP_BITS)
#define AM_PG_BITS 8
#define AM_PG_WIDTH (1<<AM_PG_BITS)
#define AM_DP_BITS 16
#define AM_DP_WIDTH (1<<AM_DP_BITS)

struct OPLL_PATCH
{
	e_uint32 TL,FB_shift,EG,ML,AR,DR,SL,RR,KR,KL,AM,PM,WF;
};

struct OPLL_SLOT
{
	// OUTPUT
	e_int32 feedback;
	e_int32 output[2];   // Output value of slot

	// for Phase Generator (PG)
	e_uint16 const* sintbl;    // Wavetable
	e_uint32 phase;      // Phase
	e_uint32 dphase;     // Phase increment amount
	e_uint32 pgout;      // output

	// for Envelope Generator (EG)
	e_int32 fnum;          // F-Number
	e_int32 block;         // Block
	e_int32 volume;        // Current volume
	e_int32 sustine;       // Sustine 1 = ON, 0 = OFF
	e_uint32 tll;	      // Total Level + Key scale level
	e_uint32 rks;        // Key scale offset (Rks)
	e_int32 eg_mode;       // Current state
	e_uint32 eg_phase;   // Phase
	e_uint32 eg_dphase;  // Phase increment amount
	e_uint32 egout;      // output

	OPLL_PATCH patch;
	e_uint8 slot_on_flag;
	e_uint8 filler [7];
};

#define OPLL_MASK_CH(x) (1<<(x))

struct OPLL
{
	OPLL_SLOT slot [6 * 2];

	// Register
	e_uint8 LowFreq [6];
	e_uint8 HiFreq  [6];
	e_uint8 InstVol [6];

	// Channel Data
	e_uint8 patch_number [6];
	
	e_uint8 CustInst [8];
	
	// LFO
	e_uint32 pm_phase;
	e_uint32 am_phase;
	
	e_uint32 pm_dphase;
	e_uint32 am_dphase;
	
	e_int16 pmtable [PM_PG_WIDTH];
	e_uint8 amtable [AM_PG_WIDTH];
		
	e_uint16 sintable [2] [PG_WIDTH]; // [0] = full, [1] = half

	// dB to Liner table
	e_int16 DB2LIN_TABLE [4 * DB_MUTE];  

	// Liner to Log curve conversion table (for Attack rate).
	e_uint8 AR_ADJUST_TABLE [1 << EG_BITS];

	// Phase incr table for Attack
	e_uint32 dphaseARTable [16] [16];

	// Phase incr table for Decay and Release
	e_uint32 dphaseDRTable [16] [16];

	// KSL + TL Table
	e_uint8 tllTable [16] [8] [1 << TL_BITS] [4];
	e_uint8 rksTable [2] [8] [2];

	// Phase incr table for PG  
	e_uint32 dphaseTable [512] [8] [16];
};

OPLL* VRC7_new( long clock_rate );
void VRC7_delete( OPLL* );
void VRC7_reset( OPLL* );

void VRC7_writeReg( OPLL*, int addr, e_uint32 data );

// Run for one clock
void VRC7_run( OPLL* );

// Generate sample for a channel
e_uint32 VRC7_calcCh( OPLL*, e_uint32 channel );

#endif
