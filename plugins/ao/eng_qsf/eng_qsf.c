/*
	Audio Overload SDK - QSF file engine

	Copyright (c) 2007, R. Belmont and Richard Bannister.

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
// eng_qsf.c
// by R. Belmont
//

/*
The program section of a QSF file, once decompressed, contains a series of
data blocks of the form:

3 bytes - ASCII section name tag
4 bytes - Starting offset (LSB-first)
4 bytes - Length (N) (LSB-first)
N bytes - Data

The data is then loaded to the given starting offset in the section described
by the ASCII tag.

The following sections are defined:

"KEY" - Kabuki decryption key.  This section should be 11 bytes and contain
        the following:
        4 bytes - swap_key1 (MSB-first)
        4 bytes - swap_key2 (MSB-first)
        2 bytes - addr_key  (MSB-first)
        1 bytes - xor_key
"Z80" - Z80 program ROM.
"SMP" - QSound sample ROM.

If the KEY section isn't given or both swap_keys are zero, then it is assumed
that no encryption is used.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ao.h"
#include "qsound.h"
#include "z80.h"

#include "corlett.h"

#define DEBUG_LOADER	(0)

// timer rate is 285 Hz
static int32 samples_per_tick = 44100/285;
static int32 samples_to_next_tick = 44100/285;

typedef struct {
    corlett_t *c;
    char qsfby[256];
    uint32 skey1, skey2;
    uint16 akey;
    uint8  xkey;
    int32 uses_kabuki;
    char *Z80ROM, *QSamples;
    char RAM[0x1000], RAM2[0x1000];
    int32 cur_bank;
} qsf_synth_t;

static struct QSound_interface qsintf = 
{
	QSOUND_CLOCK,
	NULL
};

extern void cps1_decode(unsigned char *rom, int swap_key1,int swap_key2,int addr_key,int xor_key);

static void qsf_walktags(qsf_synth_t *s, uint8 *buffer, uint8 *end)
{
	uint8 *cbuf = buffer;
	uint32 offset, length;

	while (cbuf < end)
	{
		#if DEBUG_LOADER				
		printf("cbuf: %08x end: %08x\n", (uint32)cbuf, (uint32)end);
		#endif
		offset = cbuf[3] | cbuf[4]<<8 | cbuf[5]<<16 | cbuf[6]<<24;
		length = cbuf[7] | cbuf[8]<<8 | cbuf[9]<<16 | cbuf[10]<<24;

		#if DEBUG_LOADER				
		printf("Tag: %c%c%c @ %08x, length %08x\n", cbuf[0], cbuf[1], cbuf[2], offset, length);
		#endif

		switch (cbuf[0])
		{
			case 'Z':
				memcpy(&s->Z80ROM[offset], &cbuf[11], length);
				break;

			case 'S':
				memcpy(&s->QSamples[offset], &cbuf[11], length);
				break;

			case 'K':
				s->skey1 = cbuf[11]<<24 | cbuf[12]<<16 | cbuf[13]<<8 | cbuf[14];
				s->skey2 = cbuf[15]<<24 | cbuf[16]<<16 | cbuf[17]<<8 | cbuf[18];
				s->akey = cbuf[19]<<8 | cbuf[20];
				s->xkey = cbuf[20];
				break;

			default:
				printf("ERROR: Unknown QSF tag!\n");
				break;
		}

		cbuf += 11;
		cbuf += length;
	}
}

static int32 qsf_irq_cb(int param)
{
	return 0x000000ff;	// RST_38
}

void *qsf_start(uint8 *buffer, uint32 length)
{
    qsf_synth_t *s = malloc (sizeof (qsf_synth_t));
    memset (s, 0, sizeof (qsf_synth_t));

	uint8 *file, *lib_decoded, *lib_raw_file;
	uint64 file_len, lib_len, lib_raw_length;
	corlett_t *lib;

	z80_init();

	s->Z80ROM = malloc(512*1024);
	s->QSamples = malloc(8*1024*1024);

	s->skey1 = s->skey2 = 0;
	s->akey = 0;
	s->xkey = 0;
	s->cur_bank = 0;

	memset(s->RAM, 0, 0x1000);
	memset(s->RAM2, 0, 0x1000);
		
	// Decode the current QSF
	if (corlett_decode(buffer, length, &file, &file_len, &s->c) != AO_SUCCESS)
	{
		return AO_FAIL;
	}

	// Get the library file
	if (s->c->lib[0] != 0)
	{
		uint64 tmp_length;
	
		#if DEBUG_LOADER	
		printf("Loading library: %s\n", s->c->lib);
		#endif
		if (ao_get_lib(s->c->lib, &lib_raw_file, &tmp_length) != AO_SUCCESS)
		{
            qsf_stop (s);
            return NULL;
		}
		lib_raw_length = tmp_length;
		
		if (corlett_decode(lib_raw_file, lib_raw_length, &lib_decoded, &lib_len, &lib) != AO_SUCCESS)
		{
			free(lib_raw_file);
            qsf_stop (s);
            return NULL;
		}
				
		// Free up raw file
		free(lib_raw_file);

		// use the contents
		qsf_walktags(s, lib_decoded, lib_decoded+lib_len);
		
		// Dispose the corlett structure for the lib - we don't use it
		free(lib);
	}

	// now patch the file into RAM OVER the libraries
	qsf_walktags(s, file, file+file_len);

	free(file);

	if ((s->skey1 != 0) && (s->skey2 != 0))
	{
		#if DEBUG_LOADER
		printf("Decoding Kabuki: skey1 %08x skey2 %08x akey %04x xkey %02x\n", skey1, skey2, akey, xkey);
		#endif

		s->uses_kabuki = 1;
		cps1_decode((unsigned char *)s->Z80ROM, s->skey1, s->skey2, s->akey, s->xkey);
	}

	// set qsfby tag
	strcpy(s->qsfby, "n/a");
	if (s->c)
	{
		int i;
		for (i = 0; i < MAX_UNKNOWN_TAGS; i++)
		{
			if (!strcasecmp(s->c->tag_name[i], "qsfby"))
			{
				strcpy(s->qsfby, s->c->tag_data[i]);
			}
		}
	}

	z80_reset(NULL);
	z80_set_irq_callback(qsf_irq_cb);
	qsintf.sample_rom = s->QSamples;
	qsound_sh_start(&qsintf);

	return s;
}

static void timer_tick(void)
{
	z80_set_irq_line(0, ASSERT_LINE);
	z80_set_irq_line(0, CLEAR_LINE);
}

int32 qsf_gen(int16 *buffer, uint32 samples)
{	
	int16 output[44100/30], output2[44100/30];
	int16 *stereo[2];
	int16 *outp = buffer;
	int32 i, opos, tickinc, loops;

	// our largest possible step is samples_per_tick or samples, whichever is smaller
	if (samples_to_next_tick > samples)
	{
		tickinc = samples;
	}
	else
	{
		tickinc = samples_to_next_tick;
	}

	loops = samples / tickinc;
	opos = 0;

	for (i = 0; i < loops; i++)
	{
		z80_execute((8000000/44100)*tickinc);
		stereo[0] = &output[opos];
		stereo[1] = &output2[opos];
		qsound_update(0, stereo, tickinc);

		opos += tickinc;
		samples_to_next_tick -= tickinc;

		if (samples_to_next_tick <= 0)
		{
			timer_tick();
			samples_to_next_tick = samples_per_tick;
		}
	}

	// are there "leftovers"?
	if (opos < samples)
	{
		z80_execute((8000000/44100)*(samples-opos));
		stereo[0] = &output[opos];
		stereo[1] = &output2[opos];
		qsound_update(0, stereo, (samples-opos));

		samples_to_next_tick -= (samples-opos);

		if (samples_to_next_tick <= 0)
		{
			timer_tick();
			samples_to_next_tick = samples_per_tick;
		}
	}

	for (i = 0; i < samples; i++)
	{
		*outp++ = output[i];
		*outp++ = output2[i];
	}

	return AO_SUCCESS;
}

int32 qsf_stop(void *handle)
{
    qsf_synth_t *s = (qsf_synth_t *)handle;
	free(s->Z80ROM);
	free(s->QSamples);
	free(s);

	return AO_SUCCESS;
}

int32 qsf_command(void *handle, int32 command, int32 parameter)
{
	switch (command)
	{
		case COMMAND_RESTART:
			return AO_SUCCESS;
		
	}
	return AO_FAIL;
}

int32 qsf_fill_info(void *handle, ao_display_info *info)
{
    qsf_synth_t *s = handle;
	if (s->c == NULL)
		return AO_FAIL;
		
	strcpy(info->title[1], "Name: ");
	sprintf(info->info[1], "%s", s->c->inf_title);

	strcpy(info->title[2], "Game: ");
	sprintf(info->info[2], "%s", s->c->inf_game);
	
	strcpy(info->title[3], "Artist: ");
	sprintf(info->info[3], "%s", s->c->inf_artist);

	strcpy(info->title[4], "Copyright: ");
	sprintf(info->info[4], "%s", s->c->inf_copy);

	strcpy(info->title[5], "Year: ");
	sprintf(info->info[5], "%s", s->c->inf_year);

	strcpy(info->title[6], "Length: ");
	sprintf(info->info[6], "%s", s->c->inf_length);

	strcpy(info->title[7], "Fade: ");
	sprintf(info->info[7], "%s", s->c->inf_fade);

	strcpy(info->title[8], "Ripper: ");
	sprintf(info->info[8], "%s", s->qsfby);

	return AO_SUCCESS;
}

uint8 qsf_memory_read(qsf_synth_t *s, uint16 addr)
{
	if (addr < 0x8000)
	{
		return s->Z80ROM[addr];
	}
	else if (addr < 0xc000)
	{
		return s->Z80ROM[(addr - 0x8000) + s->cur_bank];
	}
	else if (addr <= 0xcfff)
	{
		return s->RAM[addr - 0xc000];
	}
	else if (addr == 0xd007)
	{
		return qsound_status_r();
	}
	else if (addr >= 0xf000)
	{
		return s->RAM2[addr-0xf000];
	}
}

uint8 qsf_memory_readop(qsf_synth_t *s, uint16 addr)
{
	if (!s->uses_kabuki)
	{
		return qsf_memory_read(s, addr);
	}

	if (addr < 0x8000)
	{
		return s->Z80ROM[addr + (256*1024)];
	}

	return qsf_memory_read(s, addr);
}

uint8 qsf_memory_readport(qsf_synth_t *s, uint16 addr)
{
	return s->Z80ROM[0x11];
}

void qsf_memory_write(qsf_synth_t *s, uint16 addr, uint8 byte)
{
	if (addr >= 0xc000 && addr <= 0xcfff)
	{
		
		s->RAM[addr-0xc000] = byte;
		return;
	}
	else if (addr == 0xd000)
	{
		qsound_data_h_w(byte);
		return;
	}
	else if (addr == 0xd001)
	{
		qsound_data_l_w(byte);
		return;
	}
	else if (addr == 0xd002)
	{
		qsound_cmd_w(byte);
		return;
	}
	else if (addr == 0xd003)
	{
		s->cur_bank = (0x8000 + (byte & 0xf) * 0x4000);
		if (s->cur_bank > (256*1024))
		{
			s->cur_bank = 0;
		}
//		printf("Z80 bank to %x (%x)\n", cur_bank, byte);
		return;
	}
	else if (addr >= 0xf000)
	{
		s->RAM2[addr-0xf000] = byte;
		return;
	}
}

void qsf_memory_writeport(uint16 addr, uint8 byte)
{
	printf("Unk port %x @ %x\n", byte, addr);
}

