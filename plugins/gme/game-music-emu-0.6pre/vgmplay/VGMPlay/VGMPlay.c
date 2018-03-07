// VGMPlay.c: C Source File of the Main Executable
//

// Line Size:	96 Chars
// Tab Size:	4 Spaces

/*3456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
0000000001111111111222222222233333333334444444444555555555566666666667777777777888888888899999*/
// TODO: Callback "ChangeSampleRate" to fix YM2203's AY8910

// Mixer Muting ON:
//		Mixer's FM Volume is set to 0 or Mute	-> absolutely muted
//		(sometimes it can take some time to get the Mixer Control under Windows)
// Mixer Muting OFF:
//		FM Volume is set to 0 through commands	-> very very low volume level ~0.4%
//		(faster way)
//#define MIXER_MUTING

// These defines enable additional features.
//	ADDITIONAL_FORMATS enables CMF and DRO support.
//	CONSOLE_MODE switches between VGMPlay and in_vgm mode.
//	in_vgm mode can also be used for custom players.
//
//#define ADDITIONAL_FORMATS
//#define CONSOLE_MODE
//#define VGM_BIG_ENDIAN

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "stdbool.h"
#include <math.h>	// for pow()

#ifndef NO_ZLIB
#include <zlib.h>
#endif

#include "resampler.h"

#include "chips/mamedef.h"

// integer types for fast integer calculation
// the bit number is unused (it's an orientation)
#define FUINT8	unsigned int
#define FUINT16	unsigned int

#include "VGMPlay.h"
//#include "VGMPlay_Intf.h" // Already included by VGMPlay.h now

#include "chips/ChipIncl.h"

unsigned char OpenPortTalk(void *);
void ClosePortTalk(void *);

#include "ChipMapper.h"



// Function Prototypes (prototypes in comments are defined in VGMPlay_Intf.h)
//void VGMPlay_Init(void);
//void VGMPlay_Init2(void);
//void VGMPlay_Deinit(void);

INLINE UINT16 ReadLE16(const UINT8* Data);
INLINE UINT16 ReadBE16(const UINT8* Data);
INLINE UINT32 ReadLE24(const UINT8* Data);
INLINE UINT32 ReadLE32(const UINT8* Data);
INLINE int FILE_getLE16(VGM_FILE* hFile, UINT16* RetValue);
INLINE int FILE_getLE32(VGM_FILE* hFile, UINT32* RetValue);
static UINT32 gcd(UINT32 x, UINT32 y);
//void PlayVGM(void);
//void StopVGM(void);
//void RestartVGM(void);
//void PauseVGM(bool Pause);
//void SeekVGM(bool Relative, INT32 PlayBkSamples);
//void RefreshMuting(void);
//void RefreshPanning(void);
//void RefreshPlaybackOptions(void);

//UINT32 GetGZFileLength(const char* FileName);
//UINT32 GetGZFileLengthW(const wchar_t* FileName);
static UINT32 GetGZFileLength_Internal(FILE* hFile);
//bool OpenVGMFile(const char* FileName);
static bool OpenVGMFile_Internal(VGM_PLAYER*, VGM_FILE* hFile, UINT32 FileSize);
static void ReadVGMHeader(VGM_FILE* hFile, VGM_HEADER* RetVGMHead);
static UINT8 ReadGD3Tag(VGM_FILE* hFile, UINT32 GD3Offset, GD3_TAG* RetGD3Tag);
static void ReadChipExtraData32(VGM_PLAYER*, UINT32 StartOffset, VGMX_CHP_EXTRA32* ChpExtra);
static void ReadChipExtraData16(VGM_PLAYER*, UINT32 StartOffset, VGMX_CHP_EXTRA16* ChpExtra);
//void CloseVGMFile(void);
//void FreeGD3Tag(GD3_TAG* TagData);
static wchar_t* MakeEmptyWStr(void);
static wchar_t* ReadWStrFromFile(VGM_FILE* hFile, UINT32* FilePos, UINT32 EOFPos);
//UINT32 GetVGMFileInfo(const char* FileName, VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag);
static UINT32 GetVGMFileInfo_Internal(VGM_FILE* hFile, UINT32 FileSize,
									  VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag);
INLINE UINT32 MulDivRound(UINT64 Number, UINT64 Numerator, UINT64 Denominator);
//UINT32 CalcSampleMSec(VGM_PLAYER* p, UINT64 Value, UINT8 Mode);
//UINT32 CalcSampleMSecExt(VGM_PLAYER* p, UINT64 Value, UINT8 Mode, VGM_HEADER* FileHead);
//const char* GetChipName(UINT8 ChipID);
//const char* GetAccurateChipName(UINT8 ChipID, UINT8 SubType);
//UINT32 GetChipClock(void*, UINT8 ChipID, UINT8* RetSubType);
static UINT16 GetChipVolume(VGM_PLAYER*, UINT8 ChipID, UINT8 ChipNum, UINT8 ChipCnt);

static void RestartPlaying(VGM_PLAYER*);
static void Chips_GeneralActions(VGM_PLAYER*, UINT8 Mode);

INLINE INT32 SampleVGM2Pbk_I(VGM_PLAYER*, INT32 SampleVal);	// inline functions
INLINE INT32 SamplePbk2VGM_I(VGM_PLAYER*, INT32 SampleVal);
//INT32 SampleVGM2Playback(void*, INT32 SampleVal);		// non-inline functions
//INT32 SamplePlayback2VGM(void*, INT32 SampleVal);
//static bool SetMuteControl(VGM_PLAYER*, bool mute);

static void InterpretFile(VGM_PLAYER*, UINT32 SampleCount);
static void AddPCMData(VGM_PLAYER*, UINT8 Type, UINT32 DataSize, const UINT8* Data);
//INLINE FUINT16 ReadBits(UINT8* Data, UINT32* Pos, FUINT8* BitPos, FUINT8 BitsToRead);
static bool DecompressDataBlk(VGM_PLAYER* p, VGM_PCM_DATA* Bank, UINT32 DataSize, const UINT8* Data);
static UINT8 GetDACFromPCMBank(VGM_PLAYER*);
static UINT8* GetPointerFromPCMBank(VGM_PLAYER*, UINT8 Type, UINT32 DataPos);
static void ReadPCMTable(VGM_PLAYER*, UINT32 DataSize, const UINT8* Data);
static void InterpretVGM(VGM_PLAYER*, UINT32 SampleCount);
#ifdef ADDITIONAL_FORMATS
extern void InterpretOther(VGM_PLAYER*, UINT32 SampleCount);
#endif

static void GeneralChipLists(VGM_PLAYER*);
static void SetupResampler(VGM_PLAYER*, CAUD_ATTR* CAA);
static void ChangeChipSampleRate(void* DataPtr, UINT32 NewSmplRate);

INLINE INT16 Limit2Short(INT32 Value);
static void null_update(void *param, stream_sample_t **outputs, int samples);
struct dual_opl2_info
{
    void * chip;
    int ChipID;
};
static void dual_opl2_stereo(void *param, stream_sample_t **outputs, int samples);
static void ResampleChipStream(VGM_PLAYER*, CA_LIST* CLst, WAVE_32BS* RetSample, UINT32 Length);
static INT32 RecalcFadeVolume(VGM_PLAYER*);
//UINT32 FillBuffer(void *, WAVE_16BS* Buffer, UINT32 BufferSize)

// Options and such moved to VGM_PLAYER structure

void * VGMPlay_Init(void)
{
	UINT8 CurChip;
	UINT8 CurCSet;
	UINT8 CurChn;
	CHIP_OPTS* TempCOpt;
	CAUD_ATTR* TempCAud;

    VGM_PLAYER* p = (VGM_PLAYER*) calloc(1, sizeof(*p));
    if (!p)
        return NULL;

	p->SampleRate = 44100;
	p->FadeTime = 5000;

	p->HardStopOldVGMs = 0x00;
	p->FadeRAWLog = false;
	p->VolumeLevel = 1.0f;
	//p->FullBufFill = false;
	p->SurroundSound = false;
	p->VGMMaxLoop = 0x02;
	p->VGMPbRate = 0;
#ifdef ADDITIONAL_FORMATS
	p->CMFMaxLoop = 0x01;
#endif
	p->ResampleMode = 0x00;
	p->CHIP_SAMPLING_MODE = 0x00;
	p->CHIP_SAMPLE_RATE = 0x00000000;
	p->DoubleSSGVol = false;

	for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
	{
		TempCAud = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
		for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, TempCAud ++)
		{
			TempCOpt = (CHIP_OPTS*)&p->ChipOpts[CurCSet] + CurChip;

			TempCOpt->Disabled = false;
			TempCOpt->EmuCore = 0x00;
			TempCOpt->SpecialFlags = 0x00;
			TempCOpt->ChnCnt = 0x00;
			TempCOpt->ChnMute1 = 0x00;
			TempCOpt->ChnMute2 = 0x00;
			TempCOpt->ChnMute3 = 0x00;
			TempCOpt->Panning = NULL;

			// Set up some important fields to prevent in_vgm from crashing
			// when clicking on Muting checkboxes after init.
			TempCAud->ChipType = 0xFF;
			TempCAud->ChipID = CurCSet;
			TempCAud->Paired = NULL;
		}
		p->ChipOpts[CurCSet].GameBoy.SpecialFlags = 0x0003;
		// default options, 0x8000 skips the option write and keeps NSFPlay's default values
		p->ChipOpts[CurCSet].NES.SpecialFlags = 0x8000 |
										(0x00 << 12) | (0x3B << 4) | (0x01 << 2) | (0x03 << 0);

		TempCAud = p->CA_Paired[CurCSet];
		for (CurChip = 0x00; CurChip < 0x03; CurChip ++, TempCAud ++)
		{
			TempCAud->ChipType = 0xFF;
			TempCAud->ChipID = CurCSet;
			TempCAud->Paired = NULL;
		}

		// currently the only chips with Panning support are
		// SN76496 and YM2413, it should be not a problem that it's hardcoded.
		TempCOpt = (CHIP_OPTS*)&p->ChipOpts[CurCSet].SN76496;
		TempCOpt->ChnCnt = 0x04;
		TempCOpt->Panning = (INT16*)malloc(sizeof(INT16) * TempCOpt->ChnCnt);
		for (CurChn = 0x00; CurChn < TempCOpt->ChnCnt; CurChn ++)
			TempCOpt->Panning[CurChn] = 0x00;

		TempCOpt = (CHIP_OPTS*)&p->ChipOpts[CurCSet].YM2413;
		TempCOpt->ChnCnt = 0x0E;	// 0x09 + 0x05
		TempCOpt->Panning = (INT16*)malloc(sizeof(INT16) * TempCOpt->ChnCnt);
		for (CurChn = 0x00; CurChn < TempCOpt->ChnCnt; CurChn ++)
			TempCOpt->Panning[CurChn] = 0x00;
	}

	p->FileMode = 0xFF;

	return p;
}

void VGMPlay_Init2(void *_p)
{
    VGM_PLAYER* p = (VGM_PLAYER*)_p;
	// has to be called after the configuration is loaded

	p->StreamBufs[0x00] = (INT32*)malloc(SMPL_BUFSIZE * sizeof(INT32));
	p->StreamBufs[0x01] = (INT32*)malloc(SMPL_BUFSIZE * sizeof(INT32));

	if (p->CHIP_SAMPLE_RATE <= 0)
		p->CHIP_SAMPLE_RATE = p->SampleRate;
	p->PlayingMode = 0xFF;

	return;
}

void VGMPlay_Deinit(void *_p)
{
	UINT8 CurChip;
	UINT8 CurCSet;
	CHIP_OPTS* TempCOpt;

    VGM_PLAYER* p = (VGM_PLAYER*)_p;

	free(p->StreamBufs[0x00]);	p->StreamBufs[0x00] = NULL;
	free(p->StreamBufs[0x01]);	p->StreamBufs[0x01] = NULL;

	for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
	{
		for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++)
		{
			TempCOpt = (CHIP_OPTS*)&p->ChipOpts[CurCSet] + CurChip;

			if (TempCOpt->Panning != NULL)
			{
				free(TempCOpt->Panning);	TempCOpt->Panning = NULL;
			}
		}
	}

    free(p);

	return;
}

INLINE UINT16 ReadLE16(const UINT8* Data)
{
	// read 16-Bit Word (Little Endian/Intel Byte Order)
#ifndef VGM_BIG_ENDIAN
	return *(UINT16*)Data;
#else
	return (Data[0x01] << 8) | (Data[0x00] << 0);
#endif
}

INLINE UINT16 ReadBE16(const UINT8* Data)
{
	// read 16-Bit Word (Big Endian/Motorola Byte Order)
#ifndef VGM_BIG_ENDIAN
	return (Data[0x00] << 8) | (Data[0x01] << 0);
#else
	return *(UINT16*)Data;
#endif
}

INLINE UINT32 ReadLE24(const UINT8* Data)
{
	// read 24-Bit Word (Little Endian/Intel Byte Order)
#ifndef VGM_BIG_ENDIAN
	return	(*(UINT32*)Data) & 0x00FFFFFF;
#else
	return	(Data[0x02] << 16) | (Data[0x01] <<  8) | (Data[0x00] <<  0);
#endif
}

INLINE UINT32 ReadLE32(const UINT8* Data)
{
	// read 32-Bit Word (Little Endian/Intel Byte Order)
#ifndef VGM_BIG_ENDIAN
	return	*(UINT32*)Data;
#else
	return	(Data[0x03] << 24) | (Data[0x02] << 16) |
			(Data[0x01] <<  8) | (Data[0x00] <<  0);
#endif
}

INLINE int FILE_getLE16(VGM_FILE* hFile, UINT16* RetValue)
{
#ifndef VGM_BIG_ENDIAN
	return hFile->Read(hFile, RetValue, 0x02);
#else
	int RetVal;
	UINT8 Data[0x02];

	RetVal = hFile->Read(hFile, Data, 0x02);
	*RetValue =	(Data[0x01] << 8) | (Data[0x00] << 0);
	return RetVal;
#endif
}

INLINE int FILE_getLE32(VGM_FILE* hFile, UINT32* RetValue)
{
#ifndef VGM_BIG_ENDIAN
	return hFile->Read(hFile, RetValue, 0x04);
#else
	int RetVal;
	UINT8 Data[0x04];

	RetVal = hFime->Read(hFile, Data, 0x04);
	*RetValue =	(Data[0x03] << 24) | (Data[0x02] << 16) |
				(Data[0x01] <<  8) | (Data[0x00] <<  0);
	return RetVal;
#endif
}

static UINT32 gcd(UINT32 x, UINT32 y)
{
	UINT32 shift;
	UINT32 diff;

	// Thanks to Wikipedia for this algorithm
	// http://en.wikipedia.org/wiki/Binary_GCD_algorithm
	if (! x || ! y)
		return x | y;

	for (shift = 0; ((x | y) & 1) == 0; shift ++)
	{
		x >>= 1;
		y >>= 1;
	}

	while((x & 1) == 0)
		x >>= 1;

	do
	{
		while((y & 1) == 0)
			y >>= 1;

		if (x < y)
		{
			y -= x;
		}
		else
		{
			diff = x - y;
			x = y;
			y = diff;
		}
		y >>= 1;
	} while(y);

	return x << shift;
}

void PlayVGM(void *_p)
{
	/*UINT8 CurChip;*/
	/*UINT8 FMVal;*/
	INT32 TempSLng;

    VGM_PLAYER* p = (VGM_PLAYER*)_p;

	if (p->PlayingMode != 0xFF)
		return;

	p->FadePlay = false;
	p->MasterVol = 1.0f;
	p->ForceVGMExec = false;
	p->FadeStart = 0;
	p->ForceVGMExec = true;

    p->PlayingMode = 0x00;	// Normal Mode

	if (p->VGMHead.bytVolumeModifier <= VOLUME_MODIF_WRAP)
		TempSLng = p->VGMHead.bytVolumeModifier;
	else if (p->VGMHead.bytVolumeModifier == (VOLUME_MODIF_WRAP + 0x01))
		TempSLng = VOLUME_MODIF_WRAP - 0x100;
	else
		TempSLng = p->VGMHead.bytVolumeModifier - 0x100;
	p->VolumeLevelM = (float)(p->VolumeLevel * pow(2.0, TempSLng / (double)0x20));
	p->FinalVol = p->VolumeLevelM;

	if (! p->VGMMaxLoop)
	{
		p->VGMMaxLoopM = 0x00;
	}
	else
	{
		TempSLng = (p->VGMMaxLoop * p->VGMHead.bytLoopModifier + 0x08) / 0x10 - p->VGMHead.bytLoopBase;
		p->VGMMaxLoopM = (TempSLng >= 0x01) ? TempSLng : 0x01;
	}

	if (! p->VGMPbRate || ! p->VGMHead.lngRate)
	{
		p->VGMPbRateMul = 1;
		p->VGMPbRateDiv = 1;
	}
	else
	{
		// I prefer small Multiplers and Dividers, as they're used very often
		TempSLng = gcd(p->VGMHead.lngRate, p->VGMPbRate);
		p->VGMPbRateMul = p->VGMHead.lngRate / TempSLng;
		p->VGMPbRateDiv = p->VGMPbRate / TempSLng;
	}
	p->VGMSmplRateMul = p->SampleRate * p->VGMPbRateMul;
	p->VGMSmplRateDiv = p->VGMSampleRate * p->VGMPbRateDiv;
	// same as above - to speed up the VGM <-> Playback calculation
	TempSLng = gcd(p->VGMSmplRateMul, p->VGMSmplRateDiv);
	p->VGMSmplRateMul /= TempSLng;
	p->VGMSmplRateDiv /= TempSLng;

	p->PlayingTime = 0;
	p->EndPlay = false;

	p->VGMPos = p->VGMHead.lngDataOffset;
	p->VGMSmplPos = 0;
	p->VGMSmplPlayed = 0;
	p->VGMEnd = false;
	p->VGMCurLoop = 0x00;
	if (p->VGMPos >= p->VGMHead.lngEOFOffset)
		p->VGMEnd = true;

	Chips_GeneralActions(p, 0x00);	// Start chips
	// also does Reset (0x01), Muting Mask (0x10) and Panning (0x20)

	p->Last95Drum = 0xFFFF;
	p->Last95Freq = 0;
	p->Last95Max = 0xFFFF;
	p->IsVGMInit = true;
    p->ErrorHappened = false;
	InterpretFile(p, 0);
	p->IsVGMInit = false;

	p->ForceVGMExec = false;

	return;
}

void StopVGM(void *_p)
{
    VGM_PLAYER* p = (VGM_PLAYER*)_p;
	if (p->PlayingMode == 0xFF)
		return;

	Chips_GeneralActions(p, 0x02);	// Stop chips
	p->PlayingMode = 0xFF;

	return;
}

void RestartVGM(void *_p)
{
    VGM_PLAYER* p = (VGM_PLAYER*)_p;

	if (p->PlayingMode == 0xFF || ! p->VGMSmplPlayed)
		return;

	RestartPlaying(p);

	return;
}

void SeekVGM(void *_p, bool Relative, INT32 PlayBkSamples)
{
	INT32 Samples;
	UINT32 LoopSmpls;

    VGM_PLAYER* p = (VGM_PLAYER*)_p;

	if (p->PlayingMode == 0xFF || (Relative && ! PlayBkSamples))
		return;

	LoopSmpls = p->VGMCurLoop * SampleVGM2Pbk_I(p, p->VGMHead.lngLoopSamples);
	if (! Relative)
		Samples = PlayBkSamples - (LoopSmpls + p->VGMSmplPlayed);
	else
		Samples = PlayBkSamples;

	if (Samples < 0)
	{
		Samples = LoopSmpls + p->VGMSmplPlayed + Samples;
		if (Samples < 0)
			Samples = 0;
		RestartPlaying(p);
	}

	p->ForceVGMExec = true;
	InterpretFile(p, Samples);
	p->ForceVGMExec = false;

	return;
}

void RefreshMuting(void *_p)
{
    VGM_PLAYER* p = (VGM_PLAYER*)_p;
	Chips_GeneralActions(p, 0x10);	// set muting mask

	return;
}

void RefreshPanning(void *_p)
{
    VGM_PLAYER* p = (VGM_PLAYER*)_p;
	Chips_GeneralActions(p, 0x20);	// set panning

	return;
}

void RefreshPlaybackOptions(void *_p)
{
	INT32 TempVol;
	UINT8 CurChip;
	CHIP_OPTS* TempCOpt1;
	CHIP_OPTS* TempCOpt2;

    VGM_PLAYER* p = (VGM_PLAYER*)_p;

	if (p->VGMHead.bytVolumeModifier <= VOLUME_MODIF_WRAP)
		TempVol = p->VGMHead.bytVolumeModifier;
	else if (p->VGMHead.bytVolumeModifier == (VOLUME_MODIF_WRAP + 0x01))
		TempVol = VOLUME_MODIF_WRAP - 0x100;
	else
		TempVol = p->VGMHead.bytVolumeModifier - 0x100;
	p->VolumeLevelM = (float)(p->VolumeLevel * pow(2.0, TempVol / (double)0x20));
	p->FinalVol = p->VolumeLevelM * p->MasterVol * p->MasterVol;

	if (p->PlayingMode == 0xFF)
	{
		TempCOpt1 = (CHIP_OPTS*)&p->ChipOpts[0x00];
		TempCOpt2 = (CHIP_OPTS*)&p->ChipOpts[0x01];
		for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, TempCOpt1 ++, TempCOpt2 ++)
		{
			TempCOpt2->EmuCore = TempCOpt1->EmuCore;
			TempCOpt2->SpecialFlags = TempCOpt1->SpecialFlags;
		}
	}

	return;
}


UINT32 GetGZFileLength(const char* FileName)
{
	FILE* hFile;
	UINT32 FileSize;

	hFile = fopen(FileName, "rb");
	if (hFile == NULL)
		return 0xFFFFFFFF;

	FileSize = GetGZFileLength_Internal(hFile);

	fclose(hFile);
	return FileSize;
}

#ifndef NO_WCHAR_FILENAMES
UINT32 GetGZFileLengthW(const wchar_t* FileName)
{
	FILE* hFile;
	UINT32 FileSize;

	hFile = _wfopen(FileName, L"rb");
	if (hFile == NULL)
		return 0xFFFFFFFF;

	FileSize = GetGZFileLength_Internal(hFile);

	fclose(hFile);
	return FileSize;
}
#endif

static UINT32 GetGZFileLength_Internal(FILE* hFile)
{
	UINT32 FileSize;
	UINT16 gzHead;
	size_t RetVal;

	RetVal = fread(&gzHead, 0x02, 0x01, hFile);
	if (RetVal >= 1)
	{
		gzHead = ReadBE16((UINT8*)&gzHead);
		if (gzHead != 0x1F8B)
		{
			RetVal = 0;	// no .gz signature - treat as normal file
		}
		else
		{
			// .gz File
			fseek(hFile, -4, SEEK_END);
			// Note: In the error case it falls back to fseek/ftell.
			RetVal = fread(&FileSize, 0x04, 0x01, hFile);
#ifdef VGM_BIG_ENDIAN
			FileSize = ReadLE32((UINT8*)&FileSize);
#endif
		}
	}
	if (RetVal <= 0)
	{
		// normal file
		fseek(hFile, 0x00, SEEK_END);
		FileSize = ftell(hFile);
	}

	return FileSize;
}

#ifndef NO_ZLIB
typedef struct vgm_file_gz
{
	VGM_FILE vf;
	gzFile hFile;
	UINT32 Size;
} VGM_FILE_gz;

static int VGMF_gzread(VGM_FILE* hFile, void* ptr, UINT32 count)
{
	VGM_FILE_gz* File = (VGM_FILE_gz *)hFile;
	return gzread(File->hFile, ptr, count);
}

static int VGMF_gzseek(VGM_FILE* hFile, UINT32 offset)
{
	VGM_FILE_gz* File = (VGM_FILE_gz *)hFile;
	return gzseek(File->hFile, offset, SEEK_SET);
}

static UINT32 VGMF_gzgetsize(VGM_FILE* hFile)
{
	VGM_FILE_gz* File = (VGM_FILE_gz *)hFile;
	return File->Size;
}

static UINT32 VGMF_gztell(VGM_FILE* hFile)
{
	VGM_FILE_gz* File = (VGM_FILE_gz *)hFile;
	return gztell(File->hFile);
}
#endif

