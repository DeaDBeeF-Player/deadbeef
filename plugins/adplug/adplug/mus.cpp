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
 * mus.cpp - AdLib MIDI Music File Player by Stas'M <binarymaster@mail.ru> and Wraithverge <liam82067@yahoo.com>
 *
 * Based on PLAY.C by Marc Savary, Ad Lib Inc.
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/AdLib_MIDI_Format
 * http://www.shikadi.net/moddingwiki/IMS_Format
 * http://www.shikadi.net/moddingwiki/AdLib_Timbre_Bank_Format
 * http://www.shikadi.net/moddingwiki/AdLib_Instrument_Bank_Format
 * http://www.vgmpf.com/Wiki/index.php?title=MUS_(AdLib)
 * http://www.vgmpf.com/Wiki/index.php?title=SND_(AdLib)
 */

#include <cstring>
#include <stdio.h>

#include "mus.h"

#ifdef DEBUG
#include "debug.h"
#endif

/*** public methods *************************************/

CPlayer *CmusPlayer::factory(Copl *newopl)
{
	return new CmusPlayer(newopl);
}

std::string CmusPlayer::gettype()
{
	char tmpstr[30];

	if (isIMS)
		sprintf(tmpstr, "IMPlay Song Format v%d.%d", majorVersion, minorVersion);
	else
		sprintf(tmpstr, "AdLib MIDI Format v%d.%d", majorVersion, minorVersion);
	return std::string(tmpstr);
}

bool CmusPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename); if(!f) return false;

	// file validation
	if (!fp.extension(filename, ".mus") &&
		!fp.extension(filename, ".ims"))
	{
		fp.close(f);
		return false;
	}
	if (fp.filesize(f) < HEADER_LEN)
	{
		fp.close(f);
		return false;
	}

	// read file header
	isIMS = false;
	majorVersion = static_cast<uint8_t>(f->readInt(1));
	minorVersion = static_cast<uint8_t>(f->readInt(1));
	uint32_t tuneId = static_cast<uint32_t>(f->readInt(4));
	f->readString(tuneName, TUNE_NAME_SIZE);
	tickBeat = static_cast<uint8_t>(f->readInt(1));
	uint8_t beatMeasure = static_cast<uint8_t>(f->readInt(1));
	uint32_t totalTick = static_cast<uint32_t>(f->readInt(4));
	dataSize = static_cast<uint32_t>(f->readInt(4));
	uint32_t nrCommand = static_cast<uint32_t>(f->readInt(4));
	f->seek(FILLER_SIZE, binio::Add);
	soundMode = static_cast<uint8_t>(f->readInt(1));
	pitchBRange = static_cast<uint8_t>(f->readInt(1));
	basicTempo = static_cast<uint16_t>(f->readInt(2));
	f->seek(FILLER_SIZE, binio::Add);

	// validate header and data size
	if (majorVersion != 1 ||
		minorVersion != 0 ||
		tuneId != 0 ||
		tickBeat == 0 ||
		beatMeasure == 0 ||
		totalTick == 0 ||
		dataSize == 0 ||
		nrCommand == 0 ||
		fp.filesize(f) < (unsigned)(HEADER_LEN + dataSize))
	{
		fp.close(f);
		return false;
	}

	// read MIDI data
	data = new uint8_t[dataSize];
	f->readString((char *)data, dataSize);

	// read IMS timbre list (if exists)
	if (fp.filesize(f) >= (unsigned)(HEADER_LEN + dataSize) + 4 &&
		f->readInt(2) == IMS_SIGNATURE)
	{
		isIMS = true;
		nrTimbre = f->readInt(2);
		// validate post-data size
		if (fp.filesize(f) >= (unsigned)(HEADER_LEN + dataSize) + 4 + nrTimbre * TIMBRE_NAME_SIZE)
		{
			insts = new mus_inst[nrTimbre];
			// read timbre names
			for (int i = 0; i < nrTimbre; i++)
			{
				f->readString(insts[i].name, TIMBRE_NAME_SIZE);
				insts[i].name[TIMBRE_NAME_SIZE - 1] = 0;
				insts[i].loaded = false;
			}
		}
		else
			nrTimbre = 0;
	}

	fp.close(f);
	bool bankload;
	if (!insts)
	{
		// load SND timbre bank
		bankload = LoadTimbreBank(filename.substr(0, filename.length() - 3).append("snd"), fp);
		#ifndef DOS
		#ifndef WIN32
		if (!bankload) // for case-sensitive file systems
			bankload = LoadTimbreBank(filename.substr(0, filename.length() - 3).append("SND"), fp);
		#endif
		#endif
		if (!bankload)
			bankload = LoadTimbreBank(filename.substr(0, filename.length() - 3).append("tim"), fp);
		#ifndef DOS
		#ifndef WIN32
		if (!bankload) // for case-sensitive file systems
			bankload = LoadTimbreBank(filename.substr(0, filename.length() - 3).append("TIM"), fp);
		#endif
		#endif
		if (!bankload)
		{
			// fetch default bank
			size_t np = filename.find_last_of("/");
			if (np == std::string::npos)
				np = filename.find_last_of("\\");
			if (np != std::string::npos)
				bankload = LoadTimbreBank(filename.substr(0, np + 1).append("timbres.snd"), fp);
			#ifndef DOS
			#ifndef WIN32
			if (!bankload) // for case-sensitive file systems
				bankload = LoadTimbreBank(filename.substr(0, np + 1).append("TIMBRES.SND"), fp);
			#endif
			#endif
			if (!bankload)
				bankload = LoadTimbreBank(filename.substr(0, np + 1).append("timbres.tim"), fp);
			#ifndef DOS
			#ifndef WIN32
			if (!bankload) // for case-sensitive file systems
				bankload = LoadTimbreBank(filename.substr(0, np + 1).append("TIMBRES.TIM"), fp);
			#endif
			#endif
		}
	}
	else if (isIMS)	// fetch timbre data from BNK banks
	{
		// fetch bank of the song
		bankload = FetchTimbreData(filename.substr(0, filename.length() - 3).append("bnk"), fp);
		#ifndef DOS
		#ifndef WIN32
		if (!bankload) // for case-sensitive file systems
			bankload = FetchTimbreData(filename.substr(0, filename.length() - 3).append("BNK"), fp);
		#endif
		#endif
		if (!bankload)
		{
			// fetch other default banks
			size_t np = filename.find_last_of("/");
			if (np == std::string::npos)
				np = filename.find_last_of("\\");
			if (np != std::string::npos)
			{
				if (!InstsLoaded())
				{
					// fetch IMPlay bank
					bankload = FetchTimbreData(filename.substr(0, np + 1).append("implay.bnk"), fp);
					#ifndef DOS
					#ifndef WIN32
					if (!bankload) // for case-sensitive file systems
						bankload = FetchTimbreData(filename.substr(0, np + 1).append("IMPLAY.BNK"), fp);
					#endif
					#endif
				}
				if (!InstsLoaded())
				{
					// fetch standard bank
					bankload = FetchTimbreData(filename.substr(0, np + 1).append("standard.bnk"), fp);
					#ifndef DOS
					#ifndef WIN32
					if (!bankload) // for case-sensitive file systems
						bankload = FetchTimbreData(filename.substr(0, np + 1).append("STANDARD.BNK"), fp);
					#endif
					#endif
				}
			}
		}
	}
	drv = new CadlibDriver(opl);
	rewind(0);
	return true;
}

