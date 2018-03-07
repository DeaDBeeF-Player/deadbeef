// Game_Music_Emu $vers. http://www.slack.net/~ant/

#include "Vgm_Core.h"

#include "blargg_endian.h"
#include <math.h>
#include <stdio.h>

/* Copyright (C) 2003-2008 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

Vgm_Core::Vgm_Core()
{
	vgmp = (VGM_PLAYER *) VGMPlay_Init();
    vgmp->VGMMaxLoop = 0;
	VGMPlay_Init2(vgmp);
}

Vgm_Core::~Vgm_Core()
{
	StopVGM(vgmp);
	CloseVGMFile(vgmp);
	VGMPlay_Deinit(vgmp);
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

void Vgm_Core::set_tempo( double t )
{
	if ( file_begin() )
	{
        int vgm_rate_unit = header().lngRate;
        if (!vgm_rate_unit)
            vgm_rate_unit = 44100;
		int vgm_rate = (int) (vgm_rate_unit * t + 0.5);
		int old_rate = vgmp->VGMPbRate;
		vgmp->VGMPbRate = vgm_rate;
        vgmp->SampleRate = sample_rate;

		if (vgmp->PlayingMode != 0xFF)
		{
			if (!old_rate)
				old_rate = vgm_rate_unit;

			INT32 TempSLng = gcd(vgm_rate_unit, vgmp->VGMPbRate);
			vgmp->VGMPbRateMul = vgm_rate_unit / TempSLng;
			vgmp->VGMPbRateDiv = vgmp->VGMPbRate / TempSLng;

			vgmp->VGMSmplRateMul = vgmp->SampleRate * vgmp->VGMPbRateMul;
			vgmp->VGMSmplRateDiv = vgmp->VGMSampleRate * vgmp->VGMPbRateDiv;
			// same as above - to speed up the VGM <-> Playback calculation
			TempSLng = gcd(vgmp->VGMSmplRateMul, vgmp->VGMSmplRateDiv);
			vgmp->VGMSmplRateMul /= TempSLng;
			vgmp->VGMSmplRateDiv /= TempSLng;

			vgmp->VGMSmplPlayed = (INT32)((INT64)vgmp->VGMSmplPlayed * old_rate / vgm_rate);
		}
	}
}

struct VGM_FILE_mem
{
	VGM_FILE vf;
	const BOOST::uint8_t* buffer;
	UINT32 ptr;
	UINT32 size;
};

static int VGMF_mem_Read(VGM_FILE* f, void* out, UINT32 count)
{
	VGM_FILE_mem* mf = (VGM_FILE_mem *) f;
	if (count + mf->ptr > mf->size)
		count = mf->size - mf->ptr;
	memcpy(out, mf->buffer + mf->ptr, count);
	mf->ptr += count;
	return count;
}

static int VGMF_mem_Seek(VGM_FILE* f, UINT32 offset)
{
	VGM_FILE_mem* mf = (VGM_FILE_mem *) f;
	if (offset > mf->size)
		offset = mf->size;
	mf->ptr = offset;
	return 0;
}

static UINT32 VGMF_mem_GetSize(VGM_FILE* f)
{
	VGM_FILE_mem* mf = (VGM_FILE_mem *) f;
	return mf->size;
}

static UINT32 VGMF_mem_Tell(VGM_FILE* f)
{
	VGM_FILE_mem* mf = (VGM_FILE_mem *) f;
	return mf->ptr;
}

blargg_err_t Vgm_Core::load_mem_( byte const data [], int size )
{
	VGM_FILE_mem memFile;

	memFile.vf.Read = &VGMF_mem_Read;
	memFile.vf.Seek = &VGMF_mem_Seek;
	memFile.vf.GetSize = &VGMF_mem_GetSize;
	memFile.vf.Tell = &VGMF_mem_Tell;
	memFile.buffer = data;
	memFile.ptr = 0;
	memFile.size = size;

	if ( !GetVGMFileInfo_Handle( (VGM_FILE *) &memFile, &_header, 0 ) )
		return blargg_err_file_type;

	memFile.ptr = 0;

	if ( !OpenVGMFile_Handle( vgmp, (VGM_FILE *) &memFile ) )
		return blargg_err_file_type;
    
    if ( !header().lngLoopOffset )
        vgmp->VGMMaxLoop = 1;

	set_tempo( 1 );
    
	return blargg_ok;
}

int Vgm_Core::get_channel_count()
{
    // XXX may support more than this, but 32 bit masks and all...
    unsigned i;
    UINT32 j;
    for (i = 0; i < 32; i++)
    {
        if (!GetAccurateChipNameByChannel(vgmp, i, &j))
            break;
    }
    return i;
}

char* Vgm_Core::get_voice_name(int channel)
{
    UINT32 realChannel;
    const char * name = GetAccurateChipNameByChannel(vgmp, channel, &realChannel);
    size_t length = strlen(name) + 16;
    char * finalName = (char *) malloc(length);
    if (finalName)
#ifdef _MSC_VER
        sprintf_s(finalName, length, "%s #%u", name, realChannel);
#else
        sprintf(finalName, "%s #%u", name, realChannel);
#endif
    return finalName;
}

void Vgm_Core::free_voice_name(char *name)
{
    free(name);
}

void Vgm_Core::set_mute(int mask)
{
    for (int i = 0; i < 32; i++)
    {
        SetChannelMute(vgmp, i, (mask >> i) & 1);
    }
}

void Vgm_Core::start_track()
{
	PlayVGM(vgmp);
	RestartVGM(vgmp);
}

int Vgm_Core::play_( int sample_count, short out [] )
{
	// to do: timing is working mostly by luck
	int pairs = (unsigned) sample_count / 2;

    memset(out, 0, sizeof(short) * pairs * 2);
    FillBuffer(vgmp, (WAVE_16BS*) out, pairs);

	return pairs * 2;
}

void Vgm_Core::skip_( int count )
{
	SeekVGM( vgmp, true, count / 2 );
}
