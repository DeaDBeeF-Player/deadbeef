/* These #defines are TEMPORARY. They are used to write alternative code to
 * handle ambiguities in the format specification. The correct code in each
 * case will be determined most likely by experimentation.
 */
#define STEREO_SAMPLES_COUNT_AS_TWO
#define INVALID_ORDERS_END_SONG
#define SUSTAIN_LOOP_OVERRIDES_NORMAL_LOOP
#define VOLUME_OUT_OF_RANGE_SETS_MAXIMUM



#include <stdio.h>//temporary



/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * cit.c - IT Compiler                                / / \  \
 *                                                   | <  /   \_
 * Derived from an IT file loader by Bob.            |  \/ /\   /
 *                                                    \_  /  > /
 * Written by entheh. Barely works. A complete          | \ / /
 * mess.                                                |  ' /
 *                                                       \__/
 */

#include <string.h>
#include <math.h>
#include "allegro.h"



#define MUSIC_IT AL_ID('I','M','P','M')



typedef struct MODULE_MUSIC_INFO {
	char Name[29];
    int Type;
} MODULE_MUSIC_INFO;

#define ENVELOPE_ON 1
#define ENVELOPE_LOOP_ON 2
#define ENVELOPE_SUSTAINLOOP 4
#define ENVELOPE_PITCH_IS_FILTER 128

typedef struct MODULE_ENVELOPE {
	unsigned char Flag,
    	NumNodes,
        LoopBegin, LoopEnd, SustainLoopBegin, SustainLoopEnd; //in nodes.
    char NodeY[25];
    short NodeTick[25];
} MODULE_ENVELOPE;

typedef struct MODULE_VENVELOPE {
    unsigned char NextNode;
    unsigned short CurTick;
} MODULE_VENVELOPE;

#define NNA_NOTECUT      0
#define NNA_NOTECONTINUE 1
#define NNA_NOTEOFF      2
#define NNA_NOTEFADE     3

#define DCT_OFF 0
#define DCT_NOTE 1
#define DCT_SAMPLE 2
#define DCT_INSTRUMENT 3

#define DCA_CUT 0
#define DCA_NOTEOFF 1
#define DCA_NOTEFADE 2

typedef struct MODULE_INSTRUMENT {
	//unsigned char Flag;
    char VolumeLoopNodeStart, VolumeLoopNodeEnd;
    char SustainLoopNodeStart, SustainLoopNodeEnd;
    char DuplicateCheckType;
    char DuplicateCheckAction;
    char NewNoteAction;
    int FadeOut;

    unsigned char PitchPanSeparation, //0->64, Bit7: Don't use
    	PitchPanCenter; //Note, from C-0 to B-9
    unsigned char GlobalVolume, //0->128
    	DefaultPan; //0->64, Bit7: Don't use
	unsigned char RandomVolume, RandomPanning;

	unsigned char FilterCutOff;
	unsigned char FilterResonance;

    unsigned char NoteSample[120];
    unsigned char NoteNote[120];

    MODULE_ENVELOPE VolumeEnvelope, PanningEnvelope, PitchEnvelope;
} MODULE_INSTRUMENT;

#define SAMPLE_HASSAMPLE       1
#define SAMPLE_16BIT           2
#define SAMPLE_STEREO          4
#define SAMPLE_USELOOP        16
#define SAMPLE_USESUSTAINLOOP 32
#define SAMPLE_PINGPONGLOOP   64
#define SAMPLE_PINGPONGSUSTAINLOOP 128

#define VIBRATO_SINE 0
#define VIBRATO_RAMPDOWN 1
#define VIBRATO_SQUARE 2
#define VIBRATO_RANDOM 3

typedef struct MODULE_SAMPLE {
	unsigned char GlobalVolume; //0->64
    unsigned char Flag;
    unsigned char DefaultVolume;
	unsigned char DefaultPanning;
    int SampleLength; //in samples, not bytes !
    int LoopBegin, LoopEnd; //in samples
    int SustainLoopBegin, SustainLoopEnd;
    int C5Speed; //Number of bytes/sec for C-5

    SAMPLE *Sample;

    char VibratoSpeed; //0->64
    char VibratoDepth; //0->64
    char VibratoWaveForm;
    char VibratoRate; //0->64
} MODULE_SAMPLE;

#define NOTEMASK_NOTE        1
#define NOTEMASK_INSTRUMENT  2
#define NOTEMASK_VOLPAN      4
#define NOTEMASK_COMMAND     8

#define NOTE_OFF 255
#define NOTE_CUT 254

typedef struct MODULE_NOTE {
	char Mask;
	char Channel; //if -1, then end of row.    
    unsigned char Note;
    char Instrument;
    unsigned char VolPan;
    unsigned char Command, CommandValue;
} MODULE_NOTE;

typedef struct MODULE_PATTERN {
	int NumRows;
    int NumNotes;
    MODULE_NOTE *Note;
} MODULE_PATTERN;

#define VCHANNEL_PLAYING    1
#define VCHANNEL_BACKGROUND 2
#define VCHANNEL_FADING     4
#define VCHANNEL_RETRIG     8

struct MODULE_CHANNEL;

typedef struct MODULE_VCHANNEL {
	unsigned char Flag;
	unsigned char Instrument;
	unsigned char Note;
	unsigned int Volume;
	unsigned char Pan;
	signed char PanSep;

	MODULE_SAMPLE *sample;

	int pitch; /* 0 corresponds to C-5, 256 is one semitone. */

	MODULE_VENVELOPE VVolEnv;
	int fadeoutcount;

	/* WARNING: search for BLEARGH when adding to this struct. */
/*
	MODULE_SAMPLE *Sample;  //NULL is unused
    char voice;
    char ChannelVolume;
    char NoteOn;
    char NNA;
    short FadeOutCount, FadeOut;
    float MixVolume, MixPan;
    MODULE_VENVELOPE *VVolumeEnvelope, *VPanningEnvelope, *VPitchEnvelope;
    struct MODULE_VCHANNEL *next, *prev;
*/
	unsigned char NNA;

	unsigned int DUHvol, DUHpan;

	struct MODULE_CHANNEL *channel;
} MODULE_VCHANNEL;

#define CHANNEL_RETRIG 1

typedef struct MODULE_CHANNEL {
	unsigned int ID;
	unsigned char Flag;
	unsigned char Note;
	unsigned char Instrument;
	unsigned char Volume; //0->64
    unsigned char Pan;    //0->32->64, 100 = surround, Bit7: Disable
	signed char PanSep;

	int pitch;

	unsigned char ChannelVolume;

	unsigned char lastvolslide;
	unsigned char lastDKL, lastN, lastW, lastEF, lastG, lastT;
	int portamento, toneporta;
	signed char volslide;
	signed char channelvolslide;

	MODULE_SAMPLE *sample;

    MODULE_VCHANNEL *VChannel;
} MODULE_CHANNEL;

#define FLAG_STEREO         1
#define FLAG_USEINSTRUMENTS 4
#define FLAG_LINEARSLIDES   8 /* If not set, use Amiga slides */
#define FLAG_OLDEFFECT     16
#define FLAG_COMPATIBLEGXX 32

#define ORDER_END 255
#define ORDER_SKIP 254

typedef struct MODULE {
	MODULE_INSTRUMENT *Instrument;
    MODULE_SAMPLE *Sample;
    MODULE_PATTERN *Pattern;

	int NumOrders;
    int NumInstruments;
    int NumSamples;
    int NumPatterns;
    int Flags;
    short Version;
    unsigned char GlobalVolume;
	signed char globalvolslide;
    unsigned char MixVolume;
    unsigned char Speed, Tempo, tick;
	signed char temposlide;
    unsigned char PanningSeparation;

    unsigned char *Order;

    MODULE_CHANNEL Channel[64];
	MODULE_VCHANNEL VChannel[256];

	int processorder;
	int processrow;
	int breakrow;
	int rowcount;
} MODULE;



#define IT_SET_SPEED             1
#define IT_JUMP_TO_ORDER         2
#define IT_BREAK_TO_ROW          3
#define IT_VOLUME_SLIDE          4
#define IT_PORTAMENTO_DOWN       5
#define IT_PORTAMENTO_UP         6
#define IT_TONE_PORTAMENTO       7
#define IT_VIBRATO               8
#define IT_TREMOR                9
#define IT_ARPEGGIO             10
#define IT_VOLSLIDE_VIBRATO     11
#define IT_VOLSLIDE_TONEPORTA   12
#define IT_SET_CHANNEL_VOLUME   13
#define IT_CHANNEL_VOLUME_SLIDE 14
#define IT_SET_SAMPLE_OFFSET    15
#define IT_PANNING_SLIDE        16
#define IT_RETRIGGER_NOTE       17
#define IT_TREMOLO              18
#define IT_S                    19
#define IT_SET_SONG_TEMPO       20
#define IT_FINE_VIBRATO         21
#define IT_SET_GLOBAL_VOLUME    22
#define IT_GLOBAL_VOLUME_SLIDE  23
#define IT_SET_PANNING          24
#define IT_PANBRELLO            25
#define IT_MIDI_MACRO           26 //see MIDI.TXT



/* These represent the top nibble of the command value. */
#define IT_S_SET_FILTER              0 /* Greyed out in IT... */
#define IT_S_SET_GLISSANDO_CONTROL   1 /* Greyed out in IT... */
#define IT_S_FINETUNE                2 /* Greyed out in IT... */
#define IT_S_SET_VIBRATO_WAVEFORM    3
#define IT_S_SET_TREMOLO_WAVEFORM    4
#define IT_S_SET_PANBRELLO_WAVEFORM  5
#define IT_S_FINE_PATTERN_DELAY      6
#define IT_S7                        7
#define IT_S_SET_PAN                 8
#define IT_S_SET_SURROUND_SOUND      9
#define IT_S_SET_HIGH_OFFSET        10
#define IT_S_PATTERN_LOOP           11
#define IT_S_DELAYED_NOTE_CUT       12
#define IT_S_NOTE_DELAY             13
#define IT_S_PATTERN_DELAY          14
#define IT_S_SET_MIDI_MACRO         15

/*
S0x Set filter
S1x Set glissando control
S2x Set finetune


S3x Set vibrato waveform to type x
S4x Set tremelo waveform to type x
S5x Set panbrello waveform to type x
  Waveforms for commands S3x, S4x and S5x:
    0: Sine wave
    1: Ramp down
    2: Square wave
    3: Random wave
S6x Pattern delay for x ticks
S70 Past note cut
S71 Past note off
S72 Past note fade
S73 Set NNA to note cut
S74 Set NNA to continue
S75 Set NNA to note off
S76 Set NNA to note fade
S77 Turn off volume envelope
S78 Turn on volume envelope
S79 Turn off panning envelope
S7A Turn on panning envelope
S7B Turn off pitch envelope
S7C Turn on pitch envelope
S8x Set panning position
S91 Set surround sound
SAy Set high value of sample offset yxx00h
SB0 Set loopback point
SBx Loop x times to loopback point
SCx Note cut after x ticks
SDx Note delay for x ticks
SEx Pattern delay for x rows
SFx Set parameterised MIDI Macro
*/



typedef struct MODULE_PLAY {
	MODULE *Music;
    int Loop, Tick;
    int CurOrder, CurPattern, CurPos;
    int Command, CommandVal0, CommandVal1, CommandVal2;
    int pos;
} MODULE_PLAY;
extern MODULE_PLAY *song;

extern int IT_Play_Method;

MODULE *load_it(char*);
//int get_module_size(MODULE *);

int play_it(MODULE *j, int loop);
void install_module();
void set_mix_volume(int i);

