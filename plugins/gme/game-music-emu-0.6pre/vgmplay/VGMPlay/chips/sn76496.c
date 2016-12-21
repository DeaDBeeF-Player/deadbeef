/***************************************************************************

  sn76496.c
  by Nicola Salmoria
  with contributions by others

  Routines to emulate the:
  Texas Instruments SN76489, SN76489A, SN76494/SN76496
  ( Also known as, or at least compatible with, the TMS9919 and SN94624.)
  and the Sega 'PSG' used on the Master System, Game Gear, and Megadrive/Genesis
  This chip is known as the Programmable Sound Generator, or PSG, and is a 4
  channel sound generator, with three squarewave channels and a noise/arbitrary
  duty cycle channel.

  Noise emulation for all verified chips should be accurate:

  ** SN76489 uses a 15-bit shift register with taps on bits D and E, output on E,
  XOR function.
  It uses a 15-bit ring buffer for periodic noise/arbitrary duty cycle.
  Its output is inverted.
  ** SN94624 is the same as SN76489 but lacks the /8 divider on its clock input.
  ** SN76489A uses a 15-bit shift register with taps on bits D and E, output on F,
  XOR function.
  It uses a 15-bit ring buffer for periodic noise/arbitrary duty cycle.
  Its output is not inverted.
  ** SN76494 is the same as SN76489A but lacks the /8 divider on its clock input.
  ** SN76496 is identical in operation to the SN76489A, but the audio input is
  documented.
  All the TI-made PSG chips have an audio input line which is mixed with the 4 channels
  of output. (It is undocumented and may not function properly on the sn76489, 76489a
  and 76494; the sn76489a input is mentioned in datasheets for the tms5200)
  All the TI-made PSG chips act as if the frequency was set to 0x400 if 0 is
  written to the frequency register.
  ** Sega Master System III/MD/Genesis PSG uses a 16-bit shift register with taps
  on bits C and F, output on F
  It uses a 16-bit ring buffer for periodic noise/arbitrary duty cycle.
  (whether it uses an XOR or XNOR needs to be verified, assumed XOR)
  (whether output is inverted or not needs to be verified, assumed to be inverted)
  ** Sega Game Gear PSG is identical to the SMS3/MD/Genesis one except it has an
  extra register for mapping which channels go to which speaker.
  The register, connected to a z80 port, means:
  for bits 7  6  5  4  3  2  1  0
           L3 L2 L1 L0 R3 R2 R1 R0
  Noise is an XOR function, and audio output is negated before being output.
  All the Sega-made PSG chips act as if the frequency was set to 0 if 0 is written
  to the frequency register.
  ** NCR7496 (as used on the Tandy 1000) is similar to the SN76489 but with a
  different noise LFSR patttern: taps on bits A and E, output on E
  It uses a 15-bit ring buffer for periodic noise/arbitrary duty cycle.
  (all this chip's info needs to be verified)

  28/03/2005 : Sebastien Chevalier
  Update th SN76496Write func, according to SN76489 doc found on SMSPower.
   - On write with 0x80 set to 0, when LastRegister is other then TONE,
   the function is similar than update with 0x80 set to 1

  23/04/2007 : Lord Nightmare
  Major update, implement all three different noise generation algorithms and a
  set_variant call to discern among them.

  28/04/2009 : Lord Nightmare
  Add READY line readback; cleaned up struct a bit. Cleaned up comments.
  Add more TODOs. Fixed some unsaved savestate related stuff.

  04/11/2009 : Lord Nightmare
  Changed the way that the invert works (it now selects between XOR and XNOR
  for the taps), and added R->OldNoise to simulate the extra 0 that is always
  output before the noise LFSR contents are after an LFSR reset.
  This fixes SN76489/A to match chips. Added SN94624.

  14/11/2009 : Lord Nightmare
  Removed STEP mess, vastly simplifying the code. Made output bipolar rather
  than always above the 0 line, but disabled that code due to pending issues.

  16/11/2009 : Lord Nightmare
  Fix screeching in regulus: When summing together four equal channels, the
  size of the max amplitude per channel should be 1/4 of the max range, not
  1/3. Added NCR7496.

  18/11/2009 : Lord Nightmare
  Modify Init functions to support negating the audio output. The gamegear
  psg does this. Change gamegear and sega psgs to use XOR rather than XNOR
  based on testing. Got rid of R->OldNoise and fixed taps accordingly.
  Added stereo support for game gear.

  15/01/2010 : Lord Nightmare
  Fix an issue with SN76489 and SN76489A having the wrong periodic noise periods.
  Note that properly emulating the noise cycle bit timing accurately may require
  extensive rewriting.

  24/01/2010: Lord Nightmare
  Implement periodic noise as forcing one of the XNOR or XOR taps to 1 or 0 respectively.
  Thanks to PlgDavid for providing samples which helped immensely here.
  Added true clock divider emulation, so sn94624 and sn76494 run 8x faster than
  the others, as in real life.

  15/02/2010: Lord Nightmare & Michael Zapf (additional testing by PlgDavid)
  Fix noise period when set to mirror channel 3 and channel 3 period is set to 0 (tested on hardware for noise, wave needs tests) - MZ
  Fix phase of noise on sn94624 and sn76489; all chips use a standard XOR, the only inversion is the output itself - LN, Plgdavid
  Thanks to PlgDavid and Michael Zapf for providing samples which helped immensely here.

  23/02/2011: Lord Nightmare & Enik
  Made it so the Sega PSG chips have a frequency of 0 if 0 is written to the
  frequency register, while the others have 0x400 as before. Should fix a bug
  or two on sega games, particularly Vigilante on Sega Master System. Verified
  on SMS hardware.

  TODO: * Implement the TMS9919 - any difference to sn94624?
        * Implement the T6W28; has registers in a weird order, needs writes
          to be 'sanitized' first. Also is stereo, similar to game gear.
        * Test the NCR7496; Smspower says the whitenoise taps are A and E,
          but this needs verification on real hardware.
        * Factor out common code so that the SAA1099 can share some code.
        * Convert to modern device
***************************************************************************/