bool CmusPlayer::InstsLoaded()
{
	if (!insts) return false;
	for (int i = 0; i < nrTimbre; i++)
		if (!insts[i].loaded)
			return false;
	return true;
}

bool CmusPlayer::LoadTimbreBank(const std::string fname, const CFileProvider &fp)
{
	binistream *f = fp.open(fname);
	if (!f) {
		#ifdef DEBUG
		AdPlug_LogWrite("Timbre bank not found: %s\n", fname.c_str());
		#endif
		return false;
	}

	// file validation
	if (fp.filesize(f) < SND_HEADER_LEN)
	{
		fp.close(f);
		#ifdef DEBUG
		AdPlug_LogWrite("Timbre bank size is wrong.\n");
		#endif
		return false;
	}
 	uint8_t vMaj = static_cast<uint8_t>(f->readInt(1));
	uint8_t vMin = static_cast<uint8_t>(f->readInt(1));
	nrTimbre = static_cast<uint16_t>(f->readInt(2));
	uint16_t offsetDef = static_cast<uint16_t>(f->readInt(2));
	// validate header and data size
	if (vMaj != 1 || vMin != 0 ||
		offsetDef != SND_HEADER_LEN + nrTimbre * TIMBRE_NAME_SIZE ||
		fp.filesize(f) < SND_HEADER_LEN + nrTimbre * TIMBRE_NAME_SIZE + nrTimbre * TIMBRE_DEF_SIZE)
	{
		nrTimbre = 0;
		fp.close(f);
		#ifdef DEBUG
		AdPlug_LogWrite("Timbre bank format is incorrect.\n");
		#endif
		return false;
	}
	insts = new mus_inst[nrTimbre];
	// read timbre names
	for (int i = 0; i < nrTimbre; i++)
	{
		f->readString(insts[i].name, TIMBRE_NAME_SIZE);
		insts[i].name[TIMBRE_NAME_SIZE - 1] = 0;
	}
	// read timbre data
	for (int i = 0; i < nrTimbre; i++)
	{
		f->readString((char *)insts[i].data, TIMBRE_DEF_LEN * 2);
		insts[i].loaded = true;
	}
	fp.close(f);
	return true;
}