void stop_it();
int is_music_done();
void destroy_it(MODULE *j);

//Should be internal:
extern MODULE_PLAY *song;
extern int note_freq[120];

//extern void MOD_Interrupt(...);
extern int MOD_Poller(void*);

#define IT_TIMER 0
#define IT_POLL  1

/* typedef.hpp */

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

/* load_it.cpp */

/*
int	detect_it(char *f) {
	int	sig;
	PACKFILE *fn = pack_fopen(f, F_READ);
	
	if (fn == NULL)
		return FALSE;

	sig	= pack_mgetl(fn);
	if (sig	!= MUSIC_IT) {
		pack_fclose(fn);
		return FALSE;
	}
	pack_fclose(fn);

	return TRUE;
}
*/



MODULE *create_it() {
	MODULE *m = (MODULE*)malloc(sizeof(MODULE));
	if (!m)
		return NULL;
	memset(m, 0, sizeof(MODULE));
	return m;
}



void destroy_it(MODULE *j) {

	int i;

	if (j) {
		/* Remove patterns. */
		if (j->Pattern) {
			for (i = 0; i < j->NumPatterns; i++)
				free(j->Pattern[i].Note);
			free(j->Pattern);
		}

		/* Remove instruments. */
		free(j->Instrument);

		/* Remove samples. */
		if (j->Sample) {
			for (i = 0; i < j->NumSamples; i++)
				destroy_sample(j->Sample[i].Sample);
			free(j->Sample);
		}

		/* Remove orders. */
		free(j->Order);

		free(j);
	}
}



//#define DEBUG_IT_SIZE

/*
int	get_module_size(MODULE	*j)	{
	int	a, b, c, d = 0,	e;
    int i;
	a =	sizeof(MODULE) + j->NumOrders;
	b =	j->NumInstruments * sizeof(MODULE_INSTRUMENT);
	c =	j->NumSamples * sizeof(MODULE_SAMPLE);
	
	for	(i=0; i<j->NumSamples; i++)
		d += j->Sample[i].SampleLength * (j->Sample[i].Flag	& 2	? sizeof(short)	: 1) * (j->Sample[i].Flag &	4 ?	2: 1);
		
	e =	4 +	sizeof(MODULE_PATTERN) * j->NumPatterns;
	
	for	(i=0; i<j->NumPatterns;	i++)
		e += j->Pattern[i].NumNotes	* sizeof(MODULE_NOTE);
	#ifdef DEBUG_IT_SIZE
	fprintf(stderr, "Base: %i, Instruments(%i): %i, Samples(%i): %i, Data: %i, Patterns(%i): %i\n", a, j->NumInstruments, b,	j->NumSamples, c, d, j->NumPatterns, e);
	#endif

	return a+b+c+d+e;
}
*/



#define	MAX_IT_CHN 64

//#define DEBUG_HEADER
//#define DEBUG_INSTRUMENTS
//#define DEBUG_SAMPLES
//#define DEBUG_PATTERNS

static dword *sourcebuf	= NULL;
static dword *sourcepos	= NULL;
static byte	rembits	= 0;

int	readblock(PACKFILE *f) {
	long size;
	int	c =	pack_igetw(f);
	if (c == -1)
		return 0;
	size = c;

	sourcebuf =	(dword*)malloc(size+4);
	if (!sourcebuf)
		return 0;
		
	c =	pack_fread(sourcebuf, size,	f);
	if (c <	1) {
		free(sourcebuf);
		sourcebuf =	NULL;
		return 0;		 
	}
	sourcepos =	sourcebuf;
	rembits	= 32;
	return 1;
}

void freeblock() {
	if (sourcebuf)
		free(sourcebuf);
	sourcebuf =	NULL;	 
}

dword readbits(char	b) {
	dword val;
	if (b <= rembits) {
		val	= *sourcepos & ((1 << b) - 1);
		*sourcepos >>= b;
		rembits	-= b;
	}
	else {
		dword nbits	= b	- rembits;
		val	= *sourcepos;
		sourcepos++;
		val	|= ((*sourcepos	& ((1 << nbits)	- 1)) << rembits);
		*sourcepos >>= nbits;
		rembits	= 32 - nbits;
	}
	return val;
}

void decompress8(PACKFILE *f, void *data, int len, int tver) {
	char *destbuf =	(char*)data;
	char *destpos =	destbuf;
	int	blocklen, blockpos;
	byte bitwidth;
	word val;
	char d1, d2;

	memset(destbuf,	0, len);

	while (len>0) {
		//Read a block of compressed data:
		if (!readblock(f))
			return;
		//Set up a few variables
		blocklen = (len	< 0x8000) ?	len	: 0x8000; //Max block length is 0x8000 bytes
		blockpos = 0;
		bitwidth = 9;
		d1 = d2	= 0;
		//Start the decompression:
		while (blockpos	< blocklen)	{
			//Read a value:
			val	= readbits(bitwidth);
			//Check for bit width change:
			
			if (bitwidth < 7) {	//Method 1:
				if (val	== (1 << (bitwidth - 1))) {
					val	= readbits(3) +	1;
					bitwidth = (val	< bitwidth)	? val :	val	+ 1;
					continue;
				}
			}
			else if	(bitwidth <	9) { //Method 2
				byte border	= (0xFF	>> (9 -	bitwidth)) - 4;

				if (val	> border &&	val	<= (border + 8)) {
					val	-= border;
					bitwidth = (val	< bitwidth)	? val :	val	+ 1;
					continue;
				}
			}
			else if	(bitwidth == 9)	{ //Method 3
				if (val	& 0x100) {
					bitwidth = (val	+ 1) & 0xFF;
					continue;
				}
			}
			else { //Illegal width, abort ?
				freeblock();
				return;
			}

			//Expand the value to signed byte:
            {
			char v;	//The sample value:
			if (bitwidth < 8) {
				byte shift = 8 - bitwidth;
				v =	(val <<	shift);
				v >>= shift;
			}
			else
				v =	(char)val;
				
			//And integrate the sample value
			//(It always has to end with integration doesn't it ? ;-)
			d1 += v;
			d2 += d1;
            }

			//Store !
			*destpos = ((tver == 0x215)	? d2 : d1);
			destpos++;
			blockpos++;
		}
		freeblock();
		len	-= blocklen;
	}
	return;
}

void decompress16(PACKFILE *f, void	*data, int len,	int	tver) {
	//make the output buffer:
	short *destbuf = (short*)data;
	short *destpos = destbuf;
	int	blocklen, blockpos;
	byte bitwidth;
	long val;
	short d1, d2;

	memset(destbuf,	0, len);

	while (len>0) {
		//Read a block of compressed data:
		if (!readblock(f))
			return;
		//Set up a few variables
		blocklen = (len	< 0x4000) ?	len	: 0x4000; // Max block length is 0x4000 bytes
		blockpos = 0;
		bitwidth = 17;
		d1 = d2	= 0;
		//Start the decompression:
		while (blockpos	< blocklen)	{
			val	= readbits(bitwidth);
			//Check for bit width change:
			
			if (bitwidth < 7) {	//Method 1:
				if (val	== (1 << (bitwidth - 1))) {
					val	= readbits(4) +	1;
					bitwidth = (val	< bitwidth)	? val :	val	+ 1;
					continue;
				}
			}
			else if	(bitwidth <	17)	{ //Method 2
				word border	= (0xFFFF >> (17 - bitwidth)) -	8;

				if (val	> border &&	val	<= (border + 16)) {
					val	-= border;
					bitwidth = val < bitwidth ?	val	: val +	1;
					continue;
				}
			}
			else if	(bitwidth == 17) { //Method 3
				if (val	& 0x10000) {
					bitwidth = (val	+ 1) & 0xFF;
					continue;
				}
			}
			else { //Illegal width, abort ?
				freeblock();
				return;
			}

			//Expand the value to signed byte:
            {
			short v; //The sample value:
			if (bitwidth < 16) {
				byte shift = 16	- bitwidth;
				v =	(val <<	shift);
				v >>= shift;
			}
			else
				v =	(short)val;
				
			//And integrate the sample value
			//(It always has to end with integration doesn't it ? ;-)
			d1 += v;
			d2 += d1;
            }

			//Store !
			*destpos = ((tver == 0x215)	? d2 : d1);
			destpos++;
			blockpos++;
		}
		freeblock();
	   	len	-= blocklen;
	}
	return;
}



void read_envelopes(PACKFILE *f, MODULE *j, int i)
{
	int k;

	j->Instrument[i].VolumeEnvelope.Flag = pack_getc(f);
	j->Instrument[i].VolumeEnvelope.NumNodes = pack_getc(f);
	j->Instrument[i].VolumeEnvelope.LoopBegin =	pack_getc(f);
	j->Instrument[i].VolumeEnvelope.LoopEnd	= pack_getc(f);
	j->Instrument[i].VolumeEnvelope.SustainLoopBegin = pack_getc(f);
	j->Instrument[i].VolumeEnvelope.SustainLoopEnd = pack_getc(f);
	for (k = 0; k < j->Instrument[i].VolumeEnvelope.NumNodes; k++) {
		j->Instrument[i].VolumeEnvelope.NodeY[k] = pack_getc(f);
		j->Instrument[i].VolumeEnvelope.NodeTick[k]	= pack_igetw(f);
	}
	pack_fseek(f, 75 - j->Instrument[i].VolumeEnvelope.NumNodes	* 3);

	j->Instrument[i].PanningEnvelope.Flag =	pack_getc(f);
	j->Instrument[i].PanningEnvelope.NumNodes =	pack_getc(f);
	j->Instrument[i].PanningEnvelope.LoopBegin = pack_getc(f);
	j->Instrument[i].PanningEnvelope.LoopEnd = pack_getc(f);
	j->Instrument[i].PanningEnvelope.SustainLoopBegin =	pack_getc(f);
	j->Instrument[i].PanningEnvelope.SustainLoopEnd	= pack_getc(f);
	for (k = 0; k < j->Instrument[i].PanningEnvelope.NumNodes; k++)	{
		j->Instrument[i].PanningEnvelope.NodeY[k] =	pack_getc(f);
		j->Instrument[i].PanningEnvelope.NodeTick[k] = pack_igetw(f);
	}
	pack_fseek(f, 75 - j->Instrument[i].PanningEnvelope.NumNodes * 3);

	j->Instrument[i].PitchEnvelope.Flag	= pack_getc(f);
	j->Instrument[i].PitchEnvelope.NumNodes	= pack_getc(f);
	j->Instrument[i].PitchEnvelope.LoopBegin = pack_getc(f);
	j->Instrument[i].PitchEnvelope.LoopEnd = pack_getc(f);
	j->Instrument[i].PitchEnvelope.SustainLoopBegin	= pack_getc(f);
	j->Instrument[i].PitchEnvelope.SustainLoopEnd =	pack_getc(f);
	for (k = 0; k < j->Instrument[i].PitchEnvelope.NumNodes; k++) {
		j->Instrument[i].PitchEnvelope.NodeY[k]	= pack_getc(f);
		j->Instrument[i].PitchEnvelope.NodeTick[k] = pack_igetw(f);
	}
}