/* Note: I patched the core to speed the emulation up (factor 8!!)
	My Pentium2 233MHz was too slow for two SN76496 chips in release mode!
	Now a 2xSN76496 vgm takes about 45 % CPU. */

#include "mamedef.h"
#ifdef _DEBUG
#include <stdio.h>
#endif
//#include "emu.h"
//#include "streams.h"
#include <string.h>
#include <stdlib.h>
#include "sn76496.h"

#ifndef NULL
#define NULL	((void *)0)
#endif


//#define MAX_OUTPUT 0x7fff
#define MAX_OUTPUT 0x8000
#define NOISEMODE (R->Register[6]&4)?1:0


typedef struct _sn76496_state sn76496_state;
struct _sn76496_state
{
	//sound_stream * Channel;
	INT32 VolTable[16];	/* volume table (for 4-bit to db conversion)*/
	INT32 Register[8];	/* registers */
	INT32 LastRegister;	/* last register written */
	INT32 Volume[4];	/* db volume of voice 0-2 and noise */
	UINT32 RNG;			/* noise generator LFSR*/
	INT32 ClockDivider;	/* clock divider */
	INT32 CurrentClock;
	INT32 FeedbackMask;	/* mask for feedback */
	INT32 WhitenoiseTap1;	/* mask for white noise tap 1 (higher one, usually bit 14) */
	INT32 WhitenoiseTap2;	/* mask for white noise tap 2 (lower one, usually bit 13)*/
	INT32 Negate;		/* output negate flag */
	INT32 Stereo;		/* whether we're dealing with stereo or not */
	INT32 StereoMask;	/* the stereo output mask */
	INT32 Period[4];	/* Length of 1/2 of waveform */
	INT32 Count[4];		/* Position within the waveform */
	INT32 Output[4];	/* 1-bit output of each channel, pre-volume */
	INT32 CyclestoREADY;/* number of cycles until the READY line goes active */
	INT32 Freq0IsMax;	/* flag for if frequency zero acts as if it is one more than max (0x3ff+1) or if it acts like 0 */
	UINT32 MuteMsk[4];
	UINT8 NgpFlags;		/* bit 7 - NGP Mode on/off, bit 0 - is 2nd NGP chip */
	sn76496_state* NgpChip2;	/* Pointer to other Chip */
};


