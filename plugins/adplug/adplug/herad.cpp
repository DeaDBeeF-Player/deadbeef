/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * herad.cpp - Herbulot AdLib Player by Stas'M <binarymaster@mail.ru>
 *
 * Thanks goes to co-workers: 
 *   -=CHE@TER=- (SQX decompression)
 *   SynaMax (general documentation, reverse-engineering, testing)
 *   Jepael (timer code sample, DOS driver shell)
 *   Daniël van de Burgt "thatdutchguy" (pitch slides code sample)
 *
 * REFERENCES:
 * http://www.vgmpf.com/Wiki/index.php/HERAD
 *
 * TODO:
 * - Fix strange AGD sound
 * - Fix splash sound in Gorbi (at 0:23)
 * - Fix hiss sound in NewSan (at beginning)
 */

#include <cstring>
#include <stdio.h>

#include "herad.h"

#ifdef DEBUG
#include "debug.h"
#else
#define AdPlug_LogWrite
#endif

#include "load_helper.h"

const uint8_t CheradPlayer::slot_offset[HERAD_NUM_VOICES] = {
	0, 1, 2, 8, 9, 10, 16, 17, 18
};
const uint16_t CheradPlayer::FNum[HERAD_NUM_NOTES] = {
	343, 364, 385, 408, 433, 459, 486, 515, 546, 579, 614, 650
};
const uint8_t CheradPlayer::fine_bend[HERAD_NUM_NOTES + 1] = {
	19, 21, 21, 23, 25, 26, 27, 29, 31, 33, 35, 36, 37
};
const uint8_t CheradPlayer::coarse_bend[10] = {
	0, 5, 10, 15, 20,
	0, 6, 12, 18, 24
};

CPlayer *CheradPlayer::factory(Copl *newopl)
{
	return new CheradPlayer(newopl);
}

std::string CheradPlayer::gettype()
{
	char scomp[12 + 1] = "";
	if (comp > HERAD_COMP_NONE)
		snprintf(scomp, sizeof(scomp), ", %s packed", (comp == HERAD_COMP_HSQ ? "HSQ" : "SQX"));
	char type[40 + 1];
	snprintf(type, sizeof(type), "HERAD System %s (version %d%s)", (AGD ? "AGD" : "SDB"), (v2 ? 2 : 1), scomp);
	return std::string(type);
}

bool isHSQ(uint8_t * data, int size)
{
	// data[0] - word DecompSize
	// data[1]
	// data[2] - byte Null = 0
	// data[3] - word CompSize
	// data[4]
	// data[5] - byte Checksum
	if (data[2] != 0)
	{
		AdPlug_LogWrite("HERAD: Is not HSQ, wrong check byte.\n");
		return false;
	}

	const uint16_t temp_size = u16_unaligned(data + 3);

	if (temp_size != size)
	{
		AdPlug_LogWrite("HERAD: Is not HSQ, wrong compressed size.\n");
		return false;
	}
	uint8_t checksum = 0;
	for (int i = 0; i < HERAD_MIN_SIZE; i++)
	{
		checksum += data[i];
	}
	if (checksum != 0xAB)
	{
		AdPlug_LogWrite("HERAD: Is not HSQ, wrong checksum.\n");
		return false;
	}
	return true;
}

bool isSQX(uint8_t * data)
{
	// data[0] - word OutbufInit
	// data[1]
	// data[2] - byte SQX flag #1
	// data[3] - byte SQX flag #2
	// data[4] - byte SQX flag #3
	// data[5] - byte CntOffPart
	if (data[2] > 2 || data[3] > 2 || data[4] > 2)
	{
		AdPlug_LogWrite("HERAD: Is not SQX, wrong flags.\n");
		return false;
	}
	if (data[5] == 0 || data[5] > 15)
	{
		AdPlug_LogWrite("HERAD: Is not SQX, wrong bit count.\n");
		return false;
	}
	return true;
}