MODULE *load_old_instrument(char *file, long offset, MODULE *j, int i)
{
	int k;
	PACKFILE *f = pack_fopen(file, F_READ);

	if (!f) {
		#ifdef DEBUG_INSTRUMENTS
		fprintf(stderr, "Error reopening!\n");
		#endif
		destroy_it(j);
		return NULL;
	}

	pack_fseek(f, offset);

	if (pack_mgetl(f) != AL_ID('I','M','P','I')) {
		destroy_it(j);
		return NULL;
	}

	/* Skip DOS Filename */
	pack_fseek(f, 13);

	j->Instrument[i].VolumeEnvelope.Flag = pack_getc(f);
	j->Instrument[i].VolumeEnvelope.LoopBegin = pack_getc(f);
	j->Instrument[i].VolumeEnvelope.LoopEnd = pack_getc(f);
	j->Instrument[i].VolumeEnvelope.SustainLoopBegin = pack_getc(f);
	j->Instrument[i].VolumeEnvelope.SustainLoopEnd = pack_getc(f);

	/* Two unused bytes */
	pack_fseek(f, 2);

	/* OLD FadeOut:  Ranges between 0 and 64, but the fadeout "Count" is 512.
	 * NEW FadeOut:  Ranges between 0 and 128, but the fadeout "Count" is 1024
	 *
	 * TODO: Find out what this means, and make adjustments accordingly.
	 */
	j->Instrument[i].FadeOut = pack_igetw(f) << 1;
	j->Instrument[i].NewNoteAction = pack_getc(f);
	j->Instrument[i].DuplicateCheckType = pack_getc(f);
	j->Instrument[i].DuplicateCheckAction = DCA_CUT; /* This might be wrong! */

	/* Skip Tracker Version and Number of Samples. These are only used in
	 * separate instrument files. Also skip unused bytes and Instrument Name.
 	 */
	pack_fseek(f, 36);

	j->Instrument[i].PitchPanSeparation = 0;
	j->Instrument[i].PitchPanCenter = 60;
	j->Instrument[i].GlobalVolume = 128; /* Should this be 64 or something? */
	j->Instrument[i].DefaultPan = 32; /* Should this be 128? */
	j->Instrument[i].RandomVolume = 0;
	j->Instrument[i].RandomPanning = 0;

	for	(k = 0; k < 120; k++) {
		j->Instrument[i].NoteNote[k] = pack_getc(f);
		j->Instrument[i].NoteSample[k] = pack_getc(f);
	}

	/* Skip "Volume envelope (200 bytes)" - need to know better what this is
	 * for though.
	 */
	pack_fseek(f, 200);

	fprintf(stderr, "Inst %02d Env:", i);

	for (j->Instrument[i].VolumeEnvelope.NumNodes = 0;
		 j->Instrument[i].VolumeEnvelope.NumNodes < 25;
		 j->Instrument[i].VolumeEnvelope.NumNodes++)
	{
		j->Instrument[i].VolumeEnvelope.NodeTick[k]	= pack_getc(f);
		j->Instrument[i].VolumeEnvelope.NodeY[k] = pack_getc(f);

		fprintf(stderr, " %d,%d",
				j->Instrument[i].VolumeEnvelope.NodeTick[k],
				j->Instrument[i].VolumeEnvelope.NodeY[k]);
	}
	pack_fseek(f, 50 - j->Instrument[i].VolumeEnvelope.NumNodes	* 2);

	fprintf(stderr, "\n");

	j->Instrument[i].FilterCutOff = 0;    //Are these the right values
	j->Instrument[i].FilterResonance = 0; //to disable the filter?

	j->Instrument[i].PanningEnvelope.Flag =	0;
	j->Instrument[i].PanningEnvelope.NumNodes =	0;
	j->Instrument[i].PanningEnvelope.LoopBegin = 0;
	j->Instrument[i].PanningEnvelope.LoopEnd = 0;
	j->Instrument[i].PanningEnvelope.SustainLoopBegin =	0;
	j->Instrument[i].PanningEnvelope.SustainLoopEnd	= 0;

	j->Instrument[i].PitchEnvelope.Flag	= 0;
	j->Instrument[i].PitchEnvelope.NumNodes	= 0;
	j->Instrument[i].PitchEnvelope.LoopBegin = 0;
	j->Instrument[i].PitchEnvelope.LoopEnd = 0;
	j->Instrument[i].PitchEnvelope.SustainLoopBegin	= 0;
	j->Instrument[i].PitchEnvelope.SustainLoopEnd =	0;

	pack_fclose(f);

	if (errno) {
		destroy_it(j);
		return NULL;
	}

	return j;
}



MODULE *load_instrument(char *file, long offset, MODULE *j, int i)
{
	int k;
	PACKFILE *f = pack_fopen(file, F_READ);

	if (!f) {
		#ifdef DEBUG_INSTRUMENTS
		fprintf(stderr, "Error reopening!\n");
		#endif
		destroy_it(j);
		return NULL;
	}

	pack_fseek(f, offset);

	if (pack_mgetl(f) != AL_ID('I','M','P','I')) {
		destroy_it(j);
		return NULL;
	}

	/* Skip DOS Filename */
	pack_fseek(f, 13);

	j->Instrument[i].NewNoteAction = pack_getc(f);
	j->Instrument[i].DuplicateCheckType = pack_getc(f);
	j->Instrument[i].DuplicateCheckAction = pack_getc(f);
	j->Instrument[i].FadeOut = pack_igetw(f);
	j->Instrument[i].PitchPanSeparation = pack_getc(f);
	j->Instrument[i].PitchPanCenter = pack_getc(f);
	j->Instrument[i].GlobalVolume = pack_getc(f);
	j->Instrument[i].DefaultPan = pack_getc(f);
	j->Instrument[i].RandomVolume = pack_getc(f);
	j->Instrument[i].RandomPanning = pack_getc(f);

	#ifdef DEBUG_INSTRUMENTS
	fprintf(stderr, "I%02i @ 0x%X, NNA %i, DCT %i, DCA %i, FO %i, "
		"PPS %i, PPC %i, GVol %i, DPan %i, RV %i, RP %i\n",
		i, insoffs[i],
		j->Instrument[i].NewNoteAction,
		j->Instrument[i].DuplicateCheckType,
		j->Instrument[i].DuplicateCheckAction,
		j->Instrument[i].FadeOut,
		j->Instrument[i].PitchPanSeparation,
		j->Instrument[i].PitchPanCenter,
		j->Instrument[i].GlobalVolume,
		j->Instrument[i].DefaultPan,
		j->Instrument[i].RandomVolume,
		j->Instrument[i].RandomPanning);
	#endif

	/* Skip Tracker Version and Number of Samples. These are only used in
	 * separate instrument files. Also skip unused byte and Instrument Name.
 	 */
	pack_fseek(f, 30);

	j->Instrument[i].FilterCutOff = pack_getc(f);
	j->Instrument[i].FilterResonance = pack_getc(f);

	/* Skip MIDI Channel, Program and Bank */
	pack_fseek(f, 4);

	for	(k = 0; k < 120; k++) {
		j->Instrument[i].NoteNote[k] = pack_getc(f);
		j->Instrument[i].NoteSample[k] = pack_getc(f);
	}

	read_envelopes(f, j, i);

	pack_fclose(f);

	if (errno) {
		destroy_it(j);
		return NULL;
	}

	return j;
}



MODULE *load_it_sample(char *file, long offset, MODULE *j, int i)
{
	int	sam_samptr,	convert;
	int len;
	int k;
	SAMPLE *sam;
	void *dat;
	
	PACKFILE *f = pack_fopen(file, F_READ);
	if (!f) {
		#ifdef DEBUG_SAMPLES
		fprintf(stderr, "Error opening!\n");
		#endif
		destroy_it(j);
		return NULL;
	}

	pack_fseek(f, offset);

	if (pack_mgetl(f) != AL_ID('I','M','P','S')) {
		destroy_it(j);
		return NULL;
	}

	/* Skip DOS Filename. */
	pack_fseek(f, 13);

	j->Sample[i].GlobalVolume =	pack_getc(f);
	j->Sample[i].Flag =	pack_getc(f);
	j->Sample[i].DefaultVolume = pack_getc(f);

	#ifdef DEBUG_SAMPLES
	fprintf(stderr, "S%02i @ 0x%X, Vol: %i/%i, Flag: %i", i, samoffs[i], j->Sample[i].GlobalVolume, j->Sample[i].Volume, j->Sample[i].Flag);
	#endif

	/* Skip Sample Name. */
	pack_fseek(f, 26);

	convert = pack_getc(f);
	j->Sample[i].DefaultPanning = pack_getc(f);
	j->Sample[i].SampleLength =	pack_igetl(f);
	j->Sample[i].LoopBegin = pack_igetl(f);
	j->Sample[i].LoopEnd = pack_igetl(f);
	j->Sample[i].C5Speed = pack_igetl(f);
	j->Sample[i].SustainLoopBegin =	pack_igetl(f);
	j->Sample[i].SustainLoopEnd	= pack_igetl(f);

	#ifdef DEBUG_SAMPLES
	fprintf(stderr, ", SLen: %i, LpB: %i, LpE: %i, C5S: %i\n", j->Sample[i].SampleLength, j->Sample[i].LoopBegin, j->Sample[i].LoopEnd, j->Sample[i].C5Speed);
	#endif

	sam_samptr = pack_igetl(f);
		
	j->Sample[i].VibratoSpeed =	pack_getc(f);
	j->Sample[i].VibratoDepth =	pack_getc(f);
	j->Sample[i].VibratoRate = pack_getc(f);
	j->Sample[i].VibratoWaveForm = pack_getc(f);

	#ifdef DEBUG_SAMPLES
	fprintf(stderr, "SusLpB: %i, SusLpE: %i, VibSp: %i, VibDep: %i, VibWav: %i, VibRat: %i\n", j->Sample[i].SustainLoopBegin, j->Sample[i].SustainLoopEnd, j->Sample[i].VibratoSpeed, j->Sample[i].VibratoDepth,	j->Sample[i].VibratoWaveForm, j->Sample[i].VibratoRate);
	#endif

	pack_fclose(f);

	if (errno) {
		destroy_it(j);
		return NULL;
	}

	if (j->Sample[i].Flag & SAMPLE_HASSAMPLE) {
		
		f =	pack_fopen(file, F_READ);
		pack_fseek(f, sam_samptr);

		len	= j->Sample[i].SampleLength;
		if (j->Sample[i].Flag & SAMPLE_16BIT) len <<= 1;
#ifndef STEREO_SAMPLES_COUNT_AS_TWO
		if (j->Sample[i].Flag & SAMPLE_STEREO) len <<= 1;
#endif

		#ifdef DEBUG_SAMPLES
		fprintf(stderr, "Len: %i, Size: %i KB\n", j->Sample[i].SampleLength,	len/1024);
		#endif

		sam = create_sample(j->Sample[i].Flag & SAMPLE_16BIT ? 16 : 8,
							j->Sample[i].Flag &	SAMPLE_STEREO ? 1 : 0,
#ifdef STEREO_SAMPLES_COUNT_AS_TWO
							j->Sample[i].C5Speed >> 1,
#else
							j->Sample[i].C5Speed,
#endif
							j->Sample[i].SampleLength);

		if (j->Sample[i].Flag & 8) { // If the sample is packed, then we must unpack it

			if (j->Sample[i].Flag & SAMPLE_16BIT)
				decompress16(f,	sam->data, j->Sample[i].SampleLength, j->Version);
			else
				decompress8(f, sam->data, j->Sample[i].SampleLength, j->Version);

		} else if ((j->Sample[i].Flag & SAMPLE_16BIT)) {
			if (convert & 2)
				for (k = 0; k < len; k += 2)
					*(short *)((char *)sam->data + k) = pack_mgetw(f);
			else
				for (k = 0; k < len; k += 2)
					*(short *)((char *)sam->data + k) = pack_igetw(f);
		} else {
			pack_fread(sam->data, len, f);
		}
		
		if (j->Sample[i].Flag &	SAMPLE_USELOOP)	{
			sam->loop_start	= j->Sample[i].LoopBegin;
			sam->loop_end =	j->Sample[i].LoopEnd;
		}

		j->Sample[i].Sample	= sam;
		
		dat = sam->data;

		if (convert & 1) {
 			/* Convert to unsigned. */
   			if (sam->bits == 8)
	   			for (k = 0; k < len; k++)
		   			((char *)dat)[k] ^= 0x80;
   			else
   				for (k = 0; k < len; k += 2)
		   			*(short *)((char *)dat + k) ^= 0x8000;
		}

		/* NOT SUPPORTED:
		 *
		 * convert &  4 - Samples stored as delta values
		 * convert &  8 - Samples stored as byte delta values
		 * convert & 16 - Samples stored as TX-Wave 12-bit values
		 * convert & 32 - Left/Right/All Stereo prompt
		 */

		pack_fclose(f);

		if (errno) {
			destroy_it(j);
			return NULL;
		}
	}

	return j;
}



