// TODO: Check codepage stuff (SetConsoleCP) - it looks like I don't need printc anymore
// VGMPlayUI.c: C Source File for the Console User Interface

// Note: In order to make MS VC6 NOT crash when using fprintf with stdout, stderr, etc.
//		 if linked to msvcrt.lib, the following project setting is important:
//		 C/C++ -> Code Generation -> Runtime libraries: Multithreaded DLL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
#include <locale.h>	// for setlocale
#include "stdbool.h"
#include <math.h>

#ifdef WIN32
#include <conio.h>
#include <windows.h>
#else
#include <limits.h>	// for PATH_MAX
#include <termios.h>
#include <unistd.h>	// for STDIN_FILENO and usleep()
#include <sys/time.h>	// for struct timeval in _kbhit()

#define	Sleep(msec)	usleep(msec * 1000)
#define _vsnwprintf	vswprintf
#endif

#define printerr(x)	fprintf(stderr, x)

#include "chips/mamedef.h"

#include "Stream.h"
#include "VGMPlay.h"
#include "VGMPlay_Intf.h"

#ifdef XMAS_EXTRA
#include "XMasFiles/XMasBonus.h"
#endif
#ifdef WS_DEMO
#include "XMasFiles/SWJ-SQRC01_1C.h"
#endif

#ifndef WIN32
void WaveOutLinuxCallBack(void);
#endif

#ifdef WIN32
#define DIR_CHR		'\\'
#define DIR_STR		"\\"
#define QMARK_CHR	'\"'
#else
#define DIR_CHR		'/'
#define DIR_STR		"/"
#define QMARK_CHR	'\''

#ifndef SHARE_PREFIX
#define SHARE_PREFIX	"/usr/local"
#endif

#endif

#define APP_NAME	"VGM Player"
#define APP_NAME_L	L"VGM Player"


int main(int argc, char* argv[]);
static void RemoveNewLines(char* String);
static void RemoveQuotationMarks(char* String);
static char* GetLastDirSeparator(const char* FilePath);
static bool IsAbsolutePath(const char* FilePath);
static char* GetFileExtention(const char* FilePath);
static void StandardizeDirSeparators(char* FilePath);
#ifdef WIN32
static void WinNT_Check(void);
#endif
static char* GetAppFileName(void);
static void cls(void);
#ifndef WIN32
static void changemode(bool);
static int _kbhit(void);
static int _getch(void);
#endif
static INT8 stricmp_u(const char *string1, const char *string2);
static INT8 strnicmp_u(const char *string1, const char *string2, size_t count);
static void ReadOptions(const char* AppName);
static bool GetBoolFromStr(const char* TextStr);
#if defined(XMAS_EXTRA) || defined(WS_DEMO)
static bool XMas_Extra(char* FileName, bool Mode);
#endif
#ifndef WIN32
static void ConvertCP1252toUTF8(char** DstStr, const char* SrcStr);
#endif
static bool OpenPlayListFile(const char* FileName);
static bool OpenMusicFile(const char* FileName);
extern bool OpenVGMFile(const char* FileName);
extern bool OpenOtherFile(const char* FileName);

//#ifdef WIN32
//static void printc(const char* format, ...);
//#else
#define	printc	printf
//#endif
static void wprintc(const wchar_t* format, ...);
static void PrintChipStr(UINT8 ChipID, UINT8 SubType, UINT32 Clock);
static const wchar_t* GetTagStrEJ(const wchar_t* EngTag, const wchar_t* JapTag);
static void ShowVGMTag(void);

static void PlayVGM_UI(void);
INLINE INT8 sign(double Value);
INLINE long int Round(double Value);
INLINE double RoundSpecial(double Value, double RoundTo);
static void PrintMinSec(UINT32 SamplePos, UINT32 SmplRate);


// Options Variables
extern UINT32 SampleRate;	// Note: also used by some sound cores to
							//       determinate the chip sample rate

extern UINT32 VGMPbRate;
extern UINT32 VGMMaxLoop;
extern UINT32 CMFMaxLoop;
UINT32 FadeTimeN;	// normal fade time
UINT32 FadeTimePL;	// in-playlist fade time
extern UINT32 FadeTime;
UINT32 PauseTimeJ;	// Pause Time for Jingles
UINT32 PauseTimeL;	// Pause Time for Looping Songs
extern UINT32 PauseTime;
static UINT8 Show95Cmds;

extern float VolumeLevel;
extern bool SurroundSound;
extern UINT8 HardStopOldVGMs;
extern bool FadeRAWLog;
static UINT8 LogToWave;
//extern bool FullBufFill;
extern bool PauseEmulate;
extern bool DoubleSSGVol;
static UINT16 ForceAudioBuf;

extern UINT8 ResampleMode;	// 00 - HQ both, 01 - LQ downsampling, 02 - LQ both
extern UINT8 CHIP_SAMPLING_MODE;
extern INT32 CHIP_SAMPLE_RATE;

extern UINT16 FMPort;
extern bool UseFM;
extern bool FMForce;
//extern bool FMAccurate;
extern bool FMBreakFade;
extern float FMVol;

extern CHIPS_OPTION ChipOpts[0x02];


extern bool ThreadPauseEnable;
extern volatile bool ThreadPauseConfrm;
extern bool ThreadNoWait;	// don't reset the timer
extern UINT16 AUDIOBUFFERU;
extern UINT32 SMPL_P_BUFFER;
extern char SoundLogFile[MAX_PATH];

extern UINT8 OPL_MODE;
extern UINT8 OPL_CHIPS;
//extern bool WINNT_MODE;
UINT8 NEED_LARGE_AUDIOBUFS;

extern char* AppPaths[8];
static char AppPathBuffer[MAX_PATH * 2];

static char PLFileBase[MAX_PATH];
static char PLFileName[MAX_PATH];
static UINT32 PLFileCount;
static char** PlayListFile;
static UINT32 CurPLFile;
static UINT8 NextPLCmd;
static UINT8 PLMode;	// set to 1 to show Playlist text
static bool FirstInit;
extern bool AutoStopSkip;

static char VgmFileName[MAX_PATH];
static UINT8 FileMode;
extern VGM_HEADER VGMHead;
extern UINT32 VGMDataLen;
extern UINT8* VGMData;
extern GD3_TAG VGMTag;
static PreferJapTag;

extern volatile bool PauseThread;
static bool StreamStarted;

extern float MasterVol;

extern UINT32 VGMPos;
extern INT32 VGMSmplPos;
extern INT32 VGMSmplPlayed;
extern INT32 VGMSampleRate;
extern UINT32 BlocksSent;
extern UINT32 BlocksPlayed;
static bool IsRAWLog;
extern bool EndPlay;
extern bool PausePlay;
extern bool FadePlay;
extern bool ForceVGMExec;
extern UINT8 PlayingMode;

extern UINT32 PlayingTime;

extern UINT32 FadeStart;
extern UINT32 VGMMaxLoopM;
extern UINT32 VGMCurLoop;
extern float VolumeLevelM;
bool ErrorHappened;	// used by VGMPlay.c and VGMPlay_AddFmts.c
extern float FinalVol;
extern bool ResetPBTimer;

#ifndef WIN32
static struct termios oldterm;
static bool termmode;
#endif

UINT8 CmdList[0x100];

//extern UINT8 DISABLE_YMZ_FIX;
extern UINT8 IsVGMInit;
extern UINT16 Last95Drum;	// for optvgm debugging
extern UINT16 Last95Max;	// for optvgm debugging
extern UINT32 Last95Freq;	// for optvgm debugging

static bool PrintMSHours;