uint16_t HSQ_decompress(uint8_t * data, int size, uint8_t * out)
{
	uint32_t queue = 1;
	int8_t bit;
	int16_t offset;
	uint16_t count, out_size = u16_unaligned(data);
	uint8_t * src = data;
	uint8_t * dst = out;

	src += 6;
	while (true)
	{
		// get next bit of the queue
		if (queue == 1)
		{
			queue = u16_unaligned(src) | 0x10000;
			src += 2;
		}
		bit = queue & 1;
		queue >>= 1;
		// if bit is non-zero
		if (bit)
		{
			// copy next byte of the input to the output
			*dst++ = *src++;
		}
		else
		{
			// get next bit of the queue
			if (queue == 1)
			{
				queue = u16_unaligned(src) | 0x10000;
				src += 2;
			}
			bit = queue & 1;
			queue >>= 1;
			// if bit is non-zero
			if (bit)
			{
				// count = next 3 bits of the input
				// offset = next 13 bits of the input minus 8192
				count = u16_unaligned(src);
				offset = (count >> 3) - 8192;
				count &= 7;
				src += 2;
				// if count is zero
				if (!count)
				{
					// count = next 8 bits of the input
					count = *(uint8_t *)src;
					src++;
				}
				// if count is zero
				if (!count)
					break; // finish the unpacking
			}
			else
			{
				// count = next bit of the queue * 2 + next bit of the queue
				if (queue == 1)
				{
					queue = u16_unaligned(src) | 0x10000;
					src += 2;
				}
				bit = queue & 1;
				queue >>= 1;
				count = bit << 1;
				if (queue == 1)
				{
					queue = u16_unaligned(src) | 0x10000;
					src += 2;
				}
				bit = queue & 1;
				queue >>= 1;
				count += bit;
				// offset = next 8 bits of the input minus 256
				offset = *(uint8_t *)src;
				offset -= 256;
				src++;
			}
			count += 2;
			// copy count bytes at (output + offset) to the output
			while (count--)
			{
				*dst = *(dst + offset);
				dst++;
			}
		}
	}
	return out_size;
}

uint16_t SQX_decompress(uint8_t * data, int size, uint8_t * out)
{
	int16_t offset;
	uint16_t count;
	uint8_t * src = data;
	uint8_t * dst = out;
	bool done = false;

	std::memcpy(dst, src, sizeof(uint16_t));
	src += 6;
	uint16_t queue = 1;
	uint8_t bit, bit_p;
	while (true)
	{
		bit = queue & 1;
		queue >>= 1;
		if (queue == 0)
		{
			queue = u16_unaligned(src);
			src += 2;
			bit_p = bit;
			bit = queue & 1;
			queue >>= 1;
			if (bit_p)
				queue |= 0x8000;
		}
		if (bit == 0)
		{
			switch (data[2])
			{
			case 0:
				*dst++ = *src++;
				break;
			case 1:
				count = 0;
				bit = queue & 1;
				queue >>= 1;
				if (queue == 0)
				{
					queue = u16_unaligned(src);
					src += 2;
					bit_p = bit;
					bit = queue & 1;
					queue >>= 1;
					if (bit_p)
						queue |= 0x8000;
					count = bit;
					bit = queue & 1;
					queue >>= 1;
				}
				else
				{
					count = bit;
					bit = queue & 1;
					queue >>= 1;
					if (queue == 0)
					{
						queue = u16_unaligned(src);
						src += 2;
						bit_p = bit;
						bit = queue & 1;
						queue >>= 1;
						if (bit_p)
							queue |= 0x8000;
					}
				}
				count = (count << 1) | bit;
				offset = *(uint8_t *)src;
				offset -= 256;
				src++;
				count += 2;
				while (count--)
				{
					*dst = *(dst + offset);
					dst++;
				}
				break;
			case 2:
				count = u16_unaligned(src);
				offset = (count >> data[5]) - (1 << (16 - data[5]));
				count &= (1 << data[5]) - 1;
				src += 2;
				if (!count)
				{
					count = *(uint8_t *)src;
					src++;
				}
				if (!count)
				{
					done = true;
					break;
				}
				count += 2;
				while (count--)
				{
					*dst = *(dst + offset);
					dst++;
				}
				break;
			}
			if (done)
				break;
			continue;
		}
		else
		{
			bit = queue & 1;
			queue >>= 1;
			if (queue == 0)
			{
				queue = u16_unaligned(src);
				src += 2;
				bit_p = bit;
				bit = queue & 1;
				queue >>= 1;
				if (bit_p)
					queue |= 0x8000;
			}
			if (bit == 0)
			{
				switch (data[3])
				{
				case 0:
					*dst++ = *src++;
					break;
				case 1:
					count = 0;
					bit = queue & 1;
					queue >>= 1;
					if (queue == 0)
					{
						queue = u16_unaligned(src);
						src += 2;
						bit_p = bit;
						bit = queue & 1;
						queue >>= 1;
						if (bit_p)
							queue |= 0x8000;
						count = bit;
						bit = queue & 1;
						queue >>= 1;
					}
					else
					{
						count = bit;
						bit = queue & 1;
						queue >>= 1;
						if (queue == 0)
						{
							queue = u16_unaligned(src);
							src += 2;
							bit_p = bit;
							bit = queue & 1;
							queue >>= 1;
							if (bit_p)
								queue |= 0x8000;
						}
					}
					count = (count << 1) | bit;
					offset = *(uint8_t *)src;
					offset -= 256;
					src++;
					count += 2;
					while (count--)
					{
						*dst = *(dst + offset);
						dst++;
					}
					break;
				case 2:
					count = u16_unaligned(src);
					offset = (count >> data[5]) - (1 << (16 - data[5]));
					count &= (1 << data[5]) - 1;
					src += 2;
					if (!count)
					{
						count = *(uint8_t *)src;
						src++;
					}
					if (!count)
					{
						done = true;
						break;
					}
					count += 2;
					while (count--)
					{
						*dst = *(dst + offset);
						dst++;
					}
					break;
				}
				if (done)
					break;
				continue;
			}
			else
			{
				switch (data[4])
				{
				case 0:
					*dst++ = *src++;
					break;
				case 1:
					count = 0;
					bit = queue & 1;
					queue >>= 1;
					if (queue == 0)
					{
						queue = u16_unaligned(src);
						src += 2;
						bit_p = bit;
						bit = queue & 1;
						queue >>= 1;
						if (bit_p)
							queue |= 0x8000;
						count = bit;
						bit = queue & 1;
						queue >>= 1;
					}
					else
					{
						count = bit;
						bit = queue & 1;
						queue >>= 1;
						if (queue == 0)
						{
							queue = u16_unaligned(src);
							src += 2;
							bit_p = bit;
							bit = queue & 1;
							queue >>= 1;
							if (bit_p)
								queue |= 0x8000;
						}
					}
					count = (count << 1) | bit;
					offset = *(uint8_t *)src;
					offset -= 256;
					src++;
					count += 2;
					while (count--)
					{
						*dst = *(dst + offset);
						dst++;
					}
					break;
				case 2:
					count = u16_unaligned(src);
					offset = (count >> data[5]) - (1 << (16 - data[5]));
					count &= (1 << data[5]) - 1;
					src += 2;
					if (!count)
					{
						count = *(uint8_t *)src;
						src++;
					}
					if (!count)
					{
						done = true;
						break;
					}
					count += 2;
					while (count--)
					{
						*dst = *(dst + offset);
						dst++;
					}
					break;
				}
				if (done)
					break;
				continue;
			}
		}
	}
	return dst - out;
}

