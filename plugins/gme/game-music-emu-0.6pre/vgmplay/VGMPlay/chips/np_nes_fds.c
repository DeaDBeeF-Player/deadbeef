// Ported from NSFPlay 2.3 to VGMPlay (including C++ -> C conversion)
// by Valley Bell on 26 September 2013

#include <stdlib.h>	// for rand()
#include <string.h>	// for memset()
#include <stddef.h>	// for NULL
#include <math.h>	// for exp()
#include "mamedef.h"
#include "../stdbool.h"
#include "np_nes_fds.h"


#define DEFAULT_CLOCK	1789772.0
#define DEFAULT_RATE	44100


enum
{
	OPT_CUTOFF=0,
	OPT_4085_RESET,
	OPT_WRITE_PROTECT,
	OPT_END
};

enum
{
	EMOD=0,
	EVOL=1
};

//const int RC_BITS = 12;
#define RC_BITS		12

enum
{
	TMOD=0,
	TWAV=1
};


// 8 bit approximation of master volume
#define MASTER_VOL	(2.4 * 1223.0)	// max FDS vol vs max APU square (arbitrarily 1223)
#define	MAX_OUT		(32.0 * 63.0)	// value that should map to master vol
static const INT32 MASTER[4] = {
	(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 2.0f),
	(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 3.0f),
	(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 4.0f),
	(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 5.0f) };


// Although they were pretty much removed from any sound core in NSFPlay 2.3,
// I find this counter structure very useful.
#define COUNTER_SHIFT	24

typedef struct _Counter Counter;
struct _Counter
{
	double ratio;
	UINT32 val, step;
};
#define COUNTER_setcycle(cntr, s)	(cntr).step = (UINT32)((cntr).ratio / (s + 1))
#define COUNTER_iup(cntr)			(cntr).val += (cntr).step
#define COUNTER_value(cntr)			((cntr).val >> COUNTER_SHIFT)
#define COUNTER_init(cntr, clk, rate)							\
{																\
	(cntr).ratio = (1 << COUNTER_SHIFT) * (1.0 * clk / rate);	\
	(cntr).step = (UINT32)((cntr).ratio + 0.5);					\
	(cntr).val = 0;												\
}


typedef struct _NES_FDS NES_FDS;
struct _NES_FDS
{
	double rate, clock;
	int mask;
	INT32 sm[2];	// stereo mix
	INT32 fout;		// current output
	int option[OPT_END];

	bool master_io;
	UINT8 master_vol;
	UINT32 last_freq;	// for trackinfo
	UINT32 last_vol;	// for trackinfo

	// two wavetables
	//const enum { TMOD=0, TWAV=1 };
	INT32 wave[2][64];
	UINT32 freq[2];
	UINT32 phase[2];
	bool wav_write;
	bool wav_halt;
	bool env_halt;
	bool mod_halt;
	UINT32 mod_pos;
	UINT32 mod_write_pos;

	// two ramp envelopes
	//const enum { EMOD=0, EVOL=1 };
	bool env_mode[2];
	bool env_disable[2];
	UINT32 env_timer[2];
	UINT32 env_speed[2];
	UINT32 env_out[2];
	UINT32 master_env_speed;

	// 1-pole RC lowpass filter
	INT32 rc_accum;
	INT32 rc_k;
	INT32 rc_l;

	Counter tick_count;
	UINT32 tick_last;
};

void* NES_FDS_Create(int clock, int rate)
{
	NES_FDS* fds;

	fds = (NES_FDS*)malloc(sizeof(NES_FDS));
	if (fds == NULL)
		return NULL;
	memset(fds, 0x00, sizeof(NES_FDS));

	fds->option[OPT_CUTOFF] = 2000;
	fds->option[OPT_4085_RESET] = 0;
	fds->option[OPT_WRITE_PROTECT] = 0;	// not used here, see nsfplay.cpp

	fds->rc_k = 0;
	fds->rc_l = (1<<RC_BITS);

	//NES_FDS_SetClock(fds, DEFAULT_CLOCK);
	//NES_FDS_SetRate(fds, DEFAULT_RATE);
	NES_FDS_SetClock(fds, clock);
	NES_FDS_SetRate(fds, rate);
	fds->sm[0] = 128;
	fds->sm[1] = 128;

	NES_FDS_Reset(fds);

	return fds;
}

void NES_FDS_Destroy(void* chip)
{
	free(chip);
}

void NES_FDS_SetMask(void* chip, int m)
{
	NES_FDS* fds = (NES_FDS*)chip;

	fds->mask = m&1;
}

