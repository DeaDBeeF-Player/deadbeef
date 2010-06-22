/*
	Audio Overload SDK - PSF file format engine

	Copyright (c) 2007 R. Belmont and Richard Bannister.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	* Neither the names of R. Belmont and Richard Bannister nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ao.h"
#include "eng_protos.h"
#include "cpuintrf.h"
#include "psx.h"

#include "peops/stdafx.h"
#include "peops/externals.h"
#include "peops/regs.h"
#include "peops/registers.h"
#include "peops/spu.h"


#include "corlett.h"

#define DEBUG_LOADER	(0)

static corlett_t	*c = NULL;
static char 		psfby[256];
char			*spu_pOutput;
int			psf_refresh  = -1;


// main RAM
extern uint32 psx_ram[((2*1024*1024)/4)+4];
extern uint32 psx_scratch[0x400];
extern uint32 initial_ram[((2*1024*1024)/4)+4];
extern uint32 initial_scratch[0x400];
static uint32 initialPC, initialGP, initialSP;

extern void mips_init( void );
extern void mips_reset( void *param );
extern int mips_execute( int cycles );
extern void mips_set_info(UINT32 state, union cpuinfo *info);
extern void psx_hw_init(void);
extern void psx_hw_slice(void);
extern void psx_hw_frame(void);
extern void setlength(int32 stop, int32 fade);

int32 psf_start(uint8 *buffer, uint32 length)
{
	uint8 *file, *lib_decoded, *lib_raw_file, *alib_decoded;
	uint32 offset, plength, PC, SP, GP, lengthMS, fadeMS;
	uint64 file_len, lib_len, lib_raw_length, alib_len;
	corlett_t *lib;
	int i;
	union cpuinfo mipsinfo;

	// clear PSX work RAM before we start scribbling in it
	memset(psx_ram, 0, 2*1024*1024);

//	printf("Length = %d\n", length);

	// Decode the current GSF
	if (corlett_decode(buffer, length, &file, &file_len, &c) != AO_SUCCESS)
	{
		return AO_FAIL;
	}

//	printf("file_len %d reserve %d\n", file_len, c->res_size);

	// check for PSX EXE signature
	if (strncmp((char *)file, "PS-X EXE", 8))
	{
		return AO_FAIL;
	}

	#if DEBUG_LOADER
	offset = file[0x18] | file[0x19]<<8 | file[0x1a]<<16 | file[0x1b]<<24;
	printf("Text section start: %x\n", offset);
	offset = file[0x1c] | file[0x1d]<<8 | file[0x1e]<<16 | file[0x1f]<<24;
	printf("Text section size: %x\n", offset);
	printf("Region: [%s]\n", &file[0x4c]);
	printf("refresh: [%s]\n", c->inf_refresh);			
	#endif

	if (c->inf_refresh[0] == '5')
	{
		psf_refresh = 50;
	}
	if (c->inf_refresh[0] == '6')
	{
		psf_refresh = 60;
	}

	PC = file[0x10] | file[0x11]<<8 | file[0x12]<<16 | file[0x13]<<24;
	GP = file[0x14] | file[0x15]<<8 | file[0x16]<<16 | file[0x17]<<24;
	SP = file[0x30] | file[0x31]<<8 | file[0x32]<<16 | file[0x33]<<24;

	#if DEBUG_LOADER
	printf("Top level: PC %x GP %x SP %x\n", PC, GP, SP);
	#endif

	// Get the library file, if any
	if (c->lib[0] != 0)
	{
		uint64 tmp_length;
	
		#if DEBUG_LOADER	
		printf("Loading library: %s\n", c->lib);
		#endif
		if (ao_get_lib(c->lib, &lib_raw_file, &tmp_length) != AO_SUCCESS)
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

		if (strncmp((char *)lib_decoded, "PS-X EXE", 8))
		{
			printf("Major error!  PSF was OK, but referenced library is not!\n");
			free(lib);
			return AO_FAIL;
		}

		#if DEBUG_LOADER	
		offset = lib_decoded[0x18] | lib_decoded[0x19]<<8 | lib_decoded[0x1a]<<16 | lib_decoded[0x1b]<<24;
		printf("Text section start: %x\n", offset);
		offset = lib_decoded[0x1c] | lib_decoded[0x1d]<<8 | lib_decoded[0x1e]<<16 | lib_decoded[0x1f]<<24;
		printf("Text section size: %x\n", offset);
		printf("Region: [%s]\n", &lib_decoded[0x4c]);
		printf("refresh: [%s]\n", lib->inf_refresh);			
		#endif

		// if the original file had no refresh tag, give the lib a shot
		if (psf_refresh == -1)
		{
			if (lib->inf_refresh[0] == '5')
			{
				psf_refresh = 50;
			}
			if (lib->inf_refresh[0] == '6')
			{
				psf_refresh = 60;
			}
		}

		PC = lib_decoded[0x10] | lib_decoded[0x11]<<8 | lib_decoded[0x12]<<16 | lib_decoded[0x13]<<24;
		GP = lib_decoded[0x14] | lib_decoded[0x15]<<8 | lib_decoded[0x16]<<16 | lib_decoded[0x17]<<24;
		SP = lib_decoded[0x30] | lib_decoded[0x31]<<8 | lib_decoded[0x32]<<16 | lib_decoded[0x33]<<24;

		#if DEBUG_LOADER
		printf("Library: PC %x GP %x SP %x\n", PC, GP, SP);
		#endif

		// now patch the file into RAM
		offset = lib_decoded[0x18] | lib_decoded[0x19]<<8 | lib_decoded[0x1a]<<16 | lib_decoded[0x1b]<<24;
		offset &= 0x3fffffff;	// kill any MIPS cache segment indicators
		plength = lib_decoded[0x1c] | lib_decoded[0x1d]<<8 | lib_decoded[0x1e]<<16 | lib_decoded[0x1f]<<24;
		#if DEBUG_LOADER
		printf("library offset: %x plength: %d\n", offset, plength);
		#endif
		memcpy(&psx_ram[offset/4], lib_decoded+2048, plength);
		
		// Dispose the corlett structure for the lib - we don't use it
		free(lib);
	}

	// now patch the main file into RAM OVER the libraries (but not the aux lib)
	offset = file[0x18] | file[0x19]<<8 | file[0x1a]<<16 | file[0x1b]<<24;
	offset &= 0x3fffffff;	// kill any MIPS cache segment indicators
	plength = file[0x1c] | file[0x1d]<<8 | file[0x1e]<<16 | file[0x1f]<<24;

	// Philosoma has an illegal "plength".  *sigh*
	if (plength > (file_len-2048))
	{
		plength = file_len-2048;
	}
	memcpy(&psx_ram[offset/4], file+2048, plength);

	// load any auxiliary libraries now
	for (i = 0; i < 8; i++)
	{
		if (c->libaux[i][0] != 0)
		{
			uint64 tmp_length;
		
			#if DEBUG_LOADER	
			printf("Loading aux library: %s\n", c->libaux[i]);
			#endif

			if (ao_get_lib(c->libaux[i], &lib_raw_file, &tmp_length) != AO_SUCCESS)
			{
				return AO_FAIL;
			}
			lib_raw_length = tmp_length;
		
			if (corlett_decode(lib_raw_file, lib_raw_length, &alib_decoded, &alib_len, &lib) != AO_SUCCESS)
			{
				free(lib_raw_file);
				return AO_FAIL;
			}
				
			// Free up raw file
			free(lib_raw_file);

			if (strncmp((char *)alib_decoded, "PS-X EXE", 8))
			{
				printf("Major error!  PSF was OK, but referenced library is not!\n");
				free(lib);
				return AO_FAIL;
			}

			#if DEBUG_LOADER	
			offset = alib_decoded[0x18] | alib_decoded[0x19]<<8 | alib_decoded[0x1a]<<16 | alib_decoded[0x1b]<<24;
			printf("Text section start: %x\n", offset);
			offset = alib_decoded[0x1c] | alib_decoded[0x1d]<<8 | alib_decoded[0x1e]<<16 | alib_decoded[0x1f]<<24;
			printf("Text section size: %x\n", offset);
			printf("Region: [%s]\n", &alib_decoded[0x4c]);
			#endif

			// now patch the file into RAM
			offset = alib_decoded[0x18] | alib_decoded[0x19]<<8 | alib_decoded[0x1a]<<16 | alib_decoded[0x1b]<<24;
			offset &= 0x3fffffff;	// kill any MIPS cache segment indicators
			plength = alib_decoded[0x1c] | alib_decoded[0x1d]<<8 | alib_decoded[0x1e]<<16 | alib_decoded[0x1f]<<24;
			memcpy(&psx_ram[offset/4], alib_decoded+2048, plength);
		
			// Dispose the corlett structure for the lib - we don't use it
			free(lib);
		}
	}

	free(file);
//	free(lib_decoded);
	
	// Finally, set psfby tag
	strcpy(psfby, "n/a");
	if (c)
	{
		int i;
		for (i = 0; i < MAX_UNKNOWN_TAGS; i++)
		{
			if (!strcasecmp(c->tag_name[i], "psfby"))
				strcpy(psfby, c->tag_data[i]);
		}
	}

	mips_init();
	mips_reset(NULL);

	// set the initial PC, SP, GP
	#if DEBUG_LOADER	
	printf("Initial PC %x, GP %x, SP %x\n", PC, GP, SP);
	printf("Refresh = %d\n", psf_refresh);
	#endif
	mipsinfo.i = PC;
	mips_set_info(CPUINFO_INT_PC, &mipsinfo);

	// set some reasonable default for the stack
	if (SP == 0)
	{
		SP = 0x801fff00;
	}

	mipsinfo.i = SP;
	mips_set_info(CPUINFO_INT_REGISTER + MIPS_R29, &mipsinfo);
	mips_set_info(CPUINFO_INT_REGISTER + MIPS_R30, &mipsinfo);

	mipsinfo.i = GP;
	mips_set_info(CPUINFO_INT_REGISTER + MIPS_R28, &mipsinfo);

	#if DEBUG_LOADER && 1
	{
		FILE *f;

		f = fopen("psxram.bin", "wb");
		fwrite(psx_ram, 2*1024*1024, 1, f);
		fclose(f);
	}
	#endif

	psx_hw_init();
	SPUinit();
	SPUopen();

	lengthMS = psfTimeToMS(c->inf_length);
	fadeMS = psfTimeToMS(c->inf_fade);

	#if DEBUG_LOADER
	printf("length %d fade %d\n", lengthMS, fadeMS);
	#endif

	if (lengthMS == 0) 
	{
		lengthMS = ~0;
	}

	setlength(lengthMS, fadeMS);

	// patch illegal Chocobo Dungeon 2 code - CaitSith2 put a jump in the delay slot from a BNE
	// and rely on Highly Experimental's buggy-ass CPU to rescue them.  Verified on real hardware
	// that the initial code is wrong.
	if (c->inf_game)
	{
		if (!strcmp(c->inf_game, "Chocobo Dungeon 2"))
		{
			if (psx_ram[0xbc090/4] == LE32(0x0802f040))
			{
		 		psx_ram[0xbc090/4] = LE32(0);
				psx_ram[0xbc094/4] = LE32(0x0802f040);
				psx_ram[0xbc098/4] = LE32(0);
			}
		}
	}

//	psx_ram[0x118b8/4] = LE32(0);	// crash 2 hack

	// backup the initial state for restart
	memcpy(initial_ram, psx_ram, 2*1024*1024);
	memcpy(initial_scratch, psx_scratch, 0x400);
	initialPC = PC;
	initialGP = GP;
	initialSP = SP;

	mips_execute(5000);
	
	return AO_SUCCESS;
}

void spu_update(unsigned char* pSound,long lBytes)
{
	memcpy(spu_pOutput, pSound, lBytes);
}

int32 psf_gen(int16 *buffer, uint32 samples)
{	
	int i;

	for (i = 0; i < samples; i++)
	{
		psx_hw_slice();
		SPUasync(384);
	}

	spu_pOutput = (char *)buffer;
	SPU_flushboot();

	psx_hw_frame();

	return AO_SUCCESS;
}

int32 psf_stop(void)
{
	SPUclose();
	free(c);

	return AO_SUCCESS;
}

int32 psf_command(int32 command, int32 parameter)
{
	union cpuinfo mipsinfo;
	uint32 lengthMS, fadeMS;

	switch (command)
	{
		case COMMAND_RESTART:
			SPUclose();

			memcpy(psx_ram, initial_ram, 2*1024*1024);
			memcpy(psx_scratch, initial_scratch, 0x400);

			mips_init();
			mips_reset(NULL);
			psx_hw_init();
			SPUinit();
			SPUopen();

			lengthMS = psfTimeToMS(c->inf_length);
			fadeMS = psfTimeToMS(c->inf_fade);

			if (lengthMS == 0) 
			{
				lengthMS = ~0;
			}
			setlength(lengthMS, fadeMS);

			mipsinfo.i = initialPC;
			mips_set_info(CPUINFO_INT_PC, &mipsinfo);
			mipsinfo.i = initialSP;
			mips_set_info(CPUINFO_INT_REGISTER + MIPS_R29, &mipsinfo);
			mips_set_info(CPUINFO_INT_REGISTER + MIPS_R30, &mipsinfo);
			mipsinfo.i = initialGP;
			mips_set_info(CPUINFO_INT_REGISTER + MIPS_R28, &mipsinfo);

			mips_execute(5000);

			return AO_SUCCESS;
		
	}
	return AO_FAIL;
}

int32 psf_fill_info(ao_display_info *info)
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