bool CheradPlayer::validEvent(int i, uint16_t * offset, bool v2)
{
	while (*offset < track[i].size && (track[i].data[(*offset)++] & 0x80) > 0);
	if (*offset >= track[i].size)
	{
		AdPlug_LogWrite("HERAD: Reading out of range.\n");
		return false;
	}

	uint8_t status = track[i].data[(*offset)++];
	uint8_t check;

	if (status < 0x80)
	{
		AdPlug_LogWrite("HERAD: Unexpected status.\n");
		return false;
	}
	else if (status < 0x90 && v2)
	{
		check = track[i].data[(*offset)++];
		if (check > 0x7F)
		{
			AdPlug_LogWrite("HERAD: Unexpected param.\n");
			return false;
		}
	}
	else if (status < 0xC0)
	{
		check = track[i].data[(*offset)++];
		if (check > 0x7F)
		{
			AdPlug_LogWrite("HERAD: Unexpected param 1.\n");
			return false;
		}
		check = track[i].data[(*offset)++];
		if (check > 0x7F)
		{
			AdPlug_LogWrite("HERAD: Unexpected param 2.\n");
			return false;
		}
	}
	else if (status < 0xF0)
	{
		check = track[i].data[(*offset)++];
		if (check > 0x7F)
		{
			AdPlug_LogWrite("HERAD: Unexpected param.\n");
			return false;
		}
	}
	else if (status == 0xFF)
	{
		*offset = track[i].size;
	}

	return true;
}

uint8_t CheradPlayer::validTracks()
{
	for (int i = 0; i < nTracks; i++)
	{
		uint16_t of_v1 = 0, of_v2 = 0;

		while (of_v1 < track[i].size || of_v2 < track[i].size)
		{
			if (of_v1 < track[i].size)
			{
				if (!validEvent(i, &of_v1, false))
					return 1;
			}

			if (of_v2 < track[i].size)
			{
				if (!validEvent(i, &of_v2, true))
					return 2;
			}
		}
	}

	return 0;
}