static sn76496_state* LastChipInit = NULL;
static unsigned short int FNumLimit;

/*INLINE sn76496_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == SN76496 ||
		   device->type() == SN76489 ||
		   device->type() == SN76489A ||
		   device->type() == SN76494 ||
		   device->type() == SN94624 ||
		   device->type() == NCR7496 ||
		   device->type() == GAMEGEAR ||
		   device->type() == SMSIII);
	return (sn76496_state *)downcast<legacy_device_base *>(device)->token();
}*/

//READ_LINE_DEVICE_HANDLER( sn76496_ready_r )
UINT8 sn76496_ready_r(void *chip, offs_t offset)
{
	//sn76496_state *R = get_safe_token(device);
	sn76496_state *R = (sn76496_state*)chip;
	//stream_update(R->Channel);
	return (R->CyclestoREADY? 0 : 1);
}

//WRITE8_DEVICE_HANDLER( sn76496_stereo_w )
void sn76496_stereo_w(void *chip, offs_t offset, UINT8 data)
{
	//sn76496_state *R = get_safe_token(device);
	sn76496_state *R = (sn76496_state*)chip;
	//stream_update(R->Channel);
	if (R->Stereo) R->StereoMask = data;
#ifdef _DEBUG
	else logerror("Call to stereo write with mono chip!\n");
#endif
}

//WRITE8_DEVICE_HANDLER( sn76496_w )
void sn76496_write_reg(void *chip, offs_t offset, UINT8 data)
{
	//sn76496_state *R = get_safe_token(device);
	sn76496_state *R = (sn76496_state*)chip;
	int n, r, c;


	/* update the output buffer before changing the registers */
	//stream_update(R->Channel);

	/* set number of cycles until READY is active; this is always one
           'sample', i.e. it equals the clock divider exactly; until the
           clock divider is fully supported, we delay until one sample has
           played. The fact that this below is '2' and not '1' is because
           of a ?race condition? in the mess crvision driver, where after
           any sample is played at all, no matter what, the cycles_to_ready
           ends up never being not ready, unless this value is greater than
           1. Once the full clock divider stuff is written, this should no
           longer be an issue. */
	R->CyclestoREADY = 2;

	if (data & 0x80)
	{
		r = (data & 0x70) >> 4;
		R->LastRegister = r;
		R->Register[r] = (R->Register[r] & 0x3f0) | (data & 0x0f);
	}
	else
    {
		r = R->LastRegister;
	}
	c = r/2;
	switch (r)
	{
		case 0:	/* tone 0 : frequency */
		case 2:	/* tone 1 : frequency */
		case 4:	/* tone 2 : frequency */
		    if ((data & 0x80) == 0) R->Register[r] = (R->Register[r] & 0x0f) | ((data & 0x3f) << 4);
			if ((R->Register[r] != 0) || (R->Freq0IsMax == 0)) R->Period[c] = R->Register[r];
			else R->Period[c] = 0x400;
			if (r == 4)
			{
				/* update noise shift frequency */
				if ((R->Register[6] & 0x03) == 0x03)
					R->Period[3] = 2 * R->Period[2];
			}
			break;
		case 1:	/* tone 0 : volume */
		case 3:	/* tone 1 : volume */
		case 5:	/* tone 2 : volume */
		case 7:	/* noise  : volume */
			R->Volume[c] = R->VolTable[data & 0x0f];
			if ((data & 0x80) == 0) R->Register[r] = (R->Register[r] & 0x3f0) | (data & 0x0f);
			
		//	// "Every volume write resets the waveform to High level.", TmEE, 2012-11-24 on SMSPower
		//	R->Output[c] = 1;
		//	R->Count[c] = R->Period[c];
		//	disabled for now - sounds awful
			break;
		case 6:	/* noise  : frequency, mode */
			{
#ifdef _DEBUG
				//if ((data & 0x80) == 0) logerror("sn76489: write to reg 6 with bit 7 clear; data was %03x, new write is %02x! report this to LN!\n", R->Register[6], data);
#endif
				if ((data & 0x80) == 0) R->Register[r] = (R->Register[r] & 0x3f0) | (data & 0x0f);
				n = R->Register[6];
				/* N/512,N/1024,N/2048,Tone #3 output */
				R->Period[3] = ((n&3) == 3) ? 2 * R->Period[2] : (1 << (5+(n&3)));
				R->RNG = R->FeedbackMask;
			}
			break;
	}
}