int main(int argc, char* argv[])
{
	int argbase;
	int ErrRet;
	char* AppName;
#if defined(XMAS_EXTRA) || defined(WS_DEMO)
	bool XMasEnable;
#endif
	char* AppPathPtr;
	const char* StrPtr;
	const char* FileExt;
	UINT8 CurPath;
	UINT32 ChrPos;
	INT32 OldCP;
	
	// set locale to "current system locale"
	// (makes Unicode characters (like umlauts) work under Linux and fixes some
	//  Unicode -> ANSI conversions)
	setlocale(LC_CTYPE, "");
	
#ifndef WIN32
	tcgetattr(STDIN_FILENO, &oldterm);
	termmode = false;
#endif
	
	if (argc > 1)
	{
		if (! stricmp_u(argv[1], "-v") || ! stricmp_u(argv[1], "--version"))
		{
			printf("VGMPlay %s"
#if defined(APLHA)
					" alpha"
#elif defined(BETA)
					" beta"
#endif
					", supports VGM %s\n", VGMPLAY_VER_STR, VGM_VER_STR);
			return 0;
		}
	}
	
#ifdef SET_CONSOLE_TITLE
#ifdef WIN32
	SetConsoleTitle(APP_NAME);			// Set Windows Console Title
#else
	printf("\x1B]0;%s\x07", APP_NAME);	// Set xterm/rxvt Terminal Title
#endif
#endif
	
	printf(APP_NAME);
#ifdef XMAS_EXTRA
	printf(" - XMas Release");
#endif
	printf("\n----------\n");
	
	//if (argv[0x00] == NULL)
	//	printf("Argument \"Application-Name\" is NULL!\n");
	
	// Warning! It's dangerous to use Argument 0!
	// AppName may be "vgmplay" instead of "vgmplay.exe"
	
	VGMPlay_Init();
	
	// Note: Paths are checked from last to first.
	CurPath = 0x00;
	AppPathPtr = AppPathBuffer;
#ifndef WIN32
	// Path 1: global share directory
	AppPaths[CurPath] = SHARE_PREFIX "/share/vgmplay/";
	CurPath ++;
#endif
	
	// Path 2: exe's directory
	AppName = GetAppFileName();	// "C:\VGMPlay\VGMPlay.exe"
	// Note: GetAppFileName always returns native directory separators.
	StrPtr = strrchr(AppName, DIR_CHR);
	if (StrPtr != NULL)
	{
		ChrPos = StrPtr + 1 - AppName;
		strncpy(AppPathPtr, AppName, ChrPos);
		AppPathPtr[ChrPos] = 0x00;	// "C:\VGMPlay\"
		AppPaths[CurPath] = AppPathPtr;
		CurPath ++;
		AppPathPtr += ChrPos + 1;
	}
	
#ifndef WIN32
	// Path 3: home directory
	StrPtr = getenv("XDG_CONFIG_HOME");
	if (StrPtr != NULL && StrPtr[0] == '\0')
	{
		strcpy(AppPathPtr, StrPtr);
	}
	else
	{
		StrPtr = getenv("HOME");
		if (StrPtr != NULL)
			strcpy(AppPathPtr, StrPtr);
		else
			strcpy(AppPathPtr, "");
		strcat(AppPathPtr, "/.config");
	}
	strcat(AppPathPtr, "/vgmplay/");
	AppPaths[CurPath] = AppPathPtr;
	CurPath ++;
	AppPathPtr += strlen(AppPathPtr) + 1;
#endif
	
	// Path 4: working directory ("\0")
	AppPathPtr[0] = '\0';
	AppPaths[CurPath] = AppPathPtr;
	CurPath ++;
	
#if 0	// set to 1 to print all selected search paths
	CurPath = 0;
	while(AppPaths[CurPath] != NULL)
	{
		printf("Path %u: %s\n", CurPath + 1, AppPaths[CurPath]);
		CurPath ++;
	}
#endif
	
	ReadOptions(AppName);
	VGMPlay_Init2();
	
	ErrRet = 0;
	argbase = 0x01;
	if (argc >= argbase + 0x01)
	{
		if (! strnicmp_u(argv[argbase], "-LogSound:", 10))
		{
			LogToWave = (UINT8)strtoul(argv[argbase] + 10, NULL, 0);
			argbase ++;
		}
	}
	
	printf("\nFile Name:\t");
	if (argc <= argbase)
	{
#ifdef WIN32
		OldCP = GetConsoleCP();
		
		// Set the Console Input Codepage to ANSI.
		// The Output Codepage must be left at OEM, else the displayed characters are wrong.
		ChrPos = GetACP();
		ErrRet = SetConsoleCP(ChrPos);			// set input codepage
		//ErrRet = SetConsoleOutputCP(ChrPos);	// set output codepage (would be a bad idea)
		
		StrPtr = fgets(VgmFileName, MAX_PATH, stdin);
		if (StrPtr == NULL)
			VgmFileName[0] = '\0';
		
		// Playing with the console font resets the Console Codepage to OEM, so I have to
		// convert the file name in this case.
		if (GetConsoleCP() == GetOEMCP())
			OemToChar(VgmFileName, VgmFileName);	// OEM -> ANSI conversion
		
		// This fixes the display of non-ANSI characters.
		ErrRet = SetConsoleCP(OldCP);
		
		// This codepage stuff drives me insane.
		// Debug and Release build behave differently - WHAT??
		//
		// There a list of behaviours.
		// Debug and Release were tested by dropping a file on it and via Visual Studio.
		//
		// Input CP 850, Output CP 850
		//	Debug build:	Dynamite D³x
		//	Release build:	Dynamite Düx
		// Input CP 1252, Output CP 850
		//	Debug build:	Dynamite D³x
		//	Release build:	Dynamite D³x
		// Input CP 850, Output CP 1252
		//	Debug build:	Dynamite D³x [tag display wrong]
		//	Release build:	Dynamite Düx [tag display wrong]
		// Input CP 1252, Output CP 1252
		//	Debug build:	Dynamite D³x [tag display wrong]
		//	Release build:	Dynamite D³x [tag display wrong]
#else
		StrPtr = fgets(VgmFileName, MAX_PATH, stdin);
		if (StrPtr == NULL)
			VgmFileName[0] = '\0';
#endif
		
		RemoveNewLines(VgmFileName);
		RemoveQuotationMarks(VgmFileName);
	}
	else
	{
		// The argument should already use the ANSI codepage.
		strcpy(VgmFileName, argv[argbase]);
		printc("%s\n", VgmFileName);
	}
	if (! strlen(VgmFileName))
		goto ExitProgram;
	StandardizeDirSeparators(VgmFileName);
	
#if defined(XMAS_EXTRA) || defined(WS_DEMO)
	XMasEnable = XMas_Extra(VgmFileName, 0x00);
#endif
	
#if 0
	{	// Print hex characters of file name (for vgm-player script debugging)
		const char* CurChr;
		
#ifdef WIN32
		printf("Input CP: %d, Output CP: %d\n", GetConsoleCP(), GetConsoleOutputCP());
#endif
		printf("VgmFileName: ");
		
		CurChr = VgmFileName;
		while(*CurChr != '\0')
		{
			printf("%02X ", (UINT8)*CurChr);
			CurChr ++;
		}
		printf("%02X\n", (UINT8)*CurChr);
		_getch();
	}
#endif
#if 0
	{	// strip spaces and \n (fixed bugs with vgm-player script with un-7z)
		char* CurChr;
		
		// trim \n and spaces off
		CurChr = strchr(VgmFileName, '\n');
		if (CurChr != NULL)
			*CurChr = '\0';
		CurChr = VgmFileName + strlen(VgmFileName) - 1;
		while(CurChr > VgmFileName && *CurChr == ' ')
			*(CurChr --) = '\0';
	}
#endif
	
	FirstInit = true;
	StreamStarted = false;
	FileExt = GetFileExtention(VgmFileName);
	if (FileExt == NULL || stricmp_u(FileExt, "m3u"))
		PLMode = 0x00;
	else
		PLMode = 0x01;
	
	if (! PLMode)
	{
		PLFileCount = 0x00;
		CurPLFile = 0x00;
		// no Play List File
		if (! OpenMusicFile(VgmFileName))
		{
			printerr("Error opening the file!\n");
			if (argv[0][1] == ':')
				_getch();
			ErrRet = 1;
			goto ExitProgram;
		}
		printf("\n");
		
		ErrorHappened = false;
		FadeTime = FadeTimeN;
		PauseTime = PauseTimeL;
		PrintMSHours = (VGMHead.lngTotalSamples >= 158760000);	// 44100 smpl * 60 sec * 60 min
		ShowVGMTag();
		NextPLCmd = 0x80;
		PlayVGM_UI();
		
		CloseVGMFile();
	}
	else
	{
		strcpy(PLFileName, VgmFileName);
		if (! OpenPlayListFile(PLFileName))
		{
			printerr("Error opening the playlist!\n");
			if (argv[0][1] == ':')
				_getch();
			ErrRet = 1;
			goto ExitProgram;
		}
		
		for (CurPLFile = 0x00; CurPLFile < PLFileCount; CurPLFile ++)
		{
			if (PLMode)
			{
				cls();
				printf(APP_NAME);
				printf("\n----------\n");
				printc("\nPlaylist File:\t%s\n", PLFileName);
				printf("Playlist Entry:\t%u / %u\n", CurPLFile + 1, PLFileCount);
				printc("File Name:\t%s\n", PlayListFile[CurPLFile]);
			}
			
			if (IsAbsolutePath(PlayListFile[CurPLFile]))
			{
				strcpy(VgmFileName, PlayListFile[CurPLFile]);
			}
			else
			{
				strcpy(VgmFileName, PLFileBase);
				strcat(VgmFileName, PlayListFile[CurPLFile]);
			}
			
			if (! OpenMusicFile(VgmFileName))
			{
				printf("Error opening the file!\n");
				_getch();
				while(_kbhit())
					_getch();
				continue;
			}
			printf("\n");
			
			ErrorHappened = false;
			if (CurPLFile < PLFileCount - 1)
				FadeTime = FadeTimePL;
			else
				FadeTime = FadeTimeN;
			PauseTime = VGMHead.lngLoopOffset ? PauseTimeL : PauseTimeJ;
			PrintMSHours = (VGMHead.lngTotalSamples >= 158760000);
			ShowVGMTag();
			NextPLCmd = 0x00;
			PlayVGM_UI();
			
			CloseVGMFile();
			
			if (ErrorHappened)
			{
				if (_kbhit())
					_getch();
				_getch();
				ErrorHappened = false;
			}
			if (NextPLCmd == 0xFF)
				break;
			else if (NextPLCmd == 0x01)
				CurPLFile -= 0x02;	// Jump to last File (-2 + 1 = -1)
		}
	}
	
	if (ErrorHappened && argv[0][1] == ':')
	{
		if (_kbhit())
			_getch();
		_getch();
	}
	
#ifdef _DEBUG
	printf("Press any key ...");
	_getch();
#endif
	
ExitProgram:
#if defined(XMAS_EXTRA) || defined(WS_DEMO)
	if (XMasEnable)
		XMas_Extra(VgmFileName, 0x01);
#endif
#ifndef WIN32
	changemode(false);
#ifdef SET_CONSOLE_TITLE
//	printf("\x1B]0;${USER}@${HOSTNAME}: ${PWD/$HOME/~}\x07", APP_NAME);	// Reset xterm/rxvt Terminal Title
#endif
#endif
	VGMPlay_Deinit();
	free(AppName);
	
	return ErrRet;
}

static void RemoveNewLines(char* String)
{
	char* StrPtr;
	
	StrPtr = String + strlen(String) - 1;
	while(StrPtr >= String && (UINT8)*StrPtr < 0x20)
	{
		*StrPtr = '\0';
		StrPtr --;
	}
	
	return;
}

static void RemoveQuotationMarks(char* String)
{
	UINT32 StrLen;
	char* EndQMark;
	
	if (String[0x00] != QMARK_CHR)
		return;
	
	StrLen = strlen(String);
	memmove(String, String + 0x01, StrLen);	// Remove first char
	EndQMark = strrchr(String, QMARK_CHR);
	if (EndQMark != NULL)
		*EndQMark = 0x00;	// Remove last Quot.-Mark
	
	return;
}

static char* GetLastDirSeparator(const char* FilePath)
{
	char* SepPos1;
	char* SepPos2;
	
	SepPos1 = strrchr(FilePath, '/');
	SepPos2 = strrchr(FilePath, '\\');
	if (SepPos1 < SepPos2)
		return SepPos2;
	else
		return SepPos1;
}

static bool IsAbsolutePath(const char* FilePath)
{
#ifdef WIN32
	if (FilePath[0] == '\0')
		return false;	// empty string
	if (FilePath[1] == ':')
		return true;	// Device Path: C:\path
	if (! strncmp(FilePath, "\\\\", 2))
		return true;	// Network Path: \\computername\path
#else
	if (FilePath[0] == '/')
		return true;	// absolute UNIX path
#endif
	return false;
}

