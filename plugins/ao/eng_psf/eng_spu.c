/*
	Audio Overload SDK - SPU file format engine

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

//
// eng_spu.c
//
// Note: support for old-format files is not tested and may not work.  All the rips I could find
//       are in the newer format.  Also, CDDA and XA commands do not work - I've not found a rip using them.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ao.h"
#include "eng_protos.h"
#include "cpuintrf.h"
#include "psx.h"

extern int SPUinit(void);
extern int SPUopen(void);
extern int SPUclose(void);
extern void SPUinjectRAMImage(unsigned short *source);

static uint8 *start_of_file, *song_ptr;
static uint32 cur_tick, cur_event, num_events, next_tick, end_tick;
static int old_fmt;
static char name[128], song[128], company[128];

int32 spu_start(uint8 *buffer, uint32 length)
{
	int i;
	uint16 reg;

	if (strncmp((char *)buffer, "SPU", 3))
	{
		return AO_FAIL;
	}

	start_of_file = buffer;

	SPUinit();
	SPUopen();
	setlength(~0, 0);

	// upload the SPU RAM image
	SPUinjectRAMImage((unsigned short *)&buffer[0]);

	// apply the register image	
	for (i = 0; i < 512; i += 2)
	{
		reg = buffer[0x80000+i] | buffer[0x80000+i+1]<<8;

		SPUwriteRegister((i/2)+0x1f801c00, reg);
	}

	old_fmt = 1;

	if ((buffer[0x80200] != 0x44) || (buffer[0x80201] != 0xac) || (buffer[0x80202] != 0x00) || (buffer[0x80203] != 0x00))
	{
		old_fmt = 0;
	}

	if (old_fmt)
	{
		num_events = buffer[0x80204] | buffer[0x80205]<<8 | buffer[0x80206]<<16 | buffer[0x80207]<<24;

		if (((num_events * 12) + 0x80208) > length)
		{
			old_fmt = 0;
		}
		else
		{
			cur_tick = 0;
		}
	}

	if (!old_fmt)
	{
		end_tick = buffer[0x80200] | buffer[0x80201]<<8 | buffer[0x80202]<<16 | buffer[0x80203]<<24; 
		cur_tick = buffer[0x80204] | buffer[0x80205]<<8 | buffer[0x80206]<<16 | buffer[0x80207]<<24; 
		next_tick = cur_tick;
	}

	song_ptr = &buffer[0x80208];
	cur_event = 0;

	strncpy((char *)&buffer[4], name, 128);
	strncpy((char *)&buffer[0x44], song, 128);
	strncpy((char *)&buffer[0x84], company, 128);

	return AO_SUCCESS;
}

extern int SPUasync(uint32 cycles);
extern void SPU_flushboot(void);

extern char *spu_pOutput;	// this is a bit lame, but we'll deal

static void spu_tick(void)
{
	uint32 time, reg, size;
	uint16 rdata;
	uint8 opcode;

	if (old_fmt)
	{
		time = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24;

		while ((time == cur_tick) && (cur_event < num_events))
		{
			reg = song_ptr[4] | song_ptr[5]<<8 | song_ptr[6]<<16 | song_ptr[7]<<24;
			rdata = song_ptr[8] | song_ptr[9]<<8;

			SPUwriteRegister(reg, rdata);

			cur_event++;
			song_ptr += 12;

			time = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24;
		}
	}
	else
	{
		if (cur_tick < end_tick)
		{
			while (cur_tick == next_tick)
			{
				opcode = song_ptr[0];
				song_ptr++;

				switch (opcode)
				{
					case 0:	// write register
						reg = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24;
						rdata = song_ptr[4] | song_ptr[5]<<8;

						SPUwriteRegister(reg, rdata);

						next_tick = song_ptr[6] | song_ptr[7]<<8 | song_ptr[8]<<16 | song_ptr[9]<<24; 
						song_ptr += 10;
						break;

					case 1:	// read register
				 		reg = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24;
						SPUreadRegister(reg);
						next_tick = song_ptr[4] | song_ptr[5]<<8 | song_ptr[6]<<16 | song_ptr[7]<<24; 
						song_ptr += 8;
						break;

					case 2: // dma write
						size = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24; 
						song_ptr += (4 + size);
						next_tick = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24; 
						song_ptr += 4;
						break;

					case 3: // dma read
						next_tick = song_ptr[4] | song_ptr[5]<<8 | song_ptr[6]<<16 | song_ptr[7]<<24; 
						song_ptr += 8;
						break;

					case 4: // xa play
						song_ptr += (32 + 16384);
						next_tick = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24; 
						song_ptr += 4;
						break;

					case 5: // cdda play
						size = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24; 
						song_ptr += (4 + size);
						next_tick = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24; 
						song_ptr += 4;
						break;

					default:
						printf("Unknown opcode %d\n", opcode);
						exit(-1);
						break;
				}
			}
		}
		else
		{
//			ao_song_done = 1;
		}
	}

	cur_tick++;
}

int32 spu_gen(int16 *buffer, uint32 samples)
{
	int i, run = 1;

	if (old_fmt)
	{
		if (cur_event >= num_events)
		{
			run = 0;
		}
	}
	else
	{
		if (cur_tick >= end_tick)
		{
			run = 0;
		}
	}

	if (run)
	{
		for (i = 0; i < samples; i++)
		{
		  	spu_tick();
			SPUasync(384);
		}

		spu_pOutput = (char *)buffer;
		SPU_flushboot();
	}
	else
	{
		memset(buffer, 0, samples*2*sizeof(int16));
	}

	return AO_SUCCESS;
}

int32 spu_stop(void)
{
	return AO_SUCCESS;
}

int32 spu_command(int32 command, int32 parameter)
{
	switch (command)
	{
		case COMMAND_GET_MIN:
		case COMMAND_GET_MAX:
		{
			return 0;
		}
		break;
		
		case COMMAND_HAS_PREV:
		case COMMAND_HAS_NEXT:
		case COMMAND_PREV:
		case COMMAND_NEXT:
		case COMMAND_JUMP:
		{
			return AO_FAIL;
		}
		break;
		
		case COMMAND_RESTART:
		{
			song_ptr = &start_of_file[0x80200];

			if (old_fmt)
			{
				num_events = song_ptr[4] | song_ptr[5]<<8 | song_ptr[6]<<16 | song_ptr[7]<<24;
			}
			else
			{
				end_tick = song_ptr[0] | song_ptr[1]<<8 | song_ptr[2]<<16 | song_ptr[3]<<24; 
				cur_tick = song_ptr[4] | song_ptr[5]<<8 | song_ptr[6]<<16 | song_ptr[7]<<24; 
			}

			song_ptr += 8;
			cur_event = 0;
			return AO_SUCCESS;
		}
		break;
		
#if VERBOSE
		default:
			printf("Unknown command executed!\n");
			break;
#endif		
	}
	
	return AO_FAIL;
}

int32 spu_fill_info(ao_display_info *info)
{
	strcpy(info->title[1], "Game: ");
	sprintf(info->info[1], "%.128s", name);
	strcpy(info->title[2], "Song: ");
	sprintf(info->info[2], "%.128s", song);
	strcpy(info->title[3], "Company: ");
	sprintf(info->info[3], "%.128s", company);

	return AO_SUCCESS;
}