//static STREAM_UPDATE( SN76496Update )
void SN76496Update(void *chip, stream_sample_t **outputs, int samples)
{
	int i;
	//sn76496_state *R = (sn76496_state *)param;
	sn76496_state *R = (sn76496_state*)chip;
	sn76496_state *R2;
	stream_sample_t *lbuffer = outputs[0];
	//stream_sample_t *rbuffer = (R->Stereo)?outputs[1]:NULL;
	stream_sample_t *rbuffer = outputs[1];
	INT32 out = 0;
	INT32 out2 = 0;
	INT32 vol[4];
	UINT8 NGPMode;
	INT32 ggst[2];

	NGPMode = (R->NgpFlags >> 7) & 0x01;
	if (NGPMode)
		R2 = R->NgpChip2;

	if (! NGPMode)
	{
		// Speed Hack
		out = 0;
		for (i = 0; i < 3; i ++)
		{
			if (R->Period[i] || R->Volume[i])
			{
				out = 1;
				break;
			}
		}
		if (R->Volume[3])
			out = 1;
		if (! out)
		{
			memset(lbuffer, 0x00, sizeof(stream_sample_t) * samples);
			memset(rbuffer, 0x00, sizeof(stream_sample_t) * samples);
			return;
		}
	}
	
	ggst[0] = 0x01;
	ggst[1] = 0x01;
	while (samples > 0)
	{
		/* Speed Patch */
		/*// clock chip once
		if (R->CurrentClock > 0) // not ready for new divided clock
		{
			R->CurrentClock--;
		}
		else // ready for new divided clock, make a new sample
		{
			R->CurrentClock = R->ClockDivider-1;*/
			/* decrement Cycles to READY by one */
			if (R->CyclestoREADY >0) R->CyclestoREADY--;

			// handle channels 0,1,2
			for (i = 0;i < 3;i++)
			{
				R->Count[i]--;
				if (R->Count[i] <= 0)
				{
					R->Output[i] ^= 1;
					R->Count[i] = R->Period[i];
				}
			}

			// handle channel 3
			R->Count[3]--;
			if (R->Count[3] <= 0)
			{
			// if noisemode is 1, both taps are enabled
			// if noisemode is 0, the lower tap, whitenoisetap2, is held at 0
				if (((R->RNG & R->WhitenoiseTap1)?1:0) ^ ((((R->RNG & R->WhitenoiseTap2)?1:0))*(NOISEMODE)))
				{
					R->RNG >>= 1;
					R->RNG |= R->FeedbackMask;
				}
				else
				{
					R->RNG >>= 1;
				}
				R->Output[3] = R->RNG & 1;

				R->Count[3] = R->Period[3];
			}
		//}


		/*if (R->Stereo)
		{
			out = (((R->StereoMask&0x10)&&R->Output[0])?R->Volume[0]:0)
				+ (((R->StereoMask&0x20)&&R->Output[1])?R->Volume[1]:0)
				+ (((R->StereoMask&0x40)&&R->Output[2])?R->Volume[2]:0)
				+ (((R->StereoMask&0x80)&&R->Output[3])?R->Volume[3]:0);

			out2 = (((R->StereoMask&0x1)&&R->Output[0])?R->Volume[0]:0)
				+ (((R->StereoMask&0x2)&&R->Output[1])?R->Volume[1]:0)
				+ (((R->StereoMask&0x4)&&R->Output[2])?R->Volume[2]:0)
				+ (((R->StereoMask&0x8)&&R->Output[3])?R->Volume[3]:0);
		}
		else
		{
			out = (R->Output[0]?R->Volume[0]:0)
				+(R->Output[1]?R->Volume[1]:0)
				+(R->Output[2]?R->Volume[2]:0)
				+(R->Output[3]?R->Volume[3]:0);
		}*/

		// --- CUSTOM CODE START --
		out = out2 = 0;
		if (! R->NgpFlags)
		{
			for (i = 0; i < 4; i ++)
			{
				// --- Preparation Start ---
				// Bipolar output
				vol[i] = R->Output[i] ? +1 : -1;
				
				// Disable high frequencies (> SampleRate / 2) for tone channels
				// Freq. 0/1 isn't disabled becaus it would also disable PCM
				if (i != 3)
				{
					if (R->Period[i] <= FNumLimit && R->Period[i] > 1)
						vol[i] = 0;
				}
				vol[i] &= R->MuteMsk[i];
				// --- Preparation End ---
				
				if (R->Stereo)
				{
					ggst[0] = (R->StereoMask & (0x10 << i)) ? 0x01 : 0x00;
					ggst[1] = (R->StereoMask & (0x01 << i)) ? 0x01 : 0x00;
				}
				if (R->Period[i] > 1 || i == 3)
				{
					out += vol[i] * R->Volume[i] * ggst[0];
					out2 += vol[i] * R->Volume[i] * ggst[1];
				}
				else if (R->MuteMsk[i])
				{
					// Make Bipolar Output with PCM possible
					//out += (2 * R->Volume[i] - R->VolTable[5]) * ggst[0];
					//out2 += (2 * R->Volume[i] - R->VolTable[5]) * ggst[1];
					out += R->Volume[i] * ggst[0];
					out2 += R->Volume[i] * ggst[1];
				}
			}
		}
		else
		{
			if (! (R->NgpFlags & 0x01))
			{
				// Tone Channel 1-3
				if (R->Stereo)
				{
					ggst[0] = (R->StereoMask & (0x10 << i)) ? 0x01 : 0x00;
					ggst[1] = (R->StereoMask & (0x01 << i)) ? 0x01 : 0x00;
				}
				for (i = 0; i < 3; i ++)
				{
					// --- Preparation Start ---
					// Bipolar output
					vol[i] = R->Output[i] ? +1 : -1;
					
					// Disable high frequencies (> SampleRate / 2) for tone channels
					// Freq. 0 isn't disabled becaus it would also disable PCM
					if (R->Period[i] <= FNumLimit && R->Period[i])
						vol[i] = 0;
					vol[i] &= R->MuteMsk[i];
					// --- Preparation End ---
					
					//out += vol[i] * R->Volume[i];
					//out2 += vol[i] * R2->Volume[i];
					if (R->Period[i])
					{
						out += vol[i] * R->Volume[i] * ggst[0];
						out2 += vol[i] * R2->Volume[i] * ggst[1];
					}
					else if (R->MuteMsk[i])
					{
						// Make Bipolar Output with PCM possible
						out += R->Volume[i] * ggst[0];
						out2 += R2->Volume[i] * ggst[1];
					}
				}
			}
			else
			{
				// --- Preparation Start ---
				// Bipolar output
				vol[i] = R->Output[i] ? +1 : -1;
				
				//vol[i] &= R->MuteMsk[i];
				vol[i] &= R2->MuteMsk[i];	// use MuteMask from chip 0
				// --- Preparation End ---
				
				// Noise Channel
				if (R->Stereo)
				{
					ggst[0] = (R->StereoMask & 0x80) ? 0x01 : 0x00;
					ggst[1] = (R->StereoMask & 0x08) ? 0x01 : 0x00;
				}
				else
				{
					ggst[0] = 0x01;
					ggst[1] = 0x01;
				}
				//out += vol[3] * R2->Volume[3];
				//out2 += vol[3] * R->Volume[3];
				out += vol[3] * R2->Volume[3] * ggst[0];
				out2 += vol[3] * R->Volume[3] * ggst[1];
			}
		}
		// --- CUSTOM CODE END --
		
		if(R->Negate) { out = -out; out2 = -out2; }

		*(lbuffer++) = out >> 1;	// Output is Bipolar
		//if (R->Stereo) *(rbuffer++) = out2;
		*(rbuffer++) = out2 >> 1;
		samples--;
	}
}



