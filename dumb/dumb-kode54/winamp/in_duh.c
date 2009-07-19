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
 * in_duh.c - Winamp plug-in for DUMB.                / / \  \
 *                                                   | <  /   \_
 * By Bob.                                           |  \/ /\   /
 *                                                    \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */


//#define SOFTVOLUME

#include "in_duh.h"
#include "resource.h"
#include "gui.h"


typedef struct DUH_PLAYER
{
	int n_channels;
	DUH_SIGRENDERER *dr;
	float volume;
}
DUH_PLAYER;


/* Winamp Output module; we write to this to tell Winamp what to play */
In_Module mod;

/* Currently playing file */
DUH *duh;
DUH_PLAYER *duh_player;
int init_duh = TRUE;
char *duh_filename = NULL;
#ifdef SOFTVOLUME
int thevolume = 255;
#endif



/******************
 *  Configuration */
static int bits_per_sample;
static int frequency;
static int stereo;
static int resampling;
static int buffer_size;
static int thread_priority;



/****************
 * Winamp Stuff */

HANDLE input_file = INVALID_HANDLE_VALUE;    /* input file handle */

int killDecodeThread = 0;                    /* the kill switch for the decode thread */
HANDLE thread_handle = INVALID_HANDLE_VALUE; /* the handle to the decode thread */

DWORD WINAPI __stdcall DecodeThread(void *b); /* the decode thread procedure */

/* Avoid CRT. Evil. Big. Bloated. */
BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	(void)hInst;
	(void)ul_reason_for_call;
	(void)lpReserved;
	return TRUE;
}


/* Post this to the main window at end of file (after playback as stopped) */
#define WM_WA_MPEG_EOF (WM_USER+2)



/* Stuff for interfacing with Winamp */
int decode_pos_ms;
int paused;
int seek_needed;      /* if != -1, it is the point that the decode thread should seek to, in ms. */
//char *sample_buffer = NULL;

//int buffer_pos = 0;



/* Init DUH */
void init()
{
	config_init();
	dumb_register_stdfiles();
}



/* De-Init */
void quit()
{
	config_quit();

	if (duh_player)
		free(duh_player);
	if (duh)
		unload_duh(duh);

	if (duh_filename)
		free(duh_filename);

	//if (sample_buffer)
		//free(sample_buffer);

	dumb_exit();
}



/* WA SDK: used for detecting URL streams.. unused here. strncmp(fn,"http://",7) to detect HTTP streams, etc */
/* I -think- we need to tell Winamp that the file should use our plug-in */
int isourfile(char *fn) { (void)fn; return 0; }

void stop_duh(DUH_PLAYER *dp)
{
	if (dp) {
		duh_end_sigrenderer(dp->dr);
		free(dp);
	}
}


int play(char *fn) 
{ 
	static int priority_table[] = {
		THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST
	};
	int maxlatency;
	unsigned long thread_id;

	/* Get rid of an old DUH */
	if (duh_player) {
		stop_duh(duh_player);
		duh_player = NULL;
	}

	if (duh)
		unload_duh(duh);

	/* Load file */
	duh = load_duh(fn);
	if (!duh) {
		duh = dumb_load_it(fn);
		if (!duh) {
			duh = dumb_load_xm(fn);
			if (!duh) {
				duh = dumb_load_s3m(fn);
				if (!duh) {
					duh = dumb_load_mod(fn);
					if (!duh)
						return 1;
				}
			}
		}
	}

	init_duh = TRUE;

	/* Set up some things for Winamp */
	paused = 0;
	decode_pos_ms = 0;
	seek_needed = -1;

	bits_per_sample = config_bits_per_sample;
	frequency       = config_frequency;
	stereo          = config_stereo;
	resampling      = config_resampling;
	buffer_size     = config_buffer_size;
	thread_priority = priority_table[config_thread_priority];

	/* Create the sample buffer */
	//if (sample_buffer)
		//free(sample_buffer);

	//sample_buffer = malloc(((bits_per_sample + 7) / 8) * stereo * buffer_size);

	//if (!sample_buffer)
		//return 1;

	//buffer_pos = 0;

	/* Note: I -really- don't know what this does. Winamp's SDK doesn't mention this function... */
	maxlatency = mod.outMod->Open(frequency, stereo, bits_per_sample, -1, -1);
	if (maxlatency < 0) {  /* error opening device */
		unload_duh(duh);
		duh = NULL;
		return 1;
	}

	/* Store the file name */
	if (duh_filename)
		free(duh_filename);
	duh_filename = strdup(fn);

	/* Note2: Dunno what does too; damn those Winamp docs */
	/* dividing by 1000 for the first parameter of setinfo makes it */
	/* display 'H'... for hundred.. i.e. 14H Kbps. */
	mod.SetInfo((frequency * bits_per_sample * stereo) / 1000,
		frequency / 1000, stereo, 1);

	/* Ditto */
	/* initialize vis stuff */
	mod.SAVSAInit(maxlatency, frequency);
	mod.VSASetInfo(frequency, stereo);

	/* Ditthree */
#ifdef SOFTVOLUME
	mod.outMod->SetVolume(255);
#else
	mod.outMod->SetVolume(-666); /* set the output plug-ins default volume */
#endif

	/* Ok, now we set up the decoding thread */
	killDecodeThread = 0;
	thread_handle = (HANDLE) CreateThread(NULL,0,DecodeThread,&killDecodeThread,0,&thread_id);
	SetThreadPriority(thread_handle, thread_priority);

	return 0; 
}



