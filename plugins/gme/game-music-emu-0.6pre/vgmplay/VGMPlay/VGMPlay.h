// Header File for structures and constants used within VGMPlay.c

#include "chips/mamedef.h"

#include "VGMFile.h"

#include "VGMPlay_Intf.h"

#define VGMPLAY_VER_STR	"0.40.7"
//#define APLHA
//#define BETA
#define VGM_VER_STR		"1.71b"
#define VGM_VER_NUM		0x170

#define CHIP_COUNT	0x29
typedef struct chip_options
{
	bool Disabled;
	UINT8 EmuCore;
	UINT8 ChnCnt;
	// Special Flags:
	//	YM2612:	Bit 0 - DAC Highpass Enable, Bit 1 - SSG-EG Enable
	//	YM-OPN:	Bit 0 - Disable AY8910-Part
	UINT16 SpecialFlags;

	// Channel Mute Mask - 1 Channel is represented by 1 bit
	UINT32 ChnMute1;
	// Mask 2 - used by YMF287B for OPL4 Wavetable Synth and by YM2608/YM2610 for PCM
	UINT32 ChnMute2;
	// Mask 3 - used for the AY-part of some OPN-chips
	UINT32 ChnMute3;

	INT16* Panning;
} CHIP_OPTS;
typedef struct chips_options
{
	CHIP_OPTS SN76496;
	CHIP_OPTS YM2413;
	CHIP_OPTS YM2612;
	CHIP_OPTS YM2151;
	CHIP_OPTS SegaPCM;
	CHIP_OPTS RF5C68;
	CHIP_OPTS YM2203;
	CHIP_OPTS YM2608;
	CHIP_OPTS YM2610;
	CHIP_OPTS YM3812;
	CHIP_OPTS YM3526;
	CHIP_OPTS Y8950;
	CHIP_OPTS YMF262;
	CHIP_OPTS YMF278B;
	CHIP_OPTS YMF271;
	CHIP_OPTS YMZ280B;
	CHIP_OPTS RF5C164;
	CHIP_OPTS PWM;
	CHIP_OPTS AY8910;
	CHIP_OPTS GameBoy;
	CHIP_OPTS NES;
	CHIP_OPTS MultiPCM;
	CHIP_OPTS UPD7759;
	CHIP_OPTS OKIM6258;
	CHIP_OPTS OKIM6295;
	CHIP_OPTS K051649;
	CHIP_OPTS K054539;
	CHIP_OPTS HuC6280;
	CHIP_OPTS C140;
	CHIP_OPTS K053260;
	CHIP_OPTS Pokey;
	CHIP_OPTS QSound;
	CHIP_OPTS SCSP;
	CHIP_OPTS WSwan;
	CHIP_OPTS VSU;
	CHIP_OPTS SAA1099;
	CHIP_OPTS ES5503;
	CHIP_OPTS ES5506;
	CHIP_OPTS X1_010;
	CHIP_OPTS C352;
	CHIP_OPTS GA20;
//	CHIP_OPTS OKIM6376;
} CHIPS_OPTION;

typedef void (*strm_func)(void *, stream_sample_t **outputs, int samples);

typedef struct chip_audio_attributes CAUD_ATTR;
struct chip_audio_attributes
{
    UINT32 TargetSmpRate;
    UINT32 SmpRate;
		UINT32 LastSmpRate;
    UINT16 Volume;
    UINT8 ChipType;
    UINT8 ChipID;		// 0 - 1st chip, 1 - 2nd chip, etc.
		void* Resampler;
    strm_func StreamUpdate;
    void* StreamUpdateParam;
    CAUD_ATTR* Paired;
};

typedef struct chip_audio_struct
{
    CAUD_ATTR SN76496;
    CAUD_ATTR YM2413;
    CAUD_ATTR YM2612;
    CAUD_ATTR YM2151;
    CAUD_ATTR SegaPCM;
    CAUD_ATTR RF5C68;
    CAUD_ATTR YM2203;
    CAUD_ATTR YM2608;
    CAUD_ATTR YM2610;
    CAUD_ATTR YM3812;
    CAUD_ATTR YM3526;
    CAUD_ATTR Y8950;
    CAUD_ATTR YMF262;
    CAUD_ATTR YMF278B;
    CAUD_ATTR YMF271;
    CAUD_ATTR YMZ280B;
    CAUD_ATTR RF5C164;
    CAUD_ATTR PWM;
    CAUD_ATTR AY8910;
    CAUD_ATTR GameBoy;
    CAUD_ATTR NES;
    CAUD_ATTR MultiPCM;
    CAUD_ATTR UPD7759;
    CAUD_ATTR OKIM6258;
    CAUD_ATTR OKIM6295;
    CAUD_ATTR K051649;
    CAUD_ATTR K054539;
    CAUD_ATTR HuC6280;
    CAUD_ATTR C140;
    CAUD_ATTR K053260;
    CAUD_ATTR Pokey;
    CAUD_ATTR QSound;
    CAUD_ATTR SCSP;
    CAUD_ATTR WSwan;
    CAUD_ATTR VSU;
    CAUD_ATTR SAA1099;
    CAUD_ATTR ES5503;
    CAUD_ATTR ES5506;
    CAUD_ATTR X1_010;
    CAUD_ATTR C352;
    CAUD_ATTR GA20;
    //	CAUD_ATTR OKIM6376;
} CHIP_AUDIO;