bool OpenVGMFile(void *_p, const char* FileName)
{
#ifdef NO_ZLIB
	return false;
#else
	gzFile hFile;
	UINT32 FileSize;
	bool RetVal;

  VGM_PLAYER* p = (VGM_PLAYER*)_p;

	FileSize = GetGZFileLength(FileName);

	hFile = gzopen(FileName, "rb");
	if (hFile == NULL)
		return false;

	VGM_FILE_gz vgmFile;

	vgmFile.vf.Read = VGMF_gzread;
	vgmFile.vf.Seek = VGMF_gzseek;
	vgmFile.vf.GetSize = VGMF_gzgetsize;
	vgmFile.vf.Tell = VGMF_gztell;
	vgmFile.hFile = hFile;
	vgmFile.Size = FileSize;

	RetVal = OpenVGMFile_Internal(p, (VGM_FILE *)&vgmFile, FileSize);

	gzclose(hFile);
	return RetVal;
#endif
}

#ifndef NO_WCHAR_FILENAMES
bool OpenVGMFileW(void *_p, const wchar_t* FileName)
{
#ifdef NO_ZLIB
	return false;
#else
	gzFile hFile;
	UINT32 FileSize;
	bool RetVal;

    VGM_PLAYER* p = (VGM_PLAYER*)_p;

#if ZLIB_VERNUM < 0x1270
	int fDesc;

	FileSize = GetGZFileLengthW(FileName);

	fDesc = _wopen(FileName, _O_RDONLY | _O_BINARY);
	hFile = gzdopen(fDesc, "rb");
	if (hFile == NULL)
	{
		_close(fDesc);
		return false;
	}
#else
	FileSize = GetGZFileLengthW(FileName);

	hFile = gzopen_w(FileName, "rb");
	if (hFile == NULL)
		return false;
#endif
	VGM_FILE_gz vgmFile;

	vgmFile.vf.Read = VGMF_gzread;
	vgmFile.vf.Seek = VGMF_gzseek;
	vgmFile.vf.GetSize = VGMF_gzgetsize;
	vgmFile.vf.Tell = VGMF_gztell;
	vgmFile.hFile = hFile;
	vgmFile.Size = FileSize;

	RetVal = OpenVGMFile_Internal(p, (VGM_FILE *)&vgmFile, FileSize);

	gzclose(hFile);
	return RetVal;
#endif
}
#endif

bool OpenVGMFile_Handle(void* _p, VGM_FILE* hFile)
{
	UINT32 FileSize = hFile->GetSize(hFile);
	return OpenVGMFile_Internal((VGM_PLAYER*)_p, hFile, FileSize);
}

static bool OpenVGMFile_Internal(VGM_PLAYER* p, VGM_FILE* hFile, UINT32 FileSize)
{
	UINT32 fccHeader;
	UINT32 CurPos;
	UINT32 HdrLimit;

	hFile->Seek(hFile, 0x00);
	FILE_getLE32(hFile, &fccHeader);
	if (fccHeader != FCC_VGM)
		return false;

	if (p->FileMode != 0xFF)
		CloseVGMFile(p);

	p->FileMode = 0x00;
	p->VGMDataLen = FileSize;

	hFile->Seek(hFile, 0x00);
	ReadVGMHeader(hFile, &p->VGMHead);
	if (p->VGMHead.fccVGM != FCC_VGM)
	{
		printf("VGM signature matched on the first read, but not on the second one!\n");
		printf("This is a known zlib bug where gzseek fails. Please install a fixed zlib.\n");
		return false;
	}

	p->VGMSampleRate = 44100;
	if (! p->VGMDataLen)
		p->VGMDataLen = p->VGMHead.lngEOFOffset;
	if (! p->VGMHead.lngEOFOffset || p->VGMHead.lngEOFOffset > p->VGMDataLen)
	{
		p->VGMHead.lngEOFOffset = p->VGMDataLen;
	}
	if (p->VGMHead.lngLoopOffset && ! p->VGMHead.lngLoopSamples)
	{
		// 0-Sample-Loops causes the program to hangs in the playback routine
		p->VGMHead.lngLoopOffset = 0x00000000;
	}
	if (p->VGMHead.lngDataOffset < 0x00000040)
	{
		p->VGMHead.lngDataOffset = 0x00000040;
	}

	memset(&p->VGMHeadX, 0x00, sizeof(VGM_HDR_EXTRA));
	memset(&p->VGMH_Extra, 0x00, sizeof(VGM_EXTRA));

	// Read Data
	p->VGMDataLen = p->VGMHead.lngEOFOffset;
	p->VGMData = (UINT8*)malloc(p->VGMDataLen);
	if (p->VGMData == NULL)
		return false;
	hFile->Seek(hFile, 0x00);
	hFile->Read(hFile, p->VGMData, p->VGMDataLen);

	// Read Extra Header Data
	if (p->VGMHead.lngExtraOffset)
	{
		UINT32* TempPtr;

		CurPos = p->VGMHead.lngExtraOffset;
		TempPtr = (UINT32*)&p->VGMHeadX;
		// Read Header Size
		p->VGMHeadX.DataSize = ReadLE32(&p->VGMData[CurPos]);
		if (p->VGMHeadX.DataSize > sizeof(VGM_HDR_EXTRA))
			p->VGMHeadX.DataSize = sizeof(VGM_HDR_EXTRA);
		HdrLimit = CurPos + p->VGMHeadX.DataSize;
		CurPos += 0x04;
		TempPtr ++;

		// Read all relative offsets of this header and make them absolute.
		for (; CurPos < HdrLimit; CurPos += 0x04, TempPtr ++)
		{
			*TempPtr = ReadLE32(&p->VGMData[CurPos]);
			if (*TempPtr)
				*TempPtr += CurPos;
		}

		ReadChipExtraData32(p, p->VGMHeadX.Chp2ClkOffset, &p->VGMH_Extra.Clocks);
		ReadChipExtraData16(p, p->VGMHeadX.ChpVolOffset, &p->VGMH_Extra.Volumes);
	}

	// Read GD3 Tag
	HdrLimit = ReadGD3Tag(hFile, p->VGMHead.lngGD3Offset, &p->VGMTag);
	if (HdrLimit == 0x10)
	{
		p->VGMHead.lngGD3Offset = 0x00000000;
		//return false;
	}
	if (! p->VGMHead.lngGD3Offset)
	{
		// replace all NULL pointers with empty strings
		p->VGMTag.strTrackNameE = MakeEmptyWStr();
		p->VGMTag.strTrackNameJ = MakeEmptyWStr();
		p->VGMTag.strGameNameE = MakeEmptyWStr();
		p->VGMTag.strGameNameJ = MakeEmptyWStr();
		p->VGMTag.strSystemNameE = MakeEmptyWStr();
		p->VGMTag.strSystemNameJ = MakeEmptyWStr();
		p->VGMTag.strAuthorNameE = MakeEmptyWStr();
		p->VGMTag.strAuthorNameJ = MakeEmptyWStr();
		p->VGMTag.strReleaseDate = MakeEmptyWStr();
		p->VGMTag.strCreator = MakeEmptyWStr();
		p->VGMTag.strNotes = MakeEmptyWStr();
	}

	return true;
}

static void ReadVGMHeader(VGM_FILE* hFile, VGM_HEADER* RetVGMHead)
{
	VGM_HEADER CurHead;
	UINT32 CurPos;
	UINT32 HdrLimit;

	hFile->Read(hFile, &CurHead, sizeof(VGM_HEADER));
#ifdef VGM_BIG_ENDIAN
	{
		UINT8* TempPtr;

		// Warning: Lots of pointer casting ahead!
		for (CurPos = 0x00; CurPos < sizeof(VGM_HEADER); CurPos += 0x04)
		{
			TempPtr = (UINT8*)&CurHead + CurPos;
			switch(CurPos)
			{
			case 0x28:
				// 0x28	[16-bit] SN76496 Feedback Mask
				// 0x2A	[ 8-bit] SN76496 Shift Register Width
				// 0x2B	[ 8-bit] SN76496 Flags
				*(UINT16*)TempPtr = ReadLE16(TempPtr);
				break;
			case 0x78:	// 78-7B [8-bit] AY8910 Type/Flags
			case 0x7C:	// 7C-7F [8-bit] Volume/Loop Modifiers
			case 0x94:	// 94-97 [8-bit] various flags
				break;
			default:
				// everything else is 32-bit
				*(UINT32*)TempPtr = ReadLE32(TempPtr);
				break;
			}
		}
	}
#endif

	// Header preperations
	if (CurHead.lngVersion < 0x00000101)
	{
		CurHead.lngRate = 0;
	}
	if (CurHead.lngVersion < 0x00000110)
	{
		CurHead.shtPSG_Feedback = 0x0000;
		CurHead.bytPSG_SRWidth = 0x00;
		CurHead.lngHzYM2612 = CurHead.lngHzYM2413;
		CurHead.lngHzYM2151 = CurHead.lngHzYM2413;
	}
	if (CurHead.lngVersion < 0x00000150)
	{
		CurHead.lngDataOffset = 0x00000000;
	// If I would aim to be very strict, I would uncomment these few lines,
	// but I sometimes use v1.51 Flags with v1.50 for better compatibility.
	// (Some hyper-strict players refuse to play v1.51 files, even if there's
	//  no new chip used.)
	//}
	//if (CurHead.lngVersion < 0x00000151)
	//{
		CurHead.bytPSG_Flags = 0x00;
		CurHead.lngHzSPCM = 0x0000;
		CurHead.lngSPCMIntf = 0x00000000;
		// all others are zeroed by memset
	}

	if (CurHead.lngHzPSG)
	{
		if (! CurHead.shtPSG_Feedback)
			CurHead.shtPSG_Feedback = 0x0009;
		if (! CurHead.bytPSG_SRWidth)
			CurHead.bytPSG_SRWidth = 0x10;
	}

	// relative -> absolute addresses
	if (CurHead.lngEOFOffset)
		CurHead.lngEOFOffset += 0x00000004;
	if (CurHead.lngGD3Offset)
		CurHead.lngGD3Offset += 0x00000014;
	if (CurHead.lngLoopOffset)
		CurHead.lngLoopOffset += 0x0000001C;

	if (CurHead.lngVersion < 0x00000150)
		CurHead.lngDataOffset = 0x0000000C;
	//if (CurHead.lngDataOffset < 0x0000000C)
	//	CurHead.lngDataOffset = 0x0000000C;
	if (CurHead.lngDataOffset)
		CurHead.lngDataOffset += 0x00000034;

	CurPos = CurHead.lngDataOffset;
	// should actually check v1.51 (first real usage of DataOffset)
	// v1.50 is checked to support things like the Volume Modifiers in v1.50 files
	if (CurHead.lngVersion < 0x00000150 /*0x00000151*/)
		CurPos = 0x40;
	if (! CurPos)
		CurPos = 0x40;
	HdrLimit = sizeof(VGM_HEADER);
	if (HdrLimit > CurPos)
		memset((UINT8*)&CurHead + CurPos, 0x00, HdrLimit - CurPos);

	if (! CurHead.bytLoopModifier)
		CurHead.bytLoopModifier = 0x10;

	if (CurHead.lngExtraOffset)
	{
		CurHead.lngExtraOffset += 0xBC;

		CurPos = CurHead.lngExtraOffset;
		if (CurPos < HdrLimit)
			memset((UINT8*)&CurHead + CurPos, 0x00, HdrLimit - CurPos);
	}

	if (CurHead.lngGD3Offset >= CurHead.lngEOFOffset)
		CurHead.lngGD3Offset = 0x00;
	if (CurHead.lngLoopOffset >= CurHead.lngEOFOffset)
		CurHead.lngLoopOffset = 0x00;
	if (CurHead.lngDataOffset >= CurHead.lngEOFOffset)
		CurHead.lngDataOffset = 0x40;
	if (CurHead.lngExtraOffset >= CurHead.lngEOFOffset)
		CurHead.lngExtraOffset = 0x00;

	*RetVGMHead = CurHead;
	return;
}

static UINT8 ReadGD3Tag(VGM_FILE* hFile, UINT32 GD3Offset, GD3_TAG* RetGD3Tag)
{
	UINT32 CurPos;
	UINT32 TempLng;
	UINT8 ResVal;

	ResVal = 0x00;

	// Read GD3 Tag
	if (GD3Offset)
	{
		hFile->Seek(hFile, GD3Offset);
		FILE_getLE32(hFile, &TempLng);
		if (TempLng != FCC_GD3)
		{
			GD3Offset = 0x00000000;
			ResVal = 0x10;	// invalid GD3 offset
		}
	}

	if (RetGD3Tag == NULL)
		return ResVal;

	if (! GD3Offset)
	{
		RetGD3Tag->fccGD3 = 0x00000000;
		RetGD3Tag->lngVersion = 0x00000000;
		RetGD3Tag->lngTagLength = 0x00000000;
		RetGD3Tag->strTrackNameE = NULL;
		RetGD3Tag->strTrackNameJ = NULL;
		RetGD3Tag->strGameNameE = NULL;
		RetGD3Tag->strGameNameJ = NULL;
		RetGD3Tag->strSystemNameE = NULL;
		RetGD3Tag->strSystemNameJ = NULL;
		RetGD3Tag->strAuthorNameE = NULL;
		RetGD3Tag->strAuthorNameJ = NULL;
		RetGD3Tag->strReleaseDate = NULL;
		RetGD3Tag->strCreator = NULL;
		RetGD3Tag->strNotes = NULL;
	}
	else
	{
		//CurPos = GD3Offset;
		//hFile->Seek(hFile, CurPos, SEEK_SET);
		//CurPos += FILE_getLE32(hFile, &RetGD3Tag->fccGD3);

		CurPos = GD3Offset + 0x04;		// Save some back seeking, yay!
		RetGD3Tag->fccGD3 = TempLng;	// (That costs lots of CPU in .gz files.)
		CurPos += FILE_getLE32(hFile, &RetGD3Tag->lngVersion);
		CurPos += FILE_getLE32(hFile, &RetGD3Tag->lngTagLength);

		TempLng = CurPos + RetGD3Tag->lngTagLength;
		RetGD3Tag->strTrackNameE =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strTrackNameJ =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strGameNameE =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strGameNameJ =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strSystemNameE =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strSystemNameJ =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strAuthorNameE =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strAuthorNameJ =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strReleaseDate =	ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strCreator =		ReadWStrFromFile(hFile, &CurPos, TempLng);
		RetGD3Tag->strNotes =		ReadWStrFromFile(hFile, &CurPos, TempLng);
	}

	return ResVal;
}

static void ReadChipExtraData32(VGM_PLAYER* p, UINT32 StartOffset, VGMX_CHP_EXTRA32* ChpExtra)
{
	UINT32 CurPos;
	UINT8 CurChp;
	VGMX_CHIP_DATA32* TempCD;

	if (! StartOffset || StartOffset >= p->VGMDataLen)
	{
		ChpExtra->ChipCnt = 0x00;
		ChpExtra->CCData = NULL;
		return;
	}

	CurPos = StartOffset;
	ChpExtra->ChipCnt = p->VGMData[CurPos];
	if (ChpExtra->ChipCnt)
		ChpExtra->CCData = (VGMX_CHIP_DATA32*)malloc(sizeof(VGMX_CHIP_DATA32) *
													ChpExtra->ChipCnt);
	else
		ChpExtra->CCData = NULL;
	CurPos ++;

	for (CurChp = 0x00; CurChp < ChpExtra->ChipCnt; CurChp ++)
	{
		TempCD = &ChpExtra->CCData[CurChp];
		TempCD->Type = p->VGMData[CurPos + 0x00];
		TempCD->Data = ReadLE32(&p->VGMData[CurPos + 0x01]);
		CurPos += 0x05;
	}

	return;
}

static void ReadChipExtraData16(VGM_PLAYER* p, UINT32 StartOffset, VGMX_CHP_EXTRA16* ChpExtra)
{
	UINT32 CurPos;
	UINT8 CurChp;
	VGMX_CHIP_DATA16* TempCD;

	if (! StartOffset || StartOffset >= p->VGMDataLen)
	{
		ChpExtra->ChipCnt = 0x00;
		ChpExtra->CCData = NULL;
		return;
	}

	CurPos = StartOffset;
	ChpExtra->ChipCnt = p->VGMData[CurPos];
	if (ChpExtra->ChipCnt)
		ChpExtra->CCData = (VGMX_CHIP_DATA16*)malloc(sizeof(VGMX_CHIP_DATA16) *
													ChpExtra->ChipCnt);
	else
		ChpExtra->CCData = NULL;
	CurPos ++;

	for (CurChp = 0x00; CurChp < ChpExtra->ChipCnt; CurChp ++)
	{
		TempCD = &ChpExtra->CCData[CurChp];
		TempCD->Type = p->VGMData[CurPos + 0x00];
		TempCD->Flags = p->VGMData[CurPos + 0x01];
		TempCD->Data = ReadLE16(&p->VGMData[CurPos + 0x02]);
		CurPos += 0x04;
	}

	return;
}

void CloseVGMFile(void *_p)
{
    VGM_PLAYER* p = (VGM_PLAYER*)_p;

	if (p->FileMode == 0xFF)
		return;

	p->VGMHead.fccVGM = 0x00;
	free(p->VGMH_Extra.Clocks.CCData);		p->VGMH_Extra.Clocks.CCData = NULL;
	free(p->VGMH_Extra.Volumes.CCData);	p->VGMH_Extra.Volumes.CCData = NULL;
	free(p->VGMData);	p->VGMData = NULL;

	if (p->FileMode == 0x00)
	FreeGD3Tag(&p->VGMTag);

	p->FileMode = 0xFF;

	return;
}

void FreeGD3Tag(GD3_TAG* TagData)
{
	if (TagData == NULL)
		return;

	TagData->fccGD3 = 0x00;
	free(TagData->strTrackNameE);	TagData->strTrackNameE = NULL;
	free(TagData->strTrackNameJ);	TagData->strTrackNameJ = NULL;
	free(TagData->strGameNameE);	TagData->strGameNameE = NULL;
	free(TagData->strGameNameJ);	TagData->strGameNameJ = NULL;
	free(TagData->strSystemNameE);	TagData->strSystemNameE = NULL;
	free(TagData->strSystemNameJ);	TagData->strSystemNameJ = NULL;
	free(TagData->strAuthorNameE);	TagData->strAuthorNameE = NULL;
	free(TagData->strAuthorNameJ);	TagData->strAuthorNameJ = NULL;
	free(TagData->strReleaseDate);	TagData->strReleaseDate = NULL;
	free(TagData->strCreator);		TagData->strCreator = NULL;
	free(TagData->strNotes);		TagData->strNotes = NULL;

	return;
}

static wchar_t* MakeEmptyWStr(void)
{
	wchar_t* Str;

	Str = (wchar_t*)malloc(0x01 * sizeof(wchar_t));
	Str[0x00] = L'\0';

	return Str;
}

static wchar_t* ReadWStrFromFile(VGM_FILE* hFile, UINT32* FilePos, UINT32 EOFPos)
{
	// Note: Works with Windows (16-bit wchar_t) as well as Linux (32-bit wchar_t)
	UINT32 CurPos;
	wchar_t* TextStr;
	wchar_t* TempStr;
	UINT32 StrLen;
	UINT16 UnicodeChr;

	CurPos = *FilePos;
	if (CurPos >= EOFPos)
		return NULL;
	TextStr = (wchar_t*)malloc((EOFPos - CurPos) / 0x02 * sizeof(wchar_t));
	if (TextStr == NULL)
		return NULL;

	if (hFile->Tell(hFile) != CurPos)
		hFile->Seek(hFile, CurPos);
	TempStr = TextStr - 1;
	StrLen = 0x00;
	do
	{
		TempStr ++;
		FILE_getLE16(hFile, &UnicodeChr);
		*TempStr = (wchar_t)UnicodeChr;
		CurPos += 0x02;
		StrLen ++;
		if (CurPos >= EOFPos)
		{
			*TempStr = L'\0';
			break;
		}
	} while(*TempStr != L'\0');

	TextStr = (wchar_t*)realloc(TextStr, StrLen * sizeof(wchar_t));
	*FilePos = CurPos;

	return TextStr;
}

UINT32 GetVGMFileInfo(const char* FileName, VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag)
{
#ifdef NO_ZLIB
	return 0;
#else
	gzFile hFile;
	UINT32 FileSize;
	UINT32 RetVal;

	FileSize = GetGZFileLength(FileName);

	hFile = gzopen(FileName, "rb");
	if (hFile == NULL)
		return 0x00;

	VGM_FILE_gz vgmFile;

	vgmFile.vf.Read = VGMF_gzread;
	vgmFile.vf.Seek = VGMF_gzseek;
	vgmFile.vf.GetSize = VGMF_gzgetsize;
	vgmFile.hFile = hFile;
	vgmFile.Size = FileSize;

	RetVal = GetVGMFileInfo_Internal((VGM_FILE *)&vgmFile, FileSize, RetVGMHead, RetGD3Tag);

	gzclose(hFile);
	return RetVal;
#endif
}

#ifndef NO_WCHAR_FILENAMES
UINT32 GetVGMFileInfoW(const wchar_t* FileName, VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag)
{
#ifdef NO_ZLIB
	return 0;
#else
	gzFile hFile;
	UINT32 FileSize;
	UINT32 RetVal;
#if ZLIB_VERNUM < 0x1270
	int fDesc;

	FileSize = GetGZFileLengthW(FileName);

	fDesc = _wopen(FileName, _O_RDONLY | _O_BINARY);
	hFile = gzdopen(fDesc, "rb");
	if (hFile == NULL)
	{
		_close(fDesc);
		return 0x00;
	}
#else
	FileSize = GetGZFileLengthW(FileName);

	hFile = gzopen_w(FileName, "rb");
	if (hFile == NULL)
		return 0x00;
#endif

	VGM_FILE_gz vgmFile;

	vgmFile.vf.Read = VGMF_gzread;
	vgmFile.vf.Seek = VGMF_gzseek;
	vgmFile.vf.GetSize = VGMF_gzgetsize;
	vgmFile.hFile = hFile;
	vgmFile.Size = FileSize;

	RetVal = GetVGMFileInfo_Internal((VGM_FILE *)&vgmFile, FileSize, RetVGMHead, RetGD3Tag);

	gzclose(hFile);
	return RetVal;
#endif
}
#endif

UINT32 GetVGMFileInfo_Handle(VGM_FILE* hFile, VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag)
{
	UINT32 FileSize = hFile->GetSize(hFile);
	return GetVGMFileInfo_Internal(hFile, FileSize, RetVGMHead, RetGD3Tag);
}

static UINT32 GetVGMFileInfo_Internal(VGM_FILE* hFile, UINT32 FileSize,
									  VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag)
{
	// this is a copy-and-paste from OpenVGM, just a little stripped
	UINT32 fccHeader;
	UINT32 TempLng;
	VGM_HEADER TempHead;

	hFile->Seek(hFile, 0x00);
	FILE_getLE32(hFile, &fccHeader);
	if (fccHeader != FCC_VGM)
		return 0x00;

	if (RetVGMHead == NULL && RetGD3Tag == NULL)
		return FileSize;

	hFile->Seek(hFile, 0x00);
	ReadVGMHeader(hFile, &TempHead);
	if (TempHead.fccVGM != FCC_VGM)
	{
		printf("VGM signature matched on the first read, but not on the second one!\n");
		printf("This is a known zlib bug where gzseek fails. Please install a fixed zlib.\n");
		return 0x00;
	}

	if (! TempHead.lngEOFOffset || TempHead.lngEOFOffset > FileSize)
		TempHead.lngEOFOffset = FileSize;
	if (TempHead.lngDataOffset < 0x00000040)
		TempHead.lngDataOffset = 0x00000040;

	/*if (TempHead.lngGD3Offset)
	{
		gzseek(hFile, TempHead.lngGD3Offset, SEEK_SET);
		gzgetLE32(hFile, &fccHeader);
		if (fccHeader != FCC_GD3)
			TempHead.lngGD3Offset = 0x00000000;
			//return 0x00;
	}*/

	if (RetVGMHead != NULL)
		*RetVGMHead = TempHead;

	// Read GD3 Tag
	if (RetGD3Tag != NULL)
		TempLng = ReadGD3Tag(hFile, TempHead.lngGD3Offset, RetGD3Tag);

	return FileSize;
}