bool CheradPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename); if(!f) return false;

	// file validation
	if (!fp.extension(filename, ".hsq") &&
		!fp.extension(filename, ".sqx") &&
		!fp.extension(filename, ".sdb") &&
		!fp.extension(filename, ".agd") &&
		!fp.extension(filename, ".ha2"))
	{
		AdPlug_LogWrite("HERAD: Unsupported file extension.\n");
		fp.close(f);
		return false;
	}
	int size = fp.filesize(f);
	if (size < HERAD_MIN_SIZE)
	{
		AdPlug_LogWrite("HERAD: File size is too small.\n");
		fp.close(f);
		return false;
	}
	if (size > HERAD_MAX_SIZE)
	{
		AdPlug_LogWrite("HERAD: File size is too big.\n");
		fp.close(f);
		return false;
	}
	// Read entire file into memory
	uint8_t * data = new uint8_t[size];
	f->readString((char *)data, size);
	fp.close(f);
	// Detect compression
	if (isHSQ(data, size))
	{
		comp = HERAD_COMP_HSQ;
		uint8_t * out = new uint8_t[HERAD_MAX_SIZE];
		memset(out, 0, HERAD_MAX_SIZE);
		size = HSQ_decompress(data, size, out);
		delete[] data;
		data = new uint8_t[size];
		memcpy(data, out, size);
		delete[] out;
	}
	else if (isSQX(data))
	{
		comp = HERAD_COMP_SQX;
		uint8_t * out = new uint8_t[HERAD_MAX_SIZE];
		memset(out, 0, HERAD_MAX_SIZE);
		size = SQX_decompress(data, size, out);
		delete[] data;
		data = new uint8_t[size];
		memcpy(data, out, size);
		delete[] out;
	}
	else
	{
		comp = HERAD_COMP_NONE;
	}
	// Process file header
	uint16_t offset;
	if (size < HERAD_HEAD_SIZE)
	{
		AdPlug_LogWrite("HERAD: File size is too small.\n");
		goto failure;
	}
	if (size < u16_unaligned(data))
	{
		AdPlug_LogWrite("HERAD: Incorrect offset / file size.\n");
		goto failure;
	}
	nInsts = (size - u16_unaligned(data)) / HERAD_INST_SIZE;
	if (nInsts == 0)
	{
		AdPlug_LogWrite("HERAD: M32 files are not supported.\n");
		goto failure;
	}
	offset = u16_unaligned(data + 2);
	if (offset != 0x32 && offset != 0x52)
	{
		AdPlug_LogWrite("HERAD: Wrong first track offset.\n");
		goto failure;
	}
	AGD = offset == 0x52;
	wLoopStart = u16_unaligned(data + 0x2C);
	wLoopEnd = u16_unaligned(data + 0x2E);
	wLoopCount = u16_unaligned(data + 0x30);
	wSpeed = u16_unaligned(data + 0x32);
	if (wSpeed == 0)
	{
		AdPlug_LogWrite("HERAD: Speed is not defined.\n");
		goto failure;
	}
	nTracks = 0;
	for (int i = 0; i < HERAD_MAX_TRACKS; i++)
	{
		if (u16_unaligned(data + 2 + i * 2) == 0)
			break;
		nTracks++;
	}
	track = new herad_trk[nTracks];
	chn = new herad_chn[nTracks];
	for (int i = 0; i < nTracks; i++)
	{
		offset = u16_unaligned(data + 2 + i * 2) + 2;
		uint16_t next = (i < HERAD_MAX_TRACKS - 1) ? u16_unaligned(data + 2 + (i + 1) * 2) + 2 : u16_unaligned(data);
		if (next <= 2) next = u16_unaligned(data);

		track[i].size = next - offset;
		track[i].data = new uint8_t[track[i].size];
		memcpy(track[i].data, data + offset, track[i].size);
	}
	inst = new herad_inst[nInsts];
	offset = u16_unaligned(data);
	v2 = false;
	for (int i = 0; i < nInsts; i++)
	{
		memcpy(inst[i].data, data + offset + i * HERAD_INST_SIZE, HERAD_INST_SIZE);
		if (!v2 && inst[i].param.mode == HERAD_INSTMODE_KMAP)
			v2 = true;
	}
	if (!v2)
	{
		// Aggressive detection for HERAD version 2 without keymap instruments
		// (if version 1 event parser reports error, it's version 2)
		v2 = (validTracks() == 1);
	}
	delete[] data;
	goto good;

failure:
	delete[] data;
	return false;

good:
	rewind(0);
	return true;
}

void CheradPlayer::rewind(int subsong)
{
	uint32_t j;
	wTime = 0;
	songend = false;

	ticks_pos = ~0; // there's always 1 excess tick at start
	total_ticks = 0;
	loop_pos = ~0;
	loop_times = 1;

	for (int i = 0; i < nTracks; i++)
	{
		track[i].pos = 0;
		j = 0;
		while (track[i].pos < track[i].size)
		{
			j += GetTicks(i);
			switch (track[i].data[track[i].pos++] & 0xF0)
			{
			case 0x80:	// Note Off
				track[i].pos += (v2 ? 1 : 2);
				break;
			case 0x90:	// Note On
			case 0xA0:	// Unused
			case 0xB0:	// Unused
				track[i].pos += 2;
				break;
			case 0xC0:	// Program Change
			case 0xD0:	// Aftertouch
			case 0xE0:	// Pitch Bend
				track[i].pos++;
				break;
			default:
				track[i].pos = track[i].size;
				break;
			}
		}
		if (j > total_ticks)
			total_ticks = j;
		track[i].pos = 0;
		track[i].counter = 0;
		track[i].ticks = 0;
		chn[i].program = 0;
		chn[i].playprog = 0;
		chn[i].note = 0;
		chn[i].keyon = false;
		chn[i].bend = HERAD_BEND_CENTER;
		chn[i].slide_dur = 0;
	}
	if (v2)
	{
		if (!wLoopStart || wLoopCount) wLoopStart = 1; // if loop not specified, start from beginning
		if (!wLoopEnd || wLoopCount) wLoopEnd = getpatterns() + 1; // till the end
		if (wLoopCount) wLoopCount = 0; // repeats forever
	}

	opl->init();
	opl->write(1, 32); // Enable Waveform Select
	opl->write(0xBD, 0); // Disable Percussion Mode
	opl->write(8, 64); // Enable Note-Sel
	if (AGD)
	{
		opl->setchip(1);
		opl->write(5, 1); // Enable OPL3
		opl->write(4, 0); // Disable 4OP Mode
		opl->setchip(0);
	}
}