void NES_FDS_SetStereoMix(void* chip, int trk, INT16 mixl, INT16 mixr)
{
	NES_FDS* fds = (NES_FDS*)chip;

	if (trk < 0) return;
	if (trk > 1) return;
	fds->sm[0] = mixl;
	fds->sm[1] = mixr;
}

void NES_FDS_SetClock(void* chip, double c)
{
	NES_FDS* fds = (NES_FDS*)chip;

	fds->clock = c;
}

void NES_FDS_SetRate(void* chip, double r)
{
	NES_FDS* fds = (NES_FDS*)chip;
	double cutoff, leak;

	fds->rate = r;

	COUNTER_init(fds->tick_count, fds->clock, fds->rate);
	fds->tick_last = 0;
	
	// configure lowpass filter
	cutoff = (double)fds->option[OPT_CUTOFF];
	leak = 0.0;
	if (cutoff > 0)
		leak = exp(-2.0 * 3.14159 * cutoff / fds->rate);
	fds->rc_k = (INT32)(leak * (double)(1<<RC_BITS));
	fds->rc_l = (1<<RC_BITS) - fds->rc_k;
}

void NES_FDS_SetOption(void* chip, int id, int val)
{
	NES_FDS* fds = (NES_FDS*)chip;

	if(id<OPT_END) fds->option[id] = val;

	// update cutoff immediately
	if (id == OPT_CUTOFF) NES_FDS_SetRate(fds, fds->rate);
}

void NES_FDS_Reset(void* chip)
{
	NES_FDS* fds = (NES_FDS*)chip;
	int i;

	fds->master_io = true;
	fds->master_vol = 0;
	fds->last_freq = 0;
	fds->last_vol = 0;

	fds->rc_accum = 0;

	for (i=0; i<2; ++i)
	{
		memset(fds->wave[i], 0, sizeof(fds->wave[i]));
		fds->freq[i] = 0;
		fds->phase[i] = 0;
	}
	fds->wav_write = false;
	fds->wav_halt = true;
	fds->env_halt = true;
	fds->mod_halt = true;
	fds->mod_pos = 0;
	fds->mod_write_pos = 0;

	for (i=0; i<2; ++i)
	{
		fds->env_mode[i] = false;
		fds->env_disable[i] = true;
		fds->env_timer[i] = 0;
		fds->env_speed[i] = 0;
		fds->env_out[i] = 0;
	}
	fds->master_env_speed = 0xFF;

	// NOTE: the FDS BIOS reset only does the following related to audio:
	//   $4023 = $00
	//   $4023 = $83 enables master_io
	//   $4080 = $80 output volume = 0, envelope disabled
	//   $408A = $FF master envelope speed set to slowest
	NES_FDS_Write(fds, 0x4023, 0x00);
	NES_FDS_Write(fds, 0x4023, 0x83);
	NES_FDS_Write(fds, 0x4080, 0x80);
	NES_FDS_Write(fds, 0x408A, 0xFF);

	// reset other stuff
	NES_FDS_Write(fds, 0x4082, 0x00);	// wav freq 0
	NES_FDS_Write(fds, 0x4083, 0x80);	// wav disable
	NES_FDS_Write(fds, 0x4084, 0x80);	// mod strength 0
	NES_FDS_Write(fds, 0x4085, 0x00);	// mod position 0
	NES_FDS_Write(fds, 0x4086, 0x00);	// mod freq 0
	NES_FDS_Write(fds, 0x4087, 0x80);	// mod disable
	NES_FDS_Write(fds, 0x4089, 0x00);	// wav write disable, max global volume}
}

