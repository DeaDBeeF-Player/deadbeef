// VGMPlay_Intf.h: VGMPlay Interface Header File
//

//#define NO_WCHAR_FILENAMES
#ifndef WIN32
// Linux uses UTF-8 Unicode and has no special wide-character file routines.
#define NO_WCHAR_FILENAMES
#endif

typedef struct waveform_16bit_stereo
{
	INT16 Left;
	INT16 Right;
} WAVE_16BS;

typedef struct waveform_32bit_stereo
{
	INT32 Left;
	INT32 Right;
} WAVE_32BS;

typedef struct vgm_file VGM_FILE;
struct vgm_file
{
  int (*Read)(VGM_FILE*, void*, UINT32);
	int (*Seek)(VGM_FILE*, UINT32);
	UINT32 (*GetSize)(VGM_FILE*);
	UINT32 (*Tell)(VGM_FILE*);
};

#ifdef __cplusplus
extern "C" {
#endif

void * VGMPlay_Init(void);
void VGMPlay_Init2(void* vgmp);
void VGMPlay_Deinit(void* vgmp);

UINT32 GetGZFileLength(const char* FileName);

bool OpenVGMFile(void* vgmp, const char* FileName);
bool OpenVGMFile_Handle(void* vgmp, VGM_FILE*);
void CloseVGMFile(void* vgmp);

void FreeGD3Tag(GD3_TAG* TagData);
UINT32 GetVGMFileInfo(const char* FileName, VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag);
UINT32 GetVGMFileInfo_Handle(VGM_FILE*, VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag);
UINT32 CalcSampleMSec(void* vgmp, UINT64 Value, UINT8 Mode);
UINT32 CalcSampleMSecExt(void* vgmp, UINT64 Value, UINT8 Mode, VGM_HEADER* FileHead);
const char* GetChipName(UINT8 ChipID);
const char* GetAccurateChipName(UINT8 ChipID, UINT8 SubType);
UINT32 GetChipClock(void* vgmp, UINT8 ChipID, UINT8* RetSubType);
    
const char* GetAccurateChipNameByChannel(void* vgmp, UINT32 channel, UINT32 *realChannel);
    
void SetChannelMute(void* vgmp, UINT32 channel, UINT8 mute);

#ifndef NO_WCHAR_FILENAMES
UINT32 GetGZFileLengthW(const wchar_t* FileName);
bool OpenVGMFileW(void* vgmp, const wchar_t* FileName);
UINT32 GetVGMFileInfoW(const wchar_t* FileName, VGM_HEADER* RetVGMHead, GD3_TAG* RetGD3Tag);
#endif

INT32 SampleVGM2Playback(void* vgmp, INT32 SampleVal);
INT32 SamplePlayback2VGM(void* vgmp, INT32 SampleVal);

void PlayVGM(void* vgmp);
void StopVGM(void* vgmp);
void RestartVGM(void* vgmp);
void SeekVGM(void* vgmp, bool Relative, INT32 PlayBkSamples);
void RefreshMuting(void* vgmp);
void RefreshPanning(void* vgmp);
void RefreshPlaybackOptions(void* vgmp);

UINT32 FillBuffer(void* vgmp, WAVE_16BS* Buffer, UINT32 BufferSize);
    
#ifdef __cplusplus
}
#endif

