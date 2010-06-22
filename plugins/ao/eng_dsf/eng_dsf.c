//
// Audio Overload
// Emulated music player
//
// (C) 2000-2008 Richard F. Bannister
//

//
// eng_dsf.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ao.h"
#include "eng_protos.h"
#include "corlett.h"
#include "dc_hw.h"
#include "aica.h"

#define DEBUG_LOADER	(0)
#define DK_CORE	(1)

#if DK_CORE
#include "arm7.h"
#else
#include "arm7core.h"
#endif

static corlett_t	*c = NULL;
static char 		psfby[256];
static uint32		decaybegin, decayend, total_samples;

void *aica_start(const void *config);
void AICA_Update(void *param, INT16 **inputs, INT16 **buf, int samples);

int32 dsf_start(uint8 *buffer, uint32 length)
{
	uint8 *file, *lib_decoded, *lib_raw_file;
	uint32 offset, plength, lengthMS, fadeMS;
	uint64 file_len, lib_len, lib_raw_length;
	corlett_t *lib;
	char *libfile;
	int i;

	// clear Dreamcast work RAM before we start scribbling in it
	memset(dc_ram, 0, 8*1024*1024);

	// Decode the current SSF
	if (corlett_decode(buffer, length, &file, &file_len, &c) != AO_SUCCESS)
	{
		return AO_FAIL;
	}

	#if DEBUG_LOADER
	printf("%d bytes decoded\n", file_len);
	#endif

	// Get the library file, if any
	for (i=0; i<9; i++) {
		libfile = i ? c->libaux[i-1] : c->lib;
		if (libfile[0] != 0)
		{
			uint64 tmp_length;
	
			#if DEBUG_LOADER	
			printf("Loading library: %s\n", c->lib);
			#endif
			if (ao_get_lib(libfile, &lib_raw_file, &tmp_length) != AO_SUCCESS)
			{
				return AO_FAIL;
			}
			lib_raw_length = tmp_length;
		
			if (corlett_decode(lib_raw_file, lib_raw_length, &lib_decoded, &lib_len, &lib) != AO_SUCCESS)
			{
				free(lib_raw_file);
				return AO_FAIL;
			}
				
			// Free up raw file
			free(lib_raw_file);

			// patch the file into ram
			offset = lib_decoded[0] | lib_decoded[1]<<8 | lib_decoded[2]<<16 | lib_decoded[3]<<24;
			memcpy(&dc_ram[offset], lib_decoded+4, lib_len-4);

			// Dispose the corlett structure for the lib - we don't use it
			free(lib);
		}
	}

	// now patch the file into RAM over the libraries
	offset = file[3]<<24 | file[2]<<16 | file[1]<<8 | file[0];
	memcpy(&dc_ram[offset], file+4, file_len-4);

	free(file);
	
	// Finally, set psfby/ssfby tag
	strcpy(psfby, "n/a");
	if (c)
	{
		for (i = 0; i < MAX_UNKNOWN_TAGS; i++)
		{
			if ((!strcasecmp(c->tag_name[i], "psfby")) || (!strcasecmp(c->tag_name[i], "ssfby")))
				strcpy(psfby, c->tag_data[i]);
		}
	}

	#if DEBUG_LOADER && 1
	{
		FILE *f;

		f = fopen("dcram.bin", "wb");
		fwrite(dc_ram, 2*1024*1024, 1, f);
		fclose(f);
	}
	#endif

	#if DK_CORE
	ARM7_Init();
	#else
	arm7_init(0, 45000000, NULL, NULL);
	arm7_reset();
	#endif
	dc_hw_init();

	// now figure out the time in samples for the length/fade
	lengthMS = psfTimeToMS(c->inf_length);
	fadeMS = psfTimeToMS(c->inf_fade);
	total_samples = 0;

	if (lengthMS == 0)
	{
		lengthMS = ~0;
	}

	if (lengthMS == ~0)
	{
		decaybegin = lengthMS;
	}
	else
	{
		lengthMS = (lengthMS * 441) / 10;
		fadeMS = (fadeMS * 441) / 10;

		decaybegin = lengthMS;
		decayend = lengthMS + fadeMS;
	}

	return AO_SUCCESS;
}

int32 dsf_gen(int16 *buffer, uint32 samples)
{	
	int i;
	int16 output[44100/30], output2[44100/30];
	int16 *stereo[2];
	int16 *outp = buffer;
	int opos;

	opos = 0;
	for (i = 0; i < samples; i++)
	{
		#if DK_CORE
		ARM7_Execute((33000000 / 60 / 4) / 735);
		#else
		arm7_execute((33000000 / 60 / 4) / 735);
		#endif
		stereo[0] = &output[opos];
		stereo[1] = &output2[opos];
		AICA_Update(NULL, NULL, stereo, 1);
		opos++;		
	}

	for (i = 0; i < samples; i++)
	{
		// process the fade tags
		if (total_samples >= decaybegin)
		{
			if (total_samples >= decayend)
			{
				// song is done here, signal your player appropriately!
//				ao_song_done = 1;
				output[i] = 0;
				output2[i] = 0;
			}
			else
			{
				int32 fader = 256 - (256*(total_samples - decaybegin)/(decayend-decaybegin));
				output[i] = (output[i] * fader)>>8;
				output2[i] = (output2[i] * fader)>>8;

				total_samples++;
			}
		}
		else
		{
			total_samples++;
		}

		*outp++ = output[i];
		*outp++ = output2[i];
	}

	return AO_SUCCESS;
}

int32 dsf_stop(void)
{
	return AO_SUCCESS;
}

int32 dsf_command(int32 command, int32 parameter)
{
	switch (command)
	{
		case COMMAND_RESTART:
			return AO_SUCCESS;
		
	}
	return AO_FAIL;
}

int32 dsf_fill_info(ao_display_info *info)
{
	if (c == NULL)
		return AO_FAIL;
		
	strcpy(info->title[1], "Name: ");
	sprintf(info->info[1], "%s", c->inf_title);

	strcpy(info->title[2], "Game: ");
	sprintf(info->info[2], "%s", c->inf_game);
	
	strcpy(info->title[3], "Artist: ");
	sprintf(info->info[3], "%s", c->inf_artist);

	strcpy(info->title[4], "Copyright: ");
	sprintf(info->info[4], "%s", c->inf_copy);

	strcpy(info->title[5], "Year: ");
	sprintf(info->info[5], "%s", c->inf_year);

	strcpy(info->title[6], "Length: ");
	sprintf(info->info[6], "%s", c->inf_length);

	strcpy(info->title[7], "Fade: ");
	sprintf(info->info[7], "%s", c->inf_fade);

	strcpy(info->title[8], "Ripper: ");
	sprintf(info->info[8], "%s", psfby);

	return AO_SUCCESS;
}