MODULE *load_pattern(char *file, long offset, MODULE *j, int i)
{
	unsigned char *buf;
	unsigned char cmask[64],
				  cnote[64],
				  cinstrument[64],
				  cvol[64],
				  ccom[64],
				  ccomval[64];
	
	int numnotes = 0, len, pos = 0, mask = 0, chn = 0;

	PACKFILE *f;

	if (offset == 0) {
		/* Empty 64-row pattern. */

		j->Pattern[i].NumRows = 64;
		j->Pattern[i].NumNotes = 0;

		return j;
	}

	f =	pack_fopen(file, F_READ);
	if (!f) {
		destroy_it(j);
		return NULL;
	}

	buf = malloc(65536);

	memset(cmask, 0, 64);
	memset(cnote, 0, 64);
	memset(cinstrument,	0, 64);
	memset(cvol, 0,	64);
	memset(ccom, 0,	64);
	memset(ccomval,	0, 64);

	pack_fseek(f, offset);

	len = pack_igetw(f);
	j->Pattern[i].NumRows = pack_igetw(f);

	/* Skip four unused bytes. */
	pack_fseek(f, 4);

	pack_fread(buf, len, f);

	while (pos < len) {

		int	b =	buf[pos++];

		if (!b)	{
			/* End of row. */
			numnotes++;
			continue;
		}

		chn = (b - 1) & 63;

		if (b & 128) {
			mask = buf[pos];
			pos++;
			cmask[chn] = mask;
		} else
			mask = cmask[chn];
				
		if (mask)
			numnotes++;
		if (mask & 1)
			pos++;
		if (mask & 2)
			pos++;
		if (mask & 4)
			pos++;
		if (mask & 8)
			pos += 2;
	}

	j->Pattern[i].NumNotes = numnotes;
	j->Pattern[i].Note = malloc(numnotes * sizeof(MODULE_NOTE));
	memset(j->Pattern[i].Note, 0, numnotes * sizeof(MODULE_NOTE));
		
	pos	= 0;
	memset(cmask, 0, 64);
	mask = 0;
	numnotes = 0;

	while (pos < len) {

		int	b =	buf[pos];
		#ifdef DEBUG_PATTERNS
		fprintf(stderr, "NumNote: %i ", numnotes);
		#endif
		pos++;

		if (b == 0) { //If end of row:
			j->Pattern[i].Note[numnotes].Channel = -1;
			numnotes++;
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "Channel: -1\n");
			#endif
			continue;
		}

		chn	= (b - 1) &	63;

		if (b &	128) {
			mask = buf[pos];
			pos++;
			cmask[chn] = mask;
		} else
			mask = cmask[chn];

		#ifdef DEBUG_PATTERNS
		fprintf(stderr, "Channel: %i Mask: %i ",	chn, mask);
		#endif
				
		if (mask)
			j->Pattern[i].Note[numnotes].Channel = chn;

		if (mask & 1) {
			j->Pattern[i].Note[numnotes].Note =	buf[pos];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_NOTE;
			cnote[chn] = buf[pos];
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "Note: %i ",	buf[pos]);
			#endif
			pos++;
		}

		if (mask & 2) {
			j->Pattern[i].Note[numnotes].Instrument	= buf[pos];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_INSTRUMENT;
			cinstrument[chn] = buf[pos];
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "Inst: %i ",	buf[pos]);
			#endif
			pos++;
		}

		if (mask & 4) {
			j->Pattern[i].Note[numnotes].VolPan = buf[pos];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_VOLPAN;
			cvol[chn] =	buf[pos];
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "Vol: %i ", buf[pos]);
			#endif
			pos++;
		}

		if (mask & 8) {
			j->Pattern[i].Note[numnotes].Command = buf[pos];
			j->Pattern[i].Note[numnotes].CommandValue = buf[pos+1];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_COMMAND;
			ccom[chn] = buf[pos];
			ccomval[chn] = buf[pos+1];
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "Com: %i CommArg: %i ", buf[pos], buf[pos+1]);
			#endif
			pos += 2;
		}

		if (mask & 16) {
			j->Pattern[i].Note[numnotes].Note = cnote[chn];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_NOTE;
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "LNote: %i ", cnote[chn]);
			#endif
		}

		if (mask & 32) {
			j->Pattern[i].Note[numnotes].Instrument = cinstrument[chn];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_INSTRUMENT;
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "LInst: %i ", cinstrument[chn]);
			#endif
		}

		if (mask & 64) {
			j->Pattern[i].Note[numnotes].VolPan = cvol[chn];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_VOLPAN;
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "LVol: %i ",	cvol[chn]);
			#endif
		}

		if (mask & 128)	{
			j->Pattern[i].Note[numnotes].Command = ccom[chn];
			j->Pattern[i].Note[numnotes].CommandValue =	ccomval[chn];
			j->Pattern[i].Note[numnotes].Mask |= NOTEMASK_COMMAND;
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "LCom: %i LComArg: %i ",	ccom[chn], ccomval[chn]);
			#endif
		}

		#ifdef DEBUG_PATTERNS
		fprintf(stderr, "\n");
		#endif

		if (mask)
			numnotes++;

		#ifdef DEBUG_PATTERNS
		rest(1000);
		#endif
	}

	free(buf);

	pack_fclose(f);

	if (errno) {
		destroy_it(j);
		return NULL;
	}

	return j;
}



MODULE *load_it(char *file) {
	PACKFILE *f;
	MODULE *j = create_it();
	int tver, tver2, flag;
	long *insoffs = NULL, *samoffs = NULL, *patoffs = NULL;
    int i;
	
	if (!j)
		return NULL;

	f = pack_fopen(file, F_READ);
	
	if (!f)
		return NULL;

	if (pack_mgetl(f) != MUSIC_IT) {
		pack_fclose(f);
		return NULL;
	}

 	/* Skip song name and pattern row highlight info */
	pack_fseek(f, 28);
	
	j->NumOrders = pack_igetw(f);
	j->NumInstruments = pack_igetw(f);
	j->NumSamples = pack_igetw(f);
	j->NumPatterns = pack_igetw(f);

	#ifdef DEBUG_HEADER
	fprintf(stderr, "Loading IT: %i Orders, %i Instruments, %i Samples, %i Patterns\n", j->NumOrders, j->NumInstruments, j->NumSamples, j->NumPatterns);
	#endif
	
	tver = pack_igetw(f);
	j->Version = tver2 = pack_igetw(f);

	#ifdef DEBUG_HEADER
	fprintf(stderr, "Tracker ver: %X, %X\n", tver, tver2);
	#endif

	j->Flags = pack_igetw(f);
	flag = pack_igetw(f);

	j->GlobalVolume = pack_getc(f);
	j->MixVolume = pack_getc(f);
	j->Speed = pack_getc(f);
	j->Tempo = pack_getc(f);
	j->PanningSeparation = pack_getc(f);

	#ifdef DEBUG_HEADER
	fprintf(stderr, "Global Volume: %i, Mixing Volume: %i, Speed: %i, Tempo: %i, PanSep: %i\n", j->GlobalVolume,	j->MixVolume, j->Speed,	j->Tempo, j->PanningSeparation);
	#endif

	/* Skip Pitch Wheel Depth, Message Length, Message Offset and Reserved */
	pack_fseek(f, 11);

	#ifdef DEBUG_HEADER
	fprintf(stderr, "Channel Pan:");
	#endif

	for (i = 0; i < MAX_IT_CHN; i++) {
		j->Channel[i].Pan = pack_getc(f);
		#ifdef DEBUG_HEADER
		fprintf(stderr, " %i", j->Channel[i].Pan);
		#endif
	}
	#ifdef DEBUG_HEADER
	fprintf(stderr, "\nChannel Vol:");
	#endif
	for (i = 0; i < MAX_IT_CHN; i++) {
		j->Channel[i].ChannelVolume = pack_getc(f);
		#ifdef DEBUG_HEADER
		fprintf(stderr, " %i", j->Channel[i].ChannelVolume);
		#endif
	}
	#ifdef DEBUG_HEADER
	fprintf(stderr, "\n");
	#endif

	j->Order = malloc(j->NumOrders);
	if (!j->Order) {
		destroy_it(j);
		return NULL;
	}
	pack_fread(j->Order, j->NumOrders, f);

	/* Whoops, no error checking! Er, would it be better to use arrays? */
	if (j->NumInstruments)
		insoffs = malloc(j->NumInstruments * sizeof(*insoffs));
	if (j->NumSamples)
		samoffs = malloc(j->NumSamples * sizeof(*samoffs));
	if (j->NumPatterns)
		patoffs = malloc(j->NumPatterns * sizeof(*patoffs));

	for (i = 0; i < j->NumInstruments; i++) insoffs[i] = pack_igetl(f);
	for (i = 0; i < j->NumSamples; i++) samoffs[i] = pack_igetl(f);
	for (i = 0; i < j->NumPatterns; i++) patoffs[i] = pack_igetl(f);

/* No skipping necessary - we can use the offsets.

	if (flag&1)	{ //Song message attached
		//Ignore.
	}
	if (flag & 4) {	//skip something:
		short u;
		char dummy[8];
        int i;
		u =	pack_igetw(f);
		for	(i=0; i<u; u++)
			pack_fread(dummy, 8, f);
	}
	if (flag & 8) {	//skip embedded MIDI configuration
		char dummy[33];
        int i;
		for	(i=0; i<9+16+128; i++)
			pack_fread(dummy, 32, f);

	}
*/
	pack_fclose(f);

	if (j->NumInstruments) {
		j->Instrument = malloc(j->NumInstruments * sizeof(MODULE_INSTRUMENT));

		if (!j->Instrument) {
			#ifdef DEBUG_INSTRUMENTS
			fprintf(stderr, "No Mem for Instruments!\n");
			#endif
			free(insoffs);
			free(samoffs);
			free(patoffs);
			destroy_it(j);
			return NULL;
		}
		memset(j->Instrument, 0, j->NumInstruments * sizeof(MODULE_INSTRUMENT));

		for	(i = 0; i < j->NumInstruments; i++) {

			if (tver2 < 0x200)
				j = load_old_instrument(file, insoffs[i], j, i);
			else
				j = load_instrument(file, insoffs[i], j, i);

			if (!j) {
				free(insoffs);
				free(samoffs);
				free(patoffs);
				return NULL;
			}
		}
	}

	if (j->NumSamples) {
		j->Sample = malloc(j->NumSamples * sizeof(MODULE_SAMPLE));

		if (!j->Sample) {
			#ifdef DEBUG_SAMPLES
			fprintf(stderr, "No Mem for Samples!\n");
			#endif
			free(insoffs);
			free(samoffs);
			free(patoffs);
			destroy_it(j);
			return NULL;
		}
		memset(j->Sample, 0, j->NumSamples * sizeof(MODULE_SAMPLE));

		for (i=0; i<j->NumSamples; i++) {

			j = load_it_sample(file, samoffs[i], j, i);

			if (!j) {
				free(insoffs);
				free(samoffs);
				free(patoffs);
				return NULL;
			}
		}
	}

	if (j->NumPatterns) {
		j->Pattern = malloc(j->NumPatterns * sizeof(MODULE_PATTERN));

		if (!j->Pattern) {
			#ifdef DEBUG_PATTERNS
			fprintf(stderr, "No Mem for Patterns!\n");
			#endif
			free(insoffs);
			free(samoffs);
			free(patoffs);
			destroy_it(j);
			return NULL;
		}
		memset(j->Pattern, 0, j->NumPatterns * sizeof(MODULE_PATTERN));

		for (i = 0; i < j->NumPatterns; i++) {

			j = load_pattern(file, patoffs[i], j, i);

			if (!j) {
				free(insoffs);
				free(samoffs);
				free(patoffs);
				return NULL;
			}
		}
	}

	if (insoffs)
		free(insoffs);
	if (samoffs)
		free(samoffs);
	if (patoffs)
		free(patoffs);

	return j;
}



