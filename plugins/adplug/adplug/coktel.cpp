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
 * coktel.cpp - Coktel Vision ADL player by Stas'M <binarymaster@mail.ru>
 *
 * REFERENCES:
 * http://www.vgmpf.com/Wiki/index.php?title=ADL_(Coktel_Vision)
 * https://github.com/DrMcCoy/CoktelADL2VGM/blob/master/src/adlib/adlplayer.cpp
 * https://github.com/scummvm/scummvm/blob/master/engines/gob/sound/adlplayer.cpp
 */

#include <string.h>

#include "coktel.h"

#ifdef DEBUG
#include "debug.h"
#endif

/*** public methods *************************************/

CPlayer *CcoktelPlayer::factory(Copl *newopl)
{
	return new CcoktelPlayer(newopl);
}

bool CcoktelPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename);
	if (!f)
		return false;

	// file validation
	if (!fp.extension(filename, ".adl"))
	{
		fp.close(f);
		return false;
	}
	if (fp.filesize(f) < COK_MIN_SIZE)
	{
		fp.close(f);
		return false;
	}

	soundMode = static_cast<uint8_t>(f->readInt(1));
	nrTimbre = static_cast<uint8_t>(f->readInt(1));
	uint8_t reserved = static_cast<uint8_t>(f->readInt(1));

	// validate header and data size
	if (soundMode > 1 ||
		nrTimbre == 0xFF ||
		reserved != 0 ||
		fp.filesize(f) < (unsigned)(COK_HEADER_LEN + (nrTimbre + 1) * TIMBRE_DEF_SIZE + 1))
	{
		fp.close(f);
		return false;
	}

	// read timbre data
	nrTimbre++;
	insts = new adl_inst[nrTimbre];
	for (int i = 0; i < nrTimbre; i++)
	{
		for (int j = 0; j < ADLIB_INST_LEN; j++)
		{
			uint16_t val = static_cast<uint16_t>(f->readInt(2));
			insts[i].initial[j] = val & 0xFF;
		}
		insts[i].backend_index = -1;
	}

	// read MIDI data
	size = fp.filesize(f) - COK_HEADER_LEN - nrTimbre * TIMBRE_DEF_SIZE;
	data = new uint8_t[size];
	f->readString((char *)data, size);

	fp.close(f);
	rewind(0);
	return true;
}

void CcoktelPlayer::frontend_rewind(int subsong)
{
	pos = 0;
	songend = false;
	first_tick = false;

	SetRhythmMode(soundMode);

	// reset modified instruments
	for (int i = 0; i < nrTimbre; i++)
	{
		memcpy(insts[i].modified, insts[i].initial, ADLIB_INST_LEN);
		insts[i].backend_index = load_instrument_data(&insts[i].initial[0], ADLIB_INST_LEN);
	}
	// reset voices
	for (int i = 0; i < MAX_VOICES; i++)
	{
		timbre[i] = 0;
	}
	for (int i = 0; i < (soundMode ? kNumPercussiveVoices : kNumMelodicVoices); i++)
	{
		SetInstrument(i, insts[timbre[i]].backend_index);
		SetVolume(i, 127);
	}

	counter = 0;
	ticks = 0;
	modifyTimbre = 0xFF;
}

void CcoktelPlayer::executeCommand()
{
	uint8_t status, voice, note, val;
	uint16_t pitch;

	// execute MIDI command
	status = data[pos++];
	if (status == COK_END_OF_SONG)
	{
		pos = size;
	}
	else if (status == COK_SET_MOD_TIMBRE)
	{
		modifyTimbre = data[pos++];
	}
	else if (status > COK_MODIFY_TIMBRE)
	{
		note = data[pos++]; // what parameter to modify
		val = data[pos++]; // set this value for specified parameter

		if (insts && modifyTimbre != 0xFF && modifyTimbre < nrTimbre)
		{
			insts[modifyTimbre].modified[note] = val;
			insts[modifyTimbre].backend_index = load_instrument_data(&insts[modifyTimbre].modified[0], ADLIB_INST_LEN);

			// update timbre for voices where it's used
			for (int i = 0; i < (soundMode ? kNumPercussiveVoices : kNumMelodicVoices); i++)
			{
				if (timbre[i] == modifyTimbre)
				{
					SetInstrument(i, insts[modifyTimbre].backend_index);
				}
			}
		}
	}
	else
	{
		// voice command
		voice = status & 0xF;

		switch (status & 0xF0)
		{
			case COK_NOTE_ON_VOL:
				note = data[pos++];
				val = data[pos++];
				if (voice >= MAX_VOICES)
					break;
				SetVolume(voice, val);
				NoteOn(voice, note);
				break;

			case COK_NOTE_OFF:
				if (voice >= MAX_VOICES)
					break;
				NoteOff(voice);
				break;

			case COK_NOTE_ON:
				note = data[pos++];
				if (voice >= MAX_VOICES)
					break;
				NoteOn(voice, note);
				break;

			case COK_PITCH_BEND:
				pitch = data[pos++] << 7;
				if (voice >= MAX_VOICES)
					break;
				ChangePitch(voice, pitch);
				break;

			case COK_VOLUME_SLIDE:
				val = data[pos++];
				if (voice >= MAX_VOICES)
					break;
				SetVolume(voice, val);
				break;

			case COK_TIMBRE_CHANGE:
				val = data[pos++];
				if (voice >= MAX_VOICES)
					break;
				if (insts)
				{
					if (val < nrTimbre)
					{
						timbre[voice] = val;
						SetInstrument(voice, insts[val].backend_index);
					}
					#ifdef DEBUG
					else
					{
						AdPlug_LogWrite("Timbre not found: %d\n", val);
					}
					#endif
				}
				break;

			default:
				#ifdef DEBUG
				AdPlug_LogWrite("Unsupported command: 0x%02X. Stopping playback.\n", status);
				#endif
				pos = size;
				break;
		}
	}
}

bool CcoktelPlayer::update()
{
	if (pos >= size)
	{
		rewind(0);
		songend = true;
	}
	if (!counter)
	{
		ticks = data[pos++];
		if (ticks & 0x80)
			ticks = ((ticks & ~0x80) << 8) | data[pos++];
		if (ticks && !first_tick)
		{
			// skip first delay
			ticks = 0;
			first_tick = true;
		}
	}
	if (++counter >= ticks)
	{
		counter = 0;
		while (pos < size)
		{
			executeCommand();
			if (pos >= size)
			{
				return false;
			}
			else if (!data[pos]) // if next delay is zero
			{
				pos++;
			}
			else break;
		}
	}
	return !songend;
}