bool CmusPlayer::FetchTimbreData(const std::string fname, const CFileProvider &fp)
{
	binistream *f = fp.open(fname);
	if (!f) {
		#ifdef DEBUG
		AdPlug_LogWrite("Instrument bank not found: %s\n", fname.c_str());
		#endif
		return false;
	}
	// file validation
	if (fp.filesize(f) < BNK_HEADER_SIZE)
	{
		fp.close(f);
		#ifdef DEBUG
		AdPlug_LogWrite("Instrument bank size is wrong.");
		#endif
		return false;
	}
	// check bank version
	if (f->readInt(1) != 1 ||
		f->readInt(1) != 0)
	{
		fp.close(f);
		#ifdef DEBUG
		AdPlug_LogWrite("Instrument bank version is wrong.");
		#endif
		return false;
	}
	// check bank signature
	char signature[BNK_SIGNATURE_LEN + 1];
	signature[BNK_SIGNATURE_LEN] = 0;
	f->readString(signature, BNK_SIGNATURE_LEN);
	if (strcmp(signature, "ADLIB-"))
	{
		fp.close(f);
		#ifdef DEBUG
		AdPlug_LogWrite("Instrument bank signature is wrong.");
		#endif
		return false;
	}
	uint16_t numUsed = static_cast<uint16_t>(f->readInt(2));
	uint16_t numInst = static_cast<uint16_t>(f->readInt(2));
	uint32_t offsetName = static_cast<uint32_t>(f->readInt(4));
	uint32_t offsetData = static_cast<uint32_t>(f->readInt(4));
	// validate header and data size
	if (numUsed == 0 ||
		numInst == 0 ||
		numUsed > numInst ||
		offsetName == 0 ||
		offsetData == 0 ||
		offsetName > offsetData ||
		offsetName > BNK_HEADER_SIZE || /* often 0x1C, also can be 0x14 */
		fp.filesize(f) < offsetData + numInst * BNK_INST_SIZE)
	{
		fp.close(f);
		#ifdef DEBUG
		AdPlug_LogWrite("Instrument bank format is incorrect.");
		#endif
		return false;
	}
	f->seek(offsetName);
	uint8_t * names = new uint8_t[numInst * BNK_NAME_SIZE];
	f->readString((char *)names, numInst * BNK_NAME_SIZE);

	f->seek(offsetData);
	uint8_t * instData = new uint8_t[numInst * BNK_INST_SIZE];
	f->readString((char *)instData, numInst * BNK_INST_SIZE);

	fp.close(f);

	uint16_t index;
	bool match;
	for (int i = 0; i < numUsed; i++)
	{
		index = *(uint16_t *)(names + i * BNK_NAME_SIZE);
		for (int j = 0; j < nrTimbre; j++)
		{
			match = true;
			for (int k = 0; k < TIMBRE_NAME_SIZE; k++)
			{
				if (k > 0 && insts[j].name[k - 1] == 0)
					break;
				if (tolower(insts[j].name[k]) != tolower(names[i * BNK_NAME_SIZE + 3 + k]))
				{
					match = false;
					break;
				}
			}
			if (match && !insts[j].loaded && index < numInst)
			{
				for (int k = 0; k < TIMBRE_DEF_LEN; k++)
				{
					insts[j].data[k] = instData[index * BNK_INST_SIZE + 2 + k];
				}
				insts[j].loaded = true;
			}
		}
		if (InstsLoaded())
			break;
	}
	delete[] names;
	delete[] instData;
	return true;
}

void CmusPlayer::rewind(int subsong)
{
	SetTempo(basicTempo, tickBeat);
	pos = 0; songend = false;
	opl->init();
	if (drv) drv->SoundWarmInit();

	for (int i = 0; i < MAX_VOICES; i++)
		volume[i] = 0;
	counter = 0;
	ticks = 0;

	if (drv) drv->SetMode(soundMode);
	if (drv) drv->SetPitchRange(pitchBRange);
}

/*
	Change the tempo.
*/
void CmusPlayer::SetTempo(uint16_t tempo, uint8_t tickBeat)
{
	if (!tempo) tempo = basicTempo;
	timer = tempo * tickBeat / 60.0f;
}

