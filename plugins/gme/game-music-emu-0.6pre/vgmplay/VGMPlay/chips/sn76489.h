#ifndef _SN76489_H_
#define _SN76489_H_

// all these defines are defined in mamedef.h, but GCC's #ifdef doesn't seem to know typedefs
/*#ifndef INT32
#define INT32 signed long
#endif
#ifndef UINT16
#define UINT16 unsigned short
#endif
#ifndef INT16
#define INT16 signed short
#endif
#ifndef INT8
#define INT8 signed char
#endif*/
#ifndef uint8
#define uint8 signed char
#endif


/*#define MAX_SN76489     4*/

/*
    More testing is needed to find and confirm feedback patterns for
    SN76489 variants and compatible chips.
*/
enum feedback_patterns {
    FB_BBCMICRO =   0x8005, /* Texas Instruments TMS SN76489N (original) from BBC Micro computer */
    FB_SC3000   =   0x0006, /* Texas Instruments TMS SN76489AN (rev. A) from SC-3000H computer */
    FB_SEGAVDP  =   0x0009, /* SN76489 clone in Sega's VDP chips (315-5124, 315-5246, 315-5313, Game Gear) */
};

enum sr_widths {
  SRW_SC3000BBCMICRO  = 15,
  SRW_SEGAVDP = 16
};

enum volume_modes {
    VOL_TRUNC   =   0,      /* Volume levels 13-15 are identical */
    VOL_FULL    =   1,      /* Volume levels 13-15 are unique */
};

enum mute_values {
    MUTE_ALLOFF =   0,      /* All channels muted */
    MUTE_TONE1  =   1,      /* Tone 1 mute control */
    MUTE_TONE2  =   2,      /* Tone 2 mute control */
    MUTE_TONE3  =   4,      /* Tone 3 mute control */
    MUTE_NOISE  =   8,      /* Noise mute control */
    MUTE_ALLON  =   15,     /* All channels enabled */
};

typedef struct
{
    int Mute; // per-channel muting
    int BoostNoise; // double noise volume when non-zero
    
    /* Variables */
    float Clock;
    float dClock;
    int PSGStereo;
    int NumClocksForSample;
    int WhiteNoiseFeedback;
    int SRWidth;
    
    /* PSG registers: */
    int Registers[8];        /* Tone, vol x4 */
    int LatchedRegister;
    int NoiseShiftRegister;
    int NoiseFreq;            /* Noise channel signal generator frequency */
    
    /* Output calculation variables */
    int ToneFreqVals[4];      /* Frequency register values (counters) */
    int ToneFreqPos[4];        /* Frequency channel flip-flops */
    int Channels[4];          /* Value of each channel, before stereo is applied */
    float IntermediatePos[4];   /* intermediate values used at boundaries between + and - (does not need double accuracy)*/

    float panning[4][2];            /* fake stereo */

	int NgpFlags;		/* bit 7 - NGP Mode on/off, bit 0 - is 2nd NGP chip */
	void* NgpChip2;
} SN76489_Context;

/* Function prototypes */
SN76489_Context* SN76489_Init(int PSGClockValue, int SamplingRate);
void SN76489_Reset(SN76489_Context* chip);
void SN76489_Shutdown(SN76489_Context* chip);
void SN76489_Config(SN76489_Context* chip, /*int mute,*/ int feedback, int sw_width, int boost_noise);
/*
void SN76489_SetContext(SN76489_Context* chip, uint8 *data);
void SN76489_GetContext(SN76489_Context* chip, uint8 *data);
uint8 *SN76489_GetContextPtr(int chip);
int SN76489_GetContextSize(void);*/
void SN76489_Write(SN76489_Context* chip, int data);
void SN76489_GGStereoWrite(SN76489_Context* chip, int data);
//void SN76489_Update(SN76489_Context* chip, INT16 **buffer, int length);
void SN76489_Update(SN76489_Context* chip, INT32 **buffer, int length);

/* Non-standard getters and setters */
//int  SN76489_GetMute(SN76489_Context* chip);
void SN76489_SetMute(SN76489_Context* chip, int val);

void SN76489_SetPanning(SN76489_Context* chip, int ch0, int ch1, int ch2, int ch3);

/* and a non-standard data getter */
//void SN76489_UpdateOne(SN76489_Context* chip, int *l, int *r);

#endif /* _SN76489_H_ */