/* ----------------- */



#define DUH_SIGNATURE AL_ID('D', 'U', 'H', '!')



#define SIGTYPE_SAMPLE AL_ID('S', 'A', 'M', 'P')

#define SAMPFLAG_16BIT    1	/* sample in file is 16 bit, rather than 8 bit */
#define SAMPFLAG_LOOP     2 /* loop indefinitely */
#define SAMPFLAG_XLOOP    4 /* loop x times; only relevant if LOOP not set */
#define SAMPFLAG_PINGPONG 8 /* loop back and forth, if LOOP or XLOOP set */

/* SAMPPARAM_N_LOOPS: add 'value' iterations to the loop. 'value' is assumed
 * to be positive.
 */
#define SAMPPARAM_N_LOOPS 0



#define SIGTYPE_COMBINING AL_ID('C', 'O', 'M', 'B')



#define SIGTYPE_STEREOPAN AL_ID('S', 'P', 'A', 'N')

#define SPANPARAM_PAN 0



#define SIGTYPE_SEQUENCE AL_ID('S', 'E', 'Q', 'U')

#define SEQUENCE_START_SIGNAL  0
#define SEQUENCE_SET_VOLUME    1
#define SEQUENCE_SET_PITCH     2
#define SEQUENCE_SET_PARAMETER 3
#define SEQUENCE_STOP_SIGNAL   4



void write_sample(MODULE_SAMPLE *sample, PACKFILE *f, int channel)
{
	long size = sample->SampleLength;
	long i;
	int flags = 0;
	long loop_start = 0, loop_end = 0;

	SAMPLE *smp = sample->Sample;

	pack_mputl(SIGTYPE_SAMPLE, f);

#ifdef STEREO_SAMPLES_COUNT_AS_TWO
	if (sample->Flag & SAMPLE_STEREO) {
		ASSERT((size & 1) == 0);
		size >>= 1;
	}
#endif

#ifdef SUSTAIN_LOOP_OVERRIDES_NORMAL_LOOP
	if (sample->Flag & SAMPLE_USESUSTAINLOOP) {
		flags |= SAMPFLAG_XLOOP;
		if (sample->Flag & SAMPLE_PINGPONGSUSTAINLOOP)
			flags |= SAMPFLAG_PINGPONG;
	} else if (sample->Flag & SAMPLE_USELOOP) {
		flags |= SAMPFLAG_LOOP;
		if (sample->Flag & SAMPLE_PINGPONGLOOP)
			flags |= SAMPFLAG_PINGPONG;
	}
#else
	if (sample->Flag & SAMPLE_USELOOP) {
		flags |= SAMPFLAG_LOOP;
		if (sample->Flag & SAMPLE_PINGPONGLOOP)
			flags |= SAMPFLAG_PINGPONG;
	} else if (sample->Flag & SAMPLE_USESUSTAINLOOP) {
		flags |= SAMPFLAG_XLOOP;
		if (sample->Flag & SAMPLE_PINGPONGSUSTAINLOOP)
			flags |= SAMPFLAG_PINGPONG;
	}
#endif

	if (flags & (SAMPFLAG_LOOP | SAMPFLAG_XLOOP)) {
		if (flags & SAMPFLAG_LOOP) {
			loop_start = sample->LoopBegin;
			loop_end = sample->LoopEnd;
		} else {
			loop_start = sample->SustainLoopBegin;
			loop_end = sample->SustainLoopEnd;
		}

#ifdef STEREO_SAMPLES_COUNT_AS_TWO
		if (sample->Flag & SAMPLE_STEREO) {
			ASSERT(((loop_start | loop_end) & 1) == 0);
			loop_start >>= 1;
			loop_end >>= 1;
		}
#endif

		if (loop_end > size)
			loop_end = size;
		else if (flags & SAMPFLAG_LOOP)
			size = loop_end;
	}

	pack_iputl(size, f);

	if (smp->bits == 16)
		flags |= SAMPFLAG_16BIT;

	pack_putc(flags, f);

	if (flags & (SAMPFLAG_LOOP | SAMPFLAG_XLOOP)) {
		pack_iputl(loop_start, f);
		if (!(flags & SAMPFLAG_LOOP))
			pack_iputl(loop_end, f);
	}

	if (flags & SAMPFLAG_16BIT) {

		if (smp->stereo) {
			for (i = 0; i < size; i++)
				pack_iputw(((unsigned short *)smp->data)[(i << 1) + channel] ^ 0x8000, f);
		} else {
			for (i = 0; i < size; i++)
				pack_iputw(((unsigned short *)smp->data)[i] ^ 0x8000, f);
		}

	} else {

		if (smp->stereo) {
			/* TEMPORARY CHANNEL COMBINING */
			for (i = 0; i < size; i++)
				pack_putc(((unsigned char *)smp->data)[(i << 1) + channel] ^ 0x80, f);
		} else {
			for (i = 0; i < size; i++)
				pack_putc(((unsigned char *)smp->data)[i] ^ 0x80, f);
		}

	}
}



int sample_is_valid(MODULE_SAMPLE *sample)
{
	if (!(sample->Flag & SAMPLE_HASSAMPLE))
		return 0;

	if (sample->SampleLength <= 0)
		return 0;

#ifdef SUSTAIN_LOOP_OVERRIDES_NORMAL_LOOP
	if ((sample->Flag & (SAMPLE_USELOOP | SAMPLE_USESUSTAINLOOP)) == SAMPLE_USELOOP)
#else
	if (sample->Flag & SAMPLE_USELOOP)
#endif
		if (sample->LoopEnd <= 0)
			return 0;

	return 1;
}



#define first_sample 1



void write_mono_sample(MODULE_SAMPLE *sample, int n_samples, PACKFILE *f)
{
	pack_mputl(SIGTYPE_STEREOPAN, f);

	/* Refer to the next sample. */
	pack_iputl(first_sample + n_samples + 1, f);

	write_sample(sample, f, 0);
}



void write_stereo_sample(MODULE_SAMPLE *sample, int n_samples, PACKFILE *f)
{
	pack_mputl(SIGTYPE_COMBINING, f);

	/* Two samples. */
	pack_putc(2, f);

	/* Refer to the next two samples. */
	pack_iputl(first_sample + n_samples + 1, f);
	pack_iputl(first_sample + n_samples + 2, f);

	write_sample(sample, f, 0);
	write_sample(sample, f, 1);
}



unsigned char *seqdata = NULL;
int seqpos = 0;
int max_seqdata = 0;

int seqtime = 0;

int songtime;



void sequence_c(int v)
{
	if (seqpos >= max_seqdata) {
		max_seqdata += 1024;
		seqdata = realloc(seqdata, max_seqdata);
		if (!seqdata) {
			errno = ENOMEM;
			return;
		}
	}
	seqdata[seqpos++] = v;
}


void sequence_w(int v)
{
	sequence_c(v);
	sequence_c(v >> 8);
}


void sequence_l(long v)
{
	sequence_c(v);
	sequence_c(v >> 8);
	sequence_c(v >> 16);
	sequence_c(v >> 24);
}


void sequence_cl(unsigned long v)
{
	int byte_count = 0;

	while (byte_count < 4 && v >= 128) {
		sequence_c(v | 128);
		v >>= 7;
		byte_count++;
	}

	sequence_c(v & 127);
}



void write_seqtime() {
	sequence_cl(seqtime);
	seqtime = 0;
}



int *sample_signal = NULL;



void calculate_pan(MODULE *module, MODULE_CHANNEL *channel)
{
	int samp;

	if (module->Flags & FLAG_USEINSTRUMENTS && channel->Instrument >= 1 && channel->Instrument <= module->NumInstruments) {
		MODULE_INSTRUMENT *instrument = &module->Instrument[(int)channel->Instrument - 1];
		if (instrument) {
			unsigned char note = instrument->NoteNote[channel->Note];
			samp = instrument->NoteSample[channel->Note];
			if (instrument->DefaultPan <= 64)
				channel->Pan = instrument->DefaultPan;
			channel->PanSep = (note - instrument->PitchPanCenter) * instrument->PitchPanSeparation / 8;
		} else {
			samp = 0;
			channel->PanSep = 0;
		}
	} else {
		samp = channel->Instrument;
		channel->PanSep = 0;
	}

	if (samp >= 1 && samp <= module->NumSamples) {
		int pan = module->Sample[samp - 1].DefaultPanning;
		if (pan >= 128 && pan <= 192)
			channel->Pan = pan - 128;
	}
}