uint32_t CmusPlayer::GetTicks()
{
	uint32_t ticks = 0;
	while (data[pos] == OVERFLOW_BYTE && pos < dataSize)
	{
		ticks += OVERFLOW_TICKS;
		pos++;
	}
	if (pos < dataSize)
		ticks += data[pos++];
	// Wraithverge: this check reduces delay and makes loops smoother
	// in DarkSpyre (*.MUS) songs.  Their loop-point is much closer
	// to the mark with this condition.
	if (ticks / timer > MAX_SEC_DELAY) // for very long delays
		// Wraithverge: added uint32_t cast, but removed float cast,
		// because MAX_SEC_DELAY is now float.
		ticks = static_cast<uint32_t>(timer * MAX_SEC_DELAY);
	return ticks;
}

void CmusPlayer::executeCommand()
{
	uint8_t new_status = 0, voice, haut, vol, timbre;
	uint16_t pitch;

	// execute MIDI command
	if (data[pos] < NOTE_OFF_BYTE)
	{
		// running status
		new_status = status;
	}
	else
		new_status = data[pos++];
	if (new_status == STOP_BYTE)
	{
		pos = dataSize;
	}
	else if (new_status == SYSTEM_XOR_BYTE)
	{
		/*
		non-standard... this is a tempo multiplier:
		data format: <F0> <7F> <00> <integer> <frac> <F7>
		tempo = basicTempo * integerPart + basicTempo * fractionPart/128
		*/
		if (data[pos++] != ADLIB_CTRL_BYTE ||
			data[pos++] != TEMPO_CTRL_BYTE)
		{
			/* unknown format ... skip all the XOR message */
			pos -= 2;
			while (data[pos++] != EOX_BYTE);
		}
		else
		{
			uint8_t integer = data[pos++];
			uint8_t frac = data[pos++];
			uint16_t tempo = basicTempo;
			tempo = tempo * integer + ((tempo * frac) >> 7);
			SetTempo(tempo, tickBeat);
			pos++;       /* skip EOX_BYTE */
		}
	}
	else
	{
		status = new_status;
		voice = status & 0xF;
		switch (status & 0xF0)
		{
		case NOTE_ON_BYTE:
			haut = data[pos++];
			vol = data[pos++];
			if (voice > MAX_VOICES - 1)
				break;
			if (!vol)
			{
				if (drv) drv->NoteOff(voice);
			}
			else
			{
				if (vol != volume[voice])
				{
					if (drv) drv->SetVoiceVolume(voice, vol);
					volume[voice] = vol;
				}
				if (drv) drv->NoteOn(voice, haut);
			}
			break;
		case NOTE_OFF_BYTE:
			haut = data[pos++];
			vol = data[pos++];
			if (voice > MAX_VOICES - 1)
				break;
			if (drv) drv->NoteOff(voice);
			if (isIMS && vol)
			{
				if (vol != volume[voice])
				{
					if (drv) drv->SetVoiceVolume(voice, vol);
					volume[voice] = vol;
				}
				if (drv) drv->NoteOn(voice, haut);
			}
			break;
		case AFTER_TOUCH_BYTE:
			vol = data[pos++];
			if (voice > MAX_VOICES - 1)
				break;
			if (vol != volume[voice])
			{
				if (drv) drv->SetVoiceVolume(voice, vol);
				volume[voice] = vol;
			}
			break;
		case PROG_CHANGE_BYTE:
			timbre = data[pos++];
			if (voice > MAX_VOICES - 1)
				break;
			if (insts &&
				timbre < nrTimbre &&
				insts[timbre].loaded)
			{
				if (drv) drv->SetVoiceTimbre(voice, &insts[timbre].data[0]);
			}
			#ifdef DEBUG
			else
				AdPlug_LogWrite("Timbre not found: %d\n", timbre);
			#endif
			break;
		case PITCH_BEND_BYTE:
			pitch = data[pos++];
			pitch |= data[pos++] << 7;
			if (voice > MAX_VOICES - 1)
				break;
			if (drv) drv->SetVoicePitch(voice, pitch);
			break;
		case CONTROL_CHANGE_BYTE:
			/* unused */
			pos += 2;
			break;
		case CHANNEL_PRESSURE_BYTE:
			/* unused */
			pos++;
			break;
		default:
			/*
			A bad status byte ( or unimplemented MIDI command) has been encontered.
			Skip bytes until next timing byte followed by status byte.
			*/
			#ifdef DEBUG
			AdPlug_LogWrite("Bad MIDI status byte: %d\n", status);
			#endif
			while (data[pos++] < NOTE_OFF_BYTE && pos < dataSize);
			if (pos >= dataSize)
				break;
			if (data[pos] != OVERFLOW_BYTE)
				pos--;
			break;
		}
	}
}

bool CmusPlayer::update()
{
	if (!counter)
	{
		ticks = GetTicks();
	}
	if (++counter >= ticks)
	{
		counter = 0;
		while (pos < dataSize)
		{
			executeCommand();
			if (pos >= dataSize) {
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