/*
 * Get delta ticks (t - track index)
 */
uint32_t CheradPlayer::GetTicks(uint8_t t)
{
	uint32_t result = 0;
	do
	{
		result <<= 7;
		result |= track[t].data[track[t].pos] & 0x7F;
	} while (track[t].data[track[t].pos++] & 0x80 && track[t].pos < track[t].size);
	return result;
}

/*
 * Execute event (t - track index)
 */
void CheradPlayer::executeCommand(uint8_t t)
{
	uint8_t status, note, par;

	if (t >= nTracks)
		return;

	if (t >= (AGD ? HERAD_NUM_VOICES * 2 : HERAD_NUM_VOICES))
	{
		track[t].pos = track[t].size;
		return;
	}

	// execute MIDI command
	status = track[t].data[track[t].pos++];
	if (status == 0xFF)
	{
		track[t].pos = track[t].size;
	}
	else
	{
		switch (status & 0xF0)
		{
		case 0x80:	// Note Off
			note = track[t].data[track[t].pos++];
			par = (v2 ? 0 : track[t].data[track[t].pos++]);
			ev_noteOff(t, note, par);
			break;
		case 0x90:	// Note On
			note = track[t].data[track[t].pos++];
			par = track[t].data[track[t].pos++];
			ev_noteOn(t, note, par);
			break;
		case 0xA0:	// Unused
		case 0xB0:	// Unused
			track[t].pos += 2;
			break;
		case 0xC0:	// Program Change
			par = track[t].data[track[t].pos++];
			ev_programChange(t, par);
			break;
		case 0xD0:	// Aftertouch
			par = track[t].data[track[t].pos++];
			ev_aftertouch(t, par);
			break;
		case 0xE0:	// Pitch Bend
			par = track[t].data[track[t].pos++];
			ev_pitchBend(t, par);
			break;
		default:
			track[t].pos = track[t].size;
			break;
		}
	}
}

void CheradPlayer::ev_noteOn(uint8_t ch, uint8_t note, uint8_t vel)
{
	int8_t macro;

	if (chn[ch].keyon)
	{
		// turn off last active note
		chn[ch].keyon = false;
		playNote(ch, chn[ch].note, HERAD_NOTE_OFF);
	}
	if (v2 && inst[chn[ch].program].param.mode == HERAD_INSTMODE_KMAP)
	{
		// keymap is used
		int8_t mp = note - (inst[chn[ch].program].keymap.offset + 24);
		if (mp < 0 || mp >= HERAD_INST_SIZE - 4)
			return; // if not in range, skip note
		chn[ch].playprog = inst[chn[ch].program].keymap.index[mp];
		changeProgram(ch, chn[ch].playprog);
	}
	chn[ch].note = note;
	chn[ch].keyon = true;
	chn[ch].bend = HERAD_BEND_CENTER;
	if (v2 && inst[chn[ch].playprog].param.mode == HERAD_INSTMODE_KMAP)
		return; // single keymapped instrument can't be keymap (avoid recursion)
	playNote(ch, note, HERAD_NOTE_ON);
	macro = inst[chn[ch].playprog].param.mc_mod_out_vel;
	if (macro != 0)
		macroModOutput(ch, chn[ch].playprog, macro, vel);
	macro = inst[chn[ch].playprog].param.mc_car_out_vel;
	if (macro != 0)
		macroCarOutput(ch, chn[ch].playprog, macro, vel);
	macro = inst[chn[ch].playprog].param.mc_fb_vel;
	if (macro != 0)
		macroFeedback(ch, chn[ch].playprog, macro, vel);
}

void CheradPlayer::ev_noteOff(uint8_t ch, uint8_t note, uint8_t vel)
{
	if (note != chn[ch].note || !chn[ch].keyon)
		return;
	chn[ch].keyon = false;
	playNote(ch, note, HERAD_NOTE_OFF);
}

void CheradPlayer::ev_programChange(uint8_t ch, uint8_t prog)
{
	if (prog >= nInsts) // out of index
		return;
	chn[ch].program = prog;
	chn[ch].playprog = prog;
	changeProgram(ch, prog);
}