static void Tick(NES_FDS* fds, UINT32 clocks)
{
	INT32 vol_out;

	// clock envelopes
	if (!fds->env_halt && !fds->wav_halt && (fds->master_env_speed != 0))
	{
		int i;
		
		for (i=0; i<2; ++i)
		{
			if (!fds->env_disable[i])
			{
				UINT32 period;
				
				fds->env_timer[i] += clocks;
				period = ((fds->env_speed[i]+1) * fds->master_env_speed) << 3;
				while (fds->env_timer[i] >= period)
				{
					// clock the envelope
					if (fds->env_mode[i])
					{
						if (fds->env_out[i] < 32) ++fds->env_out[i];
					}
					else
					{
						if (fds->env_out[i] > 0 ) --fds->env_out[i];
					}
					fds->env_timer[i] -= period;
				}
			}
		}
	}

	// clock the mod table
	if (!fds->mod_halt)
	{
		UINT32 start_pos, end_pos, p;

		// advance phase, adjust for modulator
		start_pos = fds->phase[TMOD] >> 16;
		fds->phase[TMOD] += (clocks * fds->freq[TMOD]);
		end_pos = fds->phase[TMOD] >> 16;

		// wrap the phase to the 64-step table (+ 16 bit accumulator)
		fds->phase[TMOD] = fds->phase[TMOD] & 0x3FFFFF;

		// execute all clocked steps
		for (p = start_pos; p < end_pos; ++p)
		{
			INT32 wv = fds->wave[TMOD][p & 0x3F];
			if (wv == 4)	// 4 resets mod position
				fds->mod_pos = 0;
			else
			{
				static const INT32 BIAS[8] = { 0, 1, 2, 4, 0, -4, -2, -1 };
				fds->mod_pos += BIAS[wv];
				fds->mod_pos &= 0x7F;	// 7-bit clamp
			}
		}
	}

	// clock the wav table
	if (!fds->wav_halt)
	{
		INT32 mod, f;

		// complex mod calculation
		mod = 0;
		if (fds->env_out[EMOD] != 0)	// skip if modulator off
		{
			// convert mod_pos to 7-bit signed
			INT32 pos = (fds->mod_pos < 64) ? fds->mod_pos : (fds->mod_pos-128);

			// multiply pos by gain,
			// shift off 4 bits but with odd "rounding" behaviour
			INT32 temp = pos * fds->env_out[EMOD];
			INT32 rem = temp & 0x0F;
			temp >>= 4;
			if ((rem > 0) && ((temp & 0x80) == 0))
			{
				if (pos < 0) temp -= 1;
				else         temp += 2;
			}

			// wrap if range is exceeded
			while (temp >= 192) temp -= 256;
			while (temp <  -64) temp += 256;

			// multiply result by pitch,
			// shift off 6 bits, round to nearest
			temp = fds->freq[TWAV] * temp;
			rem = temp & 0x3F;
			temp >>= 6;
			if (rem >= 32) temp += 1;

			mod = temp;
		}

		// advance wavetable position
		f = fds->freq[TWAV] + mod;
		fds->phase[TWAV] = fds->phase[TWAV] + (clocks * f);
		fds->phase[TWAV] = fds->phase[TWAV] & 0x3FFFFF;	// wrap

		// store for trackinfo
		fds->last_freq = f;
	}

	// output volume caps at 32
	vol_out = fds->env_out[EVOL];
	if (vol_out > 32) vol_out = 32;

	// final output
	if (!fds->wav_write)
		fds->fout = fds->wave[TWAV][(fds->phase[TWAV]>>16)&0x3F] * vol_out;

	// NOTE: during wav_halt, the unit still outputs (at phase 0)
	// and volume can affect it if the first sample is nonzero.
	// haven't worked out 100% of the conditions for volume to
	// effect (vol envelope does not seem to run, but am unsure)
	// but this implementation is very close to correct

	// store for trackinfo
	fds->last_vol = vol_out;
}

UINT32 NES_FDS_Render(void* chip, INT32 b[2])
{
	NES_FDS* fds = (NES_FDS*)chip;

/*	// 8 bit approximation of master volume
	static const double MASTER_VOL = 2.4 * 1223.0;	// max FDS vol vs max APU square (arbitrarily 1223)
	static const double MAX_OUT = 32.0f * 63.0f;	// value that should map to master vol
	static const INT32 MASTER[4] = {
		(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 2.0f),
		(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 3.0f),
		(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 4.0f),
		(INT32)((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 5.0f) };*/

	UINT32 clocks;
	INT32 v, rc_out, m;

	COUNTER_iup(fds->tick_count);
	clocks = (COUNTER_value(fds->tick_count) - fds->tick_last) & 0xFF;
	Tick(fds, clocks);
	fds->tick_last = COUNTER_value(fds->tick_count);

	v = fds->fout * MASTER[fds->master_vol] >> 8;

	// lowpass RC filter
	rc_out = ((fds->rc_accum * fds->rc_k) + (v * fds->rc_l)) >> RC_BITS;
	fds->rc_accum = rc_out;
	v = rc_out;

	// output mix
	m = fds->mask ? 0 : v;
	b[0] = (m * fds->sm[0]) >> (7-2);
	b[1] = (m * fds->sm[1]) >> (7-2);
	return 2;
}