/* This function assumes note->Channel >= 0. */
void process_note_data(MODULE *module, MODULE_NOTE *note)
{
	MODULE_CHANNEL *channel = &module->Channel[(int)note->Channel];

	if (note->Mask & NOTEMASK_NOTE) {
		if (note->Note >= 120) {
			channel->Note = note->Note;
		} else if (channel->Note >= 120) {
			channel->Note = note->Note;
			channel->Flag |= CHANNEL_RETRIG;
		} else {
			channel->Note = note->Note;
			if ((note->Mask & NOTEMASK_VOLPAN) == 0 ||
				!(note->VolPan >= 193 && note->VolPan <= 202))
			{
				if ((note->Mask & NOTEMASK_COMMAND) == 0 ||
					(note->Command != IT_TONE_PORTAMENTO &&
					 note->Command != IT_VOLSLIDE_TONEPORTA))
				{
					channel->Flag |= CHANNEL_RETRIG;
				}
			}
		}
	}

	if (note->Mask & NOTEMASK_INSTRUMENT) {
		if (note->Instrument != channel->Instrument) {
			channel->Flag |= CHANNEL_RETRIG;
			channel->Instrument = note->Instrument;
			if (!(module->Flags & FLAG_USEINSTRUMENTS)) {
				if (channel->Instrument >= 1 && channel->Instrument <= module->NumSamples)
					channel->sample = &module->Sample[channel->Instrument - 1];
				else
					channel->sample = NULL;
			}
		}
	}

	if (module->Flags & FLAG_USEINSTRUMENTS) {
		if (channel->Instrument >= 1 && channel->Instrument <= module->NumInstruments) {
			int samp = module->Instrument[(int)channel->Instrument - 1].NoteSample[channel->Note];
			if (samp >= 1 && samp <= module->NumSamples)
				channel->sample = &module->Sample[samp - 1];
			else
				channel->sample = NULL;
		} else
			channel->sample = NULL;
	}

	if (channel->Flag & CHANNEL_RETRIG)
		calculate_pan(module, channel);

	if (note->Mask & NOTEMASK_INSTRUMENT)
		if (channel->sample)
			channel->Volume = channel->sample->DefaultVolume;

	if (!channel->sample)
		channel->Flag &= ~CHANNEL_RETRIG;

	if (note->Mask & NOTEMASK_VOLPAN) {
		if (note->VolPan <= 64)
			channel->Volume = note->VolPan;
		else if (note->VolPan <= 74) {
			unsigned char v = note->VolPan - 65;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect DxF where x == note->VolPan - 65 */
			channel->Volume += v;
			if (channel->Volume > 64) channel->Volume = 64;

		} else if (note->VolPan <= 84) {
			unsigned char v = note->VolPan - 75;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect DFx where x == note->VolPan - 75 */
			channel->Volume -= v;
			if (channel->Volume > 64) channel->Volume = 0;
		} else if (note->VolPan <= 94) {
			unsigned char v = note->VolPan - 85;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect Dx0 where x == note->VolPan - 85 */
			channel->volslide = v;
		} else if (note->VolPan <= 104) {
			unsigned char v = note->VolPan - 95;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect D0x where x == note->VolPan - 95 */
			channel->volslide = -v;
		} else if (note->VolPan <= 114) {
			unsigned char v = (note->VolPan - 105) << 2;
			if (v == 0)
				v = channel->lastEF;
			channel->lastEF = v;
			channel->portamento -= v << 4;
		} else if (note->VolPan <= 124) {
			unsigned char v = (note->VolPan - 115) << 2;
			if (v == 0)
				v = channel->lastEF;
			channel->lastEF = v;
			channel->portamento += v << 4;
		} else if (note->VolPan < 128) { }
		else if (note->VolPan <= 192)
			channel->Pan = note->VolPan - 128;
		else if (note->VolPan <= 202) {
			//Tone Portamento
			/* Affects G's memory. Has the equivalent slide given by this
			 * table:
			 */
			static unsigned char SlideTable[] = {0, 1, 4, 8, 16, 32, 64, 96, 128, 255};

			unsigned char v = SlideTable[note->VolPan - 193];
			if (module->Flags & FLAG_COMPATIBLEGXX) {
				if (v == 0)
					v = channel->lastG;
				channel->lastG = v;
			} else {
				if (v == 0)
					v = channel->lastEF;
				channel->lastEF = v;
			}
			channel->toneporta += v << 4;
		} else if (note->VolPan <= 212)
			;//Vibrato
			/* This uses the same 'memory' as Hxx/Uxx. */
	}

	if (note->Mask & NOTEMASK_COMMAND) {
		//Interpret effect
		//unsigned char note->Command, note->CommandValue
		switch (note->Command) {
/*
Notes about effects (as compared to other module formats)

C               This is now in *HEX*. (Used to be in decimal in ST3)
E/F/G/H/U       You need to check whether the song uses Amiga/Linear slides.
H/U             Vibrato in Impulse Tracker is two times finer than in
                any other tracker and is updated EVERY tick.
                If "Old Effects" is *ON*, then the vibrato is played in the
                normal manner (every non-row tick and normal depth)
E/F/G           These commands ALL share the same memory.
Oxx             Offsets to samples are to the 'xx00th' SAMPLE. (ie. for
                16 bit samples, the offset is xx00h*2)
                Oxx past the sample end will be ignored, unless "Old Effects"
                is ON, in which case the Oxx will play from the end of the
                sample.
Yxy             This uses a table 4 times larger (hence 4 times slower) than
                vibrato or tremelo. If the waveform is set to random, then
                the 'speed' part of the command is interpreted as a delay.
*/
			case IT_SET_SPEED: if (note->CommandValue) {module->tick = module->Speed = note->CommandValue;} break;
			//case IT_JUMP_TO_ORDER: module->processorder = note->CommandValue - 1; module->processrow = 0xFFFE; break;
			//IT_JUMP_TO_ORDER would most likely be used to jump back, so it must be handled specially.
			case IT_BREAK_TO_ROW: module->breakrow = note->CommandValue; module->processrow = 0xFFFE; break;
			case IT_VOLUME_SLIDE:
				{
					unsigned char v = note->CommandValue;
					if (v == 0)
						v = channel->lastDKL;
					channel->lastDKL = v;
					if ((v & 0x0F) == 0) { /* Dx0 */
						channel->volslide = v >> 4;
						if (channel->volslide == 15) {
							channel->Volume += 15;
							if (channel->Volume > 64) channel->Volume = 64;
						}
					} else if ((v & 0xF0) == 0) { /* D0x */
						channel->volslide = -v;
						if (channel->volslide == -15) {
							channel->Volume -= 15;
							if (channel->Volume > 64) channel->Volume = 0;
						}
					} else if ((v & 0x0F) == 0x0F) { /* DxF */
						channel->Volume += v >> 4;
						if (channel->Volume > 64) channel->Volume = 64;
					} else if ((v & 0xF0) == 0xF0) { /* DFx */
						channel->Volume -= v & 15;
						if (channel->Volume > 64) channel->Volume = 0;
					}
				}
				break;
			case IT_PORTAMENTO_DOWN:
				{
					unsigned char v = note->CommandValue;
					if (v == 0)
						v = channel->lastEF;
					channel->lastEF = v;
					channel->portamento -= v << 4;
				}
				break;
			case IT_PORTAMENTO_UP:
				{
					unsigned char v = note->CommandValue;
					if (v == 0)
						v = channel->lastEF;
					channel->lastEF = v;
					channel->portamento += v << 4;
				}
				break;
			case IT_TONE_PORTAMENTO:
				{
					unsigned char v = note->CommandValue;
					if (module->Flags & FLAG_COMPATIBLEGXX) {
						if (v == 0)
							v = channel->lastG;
						channel->lastG = v;
					} else {
						if (v == 0)
							v = channel->lastEF;
						channel->lastEF = v;
					}
					channel->toneporta += v << 4;
				}
				break;
			//case IT_VIBRATO:
			//case IT_TREMOR:
			//case IT_ARPEGGIO:
			//case IT_VOLSLIDE_VIBRATO:
			//case IT_VOLSLIDE_TONEPORTA:
			case IT_SET_CHANNEL_VOLUME:
				if (note->CommandValue <= 64)
					channel->ChannelVolume = note->CommandValue;
#ifdef VOLUME_OUT_OF_RANGE_SETS_MAXIMUM
				else
					channel->ChannelVolume = 64;
#endif
				break;
			case IT_CHANNEL_VOLUME_SLIDE:
				{
					unsigned char v = note->CommandValue;
					if (v == 0)
						v = channel->lastN;
					channel->lastN = v;
					if ((v & 0x0F) == 0) { /* Nx0 */
						channel->channelvolslide = v >> 4;
					} else if ((v & 0xF0) == 0) { /* N0x */
						channel->channelvolslide = -v;
					} else if ((v & 0x0F) == 0x0F) { /* NxF */
						channel->ChannelVolume += v >> 4;
						if (channel->ChannelVolume > 64) channel->ChannelVolume = 64;
					} else if ((v & 0xF0) == 0xF0) { /* NFx */
						channel->ChannelVolume -= v & 15;
						if (channel->ChannelVolume > 64) channel->ChannelVolume = 0;
					}
				}
				break;
			//case IT_SET_SAMPLE_OFFSET:
			//case IT_PANNING_SLIDE:
			//case IT_RETRIGGER_NOTE:
			//case IT_TREMOLO:
			case IT_S:
				switch (note->CommandValue >> 4) {
					//case IT_S_SET_FILTER:
					//case IT_S_SET_GLISSANDO_CONTROL:
					//case IT_S_FINETUNE:
					//case IT_S_SET_VIBRATO_WAVEFORM:
					//case IT_S_SET_TREMOLO_WAVEFORM:
					//case IT_S_SET_PANBRELLO_WAVEFORM:
						/* Waveforms for commands S3x, S4x and S5x:
						 *   0: Sine wave
						 *   1: Ramp down
						 *   2: Square wave
						 *   3: Random wave
						 */
					//case IT_S7:
					case IT_S_SET_PAN:
						channel->Pan = ((note->CommandValue & 15) << 2) | ((note->CommandValue & 15) >> 2);
						break;
					//case IT_S_SET_SURROUND_SOUND:
						/* S91 Set surround sound */
					//case IT_S_SET_HIGH_OFFSET:
						/* SAy Set high value of sample offset yxx00h */
					//case IT_S_PATTERN_LOOP:
						/* SB0 Set loopback point */
					//case IT_S_DELAYED_NOTE_CUT:
					//case IT_S_NOTE_DELAY:
						/* SEx Pattern delay for x rows */
					//case IT_S_SET_MIDI_MACRO:
				}
				break;
			case IT_SET_SONG_TEMPO:
				{
					unsigned char v = note->CommandValue;
					if (v == 0)
						v = channel->lastW;
					channel->lastW = v;
					if (v < 0x10)
						module->temposlide = -v;
					else if (v < 0x20)
						module->temposlide = v & 15;
					else
						module->Tempo = v;
				}
				break;
			//case IT_FINE_VIBRATO:
			case IT_SET_GLOBAL_VOLUME:
				if (note->CommandValue <= 128)
					module->GlobalVolume = note->CommandValue;
#ifdef VOLUME_OUT_OF_RANGE_SETS_MAXIMUM
				else
					module->GlobalVolume = 128;
#endif
				break;
			case IT_GLOBAL_VOLUME_SLIDE:
				{
					unsigned char v = note->CommandValue;
					if (v == 0)
						v = channel->lastW;
					channel->lastW = v;
					if ((v & 0x0F) == 0) { /* Nx0 */
						module->globalvolslide = v >> 4;
					} else if ((v & 0xF0) == 0) { /* N0x */
						module->globalvolslide = -v;
					} else if ((v & 0x0F) == 0x0F) { /* NxF */
						module->GlobalVolume += v >> 4;
						if (module->GlobalVolume > 64) module->GlobalVolume = 64;
					} else if ((v & 0xF0) == 0xF0) { /* NFx */
						module->GlobalVolume -= v & 15;
						if (module->GlobalVolume > 64) module->GlobalVolume = 0;
					}
				}
				break;
			case IT_SET_PANNING:
				channel->Pan = (note->CommandValue + 2) >> 2;
				break;
			//case IT_PANBRELLO:
			//case IT_MIDI_MACRO://see MIDI.TXT
		}
	}
}



void update_effects(MODULE *module)
{
	int i;

	module->GlobalVolume += module->globalvolslide;
	if (module->GlobalVolume > 128) {
		if (module->globalvolslide >= 0)
			module->GlobalVolume = 128;
		else
			module->GlobalVolume = 0;
	}

	module->Tempo += module->temposlide;
	if (module->Tempo < 32) {
		if (module->temposlide >= 0)
			module->Tempo = 255;
		else
			module->Tempo = 32;
	}

	for (i = 0; i < 64; i++) {
		MODULE_CHANNEL *channel = &module->Channel[i];

		channel->Volume += channel->volslide;
		if (channel->Volume > 64) {
			if (channel->volslide >= 0)
				channel->Volume = 64;
			else
				channel->Volume = 0;
		}

		channel->ChannelVolume += channel->channelvolslide;
		if (channel->ChannelVolume > 64) {
			if (channel->channelvolslide >= 0)
				channel->ChannelVolume = 64;
			else
				channel->ChannelVolume = 0;
		}

		channel->pitch += channel->portamento;
		//We do not enforce any limits here. IT surely does. Investigate.

		if (channel->toneporta && channel->sample) {
			int destpitch;
			if (module->Flags & FLAG_USEINSTRUMENTS)
				destpitch = module->Instrument[(int)channel->Instrument - 1].NoteNote[channel->Note];
			else
				destpitch = channel->Note;
			destpitch = (destpitch - 60) << 8;
			if (channel->pitch > destpitch) {
				channel->pitch -= channel->toneporta;
				if (channel->pitch < destpitch)
					channel->pitch = destpitch;
			} else if (channel->pitch < destpitch) {
				channel->pitch += channel->toneporta;
				if (channel->pitch > destpitch)
					channel->pitch = destpitch;
			}
		}
	}
}