static void SN76496_set_gain(sn76496_state *R,int gain)
{
	int i;
	double out;


	gain &= 0xff;

	/* increase max output basing on gain (0.2 dB per step) */
	out = MAX_OUTPUT / 4; // four channels, each gets 1/4 of the total range
	while (gain-- > 0)
		out *= 1.023292992;	/* = (10 ^ (0.2/20)) */

	/* build volume table (2dB per step) */
	for (i = 0;i < 15;i++)
	{
		/* limit volume to avoid clipping */
		if (out > MAX_OUTPUT / 4) R->VolTable[i] = MAX_OUTPUT / 4;
		//else R->VolTable[i] = out;
		else R->VolTable[i] = (INT32)(out + 0.5);	// I like rounding

		out /= 1.258925412;	/* = 10 ^ (2/20) = 2dB */
	}
	R->VolTable[15] = 0;
}



//static int SN76496_init(running_device *device, sn76496_state *R, int stereo)
static int SN76496_init(int clock, sn76496_state *R, int stereo)
{
	int sample_rate = clock/2;
	int i;

	//R->Channel = stream_create(device,0,(stereo?2:1),sample_rate,R,SN76496Update);

	for (i = 0;i < 4;i++) R->Volume[i] = 0;

	R->LastRegister = 0;
	for (i = 0;i < 8;i+=2)
	{
		R->Register[i] = 0;
		R->Register[i + 1] = 0x0f;	/* volume = 0 */
	}

	for (i = 0;i < 4;i++)
	{
		R->Output[i] = R->Period[i] = R->Count[i] = 0;
		R->MuteMsk[i] = ~0x00;
	}

	/* Default is SN76489A */
	R->ClockDivider = 8;
	R->FeedbackMask = 0x10000;     /* mask for feedback */
	R->WhitenoiseTap1 = 0x04;   /* mask for white noise tap 1*/
	R->WhitenoiseTap2 = 0x08;   /* mask for white noise tap 2*/
	R->Negate = 0; /* channels are not negated */
	R->Stereo = stereo; /* depends on init */
	R->CyclestoREADY = 1; /* assume ready is not active immediately on init. is this correct?*/
	R->StereoMask = 0xFF; /* all channels enabled */
	R->Freq0IsMax = 1; /* frequency set to 0 results in freq = 0x400 rather than 0 */

	R->RNG = R->FeedbackMask;
	R->Output[3] = R->RNG & 1;

	R->NgpFlags = 0x00;
	R->NgpChip2 = NULL;

	//return 0;
	return sample_rate;
}