static char* GetFileExtention(const char* FilePath)
{
	char* DirSepPos;
	char* ExtDotPos;
	
	DirSepPos = GetLastDirSeparator(FilePath);
	if (DirSepPos == NULL)
		DirSepPos = (char*)FilePath;
	ExtDotPos = strrchr(DirSepPos, '.');
	if (ExtDotPos == NULL)
		return NULL;
	else
		return ExtDotPos + 1;
}

static void StandardizeDirSeparators(char* FilePath)
{
	char* CurChr;
	
	CurChr = FilePath;
	while(*CurChr != '\0')
	{
		if (*CurChr == '\\' || *CurChr == '/')
			*CurChr = DIR_CHR;
		CurChr ++;
	}
	
	return;
}

#ifdef WIN32
static void WinNT_Check(void)
{
	OSVERSIONINFO VerInf;
	
	VerInf.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInf);
	//WINNT_MODE = (VerInf.dwPlatformId == VER_PLATFORM_WIN32_NT);
	
	/* Following Systems need larger Audio Buffers:
		- Windows 95 (500+ ms)
		- Windows Vista (200+ ms)
	Tested Systems:
		- Windows 95B
		- Windows 98 SE
		- Windows 2000
		- Windows XP (32-bit)
		- Windows Vista (32-bit)
		- Windows 7 (64-bit)
	*/
	
	NEED_LARGE_AUDIOBUFS = 0;
	if (VerInf.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		if (VerInf.dwMajorVersion == 4 && VerInf.dwMinorVersion == 0)
			NEED_LARGE_AUDIOBUFS = 50;	// Windows 95
	}
	else if (VerInf.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if (VerInf.dwMajorVersion == 6 && VerInf.dwMinorVersion == 0)
			NEED_LARGE_AUDIOBUFS = 20;	// Windows Vista
	}
	
	return;
}
#endif

static char* GetAppFileName(void)
{
	char* AppPath;
	int RetVal;
	
	AppPath = (char*)malloc(MAX_PATH * sizeof(char));
#ifdef WIN32
	RetVal = GetModuleFileName(NULL, AppPath, MAX_PATH);
	if (! RetVal)
		AppPath[0] = '\0';
#else
	RetVal = readlink("/proc/self/exe", AppPath, MAX_PATH);
	if (RetVal == -1)
		AppPath[0] = '\0';
#endif
	
	return AppPath;
}

static void cls(void)
{
#ifdef WIN32
	// CLS-Function from the MSDN Help
	HANDLE hConsole;
	COORD coordScreen = {0, 0};
	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;
	
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	
	// get the number of character cells in the current buffer
	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	
	// fill the entire screen with blanks
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen,
											&cCharsWritten);
	
	// get the current text attribute
	//bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	
	// now set the buffer's attributes accordingly
	//bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen,
	//										&cCharsWritten);
	
	// put the cursor at (0, 0)
	bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
#else
	system("clear");
#endif
	
	return;
}

#ifndef WIN32

static void changemode(bool dir)
{
	static struct termios newterm;
	
	if (termmode == dir)
		return;
	
	if (dir)
	{
		newterm = oldterm;
		newterm.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
	}
	else
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
	}
	termmode = dir;
	
	return;
}

static int _kbhit(void)
{
	struct timeval tv;
	fd_set rdfs;
	int kbret;
	bool needchg;
	
	needchg = (! termmode);
	if (needchg)
		changemode(true);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	
	FD_ZERO(&rdfs);
	FD_SET(STDIN_FILENO, &rdfs);
	
	select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &tv);
	kbret = FD_ISSET(STDIN_FILENO, &rdfs);
	if (needchg)
		changemode(false);
	
	return kbret;
}

static int _getch(void)
{
	int ch;
	bool needchg;
	
	needchg = (! termmode);
	if (needchg)
		changemode(true);
	ch = getchar();
	if (needchg)
		changemode(false);
	
	return ch;
}
#endif

static INT8 stricmp_u(const char *string1, const char *string2)
{
	// my own stricmp, because VC++6 doesn't find _stricmp when compiling without
	// standard libraries
	const char* StrPnt1;
	const char* StrPnt2;
	char StrChr1;
	char StrChr2;
	
	StrPnt1 = string1;
	StrPnt2 = string2;
	while(true)
	{
		StrChr1 = toupper(*StrPnt1);
		StrChr2 = toupper(*StrPnt2);
		
		if (StrChr1 < StrChr2)
			return -1;
		else if (StrChr1 > StrChr2)
			return +1;
		if (StrChr1 == 0x00)
			return 0;
		
		StrPnt1 ++;
		StrPnt2 ++;
	}
	
	return 0;
}

static INT8 strnicmp_u(const char *string1, const char *string2, size_t count)
{
	// my own strnicmp, because GCC doesn't seem to have _strnicmp
	const char* StrPnt1;
	const char* StrPnt2;
	char StrChr1;
	char StrChr2;
	size_t CurChr;
	
	StrPnt1 = string1;
	StrPnt2 = string2;
	CurChr = 0x00;
	while(CurChr < count)
	{
		StrChr1 = toupper(*StrPnt1);
		StrChr2 = toupper(*StrPnt2);
		
		if (StrChr1 < StrChr2)
			return -1;
		else if (StrChr1 > StrChr2)
			return +1;
		if (StrChr1 == 0x00)
			return 0;
		
		StrPnt1 ++;
		StrPnt2 ++;
		CurChr ++;
	}
	
	return 0;
}