/* Standard Winamp stuff */
void pause()   { paused = 1;   mod.outMod->Pause(1); }
void unpause() { paused = 0;   mod.outMod->Pause(0); }
int ispaused() { return paused; }



/* Stop playing the file */
void stop()
{
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killDecodeThread=1;
		if (WaitForSingleObject(thread_handle,INFINITE) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,"Error asking thread to die!\n","Error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}
	if (duh_player) {
		stop_duh(duh_player);
		duh_player = NULL;
	}

	if (duh) {
		unload_duh(duh);
		duh = NULL;
	}

	/* Should I unload the file? It takes time to reload... */

	mod.outMod->Close();

	mod.SAVSADeInit();
}



int getlength()
{
	return (int) ((LONG_LONG)duh_get_length(duh) * 1000 >> 16);
}



int getoutputtime()
{
	return decode_pos_ms + (mod.outMod->GetOutputTime() - mod.outMod->GetWrittenTime());
}



void setoutputtime(int time_in_ms)
{
	seek_needed = time_in_ms;
}



void setvolume(int volume)
{
#ifdef SOFTVOLUME
	thevolume = volume;
	if (duh_player) duh_player->volume = volume / 255.0f;
#else
	mod.outMod->SetVolume(volume);
#endif
}



void setpan(int pan) { mod.outMod->SetPan(pan); }



int infoDlg(char *fn, HWND hwnd)
{
	(void)fn;
	(void)hwnd;
	// TODO: implement info dialog.
	return 0;
}



static const char *fn_basename(const char *filename)
{
	for (;;) {
		const char *p = strpbrk(filename, "/\\");
		if (!p) return filename;
		filename = p + 1;
	}
}



void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	if (!filename || !*filename) { /* currently playing file */

		if (length_in_ms)
			*length_in_ms = getlength();

		if (title) {
			const char *mod_title = duh_get_tag(duh, "TITLE");
			if (mod_title && mod_title[0])
				sprintf(title, "%s - %s", fn_basename(filename), mod_title);
			else
				strcpy(title, fn_basename(filename));
		}
	}
	else { /* some other file */
#if 1 // needs fixing better than this! more to add to DUMB's API?
		if (length_in_ms || title) {
			DUH *duh = load_duh(filename);
			if (!duh) {
				duh = dumb_load_it(filename);
				if (!duh) {
					duh = dumb_load_xm(filename);
					if (!duh) {
						duh = dumb_load_s3m(filename);
						if (!duh) {
							duh = dumb_load_mod(filename);
							if (!duh)
								return;
						}
					}
				}
			}

			if (length_in_ms)
				*length_in_ms = (int)((LONG_LONG)duh_get_length(duh) * 1000 >> 16);

			if (title) {
				const char *mod_title = duh_get_tag(duh, "TITLE");
				if (mod_title && mod_title[0])
					sprintf(title, "%s - %s", fn_basename(duh_filename), mod_title);
				else
					strcpy(title, fn_basename(duh_filename));
			}

			unload_duh(duh);
		}
#elif 0
		/* This code only works (worked?) for DUH files. */
		DUMBFILE *d = dumbfile_open(filename);

		if (!d)
			return;

		if (dumbfile_mgetl(d) != DUH_SIGNATURE)
			return;

		*length_in_ms = (unsigned int)((LONG_LONG)dumbfile_igetl(d) * 1000 >> 16);

		if (title)
			strcpy(title, filename);

		dumbfile_close(d);
#else
		*length_in_ms = 1000 * 60 * 10;

		if (title)
			strcpy(title, filename);
#endif
	}
}



void eq_set(int on, char data[10], int preamp)
{
	(void)on;
	(void)data;
	(void)preamp;
	/* No equalizer support, sorry */
}



DUH_PLAYER *start_duh(DUH *duh, int n_channels, long pos, float volume)
{
	DUH_PLAYER *dp;

	// This restriction is imposed by Allegro. <-- um...?
	ASSERT(n_channels > 0);
	ASSERT(n_channels <= 2);

	if (!duh)
		return NULL;

	dp = malloc(sizeof(*dp));
	if (!dp)
		return NULL;

	dp->n_channels = n_channels;

	dp->dr = duh_start_sigrenderer(duh, 0, n_channels, pos);

	if (!dp->dr) {
		free(dp);
		return NULL;
	}

	{
		DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(dp->dr);
		dumb_it_set_loop_callback(itsr, &dumb_it_callback_terminate, NULL);
		dumb_it_set_xm_speed_zero_callback(itsr, &dumb_it_callback_terminate, NULL);
	}

	dp->volume = volume;

	return dp;
}