//static void generic_start(running_device *device, int feedbackmask, int noisetap1, int noisetap2, int negate, int stereo, int clockdivider, int freq0)
static int generic_start(sn76496_state *chip, int clock, int feedbackmask, int noisetap1, int noisetap2, int negate, int stereo, int clockdivider, int freq0)
{
	int sample_rate;
	
	//sn76496_state *chip = get_safe_token(device);
	//sn76496_state *chip;
	sn76496_state *chip2;
	
	//if (SN76496_init(device,chip,stereo) != 0)
	//	fatalerror("Error creating SN76496 chip");
	sample_rate = SN76496_init(clock & 0x7FFFFFFF, chip, stereo);
	if ((clock & 0x80000000) && LastChipInit != NULL)
	{
		// Activate special NeoGeoPocket Mode
		chip2 = LastChipInit;
		chip2->NgpFlags = 0x80 | 0x00;
		chip->NgpFlags = 0x80 | 0x01;
		chip->NgpChip2 = chip2;
		chip2->NgpChip2 = chip;
		LastChipInit = NULL;
	}
	else
	{
		LastChipInit = chip;
	}
	SN76496_set_gain(chip, 0);
	
	chip->FeedbackMask = feedbackmask;
	chip->WhitenoiseTap1 = noisetap1;
	chip->WhitenoiseTap2 = noisetap2;
	chip->Negate = negate;
	chip->Stereo = stereo;
	if (clockdivider)
		chip->ClockDivider = clockdivider;
	chip->CurrentClock = clockdivider-1;
	chip->Freq0IsMax = freq0;
	
	/* Speed Patch*/
	sample_rate /= chip->ClockDivider;
	
	/*state_save_register_device_item_array(device, 0, chip->VolTable);
	state_save_register_device_item_array(device, 0, chip->Register);
	state_save_register_device_item(device, 0, chip->LastRegister);
	state_save_register_device_item_array(device, 0, chip->Volume);
	state_save_register_device_item(device, 0, chip->RNG);
	state_save_register_device_item(device, 0, chip->ClockDivider);
	state_save_register_device_item(device, 0, chip->CurrentClock);
	state_save_register_device_item(device, 0, chip->FeedbackMask);
	state_save_register_device_item(device, 0, chip->WhitenoiseTap1);
	state_save_register_device_item(device, 0, chip->WhitenoiseTap2);
	state_save_register_device_item(device, 0, chip->Negate);
	state_save_register_device_item(device, 0, chip->Stereo);
	state_save_register_device_item(device, 0, chip->StereoMask);
	state_save_register_device_item_array(device, 0, chip->Period);
	state_save_register_device_item_array(device, 0, chip->Count);
	state_save_register_device_item_array(device, 0, chip->Output);
	state_save_register_device_item(device, 0, chip->CyclestoREADY);*/
	
	return sample_rate;
}