static void ReadOptions(const char* AppName)
{
	const UINT8 CHN_COUNT[CHIP_COUNT] =
	{	0x04, 0x09, 0x06, 0x08, 0x10, 0x08, 0x03, 0x00,
		0x00, 0x09, 0x09, 0x09, 0x12, 0x00, 0x0C, 0x08,
		0x08, 0x00, 0x03, 0x04, 0x05, 0x1C, 0x00, 0x00,
		0x04, 0x05, 0x08, 0x08, 0x18, 0x04, 0x04, 0x10,
		0x20, 0x04, 0x06, 0x06, 0x20, 0x20, 0x10, 0x20,
		0x04
	};
	const UINT8 CHN_MASK_CNT[CHIP_COUNT] =
	{	0x04, 0x0E, 0x07, 0x08, 0x10, 0x08, 0x03, 0x06,
		0x06, 0x0E, 0x0E, 0x0E, 0x17, 0x18, 0x0C, 0x08,
		0x08, 0x00, 0x03, 0x04, 0x05, 0x1C, 0x00, 0x00,
		0x04, 0x05, 0x08, 0x08, 0x18, 0x04, 0x04, 0x10,
		0x20, 0x04, 0x06, 0x06, 0x20, 0x20, 0x10, 0x20,
		0x04
	};
	char* FileName;
	FILE* hFile;
	char TempStr[0x40];
	UINT32 StrLen;
	UINT32 TempLng;
	char* LStr;
	char* RStr;
	UINT8 IniSection;
	UINT8 CurChip;
	CHIP_OPTS* TempCOpt;
	CHIP_OPTS* TempCOpt2;
	UINT8 CurChn;
	char* TempPnt;
	bool TempFlag;
	
	// most defaults are set by VGMPlay_Init()
	FadeTimeN = FadeTime;
	PauseTimeJ = PauseTime;
	PauseTimeL = 0;
	Show95Cmds = 0x00;
	LogToWave = 0x00;
	ForceAudioBuf = 0x00;
	PreferJapTag = false;
	
	if (AppName == NULL)
	{
		printerr("Argument \"Application-Path\" is NULL!\nSkip loading INI.\n");
		return;
	}
	
	// AppName: "C:\VGMPlay\VGMPlay.exe"
	RStr = strrchr(AppName, DIR_CHR);
	if (RStr != NULL)
		RStr ++;
	else
		RStr = (char*)AppName;
	FileName = (char*)malloc(strlen(RStr) + 0x05);	// ".ini" + 00
	strcpy(FileName, RStr);
	// FileName: "VGMPlay.exe"
	
	RStr = GetFileExtention(FileName);
	if (RStr == NULL)
	{
		RStr = FileName + strlen(FileName);
		*RStr = '.';
		RStr ++;
	}
	strcpy(RStr, "ini");
	// FileName: "VGMPlay.ini"
	
	LStr = FileName;
	FileName = FindFile(LStr);
	free(LStr);
	if (FileName == NULL)
	{
		// on Linux platforms, it searches for "vgmplay.ini" first and
		// file names are case sensitive
		FileName = FindFile("VGMPlay.ini");
	}
	if (FileName == NULL)
	{
		printerr("Failed to load INI.\n");
		return;
	}
	hFile = fopen(FileName, "rt");
	free(FileName);
	if (hFile == NULL)
	{
		printerr("Failed to load INI.\n");
		return;
	}
	
	IniSection = 0x00;
	while(! feof(hFile))
	{
		LStr = fgets(TempStr, 0x40, hFile);
		if (LStr == NULL)
			break;
		if (TempStr[0x00] == ';')	// Comment line
			continue;
		
		StrLen = strlen(TempStr) - 0x01;
		//if (TempStr[StrLen] == '\n')
		//	TempStr[StrLen] = 0x00;
		while(TempStr[StrLen] < 0x20)
		{
			TempStr[StrLen] = 0x00;
			if (! StrLen)
				break;
			StrLen --;
		}
		if (! StrLen)
			continue;
		StrLen ++;
		
		LStr = &TempStr[0x00];
		while(*LStr == ' ')
			LStr ++;
		if (LStr[0x00] == ';')	// Comment line
			continue;
		
		if (LStr[0x00] == '[')
			RStr = strchr(TempStr, ']');
		else
			RStr = strchr(TempStr, '=');
		if (RStr == NULL)
			continue;
		
		if (LStr[0x00] == '[')
		{
			// Line pattern: [Group]
			LStr ++;
			RStr = strchr(TempStr, ']');
			if (RStr != NULL)
				RStr[0x00] = 0x00;
			
			if (! stricmp_u(LStr, "General"))
			{
				IniSection = 0x00;
			}
			else
			{
				IniSection = 0xFF;
				for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++)
				{
					if (! stricmp_u(LStr, GetChipName(CurChip)))
					{
						IniSection = 0x80 | CurChip;
						break;
					}
				}
				if (IniSection == 0xFF)
					continue;
			}
		}
		else
		{
			// Line pattern: Option = Value
			TempLng = RStr - TempStr;
			TempStr[TempLng] = 0x00;
			
			// Prepare Strings (trim the spaces)
			RStr = &TempStr[TempLng - 0x01];
			while(*RStr == ' ')
				*(RStr --) = 0x00;
			
			RStr = &TempStr[StrLen - 0x01];
			while(*RStr == ' ')
				*(RStr --) = 0x00;
			RStr = &TempStr[TempLng + 0x01];
			while(*RStr == ' ')
				RStr ++;
			
			switch(IniSection)
			{
			case 0x00:	// General Sction
				if (! stricmp_u(LStr, "SampleRate"))
				{
					SampleRate = strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "PlaybackRate"))
				{
					VGMPbRate = strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "DoubleSSGVol"))
				{
					DoubleSSGVol = GetBoolFromStr(RStr);
				}
				else if (! stricmp_u(LStr, "PreferJapTag"))
				{
					PreferJapTag = GetBoolFromStr(RStr);
				}
				else if (! stricmp_u(LStr, "FadeTime"))
				{
					FadeTimeN = strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "FadeTimePL"))
				{
					FadeTimePL = strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "JinglePause"))
				{
					PauseTimeJ = strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "HardStopOld"))
				{
					HardStopOldVGMs = (UINT8)strtoul(RStr, &TempPnt, 0);
					if (TempPnt == RStr)
						HardStopOldVGMs = GetBoolFromStr(RStr) ? 0x01 : 0x00;
				}
				else if (! stricmp_u(LStr, "FadeRAWLogs"))
				{
					FadeRAWLog = GetBoolFromStr(RStr);
				}
				else if (! stricmp_u(LStr, "Volume"))
				{
					VolumeLevel = (float)strtod(RStr, NULL);
				}
				else if (! stricmp_u(LStr, "LogSound"))
				{
					//LogToWave = GetBoolFromStr(RStr);
					LogToWave = (UINT8)strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "MaxLoops"))
				{
					VGMMaxLoop = strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "MaxLoopsCMF"))
				{
					CMFMaxLoop = strtoul(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "ResamplingMode"))
				{
					ResampleMode = (UINT8)strtol(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "ChipSmplMode"))
				{
					CHIP_SAMPLING_MODE = (UINT8)strtol(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "ChipSmplRate"))
				{
					CHIP_SAMPLE_RATE = strtol(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "AudioBuffers"))
				{
					ForceAudioBuf = (UINT16)strtol(RStr, NULL, 0);
					if (ForceAudioBuf < 0x04)
						ForceAudioBuf = 0x00;
				}
				else if (! stricmp_u(LStr, "SurroundSound"))
				{
					SurroundSound = GetBoolFromStr(RStr);
				}
				else if (! stricmp_u(LStr, "EmulatePause"))
				{
					PauseEmulate = GetBoolFromStr(RStr);
				}
				else if (! stricmp_u(LStr, "ShowStreamCmds"))
				{
					Show95Cmds = (UINT8)strtol(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "FMPort"))
				{
					FMPort = (UINT16)strtoul(RStr, NULL, 16);
				}
				else if (! stricmp_u(LStr, "FMForce"))
				{
					FMForce = GetBoolFromStr(RStr);
				}
				else if (! stricmp_u(LStr, "FMVolume"))
				{
					FMVol = (float)strtod(RStr, NULL);
				}
				/*else if (! stricmp_u(LStr, "AccurateFM"))
				{
					FMAccurate = GetBoolFromStr(RStr);
				}*/
				else if (! stricmp_u(LStr, "FMSoftStop"))
				{
					FMBreakFade = GetBoolFromStr(RStr);
				}
				break;
			case 0x80:	// SN76496
			case 0x81:	// YM2413
			case 0x82:	// YM2612
			case 0x83:	// YM2151
			case 0x84:	// SegaPCM
			case 0x85:	// RF5C68
			case 0x86:	// YM2203
			case 0x87:	// YM2608
			case 0x88:	// YM2610
			case 0x89:	// YM3812
			case 0x8A:	// YM3526
			case 0x8B:	// Y8950
			case 0x8C:	// YMF262
			case 0x8D:	// YMF278B
			case 0x8E:	// YMF271
			case 0x8F:	// YMZ280B
			case 0x90:	// RF5C164
			case 0x91:	// PWM
			case 0x92:	// AY8910
			case 0x93:	// GameBoy
			case 0x94:	// NES
			case 0x95:	// MultiPCM
			case 0x96:	// UPD7759
			case 0x97:	// OKIM6258
			case 0x98:	// OKIM6295
			case 0x99:	// K051649
			case 0x9A:	// K054539
			case 0x9B:	// HuC6280
			case 0x9C:	// C140
			case 0x9D:	// K053260
			case 0x9E:	// Pokey
			case 0x9F:	// QSound
			case 0xA0:	// SCSP
			case 0xA1:	// WonderSwan
			case 0xA2:	// VSU
			case 0xA3:	// SAA1099
			case 0xA4:	// ES5503
			case 0xA5:	// ES5506
			case 0xA6:	// X1_010
			case 0xA7:	// C352
			case 0xA8:	// GA20
				CurChip = IniSection & 0x7F;
				TempCOpt = (CHIP_OPTS*)&ChipOpts[0x00] + CurChip;
				
				if (! stricmp_u(LStr, "Disabled"))
				{
					TempCOpt->Disabled = GetBoolFromStr(RStr);
				}
				else if (! stricmp_u(LStr, "EmulatorType"))
				{
					TempCOpt->EmuCore = (UINT8)strtol(RStr, NULL, 0);
				}
				else if (! stricmp_u(LStr, "MuteMask"))
				{
					if (! CHN_COUNT[CurChip])
						break;	// must use MuteMaskFM and MuteMask???
					TempCOpt->ChnMute1 = strtoul(RStr, NULL, 0);
					if (CHN_MASK_CNT[CurChip] < 0x20)
						TempCOpt->ChnMute1 &= (1 << CHN_MASK_CNT[CurChip]) - 1;
				}
				else if (! strnicmp_u(LStr, "MuteCh", 0x06))
				{
					if (! CHN_COUNT[CurChip])
						break;	// must use MuteFM and Mute???
					CurChn = (UINT8)strtol(LStr + 0x06, &TempPnt, 0);
					if (TempPnt == NULL || *TempPnt)
						break;
					if (CurChn >= CHN_COUNT[CurChip])
						break;
					TempFlag = GetBoolFromStr(RStr);
					TempCOpt->ChnMute1 &= ~(0x01 << CurChn);
					TempCOpt->ChnMute1 |= TempFlag << CurChn;
				}
				else
				{
					switch(CurChip)
					{
					//case 0x00:	// SN76496
					case 0x02:	// YM2612
						if (! stricmp_u(LStr, "MuteDAC"))
						{
							CurChn = 0x06;
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->ChnMute1 &= ~(0x01 << CurChn);
							TempCOpt->ChnMute1 |= TempFlag << CurChn;
						}
						else if (! stricmp_u(LStr, "DACHighpass"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 0);
							TempCOpt->SpecialFlags |= TempFlag << 0;
						}
						else if (! stricmp_u(LStr, "SSG-EG"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 1);
							TempCOpt->SpecialFlags |= TempFlag << 1;
						}
						else if (! stricmp_u(LStr, "PseudoStereo"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 2);
							TempCOpt->SpecialFlags |= TempFlag << 2;
						}
						break;
					//case 0x03:	// YM2151
					//case 0x04:	// SegaPCM
					//case 0x05:	// RF5C68
					case 0x06:	// YM2203
						if (! stricmp_u(LStr, "DisableAY"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 0);
							TempCOpt->SpecialFlags |= TempFlag << 0;
						}
						break;
					case 0x07:	// YM2608
					case 0x08:	// YM2610
						if (! stricmp_u(LStr, "DisableAY"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 0);
							TempCOpt->SpecialFlags |= TempFlag << 0;
						}
						else if (! stricmp_u(LStr, "MuteMask_FM"))
						{
							TempCOpt->ChnMute1 = strtoul(RStr, NULL, 0);
							TempCOpt->ChnMute1 &= (1 << CHN_MASK_CNT[CurChip]) - 1;
						}
						else if (! stricmp_u(LStr, "MuteMask_PCM"))
						{
							TempCOpt->ChnMute2 = strtoul(RStr, NULL, 0);
							TempCOpt->ChnMute2 &= (1 << (CHN_MASK_CNT[CurChip] + 1)) - 1;
						}
						else if (! strnicmp_u(LStr, "MuteFMCh", 0x08))
						{
							CurChn = (UINT8)strtol(LStr + 0x08, &TempPnt, 0);
							if (TempPnt == NULL || *TempPnt)
								break;
							if (CurChn >= CHN_COUNT[CurChip])
								break;
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->ChnMute1 &= ~(0x01 << CurChn);
							TempCOpt->ChnMute1 |= TempFlag << CurChn;
						}
						else if (! strnicmp_u(LStr, "MutePCMCh", 0x08))
						{
							CurChn = (UINT8)strtol(LStr + 0x08, &TempPnt, 0);
							if (TempPnt == NULL || *TempPnt)
								break;
							if (CurChn >= CHN_COUNT[CurChip])
								break;
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->ChnMute2 &= ~(0x01 << CurChn);
							TempCOpt->ChnMute2 |= TempFlag << CurChn;
						}
						else if (! stricmp_u(LStr, "MuteDT"))
						{
							CurChn = 0x06;
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->ChnMute2 &= ~(0x01 << CurChn);
							TempCOpt->ChnMute2 |= TempFlag << CurChn;
						}
						break;
					case 0x01:	// YM2413
					case 0x09:	// YM3812
					case 0x0A:	// YM3526
					case 0x0B:	// Y8950
					case 0x0C:	// YMF262
						CurChn = 0xFF;
						if (! stricmp_u(LStr, "MuteBD"))
							CurChn = 0x00;
						else if (! stricmp_u(LStr, "MuteSD"))
							CurChn = 0x01;
						else if (! stricmp_u(LStr, "MuteTOM"))
							CurChn = 0x02;
						else if (! stricmp_u(LStr, "MuteTC"))
							CurChn = 0x03;
						else if (! stricmp_u(LStr, "MuteHH"))
							CurChn = 0x04;
						else if (CurChip == 0x0B && ! stricmp_u(LStr, "MuteDT"))
							CurChn = 0x05;
						if (CurChn != 0xFF)
						{
							if (CurChip < 0x0C)
								CurChn += 9;
							else
								CurChn += 18;
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->ChnMute1 &= ~(0x01 << CurChn);
							TempCOpt->ChnMute1 |= TempFlag << CurChn;
						}
						break;
					case 0x0D:	// YMF278B
						if (! stricmp_u(LStr, "MuteMask_FM"))
						{
							TempCOpt->ChnMute1 = strtoul(RStr, NULL, 0);
							TempCOpt->ChnMute1 &= (1 << CHN_MASK_CNT[CurChip - 0x01]) - 1;
						}
						else if (! stricmp_u(LStr, "MuteMask_WT"))
						{
							TempCOpt->ChnMute2 = strtoul(RStr, NULL, 0);
							TempCOpt->ChnMute2 &= (1 << CHN_MASK_CNT[CurChip]) - 1;
						}
						else if (! strnicmp_u(LStr, "MuteFMCh", 0x08))
						{
							CurChn = (UINT8)strtol(LStr + 0x08, &TempPnt, 0);
							if (TempPnt == NULL || *TempPnt)
								break;
							if (CurChn >= CHN_COUNT[CurChip - 0x01])
								break;
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->ChnMute1 &= ~(0x01 << CurChn);
							TempCOpt->ChnMute1 |= TempFlag << CurChn;
						}
						else if (! strnicmp_u(LStr, "MuteFM", 0x06))
						{
							CurChn = 0xFF;
							if (! stricmp_u(LStr + 6, "BD"))
								CurChn = 0x00;
							else if (! stricmp_u(LStr + 6, "SD"))
								CurChn = 0x01;
							else if (! stricmp_u(LStr + 6, "TOM"))
								CurChn = 0x02;
							else if (! stricmp_u(LStr + 6, "TC"))
								CurChn = 0x03;
							else if (! stricmp_u(LStr + 6, "HH"))
								CurChn = 0x04;
							if (CurChn != 0xFF)
							{
								CurChn += 18;
								TempFlag = GetBoolFromStr(RStr);
								TempCOpt->ChnMute1 &= ~(0x01 << CurChn);
								TempCOpt->ChnMute1 |= TempFlag << CurChn;
							}
						}
						else if (! strnicmp_u(LStr, "MuteWTCh", 0x08))
						{
							CurChn = (UINT8)strtol(LStr + 0x08, &TempPnt, 0);
							if (TempPnt == NULL || *TempPnt)
								break;
							if (CurChn >= CHN_MASK_CNT[CurChip])
								break;
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->ChnMute2 &= ~(0x01 << CurChn);
							TempCOpt->ChnMute2 |= TempFlag << CurChn;
						}
						break;
					//case 0x0E:	// YMF271
					//case 0x0F:	// YMZ280B
						/*if (! stricmp_u(LStr, "DisableFix"))
						{
							DISABLE_YMZ_FIX = GetBoolFromStr(RStr);
						}
						break;*/
					//case 0x10:	// RF5C164
					//case 0x11:	// PWM
					//case 0x12:	// AY8910
					case 0x13:	// GameBoy
						if (! stricmp_u(LStr, "BoostWaveChn"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 0);
							TempCOpt->SpecialFlags |= TempFlag << 0;
						}
						else if (! stricmp_u(LStr, "LowerNoiseChn"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 1);
							TempCOpt->SpecialFlags |= TempFlag << 1;
						}
						else if (! stricmp_u(LStr, "Inaccurate"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 2);
							TempCOpt->SpecialFlags |= TempFlag << 2;
						}
						break;
					case 0x14:	// NES
						if (! stricmp_u(LStr, "SharedOpts"))
						{
							// 2 bits
							TempLng = (UINT32)strtol(RStr, NULL, 0) & 0x03;
							TempCOpt->SpecialFlags &= ~(0x03 << 0) & 0x7FFF;
							TempCOpt->SpecialFlags |= TempLng << 0;
						}
						else if (! stricmp_u(LStr, "APUOpts"))
						{
							// 2 bits
							TempLng = (UINT32)strtol(RStr, NULL, 0) & 0x03;
							TempCOpt->SpecialFlags &= ~(0x03 << 2) & 0x7FFF;
							TempCOpt->SpecialFlags |= TempLng << 2;
						}
						else if (! stricmp_u(LStr, "DMCOpts"))
						{
							// 8 bits (6 bits used)
							TempLng = (UINT32)strtol(RStr, NULL, 0) & 0xFF;
							TempCOpt->SpecialFlags &= ~(0xFF << 4) & 0x7FFF;
							TempCOpt->SpecialFlags |= TempLng << 4;
						}
						else if (! stricmp_u(LStr, "FDSOpts"))
						{
							// 1 bit
							TempLng = (UINT32)strtol(RStr, NULL, 0) & 0x01;
							TempCOpt->SpecialFlags &= ~(0x01 << 12) & 0x7FFF;
							TempCOpt->SpecialFlags |= TempLng << 12;
						}
						break;
					case 0x17:	// OKIM6258
						if (! stricmp_u(LStr, "Enable10Bit"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 0);
							TempCOpt->SpecialFlags |= TempFlag << 0;
						}
						else if (! stricmp_u(LStr, "RemoveDCOfs"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 1);
							TempCOpt->SpecialFlags |= TempFlag << 1;
						}
						break;
					case 0x20:	// SCSP
						if (! stricmp_u(LStr, "BypassDSP"))
						{
							TempFlag = GetBoolFromStr(RStr);
							TempCOpt->SpecialFlags &= ~(0x01 << 0);
							TempCOpt->SpecialFlags |= TempFlag << 0;
						}
						break;
					}
				}
				break;
			case 0xFF:	// Dummy Section
				break;
			}
		}
	}
	
	TempCOpt = (CHIP_OPTS*)&ChipOpts[0x00];
	TempCOpt2 = (CHIP_OPTS*)&ChipOpts[0x01];
	for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++, TempCOpt ++, TempCOpt2 ++)
	{
		TempCOpt2->Disabled = TempCOpt->Disabled;
		TempCOpt2->EmuCore = TempCOpt->EmuCore;
		TempCOpt2->SpecialFlags = TempCOpt->SpecialFlags;
		TempCOpt2->ChnMute1 = TempCOpt->ChnMute1;
		TempCOpt2->ChnMute2 = TempCOpt->ChnMute2;
		TempCOpt2->ChnMute3 = TempCOpt->ChnMute3;
	}
	
	fclose(hFile);
	