void pause_duh(DUH_PLAYER *dp)
{
	(void)dp;
	pause();
}



void resume_duh(DUH_PLAYER *dp)
{
	(void)dp;
	unpause();
}



void set_duh_volume(DUH_PLAYER *dp, float volume)
{
	(void)dp;
	setvolume((int)(volume * 255.0f));
}




/* Generate 576 samples of data from the DUH_PLAYER */
int get_576_samples(DUH_PLAYER *dp, char *buf)
{
#if 1
	if (!dp) return 0;

	long n = duh_render(dp->dr, bits_per_sample, bits_per_sample == 8 ? 1 : 0,
		dp->volume, 65536.0f / frequency, 576, buf);

	return n * (bits_per_sample >> 3) * stereo;
#else
	long n;
	int bps = ((bits_per_sample + 7) / 8) * stereo;

	if (!dp)
		return 0;

	if (buffer_pos == 0 || buffer_pos + 576 >= buffer_size) {

		if (buffer_pos) {
			memmove(sample_buffer, sample_buffer + bps * buffer_pos, (buffer_size - buffer_pos) * bps);
			buffer_pos = buffer_size - buffer_pos;
		}

		n = duh_render(dp->dr, bits_per_sample, bits_per_sample == 8 ? 1 : 0,
			dp->volume, 65536.0f / frequency, buffer_size - buffer_pos,
			sample_buffer + buffer_pos * bps);

		if (n > 576) n = 576;

		n *= bps;
		buffer_pos = 0;
	}

	memcpy(buf, sample_buffer + buffer_pos * bps, 576 * bps);

	buffer_pos += 576;

	return 576 * bps;
#endif
}


DWORD WINAPI __stdcall DecodeThread(void *b)
{
	static char buf[576 * 4];
	int done = 0;
	int length = 0;

	if (init_duh) {
		dumb_resampling_quality = resampling;
#ifdef SOFTVOLUME
		duh_player = start_duh(duh, stereo, 0, thevolume / 255.0f);
#else
		duh_player = start_duh(duh, stereo, 0, 1.0f);
#endif
		init_duh = FALSE;
	}
	length = getlength();


	while (! *((int *)b) )
	{
		if (seek_needed != -1) {

			decode_pos_ms = seek_needed-(seek_needed%1000);
			seek_needed = -1;
			done = 0;

			mod.outMod->Flush(decode_pos_ms);

			/* Position the playback pointer */
			stop_duh(duh_player);
			duh_player = start_duh(duh, stereo, (unsigned int)(decode_pos_ms * 65536.0 / 1000.0), 1.0);
		}
		if (done) {
			mod.outMod->CanWrite();

			if (!mod.outMod->IsPlaying()) {
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				return 0;
			}
			Sleep(10);
		}
		else if (mod.outMod->CanWrite() >= ((576 * stereo * ((bits_per_sample + 7) / 8)))) {

			int l = get_576_samples(duh_player, buf);

			if (!l || decode_pos_ms >= length) {
				done = 1;
			}
			else {
				/* Vis plug-ins interface */
				mod.SAAddPCMData((char *)buf,  stereo, bits_per_sample, decode_pos_ms);
				mod.VSAAddPCMData((char *)buf, stereo, bits_per_sample, decode_pos_ms);

				/* Add PCM to output buffer */
				decode_pos_ms += (576 * 1000) / frequency;

				if (mod.dsp_isactive())
					l = mod.dsp_dosamples((short *)buf, l / stereo / ((bits_per_sample + 7) / 8), bits_per_sample,
						stereo, frequency) * (stereo * ((bits_per_sample + 7) / 8));

				mod.outMod->Write(buf, l);
			}
		}
		else /* Nothing to do this pass */
			Sleep(config_frequency / 1000);
	}
	return 0;
}



In_Module mod =
{
	IN_VER,
	"DUH! Player v" VERSION
#ifdef __alpha
	" (AXP)"
#else
	" (x86)"
#endif
	,
	0,  /* hMainWindow */
	0,  /* hDllInstance */
	"DUH\0Dynamic Universal Harmony File (*.DUH)\0"
	"IT\0Impulse Tracker Module (*.IT)\0"
	"XM\0Fast Tracker 2 Module (*.XM)\0"
	"S3M\0Scream Tracker 3 Module (*.S3M)\0"
	"MOD\0Amiga Module (*.MOD)\0"
	,
	1,  /* is_seekable */
	1,  /* uses output */
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,

	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, /* vis stuff */


	0,0, /* dsp */

	eq_set,

	NULL,		/* setinfo */

	0 /* out_mod */

};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}