unsigned long int sn76496_start(void **chip, int clock, int shiftregwidth, int noisetaps,
								int negate, int stereo, int clockdivider, int freq0)
{
	sn76496_state* sn_chip;
	int ntap[2];
	int curbit;
	int curtap;
	
	sn_chip = (sn76496_state*)malloc(sizeof(sn76496_state));
	if (sn_chip == NULL)
		return 0;
	memset(sn_chip, 0x00, sizeof(sn76496_state));
	*chip = sn_chip;
	
	// extract single noise tap bits
	curtap = 0;
	for (curbit = 0; curbit < 16; curbit ++)
	{
		if (noisetaps & (1 << curbit))
		{
			ntap[curtap] = (1 << curbit);
			curtap ++;
			if (curtap >= 2)
				break;
		}
	}
	while(curtap < 2)
	{
		ntap[curtap] = ntap[0];
		curtap ++;
	}
	
	return generic_start(sn_chip, clock, 1 << (shiftregwidth - 1), ntap[0], ntap[1],
						negate, ! stereo, clockdivider ? 1 : 8, freq0);
}

void sn76496_shutdown(void *chip)
{
	sn76496_state *R = (sn76496_state*)chip;
	
	free(R);
	return;
}

void sn76496_reset(void *chip)
{
	sn76496_state *R;
	UINT8 i;
	
	R = (sn76496_state*)chip;
	
	for (i = 0;i < 4;i++) R->Volume[i] = 0;

	R->LastRegister = 0;
	for (i = 0;i < 8;i+=2)
	{
		R->Register[i] = 0;
		R->Register[i + 1] = 0x0f;	/* volume = 0 */
	}

	for (i = 0;i < 4;i++)
	{
		R->Output[i] = R->Period[i] = R->Count[i] = 0;
	}

	R->CyclestoREADY = 1;
	R->StereoMask = 0xFF; /* all channels enabled */

	R->RNG = R->FeedbackMask;
	R->Output[3] = R->RNG & 1;
	
	return;
}

void sn76496_freq_limiter(int clock, int clockdiv, int sample_rate)
{
	FNumLimit = (unsigned short int)((clock / (clockdiv ? 2.0 : 16.0)) / sample_rate);
	
	return;
}

void sn76496_set_mutemask(void *chip, UINT32 MuteMask)
{
	sn76496_state *R = (sn76496_state*)chip;
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 4; CurChn ++)
		R->MuteMsk[CurChn] = (MuteMask & (1 << CurChn)) ? 0 : ~0;
	
	return;
}

// function parameters: device, feedback destination tap, feedback source taps,
// normal(false)/invert(true), mono(false)/stereo(true), clock divider factor