void CheradPlayer::ev_aftertouch(uint8_t ch, uint8_t vel)
{
	int8_t macro;

	if (v2) // version 2 ignores this event
		return;
	macro = inst[chn[ch].playprog].param.mc_mod_out_at;
	if (macro != 0)
		macroModOutput(ch, chn[ch].playprog, macro, vel);
	macro = inst[chn[ch].playprog].param.mc_car_out_at;
	if (macro != 0 && inst[chn[ch].playprog].param.mc_car_out_vel != 0)
		macroCarOutput(ch, chn[ch].playprog, macro, vel);
	macro = inst[chn[ch].playprog].param.mc_fb_at;
	if (macro != 0)
		macroFeedback(ch, chn[ch].playprog, macro, vel);
}

void CheradPlayer::ev_pitchBend(uint8_t ch, uint8_t bend)
{
	chn[ch].bend = bend;
	if (chn[ch].keyon) // update pitch
		playNote(ch, chn[ch].note, HERAD_NOTE_UPDATE);
}

/*
 * Play Note (c - channel, note number, note state - see HERAD_NOTE_*)
 */
void CheradPlayer::playNote(uint8_t c, uint8_t note, uint8_t state)
{
	if (inst[chn[c].playprog].param.mc_transpose != 0)
		macroTranspose(&note, chn[c].playprog);
	note = (note - 24) & 0xFF;
	if (state != HERAD_NOTE_UPDATE && note >= 0x60)
		note = 0; // clip too low/high notes
	int8_t oct = note / HERAD_NUM_NOTES;
	int8_t key = note % HERAD_NUM_NOTES;
	if (state != HERAD_NOTE_UPDATE && inst[chn[c].playprog].param.mc_slide_dur)
	{
		chn[c].slide_dur = (state == HERAD_NOTE_ON ? inst[chn[c].playprog].param.mc_slide_dur : 0);
	}
	uint8_t bend = chn[c].bend;
	int16_t amount, detune = 0;
	uint8_t amount_lo, amount_hi;
	if (!(inst[chn[c].playprog].param.mc_slide_coarse & 1))
	{ // fine tune
		if (bend - HERAD_BEND_CENTER < 0)
		{ // slide down
			amount = HERAD_BEND_CENTER - bend;
			amount_lo = (amount >> 5);
			amount_hi = (amount << 3) & 0xFF;
			key -= amount_lo;

			if (key < 0)
			{
				key += HERAD_NUM_NOTES;
				oct--;
			}
			if (oct < 0)
			{
				key = 0;
				oct = 0;
			}
			detune = -1 * ((fine_bend[key] * amount_hi) >> 8);
		}
		else
		{ // slide up
			amount = bend - HERAD_BEND_CENTER;
			amount_lo = (amount >> 5);
			amount_hi = (amount << 3) & 0xFF;
			key += amount_lo;

			if (key >= HERAD_NUM_NOTES)
			{
				key -= HERAD_NUM_NOTES;
				oct++;
			}
			detune = (fine_bend[key + 1] * amount_hi) >> 8;
		}
	}
	else
	{ // coarse tune
		uint8_t offset;
		if (bend - HERAD_BEND_CENTER < 0)
		{ // slide down
			amount = HERAD_BEND_CENTER - bend;
			key -= amount / 5;

			if (key < 0)
			{
				key += HERAD_NUM_NOTES;
				oct--;
			}
			if (oct < 0)
			{
				key = 0;
				oct = 0;
			}
			offset = (amount % 5) + (key >= 6 ? 5 : 0);
			detune = -1 * coarse_bend[offset];
		}
		else
		{ // slide up
			amount = bend - HERAD_BEND_CENTER;
			key += amount / 5;

			if (key >= HERAD_NUM_NOTES)
			{
				key -= HERAD_NUM_NOTES;
				oct++;
			}
			offset = (amount % 5) + (key >= 6 ? 5 : 0);
			detune = coarse_bend[offset];
		}
	}
	setFreq(c, oct, FNum[key] + detune, state != HERAD_NOTE_OFF);
}

/*
 * Set Frequency and Key (c - channel, octave, frequency, note on)
 */
void CheradPlayer::setFreq(uint8_t c, uint8_t oct, uint16_t freq, bool on)
{
	uint8_t reg, val;

	if (c >= HERAD_NUM_VOICES) opl->setchip(1);

	reg = 0xA0 + (c % HERAD_NUM_VOICES);
	val = freq & 0xFF;
	opl->write(reg, val);
	reg = 0xB0 + (c % HERAD_NUM_VOICES);
	val = ((freq >> 8) & 3) |
		((oct & 7) << 2) |
		((on ? 1 : 0) << 5);
	opl->write(reg, val);

	if (c >= HERAD_NUM_VOICES) opl->setchip(0);
}

/*
 * Change Program (c - channel, i - instrument index)
 */