typedef struct chip_aud_list CA_LIST;
struct chip_aud_list
{
    CAUD_ATTR* CAud;
    CHIP_OPTS* COpts;
    CA_LIST* next;
};

typedef struct daccontrol_data
{
    bool Enable;
    UINT8 Bank;
} DACCTRL_DATA;

typedef struct pcmbank_table
{
    UINT8 ComprType;
    UINT8 CmpSubType;
    UINT8 BitDec;
    UINT8 BitCmp;
    UINT16 EntryCount;
    void* Entries;
} PCMBANK_TBL;

typedef struct vgm_player
{
    // Options Variables
    UINT32 SampleRate;	// Note: also used by some sound cores to determinate the chip sample rate

    UINT32 VGMMaxLoop;
    UINT32 VGMPbRate;	// in Hz, ignored if this value or VGM's lngRate Header value is 0
#ifdef ADDITIONAL_FORMATS
    UINT32 CMFMaxLoop;
#endif
    UINT32 FadeTime;

    float VolumeLevel;
    bool SurroundSound;
    UINT8 HardStopOldVGMs;
    bool FadeRAWLog;
    //bool FullBufFill;	// Fill Buffer until it's full

    bool DoubleSSGVol;

    UINT8 ResampleMode;	// 00 - HQ both, 01 - LQ downsampling, 02 - LQ both
    UINT8 CHIP_SAMPLING_MODE;
    INT32 CHIP_SAMPLE_RATE;

    CHIPS_OPTION ChipOpts[0x02];

    stream_sample_t* DUMMYBUF[0x02];

    char* AppPaths[8];

    UINT8 FileMode;
    VGM_HEADER VGMHead;
    VGM_HDR_EXTRA VGMHeadX;
    VGM_EXTRA VGMH_Extra;
    UINT32 VGMDataLen;
    UINT8* VGMData;
    GD3_TAG VGMTag;

#define PCM_BANK_COUNT	0x40
    VGM_PCM_BANK PCMBank[PCM_BANK_COUNT];
    PCMBANK_TBL PCMTbl;
    UINT8 DacCtrlUsed;
    UINT8 DacCtrlUsg[0xFF];
    DACCTRL_DATA DacCtrl[0xFF];

    CHIP_AUDIO ChipAudio[0x02];
    CAUD_ATTR CA_Paired[0x02][0x03];
    float MasterVol;

    CA_LIST ChipListBuffer[0x200];
    CA_LIST* ChipListAll;	// all chips needed for playback (in general)
    //CA_LIST* ChipListOpt;	// ChipListAll minus muted chips

#define SMPL_BUFSIZE	0x100
    INT32* StreamBufs[0x02];

    UINT32 VGMPos;
    INT32 VGMSmplPos;
    INT32 VGMSmplPlayed;
    INT32 VGMSampleRate;
    UINT32 VGMPbRateMul;
    UINT32 VGMPbRateDiv;
    UINT32 VGMSmplRateMul;
    UINT32 VGMSmplRateDiv;
    bool VGMEnd;
    bool EndPlay;
    bool FadePlay;
    bool ForceVGMExec;
    UINT8 PlayingMode;
    UINT32 PlayingTime;
    UINT32 FadeStart;
    UINT32 VGMMaxLoopM;
    UINT32 VGMCurLoop;
    float VolumeLevelM;
    float FinalVol;
    bool ResetPBTimer;

    UINT8 IsVGMInit;
    UINT16 Last95Drum;	// for optvgm debugging
    UINT16 Last95Max;	// for optvgm debugging
    UINT32 Last95Freq;	// for optvgm debugging

    bool ErrorHappened;

    // the chips' states
    void * sn764xx[2];
    void * ym2413[2];
    void * ym2612[2];
    void * ym2151[2];
    void * segapcm[2];
    void * rf5c68;
    void * ym2203[2];
    void * ym2608[2];
    void * ym2610[2];
    void * ym3812[2];
    void * ym3812_dual_data[2];
    void * ym3526[2];
    void * y8950[2];
    void * ymf262[2];
    void * ymf278b[2];
    void * ymf271[2];
    void * ymz280b[2];
    void * rf5c164;
    void * pwm;
    void * ay8910[2];
    void * gbdmg[2];
    void * nesapu[2];
    void * multipcm[2];
    void * upd7759[2];
    void * okim6258[2];
    void * okim6295[2];
    void * k051649[2];
    void * k054539[2];
    void * huc6280[2];
    void * c140[2];
    void * k053260[2];
    void * pokey[2];
    void * qsound[2];
    void * scsp[2];
    void * wswan[2];
    void * vsu[2];
    void * saa1099[2];
    void * es5503[2];
    void * es550x[2];
    void * x1_010[2];
    void * c352[2];
    void * ga20[2];
    void * daccontrol[255];
} VGM_PLAYER;