/*static DEVICE_START( sn76489 )
{
	generic_start(device, 0x4000, 0x01, 0x02, TRUE, FALSE, 8, TRUE); // SN76489 not verified yet. todo: verify;
}

static DEVICE_START( sn76489a )
{
	generic_start(device, 0x10000, 0x04, 0x08, FALSE, FALSE, 8, TRUE); // SN76489A: whitenoise verified, phase verified, periodic verified (by plgdavid)
}

static DEVICE_START( sn76494 )
{
	generic_start(device, 0x10000, 0x04, 0x08, FALSE, FALSE, 1, TRUE); // SN76494 not verified, (according to datasheet: same as sn76489a but without the /8 divider)
}

static DEVICE_START( sn76496 )
{
	generic_start(device, 0x10000, 0x04, 0x08, FALSE, FALSE, 8, TRUE); // SN76496: Whitenoise verified, phase verified, periodic verified (by Michael Zapf)
}

static DEVICE_START( sn94624 )
{
	generic_start(device, 0x4000, 0x01, 0x02, TRUE, FALSE, 1, TRUE); // SN94624 whitenoise verified, phase verified, period verified; verified by PlgDavid
}

static DEVICE_START( ncr7496 )
{
	generic_start(device, 0x8000, 0x02, 0x20, FALSE, FALSE, 8, TRUE); // NCR7496 not verified; info from smspower wiki
}

static DEVICE_START( gamegear )
{
	generic_start(device, 0x8000, 0x01, 0x08, TRUE, TRUE, 8, FALSE); // Verified by Justin Kerk
}

static DEVICE_START( smsiii )
{
	generic_start(device, 0x8000, 0x01, 0x08, TRUE, FALSE, 8, FALSE); // todo: verify; from smspower wiki, assumed to have same invert as gamegear
}*/


/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( sn76496 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(sn76496_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( sn76496 );		break;
		case DEVINFO_FCT_STOP:							// Nothing										break;
		case DEVINFO_FCT_RESET:							// Nothing										break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "SN76496");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "TI PSG");						break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.1");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}

DEVICE_GET_INFO( sn76489 )
{
	switch (state)
	{
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( sn76489 );		break;
		case DEVINFO_STR_NAME:							strcpy(info->s, "SN76489");						break;
		default:										DEVICE_GET_INFO_CALL(sn76496);						break;
	}
}

DEVICE_GET_INFO( sn76489a )
{
	switch (state)
	{
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( sn76489a );		break;
		case DEVINFO_STR_NAME:							strcpy(info->s, "SN76489A");					break;
		default:										DEVICE_GET_INFO_CALL(sn76496);						break;
	}
}

DEVICE_GET_INFO( sn76494 )
{
	switch (state)
	{
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( sn76494 );		break;
		case DEVINFO_STR_NAME:							strcpy(info->s, "SN76494");						break;
		default:										DEVICE_GET_INFO_CALL(sn76496);						break;
	}
}

DEVICE_GET_INFO( sn94624 )
{
	switch (state)
	{
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( sn94624 );		break;
		case DEVINFO_STR_NAME:							strcpy(info->s, "SN94624");						break;
		default:										DEVICE_GET_INFO_CALL(sn76496);						break;
	}
}

DEVICE_GET_INFO( ncr7496 )
{
	switch (state)
	{
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( ncr7496 );		break;
		case DEVINFO_STR_NAME:							strcpy(info->s, "NCR7496");						break;
		default:										DEVICE_GET_INFO_CALL(sn76496);						break;
	}
}

DEVICE_GET_INFO( gamegear )
{
	switch (state)
	{
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( gamegear );		break;
		case DEVINFO_STR_NAME:							strcpy(info->s, "Game Gear PSG");				break;
		default:										DEVICE_GET_INFO_CALL(sn76496);						break;
	}
}

DEVICE_GET_INFO( smsiii )
{
	switch (state)
	{
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( smsiii );			break;
		case DEVINFO_STR_NAME:							strcpy(info->s, "SMSIII PSG");					break;
		default:										DEVICE_GET_INFO_CALL(sn76496);						break;
	}
}*/


/*DEFINE_LEGACY_SOUND_DEVICE(SN76496, sn76496);
DEFINE_LEGACY_SOUND_DEVICE(SN76489, sn76489);
DEFINE_LEGACY_SOUND_DEVICE(SN76489A, sn76489a);
DEFINE_LEGACY_SOUND_DEVICE(SN76494, sn76494);
DEFINE_LEGACY_SOUND_DEVICE(SN94624, sn94624);
DEFINE_LEGACY_SOUND_DEVICE(NCR7496, ncr7496);
DEFINE_LEGACY_SOUND_DEVICE(GAMEGEAR, gamegear);
DEFINE_LEGACY_SOUND_DEVICE(SMSIII, smsiii);*/