INLINE UINT32 MulDivRound(UINT64 Number, UINT64 Numerator, UINT64 Denominator)
{
	return (UINT32)((Number * Numerator + Denominator / 2) / Denominator);
}

UINT32 CalcSampleMSec(void* _p, UINT64 Value, UINT8 Mode)
{
	// Mode:
	//	Bit 0 (01):	Calculation Mode
	//				0 - Sample2MSec
	//				1 - MSec2Sample
	//	Bit 1 (02):	Calculation Samlpe Rate
	//				0 - current playback rate
	//				1 - 44.1 KHz (VGM native)
	UINT32 SmplRate;
	UINT32 PbMul;
	UINT32 PbDiv;
	UINT32 RetVal;

    VGM_PLAYER* p = (VGM_PLAYER *)_p;

	if (! (Mode & 0x02))
	{
		SmplRate = p->SampleRate;
		PbMul = 1;
		PbDiv = 1;
	}
	else
	{
		SmplRate = p->VGMSampleRate;
		PbMul = p->VGMPbRateMul;
		PbDiv = p->VGMPbRateDiv;
	}

	switch(Mode & 0x01)
	{
	case 0x00:
		RetVal = MulDivRound(Value, (UINT64)1000 * PbMul, (UINT64)SmplRate * PbDiv);
		break;
	case 0x01:
		RetVal = MulDivRound(Value, (UINT64)SmplRate * PbDiv, (UINT64)1000 * PbMul);
		break;
	}

	return RetVal;
}

UINT32 CalcSampleMSecExt(void *_p, UINT64 Value, UINT8 Mode, VGM_HEADER* FileHead)
{
	// Note: This function was NOT tested with non-VGM formats!

	// Mode: see function above
	UINT32 SmplRate;
	UINT32 PbMul;
	UINT32 PbDiv;
	UINT32 RetVal;

    VGM_PLAYER* p = (VGM_PLAYER *)_p;

	if (! (Mode & 0x02))
	{
		SmplRate = p->SampleRate;
		PbMul = 1;
		PbDiv = 1;
	}
	else
	{
		// TODO: make it work for non-VGM formats
		// (i.e. get VGMSampleRate information from FileHead)
		//
		// But currently GetVGMFileInfo doesn't support them, it doesn't matter either way
		SmplRate = 44100;
		if (! p->VGMPbRate || ! FileHead->lngRate)
		{
			PbMul = 1;
			PbDiv = 1;
		}
		else
		{
			PbMul = FileHead->lngRate;
			PbDiv = p->VGMPbRate;
		}
	}

	switch(Mode & 0x01)
	{
	case 0x00:
		RetVal = MulDivRound(Value, 1000 * PbMul, SmplRate * PbDiv);
		break;
	case 0x01:
		RetVal = MulDivRound(Value, SmplRate * PbDiv, 1000 * PbMul);
		break;
	}

	return RetVal;
}

/*static UINT32 EncryptChipName(void* DstBuf, const void* SrcBuf, UINT32 Length)
{
	// using nineko's awesome encryption algorithm
	// http://forums.sonicretro.org/index.php?showtopic=25300
	// based on C code by sasuke
	const UINT8* SrcPos;
	UINT8* DstPos;
	UINT32 CurPos;
	UINT8 CryptShift;	// Src Bit/Dst Byte
	UINT8 PlainShift;	// Src Byte/Dst Bit

	if (Length & 0x07)
		return 0x00;	// Length MUST be a multiple of 8

	SrcPos = (const UINT8*)SrcBuf;
	DstPos = (UINT8*)DstBuf;
	for (CurPos = 0; CurPos < Length; CurPos += 8, SrcPos += 8, DstPos += 8)
	{
		for (CryptShift = 0; CryptShift < 8; CryptShift ++)
		{
			DstPos[CryptShift] = 0x00;
			for (PlainShift = 0; PlainShift < 8; PlainShift ++)
			{
				if (SrcPos[PlainShift] & (1 << CryptShift))
					DstPos[CryptShift] |= (1 << PlainShift);
			}
		}
	}

	return Length;
}*/

const char* GetChipName(UINT8 ChipID)
{
	const char* CHIP_STRS[CHIP_COUNT] =
	{	"SN76496", "YM2413", "YM2612", "YM2151", "SegaPCM", "RF5C68", "YM2203", "YM2608",
		"YM2610", "YM3812", "YM3526", "Y8950", "YMF262", "YMF278B", "YMF271", "YMZ280B",
		"RF5C164", "PWM", "AY8910", "GameBoy", "NES APU", "MultiPCM", "uPD7759", "OKIM6258",
		"OKIM6295", "K051649", "K054539", "HuC6280", "C140", "K053260", "Pokey", "QSound",
		"SCSP", "WSwan", "VSU", "SAA1099", "ES5503", "ES5506", "X1-010", "C352",
		"GA20"};

	/*if (ChipID == 0x21)
	{
		static char TempStr[0x08];
		UINT32 TempData[2];

		//EncryptChipName(TempData, "WSwan", 0x08);
		TempData[0] = 0x1015170F;
		TempData[1] = 0x001F1C07;
		EncryptChipName(TempStr, TempData, 0x08);
		return TempStr;	// "WSwan"
	}*/

	if (ChipID < CHIP_COUNT)
		return CHIP_STRS[ChipID];
	else
		return NULL;
}

const char* GetAccurateChipName(UINT8 ChipID, UINT8 SubType)
{
	const char* RetStr;

	if ((ChipID & 0x7F) >= CHIP_COUNT)
		return NULL;

	RetStr = NULL;
	switch(ChipID & 0x7F)
	{
	case 0x00:
		if (! (ChipID & 0x80))
		{
			switch(SubType)
			{
			case 0x01:
				RetStr = "SN76489";
				break;
			case 0x02:
				RetStr = "SN76489A";
				break;
			case 0x03:
				RetStr = "SN76494";
				break;
			case 0x04:
				RetStr = "SN76496";
				break;
			case 0x05:
				RetStr = "SN94624";
				break;
			case 0x06:
				RetStr = "NCR7496";
				break;
			case 0x07:
				RetStr = "SEGA PSG";
				break;
			default:
				RetStr = "SN76496";
				break;
			}
		}
		else
		{
			RetStr = "T6W28";
		}
		break;
	case 0x01:
		if (ChipID & 0x80)
			RetStr = "VRC7";
		break;
	case 0x04:
		RetStr = "Sega PCM";
		break;
	case 0x08:
		if (! (ChipID & 0x80))
			RetStr = "YM2610";
		else
			RetStr = "YM2610B";
		break;
	case 0x12:	// AY8910
		switch(SubType)
		{
		case 0x00:
			RetStr = "AY-3-8910";
			break;
		case 0x01:
			RetStr = "AY-3-8912";
			break;
		case 0x02:
			RetStr = "AY-3-8913";
			break;
		case 0x03:
			RetStr = "AY8930";
			break;
		case 0x04:
			RetStr = "AY-3-8914";
			break;
		case 0x10:
			RetStr = "YM2149";
			break;
		case 0x11:
			RetStr = "YM3439";
			break;
		case 0x12:
			RetStr = "YMZ284";
			break;
		case 0x13:
			RetStr = "YMZ294";
			break;
		}
		break;
	case 0x13:
		RetStr = "GB DMG";
		break;
	case 0x14:
		if (! (ChipID & 0x80))
			RetStr = "NES APU";
		else
			RetStr = "NES APU + FDS";
		break;
	case 0x19:
		if (! (ChipID & 0x80))
			RetStr = "K051649";
		else
			RetStr = "K052539";
		break;
	case 0x1C:
		switch(SubType)
		{
		case 0x00:
		case 0x01:
			RetStr = "C140";
			break;
		case 0x02:
			RetStr = "C140 (219)";
			break;
		}
		break;
	case 0x21:
		RetStr = "WonderSwan";
		break;
	case 0x22:
		RetStr = "VSU-VUE";
		break;
	case 0x25:
		if (! (ChipID & 0x80))
			RetStr = "ES5505";
		else
			RetStr = "ES5506";
		break;
	case 0x28:
		RetStr = "Irem GA20";
		break;
	}
	// catch all default-cases
	if (RetStr == NULL)
		RetStr = GetChipName(ChipID & 0x7F);

	return RetStr;
}

UINT32 GetChipClock(void* _p, UINT8 ChipID, UINT8* RetSubType)
{
	UINT32 Clock;
	UINT8 SubType;
	UINT8 CurChp;
	bool AllowBit31;

    VGM_PLAYER* p = (VGM_PLAYER *)_p;

    VGM_HEADER* FileHead = &p->VGMHead;

	SubType = 0x00;
	AllowBit31 = 0x00;
	switch(ChipID & 0x7F)
	{
	case 0x00:
		Clock = FileHead->lngHzPSG;
		AllowBit31 = 0x01;	// T6W28 Mode
		if (RetSubType != NULL && ! (Clock & 0x80000000))	// The T6W28 is handles differently.
		{
			switch(FileHead->bytPSG_SRWidth)
			{
			case 0x0F:	//  0x4000
				if (FileHead->bytPSG_Flags & 0x08)	// Clock Divider == 1?
					SubType = 0x05;	// SN94624
				else
					SubType = 0x01;	// SN76489
				break;
			case 0x10:	//  0x8000
				if (FileHead->shtPSG_Feedback == 0x0009)
					SubType = 0x07;	// SEGA PSG
				else if (FileHead->shtPSG_Feedback == 0x0022)
					SubType = 0x06;	// NCR7496
				break;
			case 0x11:	// 0x10000
				if (FileHead->bytPSG_Flags & 0x08)	// Clock Divider == 1?
					SubType = 0x03;	// SN76494
				else
					SubType = 0x02;	// SN76489A/SN76496
				break;
			}
			/*
								 FbMask  Noise Taps  Negate Stereo Dv Freq0		Fb	SR	Flags
				01 SN76489		 0x4000, 0x01, 0x02, TRUE,  FALSE, 8, TRUE		03	0F	07 (02|04|00|01) [unverified]
				02 SN76489A		0x10000, 0x04, 0x08, FALSE, FALSE, 8, TRUE		0C	11	05 (00|04|00|01)
				03 SN76494		0x10000, 0x04, 0x08, FALSE, FALSE, 1, TRUE		0C	11	0D (00|04|08|01)
				04 SN76496		0x10000, 0x04, 0x08, FALSE, FALSE, 8, TRUE		0C	11	05 (00|04|00|01) [same as SN76489A]
				05 SN94624		 0x4000, 0x01, 0x02, TRUE,  FALSE, 1, TRUE		03	0F	0F (02|04|08|01) [unverified, SN76489A without /8]
				06 NCR7496		 0x8000, 0x02, 0x20, FALSE, FALSE, 8, TRUE		22	10	05 (00|04|00|01) [unverified]
				07 GameGear PSG	 0x8000, 0x01, 0x08, TRUE,  TRUE,  8, FALSE		09	10	02 (02|00|00|00)
				07 SEGA VDP PSG	 0x8000, 0x01, 0x08, TRUE,  FALSE, 8, FALSE		09	10	06 (02|04|00|00)
				01 U8106		 0x4000, 0x01, 0x02, TRUE,  FALSE, 8, TRUE		03	0F	07 (02|04|00|01) [unverified, same as SN76489]
				02 Y2404		0x10000, 0x04, 0x08, FALSE, FALSE; 8, TRUE		0C	11	05 (00|04|00|01) [unverified, same as SN76489A]
				-- T6W28		 0x4000, 0x01, 0x04, ????,  FALSE, 8, ????		05	0F	?? (??|??|00|01) [It IS stereo, but not in GameGear way].
			*/
		}
		break;
	case 0x01:
		Clock = FileHead->lngHzYM2413;
		AllowBit31 = 0x01;	// VRC7 Mode
		break;
	case 0x02:
		Clock = FileHead->lngHzYM2612;
		break;
	case 0x03:
		Clock = FileHead->lngHzYM2151;
		break;
	case 0x04:
		Clock = FileHead->lngHzSPCM;
		break;
	case 0x05:
		if (ChipID & 0x80)
			return 0;
		Clock = FileHead->lngHzRF5C68;
		break;
	case 0x06:
		Clock = FileHead->lngHzYM2203;
		break;
	case 0x07:
		Clock = FileHead->lngHzYM2608;
		break;
	case 0x08:
		Clock = FileHead->lngHzYM2610;
		AllowBit31 = 0x01;	// YM2610B Mode
		break;
	case 0x09:
		Clock = FileHead->lngHzYM3812;
		AllowBit31 = 0x01;	// Dual OPL2, panned to the L/R speakers
		break;
	case 0x0A:
		Clock = FileHead->lngHzYM3526;
		break;
	case 0x0B:
		Clock = FileHead->lngHzY8950;
		break;
	case 0x0C:
		Clock = FileHead->lngHzYMF262;
		break;
	case 0x0D:
		Clock = FileHead->lngHzYMF278B;
		break;
	case 0x0E:
		Clock = FileHead->lngHzYMF271;
		break;
	case 0x0F:
		Clock = FileHead->lngHzYMZ280B;
		break;
	case 0x10:
		if (ChipID & 0x80)
			return 0;
		Clock = FileHead->lngHzRF5C164;
		AllowBit31 = 0x01;	// hack for Cosmic Fantasy Stories
		break;
	case 0x11:
		if (ChipID & 0x80)
			return 0;
		Clock = FileHead->lngHzPWM;
		break;
	case 0x12:
		Clock = FileHead->lngHzAY8910;
		SubType = FileHead->bytAYType;
		break;
	case 0x13:
		Clock = FileHead->lngHzGBDMG;
		break;
	case 0x14:
		Clock = FileHead->lngHzNESAPU;
		AllowBit31 = 0x01;	// FDS Enable
		break;
	case 0x15:
		Clock = FileHead->lngHzMultiPCM;
		break;
	case 0x16:
		Clock = FileHead->lngHzUPD7759;
		AllowBit31 = 0x01;	// Master/Slave Bit
		break;
	case 0x17:
		Clock = FileHead->lngHzOKIM6258;
		break;
	case 0x18:
		Clock = FileHead->lngHzOKIM6295;
		AllowBit31 = 0x01;	// Pin 7 State
		break;
	case 0x19:
		Clock = FileHead->lngHzK051649;
		AllowBit31 = 0x01;	// SCC/SCC+ Bit
		break;
	case 0x1A:
		Clock = FileHead->lngHzK054539;
		break;
	case 0x1B:
		Clock = FileHead->lngHzHuC6280;
		break;
	case 0x1C:
		Clock = FileHead->lngHzC140;
		SubType = FileHead->bytC140Type;
		break;
	case 0x1D:
		Clock = FileHead->lngHzK053260;
		break;
	case 0x1E:
		Clock = FileHead->lngHzPokey;
		break;
	case 0x1F:
		if (ChipID & 0x80)
			return 0;
		Clock = FileHead->lngHzQSound;
		break;
	case 0x20:
		Clock = FileHead->lngHzSCSP;
		break;
	case 0x21:
		Clock = FileHead->lngHzWSwan;
		break;
	case 0x22:
		Clock = FileHead->lngHzVSU;
		break;
	case 0x23:
		Clock = FileHead->lngHzSAA1099;
		break;
	case 0x24:
		Clock = FileHead->lngHzES5503;
		break;
	case 0x25:
		Clock = FileHead->lngHzES5506;
		AllowBit31 = 0x01;	// ES5505/5506 switch
		break;
	case 0x26:
		Clock = FileHead->lngHzX1_010;
		break;
	case 0x27:
		Clock = FileHead->lngHzC352;
		AllowBit31 = 0x01;	// disable rear channels
		break;
	case 0x28:
		Clock = FileHead->lngHzGA20;
		break;
	default:
		return 0;
	}
	if (ChipID & 0x80)
	{
		VGMX_CHP_EXTRA32* TempCX;

		if (! (Clock & 0x40000000))
			return 0;

		ChipID &= 0x7F;
		TempCX = &p->VGMH_Extra.Clocks;
		for (CurChp = 0x00; CurChp < TempCX->ChipCnt; CurChp ++)
		{
			if (TempCX->CCData[CurChp].Type == ChipID)
			{
				if (TempCX->CCData[CurChp].Data)
					Clock = TempCX->CCData[CurChp].Data;
				break;
			}
		}
	}

	if (RetSubType != NULL)
		*RetSubType = SubType;
	if (AllowBit31)
		return Clock & 0xBFFFFFFF;
	else
		return Clock & 0x3FFFFFFF;
}

static UINT16 GetChipVolume(VGM_PLAYER* p, UINT8 ChipID, UINT8 ChipNum, UINT8 ChipCnt)
{
	// ChipID: ID of Chip
	//		Bit 7 - Is Paired Chip
	// ChipNum: chip number (0 - first chip, 1 - second chip)
	// ChipCnt: chip volume divider (number of used chips)
	const UINT16 CHIP_VOLS[CHIP_COUNT] =
	{	0x80, 0x200/*0x155*/, 0x100, 0x100, 0x180, 0xB0, 0x100, 0x80,	// 00-07
		0x80, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x98,			// 08-0F
		0x80, 0xE0/*0xCD*/, 0x100, 0xC0, 0x100, 0x40, 0x11E, 0x1C0,		// 10-17
		0x100/*110*/, 0xA0, 0x100, 0x100, 0x100, 0xB3, 0x100, 0x100,	// 18-1F
		0x20, 0x100, 0x100, 0x100, 0x40, 0x20, 0x100, 0x40,			// 20-27
		0x280};
	UINT16 Volume;
	UINT8 CurChp;
	VGMX_CHP_EXTRA16* TempCX;
	VGMX_CHIP_DATA16* TempCD;

    /*VGM_HEADER* FileHead = &p->VGMHead;*/

	Volume = CHIP_VOLS[ChipID & 0x7F];
	switch(ChipID)
	{
	case 0x00:	// SN76496
		// if T6W28, set Volume Divider to 01
		if (GetChipClock(p, (ChipID << 7) | ChipID, NULL) & 0x80000000)
		{
			// The T6W28 consists of 2 "half" chips.
			ChipNum = 0x01;
			ChipCnt = 0x01;
		}
		break;
	case 0x18:	// OKIM6295
		// CP System 1 patch
		if (p->VGMTag.strSystemNameE != NULL && ! wcsncmp(p->VGMTag.strSystemNameE, L"CP", 0x02))
			Volume = 110;
		break;
	case 0x86:	// YM2203's AY
		Volume /= 2;
		break;
	case 0x87:	// YM2608's AY
		// The YM2608 outputs twice as loud as the YM2203 here.
		//Volume *= 1;
		break;
	case 0x88:	// YM2610's AY
		//Volume *= 1;
		break;
	}
	if (ChipCnt > 1)
		Volume /= ChipCnt;

	TempCX = &p->VGMH_Extra.Volumes;
	TempCD = TempCX->CCData;
	for (CurChp = 0x00; CurChp < TempCX->ChipCnt; CurChp ++, TempCD ++)
	{
		if (TempCD->Type == ChipID && (TempCD->Flags & 0x01) == ChipNum)
		{
			// Bit 15 - absolute/relative volume
			//	0 - absolute
			//	1 - relative (0x0100 = 1.0, 0x80 = 0.5, etc.)
			if (TempCD->Data & 0x8000)
				Volume = (Volume * (TempCD->Data & 0x7FFF) + 0x80) >> 8;
			else
			{
				Volume = TempCD->Data;
				if ((ChipID & 0x80) && p->DoubleSSGVol)
					Volume *= 2;
			}
			break;
		}
	}

	return Volume;
}


static void RestartPlaying(VGM_PLAYER* p)
{
	p->VGMPos = p->VGMHead.lngDataOffset;
	p->VGMSmplPos = 0;
	p->VGMSmplPlayed = 0;
	p->VGMEnd = false;
	p->EndPlay = false;
	p->VGMCurLoop = 0x00;

	Chips_GeneralActions(p, 0x01);	// Reset Chips
	// also does Muting Mask (0x10) and Panning (0x20)

	p->Last95Drum = 0xFFFF;
	p->Last95Freq = 0;
	p->ForceVGMExec = true;
	p->IsVGMInit = true;
	InterpretFile(p, 0);
	p->IsVGMInit = false;
	p->ForceVGMExec = false;

	return;
}