void CheradPlayer::changeProgram(uint8_t c, uint8_t i)
{
	uint8_t reg, val;

	if (v2 && inst[i].param.mode == HERAD_INSTMODE_KMAP)
		return;

	if (c >= HERAD_NUM_VOICES) opl->setchip(1);

	// Amp Mod / Vibrato / EG type / Key Scaling / Multiple
	reg = 0x20 + slot_offset[c % HERAD_NUM_VOICES];
	val = (inst[i].param.mod_mul & 15) |
		((inst[i].param.mod_ksr & 1) << 4) |
		((inst[i].param.mod_eg > 0 ? 1 : 0) << 5) |
		((inst[i].param.mod_vib & 1) << 6) |
		((inst[i].param.mod_am & 1) << 7);
	opl->write(reg, val);
	reg += 3;
	val = (inst[i].param.car_mul & 15) |
		((inst[i].param.car_ksr & 1) << 4) |
		((inst[i].param.car_eg > 0 ? 1 : 0) << 5) |
		((inst[i].param.car_vib & 1) << 6) |
		((inst[i].param.car_am & 1) << 7);
	opl->write(reg, val);

	// Key scaling level / Output level
	reg = 0x40 + slot_offset[c % HERAD_NUM_VOICES];
	val = (inst[i].param.mod_out & 63) |
		((inst[i].param.mod_ksl & 3) << 6);
	opl->write(reg, val);
	reg += 3;
	val = (inst[i].param.car_out & 63) |
		((inst[i].param.car_ksl & 3) << 6);
	opl->write(reg, val);

	// Attack Rate / Decay Rate
	reg = 0x60 + slot_offset[c % HERAD_NUM_VOICES];
	val = (inst[i].param.mod_D & 15) |
		((inst[i].param.mod_A & 15) << 4);
	opl->write(reg, val);
	reg += 3;
	val = (inst[i].param.car_D & 15) |
		((inst[i].param.car_A & 15) << 4);
	opl->write(reg, val);

	// Sustain Level / Release Rate
	reg = 0x80 + slot_offset[c % HERAD_NUM_VOICES];
	val = (inst[i].param.mod_R & 15) |
		((inst[i].param.mod_S & 15) << 4);
	opl->write(reg, val);
	reg += 3;
	val = (inst[i].param.car_R & 15) |
		((inst[i].param.car_S & 15) << 4);
	opl->write(reg, val);

	// Panning / Feedback strength / Connection type
	reg = 0xC0 + (c % HERAD_NUM_VOICES);
	val = (inst[i].param.con > 0 ? 0 : 1) |
		((inst[i].param.feedback & 7) << 1) |
		((AGD ? (inst[i].param.pan == 0 || inst[i].param.pan > 3 ? 3 : inst[i].param.pan) : 0) << 4);
	opl->write(reg, val);

	// Wave Select
	reg = 0xE0 + slot_offset[c % HERAD_NUM_VOICES];
	val = inst[i].param.mod_wave & (AGD ? 7 : 3);
	opl->write(reg, val);
	reg += 3;
	val = inst[i].param.car_wave & (AGD ? 7 : 3);
	opl->write(reg, val);

	if (c >= HERAD_NUM_VOICES) opl->setchip(0);
}

/*
 * Macro: Change Modulator Output (c - channel, i - instrument index, sensitivity, level)
 */
void CheradPlayer::macroModOutput(uint8_t c, uint8_t i, int8_t sens, uint8_t level)
{
	uint8_t reg, val;
	uint16_t output;

	if (sens < -4 || sens > 4)
		return;

	if (sens < 0)
	{
		output = (level >> (sens + 4) > 63 ? 63 : level >> (sens + 4));
	}
	else
	{
		output = ((0x80 - level) >> (4 - sens) > 63 ? 63 : (0x80 - level) >> (4 - sens));
	}
	output += inst[i].param.mod_out;
	if (output > 63) output = 63;

	if (c >= HERAD_NUM_VOICES) opl->setchip(1);

	// Key scaling level / Output level
	reg = 0x40 + slot_offset[c % HERAD_NUM_VOICES];
	val = (output & 63) |
		((inst[i].param.mod_ksl & 3) << 6);
	opl->write(reg, val);

	if (c >= HERAD_NUM_VOICES) opl->setchip(0);
}

/*
 * Macro: Change Carrier Output (c - channel, i - instrument index, sensitivity, level)
 */
void CheradPlayer::macroCarOutput(uint8_t c, uint8_t i, int8_t sens, uint8_t level)
{
	uint8_t reg, val;
	uint16_t output;

	if (sens < -4 || sens > 4)
		return;

	if (sens < 0)
	{
		output = (level >> (sens + 4) > 63 ? 63 : level >> (sens + 4));
	}
	else
	{
		output = ((0x80 - level) >> (4 - sens) > 63 ? 63 : (0x80 - level) >> (4 - sens));
	}
	output += inst[i].param.car_out;
	if (output > 63) output = 63;

	if (c >= HERAD_NUM_VOICES) opl->setchip(1);

	// Key scaling level / Output level
	reg = 0x43 + slot_offset[c % HERAD_NUM_VOICES];
	val = (output & 63) |
		((inst[i].param.car_ksl & 3) << 6);
	opl->write(reg, val);

	if (c >= HERAD_NUM_VOICES) opl->setchip(0);
}