void reset_effects(MODULE *module)
{
	int i;

	module->globalvolslide = 0;
	module->temposlide = 0;

	for (i = 0; i < 64; i++) {
		module->Channel[i].volslide = 0;
		module->Channel[i].channelvolslide = 0;
		module->Channel[i].portamento = 0;
		module->Channel[i].toneporta = 0;
	}
}



/* This function assumes note->Channel >= 0... or it might later. */
void update_pattern_variables(MODULE *module, MODULE_NOTE *note)
{
	/* MODULE_CHANNEL *channel = &module->Channel[(int)note->Channel]; */

	if (note->Mask & NOTEMASK_COMMAND) {
		switch (note->Command) {
			case IT_S:
				switch (note->CommandValue >> 4) {
					case IT_S_FINE_PATTERN_DELAY: module->tick = module->Speed + (note->CommandValue & 15); break;
					//case IT_S7:
					//case IT_S_PATTERN_LOOP:
						/* SB0 Set loopback point */
					//case IT_S_DELAYED_NOTE_CUT:
					//case IT_S_NOTE_DELAY:
					case IT_S_PATTERN_DELAY: module->rowcount = 1 + (note->CommandValue & 15); break;
				}
				break;
		}
	}
}



unsigned char envelope_get_y(MODULE_ENVELOPE *envelope, MODULE_VENVELOPE *venvelope)
{
	int ys, ye;
	int ts, te;
	int t;

	if (venvelope->NextNode <= 0)
		return envelope->NodeY[0];

	if (venvelope->NextNode >= envelope->NumNodes)
		return envelope->NodeY[envelope->NumNodes - 1];

	ys = envelope->NodeY[venvelope->NextNode - 1];
	ye = envelope->NodeY[venvelope->NextNode];
	ts = envelope->NodeTick[venvelope->NextNode - 1];
	te = envelope->NodeTick[venvelope->NextNode];
	t = venvelope->CurTick;

	if (ts == te)
		return ys;

	return ys + (ye - ys) * (t - ts) / (te - ts);
}



unsigned int calculate_volume(MODULE *module, MODULE_VCHANNEL *vchannel)
{
	if (module->Flags & FLAG_USEINSTRUMENTS) {
		int vev = 64;
		if (module->Instrument[vchannel->Instrument - 1].VolumeEnvelope.Flag & ENVELOPE_ON)
			vev = envelope_get_y(&module->Instrument[vchannel->Instrument - 1].VolumeEnvelope, &vchannel->VVolEnv);
		return (((((((vchannel->Volume *
					  vchannel->sample->GlobalVolume *
					  module->Instrument[vchannel->Instrument - 1].GlobalVolume) >> 7) *
					  module->GlobalVolume) >> 11) *
					  vev *
					  module->MixVolume) >> 10) *
					  vchannel->fadeoutcount) >> 11;
	}

	return (((vchannel->Volume *
			  vchannel->sample->GlobalVolume *
			  module->GlobalVolume) >> 11) *
			  module->MixVolume) >> 5;
}



void process_channels(MODULE *module)
{
	int i;

	for (i = 0; i < 64; i++) {
		MODULE_CHANNEL *channel = &module->Channel[i];
		if (channel->Note < 120) {
			if (channel->Flag & CHANNEL_RETRIG) {
				channel->Flag &= ~CHANNEL_RETRIG;
				if (channel->VChannel) {
					if (!(module->Flags & FLAG_USEINSTRUMENTS) ||
						(channel->VChannel->NNA = module->Instrument[channel->VChannel->Instrument - 1].NewNoteAction)
						== NNA_NOTECUT)
					{
						if (channel->VChannel->Flag & VCHANNEL_PLAYING) {
							write_seqtime();
							sequence_c(SEQUENCE_STOP_SIGNAL);
							sequence_c(channel->VChannel - module->VChannel);
						}
					} else {
						if (channel->VChannel->NNA == NNA_NOTEOFF)
							if ((module->Instrument[channel->VChannel->Instrument - 1].VolumeEnvelope.Flag & (ENVELOPE_ON | ENVELOPE_LOOP_ON)) != ENVELOPE_ON)
								channel->VChannel->NNA = NNA_NOTEFADE;
						if (channel->VChannel->NNA == NNA_NOTEFADE)
							channel->VChannel->Flag |= VCHANNEL_FADING;
						channel->VChannel->Flag |= VCHANNEL_BACKGROUND;
						channel->VChannel->channel = NULL;
						channel->VChannel = NULL;
					}
				}
				if (!channel->VChannel) {
					int k;
					for (k = 0; k < 256; k++) {
						if (!(module->VChannel[k].Flag & VCHANNEL_PLAYING)) {
							channel->VChannel = &module->VChannel[k];
							break;
						}
					}
					if (!channel->VChannel) {
						for (k = 0; k < 256; k++) {
							if (module->VChannel[k].Flag & VCHANNEL_BACKGROUND) {
								write_seqtime();
								sequence_c(SEQUENCE_STOP_SIGNAL);
								sequence_c(k);
								channel->VChannel = &module->VChannel[k];
								break;
							}
						}
					}
				}
				if (channel->VChannel) {
					unsigned char note;
					channel->VChannel->Flag = VCHANNEL_PLAYING | VCHANNEL_RETRIG;
					channel->VChannel->channel = channel;
					channel->VChannel->Note = channel->Note;
					if (module->Flags & FLAG_USEINSTRUMENTS)
						note = module->Instrument[(int)channel->Instrument - 1].NoteNote[channel->Note];
					else
						note = channel->Note;
					channel->pitch = (note - 60) << 8;
					channel->VChannel->pitch = channel->pitch;
					channel->VChannel->Instrument = channel->Instrument;
					channel->VChannel->sample = channel->sample;
					channel->VChannel->VVolEnv.NextNode = 0;
					channel->VChannel->VVolEnv.CurTick = 0;
					channel->VChannel->fadeoutcount = 1024;
					channel->VChannel->NNA = NNA_NOTECONTINUE;
					channel->VChannel->Volume = channel->ChannelVolume * channel->Volume;
					channel->VChannel->Pan = channel->Pan;
					channel->VChannel->PanSep = channel->PanSep;
					/* Note: The DUH* fields are set when writing the start command to the DUH sequence. */
					//BLEARGH: Anything else to initialise?
				}
			}
		} else if (channel->VChannel) {
			if (channel->Note == NOTE_CUT) {
				if (channel->VChannel->Flag & VCHANNEL_PLAYING) {
					write_seqtime();
					sequence_c(SEQUENCE_STOP_SIGNAL);
					sequence_c(channel->VChannel - module->VChannel);
					channel->VChannel->Flag &= ~VCHANNEL_PLAYING;
					channel->VChannel->channel = NULL;
					channel->VChannel = NULL;
				}
			} else {
				channel->VChannel->Flag |= VCHANNEL_BACKGROUND;

				if (channel->Note == NOTE_OFF) {
					if ((module->Instrument[channel->VChannel->Instrument - 1].VolumeEnvelope.Flag & (ENVELOPE_ON | ENVELOPE_LOOP_ON)) != ENVELOPE_ON)
						channel->VChannel->NNA = NNA_NOTEFADE;
					else
						channel->VChannel->NNA = NNA_NOTEOFF;
				} else
					channel->VChannel->NNA = NNA_NOTEFADE;

				if (channel->VChannel->NNA == NNA_NOTEFADE)
					channel->VChannel->Flag |= VCHANNEL_FADING;
				/*
				channel->VChannel->channel = NULL;
				channel->VChannel = NULL;
				*/
			}
		}
	}
}



/* This returns 1 if the envelope finishes. */
int update_envelope(MODULE_VCHANNEL *vchannel, MODULE_ENVELOPE *envelope, MODULE_VENVELOPE *venvelope)
{
	if (!(envelope->Flag & ENVELOPE_ON))
		return 0;

	if (venvelope->NextNode >= envelope->NumNodes)
		return 1;

	while (venvelope->CurTick >= envelope->NodeTick[venvelope->NextNode]) {
		if ((envelope->Flag & ENVELOPE_LOOP_ON) && venvelope->NextNode == envelope->LoopEnd) {
			venvelope->NextNode = envelope->LoopBegin;
			venvelope->CurTick = envelope->NodeTick[envelope->LoopBegin];
			return 0;
		}
		if ((envelope->Flag & ENVELOPE_SUSTAINLOOP) && !(vchannel->Flag & VCHANNEL_BACKGROUND) && venvelope->NextNode == envelope->SustainLoopEnd) {
			venvelope->NextNode = envelope->SustainLoopBegin;
			venvelope->CurTick = envelope->NodeTick[envelope->SustainLoopBegin];
			return 0;
		}
		if (venvelope->NextNode >= envelope->NumNodes)
			return 1;

		venvelope->NextNode++;
	}

	venvelope->CurTick++;

	return 0;
}



/* This assumes the module uses instruments. */
void update_envelopes(MODULE *module, MODULE_VCHANNEL *vchannel)
{
	if (vchannel->Flag & VCHANNEL_PLAYING) {
		MODULE_ENVELOPE *volenv = &module->Instrument[vchannel->Instrument - 1].VolumeEnvelope;

		if (update_envelope(vchannel, volenv, &vchannel->VVolEnv)) {
			if (volenv->NumNodes && volenv->NodeY[volenv->NumNodes - 1] == 0) {
				write_seqtime();
				sequence_c(SEQUENCE_STOP_SIGNAL);
				sequence_c(vchannel - module->VChannel);
				vchannel->Flag = 0;
				if (vchannel->channel)
					vchannel->channel->VChannel = NULL;
				vchannel->channel = NULL;
			} else
				vchannel->Flag |= VCHANNEL_FADING;
		}
	}
}



/* This assumes the module uses instruments. */
void update_fadeout(MODULE *module, MODULE_VCHANNEL *vchannel)
{
	if (vchannel->Flag & VCHANNEL_PLAYING) {
		if (vchannel->Flag & VCHANNEL_FADING) {
			vchannel->fadeoutcount -= module->Instrument[vchannel->Instrument - 1].FadeOut;

			if (vchannel->fadeoutcount <= 0) {
				write_seqtime();
				sequence_c(SEQUENCE_STOP_SIGNAL);
				sequence_c(vchannel - module->VChannel);
				vchannel->Flag = 0;
				if (vchannel->channel)
					vchannel->channel->VChannel = NULL;
				vchannel->channel = NULL;
			}
		}
	}
}