#ifdef WIN32
	WinNT_Check();
#endif
	if (CHIP_SAMPLE_RATE <= 0)
		CHIP_SAMPLE_RATE = SampleRate;
	
	return;
}

static bool GetBoolFromStr(const char* TextStr)
{
	if (! stricmp_u(TextStr, "True"))
		return true;
	else if (! stricmp_u(TextStr, "False"))
		return false;
	else
		return strtol(TextStr, NULL, 0) ? true : false;
}

#if defined(XMAS_EXTRA) || defined(WS_DEMO)
static bool XMas_Extra(char* FileName, bool Mode)
{
	char* FileTitle;
	const UINT8* XMasData;
	UINT32 XMasSize;
	FILE* hFile;
	
	if (! Mode)
	{	// Prepare Mode
		FileTitle = NULL;
		XMasData = NULL;
#ifdef XMAS_EXTRA
		if (! stricmp_u(FileName, "WEWISH")
		{
			FileTitle = "WEWISH.CMF";
			XMasSize = sizeof(WEWISH_CMF);
			XMasData = WEWISH_CMF;
		}
		else if (! stricmp_u(FileName, "tim7")
		{
			FileTitle = "lem_tim7.vgz";
			XMasSize = sizeof(TIM7_VGZ);
			XMasData = TIM7_VGZ;
		}
		else if (! stricmp_u(FileName, "jingleb")
		{
			FileTitle = "lxmas_jb.dro";
			XMasSize = sizeof(JB_DRO);
			XMasData = JB_DRO;
		}
		else if (! stricmp_u(FileName, "rudolph")
		{
			FileTitle = "rudolph.dro";
			XMasSize = sizeof(RODOLPH_DRO);
			XMasData = RODOLPH_DRO;
		}
		else if (! stricmp_u(FileName, "clyde"))
		{
			FileTitle = "clyde1_1.dro";
			XMasSize = sizeof(clyde1_1_dro);
			XMasData = clyde1_1_dro;
		}
#elif defined(WS_DEMO)
		if (! stricmp_u(FileName, "wswan"))
		{
			FileTitle = "SWJ-SQRC01_1C.vgz";
			XMasSize = sizeof(FF1ws_1C);
			XMasData = FF1ws_1C;
		}
#endif
		
		if (XMasData)
		{
#ifdef WIN32
			GetEnvironmentVariable("Temp", FileName, MAX_PATH);
#else
			strcpy(FileName, "/tmp");
#endif
			strcat(FileName, DIR_STR);
			if (FileTitle == NULL)
				FileTitle = "XMas.dat";
			strcat(FileName, FileTitle);
			
			hFile = fopen(FileName, "wb");
			if (hFile == NULL)
			{
				FileName[0x00] = 0x00;
				printerr("Critical XMas-Error!\n");
				return false;
			}
			fwrite(XMasData, 0x01, XMasSize, hFile);
			fclose(hFile);
		}
		else
		{
			FileName = NULL;
			return false;
		}
	}
	else
	{	// Unprepare Mode
		if (! remove(FileName))
			return false;
		// btw: it's intentional that the user can grab the file from the temp-folder
	}
	
	return true;
}
#endif

#ifndef WIN32
static void ConvertCP1252toUTF8(char** DstStr, const char* SrcStr)
{
	const UINT16 CONV_TBL[0x20] =
	{	0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,		// 80-87
		0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,		// 88-8F
		0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,		// 90-97
		0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178};	// 98-9F
	UINT32 StrLen;
	UINT16 UnicodeChr;
	char* DstPtr;
	const char* SrcPtr;
	
	SrcPtr = SrcStr;
	StrLen = 0x00;
	while(*SrcPtr != '\0')
	{
		if (*SrcPtr < 0x80 || *SrcPtr >= 0xA0)
			UnicodeChr = *SrcPtr;
		else
			UnicodeChr = CONV_TBL[*SrcPtr - 0x80];
		if (UnicodeChr < 0x0080)
			StrLen ++;
		else if (UnicodeChr < 0x0800)
			StrLen += 2;
		else
			StrLen += 3;
		SrcPtr ++;
	}
	
	*DstStr = (char*)malloc((StrLen + 0x01) * sizeof(char));
	SrcPtr = SrcStr;
	DstPtr = *DstStr;
	while(*SrcPtr != '\0')
	{
		if (*SrcPtr < 0x80 || *SrcPtr >= 0xA0)
			UnicodeChr = (unsigned char)*SrcPtr;
		else
			UnicodeChr = CONV_TBL[*SrcPtr - 0x80];
		if (UnicodeChr < 0x0080)
		{
			*DstPtr = UnicodeChr & 0xFF;
			DstPtr ++;
		}
		else if (UnicodeChr < 0x0800)
		{
			DstPtr[0x00] = 0xC0 | ((UnicodeChr >> 6) & 0x1F);
			DstPtr[0x01] = 0x80 | ((UnicodeChr >> 0) & 0x3F);
			DstPtr += 0x02;
		}
		else
		{
			DstPtr[0x00] = 0xE0 | ((UnicodeChr >> 12) & 0x0F);
			DstPtr[0x01] = 0x80 | ((UnicodeChr >>  6) & 0x3F);
			DstPtr[0x02] = 0x80 | ((UnicodeChr >>  0) & 0x3F);
			DstPtr += 0x03;
		}
		SrcPtr ++;
	}
	*DstPtr = '\0';
	
	return;
}
#endif

static bool OpenPlayListFile(const char* FileName)
{
	const char M3UV2_HEAD[] = "#EXTM3U";
	const char M3UV2_META[] = "#EXTINF:";
	const UINT8 UTF8_SIG[] = {0xEF, 0xBB, 0xBF};
	UINT32 METASTR_LEN;
	size_t RetVal;
	
	FILE* hFile;
	UINT32 LineNo;
	bool IsV2Fmt;
	UINT32 PLAlloc;
	char TempStr[0x1000];	// 4096 chars should be enough
	char* RetStr;
	bool IsUTF8;
	
	hFile = fopen(FileName, "rt");
	if (hFile == NULL)
		return false;
	
	RetVal = fread(TempStr, 0x01, 0x03, hFile);
	if (RetVal >= 0x03)
		IsUTF8 = ! memcmp(TempStr, UTF8_SIG, 0x03);
	else
		IsUTF8 = false;
	
	rewind(hFile);
	
	PLAlloc = 0x0100;
	PLFileCount = 0x00;
	LineNo = 0x00;
	IsV2Fmt = false;
	METASTR_LEN = strlen(M3UV2_META);
	PlayListFile = (char**)malloc(PLAlloc * sizeof(char*));
	while(! feof(hFile))
	{
		RetStr = fgets(TempStr, 0x1000, hFile);
		if (RetStr == NULL)
			break;
		//RetStr = strchr(TempStr, 0x0D);
		//if (RetStr)
		//	*RetStr = 0x00;	// remove NewLine-Character
		RetStr = TempStr + strlen(TempStr) - 0x01;
		while(RetStr >= TempStr && *RetStr < 0x20)
		{
			*RetStr = 0x00;	// remove NewLine-Characters
			RetStr --;
		}
		if (! strlen(TempStr))
			continue;
		
		if (! LineNo)
		{
			if (! strcmp(TempStr, M3UV2_HEAD))
			{
				IsV2Fmt = true;
				LineNo ++;
				continue;
			}
		}
		if (IsV2Fmt)
		{
			if (! strncmp(TempStr, M3UV2_META, METASTR_LEN))
			{
				// Ignore Metadata of m3u Version 2
				LineNo ++;
				continue;
			}
		}
		
		if (PLFileCount >= PLAlloc)
		{
			PLAlloc += 0x0100;
			PlayListFile = (char**)realloc(PlayListFile, PLAlloc * sizeof(char*));
		}
		
		// TODO:
		//	- supprt UTF-8 m3us under Windows
		//	- force IsUTF8 via Commandline
#ifdef WIN32
		// Windows uses the 1252 Codepage by default
		PlayListFile[PLFileCount] = (char*)malloc((strlen(TempStr) + 0x01) * sizeof(char));
		strcpy(PlayListFile[PLFileCount], TempStr);
#else
		if (! IsUTF8)
		{
			// Most recent Linux versions use UTF-8, so I need to convert all strings.
			ConvertCP1252toUTF8(&PlayListFile[PLFileCount], TempStr);
		}
		else
		{
			PlayListFile[PLFileCount] = (char*)malloc((strlen(TempStr) + 0x01) * sizeof(char));
			strcpy(PlayListFile[PLFileCount], TempStr);
		}
#endif
		StandardizeDirSeparators(PlayListFile[PLFileCount]);
		PLFileCount ++;
		LineNo ++;
	}
	
	fclose(hFile);
	
	RetStr = GetLastDirSeparator(FileName);
	if (RetStr != NULL)
	{
		RetStr ++;
		strncpy(PLFileBase, FileName, RetStr - FileName);
		PLFileBase[RetStr - FileName] = '\0';
		StandardizeDirSeparators(PLFileBase);
	}
	else
	{
		strcpy(PLFileBase, "");
	}
	
	return true;
}

static bool OpenMusicFile(const char* FileName)
{
	if (OpenVGMFile(FileName))
		return true;
	else if (OpenOtherFile(FileName))
		return true;
	
	return false;
}

/*#ifdef WIN32
// "printc" initially meant "print correct, though "print console" would also make sense ;)
static void printc(const char* format, ...)
{
	int RetVal;
	UINT32 BufSize;
	char* printbuf;
	va_list arg_list;
	
	BufSize = 0x00;
	printbuf = NULL;
	do
	{
		BufSize += 0x100;
		printbuf = (char*)realloc(printbuf, BufSize);
		va_start(arg_list, format);
		RetVal = _vsnprintf(printbuf, BufSize - 0x01, format, arg_list);
		va_end(arg_list);
	} while(RetVal == -1 && BufSize < 0x1000);
	
	CharToOem(printbuf, printbuf);
	
	printf("%s", printbuf);
	
	free(printbuf);
	
	return;
}
#endif*/

static void wprintc(const wchar_t* format, ...)
{
	va_list arg_list;
	int RetVal;
	UINT32 BufSize;
	wchar_t* printbuf;
#ifdef WIN32
	UINT32 StrLen;
	char* oembuf;
	DWORD CPMode;
#endif
	
	BufSize = 0x00;
	printbuf = NULL;
	do
	{
		BufSize += 0x100;
		printbuf = (wchar_t*)realloc(printbuf, BufSize * sizeof(wchar_t));
		
		// Note: On Linux every vprintf call needs its own set of va_start/va_end commands.
		//       Under Windows (with VC6) one only one block for all calls works, too.
		va_start(arg_list, format);
		RetVal = _vsnwprintf(printbuf, BufSize - 0x01, format, arg_list);
		va_end(arg_list);
	} while(RetVal == -1 && BufSize < 0x1000);
#ifdef WIN32
	StrLen = wcslen(printbuf);
	
	// This is the only way to print Unicode stuff to the Windows console.
	// No, wprintf doesn't work.
	RetVal = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), printbuf, StrLen, &CPMode, NULL);
	if (! RetVal)	// call failed (e.g. with ERROR_CALL_NOT_IMPLEMENTED on Win95)
	{
		// fallback to printf with OEM codepage
		oembuf = (char*)malloc(BufSize);
		/*if (GetConsoleOutputCP() == GetOEMCP())
			CPMode = CP_OEMCP;
		else
			CPMode = CP_ACP;*/
		CPMode = GetConsoleOutputCP();
		WideCharToMultiByte(CPMode, 0x00, printbuf, StrLen + 1, oembuf, BufSize, NULL, NULL);
		
		printf("%s", oembuf);
		free(oembuf);
	}
#else
	printf("%ls", printbuf);
#endif
	
	free(printbuf);
	
	return;
}

static void PrintChipStr(UINT8 ChipID, UINT8 SubType, UINT32 Clock)
{
	if (! Clock)
		return;
	
	if (ChipID == 0x00 && (Clock & 0x80000000))
		Clock &= ~0x40000000;
	if (Clock & 0x80000000)
	{
		Clock &= ~0x80000000;
		ChipID |= 0x80;
	}
	
	if (Clock & 0x40000000)
		printf("2x");
	printf("%s, ", GetAccurateChipName(ChipID, SubType));
	
	return;
}

static const wchar_t* GetTagStrEJ(const wchar_t* EngTag, const wchar_t* JapTag)
{
	const wchar_t* RetTag;
	
	if (EngTag == NULL || ! wcslen(EngTag))
	{
		RetTag = JapTag;
	}
	else if (JapTag == NULL || ! wcslen(JapTag))
	{
		RetTag = EngTag;
	}
	else
	{
		if (! PreferJapTag)
			RetTag = EngTag;
		else
			RetTag = JapTag;
	}
	
	if (RetTag == NULL)
		return L"";
	else
		return RetTag;
}

static void ShowVGMTag(void)
{
	const wchar_t* TitleTag;
	const wchar_t* GameTag;
	const wchar_t* AuthorTag;
	const wchar_t* SystemTag;
	UINT8 CurChip;
	UINT32 ChpClk;
	UINT8 ChpType;
	INT16 VolMod;
#ifdef SET_CONSOLE_TITLE
	wchar_t TitleStr[0x80];
	UINT32 StrLen;
#endif
	
	TitleTag = GetTagStrEJ(VGMTag.strTrackNameE, VGMTag.strTrackNameJ);
	GameTag = GetTagStrEJ(VGMTag.strGameNameE, VGMTag.strGameNameJ);
	AuthorTag = GetTagStrEJ(VGMTag.strAuthorNameE, VGMTag.strAuthorNameJ);
	SystemTag = GetTagStrEJ(VGMTag.strSystemNameE, VGMTag.strSystemNameJ);
	
#ifdef SET_CONSOLE_TITLE
	// --- Show "Song (Game) - VGM Player" as Console Title ---
	if (! wcslen(TitleTag))
	{
		char* TempPtr1;
		char* TempPtr2;
		
		TempPtr1 = strrchr(VgmFileName, '\\');
		TempPtr2 = strrchr(VgmFileName, '/');
		if (TempPtr1 < TempPtr2)
			TempPtr1 = TempPtr2;
		if (TempPtr1 == NULL)
			TempPtr1 = VgmFileName;
		else
			TempPtr1 ++;
		//strncpy(TitleStr, TempPtr1, 0x70);
		mbstowcs(TitleStr, TempPtr1, 0x7F);
		TitleStr[0x70] = '\0';
	}
	else
	{
#if (defined(_MSC_VER) && _MSC_VER < 1400)// || defined(__MINGW32__)
		swprintf(TitleStr, L"%.*ls", 0x70, TitleTag);
#else
		swprintf(TitleStr, 0x80, L"%.*ls", 0x70, TitleTag);
#endif
	}
	StrLen = wcslen(TitleStr);
	
	if (wcslen(GameTag) && StrLen < 0x6C)
	{
#if (defined(_MSC_VER) && _MSC_VER < 1400)// || defined(__MINGW32__)
		swprintf(TitleStr + StrLen, L" (%.*ls)", 0x70 - 3 - StrLen, GameTag);
#else
		swprintf(TitleStr + StrLen, 0x80, L" (%.*ls)", 0x70 - 3 - StrLen, GameTag);
#endif
		StrLen = wcslen(TitleStr);
	}
	
	wcscat(TitleStr, L" - " APP_NAME_L);
#ifdef WIN32
	SetConsoleTitleW(TitleStr);			// Set Windows Console Title
#else
	printf("\x1B]0;%ls\x07", TitleStr);	// Set xterm/rxvt Terminal Title
#endif
#endif
	
	// --- Display Tag Data ---
	if (VGMHead.bytVolumeModifier <= VOLUME_MODIF_WRAP)
		VolMod = VGMHead.bytVolumeModifier;
	else if (VGMHead.bytVolumeModifier == (VOLUME_MODIF_WRAP + 0x01))
		VolMod = VOLUME_MODIF_WRAP - 0x100;
	else
		VolMod = VGMHead.bytVolumeModifier - 0x100;
	
	wprintc(L"Track Title:\t%ls\n", TitleTag);
	wprintc(L"Game Name:\t%ls\n", GameTag);
	wprintc(L"System:\t\t%ls\n", SystemTag);
	wprintc(L"Composer:\t%ls\n", AuthorTag);
	wprintc(L"Release:\t%ls\n", VGMTag.strReleaseDate);
	printf("Version:\t%X.%02X\t", VGMHead.lngVersion >> 8, VGMHead.lngVersion & 0xFF);
	printf("  Gain:%5.2f\t", pow(2.0, VolMod / (double)0x20));
	printf("Loop: ");
	if (VGMHead.lngLoopOffset)
	{
		UINT32 PbRateMul;
		UINT32 PbRateDiv;
		UINT32 PbSamples;
		
		// calculate samples for correct display with changed playback rate
		if (! VGMPbRate || ! VGMHead.lngRate)
		{
			PbRateMul = 1;
			PbRateDiv = 1;
		}
		else
		{
			PbRateMul = VGMHead.lngRate;
			PbRateDiv = VGMPbRate;
		}
		PbSamples = (UINT32)((UINT64)VGMHead.lngLoopSamples * PbRateMul / PbRateDiv);
		
		printf("Yes (");
		PrintMinSec(PbSamples, VGMSampleRate);
		printf(")\n");
	}
	else
	{
		printf("No\n");
	}
	wprintc(L"VGM by:\t\t%ls\n", VGMTag.strCreator);
	wprintc(L"Notes:\t\t%ls\n", VGMTag.strNotes);
	printf("\n");
	
	printf("Used chips:\t");
	for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++)
	{
		ChpClk = GetChipClock(&VGMHead, CurChip, &ChpType);
		if (ChpClk && GetChipClock(&VGMHead, 0x80 | CurChip, NULL))
			ChpClk |= 0x40000000;
		PrintChipStr(CurChip, ChpType, ChpClk);
	}
	printf("\b\b \n");
	printf("\n");
	
	return;
}


#define LOG_SAMPLES	(SampleRate / 5)
static void PlayVGM_UI(void)
{
	INT32 VGMPbSmplCount;
	INT32 PlaySmpl;
	UINT8 KeyCode;
	UINT32 VGMPlaySt;
	UINT32 VGMPlayEnd;
	char WavFileName[MAX_PATH];
	char* TempStr;
	WAVE_16BS* TempBuf;
	UINT8 RetVal;
	UINT32 TempLng;
	bool PosPrint;
	bool LastUninit;
	bool QuitPlay;
	UINT32 PlayTimeEnd;
	
	printf("Initializing ...\r");
	
	PlayVGM();
	
	/*switch(LogToWave)
	{
	case 0x00:
		break;
	case 0x01:
		// Currently there's no record for Hardware FM
		PlayingMode = 0x00;	// Impossible to log at full speed AND use FMPort
		break;
	case 0x02:
		if (PlayingMode == 0x01)
			LogToWave = 0x00;	// Output and log sound (FM isn't logged)
		break;
	}*/
	switch(PlayingMode)
	{
	case 0x00:
		AUDIOBUFFERU = 10;
		break;
	case 0x01:
		AUDIOBUFFERU = 0;	// no AudioBuffers needed
		break;
	case 0x02:
		AUDIOBUFFERU = 5;	// try to sync Hardware/Software Emulator as well as possible
		break;
	}
	if (AUDIOBUFFERU < NEED_LARGE_AUDIOBUFS)
		AUDIOBUFFERU = NEED_LARGE_AUDIOBUFS;
	if (ForceAudioBuf && AUDIOBUFFERU)
		AUDIOBUFFERU = ForceAudioBuf;
	
	switch(FileMode)
	{
	case 0x00:	// VGM
		// RAW Log: no loop, no Creator, System Name set
		IsRAWLog = (! VGMHead.lngLoopOffset && ! wcslen(VGMTag.strCreator) &&
					(wcslen(VGMTag.strSystemNameE) || wcslen(VGMTag.strSystemNameJ)));
		break;
	case 0x01:	// CMF
		IsRAWLog = false;
		break;
	case 0x02:	// DRO
		IsRAWLog = true;
		break;
	}
	if (! VGMHead.lngTotalSamples)
		IsRAWLog = false;
	
#ifndef WIN32
	changemode(true);
#endif
	
	switch(PlayingMode)
	{
	case 0x00:
	case 0x02:
		if (LogToWave)
		{
			strcpy(WavFileName, VgmFileName);
			TempStr = GetFileExtention(WavFileName);
			if (TempStr == NULL)
				TempStr = WavFileName + strlen(WavFileName);
			else
				TempStr --;
			strcpy(TempStr, ".wav");
			
			strcpy(SoundLogFile, WavFileName);
		}
		//FullBufFill = ! LogToWave;
		
		switch(LogToWave)
		{
		case 0x00:
		case 0x02:
			SoundLogging(LogToWave ? true : false);
			if (FirstInit || ! StreamStarted)
			{
				// support smooth transistions between songs
				RetVal = StartStream(0x00);
				if (RetVal)
				{
					printf("Error openning Sound Device!\n");
					return;
				}
				StreamStarted = true;
			}
			PauseStream(PausePlay);
			break;
		case 0x01:
			TempBuf = (WAVE_16BS*)malloc(SAMPLESIZE * LOG_SAMPLES);
			if (TempBuf == NULL)
			{
				printf("Allocation Error!\n");
				return;
			}
			
			StartStream(0xFF);
			RetVal = SaveFile(0x00000000, NULL);
			if (RetVal)
			{
				printf("Can't open %s!\n", SoundLogFile);
				return;
			}
			break;
		}
		break;
	case 0x01:
		// PlayVGM() does it all
		//FullBufFill = true;
		break;
	}
	FirstInit = false;
	
	VGMPlaySt = VGMPos;
	if (VGMHead.lngGD3Offset)
		VGMPlayEnd = VGMHead.lngGD3Offset;
	else
		VGMPlayEnd = VGMHead.lngEOFOffset;
	VGMPlayEnd -= VGMPlaySt;
	if (! FileMode)
		VGMPlayEnd --;	// EOF Command doesn't count
	PosPrint = true;
	
	PlayTimeEnd = 0;
	QuitPlay = false;
	while(! QuitPlay)
	{
		if (! PausePlay || PosPrint)
		{
			PosPrint = false;
			
			VGMPbSmplCount = SampleVGM2Playback(VGMHead.lngTotalSamples);
			PlaySmpl = VGMPos - VGMPlaySt;
#ifdef WIN32
			printf("Playing %01.2f%%\t", 100.0 * PlaySmpl / VGMPlayEnd);
#else
			// \t doesn't display correctly under Linux
			// but \b causes flickering under Windows
			printf("Playing %01.2f%%   \b\b\b\t", 100.0 * PlaySmpl / VGMPlayEnd);
#endif
			if (LogToWave != 0x01)
			{
				PlaySmpl = (BlocksSent - BlocksPlayed) * SMPL_P_BUFFER;
				PlaySmpl = VGMSmplPlayed - PlaySmpl;
			}
			else
			{
				PlaySmpl = VGMSmplPlayed;
			}
			if (! VGMCurLoop)
			{
				if (PlaySmpl < 0)
					PlaySmpl = 0;
			}
			else
			{
				while(PlaySmpl < SampleVGM2Playback(VGMHead.lngTotalSamples -
					VGMHead.lngLoopSamples))
					PlaySmpl += SampleVGM2Playback(VGMHead.lngLoopSamples);
			}
			//if (PlaySmpl > VGMPbSmplCount)
			//	PlaySmpl = VGMPbSmplCount;
			PrintMinSec(PlaySmpl, SampleRate);
			printf(" / ");
			PrintMinSec(VGMPbSmplCount, SampleRate);
			printf(" seconds");
			if (Show95Cmds && Last95Max != 0xFFFF)
			{
				if (Show95Cmds == 0x01)
					printf("  %02hX / %02hX", 1 + Last95Drum, Last95Max);
				else if (Show95Cmds == 0x02)
					printf("  %02hX / %02hX at %5u Hz", 1 + Last95Drum, Last95Max, Last95Freq);
				else if (Show95Cmds == 0x03)
					printf("  %02hX / %02hX at %4.1f KHz", 1 + Last95Drum, Last95Max,
							Last95Freq / 1000.0);
			}
			//printf("  %u / %u", multipcm_get_channels(0, NULL), 28);
			printf("\r");
#ifndef WIN32
			fflush(stdout);
#endif
			
			if (LogToWave == 0x01 && ! PausePlay)
			{
				TempLng = FillBuffer(TempBuf, LOG_SAMPLES);
				if (TempLng)
					SaveFile(TempLng, TempBuf);
				if (EndPlay)
					break;
			}
			else
			{
#ifdef WIN32
				Sleep(50);
#endif
			}
		}
		else
		{
#ifdef WIN32
			Sleep(1);
#endif
		}
#ifndef WIN32
		if (! PausePlay)
			WaveOutLinuxCallBack();
		else
			Sleep(100);
#endif
		
		if (EndPlay)
		{
			if (! PlayTimeEnd)
			{
				PlayTimeEnd = PlayingTime;
				// quitting now terminates the program, so I need some special
				// checks to make sure that the rest of the audio buffer is played
				if (! PLFileCount || CurPLFile >= PLFileCount - 0x01)
				{
					if (FileMode == 0x01)
						PlayTimeEnd += SampleRate << 1;	// Add 2 secs
					PlayTimeEnd += AUDIOBUFFERU * SMPL_P_BUFFER;
				}
			}
			
			if (PlayingTime >= PlayTimeEnd)
				QuitPlay = true;
		}
		if (_kbhit())
		{
			KeyCode = _getch();
			if (KeyCode < 0x80)
				KeyCode = toupper(KeyCode);
			switch(KeyCode)
			{
#ifndef WIN32
			case 0x1B:	// Special Key
				KeyCode = _getch();
				if (KeyCode == 0x1B || KeyCode == 0x00)
				{
					// ESC Key pressed
					QuitPlay = true;
					NextPLCmd = 0xFF;
					break;
				}
				switch(KeyCode)
				{
				case 0x5B:
					// Cursor-Key Table
					//	Key		KeyCode
					//	Up		41
					//	Down	42
					//	Left	44
					//	Right	43
					// Cursor only: CursorKey
					// Ctrl: 0x31 + 0x3B + 0x35 + CursorKey
					// Alt: 0x31 + 0x3B + 0x33 + CursorKey
					
					// Page-Keys: PageKey + 0x7E
					//	PageUp		35
					//	PageDown	36
					KeyCode = _getch();	// Get 2nd Key
					// Convert Cursor Key Code from Linux to Windows
					switch(KeyCode)
					{
					case 0x31:	// Ctrl or Alt key
						KeyCode = _getch();
						if (KeyCode == 0x3B)
						{
							KeyCode = _getch();
							if (KeyCode == 0x35)
							{
								KeyCode = _getch();
								switch(KeyCode)
								{
								case 0x41:
									KeyCode = 0x8D;
									break;
								case 0x42:
									KeyCode = 0x91;
									break;
								case 0x43:
									KeyCode = 0x74;
									break;
								case 0x44:
									KeyCode = 0x73;
									break;
								default:
									KeyCode = 0x00;
									break;
								}
							}
						}
						
						if ((KeyCode & 0xF0) == 0x30)
							KeyCode = 0x00;
						break;
					case 0x35:
						KeyCode = 0x49;
						_getch();
						break;
					case 0x36:
						KeyCode = 0x51;
						_getch();
						break;
					case 0x41:
						KeyCode = 0x48;
						break;
					case 0x42:
						KeyCode = 0x50;
						break;
					case 0x43:
						KeyCode = 0x4D;
						break;
					case 0x44:
						KeyCode = 0x4B;
						break;
					default:
						KeyCode = 0x00;
						break;
					}
				}
				// At this point I have Windows-style keys.
#else	//#ifdef WIN32
			case 0xE0:	// Special Key
				// Cursor-Key Table
				// Shift + Cursor results in the usual value for the Cursor Key
				// Alt + Cursor results in 0x00 + (0x50 + CursorKey) (0x00 instead of 0xE0)
				//	Key		None	Ctrl
				//	Up		48		8D
				//	Down	50		91
				//	Left	4B		73
				//	Right	4D		74
				KeyCode = _getch();	// Get 2nd Key
#endif
				switch(KeyCode)
				{
				case 0x4B:	// Cursor Left
					PlaySmpl = -5;
					break;
				case 0x4D:	// Cursor Right
					PlaySmpl = 5;
					break;
				case 0x73:	// Ctrl + Cursor Left
					PlaySmpl = -60;
					break;
				case 0x74:	// Ctrl + Cursor Right
					PlaySmpl = 60;
					break;
				case 0x49:	// Page Up
					if (PLFileCount && /*! NextPLCmd &&*/ CurPLFile)
					{
						NextPLCmd = 0x01;
						QuitPlay = true;
					}
					PlaySmpl = 0;
					break;
				case 0x51:	// Page Down
					if (PLFileCount && /*! NextPLCmd &&*/ CurPLFile < PLFileCount - 0x01)
					{
						NextPLCmd = 0x00;
						QuitPlay = true;
					}
					PlaySmpl = 0;
					break;
				default:
					PlaySmpl = 0;
					break;
				}
				if (PlaySmpl)
				{
					SeekVGM(true, PlaySmpl * SampleRate);
					PosPrint = true;
				}
				break;
#ifdef WIN32
			case 0x1B:	// ESC
#endif
			case 'Q':
				QuitPlay = true;
				NextPLCmd = 0xFF;
				break;
			case ' ':
				PauseVGM(! PausePlay);
				PosPrint = true;
				break;
			case 'F':	// Fading
				FadeTime = FadeTimeN;
				FadePlay = true;
				break;
			case 'R':	// Restart
				RestartVGM();
				PosPrint = true;
				break;
			case 'B':	// Previous file (Back)
				if (PLFileCount && /*! NextPLCmd &&*/ CurPLFile)
				{
					NextPLCmd = 0x01;
					QuitPlay = true;
				}
				break;
			case 'N':	// Next file
				if (PLFileCount && /*! NextPLCmd &&*/ CurPLFile < PLFileCount - 0x01)
				{
					NextPLCmd = 0x00;
					QuitPlay = true;
				}
				break;
			}
		}
		
		/*if (! PauseThread && FadePlay && (! FadeTime || MasterVol == 0.0f))
		{
			QuitPlay = true;
		}*/
		if (FadeRAWLog && IsRAWLog && ! PausePlay && ! FadePlay && FadeTimeN)
		{
			PlaySmpl = (INT32)VGMHead.lngTotalSamples -
						FadeTimeN * VGMSampleRate / 1500;
			if (VGMSmplPos >= PlaySmpl)
			{
				FadeTime = FadeTimeN;
				FadePlay = true;	// (FadeTime / 1500) ends at 33%
			}
		}
	}
	ThreadNoWait = false;
	
	// Last Uninit: ESC pressed, no playlist, last file in playlist
	LastUninit = (NextPLCmd & 0x80) || ! PLFileCount ||
				(NextPLCmd == 0x00 && CurPLFile >= PLFileCount - 0x01);
	switch(PlayingMode)
	{
	case 0x00:
		switch(LogToWave)
		{
		case 0x00:
		case 0x02:
			if (LastUninit)
			{
				StopStream();
				StreamStarted = false;
			}
			else
			{
				if (ThreadPauseEnable)
				{
					ThreadPauseConfrm = false;
					PauseThread = true;
					while(! ThreadPauseConfrm)
						Sleep(1);	// Wait until the Thread is finished
				}
				else
				{
					PauseThread = true;
				}
				if (LogToWave)
					SaveFile(0xFFFFFFFF, NULL);
			}
			break;
		case 0x01:
			SaveFile(0xFFFFFFFF, NULL);
			break;
		}
		break;
	case 0x01:
		if (StreamStarted)
		{
			StopStream();
			StreamStarted = false;
		}
		break;
	case 0x02:
		if (LastUninit)
		{
			StopStream();
			StreamStarted = false;
#ifdef MIXER_MUTING
#ifdef WIN32
			mixerClose(hmixer);
#else
			close(hmixer);
#endif
#endif
		}
		else
		{
			if (ThreadPauseEnable)
			{
				ThreadPauseConfrm = false;
				PauseThread = true;
				while(! ThreadPauseConfrm)
					Sleep(1);	// Wait until the Thread is finished
				PauseStream(true);
			}
			else
			{
				PauseThread = true;
			}
		}
		break;
	}
#ifndef WIN32
	changemode(false);
#endif
	
	StopVGM();
	
	printf("\nPlaying finished.\n");
	
	return;
}

INLINE INT8 sign(double Value)
{
	if (Value > 0.0)
		return 1;
	else if (Value < 0.0)
		return -1;
	else
		return 0;
}

INLINE long int Round(double Value)
{
	// Alternative:	(fabs(Value) + 0.5) * sign(Value);
	return (long int)(Value + 0.5 * sign(Value));
}

INLINE double RoundSpecial(double Value, double RoundTo)
{
	return (long int)(Value / RoundTo + 0.5 * sign(Value)) * RoundTo;
}

static void PrintMinSec(UINT32 SamplePos, UINT32 SmplRate)
{
	float TimeSec;
	UINT16 TimeMin;
	UINT16 TimeHours;
	
	TimeSec = (float)RoundSpecial(SamplePos / (double)SmplRate, 0.01);
	//TimeSec = SamplePos / (float)SmplRate;
	TimeMin = (UINT16)TimeSec / 60;
	TimeSec -= TimeMin * 60;
	if (! PrintMSHours)
	{
		printf("%02hu:%05.2f", TimeMin, TimeSec);
	}
	else
	{
		TimeHours = TimeMin / 60;
		TimeMin %= 60;
		printf("%hu:%02hu:%05.2f", TimeHours, TimeMin, TimeSec);
	}
	
	return;
}