static void Chips_GeneralActions(VGM_PLAYER* p, UINT8 Mode)
{
	UINT32 AbsVol;
	//UINT16 ChipVol;
	CAUD_ATTR* CAA;
	CHIP_OPTS* COpt;
	UINT8 ChipCnt;
	UINT8 CurChip;
	UINT8 CurCSet;	// Chip Set
	UINT32 MaskVal;
	UINT32 ChipClk;

	switch(Mode)
	{
	case 0x00:	// Start Chips
		for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
		{
			CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
			for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
			{
				CAA->SmpRate = 0x00;
				CAA->Volume = 0x00;
				CAA->ChipType = 0xFF;
				CAA->ChipID = CurCSet;
				CAA->Resampler = 0x00;
				CAA->StreamUpdate = &null_update;
                CAA->StreamUpdateParam = NULL;
				CAA->Paired = NULL;
			}
			CAA = p->CA_Paired[CurCSet];
			for (CurChip = 0x00; CurChip < 0x03; CurChip ++, CAA ++)
			{
				CAA->SmpRate = 0x00;
				CAA->Volume = 0x00;
				CAA->ChipType = 0xFF;
				CAA->ChipID = CurCSet;
				CAA->Resampler = 0x00;
				CAA->StreamUpdate = &null_update;
                CAA->StreamUpdateParam = NULL;
				CAA->Paired = NULL;
			}
		}

		// Initialize Sound Chips
		AbsVol = 0x00;
		if (p->VGMHead.lngHzPSG)
		{
			//ChipVol = UseFM ? 0x00 : 0x80;
			p->ChipOpts[0x01].SN76496.EmuCore = p->ChipOpts[0x00].SN76496.EmuCore;

			ChipCnt = (p->VGMHead.lngHzPSG & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].SN76496;
				CAA->ChipType = 0x00;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				ChipClk &= ~0x80000000;
				ChipClk |= p->VGMHead.lngHzPSG & ((CurChip & 0x01) << 31);
                CAA->SmpRate = device_start_sn764xx(&p->sn764xx[CurChip],
                                                    p->ChipOpts[CurChip].SN76496.EmuCore, ChipClk, p->SampleRate,
                                                    p->VGMHead.bytPSG_SRWidth,
                                                    p->VGMHead.shtPSG_Feedback,
                                                    (p->VGMHead.bytPSG_Flags & 0x02) >> 1,
                                                    (p->VGMHead.bytPSG_Flags & 0x04) >> 2,
                                                    (p->VGMHead.bytPSG_Flags & 0x08) >> 3,
                                                    (p->VGMHead.bytPSG_Flags & 0x01) >> 0);
                CAA->StreamUpdate = &sn764xx_stream_update;
                CAA->StreamUpdateParam = p->sn764xx[CurChip];

                CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
                if (! CurChip || ! (ChipClk & 0x80000000))
                    AbsVol += CAA->Volume;
			}
			if (p->VGMHead.lngHzPSG & 0x80000000)
				ChipCnt = 0x01;
		}
		if (p->VGMHead.lngHzYM2413)
		{
			//ChipVol = UseFM ? 0x00 : 0x200/*0x155*/;
			p->ChipOpts[0x01].YM2413.EmuCore = p->ChipOpts[0x00].YM2413.EmuCore;

			ChipCnt = (p->VGMHead.lngHzYM2413 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM2413;
				CAA->ChipType = 0x01;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
                CAA->SmpRate = device_start_ym2413(&p->ym2413[CurChip], p->ChipOpts[CurChip].YM2413.EmuCore, ChipClk, p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
                CAA->StreamUpdate = &ym2413_stream_update;
                CAA->StreamUpdateParam = p->ym2413[CurChip];

                CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
                // WHY has this chip such a low volume???
                //AbsVol += (CAA->Volume + 1) * 3 / 4;
                AbsVol += CAA->Volume / 2;
            }
		}
		if (p->VGMHead.lngHzYM2612)
		{
			//ChipVol = 0x100;
			p->ChipOpts[0x01].YM2612.EmuCore = p->ChipOpts[0x00].YM2612.EmuCore;
			p->ChipOpts[0x01].YM2612.SpecialFlags = p->ChipOpts[0x00].YM2612.SpecialFlags;

			ChipCnt = (p->VGMHead.lngHzYM2612 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM2612;
				CAA->ChipType = 0x02;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_ym2612(&p->ym2612[CurChip], p->ChipOpts[CurChip].YM2612.EmuCore, p->ChipOpts[CurChip].YM2612.SpecialFlags, ChipClk, p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE, &p->IsVGMInit);
				CAA->StreamUpdate = &ym2612_stream_update;
                CAA->StreamUpdateParam = p->ym2612[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzYM2151)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzYM2151 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM2151;
				CAA->ChipType = 0x03;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_ym2151(&p->ym2151[CurChip], ChipClk, p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &ym2151_update;
                CAA->StreamUpdateParam = p->ym2151[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzSPCM)
		{
			//ChipVol = 0x180;
			ChipCnt = (p->VGMHead.lngHzSPCM & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].SegaPCM;
				CAA->ChipType = 0x04;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_segapcm(&p->segapcm[CurChip], ChipClk, p->VGMHead.lngSPCMIntf);
				CAA->StreamUpdate = &SEGAPCM_update;
                CAA->StreamUpdateParam = p->segapcm[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzRF5C68)
		{
			//ChipVol = 0xB0;	// that's right according to MAME, but it's almost too loud
			ChipCnt = 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].RF5C68;
				CAA->ChipType = 0x05;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_rf5c68(&p->rf5c68, ChipClk);
				CAA->StreamUpdate = &rf5c68_update;
                CAA->StreamUpdateParam = p->rf5c68;

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzYM2203)
		{
			//ChipVol = 0x100;
			p->ChipOpts[0x01].YM2203.EmuCore = p->ChipOpts[0x00].YM2203.EmuCore;
			p->ChipOpts[0x01].YM2203.SpecialFlags = p->ChipOpts[0x00].YM2203.SpecialFlags;

			ChipCnt = (p->VGMHead.lngHzYM2203 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM2203;
				COpt = &p->ChipOpts[CurChip].YM2203;
				CAA->ChipType = 0x06;
				CAA->Paired = &p->CA_Paired[CurChip][0x00];
				CAA->Paired->ChipType = 0x80 | CAA->ChipType;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
                CAA->SmpRate = device_start_ym2203(&p->ym2203[CurChip], COpt->EmuCore,
                                                    ChipClk, COpt->SpecialFlags & 0x01,
													p->VGMHead.bytAYFlagYM2203,
													(int*) &CAA->Paired->SmpRate,
                                                    p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &ym2203_stream_update;
                CAA->StreamUpdateParam = p->ym2203[CurChip];
				CAA->Paired->StreamUpdate = &ym2203_stream_update_ay;
                CAA->Paired->StreamUpdateParam = p->ym2203[CurChip];
				ym2203_set_srchg_cb(p->ym2203[CurChip], &ChangeChipSampleRate, CAA, CAA->Paired);

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				CAA->Paired->Volume = GetChipVolume(p, CAA->Paired->ChipType,
													CurChip, ChipCnt);
				AbsVol += CAA->Volume + CAA->Paired->Volume;
			}
		}
		if (p->VGMHead.lngHzYM2608)
		{
			//ChipVol = 0x80;
			p->ChipOpts[0x01].YM2608.EmuCore = p->ChipOpts[0x00].YM2608.EmuCore;
			p->ChipOpts[0x01].YM2608.SpecialFlags = p->ChipOpts[0x00].YM2608.SpecialFlags;

			ChipCnt = (p->VGMHead.lngHzYM2608 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM2608;
				COpt = &p->ChipOpts[CurChip].YM2608;
				CAA->ChipType = 0x07;
				CAA->Paired = &p->CA_Paired[CurChip][0x01];
				CAA->Paired->ChipType = 0x80 | CAA->ChipType;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_ym2608(&p->ym2608[CurChip], COpt->EmuCore,
                                                    ChipClk, COpt->SpecialFlags & 0x01,
													p->VGMHead.bytAYFlagYM2608,
													(int*) &CAA->Paired->SmpRate,
                                                    p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &ym2608_stream_update;
                CAA->StreamUpdateParam = p->ym2608[CurChip];
				CAA->Paired->StreamUpdate = &ym2608_stream_update_ay;
                CAA->Paired->StreamUpdateParam = p->ym2608[CurChip];
				ym2608_set_srchg_cb(p->ym2608[CurChip], &ChangeChipSampleRate, CAA, CAA->Paired);

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				CAA->Paired->Volume = GetChipVolume(p, CAA->Paired->ChipType,
													CurChip, ChipCnt);
				AbsVol += CAA->Volume + CAA->Paired->Volume;
				//CAA->Volume = ChipVol;
				//CAA->Paired->Volume = ChipVol * 2;
			}
		}
		if (p->VGMHead.lngHzYM2610)
		{
			//ChipVol = 0x80;
			p->ChipOpts[0x01].YM2610.EmuCore = p->ChipOpts[0x00].YM2610.EmuCore;
			p->ChipOpts[0x01].YM2610.SpecialFlags = p->ChipOpts[0x00].YM2610.SpecialFlags;

			ChipCnt = (p->VGMHead.lngHzYM2610 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM2610;
				COpt = &p->ChipOpts[CurChip].YM2610;
				CAA->ChipType = 0x08;
				CAA->Paired = &p->CA_Paired[CurChip][0x02];
				CAA->Paired->ChipType = 0x80 | CAA->ChipType;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_ym2610(&p->ym2610[CurChip], COpt->EmuCore,
                                                    ChipClk, COpt->SpecialFlags & 0x01,
													(int*) &CAA->Paired->SmpRate,
                                                    p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = (ChipClk & 0x80000000) ? ym2610b_stream_update :
															ym2610_stream_update;
                CAA->StreamUpdateParam = p->ym2610[CurChip];
				CAA->Paired->StreamUpdate = &ym2610_stream_update_ay;
                CAA->Paired->StreamUpdateParam = p->ym2610[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				CAA->Paired->Volume = GetChipVolume(p, CAA->Paired->ChipType,
													CurChip, ChipCnt);
				AbsVol += CAA->Volume + CAA->Paired->Volume;
				//CAA->Volume = ChipVol;
				//CAA->Paired->Volume = ChipVol * 2;
			}
		}
		if (p->VGMHead.lngHzYM3812)
		{
			//ChipVol = UseFM ? 0x00 : 0x100;
			p->ChipOpts[0x01].YM3812.EmuCore = p->ChipOpts[0x00].YM3812.EmuCore;

			ChipCnt = (p->VGMHead.lngHzYM3812 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM3812;
				CAA->ChipType = 0x09;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
                CAA->SmpRate = device_start_ym3812(&p->ym3812[CurChip],
                                                    p->ChipOpts[CurChip].YM3812.EmuCore,
                                                    ChipClk,
                                                    p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
                if (ChipClk & 0x80000000)
                {
                    struct dual_opl2_info * info = (struct dual_opl2_info *) malloc(sizeof(struct dual_opl2_info));

                    CAA->StreamUpdate = dual_opl2_stereo;
                    CAA->StreamUpdateParam = (void *) info;

                    info->chip = p->ym3812[CurChip];
                    info->ChipID = CurChip;

                    p->ym3812_dual_data[CurChip] = (void *) info;
                }
                else
                {
                    CAA->StreamUpdate = ym3812_stream_update;
                    CAA->StreamUpdateParam = p->ym3812[CurChip];
										p->ym3812_dual_data[CurChip] = NULL;
                }

                CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
                if (! CurChip || ! (ChipClk & 0x80000000))
                    AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzYM3526)
		{
			//ChipVol = UseFM ? 0x00 : 0x100;
			ChipCnt = (p->VGMHead.lngHzYM3526 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YM3526;
				CAA->ChipType = 0x0A;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
                CAA->SmpRate = device_start_ym3526(&p->ym3526[CurChip], ChipClk,
                                                    p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
                CAA->StreamUpdate = &ym3526_stream_update;
                CAA->StreamUpdateParam = p->ym3526[CurChip];

                CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
                AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzY8950)
		{
			//ChipVol = UseFM ? 0x00 : 0x100;
			ChipCnt = (p->VGMHead.lngHzY8950 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].Y8950;
				CAA->ChipType = 0x0B;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
                CAA->SmpRate = device_start_y8950(&p->y8950[CurChip], ChipClk,
                                                   p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
                CAA->StreamUpdate = &y8950_stream_update;
                CAA->StreamUpdateParam = p->y8950[CurChip];

                CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
                AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzYMF262)
		{
			//ChipVol = UseFM ? 0x00 : 0x100;
			p->ChipOpts[0x01].YMF262.EmuCore = p->ChipOpts[0x00].YMF262.EmuCore;

			ChipCnt = (p->VGMHead.lngHzYMF262 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YMF262;
				CAA->ChipType = 0x0C;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
                CAA->SmpRate = device_start_ymf262(&p->ymf262[CurChip],
                                                    p->ChipOpts[CurChip].YMF262.EmuCore,
                                                    ChipClk,
                                                    p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
                CAA->StreamUpdate = &ymf262_stream_update;
                CAA->StreamUpdateParam = p->ymf262[CurChip];

                CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
                AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzYMF278B)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzYMF278B & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YMF278B;
				CAA->ChipType = 0x0D;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_ymf278b(&p->ymf278b[CurChip], ChipClk);
				CAA->StreamUpdate = &ymf278b_pcm_update;
                CAA->StreamUpdateParam = p->ymf278b[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;	//good as long as it only uses WaveTable Synth
			}
		}
		if (p->VGMHead.lngHzYMF271)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzYMF271 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YMF271;
				CAA->ChipType = 0x0E;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_ymf271(&p->ymf271[CurChip], ChipClk);
				CAA->StreamUpdate = &ymf271_update;
				CAA->StreamUpdateParam = p->ymf271[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzYMZ280B)
		{
			//ChipVol = 0x98;
			ChipCnt = (p->VGMHead.lngHzYMZ280B & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].YMZ280B;
				CAA->ChipType = 0x0F;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_ymz280b(&p->ymz280b[CurChip], ChipClk);
				CAA->StreamUpdate = &ymz280b_update;
                CAA->StreamUpdateParam = p->ymz280b[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += (CAA->Volume * 0x20 / 0x13);
			}
		}
		if (p->VGMHead.lngHzRF5C164)
		{
			//ChipVol = 0x80;
			ChipCnt = 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].RF5C164;
				CAA->ChipType = 0x10;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_rf5c164(&p->rf5c164, ChipClk,
                                                     p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &rf5c164_update;
                CAA->StreamUpdateParam = p->rf5c164;

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzPWM)
		{
			//ChipVol = 0xE0;	// 0xCD
			ChipCnt = 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].PWM;
				CAA->ChipType = 0x11;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_pwm(&p->pwm, ChipClk, p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &pwm_update;
                CAA->StreamUpdateParam = p->pwm;

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzAY8910)
		{
			//ChipVol = 0x100;
			p->ChipOpts[0x01].AY8910.EmuCore = p->ChipOpts[0x00].AY8910.EmuCore;

			ChipCnt = (p->VGMHead.lngHzAY8910 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].AY8910;
				CAA->ChipType = 0x12;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
                CAA->SmpRate = device_start_ayxx(&p->ay8910[CurChip],
                                                 p->ChipOpts[CurChip].AY8910.EmuCore,
                                                 ChipClk, p->VGMHead.bytAYType, p->VGMHead.bytAYFlag,
                                                 p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
                CAA->StreamUpdate = &ayxx_stream_update;
                CAA->StreamUpdateParam = p->ay8910[CurChip];

                CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
                AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzGBDMG)
		{
			//ChipVol = 0xC0;
			p->ChipOpts[0x01].GameBoy.SpecialFlags = p->ChipOpts[0x00].GameBoy.SpecialFlags;

			ChipCnt = (p->VGMHead.lngHzGBDMG & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].GameBoy;
				CAA->ChipType = 0x13;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_gameboy_sound(&p->gbdmg[CurChip], ChipClk,
                                                          p->ChipOpts[CurChip].GameBoy.SpecialFlags,
                                                          p->SampleRate);
				CAA->StreamUpdate = &gameboy_update;
                CAA->StreamUpdateParam = p->gbdmg[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzNESAPU)
		{
			//ChipVol = 0x100;
			p->ChipOpts[0x01].NES.EmuCore = p->ChipOpts[0x00].NES.EmuCore;
			p->ChipOpts[0x01].NES.SpecialFlags = p->ChipOpts[0x00].NES.SpecialFlags;

			ChipCnt = (p->VGMHead.lngHzNESAPU & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].NES;
                COpt = &p->ChipOpts[CurChip].NES;
				CAA->ChipType = 0x14;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_nes(&p->nesapu[CurChip], COpt->EmuCore,
                                                ChipClk, COpt->SpecialFlags,
                                                p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &nes_stream_update;
                CAA->StreamUpdateParam = p->nesapu[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzMultiPCM)
		{
			//ChipVol = 0x40;
			ChipCnt = (p->VGMHead.lngHzMultiPCM & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].MultiPCM;
				CAA->ChipType = 0x15;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_multipcm(&p->multipcm[CurChip], ChipClk);
				CAA->StreamUpdate = &MultiPCM_update;
                CAA->StreamUpdateParam = p->multipcm[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 4;
			}
		}
		if (p->VGMHead.lngHzUPD7759)
		{
			//ChipVol = 0x11E;
			ChipCnt = (p->VGMHead.lngHzUPD7759 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].UPD7759;
				CAA->ChipType = 0x16;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_upd7759(&p->upd7759[CurChip], ChipClk);
				CAA->StreamUpdate = &upd7759_update;
                CAA->StreamUpdateParam = p->upd7759[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzOKIM6258)
		{
			//ChipVol = 0x1C0;
			p->ChipOpts[0x01].OKIM6258.SpecialFlags = p->ChipOpts[0x00].OKIM6258.SpecialFlags;

			ChipCnt = (p->VGMHead.lngHzOKIM6258 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].OKIM6258;
				CAA->ChipType = 0x17;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_okim6258(&p->okim6258[CurChip], ChipClk,
                                                     p->ChipOpts[CurChip].OKIM6258.SpecialFlags,
													(p->VGMHead.bytOKI6258Flags & 0x03) >> 0,
													(p->VGMHead.bytOKI6258Flags & 0x04) >> 2,
													(p->VGMHead.bytOKI6258Flags & 0x08) >> 3);
				CAA->StreamUpdate = &okim6258_update;
                CAA->StreamUpdateParam = p->okim6258[CurChip];
				okim6258_set_srchg_cb(p->okim6258[CurChip], &ChangeChipSampleRate, CAA);

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzOKIM6295)
		{
			/*// Use the System Tag to decide between normal and CP System volume level.
			// I know, this is very hackish, but ATM there's no better solution.
			if (VGMTag.strSystemNameE != NULL && ! wcsncmp(VGMTag.strSystemNameE, L"CP", 0x02))
				ChipVol = 110;
			else
				ChipVol = 0x100;*/
			ChipCnt = (p->VGMHead.lngHzOKIM6295 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].OKIM6295;
				CAA->ChipType = 0x18;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_okim6295(&p->okim6295[CurChip], ChipClk);
				CAA->StreamUpdate = &okim6295_update;
                CAA->StreamUpdateParam = p->okim6295[CurChip];
				okim6295_set_srchg_cb(p->okim6295[CurChip], &ChangeChipSampleRate, CAA);

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 2;
			}
		}
		if (p->VGMHead.lngHzK051649)
		{
			//ChipVol = 0xA0;
			ChipCnt = (p->VGMHead.lngHzK051649 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].K051649;
				CAA->ChipType = 0x19;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_k051649(&p->k051649[CurChip], ChipClk);
				CAA->StreamUpdate = &k051649_update;
                CAA->StreamUpdateParam = p->k051649[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzK054539)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzK054539 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].K054539;
				CAA->ChipType = 0x1A;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_k054539(&p->k054539[CurChip], ChipClk);
				CAA->StreamUpdate = &k054539_update;
                CAA->StreamUpdateParam = p->k054539[CurChip];
				k054539_init_flags(p->k054539[CurChip], p->VGMHead.bytK054539Flags);

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzHuC6280)
		{
			//ChipVol = 0x100;
			p->ChipOpts[0x01].HuC6280.EmuCore = p->ChipOpts[0x00].HuC6280.EmuCore;

			ChipCnt = (p->VGMHead.lngHzHuC6280 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].HuC6280;
				CAA->ChipType = 0x1B;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_c6280(&p->huc6280[CurChip],
                                                  p->ChipOpts[CurChip].HuC6280.EmuCore,
                                                  ChipClk, p->SampleRate,
                                                  p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &c6280_update;
                CAA->StreamUpdateParam = p->huc6280[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzC140)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzC140 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].C140;
				CAA->ChipType = 0x1C;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_c140(&p->c140[CurChip], ChipClk, p->VGMHead.bytC140Type,
                                                 p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &c140_update;
                CAA->StreamUpdateParam = p->c140[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzK053260)
		{
			//ChipVol = 0xB3;
			ChipCnt = (p->VGMHead.lngHzK053260 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].K053260;
				CAA->ChipType = 0x1D;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_k053260(&p->k053260[CurChip], ChipClk);
				CAA->StreamUpdate = &k053260_update;
                CAA->StreamUpdateParam = p->k053260[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzPokey)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzPokey & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].Pokey;
				CAA->ChipType = 0x1E;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_pokey(&p->pokey[CurChip], ChipClk);
				CAA->StreamUpdate = &pokey_update;
                CAA->StreamUpdateParam = p->pokey[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzQSound)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzQSound & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].QSound;
				CAA->ChipType = 0x1F;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_qsound(&p->qsound[CurChip], ChipClk);
				CAA->StreamUpdate = &qsound_update;
                CAA->StreamUpdateParam = p->qsound[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzSCSP)
		{
			p->ChipOpts[0x01].SCSP.SpecialFlags = p->ChipOpts[0x00].SCSP.SpecialFlags;

			//ChipVol = 0x20;
			ChipCnt = (p->VGMHead.lngHzSCSP & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].SCSP;
				CAA->ChipType = 0x20;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_scsp(&p->scsp[CurChip], ChipClk, p->ChipOpts[CurChip].SCSP.SpecialFlags);
				CAA->StreamUpdate = &SCSP_Update;
                CAA->StreamUpdateParam = p->scsp[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 8;
			}
		}
		if (p->VGMHead.lngHzWSwan)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzWSwan & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].WSwan;
				CAA->ChipType = 0x21;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = ws_audio_init(&p->wswan[CurChip], ChipClk, p->SampleRate,
                                             p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &ws_audio_update;
                CAA->StreamUpdateParam = p->wswan[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzVSU)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzVSU & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].VSU;
				CAA->ChipType = 0x22;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_vsu(&p->vsu[CurChip], ChipClk, p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &vsu_stream_update;
                CAA->StreamUpdateParam = p->vsu[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzSAA1099)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzSAA1099 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].SAA1099;
				CAA->ChipType = 0x23;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_saa1099(&p->saa1099[CurChip], ChipClk);
				CAA->StreamUpdate = &saa1099_update;
                CAA->StreamUpdateParam = p->saa1099[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzES5503)
		{
			//ChipVol = 0x40;
			ChipCnt = (p->VGMHead.lngHzES5503 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].ES5503;
				CAA->ChipType = 0x24;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_es5503(&p->es5503[CurChip], ChipClk, p->VGMHead.bytES5503Chns);
				CAA->StreamUpdate = &es5503_pcm_update;
                CAA->StreamUpdateParam = p->es5503[CurChip];
				es5503_set_srchg_cb(p->es5503[CurChip], &ChangeChipSampleRate, CAA);

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 8;
			}
		}
		if (p->VGMHead.lngHzES5506)
		{
			//ChipVol = 0x20;
			ChipCnt = (p->VGMHead.lngHzES5506 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].ES5506;
				CAA->ChipType = 0x25;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_es5506(&p->es550x[CurChip], ChipClk, p->VGMHead.bytES5506Chns);
				CAA->StreamUpdate = &es5506_update;
                CAA->StreamUpdateParam = p->es550x[CurChip];
				es5506_set_srchg_cb(p->es550x[CurChip], &ChangeChipSampleRate, CAA);

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 16;
			}
		}
		if (p->VGMHead.lngHzX1_010)
		{
			//ChipVol = 0x100;
			ChipCnt = (p->VGMHead.lngHzX1_010 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].X1_010;
				CAA->ChipType = 0x26;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_x1_010(&p->x1_010[CurChip], ChipClk,
                                                   p->CHIP_SAMPLING_MODE, p->CHIP_SAMPLE_RATE);
				CAA->StreamUpdate = &seta_update;
                CAA->StreamUpdateParam = p->x1_010[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}
		if (p->VGMHead.lngHzC352)
		{
			//ChipVol = 0x40;
			ChipCnt = (p->VGMHead.lngHzC352 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].C352;
				CAA->ChipType = 0x27;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_c352(&p->c352[CurChip], ChipClk, p->VGMHead.bytC352ClkDiv * 4);
				CAA->StreamUpdate = &c352_update;
                CAA->StreamUpdateParam = p->c352[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume * 8;
			}
		}
		if (p->VGMHead.lngHzGA20)
		{
			//ChipVol = 0x280;
			ChipCnt = (p->VGMHead.lngHzGA20 & 0x40000000) ? 0x02 : 0x01;
			for (CurChip = 0x00; CurChip < ChipCnt; CurChip ++)
			{
				CAA = &p->ChipAudio[CurChip].GA20;
				CAA->ChipType = 0x28;

				ChipClk = GetChipClock(p, (CurChip << 7) | CAA->ChipType, NULL);
				CAA->SmpRate = device_start_iremga20(&p->ga20[CurChip], ChipClk);
				CAA->StreamUpdate = &IremGA20_update;
                CAA->StreamUpdateParam = p->ga20[CurChip];

				CAA->Volume = GetChipVolume(p, CAA->ChipType, CurChip, ChipCnt);
				AbsVol += CAA->Volume;
			}
		}

		// Initialize DAC Control and PCM Bank
		p->DacCtrlUsed = 0x00;
		//memset(DacCtrlUsg, 0x00, 0x01 * 0xFF);
		for (CurChip = 0x00; CurChip < 0xFF; CurChip ++)
		{
			p->DacCtrl[CurChip].Enable = false;
		}
		//memset(p->DacCtrl, 0x00, sizeof(DACCTRL_DATA) * 0xFF);

		memset(p->PCMBank, 0x00, sizeof(VGM_PCM_BANK) * PCM_BANK_COUNT);
		memset(&p->PCMTbl, 0x00, sizeof(PCMBANK_TBL));

		// Reset chips
		Chips_GeneralActions(p, 0x01);

		while(AbsVol < 0x200 && AbsVol)
		{
			for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
			{
				CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
				for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
					CAA->Volume *= 2;
				CAA = p->CA_Paired[CurCSet];
				for (CurChip = 0x00; CurChip < 0x03; CurChip ++, CAA ++)
					CAA->Volume *= 2;
			}
			AbsVol *= 2;
		}
		while(AbsVol > 0x300)
		{
			for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
			{
				CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
				for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
					CAA->Volume /= 2;
				CAA = p->CA_Paired[CurCSet];
				for (CurChip = 0x00; CurChip < 0x03; CurChip ++, CAA ++)
					CAA->Volume /= 2;
			}
			AbsVol /= 2;
		}

		// Initialize Resampler
		for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
		{
			CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
			for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
				SetupResampler(p, CAA);

			CAA = p->CA_Paired[CurCSet];
			for (CurChip = 0x00; CurChip < 0x03; CurChip ++, CAA ++)
				SetupResampler(p, CAA);
		}

		GeneralChipLists(p);
		break;
	case 0x01:	// Reset chips
		for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
		{

		CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
		for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
		{
			if (CAA->ChipType == 0xFF)	// chip unused
				continue;
			else if (CAA->ChipType == 0x00)
				device_reset_sn764xx(p->sn764xx[CurCSet]);
			else if (CAA->ChipType == 0x01)
				device_reset_ym2413(p->ym2413[CurCSet]);
			else if (CAA->ChipType == 0x02)
				device_reset_ym2612(p->ym2612[CurCSet]);
			else if (CAA->ChipType == 0x03)
				device_reset_ym2151(p->ym2151[CurCSet]);
			else if (CAA->ChipType == 0x04)
				device_reset_segapcm(p->segapcm[CurCSet]);
			else if (CAA->ChipType == 0x05 && CurCSet == 0x00)
				device_reset_rf5c68(p->rf5c68);
			else if (CAA->ChipType == 0x06)
				device_reset_ym2203(p->ym2203[CurCSet]);
			else if (CAA->ChipType == 0x07)
				device_reset_ym2608(p->ym2608[CurCSet]);
			else if (CAA->ChipType == 0x08)
				device_reset_ym2610(p->ym2610[CurCSet]);
			else if (CAA->ChipType == 0x09)
                device_reset_ym3812(p->ym3812[CurCSet]);
			else if (CAA->ChipType == 0x0A)
				device_reset_ym3526(p->ym3526[CurCSet]);
			else if (CAA->ChipType == 0x0B)
				device_reset_y8950(p->y8950[CurCSet]);
			else if (CAA->ChipType == 0x0C)
                device_reset_ymf262(p->ymf262[CurCSet]);
			else if (CAA->ChipType == 0x0D)
				device_reset_ymf278b(p->ymf278b[CurCSet]);
			else if (CAA->ChipType == 0x0E)
				device_reset_ymf271(p->ymf271[CurCSet]);
			else if (CAA->ChipType == 0x0F)
				device_reset_ymz280b(p->ymz280b[CurCSet]);
			else if (CAA->ChipType == 0x10 && CurCSet == 0x00)
				device_reset_rf5c164(p->rf5c164);
			else if (CAA->ChipType == 0x11 && CurCSet == 0x00)
				device_reset_pwm(p->pwm);
			else if (CAA->ChipType == 0x12)
				device_reset_ayxx(p->ay8910[CurCSet]);
			else if (CAA->ChipType == 0x13)
				device_reset_gameboy_sound(p->gbdmg[CurCSet]);
			else if (CAA->ChipType == 0x14)
				device_reset_nes(p->nesapu[CurCSet]);
			else if (CAA->ChipType == 0x15)
				device_reset_multipcm(p->multipcm[CurCSet]);
			else if (CAA->ChipType == 0x16)
				device_reset_upd7759(p->upd7759[CurCSet]);
			else if (CAA->ChipType == 0x17)
				device_reset_okim6258(p->okim6258[CurCSet]);
			else if (CAA->ChipType == 0x18)
				device_reset_okim6295(p->okim6295[CurCSet]);
			else if (CAA->ChipType == 0x19)
				device_reset_k051649(p->k051649[CurCSet]);
			else if (CAA->ChipType == 0x1A)
				device_reset_k054539(p->k054539[CurCSet]);
			else if (CAA->ChipType == 0x1B)
				device_reset_c6280(p->huc6280[CurCSet]);
			else if (CAA->ChipType == 0x1C)
				device_reset_c140(p->c140[CurCSet]);
			else if (CAA->ChipType == 0x1D)
				device_reset_k053260(p->k053260[CurCSet]);
			else if (CAA->ChipType == 0x1E)
				device_reset_pokey(p->pokey[CurCSet]);
			else if (CAA->ChipType == 0x1F)
				device_reset_qsound(p->qsound[CurCSet]);
			else if (CAA->ChipType == 0x20)
				device_reset_scsp(p->scsp[CurCSet]);
			else if (CAA->ChipType == 0x21)
				ws_audio_reset(p->wswan[CurCSet]);
			else if (CAA->ChipType == 0x22)
				device_reset_vsu(p->vsu[CurCSet]);
			else if (CAA->ChipType == 0x23)
				device_reset_saa1099(p->saa1099[CurCSet]);
			else if (CAA->ChipType == 0x24)
				device_reset_es5503(p->es5503[CurCSet]);
			else if (CAA->ChipType == 0x25)
				device_reset_es5506(p->es550x[CurCSet]);
			else if (CAA->ChipType == 0x26)
				device_reset_x1_010(p->x1_010[CurCSet]);
			else if (CAA->ChipType == 0x27)
				device_reset_c352(p->c352[CurCSet]);
			else if (CAA->ChipType == 0x28)
				device_reset_iremga20(p->ga20[CurCSet]);
		}	// end for CurChip

		}	// end for CurCSet

		Chips_GeneralActions(p, 0x10);	// set muting mask
		Chips_GeneralActions(p, 0x20);	// set panning

		for (CurChip = 0x00; CurChip < p->DacCtrlUsed; CurChip ++)
		{
			CurCSet = p->DacCtrlUsg[CurChip];
			device_reset_daccontrol(p->daccontrol[CurCSet]);
			//DacCtrl[CurCSet].Enable = false;
		}
		//DacCtrlUsed = 0x00;
		//memset(DacCtrlUsg, 0x00, 0x01 * 0xFF);

		for (CurChip = 0x00; CurChip < PCM_BANK_COUNT; CurChip ++)
		{
			// reset PCM Bank, but not the data
			// (this way I don't need to decompress the data again when restarting)
			p->PCMBank[CurChip].DataPos = 0x00000000;
			p->PCMBank[CurChip].BnkPos = 0x00000000;
		}
		p->PCMTbl.EntryCount = 0x00;
		break;
	case 0x02:	// Stop chips
		for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
		{

		CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
		for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
		{
			if (CAA->ChipType == 0xFF)	// chip unused
				continue;
			else if (CAA->ChipType == 0x00)
				device_stop_sn764xx(p->sn764xx[CurCSet]);
			else if (CAA->ChipType == 0x01)
				device_stop_ym2413(p->ym2413[CurCSet]);
			else if (CAA->ChipType == 0x02)
				device_stop_ym2612(p->ym2612[CurCSet]);
			else if (CAA->ChipType == 0x03)
				device_stop_ym2151(p->ym2151[CurCSet]);
			else if (CAA->ChipType == 0x04)
				device_stop_segapcm(p->segapcm[CurCSet]);
			else if (CAA->ChipType == 0x05 && CurCSet == 0x00)
				device_stop_rf5c68(p->rf5c68);
			else if (CAA->ChipType == 0x06)
				device_stop_ym2203(p->ym2203[CurCSet]);
			else if (CAA->ChipType == 0x07)
				device_stop_ym2608(p->ym2608[CurCSet]);
			else if (CAA->ChipType == 0x08)
				device_stop_ym2610(p->ym2610[CurCSet]);
			else if (CAA->ChipType == 0x09)
			{
				device_stop_ym3812(p->ym3812[CurCSet]);
				free(p->ym3812_dual_data[CurCSet]);
				p->ym3812_dual_data[CurCSet] = NULL;
			}
			else if (CAA->ChipType == 0x0A)
				device_stop_ym3526(p->ym3526[CurCSet]);
			else if (CAA->ChipType == 0x0B)
				device_stop_y8950(p->y8950[CurCSet]);
			else if (CAA->ChipType == 0x0C)
				device_stop_ymf262(p->ymf262[CurCSet]);
			else if (CAA->ChipType == 0x0D)
				device_stop_ymf278b(p->ymf278b[CurCSet]);
			else if (CAA->ChipType == 0x0E)
				device_stop_ymf271(p->ymf271[CurCSet]);
			else if (CAA->ChipType == 0x0F)
				device_stop_ymz280b(p->ymz280b[CurCSet]);
			else if (CAA->ChipType == 0x10 && CurCSet == 0x00)
				device_stop_rf5c164(p->rf5c164);
			else if (CAA->ChipType == 0x11 && CurCSet == 0x00)
				device_stop_pwm(p->pwm);
			else if (CAA->ChipType == 0x12)
				device_stop_ayxx(p->ay8910[CurCSet]);
			else if (CAA->ChipType == 0x13)
				device_stop_gameboy_sound(p->gbdmg[CurCSet]);
			else if (CAA->ChipType == 0x14)
				device_stop_nes(p->nesapu[CurCSet]);
			else if (CAA->ChipType == 0x15)
				device_stop_multipcm(p->multipcm[CurCSet]);
			else if (CAA->ChipType == 0x16)
				device_stop_upd7759(p->upd7759[CurCSet]);
			else if (CAA->ChipType == 0x17)
				device_stop_okim6258(p->okim6258[CurCSet]);
			else if (CAA->ChipType == 0x18)
				device_stop_okim6295(p->okim6295[CurCSet]);
			else if (CAA->ChipType == 0x19)
				device_stop_k051649(p->k051649[CurCSet]);
			else if (CAA->ChipType == 0x1A)
				device_stop_k054539(p->k054539[CurCSet]);
			else if (CAA->ChipType == 0x1B)
				device_stop_c6280(p->huc6280[CurCSet]);
			else if (CAA->ChipType == 0x1C)
				device_stop_c140(p->c140[CurCSet]);
			else if (CAA->ChipType == 0x1D)
				device_stop_k053260(p->k053260[CurCSet]);
			else if (CAA->ChipType == 0x1E)
				device_stop_pokey(p->pokey[CurCSet]);
			else if (CAA->ChipType == 0x1F)
				device_stop_qsound(p->qsound[CurCSet]);
			else if (CAA->ChipType == 0x20)
				device_stop_scsp(p->scsp[CurCSet]);
			else if (CAA->ChipType == 0x21)
				ws_audio_done(p->wswan[CurCSet]);
			else if (CAA->ChipType == 0x22)
				device_stop_vsu(p->vsu[CurCSet]);
			else if (CAA->ChipType == 0x23)
				device_stop_saa1099(p->saa1099[CurCSet]);
			else if (CAA->ChipType == 0x24)
				device_stop_es5503(p->es5503[CurCSet]);
			else if (CAA->ChipType == 0x25)
				device_stop_es5506(p->es550x[CurCSet]);
			else if (CAA->ChipType == 0x26)
				device_stop_x1_010(p->x1_010[CurCSet]);
			else if (CAA->ChipType == 0x27)
				device_stop_c352(p->c352[CurCSet]);
			else if (CAA->ChipType == 0x28)
				device_stop_iremga20(p->ga20[CurCSet]);

		  resampler_destroy(CAA->Resampler);
			CAA->Resampler = 0x00;

			CAA->ChipType = 0xFF;	// mark as "unused"
		}	// end for CurChip

		}	// end for CurCSet

		for (CurChip = 0x00; CurChip < p->DacCtrlUsed; CurChip ++)
		{
			CurCSet = p->DacCtrlUsg[CurChip];
			device_stop_daccontrol(p->daccontrol[CurCSet]);
			p->DacCtrl[CurCSet].Enable = false;
		}
		p->DacCtrlUsed = 0x00;

		for (CurChip = 0x00; CurChip < PCM_BANK_COUNT; CurChip ++)
		{
			free(p->PCMBank[CurChip].Bank);
			free(p->PCMBank[CurChip].Data);
		}
		//memset(PCMBank, 0x00, sizeof(VGM_PCM_BANK) * PCM_BANK_COUNT);
		free(p->PCMTbl.Entries);
		//memset(&PCMTbl, 0x00, sizeof(PCMBANK_TBL));
		break;
	case 0x10:	// Set Muting Mask
		for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
		{

		CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
		for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
		{
			if (CAA->ChipType == 0xFF)	// chip unused
				continue;
			else if (CAA->ChipType == 0x00)
				sn764xx_set_mute_mask(p->sn764xx[CurCSet], p->ChipOpts[CurCSet].SN76496.ChnMute1);
			else if (CAA->ChipType == 0x01)
				ym2413_set_mute_mask(p->ym2413[CurCSet], p->ChipOpts[CurCSet].YM2413.ChnMute1);
			else if (CAA->ChipType == 0x02)
				ym2612_set_mute_mask(p->ym2612[CurCSet], p->ChipOpts[CurCSet].YM2612.ChnMute1);
			else if (CAA->ChipType == 0x03)
				ym2151_set_mute_mask(p->ym2151[CurCSet], p->ChipOpts[CurCSet].YM2151.ChnMute1);
			else if (CAA->ChipType == 0x04)
				segapcm_set_mute_mask(p->segapcm[CurCSet], p->ChipOpts[CurCSet].SegaPCM.ChnMute1);
			else if (CAA->ChipType == 0x05 && CurCSet == 0x00)
				rf5c68_set_mute_mask(p->rf5c68, p->ChipOpts[CurCSet].RF5C68.ChnMute1);
			else if (CAA->ChipType == 0x06)
				ym2203_set_mute_mask(p->ym2203[CurCSet], p->ChipOpts[CurCSet].YM2203.ChnMute1,
									p->ChipOpts[CurCSet].YM2203.ChnMute3);
			else if (CAA->ChipType == 0x07)
			{
				MaskVal  = (p->ChipOpts[CurCSet].YM2608.ChnMute1 & 0x3F) << 0;
				MaskVal |= (p->ChipOpts[CurCSet].YM2608.ChnMute2 & 0x7F) << 6;
				ym2608_set_mute_mask(p->ym2608[CurCSet], MaskVal, p->ChipOpts[CurCSet].YM2608.ChnMute3);
			}
			else if (CAA->ChipType == 0x08)
			{
				MaskVal  = (p->ChipOpts[CurCSet].YM2610.ChnMute1 & 0x3F) << 0;
				MaskVal |= (p->ChipOpts[CurCSet].YM2610.ChnMute2 & 0x7F) << 6;
				ym2610_set_mute_mask(p->ym2610[CurCSet], MaskVal, p->ChipOpts[CurCSet].YM2610.ChnMute3);
			}
			else if (CAA->ChipType == 0x09)
				ym3812_set_mute_mask(p->ym3812[CurCSet], p->ChipOpts[CurCSet].YM3812.ChnMute1);
			else if (CAA->ChipType == 0x0A)
				ym3526_set_mute_mask(p->ym3526[CurCSet], p->ChipOpts[CurCSet].YM3526.ChnMute1);
			else if (CAA->ChipType == 0x0B)
				y8950_set_mute_mask(p->y8950[CurCSet], p->ChipOpts[CurCSet].Y8950.ChnMute1);
			else if (CAA->ChipType == 0x0C)
				ymf262_set_mute_mask(p->ymf262[CurCSet], p->ChipOpts[CurCSet].YMF262.ChnMute1);
			else if (CAA->ChipType == 0x0D)
				ymf278b_set_mute_mask(p->ymf278b[CurCSet], p->ChipOpts[CurCSet].YMF278B.ChnMute1,
										p->ChipOpts[CurCSet].YMF278B.ChnMute2);
			else if (CAA->ChipType == 0x0E)
				ymf271_set_mute_mask(p->ymf271[CurCSet], p->ChipOpts[CurCSet].YMF271.ChnMute1);
			else if (CAA->ChipType == 0x0F)
				ymz280b_set_mute_mask(p->ymz280b[CurCSet], p->ChipOpts[CurCSet].YMZ280B.ChnMute1);
			else if (CAA->ChipType == 0x10 && CurCSet == 0x00)
				rf5c164_set_mute_mask(p->rf5c164, p->ChipOpts[CurCSet].RF5C164.ChnMute1);
            else if (CAA->ChipType == 0x11 && CurCSet == 0x00)
                pwm_mute(p->pwm, p->ChipOpts[CurCSet].PWM.ChnMute1);
			else if (CAA->ChipType == 0x12)
				ayxx_set_mute_mask(p->ay8910[CurCSet], p->ChipOpts[CurCSet].AY8910.ChnMute1);
			else if (CAA->ChipType == 0x13)
				gameboy_sound_set_mute_mask(p->gbdmg[CurCSet], p->ChipOpts[CurCSet].GameBoy.ChnMute1);
			else if (CAA->ChipType == 0x14)
				nes_set_mute_mask(p->nesapu[CurCSet], p->ChipOpts[CurCSet].NES.ChnMute1);
			else if (CAA->ChipType == 0x15)
				multipcm_set_mute_mask(p->multipcm[CurCSet], p->ChipOpts[CurCSet].MultiPCM.ChnMute1);
			else if (CAA->ChipType == 0x16)
                upd7759_mute(p->upd7759[CurCSet], p->ChipOpts[CurCSet].UPD7759.ChnMute1);
			else if (CAA->ChipType == 0x17)
                okim6258_mute(p->okim6258[CurCSet], p->ChipOpts[CurCSet].OKIM6258.ChnMute1);
			else if (CAA->ChipType == 0x18)
				okim6295_set_mute_mask(p->okim6295[CurCSet], p->ChipOpts[CurCSet].OKIM6295.ChnMute1);
			else if (CAA->ChipType == 0x19)
				k051649_set_mute_mask(p->k051649[CurCSet], p->ChipOpts[CurCSet].K051649.ChnMute1);
			else if (CAA->ChipType == 0x1A)
				k054539_set_mute_mask(p->k054539[CurCSet], p->ChipOpts[CurCSet].K054539.ChnMute1);
			else if (CAA->ChipType == 0x1B)
				c6280_set_mute_mask(p->huc6280[CurCSet], p->ChipOpts[CurCSet].HuC6280.ChnMute1);
			else if (CAA->ChipType == 0x1C)
				c140_set_mute_mask(p->c140[CurCSet], p->ChipOpts[CurCSet].C140.ChnMute1);
			else if (CAA->ChipType == 0x1D)
				k053260_set_mute_mask(p->k053260[CurCSet], p->ChipOpts[CurCSet].K053260.ChnMute1);
			else if (CAA->ChipType == 0x1E)
				pokey_set_mute_mask(p->pokey[CurCSet], p->ChipOpts[CurCSet].Pokey.ChnMute1);
			else if (CAA->ChipType == 0x1F)
				qsound_set_mute_mask(p->qsound[CurCSet], p->ChipOpts[CurCSet].QSound.ChnMute1);
			else if (CAA->ChipType == 0x20)
				scsp_set_mute_mask(p->scsp[CurCSet], p->ChipOpts[CurCSet].SCSP.ChnMute1);
			else if (CAA->ChipType == 0x21)
				ws_set_mute_mask(p->wswan[CurCSet], p->ChipOpts[CurCSet].WSwan.ChnMute1);
			else if (CAA->ChipType == 0x22)
				vsu_set_mute_mask(p->vsu[CurCSet], p->ChipOpts[CurCSet].VSU.ChnMute1);
			else if (CAA->ChipType == 0x23)
				saa1099_set_mute_mask(p->saa1099[CurCSet], p->ChipOpts[CurCSet].SAA1099.ChnMute1);
			else if (CAA->ChipType == 0x24)
				es5503_set_mute_mask(p->es5503[CurCSet], p->ChipOpts[CurCSet].ES5503.ChnMute1);
			else if (CAA->ChipType == 0x25)
				es5506_set_mute_mask(p->es550x[CurCSet], p->ChipOpts[CurCSet].ES5506.ChnMute1);
			else if (CAA->ChipType == 0x26)
				x1_010_set_mute_mask(p->x1_010[CurCSet], p->ChipOpts[CurCSet].X1_010.ChnMute1);
			else if (CAA->ChipType == 0x27)
				c352_set_mute_mask(p->c352[CurCSet], p->ChipOpts[CurCSet].C352.ChnMute1);
			else if (CAA->ChipType == 0x28)
				iremga20_set_mute_mask(p->ga20[CurCSet], p->ChipOpts[CurCSet].GA20.ChnMute1);
		}	// end for CurChip

		}	// end for CurCSet
		break;
	case 0x20:	// Set Panning
		for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
		{

		CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet];
		for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, CAA ++)
		{
			if (CAA->ChipType == 0xFF)	// chip unused
				continue;
			else if (CAA->ChipType == 0x00)
				sn764xx_set_panning(p->sn764xx[CurCSet], p->ChipOpts[CurCSet].SN76496.Panning);
			else if (CAA->ChipType == 0x01)
				ym2413_set_panning(p->ym2413[CurCSet], p->ChipOpts[CurCSet].YM2413.Panning);
		}	// end for CurChip

		}	// end for CurCSet
		break;
	}

	return;
}

INLINE INT32 SampleVGM2Pbk_I(VGM_PLAYER* p, INT32 SampleVal)
{
	return (INT32)((INT64)SampleVal * p->VGMSmplRateMul / p->VGMSmplRateDiv);
}

INLINE INT32 SamplePbk2VGM_I(VGM_PLAYER* p, INT32 SampleVal)
{
	return (INT32)((INT64)SampleVal * p->VGMSmplRateDiv / p->VGMSmplRateMul);
}

INT32 SampleVGM2Playback(void* _p, INT32 SampleVal)
{
    VGM_PLAYER* p = (VGM_PLAYER *)_p;
	return (INT32)((INT64)SampleVal * p->VGMSmplRateMul / p->VGMSmplRateDiv);
}

INT32 SamplePlayback2VGM(void* _p, INT32 SampleVal)
{
    VGM_PLAYER* p = (VGM_PLAYER *)_p;
	return (INT32)((INT64)SampleVal * p->VGMSmplRateDiv / p->VGMSmplRateMul);
}


static void InterpretFile(VGM_PLAYER* p, UINT32 SampleCount)
{
	/*UINT32 TempLng;*/
	UINT8 CurChip;

	if (p->DacCtrlUsed && SampleCount > 1)	// handle skipping
	{
		for (CurChip = 0x00; CurChip < p->DacCtrlUsed; CurChip ++)
		{
			daccontrol_update(p->daccontrol[p->DacCtrlUsg[CurChip]], SampleCount - 1);
		}
	}

	if (! p->FileMode)
		InterpretVGM(p, SampleCount);
#ifdef ADDITIONAL_FORMATS
	else
		InterpretOther(p, SampleCount);
#endif

	if (p->DacCtrlUsed && SampleCount)
	{
		// calling this here makes "Emulating while Paused" nicer
		for (CurChip = 0x00; CurChip < p->DacCtrlUsed; CurChip ++)
		{
			daccontrol_update(p->daccontrol[p->DacCtrlUsg[CurChip]], 1);
		}
	}

	p->VGMSmplPlayed += SampleCount;
	p->PlayingTime += SampleCount;

	//if (FadePlay && ! FadeTime)
	//	EndPlay = true;

	return;
}

static void AddPCMData(VGM_PLAYER* p, UINT8 Type, UINT32 DataSize, const UINT8* Data)
{
	UINT32 CurBnk;
	VGM_PCM_BANK* TempPCM;
	VGM_PCM_DATA* TempBnk;
	UINT32 BankSize;
	bool RetVal;
	UINT8 BnkType;
	UINT8 CurDAC;

	BnkType = Type & 0x3F;
	if (BnkType >= PCM_BANK_COUNT || p->VGMCurLoop)
		return;

	if (Type == 0x7F)
	{
		ReadPCMTable(p, DataSize, Data);
		return;
	}

	TempPCM = &p->PCMBank[BnkType];
	TempPCM->BnkPos ++;
	if (TempPCM->BnkPos <= TempPCM->BankCount)
		return;	// Speed hack for restarting playback (skip already loaded blocks)
	CurBnk = TempPCM->BankCount;
	TempPCM->BankCount ++;
	if (p->Last95Max != 0xFFFF)
		p->Last95Max = TempPCM->BankCount;
	TempPCM->Bank = (VGM_PCM_DATA*)realloc(TempPCM->Bank,
											sizeof(VGM_PCM_DATA) * TempPCM->BankCount);

	if (! (Type & 0x40))
		BankSize = DataSize;
	else
		BankSize = ReadLE32(&Data[0x01]);
	TempPCM->Data = realloc(TempPCM->Data, TempPCM->DataSize + BankSize);
	TempBnk = &TempPCM->Bank[CurBnk];
	TempBnk->DataStart = TempPCM->DataSize;
	if (! (Type & 0x40))
	{
		TempBnk->DataSize = DataSize;
		TempBnk->Data = TempPCM->Data + TempBnk->DataStart;
		memcpy(TempBnk->Data, Data, DataSize);
	}
	else
	{
		TempBnk->Data = TempPCM->Data + TempBnk->DataStart;
		RetVal = DecompressDataBlk(p, TempBnk, DataSize, Data);
		if (! RetVal)
		{
			TempBnk->Data = NULL;
			TempBnk->DataSize = 0x00;
			//return;
			goto RefreshDACStrm;	// sorry for the goto, but I don't want to copy-paste the code
		}
	}
	if (BankSize != TempBnk->DataSize)
		printf("Error reading Data Block! Data Size conflict!\n");
	TempPCM->DataSize += BankSize;

	// realloc may've moved the Bank block, so refresh all DAC Streams
RefreshDACStrm:
	for (CurDAC = 0x00; CurDAC < p->DacCtrlUsed; CurDAC ++)
	{
		if (p->DacCtrl[p->DacCtrlUsg[CurDAC]].Bank == BnkType)
			daccontrol_refresh_data(p->daccontrol[p->DacCtrlUsg[CurDAC]], TempPCM->Data, TempPCM->DataSize);
	}

	return;
}

/*INLINE FUINT16 ReadBits(UINT8* Data, UINT32* Pos, FUINT8* BitPos, FUINT8 BitsToRead)
{
	FUINT8 BitReadVal;
	UINT32 InPos;
	FUINT8 InVal;
	FUINT8 BitMask;
	FUINT8 InShift;
	FUINT8 OutBit;
	FUINT16 RetVal;

	InPos = *Pos;
	InShift = *BitPos;
	OutBit = 0x00;
	RetVal = 0x0000;
	while(BitsToRead)
	{
		BitReadVal = (BitsToRead >= 8) ? 8 : BitsToRead;
		BitsToRead -= BitReadVal;
		BitMask = (1 << BitReadVal) - 1;

		InShift += BitReadVal;
		InVal = (Data[InPos] << InShift >> 8) & BitMask;
		if (InShift >= 8)
		{
			InShift -= 8;
			InPos ++;
			if (InShift)
				InVal |= (Data[InPos] << InShift >> 8) & BitMask;
		}

		RetVal |= InVal << OutBit;
		OutBit += BitReadVal;
	}

	*Pos = InPos;
	*BitPos = InShift;
	return RetVal;
}

static void DecompressDataBlk(VGM_PCM_DATA* Bank, UINT32 DataSize, const UINT8* Data)
{
	UINT8 ComprType;
	UINT8 BitDec;
	FUINT8 BitCmp;
	UINT8 CmpSubType;
	UINT16 AddVal;
	UINT32 InPos;
	UINT32 OutPos;
	FUINT16 InVal;
	FUINT16 OutVal;
	FUINT8 ValSize;
	FUINT8 InShift;
	FUINT8 OutShift;
	UINT8* Ent1B;
	UINT16* Ent2B;
	//UINT32 Time;

	//Time = GetTickCount();
	ComprType = Data[0x00];
	Bank->DataSize = ReadLE32(&Data[0x01]);
	BitDec = Data[0x05];
	BitCmp = Data[0x06];
	CmpSubType = Data[0x07];
	AddVal = ReadLE16(&Data[0x08]);

	switch(ComprType)
	{
	case 0x00:	// n-Bit compression
		if (CmpSubType == 0x02)
		{
			Ent1B = (UINT8*)PCMTbl.Entries;
			Ent2B = (UINT16*)PCMTbl.Entries;
			if (! PCMTbl.EntryCount)
			{
				printf("Error loading table-compressed data block! No table loaded!\n");
				return;
			}
			else if (BitDec != PCMTbl.BitDec || BitCmp != PCMTbl.BitCmp)
			{
				printf("Warning! Data block and loaded value table incompatible!\n");
				return;
			}
		}

		ValSize = (BitDec + 7) / 8;
		InPos = 0x0A;
		InShift = 0;
		OutShift = BitDec - BitCmp;

		for (OutPos = 0x00; OutPos < Bank->DataSize; OutPos += ValSize)
		{
			if (InPos >= DataSize)
				break;
			InVal = ReadBits(Data, &InPos, &InShift, BitCmp);
			switch(CmpSubType)
			{
			case 0x00:	// Copy
				OutVal = InVal + AddVal;
				break;
			case 0x01:	// Shift Left
				OutVal = (InVal << OutShift) + AddVal;
				break;
			case 0x02:	// Table
				switch(ValSize)
				{
				case 0x01:
					OutVal = Ent1B[InVal];
					break;
				case 0x02:
					OutVal = Ent2B[InVal];
					break;
				}
				break;
			}
			memcpy(&Bank->Data[OutPos], &OutVal, ValSize);
		}
		break;
	}

	//Time = GetTickCount() - Time;
	//printf("Decompression Time: %lu\n", Time);

	return;
}*/

static bool DecompressDataBlk(VGM_PLAYER* p, VGM_PCM_DATA* Bank, UINT32 DataSize, const UINT8* Data)
{
	UINT8 ComprType;
	UINT8 BitDec;
	FUINT8 BitCmp;
	UINT8 CmpSubType;
	UINT16 AddVal;
	const UINT8* InPos;
	const UINT8* InDataEnd;
	UINT8* OutPos;
	const UINT8* OutDataEnd;
	FUINT16 InVal;
	FUINT16 OutVal;
	FUINT8 ValSize;
	FUINT8 InShift;
	FUINT8 OutShift;
	UINT8* Ent1B;
	UINT16* Ent2B;

	// ReadBits Variables
	FUINT8 BitsToRead;
	FUINT8 BitReadVal;
	FUINT8 InValB;
	FUINT8 BitMask;
	FUINT8 OutBit;

	// Variables for DPCM
	UINT16 OutMask;

	ComprType = Data[0x00];
	Bank->DataSize = ReadLE32(&Data[0x01]);

	switch(ComprType)
	{
	case 0x00:	// n-Bit compression
		BitDec = Data[0x05];
		BitCmp = Data[0x06];
		CmpSubType = Data[0x07];
		AddVal = ReadLE16(&Data[0x08]);

		if (CmpSubType == 0x02)
		{
			Ent1B = (UINT8*)p->PCMTbl.Entries;	// Big Endian note: Those are stored in LE and converted when reading.
			Ent2B = (UINT16*)p->PCMTbl.Entries;
			if (! p->PCMTbl.EntryCount)
			{
				Bank->DataSize = 0x00;
				printf("Error loading table-compressed data block! No table loaded!\n");
				return false;
			}
			else if (BitDec != p->PCMTbl.BitDec || BitCmp != p->PCMTbl.BitCmp)
			{
				Bank->DataSize = 0x00;
				printf("Warning! Data block and loaded value table incompatible!\n");
				return false;
			}
		}

		ValSize = (BitDec + 7) / 8;
		InPos = Data + 0x0A;
		InDataEnd = Data + DataSize;
		InShift = 0;
		OutShift = BitDec - BitCmp;
		OutDataEnd = Bank->Data + Bank->DataSize;

		for (OutPos = Bank->Data; OutPos < OutDataEnd && InPos < InDataEnd; OutPos += ValSize)
		{
			//InVal = ReadBits(Data, InPos, &InShift, BitCmp);
			// inlined - is 30% faster
			OutBit = 0x00;
			InVal = 0x0000;
			BitsToRead = BitCmp;
			while(BitsToRead)
			{
				BitReadVal = (BitsToRead >= 8) ? 8 : BitsToRead;
				BitsToRead -= BitReadVal;
				BitMask = (1 << BitReadVal) - 1;

				InShift += BitReadVal;
				InValB = (*InPos << InShift >> 8) & BitMask;
				if (InShift >= 8)
				{
					InShift -= 8;
					InPos ++;
					if (InShift)
						InValB |= (*InPos << InShift >> 8) & BitMask;
				}

				InVal |= InValB << OutBit;
				OutBit += BitReadVal;
			}

			switch(CmpSubType)
			{
			case 0x00:	// Copy
				OutVal = InVal + AddVal;
				break;
			case 0x01:	// Shift Left
				OutVal = (InVal << OutShift) + AddVal;
				break;
			case 0x02:	// Table
				switch(ValSize)
				{
				case 0x01:
					OutVal = Ent1B[InVal];
					break;
				case 0x02:
#ifndef VGM_BIG_ENDIAN
					OutVal = Ent2B[InVal];
#else
					OutVal = ReadLE16((UINT8*)&Ent2B[InVal]);
#endif
					break;
				}
				break;
			}

#ifndef VGM_BIG_ENDIAN
			//memcpy(OutPos, &OutVal, ValSize);
			if (ValSize == 0x01)
				*((UINT8*)OutPos) = (UINT8)OutVal;
			else //if (ValSize == 0x02)
				*((UINT16*)OutPos) = (UINT16)OutVal;
#else
			if (ValSize == 0x01)
			{
				*OutPos = (UINT8)OutVal;
			}
			else //if (ValSize == 0x02)
			{
				OutPos[0x00] = (UINT8)((OutVal & 0x00FF) >> 0);
				OutPos[0x01] = (UINT8)((OutVal & 0xFF00) >> 8);
			}
#endif
		}
		break;
	case 0x01:	// Delta-PCM
		BitDec = Data[0x05];
		BitCmp = Data[0x06];
		OutVal = ReadLE16(&Data[0x08]);

		Ent1B = (UINT8*)p->PCMTbl.Entries;
		Ent2B = (UINT16*)p->PCMTbl.Entries;
		if (! p->PCMTbl.EntryCount)
		{
			Bank->DataSize = 0x00;
			printf("Error loading table-compressed data block! No table loaded!\n");
			return false;
		}
		else if (BitDec != p->PCMTbl.BitDec || BitCmp != p->PCMTbl.BitCmp)
		{
			Bank->DataSize = 0x00;
			printf("Warning! Data block and loaded value table incompatible!\n");
			return false;
		}

		ValSize = (BitDec + 7) / 8;
		OutMask = (1 << BitDec) - 1;
		InPos = Data + 0x0A;
		InDataEnd = Data + DataSize;
		InShift = 0;
		OutShift = BitDec - BitCmp;
		OutDataEnd = Bank->Data + Bank->DataSize;
		AddVal = 0x0000;

		for (OutPos = Bank->Data; OutPos < OutDataEnd && InPos < InDataEnd; OutPos += ValSize)
		{
			//InVal = ReadBits(Data, InPos, &InShift, BitCmp);
			// inlined - is 30% faster
			OutBit = 0x00;
			InVal = 0x0000;
			BitsToRead = BitCmp;
			while(BitsToRead)
			{
				BitReadVal = (BitsToRead >= 8) ? 8 : BitsToRead;
				BitsToRead -= BitReadVal;
				BitMask = (1 << BitReadVal) - 1;

				InShift += BitReadVal;
				InValB = (*InPos << InShift >> 8) & BitMask;
				if (InShift >= 8)
				{
					InShift -= 8;
					InPos ++;
					if (InShift)
						InValB |= (*InPos << InShift >> 8) & BitMask;
				}

				InVal |= InValB << OutBit;
				OutBit += BitReadVal;
			}

			switch(ValSize)
			{
			case 0x01:
				AddVal = Ent1B[InVal];
				OutVal += AddVal;
				OutVal &= OutMask;
				*((UINT8*)OutPos) = (UINT8)OutVal;
				break;
			case 0x02:
#ifndef VGM_BIG_ENDIAN
				AddVal = Ent2B[InVal];
#else
				AddVal = ReadLE16((UINT8*)&Ent2B[InVal]);
#endif
				OutVal += AddVal;
				OutVal &= OutMask;
#ifndef VGM_BIG_ENDIAN
				*((UINT16*)OutPos) = (UINT16)OutVal;
#else
				OutPos[0x00] = (UINT8)((OutVal & 0x00FF) >> 0);
				OutPos[0x01] = (UINT8)((OutVal & 0xFF00) >> 8);
#endif
				break;
			}
		}
		break;
	default:
		printf("Error: Unknown data block compression!\n");
		return false;
	}

	return true;
}

static UINT8 GetDACFromPCMBank(VGM_PLAYER* p)
{
	// for YM2612 DAC data only
	/*VGM_PCM_BANK* TempPCM;
	UINT32 CurBnk;*/
	UINT32 DataPos;

	/*TempPCM = &PCMBank[0x00];
	DataPos = TempPCM->DataPos;
	for (CurBnk = 0x00; CurBnk < TempPCM->BankCount; CurBnk ++)
	{
		if (DataPos < TempPCM->Bank[CurBnk].DataSize)
		{
			if (TempPCM->DataPos < TempPCM->DataSize)
				TempPCM->DataPos ++;
			return TempPCM->Bank[CurBnk].Data[DataPos];
		}
		DataPos -= TempPCM->Bank[CurBnk].DataSize;
	}
	return 0x80;*/

	DataPos = p->PCMBank[0x00].DataPos;
	if (DataPos >= p->PCMBank[0x00].DataSize)
		return 0x80;

	p->PCMBank[0x00].DataPos ++;
	return p->PCMBank[0x00].Data[DataPos];
}

static UINT8* GetPointerFromPCMBank(VGM_PLAYER* p, UINT8 Type, UINT32 DataPos)
{
	if (Type >= PCM_BANK_COUNT)
		return NULL;

	if (DataPos >= p->PCMBank[Type].DataSize)
		return NULL;

	return &p->PCMBank[Type].Data[DataPos];
}

static void ReadPCMTable(VGM_PLAYER* p, UINT32 DataSize, const UINT8* Data)
{
	UINT8 ValSize;
	UINT32 TblSize;

	p->PCMTbl.ComprType = Data[0x00];
	p->PCMTbl.CmpSubType = Data[0x01];
	p->PCMTbl.BitDec = Data[0x02];
	p->PCMTbl.BitCmp = Data[0x03];
	p->PCMTbl.EntryCount = ReadLE16(&Data[0x04]);

	ValSize = (p->PCMTbl.BitDec + 7) / 8;
	TblSize = p->PCMTbl.EntryCount * ValSize;

	p->PCMTbl.Entries = realloc(p->PCMTbl.Entries, TblSize);
	memcpy(p->PCMTbl.Entries, &Data[0x06], TblSize);

	if (DataSize < 0x06 + TblSize)
		printf("Warning! Bad PCM Table Length!\n");

	return;
}

#define CHIP_CHECK(name)	(p->ChipAudio[CurChip].name.ChipType != 0xFF)
static void InterpretVGM(VGM_PLAYER* p, UINT32 SampleCount)
{
	INT32 SmplPlayed;
	UINT8 Command;
	UINT8 TempByt;
	UINT16 TempSht;
	UINT32 TempLng;
	VGM_PCM_BANK* TempPCM;
	VGM_PCM_DATA* TempBnk;
	UINT32 ROMSize;
	UINT32 DataStart;
	UINT32 DataLen;
	const UINT8* ROMData;
	UINT8 CurChip;
	const UINT8* VGMPnt;

	if (p->VGMEnd)
		return;

	SmplPlayed = SamplePbk2VGM_I(p, p->VGMSmplPlayed + SampleCount);
	while(p->VGMSmplPos <= SmplPlayed)
	{
		Command = p->VGMData[p->VGMPos + 0x00];
		if (Command >= 0x70 && Command <= 0x8F)
		{
			switch(Command & 0xF0)
			{
			case 0x70:
				p->VGMSmplPos += (Command & 0x0F) + 0x01;
				break;
			case 0x80:
				TempByt = GetDACFromPCMBank(p);
				if (p->VGMHead.lngHzYM2612)
				{
					chip_reg_write(p, 0x02, 0x00, 0x00, 0x2A, TempByt);
				}
				p->VGMSmplPos += (Command & 0x0F);
				break;
			}
			p->VGMPos += 0x01;
		}
		else
		{
			VGMPnt = &p->VGMData[p->VGMPos];

			// Cheat Mode (to use 2 instances of 1 chip)
			CurChip = 0x00;
			switch(Command)
			{
			case 0x30:
				if (p->VGMHead.lngHzPSG & 0x40000000)
				{
					Command += 0x20;
					CurChip = 0x01;
				}
				break;
			case 0x3F:
				if (p->VGMHead.lngHzPSG & 0x40000000)
				{
					Command += 0x10;
					CurChip = 0x01;
				}
				break;
			case 0xA1:
				if (p->VGMHead.lngHzYM2413 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xA2:
			case 0xA3:
				if (p->VGMHead.lngHzYM2612 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xA4:
				if (p->VGMHead.lngHzYM2151 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xA5:
				if (p->VGMHead.lngHzYM2203 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xA6:
			case 0xA7:
				if (p->VGMHead.lngHzYM2608 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xA8:
			case 0xA9:
				if (p->VGMHead.lngHzYM2610 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xAA:
				if (p->VGMHead.lngHzYM3812 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xAB:
				if (p->VGMHead.lngHzYM3526 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xAC:
				if (p->VGMHead.lngHzY8950 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xAE:
			case 0xAF:
				if (p->VGMHead.lngHzYMF262 & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			case 0xAD:
				if (p->VGMHead.lngHzYMZ280B & 0x40000000)
				{
					Command -= 0x50;
					CurChip = 0x01;
				}
				break;
			}

			switch(Command)
			{
			case 0x66:	// End Of File
				if (p->VGMHead.lngLoopOffset)
				{
					p->VGMPos = p->VGMHead.lngLoopOffset;
					p->VGMSmplPos -= p->VGMHead.lngLoopSamples;
					p->VGMSmplPlayed -= SampleVGM2Pbk_I(p, p->VGMHead.lngLoopSamples);
					SmplPlayed = SamplePbk2VGM_I(p, p->VGMSmplPlayed + SampleCount);
					p->VGMCurLoop ++;

					if (p->VGMMaxLoopM && p->VGMCurLoop >= p->VGMMaxLoopM)
					{
						if (! p->FadePlay)
						{
							p->FadeStart = SampleVGM2Pbk_I(p, p->VGMHead.lngTotalSamples +
															(p->VGMCurLoop - 1) * p->VGMHead.lngLoopSamples);
						}
						p->FadePlay = true;
					}
					if (p->FadePlay && ! p->FadeTime)
						p->VGMEnd = true;
				}
				else
				{
					if (p->VGMHead.lngTotalSamples != (UINT32)p->VGMSmplPos)
					{
#ifdef CONSOLE_MODE
						printf("Warning! Header Samples: %u\t Counted Samples: %u\n",
								p->VGMHead.lngTotalSamples, p->VGMSmplPos);
						p->ErrorHappened = true;
#endif
						p->VGMHead.lngTotalSamples = p->VGMSmplPos;
					}
					
					if (p->HardStopOldVGMs)
					{
						if (p->VGMHead.lngVersion < 0x150 ||
							(p->VGMHead.lngVersion == 0x150 && p->HardStopOldVGMs == 0x02))
						Chips_GeneralActions(p, 0x01); // reset all chips, for instant silence
					}

					p->VGMEnd = true;
					break;
				}
				break;
			case 0x62:	// 1/60s delay
				p->VGMSmplPos += 735;
				p->VGMPos += 0x01;
				break;
			case 0x63:	// 1/50s delay
				p->VGMSmplPos += 882;
				p->VGMPos += 0x01;
				break;
			case 0x61:	// xx Sample Delay
				TempSht = ReadLE16(&VGMPnt[0x01]);
				p->VGMSmplPos += TempSht;
				p->VGMPos += 0x03;
				break;
			case 0x50:	// SN76496 write
				if (CHIP_CHECK(SN76496))
				{
					chip_reg_write(p, 0x00, CurChip, 0x00, 0x00, VGMPnt[0x01]);
				}
				p->VGMPos += 0x02;
				break;
			case 0x51:	// YM2413 write
				if (CHIP_CHECK(YM2413))
				{
					chip_reg_write(p, 0x01, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x52:	// YM2612 write port 0
			case 0x53:	// YM2612 write port 1
				if (CHIP_CHECK(YM2612))
				{
					chip_reg_write(p, 0x02, CurChip, Command & 0x01, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x67:	// PCM Data Stream
				TempByt = VGMPnt[0x02];
				TempLng = ReadLE32(&VGMPnt[0x03]);
				if (TempLng & 0x80000000)
				{
					TempLng &= 0x7FFFFFFF;
					CurChip = 0x01;
				}

				switch(TempByt & 0xC0)
				{
				case 0x00:	// Database Block
				case 0x40:
					AddPCMData(p, TempByt, TempLng, &VGMPnt[0x07]);
					/*switch(TempByt)
					{
					case 0x00:	// YM2612 PCM Database
						break;
					case 0x01:	// RF5C68 PCM Database
						break;
					case 0x02:	// RF5C164 PCM Database
						break;
					}*/
					break;
				case 0x80:	// ROM/RAM Dump
					if (p->VGMCurLoop)
						break;

					ROMSize = ReadLE32(&VGMPnt[0x07]);
					DataStart = ReadLE32(&VGMPnt[0x0B]);
					DataLen = TempLng - 0x08;
					ROMData = &VGMPnt[0x0F];
					switch(TempByt)
					{
					case 0x80:	// SegaPCM ROM
						if (! CHIP_CHECK(SegaPCM))
							break;
						sega_pcm_write_rom(p->segapcm[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x81:	// YM2608 DELTA-T ROM Image
						if (! CHIP_CHECK(YM2608))
							break;
						ym2608_write_data_pcmrom(p->ym2608[CurChip], 0x02, ROMSize, DataStart, DataLen,
												ROMData);
						break;
					case 0x82:	// YM2610 ADPCM ROM Image
					case 0x83:	// YM2610 DELTA-T ROM Image
						if (! CHIP_CHECK(YM2610))
							break;
						TempByt = 0x01 + (TempByt - 0x82);
						ym2610_write_data_pcmrom(p->ym2610[CurChip], TempByt, ROMSize, DataStart, DataLen,
												ROMData);
						break;
					case 0x84:	// YMF278B ROM Image
						if (! CHIP_CHECK(YMF278B))
							break;
						ymf278b_write_rom(p->ymf278b[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x85:	// YMF271 ROM Image
						if (! CHIP_CHECK(YMF271))
							break;
						ymf271_write_rom(p->ymf271[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x86:	// YMZ280B ROM Image
						if (! CHIP_CHECK(YMZ280B))
							break;
						ymz280b_write_rom(p->ymz280b[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x87:	// YMF278B RAM Image
						if (! CHIP_CHECK(YMF278B))
							break;
						//ymf278b_write_ram(CurChip, ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x88:	// Y8950 DELTA-T ROM Image
						if (! CHIP_CHECK(Y8950) || p->PlayingMode == 0x01)
							break;
						y8950_write_data_pcmrom(p->y8950[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x89:	// MultiPCM ROM Image
						if (! CHIP_CHECK(MultiPCM))
							break;
						multipcm_write_rom(p->multipcm[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x8A:	// UPD7759 ROM Image
						if (! CHIP_CHECK(UPD7759))
							break;
						upd7759_write_rom(p->upd7759[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x8B:	// OKIM6295 ROM Image
						if (! CHIP_CHECK(OKIM6295))
							break;
						okim6295_write_rom(p->okim6295[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x8C:	// K054539 ROM Image
						if (! CHIP_CHECK(K054539))
							break;
						k054539_write_rom(p->k054539[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x8D:	// C140 ROM Image
						if (! CHIP_CHECK(C140))
							break;
						c140_write_rom(p->c140[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x8E:	// K053260 ROM Image
						if (! CHIP_CHECK(K053260))
							break;
						k053260_write_rom(p->k053260[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x8F:	// QSound ROM Image
						if (! CHIP_CHECK(QSound))
							break;
						qsound_write_rom(p->qsound[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x90:	// ES5506 ROM Image
						if (! CHIP_CHECK(ES5506))
							break;
						es5506_write_rom(p->es550x[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x91:	// X1-010 ROM Image
						if (! CHIP_CHECK(X1_010))
							break;
						x1_010_write_rom(p->x1_010[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x92:	// C352 ROM Image
						if (! CHIP_CHECK(C352))
							break;
						c352_write_rom(p->c352[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
					case 0x93:	// GA20 ROM Image
						if (! CHIP_CHECK(GA20))
							break;
						iremga20_write_rom(p->ga20[CurChip], ROMSize, DataStart, DataLen, ROMData);
						break;
				//	case 0x8C:	// OKIM6376 ROM Image
				//		if (! CHIP_CHECK(OKIM6376))
				//			break;
				//		break;
					}
					break;
				case 0xC0:	// RAM Write
					if (! (TempByt & 0x20))
					{
						DataStart = ReadLE16(&VGMPnt[0x07]);
						DataLen = TempLng - 0x02;
						ROMData = &VGMPnt[0x09];
					}
					else
					{
						DataStart = ReadLE32(&VGMPnt[0x07]);
						DataLen = TempLng - 0x04;
						ROMData = &VGMPnt[0x0B];
					}
					switch(TempByt)
					{
					case 0xC0:	// RF5C68 RAM Database
						if (! CHIP_CHECK(RF5C68))
							break;
						rf5c68_write_ram(p->rf5c68, DataStart, DataLen, ROMData);
						break;
					case 0xC1:	// RF5C164 RAM Database
						if (! CHIP_CHECK(RF5C164))
							break;
						rf5c164_write_ram(p->rf5c164, DataStart, DataLen, ROMData);
						break;
					case 0xC2:	// NES APU RAM
						if (! CHIP_CHECK(NES))
							break;
						nes_write_ram(p->nesapu[CurChip], DataStart, DataLen, ROMData);
						break;
					case 0xE0:	// SCSP RAM
						if (! CHIP_CHECK(SCSP))
							break;
						scsp_write_ram(p->scsp[CurChip], DataStart, DataLen, ROMData);
						break;
					case 0xE1:	// ES5503 RAM
						if (! CHIP_CHECK(ES5503))
							break;
						es5503_write_ram(p->es5503[CurChip], DataStart, DataLen, ROMData);
						break;
					}
					break;
				}
				p->VGMPos += 0x07 + TempLng;
				break;
			case 0xE0:	// Seek to PCM Data Bank Pos
				p->PCMBank[0x00].DataPos = ReadLE32(&VGMPnt[0x01]);
				p->VGMPos += 0x05;
				break;
			case 0x4F:	// GG Stereo
				if (CHIP_CHECK(SN76496))
				{
					chip_reg_write(p, 0x00, CurChip, 0x01, 0x00, VGMPnt[0x01]);
				}
				p->VGMPos += 0x02;
				break;
			case 0x54:	// YM2151 write
				if (CHIP_CHECK(YM2151))
				{
					chip_reg_write(p, 0x03, CurChip, 0x01, VGMPnt[0x01], VGMPnt[0x02]);
				}
				//p->VGMSmplPos += 80;
				p->VGMPos += 0x03;
				break;
			case 0xC0:	// Sega PCM memory write
				TempSht = ReadLE16(&VGMPnt[0x01]);
				CurChip = (TempSht & 0x8000) >> 15;
				if (CHIP_CHECK(SegaPCM))
				{
					sega_pcm_w(p->segapcm[CurChip], TempSht & 0x7FFF, VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xB0:	// RF5C68 register write
				if (CHIP_CHECK(RF5C68))
				{
					chip_reg_write(p, 0x05, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xC1:	// RF5C68 memory write
				if (CHIP_CHECK(RF5C68))
				{
					TempSht = ReadLE16(&VGMPnt[0x01]);
					rf5c68_mem_w(p->rf5c68, TempSht, VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0x55:	// YM2203
				if (CHIP_CHECK(YM2203))
				{
					chip_reg_write(p, 0x06, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x56:	// YM2608 write port 0
			case 0x57:	// YM2608 write port 1
				if (CHIP_CHECK(YM2608))
				{
					chip_reg_write(p, 0x07, CurChip, Command & 0x01, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x58:	// YM2610 write port 0
			case 0x59:	// YM2610 write port 1
				if (CHIP_CHECK(YM2610))
				{
					chip_reg_write(p, 0x08, CurChip, Command & 0x01, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x5A:	// YM3812 write
				if (CHIP_CHECK(YM3812))
				{
					chip_reg_write(p, 0x09, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x5B:	// YM3526 write
				if (CHIP_CHECK(YM3526))
				{
					chip_reg_write(p, 0x0A, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x5C:	// Y8950 write
				if (CHIP_CHECK(Y8950))
				{
					chip_reg_write(p, 0x0B, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x5E:	// YMF262 write port 0
			case 0x5F:	// YMF262 write port 1
				if (CHIP_CHECK(YMF262))
				{
					chip_reg_write(p, 0x0C, CurChip, Command & 0x01, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x5D:	// YMZ280B write
				if (CHIP_CHECK(YMZ280B))
				{
					chip_reg_write(p, 0x0F, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xD0:	// YMF278B write
				if (CHIP_CHECK(YMF278B))
				{
					CurChip = (VGMPnt[0x01] & 0x80) >> 7;
					chip_reg_write(p, 0x0D, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02], VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xD1:	// YMF271 write
				if (CHIP_CHECK(YMF271))
				{
					CurChip = (VGMPnt[0x01] & 0x80) >> 7;
					chip_reg_write(p, 0x0E, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02], VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xB1:	// RF5C164 register write
				if (CHIP_CHECK(RF5C164))
				{
					chip_reg_write(p, 0x10, CurChip, 0x00, VGMPnt[0x01], VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xC2:	// RF5C164 memory write
				if (CHIP_CHECK(RF5C164))
				{
					TempSht = ReadLE16(&VGMPnt[0x01]);
					rf5c164_mem_w(p->rf5c164, TempSht, VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xB2:	// PWM channel write
				if (CHIP_CHECK(PWM))
				{
					chip_reg_write(p, 0x11, CurChip, (VGMPnt[0x01] & 0xF0) >> 4,
									VGMPnt[0x01] & 0x0F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x68:	// PCM RAM write
				CurChip = (VGMPnt[0x02] & 0x80) >> 7;
				TempByt =  VGMPnt[0x02] & 0x7F;

				DataStart = ReadLE24(&VGMPnt[0x03]);
				TempLng = ReadLE24(&VGMPnt[0x06]);
				DataLen = ReadLE24(&VGMPnt[0x09]);
				if (! DataLen)
					DataLen += 0x01000000;
				ROMData = GetPointerFromPCMBank(p, TempByt, DataStart);
				if (ROMData == NULL)
				{
					p->VGMPos += 0x0C;
					break;
				}

				switch(TempByt)
				{
				case 0x01:
					if (! CHIP_CHECK(RF5C68))
						break;
					rf5c68_write_ram(p->rf5c68, TempLng, DataLen, ROMData);
					break;
				case 0x02:
					if (! CHIP_CHECK(RF5C164))
						break;
					rf5c164_write_ram(p->rf5c164, TempLng, DataLen, ROMData);
					break;
				case 0x06:
					if (! CHIP_CHECK(SCSP))
						break;
					scsp_write_ram(p->scsp[CurChip], TempLng, DataLen, ROMData);
					break;
				case 0x07:
					if (! CHIP_CHECK(NES))
						break;
					p->Last95Drum = DataStart / DataLen - 1;
					p->Last95Max = p->PCMBank[TempByt].DataSize / DataLen;
					nes_write_ram(p->nesapu[CurChip], TempLng, DataLen, ROMData);
					break;
				}
				p->VGMPos += 0x0C;
				break;
			case 0xA0:	// AY8910 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(AY8910))
				{
					chip_reg_write(p, 0x12, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xB3:	// GameBoy DMG write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(GameBoy))
				{
					chip_reg_write(p, 0x13, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xB4:	// NES APU write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(NES))
				{
					chip_reg_write(p, 0x14, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xB5:	// MultiPCM write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(MultiPCM))
				{
					chip_reg_write(p, 0x15, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xC3:	// MultiPCM memory write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(MultiPCM))
				{
					TempSht = ReadLE16(&VGMPnt[0x02]);
					multipcm_bank_write(p->multipcm[CurChip], VGMPnt[0x01] & 0x7F, TempSht);
				}
				p->VGMPos += 0x04;
				break;
			case 0xB6:	// UPD7759 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(UPD7759))
				{
					chip_reg_write(p, 0x16, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xB7:	// OKIM6258 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(OKIM6258))
				{
					chip_reg_write(p, 0x17, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xB8:	// OKIM6295 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(OKIM6295))
				{
					chip_reg_write(p, 0x18, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xD2:	// SCC1 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(K051649))
				{
					chip_reg_write(p, 0x19, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02],
									VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xD3:	// K054539 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(K054539))
				{
					chip_reg_write(p, 0x1A, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02],
									VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xB9:	// HuC6280 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(HuC6280))
				{
					chip_reg_write(p, 0x1B, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xD4:	// C140 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(C140))
				{
					chip_reg_write(p, 0x1C, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02],
									VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xBA:	// K053260 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(K053260))
				{
					chip_reg_write(p, 0x1D, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xBB:	// Pokey write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(Pokey))
				{
					chip_reg_write(p, 0x1E, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xC4:	// QSound write
				if (CHIP_CHECK(QSound))
				{
					chip_reg_write(p, 0x1F, CurChip, VGMPnt[0x01], VGMPnt[0x02], VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xC5:	// YMF292/SCSP write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(SCSP))
				{
					chip_reg_write(p, 0x20, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02],
									VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xBC:	// WonderSwan write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(WSwan))
				{
					chip_reg_write(p, 0x21, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xC6:	// WonderSwan memory write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(WSwan))
				{
					TempSht = ReadBE16(&VGMPnt[0x01]) & 0x7FFF;
					ws_write_ram(p->wswan[CurChip], TempSht, VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xC7:	// VSU write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(VSU))
				{
					chip_reg_write(p, 0x22, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02],
									VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xBD:	// SAA1099 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(SAA1099))
				{
					chip_reg_write(p, 0x23, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xD5:	// ES5503 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(ES5503))
				{
					chip_reg_write(p, 0x24, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02],
									VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xBE:	// ES5506 write (8-bit data)
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(ES5506))
				{
					chip_reg_write(p, 0x25, CurChip, VGMPnt[0x01] & 0x7F, 0x00, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0xD6:	// ES5506 write (16-bit data)
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(ES5506))
				{
					chip_reg_write(p, 0x25, CurChip, 0x80 | (VGMPnt[0x01] & 0x7F),
									VGMPnt[0x02], VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
			case 0xC8:	// X1-010 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(X1_010))
				{
					chip_reg_write(p, 0x26, CurChip, VGMPnt[0x01] & 0x7F, VGMPnt[0x02],
									VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
#if 0	// for ctr's WIP rips
			case 0xC9:	// C352 write
				CurChip = 0x00;
				if (CHIP_CHECK(C352))
				{
					if (VGMPnt[0x01] == 0x03 && VGMPnt[0x02] == 0xFF && VGMPnt[0x03] == 0xFF)
						c352_w(p->c352[CurChip], 0x202, 0x0020);
					else
						chip_reg_write(p, 0x27, CurChip, VGMPnt[0x01], VGMPnt[0x02],
										VGMPnt[0x03]);
				}
				p->VGMPos += 0x04;
				break;
#endif
			case 0xE1:	// C352 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(C352))
				{
					TempSht = ((VGMPnt[0x01] & 0x7F) << 8) | (VGMPnt[0x02] << 0);
					c352_w(p->c352[CurChip], TempSht, (VGMPnt[0x03] << 8) | VGMPnt[0x04]);
				}
				p->VGMPos += 0x05;
				break;
			case 0xBF:	// GA20 write
				CurChip = (VGMPnt[0x01] & 0x80) >> 7;
				if (CHIP_CHECK(GA20))
				{
					chip_reg_write(p, 0x28, CurChip, 0x00, VGMPnt[0x01] & 0x7F, VGMPnt[0x02]);
				}
				p->VGMPos += 0x03;
				break;
			case 0x90:	// DAC Ctrl: Setup Chip
				CurChip = VGMPnt[0x01];
				if (CurChip == 0xFF)
				{
					p->VGMPos += 0x05;
					break;
				}
				if (! p->DacCtrl[CurChip].Enable)
				{
					device_start_daccontrol(&p->daccontrol[CurChip], p, p->SampleRate);
					device_reset_daccontrol(p->daccontrol[CurChip]);
					p->DacCtrl[CurChip].Enable = true;
					p->DacCtrlUsg[p->DacCtrlUsed] = CurChip;
					p->DacCtrlUsed ++;
				}
				TempByt = VGMPnt[0x02];	// Chip Type
				TempSht = ReadBE16(&VGMPnt[0x03]);
				daccontrol_setup_chip(p->daccontrol[CurChip], TempByt & 0x7F, (TempByt & 0x80) >> 7, TempSht);
				p->VGMPos += 0x05;
				break;
			case 0x91:	// DAC Ctrl: Set Data
				CurChip = VGMPnt[0x01];
				if (CurChip == 0xFF || ! p->DacCtrl[CurChip].Enable)
				{
					p->VGMPos += 0x05;
					break;
				}
				p->DacCtrl[CurChip].Bank = VGMPnt[0x02];
				if (p->DacCtrl[CurChip].Bank >= PCM_BANK_COUNT)
					p->DacCtrl[CurChip].Bank = 0x00;

				TempPCM = &p->PCMBank[p->DacCtrl[CurChip].Bank];
				p->Last95Max = TempPCM->BankCount;
				daccontrol_set_data(p->daccontrol[CurChip], TempPCM->Data, TempPCM->DataSize,
									VGMPnt[0x03], VGMPnt[0x04]);
				p->VGMPos += 0x05;
				break;
			case 0x92:	// DAC Ctrl: Set Freq
				CurChip = VGMPnt[0x01];
				if (CurChip == 0xFF || ! p->DacCtrl[CurChip].Enable)
				{
					p->VGMPos += 0x06;
					break;
				}
				TempLng = ReadLE32(&VGMPnt[0x02]);
				p->Last95Freq = TempLng;
				daccontrol_set_frequency(p->daccontrol[CurChip], TempLng);
				p->VGMPos += 0x06;
				break;
			case 0x93:	// DAC Ctrl: Play from Start Pos
				CurChip = VGMPnt[0x01];
				if (CurChip == 0xFF || ! p->DacCtrl[CurChip].Enable ||
					! p->PCMBank[p->DacCtrl[CurChip].Bank].BankCount)
				{
					p->VGMPos += 0x0B;
					break;
				}
				DataStart = ReadLE32(&VGMPnt[0x02]);
				p->Last95Drum = 0xFFFF;
				TempByt = VGMPnt[0x06];
				DataLen = ReadLE32(&VGMPnt[0x07]);
				daccontrol_start(p->daccontrol[CurChip], DataStart, TempByt, DataLen);
				p->VGMPos += 0x0B;
				break;
			case 0x94:	// DAC Ctrl: Stop immediately
				CurChip = VGMPnt[0x01];
				if (! p->DacCtrl[CurChip].Enable)
				{
					p->VGMPos += 0x02;
					break;
				}
				p->Last95Drum = 0xFFFF;
				if (CurChip < 0xFF)
				{
					daccontrol_stop(p->daccontrol[CurChip]);
				}
				else
				{
					for (CurChip = 0x00; CurChip < 0xFF; CurChip ++)
						daccontrol_stop(p->daccontrol[CurChip]);
				}
				p->VGMPos += 0x02;
				break;
			case 0x95:	// DAC Ctrl: Play Block (small)
				CurChip = VGMPnt[0x01];
				if (CurChip == 0xFF || ! p->DacCtrl[CurChip].Enable ||
					! p->PCMBank[p->DacCtrl[CurChip].Bank].BankCount)
				{
					p->VGMPos += 0x05;
					break;
				}
				TempPCM = &p->PCMBank[p->DacCtrl[CurChip].Bank];
				TempSht = ReadLE16(&VGMPnt[0x02]);
				p->Last95Drum = TempSht;
				p->Last95Max = TempPCM->BankCount;
				if (TempSht >= TempPCM->BankCount)
					TempSht = 0x00;
				TempBnk = &TempPCM->Bank[TempSht];

				TempByt = DCTRL_LMODE_BYTES |
							(VGMPnt[0x04] & 0x10) |			// Reverse Mode
							((VGMPnt[0x04] & 0x01) << 7);	// Looping
				daccontrol_start(p->daccontrol[CurChip], TempBnk->DataStart, TempByt, TempBnk->DataSize);
				p->VGMPos += 0x05;
				break;
			default:

				switch(Command & 0xF0)
				{
				case 0x00:
				case 0x10:
				case 0x20:
					p->VGMPos += 0x01;
					break;
				case 0x30:
					p->VGMPos += 0x02;
					break;
				case 0x40:
				case 0x50:
				case 0xA0:
				case 0xB0:
					p->VGMPos += 0x03;
					break;
				case 0xC0:
				case 0xD0:
					p->VGMPos += 0x04;
					break;
				case 0xE0:
				case 0xF0:
					p->VGMPos += 0x05;
					break;
				default:
					p->VGMEnd = true;
					p->EndPlay = true;
					break;
				}
				break;
			}
		}

		if (p->VGMPos >= p->VGMHead.lngEOFOffset)
			p->VGMEnd = true;

		if (p->VGMEnd)
			break;
	}

	return;
}


static void GeneralChipLists(VGM_PLAYER* p)
{
	// Generate Chip List for playback loop
	UINT16 CurBufIdx;
	CA_LIST* CLstOld;
	CA_LIST* CLst;
	/*CA_LIST* CurLst;*/
	UINT8 CurChip;
	UINT8 CurCSet;
	CAUD_ATTR* CAA;

	p->ChipListAll = NULL;
	//ChipListPause = NULL;
	//ChipListOpt = NULL;

	// generate list of all chips that are used in the current VGM
	CurBufIdx = 0x00;
	CLstOld = NULL;
	for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++)
	{
		for (CurCSet = 0x00; CurCSet < 0x02; CurCSet ++)
		{
			CAA = (CAUD_ATTR*)&p->ChipAudio[CurCSet] + CurChip;
			if (CAA->ChipType != 0xFF)
			{
				CLst = &p->ChipListBuffer[CurBufIdx];
				CurBufIdx ++;
				if (CLstOld == NULL)
					p->ChipListAll = CLst;
				else
					CLstOld->next = CLst;

				CLst->CAud = CAA;
				CLst->COpts = (CHIP_OPTS*)&p->ChipOpts[CurCSet] + CurChip;
				CLstOld = CLst;
			}
		}
	}
	if (CLstOld != NULL)
		CLstOld->next = NULL;

	/*// Go through the chip list and copy all chips to a new list, except for a few
	// selected ones.
	CLstOld = NULL;
	CurLst = ChipListAll;
	while(CurLst != NULL)
	{
		// don't emulate the RF5Cxx chips when paused+emulated
		if (CurLst->CAud->ChipType != 0x05 && CurLst->CAud->ChipType != 0x10)
		{
			CLst = &p->ChipListBuffer[CurBufIdx];
			CurBufIdx ++;
			if (CLstOld == NULL)
				p->ChipListPause = CLst;
			else
				CLstOld->next = CLst;

			*CLst = *CurLst;
			CLstOld = CLst;
		}
		CurLst = CurLst->next;
	}
	if (CLstOld != NULL)
		CLstOld->next = NULL;*/

	return;
}

static void SetupResampler(VGM_PLAYER* p, CAUD_ATTR* CAA)
{
	if (! CAA->SmpRate)
	{
		CAA->Resampler = 0x00;
		return;
	}

    CAA->TargetSmpRate = p->SampleRate;

		CAA->Resampler = resampler_create();

	return;
}

static void ChangeChipSampleRate(void* DataPtr, UINT32 NewSmplRate)
{
	CAUD_ATTR* CAA = (CAUD_ATTR*)DataPtr;

	if (CAA->SmpRate == NewSmplRate)
		return;

	CAA->SmpRate = NewSmplRate;

	return;
}


INLINE INT16 Limit2Short(INT32 Value)
{
	INT32 NewValue;

	NewValue = Value;
	if (NewValue < -0x8000)
		NewValue = -0x8000;
	else if (NewValue > 0x7FFF)
		NewValue = 0x7FFF;

	return (INT16)NewValue;
}

INLINE INT32 LimitScaleAdd(INT32 Target, INT32 Value, UINT16 Scale)
{
	INT64 NewValue;

	NewValue = (INT64)Value;
	NewValue *= (INT64)Scale;
	NewValue += (INT64)Target;

	if (NewValue < -0x80000000LL)
		NewValue = -0x80000000LL;
	else if (NewValue > 0x7FFFFFFFLL)
		NewValue = 0x7FFFFFFFLL;

	return (INT32)NewValue;
}

static void null_update(void *param, stream_sample_t **outputs, int samples)
{
	memset(outputs[0x00], 0x00, sizeof(stream_sample_t) * samples);
	memset(outputs[0x01], 0x00, sizeof(stream_sample_t) * samples);

	return;
}

static void dual_opl2_stereo(void *param, stream_sample_t **outputs, int samples)
{
    struct dual_opl2_info * info = (struct dual_opl2_info *) param;

	ym3812_stream_update(info->chip, outputs, samples);

	// Dual-OPL with Stereo
	if (info->ChipID & 0x01)
		memset(outputs[0x00], 0x00, sizeof(stream_sample_t) * samples);	// Mute Left Chanel
	else
		memset(outputs[0x01], 0x00, sizeof(stream_sample_t) * samples);	// Mute Right Chanel

	return;
}

static void ResampleChipStream(VGM_PLAYER* p, CA_LIST* CLst, WAVE_32BS* RetSample, UINT32 Length)
{
	CAUD_ATTR* CAA;
	INT32* CurBufL;
	INT32* CurBufR;
	INT32 SmpCnt;	// must be signed, else I'm getting calculation errors
	INT32 CurSmpl;
  UINT32 SampleRate;
	UINT32 OutPos;
	sample_t ls, rs;

	CAA = CLst->CAud;
	if (!CAA->Resampler)
		return;
    
	CurBufL = p->StreamBufs[0x00];
	CurBufR = p->StreamBufs[0x01];

  SampleRate = p->SampleRate;

	OutPos = 0;

	// This Do-While-Loop gets and resamples the chip output of one or more chips.
	// It's a loop to support the AY8910 paired with the YM2203/YM2608/YM2610.
	do
	{
		for (OutPos = 0; OutPos < Length; OutPos++)
		{
			if (CAA->LastSmpRate != CAA->SmpRate)
			{
				resampler_set_rate(CAA->Resampler, (double)CAA->SmpRate / (double)CAA->TargetSmpRate);
				CAA->LastSmpRate = CAA->SmpRate;
			}

			SmpCnt = resampler_get_min_fill(CAA->Resampler) / 2;

			if (SmpCnt)
			{
				CAA->StreamUpdate(CAA->StreamUpdateParam, p->StreamBufs, SmpCnt);
				for (CurSmpl = 0; CurSmpl < SmpCnt; CurSmpl++)
					resampler_write_pair(CAA->Resampler, CurBufL[CurSmpl], CurBufR[CurSmpl]);
			}

			resampler_read_pair(CAA->Resampler, &ls, &rs);

			RetSample[OutPos].Left = LimitScaleAdd(RetSample[OutPos].Left, ls, CAA->Volume);
			RetSample[OutPos].Right = LimitScaleAdd(RetSample[OutPos].Right, rs, CAA->Volume);
		}

		CAA = CAA->Paired;
	} while(CAA != NULL);

	return;
}

static INT32 RecalcFadeVolume(VGM_PLAYER* p)
{
	float TempSng;

	if (p->FadePlay)
	{
		if (! p->FadeStart)
			p->FadeStart = p->PlayingTime;

		TempSng = (p->PlayingTime - p->FadeStart) / (float)p->SampleRate;
		p->MasterVol = 1.0f - TempSng / (p->FadeTime * 0.001f);
		if (p->MasterVol < 0.0f)
		{
			p->MasterVol = 0.0f;
			//EndPlay = true;
			p->VGMEnd = true;
		}
		p->FinalVol = p->VolumeLevelM * p->MasterVol * p->MasterVol;
	}

	return (INT32)(0x100 * p->FinalVol + 0.5f);
}

UINT32 FillBuffer(void *_p, WAVE_16BS* Buffer, UINT32 BufferSize)
{
	UINT32 CurSmpl;
	WAVE_32BS TempBuf;
	INT32 CurMstVol;
	UINT32 RecalcStep;
	CA_LIST* CurCLst;

    VGM_PLAYER* p = (VGM_PLAYER *)_p;

	//memset(Buffer, 0x00, sizeof(WAVE_16BS) * BufferSize);

	RecalcStep = p->FadePlay ? p->SampleRate / 44100 : 0;
	CurMstVol = RecalcFadeVolume(p);

	if (Buffer == NULL)
	{
		//for (CurSmpl = 0x00; CurSmpl < BufferSize; CurSmpl ++)
		//	InterpretFile(1);
		InterpretFile(p, BufferSize);

		if (p->FadePlay && ! p->FadeStart)
		{
			p->FadeStart = p->PlayingTime;
			RecalcStep = p->FadePlay ? p->SampleRate / 100 : 0;
		}
		//if (RecalcStep && ! (CurSmpl % RecalcStep))
		if (RecalcStep)
			CurMstVol = RecalcFadeVolume(p);

        if (p->VGMEnd)
        {
            p->EndPlay = true;
        }

		return BufferSize;
	}

	for (CurSmpl = 0x00; CurSmpl < BufferSize; CurSmpl ++)
	{
		InterpretFile(p, 1);

		// Sample Structures
		//	00 - SN76496
		//	01 - YM2413
		//	02 - YM2612
		//	03 - YM2151
		//	04 - SegaPCM
		//	05 - RF5C68
		//	06 - YM2203
		//	07 - YM2608
		//	08 - YM2610/YM2610B
		//	09 - YM3812
		//	0A - YM3526
		//	0B - Y8950
		//	0C - YMF262
		//	0D - YMF278B
		//	0E - YMF271
		//	0F - YMZ280B
		//	10 - RF5C164
		//	11 - PWM
		//	12 - AY8910
		//	13 - GameBoy
		//	14 - NES APU
		//	15 - MultiPCM
		//	16 - UPD7759
		//	17 - OKIM6258
		//	18 - OKIM6295
		//	19 - K051649
		//	1A - K054539
		//	1B - HuC6280
		//	1C - C140
		//	1D - K053260
		//	1E - Pokey
		//	1F - QSound
		//	20 - YMF292/SCSP
		//	21 - WonderSwan
		//	22 - VSU
		//	23 - SAA1099
		//	24 - ES5503
		//	25 - ES5506
		//	26 - X1-010
		//	27 - C352
		//	28 - GA20
		TempBuf.Left = 0x00;
		TempBuf.Right = 0x00;
		CurCLst = p->ChipListAll;
		while(CurCLst != NULL)
		{
			if (! CurCLst->COpts->Disabled)
			{
				ResampleChipStream(p, CurCLst, &TempBuf, 1);
			}
			CurCLst = CurCLst->next;
		}

		// ChipData << 9 [ChipVol] >> 5 << 8 [MstVol] >> 11  ->  9-5+8-11 = <<1
		TempBuf.Left = ((TempBuf.Left >> 5) * CurMstVol) >> 11;
		TempBuf.Right = ((TempBuf.Right >> 5) * CurMstVol) >> 11;
		if (p->SurroundSound)
			TempBuf.Right *= -1;
		Buffer[CurSmpl].Left = Limit2Short(TempBuf.Left);
		Buffer[CurSmpl].Right = Limit2Short(TempBuf.Right);

		if (p->FadePlay && ! p->FadeStart)
		{
			p->FadeStart = p->PlayingTime;
			RecalcStep = p->FadePlay ? p->SampleRate / 100 : 0;
		}
		if (RecalcStep && ! (CurSmpl % RecalcStep))
			CurMstVol = RecalcFadeVolume(p);

		if (p->VGMEnd)
		{
            if (! p->EndPlay)
            {
                p->EndPlay = true;
                break;
            }
        }
	}

	return CurSmpl;
}

// ChanCount is an array of 3, for the 3 mute masks per chip
static void GetChipByChannel(void* vgmp, UINT32 channel, UINT8 *ChipID, UINT8 *ChipType, UINT8 *Channel, UINT8 *ChanCount)
{
    VGM_PLAYER* p = (VGM_PLAYER *)vgmp;
    
    *ChipType = 0xFF;

    if (p->VGMHead.lngHzPSG)
    {
        if (channel < 4)
        {
            *ChipID = 0x00;
            *ChipType = 0x00;
            *Channel = channel;
            ChanCount[0] = 4;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 4;
        
        if (p->VGMHead.lngHzPSG & 0x40000000)
        {
            if (channel < 4)
            {
                *ChipID = 0x01;
                *ChipType = 0x00;
                *Channel = channel;
                ChanCount[0] = 4;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 4;
        }
    }
    
    if (p->VGMHead.lngHzYM2413)
    {
        if (channel < 14)
        {
            *ChipID = 0x00;
            *ChipType = 0x01;
            *Channel = channel;
            ChanCount[0] = 14;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 14;
        
        if (p->VGMHead.lngHzYM2413 & 0x40000000)
        {
            if (channel < 14)
            {
                *ChipID = 0x01;
                *ChipType = 0x01;
                *Channel = channel;
                ChanCount[0] = 14;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 14;
        }
    }
    
    if (p->VGMHead.lngHzYM2612)
    {
        if (channel < 7)
        {
            *ChipID = 0x00;
            *ChipType = 0x02;
            *Channel = channel;
            ChanCount[0] = 7;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 7;
        
        if (p->VGMHead.lngHzYM2612 & 0x40000000)
        {
            if (channel < 7)
            {
                *ChipID = 0x01;
                *ChipType = 0x02;
                *Channel = channel;
                ChanCount[0] = 7;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 7;
        }
    }

    if (p->VGMHead.lngHzYM2151)
    {
        if (channel < 8)
        {
            *ChipID = 0x00;
            *ChipType = 0x03;
            *Channel = channel;
            ChanCount[0] = 8;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 8;
        
        if (p->VGMHead.lngHzYM2151 & 0x40000000)
        {
            if (channel < 8)
            {
                *ChipID = 0x01;
                *ChipType = 0x03;
                *Channel = channel;
                ChanCount[0] = 8;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 8;
        }
    }
    
    if (p->VGMHead.lngHzSPCM)
    {
        if (channel < 16)
        {
            *ChipID = 0x00;
            *ChipType = 0x04;
            *Channel = channel;
            ChanCount[0] = 16;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 16;
        
        if (p->VGMHead.lngHzSPCM & 0x40000000)
        {
            if (channel < 16)
            {
                *ChipID = 0x01;
                *ChipType = 0x04;
                *Channel = channel;
                ChanCount[0] = 16;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 16;
        }
    }

    if (p->VGMHead.lngHzRF5C68)
    {
        if (channel < 8)
        {
            *ChipID = 0x00;
            *ChipType = 0x05;
            *Channel = channel;
            ChanCount[0] = 8;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 8;
    }

    if (p->VGMHead.lngHzYM2203)
    {
        if (channel < 6)
        {
            *ChipID = 0x00;
            *ChipType = 0x06;
            *Channel = channel;
            ChanCount[0] = 3;
            ChanCount[1] = 0;
            ChanCount[2] = 3;
        }
        
        channel -= 6;
        
        if (p->VGMHead.lngHzYM2203 & 0x40000000)
        {
            if (channel < 6)
            {
                *ChipID = 0x01;
                *ChipType = 0x06;
                *Channel = channel;
                ChanCount[0] = 3;
                ChanCount[1] = 0;
                ChanCount[2] = 3;
            }
            
            channel -= 6;
        }
    }

    if (p->VGMHead.lngHzYM2608)
    {
        if (channel < 16)
        {
            *ChipID = 0x00;
            *ChipType = 0x07;
            *Channel = channel;
            ChanCount[0] = 6;
            ChanCount[1] = 7;
            ChanCount[2] = 3;
        }
        
        channel -= 16;
        
        if (p->VGMHead.lngHzYM2608 & 0x40000000)
        {
            if (channel < 16)
            {
                *ChipID = 0x01;
                *ChipType = 0x07;
                *Channel = channel;
                ChanCount[0] = 6;
                ChanCount[1] = 7;
                ChanCount[2] = 3;
            }
            
            channel -= 16;
        }
    }
    
    if (p->VGMHead.lngHzYM2610)
    {
        if (channel < 16)
        {
            *ChipID = 0x00;
            *ChipType = 0x08;
            *Channel = channel;
            ChanCount[0] = 6;
            ChanCount[1] = 7;
            ChanCount[2] = 3;
        }
        
        channel -= 16;
        
        if (p->VGMHead.lngHzYM2610 & 0x40000000)
        {
            if (channel < 16)
            {
                *ChipID = 0x01;
                *ChipType = 0x08;
                *Channel = channel;
                ChanCount[0] = 6;
                ChanCount[1] = 7;
                ChanCount[2] = 3;
            }
            
            channel -= 16;
        }
    }
    
    if (p->VGMHead.lngHzYM3812)
    {
        if (channel < 14)
        {
            *ChipID = 0x00;
            *ChipType = 0x09;
            *Channel = channel;
            ChanCount[0] = 14;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 14;
        
        if (p->VGMHead.lngHzYM3812 & 0x40000000)
        {
            if (channel < 14)
            {
                *ChipID = 0x01;
                *ChipType = 0x09;
                *Channel = channel;
                ChanCount[0] = 14;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 14;
        }
    }
    
    if (p->VGMHead.lngHzYM3526)
    {
        if (channel < 15)
        {
            *ChipID = 0x00;
            *ChipType = 0x0A;
            *Channel = channel;
            ChanCount[0] = 15;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 15;
        
        if (p->VGMHead.lngHzYM3526 & 0x40000000)
        {
            if (channel < 15)
            {
                *ChipID = 0x01;
                *ChipType = 0x0B;
                *Channel = channel;
                ChanCount[0] = 15;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 15;
        }
    }
    
    if (p->VGMHead.lngHzY8950)
    {
        if (channel < 15)
        {
            *ChipID = 0x00;
            *ChipType = 0x0B;
            *Channel = channel;
            ChanCount[0] = 15;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 15;
        
        if (p->VGMHead.lngHzY8950 & 0x40000000)
        {
            if (channel < 15)
            {
                *ChipID = 0x01;
                *ChipType = 0x0B;
                *Channel = channel;
                ChanCount[0] = 15;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 15;
        }
    }
    
    if (p->VGMHead.lngHzYMF262)
    {
        if (channel < 23)
        {
            *ChipID = 0x00;
            *ChipType = 0x0C;
            *Channel = channel;
            ChanCount[0] = 23;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 23;
        
        if (p->VGMHead.lngHzYMF262 & 0x40000000)
        {
            if (channel < 23)
            {
                *ChipID = 0x00;
                *ChipType = 0x0C;
                *Channel = channel;
                ChanCount[0] = 23;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 23;
        }
    }
    
    if (p->VGMHead.lngHzYMF278B)
    {
        if (channel < 47)
        {
            *ChipID = 0x00;
            *ChipType = 0x0D;
            *Channel = channel;
            ChanCount[0] = 23;
            ChanCount[1] = 24;
            ChanCount[2] = 0;
        }
        
        channel -= 47;
        
        if (p->VGMHead.lngHzYMF278B & 0x40000000)
        {
            if (channel < 47)
            {
                *ChipID = 0x01;
                *ChipType = 0x0D;
                *Channel = channel;
                ChanCount[0] = 23;
                ChanCount[1] = 24;
                ChanCount[2] = 0;
            }
            
            channel -= 47;
        }
    }
    
    if (p->VGMHead.lngHzYMF271)
    {
        if (channel < 12)
        {
            *ChipID = 0x00;
            *ChipType = 0x0E;
            *Channel = channel;
            ChanCount[0] = 12;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 12;
        
        if (p->VGMHead.lngHzYMF271 & 0x40000000)
        {
            if (channel < 12)
            {
                *ChipID = 0x01;
                *ChipType = 0x0E;
                *Channel = channel;
                ChanCount[0] = 12;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 12;
        }
    }
    
    if (p->VGMHead.lngHzYMZ280B)
    {
        if (channel < 8)
        {
            *ChipID = 0x00;
            *ChipType = 0x0F;
            *Channel = channel;
            ChanCount[0] = 8;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 8;
        
        if (p->VGMHead.lngHzYMZ280B & 0x40000000)
        {
            if (channel < 8)
            {
                *ChipID = 0x01;
                *ChipType = 0x0F;
                *Channel = channel;
                ChanCount[0] = 8;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 8;
        }
    }
    
    if (p->VGMHead.lngHzRF5C164)
    {
        if (channel < 8)
        {
            *ChipID = 0x00;
            *ChipType = 0x10;
            *Channel = channel;
            ChanCount[0] = 8;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 8;
    }
    
    if (p->VGMHead.lngHzPWM)
    {
        if (channel < 1)
        {
            *ChipID = 0x00;
            *ChipType = 0x11;
            *Channel = channel;
            ChanCount[0] = 1;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel--;
    }

    if (p->VGMHead.lngHzAY8910)
    {
        if (channel < 3)
        {
            *ChipID = 0x00;
            *ChipType = 0x12;
            *Channel = channel;
            ChanCount[0] = 3;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 3;
        
        if (p->VGMHead.lngHzAY8910 & 0x40000000)
        {
            if (channel < 3)
            {
                *ChipID = 0x01;
                *ChipType = 0x12;
                *Channel = channel;
                ChanCount[0] = 3;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 3;
        }
    }

    if (p->VGMHead.lngHzGBDMG)
    {
        if (channel < 4)
        {
            *ChipID = 0x00;
            *ChipType = 0x13;
            *Channel = channel;
            ChanCount[0] = 4;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 4;
        
        if (p->VGMHead.lngHzGBDMG & 0x40000000)
        {
            if (channel < 4)
            {
                *ChipID = 0x01;
                *ChipType = 0x13;
                *Channel = channel;
                ChanCount[0] = 4;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 4;
        }
    }
    
    if (p->VGMHead.lngHzNESAPU)
    {
        if (channel < 6)
        {
            *ChipID = 0x00;
            *ChipType = 0x14;
            *Channel = channel;
            ChanCount[0] = 6;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 6;
        
        if (p->VGMHead.lngHzNESAPU & 0x40000000)
        {
            if (channel < 6)
            {
                *ChipID = 0x01;
                *ChipType = 0x14;
                *Channel = channel;
                ChanCount[0] = 6;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 6;
        }
    }

    if (p->VGMHead.lngHzMultiPCM)
    {
        if (channel < 28)
        {
            *ChipID = 0x00;
            *ChipType = 0x15;
            *Channel = channel;
            ChanCount[0] = 28;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 28;
        
        if (p->VGMHead.lngHzMultiPCM & 0x40000000)
        {
            if (channel < 28)
            {
                *ChipID = 0x01;
                *ChipType = 0x15;
                *Channel = channel;
                ChanCount[0] = 28;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 28;
        }
    }
    
    if (p->VGMHead.lngHzUPD7759)
    {
        if (channel < 1)
        {
            *ChipID = 0x00;
            *ChipType = 0x16;
            *Channel = channel;
            ChanCount[0] = 1;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel--;
        
        if (p->VGMHead.lngHzUPD7759 & 0x40000000)
        {
            if (channel < 1)
            {
                *ChipID = 0x01;
                *ChipType = 0x16;
                *Channel = channel;
                ChanCount[0] = 1;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel--;
        }
    }
    
    if (p->VGMHead.lngHzOKIM6258)
    {
        if (channel < 1)
        {
            *ChipID = 0x00;
            *ChipType = 0x17;
            *Channel = channel;
            ChanCount[0] = 1;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel--;
        
        if (p->VGMHead.lngHzOKIM6258 & 0x40000000)
        {
            if (channel < 1)
            {
                *ChipID = 0x01;
                *ChipType = 0x17;
                *Channel = channel;
                ChanCount[0] = 1;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel--;
        }
    }
    
    if (p->VGMHead.lngHzOKIM6295)
    {
        if (channel < 4)
        {
            *ChipID = 0x00;
            *ChipType = 0x18;
            *Channel = channel;
            ChanCount[0] = 4;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 4;
        
        if (p->VGMHead.lngHzOKIM6295 & 0x40000000)
        {
            if (channel < 4)
            {
                *ChipID = 0x01;
                *ChipType = 0x18;
                *Channel = channel;
                ChanCount[0] = 4;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 4;
        }
    }
    
    if (p->VGMHead.lngHzK051649)
    {
        if (channel < 5)
        {
            *ChipID = 0x00;
            *ChipType = 0x19;
            *Channel = channel;
            ChanCount[0] = 5;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 5;
        
        if (p->VGMHead.lngHzK051649 & 0x40000000)
        {
            if (channel < 5)
            {
                *ChipID = 0x01;
                *ChipType = 0x19;
                *Channel = channel;
                ChanCount[0] = 5;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 5;
        }
    }

    if (p->VGMHead.lngHzK054539)
    {
        if (channel < 8)
        {
            *ChipID = 0x00;
            *ChipType = 0x1A;
            *Channel = channel;
            ChanCount[0] = 8;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 8;
        
        if (p->VGMHead.lngHzK054539 & 0x40000000)
        {
            if (channel < 8)
            {
                *ChipID = 0x01;
                *ChipType = 0x1A;
                *Channel = channel;
                ChanCount[0] = 8;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 8;
        }
    }
    
    if (p->VGMHead.lngHzHuC6280)
    {
        if (channel < 6)
        {
            *ChipID = 0x00;
            *ChipType = 0x1B;
            *Channel = channel;
            ChanCount[0] = 6;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 6;
        
        if (p->VGMHead.lngHzHuC6280 & 0x40000000)
        {
            if (channel < 6)
            {
                *ChipID = 0x01;
                *ChipType = 0x1B;
                *Channel = channel;
                ChanCount[0] = 6;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 6;
        }
    }
    
    if (p->VGMHead.lngHzC140)
    {
        if (channel < 24)
        {
            *ChipID = 0x00;
            *ChipType = 0x1C;
            *Channel = channel;
            ChanCount[0] = 24;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 24;
        
        if (p->VGMHead.lngHzC140 & 0x40000000)
        {
            if (channel < 24)
            {
                *ChipID = 0x01;
                *ChipType = 0x1C;
                *Channel = channel;
                ChanCount[0] = 24;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 24;
        }
    }

    if (p->VGMHead.lngHzK053260)
    {
        if (channel < 4)
        {
            *ChipID = 0x00;
            *ChipType = 0x1D;
            *Channel = channel;
            ChanCount[0] = 4;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 4;
        
        if (p->VGMHead.lngHzK053260 & 0x40000000)
        {
            if (channel < 4)
            {
                *ChipID = 0x01;
                *ChipType = 0x1D;
                *Channel = channel;
                ChanCount[0] = 4;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 4;
        }
    }
    
    if (p->VGMHead.lngHzPokey)
    {
        if (channel < 4)
        {
            *ChipID = 0x00;
            *ChipType = 0x1E;
            *Channel = channel;
            ChanCount[0] = 4;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 4;
        
        if (p->VGMHead.lngHzPokey & 0x40000000)
        {
            if (channel < 4)
            {
                *ChipID = 0x01;
                *ChipType = 0x1E;
                *Channel = channel;
                ChanCount[0] = 4;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 4;
        }
    }
    
    if (p->VGMHead.lngHzQSound)
    {
        if (channel < 16)
        {
            *ChipID = 0x00;
            *ChipType = 0x1F;
            *Channel = channel;
            ChanCount[0] = 16;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 16;
        
        if (p->VGMHead.lngHzQSound & 0x40000000)
        {
            if (channel < 16)
            {
                *ChipID = 0x01;
                *ChipType = 0x1F;
                *Channel = channel;
                ChanCount[0] = 16;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 16;
        }
    }
    
    if (p->VGMHead.lngHzSCSP)
    {
        if (channel < 32)
        {
            *ChipID = 0x00;
            *ChipType = 0x20;
            *Channel = channel;
            ChanCount[0] = 32;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 32;
        
        if (p->VGMHead.lngHzSCSP & 0x40000000)
        {
            if (channel < 32)
            {
                *ChipID = 0x01;
                *ChipType = 0x20;
                *Channel = channel;
                ChanCount[0] = 32;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 32;
        }
    }
    
    if (p->VGMHead.lngHzWSwan)
    {
        if (channel < 4)
        {
            *ChipID = 0x00;
            *ChipType = 0x21;
            *Channel = channel;
            ChanCount[0] = 4;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 4;
        
        if (p->VGMHead.lngHzWSwan & 0x40000000)
        {
            if (channel < 4)
            {
                *ChipID = 0x01;
                *ChipType = 0x21;
                *Channel = channel;
                ChanCount[0] = 4;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 4;
        }
    }
    
    if (p->VGMHead.lngHzVSU)
    {
        if (channel < 6)
        {
            *ChipID = 0x00;
            *ChipType = 0x22;
            *Channel = channel;
            ChanCount[0] = 6;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 6;
        
        if (p->VGMHead.lngHzVSU & 0x40000000)
        {
            if (channel < 6)
            {
                *ChipID = 0x01;
                *ChipType = 0x22;
                *Channel = channel;
                ChanCount[0] = 6;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 6;
        }
    }
    
    if (p->VGMHead.lngHzSAA1099)
    {
        if (channel < 6)
        {
            *ChipID = 0x00;
            *ChipType = 0x23;
            *Channel = channel;
            ChanCount[0] = 6;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 6;
        
        if (p->VGMHead.lngHzSAA1099 & 0x40000000)
        {
            if (channel < 6)
            {
                *ChipID = 0x01;
                *ChipType = 0x23;
                *Channel = channel;
                ChanCount[0] = 6;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 6;
        }
    }
    
    if (p->VGMHead.lngHzES5503)
    {
        if (channel < 32)
        {
            *ChipID = 0x00;
            *ChipType = 0x24;
            *Channel = channel;
            ChanCount[0] = 32;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 32;
        
        if (p->VGMHead.lngHzES5503 & 0x40000000)
        {
            if (channel < 32)
            {
                *ChipID = 0x01;
                *ChipType = 0x24;
                *Channel = channel;
                ChanCount[0] = 32;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 32;
        }
    }
    
    if (p->VGMHead.lngHzES5506)
    {
        if (channel < 32)
        {
            *ChipID = 0x00;
            *ChipType = 0x25;
            *Channel = channel;
            ChanCount[0] = 32;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 32;
        
        if (p->VGMHead.lngHzES5506 & 0x40000000)
        {
            if (channel < 32)
            {
                *ChipID = 0x01;
                *ChipType = 0x25;
                *Channel = channel;
                ChanCount[0] = 32;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 32;
        }
    }

    if (p->VGMHead.lngHzX1_010)
    {
        if (channel < 16)
        {
            *ChipID = 0x00;
            *ChipType = 0x26;
            *Channel = channel;
            ChanCount[0] = 16;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 16;
        
        if (p->VGMHead.lngHzX1_010 & 0x40000000)
        {
            if (channel < 16)
            {
                *ChipID = 0x01;
                *ChipType = 0x26;
                *Channel = channel;
                ChanCount[0] = 16;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 16;
        }
    }
    
    if (p->VGMHead.lngHzC352)
    {
        if (channel < 32)
        {
            *ChipID = 0x00;
            *ChipType = 0x27;
            *Channel = channel;
            ChanCount[0] = 32;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 32;
        
        if (p->VGMHead.lngHzC352 & 0x40000000)
        {
            if (channel < 32)
            {
                *ChipID = 0x01;
                *ChipType = 0x27;
                *Channel = channel;
                ChanCount[0] = 32;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 32;
        }
    }
    
    if (p->VGMHead.lngHzGA20)
    {
        if (channel < 4)
        {
            *ChipID = 0x00;
            *ChipType = 0x28;
            *Channel = channel;
            ChanCount[0] = 4;
            ChanCount[1] = 0;
            ChanCount[2] = 0;
        }
        
        channel -= 4;
        
        if (p->VGMHead.lngHzGA20 & 0x40000000)
        {
            if (channel < 4)
            {
                *ChipID = 0x01;
                *ChipType = 0x28;
                *Channel = channel;
                ChanCount[0] = 4;
                ChanCount[1] = 0;
                ChanCount[2] = 0;
            }
            
            channel -= 4;
        }
    }
}

const char* GetAccurateChipNameByChannel(void* vgmp, UINT32 channel, UINT32 *realChannel)
{
    UINT8 ChipID, ChipType, SubType, Channel, ChanCount[3];
    GetChipByChannel(vgmp, channel, &ChipID, &ChipType, &Channel, ChanCount);
    if (ChipType == 0xFF)
        return NULL;
    *realChannel = Channel;
    GetChipClock(vgmp, ChipType, &SubType);
    return GetAccurateChipName(ChipType, SubType);
}

void SetChannelMute(void* vgmp, UINT32 channel, UINT8 mute)
{
    VGM_PLAYER *p = (VGM_PLAYER *)vgmp;
    
    UINT8 ChipID, ChipType, Channel, ChanCount[3];
    
    CHIP_OPTS *opts;
    
    UINT8 FieldNumber;
    
    UINT32 *ChnMutes;
    
    GetChipByChannel(vgmp, channel, &ChipID, &ChipType, &Channel, ChanCount);
    
    if (ChipType == 0xFF)
        return;

    opts = (CHIP_OPTS *)(&p->ChipOpts[ChipID]) + ChipType;
    
    ChnMutes = (UINT32 *)(&opts->ChnMute1);
    
    for (FieldNumber = 0; FieldNumber < 3; FieldNumber++)
    {
        if (Channel < ChanCount[FieldNumber])
        {
            if (mute)
                ChnMutes[FieldNumber] |= 1 << Channel;
            else
                ChnMutes[FieldNumber] &= ~(1 << Channel);
            break;
        }
        
        Channel -= ChanCount[FieldNumber];
    }
    
    Chips_GeneralActions(p, 0x10);
}