#define INCLUDE_RETRIG
#define INCLUDE_PLAYING
void process_vchannels(MODULE *module)
{
	int i;

	for (i = 0; i < 256; i++) {
		MODULE_VCHANNEL *vchannel = &module->VChannel[i];

		if ((vchannel->Flag & VCHANNEL_PLAYING) && vchannel->channel) {
			vchannel->Volume = vchannel->channel->ChannelVolume * vchannel->channel->Volume;
			vchannel->Pan = vchannel->channel->Pan;
			vchannel->PanSep = vchannel->channel->PanSep;
		}

		if (module->Flags & FLAG_USEINSTRUMENTS) {
			//Update envelopes as required
			update_envelopes(module, vchannel);
			//Update fadeout as required
			update_fadeout(module, vchannel);
			//Calculate final volume if req
			//Calculate final pan if req
			//Process sample vibrato if req
		} else {
			//Calculate final volume if required
			//Calculate final pan if required
			//Process sample vibrato if required
		}

#ifdef INCLUDE_RETRIG
		if (vchannel->Flag & VCHANNEL_RETRIG) {
			vchannel->Flag &= ~VCHANNEL_RETRIG;

			if (vchannel->sample) {
				int C5Speed = vchannel->sample->C5Speed;
				int adjust;

#ifdef STEREO_SAMPLES_COUNT_AS_TWO
				if (vchannel->sample->Flag & SAMPLE_STEREO)
					C5Speed >>= 1;
#endif

				write_seqtime();
				sequence_c(SEQUENCE_START_SIGNAL);
				sequence_c(vchannel - module->VChannel);

				sequence_cl(sample_signal[vchannel->sample - module->Sample]);
				sequence_cl(0); // sample position: 65536 is one second

				vchannel->DUHvol = calculate_volume(module, vchannel);

				sequence_w(vchannel->DUHvol - (vchannel->DUHvol >> 16));

				adjust = floor(12 * 256 *
						(log(C5Speed / 65536.0) / log(2.0)) + 0.5);

				/* pitch */
				sequence_w(adjust + vchannel->pitch);

				vchannel->DUHpan = 32;

			} else
				vchannel->Flag = 0;
		}
#endif

#ifdef INCLUDE_PLAYING
		if (vchannel->Flag & VCHANNEL_PLAYING) {
			//Update to vchannel->channel->stuff if different from vchannel->stuff.
			//stuff would be volume, panning, pitch, etc.
			{
				unsigned int volume = calculate_volume(module, vchannel);
				if (volume != vchannel->DUHvol) {
					vchannel->DUHvol = volume;
					write_seqtime();
					sequence_c(SEQUENCE_SET_VOLUME);
					sequence_c(vchannel - module->VChannel);
					sequence_w(volume - (volume >> 16));
				}
			}
			{
				int pan = vchannel->Pan + vchannel->PanSep;
				if (pan < 0)
					pan = 0;
				else if (pan > 64)
					pan = 64;
				if (pan != vchannel->DUHpan) {
					vchannel->DUHpan = pan;
					write_seqtime();
					sequence_c(SEQUENCE_SET_PARAMETER);
					sequence_c(vchannel - module->VChannel);
					sequence_c(SPANPARAM_PAN);
					sequence_l(((int)pan << 3) - 256);
				}
			}
			if (vchannel->channel) {
				if (vchannel->channel->pitch != vchannel->pitch) {
					int C5Speed = vchannel->sample->C5Speed;
					int adjust;

#ifdef STEREO_SAMPLES_COUNT_AS_TWO
					if (vchannel->sample->Flag & SAMPLE_STEREO)
						C5Speed >>= 1;
#endif

					vchannel->pitch = vchannel->channel->pitch;

					write_seqtime();
					sequence_c(SEQUENCE_SET_PITCH);
					sequence_c(vchannel - module->VChannel);

					adjust = floor(12 * 256 *
							(log(C5Speed / 65536.0) / log(2.0)) + 0.5);

					sequence_w(adjust + vchannel->pitch);
				}
			}

			if (vchannel->Flag & VCHANNEL_BACKGROUND) {
				//Process vchannel->NNA
				//If the note has ended then:
				/*
				{
					write_seqtime();
					sequence_c(SEQUENCE_STOP_SIGNAL);
					sequence_c(vchannel - module->VChannel);
					vchannel->Flag = 0;
				}
				*/
			} else {
				//Process note
			}
		}
#endif
	}
}



void generate_patterns(MODULE *module)
{
	int patnum = module->Order[0];
	int row;

	MODULE_PATTERN *pattern;
	MODULE_NOTE *note, *note_end;

	module->processorder = 0;
	module->processrow = 0;
	module->breakrow = 0;
	module->rowcount = 1;

	module->tick = module->Speed;

	goto start_the_loop; /* This ungainly goto will be averted later. */

	for (;;) {
		// Set note vol/freq to vol/freq set for each channel

		module->tick--;
		if (module->tick == 0) {
			module->tick = module->Speed;
			module->rowcount--;
			if (module->rowcount == 0) {
				module->rowcount = 1;
				module->processrow++;
				if (module->processrow >= pattern->NumRows) {
					module->processrow = module->breakrow;
					module->breakrow = 0;
					module->processorder++;

					start_the_loop:

					for (; ; module->processorder++) {
						if (module->processorder >= module->NumOrders)
							return;

						patnum = module->Order[module->processorder];

						if (patnum < module->NumPatterns)
							break;

#ifdef INVALID_ORDERS_END_SONG
						if (patnum != ORDER_SKIP)
							return;
#else
						if (patnum == ORDER_END)
							return;
#endif
					}

					printf("  Order %i, Pattern %i\n", module->processorder, patnum);

					pattern = &module->Pattern[patnum];

				 	note = pattern->Note;
				 	note_end = note + pattern->NumNotes;
				}

				row = module->processrow;

				{
					MODULE_NOTE *note_start = note;

					while (note < note_end && note->Channel >= 0)
						update_pattern_variables(module, note++);

					note = note_start;
				}

				reset_effects(module);

				//Should this code go before updating the pattern variables?
				while (note < note_end) {
 					if (note->Channel < 0) {
						note++;
						break;
					}
					process_note_data(module, note++);
				}

			} else
				update_effects(module);
		} else
			update_effects(module);

		process_channels(module);

		/* Output sound!!! */
		process_vchannels(module);

		/* Increment time by one tick. */
		{
			int inc = ((65536.0 * 60.0) / (4.0 * 6.0)) / module->Tempo;
			seqtime += inc;
			songtime += inc;
		}
	}
}



void stop_notes(MODULE *module)
{
	int c;
	for (c = 0; c < 256; c++) {
		if (module->VChannel[c].Flag & VCHANNEL_PLAYING) {
			write_seqtime();
			sequence_c(SEQUENCE_STOP_SIGNAL);
			sequence_c(c);
		}
	}
}



void generate_sequence(MODULE *module)
{
	int i;

	for (i = 0; i < 64; i++) {
		module->Channel[i].Note = 255;
		module->Channel[i].Instrument = 0;
		module->Channel[i].sample = NULL;
		module->Channel[i].VChannel = NULL;
		module->Channel[i].lastvolslide = 0;
		module->Channel[i].lastDKL = 0;
		module->Channel[i].lastN = 0;
		module->Channel[i].lastW = 0;
		module->Channel[i].lastEF = 0;
		module->Channel[i].lastG = 0;
		module->Channel[i].lastT = 0;
	}

	for (i = 0; i < 256; i++)
		module->VChannel[i].Flag = 0;

	songtime = 0;

	generate_patterns(module);
/*
	for (i = 0; i < module->NumOrders; i++) {

		if (module->Order[i] < module->NumPatterns)
			write_pattern(module, &module->Pattern[module->Order[i]]);
		else {
#ifdef INVALID_ORDERS_END_SONG
			if (module->Order[i] != ORDER_SKIP)
				break;
#else
			if (module->Order[i] == ORDER_END)
				break;
#endif
		}
	}
*/
	stop_notes(module);

	/* Negative time to indicate the end of the sequence. */
	sequence_cl(-1);

	if (errno)
		return;
}



void write_sequence(PACKFILE *f)
{
	pack_mputl(SIGTYPE_SEQUENCE, f);

	pack_iputl(seqpos, f);
	pack_fwrite(seqdata, seqpos, f);
}



void free_sequence()
{
	free(seqdata);
	seqdata = NULL;
	seqpos = 0;
	max_seqdata = 0;
	seqtime = 0;
}



int save_it_to_duh(MODULE *module, const char *filename)
{
	int i;
	int n_samples = 0;

	PACKFILE *f = pack_fopen(filename, F_WRITE_PACKED);
	if (!f) return 1;

	/* Write signature. */
	pack_mputl(DUH_SIGNATURE, f);

	/* Create array of true samples. */
	sample_signal = malloc(module->NumSamples * sizeof(*sample_signal));

	for (i = 0; i < module->NumSamples; i++) {
		if (sample_is_valid(&module->Sample[i])) {
			sample_signal[i] = first_sample + n_samples;
			if (module->Sample[i].Flag & SAMPLE_STEREO)
				n_samples += 3; /* COMB,SAMP,SAMP */
			else
				n_samples += 2; /* SPAN,SAMP */
		} else
			sample_signal[i] = -1;
	}

	printf("Generating sequence\n");
	generate_sequence(module);

	/* Write length of song. */
	pack_iputl(songtime, f);

	/* Write number of signals - one for the sequence, one for each sample. */
	pack_iputl(1 + n_samples, f);

	if (errno) {pack_fclose(f); free(sample_signal); return 1;}

	printf("Writing sequence\n");
	write_sequence(f);
	free_sequence();

	if (errno) {pack_fclose(f); free(sample_signal); return 1;}

	printf("Writing samples");
	n_samples = 0;
	for (i = 0; i < module->NumSamples; i++) {
		if ((i & 7) == 0)
			printf("\n  ");
		printf("%4d", i + 1);

		if (sample_is_valid(&module->Sample[i])) {

			if (module->Sample[i].Flag & SAMPLE_STEREO) {
				write_stereo_sample(&module->Sample[i], n_samples, f);
				n_samples += 3;
			} else {
				write_mono_sample(&module->Sample[i], n_samples, f);
				n_samples += 2;
			}

			if (errno) {pack_fclose(f); free(sample_signal); return 1;}
		}
	}
	printf("\n");

	free(sample_signal);
	sample_signal = NULL;

	pack_fclose(f);

	if (errno) {pack_fclose(f); return 1;}

	return 0;
}



#undef first_sample



void usage()
{
	allegro_message(
		"Usage: cit.exe filename.it [filename.duh]\n"
		"Converts an Impulse Tracker file into a Dynamic Universal Harmony.\n"
	);

	exit(1);
}



int main(int argc, char *argv[])
{
	MODULE *module;
	int rv;
	char *outmod;

	if (argc < 2 || argc > 3)
		usage();

	allegro_init();
	
	/* Make the output file name if it wasn't specified */
	if (argc == 2) {
		int size = sizeof(char) * (ustrsizez(argv[1]) + 32);
		outmod = malloc(size);
		if (!outmod) {
			allegro_message("Ran out of memory while trying to allocate %i bytes. (%s:(%i))\n", size, __FILE__, __LINE__);
			return -1;
		}
		ustrzcpy(outmod, size, argv[1]);
		replace_extension(outmod, argv[1], "duh", size);
	}
	else
		outmod = argv[2];

	module = load_it(argv[1]);

	if (!module) {
		allegro_exit();
		allegro_message("Unable to load %s!\n", argv[1]);
		return 1;
	}

	rv = save_it_to_duh(module, outmod);

	destroy_it(module);

	if (rv) {
		delete_file(outmod);
		allegro_exit();
		allegro_message("Unable to save %s!\n", outmod);
		return 1;
	}
	
	if (argc == 2 && outmod)
		free(outmod);

	return 0;
}
END_OF_MAIN();
