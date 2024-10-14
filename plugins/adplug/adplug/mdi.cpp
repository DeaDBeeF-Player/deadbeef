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
 * mdi.cpp - AdLib SMF (MDI) Player by Stas'M <binarymaster@mail.ru>
 *
 * Based on MIDIPLAY.C by Dale Glowinski, Ad Lib Inc.
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/MDI_Format
 * http://www.vgmpf.com/Wiki/index.php?title=MDI
 */

#include <string.h>

#include "mdi.h"

/*** public methods *************************************/

CPlayer *CmdiPlayer::factory(Copl *newopl)
{
	return new CmdiPlayer(newopl);
}

bool CmdiPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename); if(!f) return false;

	// file validation
	if (!fp.extension(filename, ".mdi"))
	{
		fp.close(f);
		return false;
	}
	if (fp.filesize(f) < MIDI_MIN_SIZE)
	{
		fp.close(f);
		return false;
	}

	char chunk[MIDI_CHUNK_SIZE + 1];
	chunk[MIDI_CHUNK_SIZE] = 0;

	// header validation
	f->readString(chunk, MIDI_CHUNK_SIZE);
	if (strcmp(chunk, "MThd"))
	{
		fp.close(f);
		return false;
	}
	f->setFlag(binio::BigEndian, true);
	if (f->readInt(4) != MIDI_HEAD_SIZE || // chunk size
		f->readInt(2) != 0 || // MIDI Type must be 0
		f->readInt(2) != 1)   // track count must be 1
	{
		fp.close(f);
		return false;
	}
	// division (UINT16_BE)
	division = f->readInt(2);
	// track validation
	f->readString(chunk, MIDI_CHUNK_SIZE);
	if (strcmp(chunk, "MTrk"))
	{
		fp.close(f);
		return false;
	}
	// chunk size (UINT32_BE)
	size = f->readInt(4);
	// data size validation
	if (fp.filesize(f) < MIDI_MIN_SIZE + size)
	{
		fp.close(f);
		return false;
	}
	// load section
	data = new uint8_t[size];
	f->readString((char *)data, size);

	fp.close(f);
	rewind(0);
	return true;
}

void CmdiPlayer::frontend_rewind(int subsong)
{
	// set default MIDI tempo
	SetTempo(MIDI_DEF_TEMPO);
	pos = 0; songend = false;

	// midiplay uses rhythm mode by default
	SetRhythmMode(1);

	for (int i = 0; i < MAX_VOICES; i++)
	{
		volume[i] = 0;
		SetDefaultInstrument(i);
	}
	counter = 0;
	ticks = 0;
}

/*
	Change the tempo.
*/
void CmdiPlayer::SetTempo(uint32_t tempo)
{
	if (!tempo) tempo = MIDI_DEF_TEMPO;
	timer = division * 1000000 / (float)tempo;
}

uint32_t CmdiPlayer::GetVarVal()
{
	uint32_t result = 0;
	do
	{
		result <<= 7;
		result |= data[pos] & 0x7F;
	} while (data[pos++] & 0x80 && pos < size);
	return result;
}

void CmdiPlayer::executeCommand()
{
	uint32_t len, tempo;
	uint8_t new_status = 0, meta, voice, note, vol;
	uint16_t code, pitch;

	// execute MIDI command
	if (data[pos] < 0x80)
	{
		// running status
		new_status = status;
	}
	else
		new_status = data[pos++];
	if (new_status == STOP_FC)
	{
		pos = size;
	}
	else if (new_status == SYSEX_F0 || new_status == SYSEX_F7)
	{
		/* skip over system exclusive event */
		len = GetVarVal();
		pos += len;
	}
	else if (new_status == META)
	{
		/* Process meta-event */
		meta = data[pos++];
		len = GetVarVal();
		switch (meta)
		{
		case END_OF_TRACK:
			pos = size - len; // pos incremented later
			break;
		case TEMPO:
			if (len >= 3)
			{
				tempo = data[pos] << 16 | data[pos + 1] << 8 | data[pos + 2];
				SetTempo(tempo);
			}
			break;
		case SEQ_SPECIFIC:
			if (len >= META_MIN_SIZE)
			{
				/* Ad Lib midi ID is 00 00 3f. */
				if (data[pos] == 0 &&
					data[pos + 1] == 0 &&
					data[pos + 2] == 0x3f)
				{
					/*
					The first two bytes after the ID contain the Ad Lib event code.
					The following bytes contain the data pertaining to the event.
					*/
					code = data[pos + 3] << 8 | data[pos + 4];
					if (code == ADLIB_TIMBRE && len >= META_MIN_SIZE + ADLIB_INST_LEN)
					{
						/*
						Instrument change code.  First byte of data contains voice number.
						Following bytes contain instrument parameters.
						*/
						voice = data[pos + 5];
						int index = load_instrument_data(&data[pos + META_MIN_SIZE], ADLIB_INST_LEN);
						SetInstrument(voice, index);
					}
					else if (code == ADLIB_RHYTHM) {
						/* Melo/perc mode code.  0 is melodic, !0 is percussive. */
						SetRhythmMode((int)data[pos + 5]);
					}
					else if (code == ADLIB_PITCH) {
						/* Sets the interval over which pitch bend changes will be applied. */
						SetPitchRange((int)data[pos + 5]);
					}
				}
			}
			break;
		}
		pos += len;
	}
	else
	{
		status = new_status;
		voice = status & 0xF;
		switch (status & 0xF0)
		{
		case NOTE_OFF:
			pos += 2;
			if (voice >= MAX_VOICES)
				break;
			NoteOff(voice);
			break;
		case NOTE_ON:
			note = data[pos++];
			vol = data[pos++];
			if (voice >= MAX_VOICES)
				break;
			if (!vol)
			{
				/* A note-on with a volume of 0 is equivalent to a note-off. */
				NoteOff(voice);
				volume[voice] = vol;
			}
			else
			{
				/* Regular note-on */
				if (vol != volume[voice])
				{
					SetVolume(voice, vol);
					volume[voice] = vol;
				}
				NoteOn(voice, note);
			}
			break;
		case AFTER_TOUCH:
			pos++; // skip note
			vol = data[pos++];
			if (voice >= MAX_VOICES)
				break;
			if (vol != volume[voice])
			{
				SetVolume(voice, vol);
				volume[voice] = vol;
			}
			break;
		case CONTROL_CHANGE:
			/* unused */
			pos += 2;
			break;
		case PROG_CHANGE:
			/* unused */
			pos += 1;
			break;
		case CHANNEL_PRESSURE:
			vol = data[pos++];
			if (voice >= MAX_VOICES)
				break;
			if (vol != volume[voice])
			{
				SetVolume(voice, vol);
				volume[voice] = vol;
			}
			break;
		case PITCH_BEND:
			pitch = data[pos++];
			pitch |= data[pos++] << 7;
			if (voice >= MAX_VOICES)
				break;
			ChangePitch(voice, pitch);
			break;
		default:
			/*
			A bad status byte ( or unimplemented MIDI command) has been encontered.
			Skip bytes until next timing byte followed by status byte.
			*/
			while (data[pos++] < NOTE_OFF && pos < size);
			if (pos >= size)
				break;
			break;
		}
	}
}

bool CmdiPlayer::update()
{
	if (!counter)
	{
		ticks = GetVarVal();
	}
	if (++counter >= ticks)
	{
		counter = 0;
		while (pos < size)
		{
			executeCommand();
			if (pos >= size) {
				pos = 0;
				songend = true;
				break;
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