bool NES_FDS_Write(void* chip, UINT32 adr, UINT32 val)
{
	NES_FDS* fds = (NES_FDS*)chip;

	// $4023 master I/O enable/disable
	if (adr == 0x4023)
	{
		fds->master_io = ((val & 2) != 0);
		return true;
	}

	if (!fds->master_io)
		return false;
	if (adr < 0x4040 || adr > 0x408A)
		return false;

	if (adr < 0x4080)	// $4040-407F wave table write
	{
		if (fds->wav_write)
			fds->wave[TWAV][adr - 0x4040] = val & 0x3F;
		return true;
	}

	switch (adr & 0x00FF)
	{
	case 0x80:	// $4080 volume envelope
		fds->env_disable[EVOL] = ((val & 0x80) != 0);
		fds->env_mode[EVOL] = ((val & 0x40) != 0);
		fds->env_timer[EVOL] = 0;
		fds->env_speed[EVOL] = val & 0x3F;
		if (fds->env_disable[EVOL])
			fds->env_out[EVOL] = fds->env_speed[EVOL];
		return true;
	case 0x81:	// $4081 ---
		return false;
	case 0x82:	// $4082 wave frequency low
		fds->freq[TWAV] = (fds->freq[TWAV] & 0xF00) | val;
		return true;
	case 0x83:	// $4083 wave frequency high / enables
		fds->freq[TWAV] = (fds->freq[TWAV] & 0x0FF) | ((val & 0x0F) << 8);
		fds->wav_halt = ((val & 0x80) != 0);
		fds->env_halt = ((val & 0x40) != 0);
		if (fds->wav_halt)
			fds->phase[TWAV] = 0;
		if (fds->env_halt)
		{
			fds->env_timer[EMOD] = 0;
			fds->env_timer[EVOL] = 0;
		}
		return true;
	case 0x84:	// $4084 mod envelope
		fds->env_disable[EMOD] = ((val & 0x80) != 0);
		fds->env_mode[EMOD] = ((val & 0x40) != 0);
		fds->env_timer[EMOD] = 0;
		fds->env_speed[EMOD] = val & 0x3F;
		if (fds->env_disable[EMOD])
			fds->env_out[EMOD] = fds->env_speed[EMOD];
		return true;
	case 0x85:	// $4085 mod position
		fds->mod_pos = val & 0x7F;
		// not hardware accurate., but prevents detune due to cycle inaccuracies
		// (notably in Bio Miracle Bokutte Upa)
		if (fds->option[OPT_4085_RESET])
			fds->phase[TMOD] = fds->mod_write_pos << 16;
		return true;
	case 0x86:	// $4086 mod frequency low
		fds->freq[TMOD] = (fds->freq[TMOD] & 0xF00) | val;
		return true;
	case 0x87:	// $4087 mod frequency high / enable
		fds->freq[TMOD] = (fds->freq[TMOD] & 0x0FF) | ((val & 0x0F) << 8);
		fds->mod_halt = ((val & 0x80) != 0);
		if (fds->mod_halt)
			fds->phase[TMOD] = fds->phase[TMOD] & 0x3F0000;	// reset accumulator phase
		return true;
	case 0x88:	// $4088 mod table write
		if (fds->mod_halt)
		{
			// writes to current playback position (there is no direct way to set phase)
			fds->wave[TMOD][(fds->phase[TMOD] >> 16) & 0x3F] = val & 0x7F;
			fds->phase[TMOD] = (fds->phase[TMOD] + 0x010000) & 0x3FFFFF;
			fds->wave[TMOD][(fds->phase[TMOD] >> 16) & 0x3F] = val & 0x7F;
			fds->phase[TMOD] = (fds->phase[TMOD] + 0x010000) & 0x3FFFFF;
			fds->mod_write_pos = fds->phase[TMOD] >> 16;	// used by OPT_4085_RESET
		}
		return true;
	case 0x89:	// $4089 wave write enable, master volume
		fds->wav_write = ((val & 0x80) != 0);
		fds->master_vol = val & 0x03;
		return true;
	case 0x8A:	// $408A envelope speed
		fds->master_env_speed = val;
		// haven't tested whether this register resets phase on hardware,
		// but this ensures my inplementation won't spam envelope clocks
		// if this value suddenly goes low.
		fds->env_timer[EMOD] = 0;
		fds->env_timer[EVOL] = 0;
		return true;
	default:
		return false;
	}
	return false;
}

bool NES_FDS_Read(void* chip, UINT32 adr, UINT32* val)
{
	NES_FDS* fds = (NES_FDS*)chip;

	if (adr >= 0x4040 && adr < 0x407F)
	{
		// TODO: if wav_write is not enabled, the
		// read address may not be reliable? need
		// to test this on hardware.
		*val = fds->wave[TWAV][adr - 0x4040];
		return true;
	}

	if (adr == 0x4090)	// $4090 read volume envelope
	{
		*val = fds->env_out[EVOL] | 0x40;
		return true;
	}

	if (adr == 0x4092)	// $4092 read mod envelope
	{
		*val = fds->env_out[EMOD] | 0x40;
		return true;
	}

	return false;
}