/*
 * Macro: Change Feedback (c - channel, i - instrument index, sensitivity, level)
 */
void CheradPlayer::macroFeedback(uint8_t c, uint8_t i, int8_t sens, uint8_t level)
{
	uint8_t reg, val;
	uint8_t feedback;

	if (sens < -6 || sens > 6)
		return;

	if (sens < 0)
	{
		feedback = (level >> (sens + 7) > 7 ? 7 : level >> (sens + 7));
	}
	else
	{
		feedback = ((0x80 - level) >> (7 - sens) > 7 ? 7 : (0x80 - level) >> (7 - sens));
	}
	feedback += inst[i].param.feedback;
	if (feedback > 7) feedback = 7;

	if (c >= HERAD_NUM_VOICES) opl->setchip(1);

	// Panning / Feedback strength / Connection type
	reg = 0xC0 + (c % HERAD_NUM_VOICES);
	val = (inst[i].param.con > 0 ? 0 : 1) |
		((feedback & 7) << 1) |
		((AGD ? (inst[i].param.pan == 0 || inst[i].param.pan > 3 ? 3 : inst[i].param.pan) : 0) << 4);
	opl->write(reg, val);

	if (c >= HERAD_NUM_VOICES) opl->setchip(0);
}

/*
 * Macro: Root Note Transpose (note, i - instrument index)
 */
void CheradPlayer::macroTranspose(uint8_t * note, uint8_t i)
{
	uint8_t tran = inst[i].param.mc_transpose;
	uint8_t diff = (tran - 0x31) & 0xFF;
	if (v2 && diff < 0x60)
		*note = (diff + 0x18) & 0xFF;
	else
		*note = (*note + tran) & 0xFF;
}

/*
 * Macro: Pitch Bend Slide (c - channel)
 */
void CheradPlayer::macroSlide(uint8_t c)
{
	if (!chn[c].slide_dur)
		return;

	chn[c].slide_dur--;
	chn[c].bend += inst[chn[c].playprog].param.mc_slide_range;

	if (!(chn[c].note & 0x7F))
		return;
	playNote(c, chn[c].note, HERAD_NOTE_UPDATE);
}

void CheradPlayer::processEvents()
{
	uint8_t i;
	songend = true;

	if (wLoopStart && wLoopEnd && (ticks_pos + 1) % HERAD_MEASURE_TICKS == 0 && (ticks_pos + 1) / HERAD_MEASURE_TICKS + 1 == wLoopStart)
	{
		loop_pos = ticks_pos;
		for (i = 0; i < nTracks; i++)
		{
			loop_data[i].counter = track[i].counter;
			loop_data[i].ticks = track[i].ticks;
			loop_data[i].pos = track[i].pos;
		}
	}
	for (i = 0; i < nTracks; i++)
	{
		if (chn[i].slide_dur > 0 && chn[i].keyon)
			macroSlide(i);
		if (track[i].pos >= track[i].size)
			continue;
		songend = false; // track is not finished
		if (!track[i].counter)
		{
			bool first = track[i].pos == 0;
			track[i].ticks = GetTicks(i);
			if (first && track[i].ticks)
				track[i].ticks++; // workaround to synchronize tracks (there's always 1 excess tick at start)
		}
		if (++track[i].counter >= track[i].ticks)
		{
			track[i].counter = 0;
			while (track[i].pos < track[i].size)
			{
				executeCommand(i);
				if (track[i].pos >= track[i].size) {
					break;
				}
				else if (!track[i].data[track[i].pos]) // if next delay is zero
				{
					track[i].pos++;
				}
				else break;
			}
		}
		else if (track[i].ticks >= 0x8000)
		{
			track[i].pos = track[i].size;
			track[i].counter = track[i].ticks;
		}
	}
	if (!songend)
		ticks_pos++;
	if (wLoopStart && wLoopEnd && (ticks_pos == total_ticks || (ticks_pos % HERAD_MEASURE_TICKS == 0 && ticks_pos / HERAD_MEASURE_TICKS + 1 == wLoopEnd)))
	{
#ifdef HERAD_USE_LOOPING
		if (!wLoopCount)
			songend = true;
		else if (songend && loop_times < wLoopCount)
			songend = false;

		if (!wLoopCount || loop_times < wLoopCount)
		{
			ticks_pos = loop_pos;
			for (i = 0; i < nTracks; i++)
			{
				track[i].counter = loop_data[i].counter;
				track[i].ticks = loop_data[i].ticks;
				track[i].pos = loop_data[i].pos;
			}
			if (wLoopCount)
				loop_times++;
		}
#endif
	}
}

bool CheradPlayer::update()
{
	wTime = wTime - 256;
	if (wTime < 0)
	{
		wTime = wTime + wSpeed;
		processEvents();
	}
	return !songend;
}
