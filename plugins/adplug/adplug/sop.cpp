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
 * sop.cpp - sopepos' Note Player by Stas'M <binarymaster@mail.ru>
 *
 * REFERENCES:
 * http://www.shikadi.net/moddingwiki/SOP_Format
 */

#include <string.h>

#include "sop.h"

CPlayer *CsopPlayer::factory(Copl *newopl)
{
	return new CsopPlayer(newopl);
}

bool CsopPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename); if(!f) return false;

	// file validation
	if (!fp.extension(filename, ".sop"))
	{
		fp.close(f);
		return false;
	}
	if (fp.filesize(f) < SOP_HEAD_SIZE)
	{
		fp.close(f);
		return false;
	}
	f->setFlag(binio::BigEndian, false);

	/****** static header validation ******/

	char sign[SOP_SIGN_SIZE + 1];
	sign[SOP_SIGN_SIZE] = 0;
	f->readString(sign, SOP_SIGN_SIZE);
	if (strcmp(sign, "sopepos"))
	{
		fp.close(f);
		return false;
	}
	uint32_t check = f->readInt(3);
	if (check != 0x100 && check != 0x200)
	{
		fp.close(f);
		return false;
	}
	version = check;
	f->readString(fname, SOP_FILENAME);
	fname[SOP_FILENAME - 1] = 0;
	f->readString(title, SOP_TITLE);
	title[SOP_TITLE - 1] = 0;
	percussive = f->readInt(1);
	check = f->readInt(1);
	if (percussive > 1 || check != 0)
	{
		fp.close(f);
		return false;
	}
	tickBeat = f->readInt(1);
	check = f->readInt(1);
	if (tickBeat == 0 || check != 0)
	{
		fp.close(f);
		return false;
	}
	check = f->readInt(1); // beatMeasure
	basicTempo = f->readInt(1);
	if (!basicTempo) basicTempo = SOP_DEF_TEMPO;
	if (check == 0)
	{
		fp.close(f);
		return false;
	}
	f->readString(comment, SOP_COMMENT);
	comment[SOP_COMMENT - 1] = 0;
	nTracks = f->readInt(1);
	nInsts = f->readInt(1);
	check = f->readInt(1);
	if (nTracks == 0 || nInsts == 0 || nTracks > SOP_MAX_TRACK || nInsts > SOP_MAX_INST ||
		check != 0 || fp.filesize(f) < (unsigned)SOP_HEAD_SIZE + nTracks)
	{
		fp.close(f);
		return false;
	}

	/****** dynamic data load ******/

	// channel modes
	chanMode = new uint8_t[nTracks];
	f->readString((char *)chanMode, nTracks);
	// instruments
	inst = new sop_inst[nInsts];
	unsigned int i;
	for (i = 0; i < nInsts; i++)
	{
		inst[i].type = f->readInt(1);
		if (inst[i].type > SOP_INST_NONE)
		{
			fp.close(f);
			return false;
		}
		f->readString(inst[i].filename, SOP_INSTNAME);
		inst[i].filename[SOP_INSTNAME] = 0;
		f->readString(inst[i].longname, SOP_LONGNAME);
		inst[i].longname[SOP_LONGNAME] = 0;
		if (inst[i].type == SOP_INST_NONE)
		{
			continue;
		}
		else if (inst[i].type == SOP_INST_WAV)
		{
			if (fp.filesize(f) - f->pos() < sizeof(sop_sample))
			{
				fp.close(f);
				return false;
			}
			sop_sample sample;
			sample.val1 = f->readInt(2);
			sample.val2 = f->readInt(2);
			sample.length = f->readInt(2);
			sample.val4 = f->readInt(2);
			sample.base_freq = f->readInt(2);
			sample.val6 = f->readInt(2);
			sample.val7 = f->readInt(2);
			sample.val8 = f->readInt(1);
			sample.val9 = f->readInt(2);
			sample.val10 = f->readInt(2);
			if (fp.filesize(f) - f->pos() < sample.length)
			{
				fp.close(f);
				return false;
			}
			f->seek(sample.length, binio::Add);
			memset(inst[i].data, 0, sizeof(inst[i].data));
		}
		else if (inst[i].type == SOP_INST_4OP)
		{
			if (fp.filesize(f) - f->pos() < SOP_INST4OP)
			{
				fp.close(f);
				return false;
			}
			f->readString((char *)inst[i].data, SOP_INST4OP);
		}
		else
		{
			if (fp.filesize(f) - f->pos() < SOP_INST2OP)
			{
				fp.close(f);
				return false;
			}
			f->readString((char *)inst[i].data, SOP_INST2OP);
		}
	}
	// event tracks
	track = new sop_trk[nTracks + 1];
	for (i = 0; i < (unsigned)nTracks + 1; i++) track[i].data = 0;
	for (i = 0; i < (unsigned)nTracks + 1; i++)
	{
		track[i].nEvents = f->readInt(2);
		track[i].size = f->readInt(4);
		if (fp.filesize(f) - f->pos() < track[i].size)
		{
			fp.close(f);
			return false;
		}
		track[i].data = new uint8_t[track[i].size];
		f->readString((char *)track[i].data, track[i].size);
	}
	// done
	fp.close(f);
	drv = new Cad262Driver(opl);
	rewind(0);
	return true;
}

void CsopPlayer::rewind(int subsong)
{
	// set default tempo
	SetTempo(basicTempo);
	opl->init();
	if (drv) drv->SoundWarmInit();
	if (drv) drv->SetYM_262_SOP(1);

	int i;
	for (i = 0; i < nTracks + 1; i++)
	{
		track[i].pos = 0;
		track[i].counter = 0;
		track[i].ticks = 0;
		track[i].dur = 0;
	}
	songend = false;

	for (i = 0; i < SOP_MAX_TRACK; i++)
	{
		volume[i] = 0;
		lastvol[i] = 0;
	}
	master_vol = SOP_MAX_VOL;

	for (i = 0; i < nTracks; i++)
	{
		if (!drv) break;
		if (chanMode[i] & SOP_CHAN_4OP)
			drv->Set_4OP_Mode(i, 1);
	}
	if (drv) drv->SetMode_SOP(percussive);
}

/*
 * Set Tempo (tempo - BPM)
 */
void CsopPlayer::SetTempo(uint8_t tempo)
{
	if (!tempo) tempo = basicTempo;
	timer = tempo * tickBeat / 60.0f;
	cur_tempo = tempo;
}

/*
 * Execute SOP event (t - track index)
 */
void CsopPlayer::executeCommand(uint8_t t)
{
	uint8_t event = track[t].data[track[t].pos++];
	uint8_t value;

	switch (event)
	{
	case SOP_EVNT_NOTE:
		if (track[t].pos + 2 >= track[t].size) break;
		value = track[t].data[track[t].pos++];
		track[t].dur  = track[t].data[track[t].pos++];
		track[t].dur |= track[t].data[track[t].pos++] << 8;
		// skip this event on control track, ignore notes with zero duration
		if (t == nTracks || !track[t].dur) break;
		if (drv) drv->NoteOn_SOP(t, value);
		break;
	case SOP_EVNT_TEMPO:
		if (track[t].pos >= track[t].size) break;
		value = track[t].data[track[t].pos++];
		// process this event only on control track
		if (t < nTracks) break;
		SetTempo(value);
		break;
	case SOP_EVNT_VOL:
		if (track[t].pos >= track[t].size) break;
		value = track[t].data[track[t].pos++];
		// skip this event on control track
		if (t == nTracks) break;
		lastvol[t] = value;
		value = value * master_vol / SOP_MAX_VOL;
		if (value != volume[t])
		{
			if (drv) drv->SetVoiceVolume_SOP(t, value);
			volume[t] = value;
		}
		break;
	case SOP_EVNT_PITCH:
		if (track[t].pos >= track[t].size) break;
		value = track[t].data[track[t].pos++];
		// skip this event on control track
		if (t == nTracks) break;
		if (drv) drv->SetVoicePitch_SOP(t, value);
		break;
	case SOP_EVNT_INST:
		if (track[t].pos >= track[t].size) break;
		value = track[t].data[track[t].pos++];
		// skip this event on control track, ignore values out of range
		if (t == nTracks || value >= nInsts) break;
		if (drv) drv->SetVoiceTimbre_SOP(t, inst[value].data);
		break;
	case SOP_EVNT_PAN:
		if (track[t].pos >= track[t].size) break;
		value = track[t].data[track[t].pos++];
		// skip this event on control track
		if (t == nTracks) break;
		if (version == 0x200)
		{
			switch (value) // SOP v2 panning
			{
			case 0x00: value = 2; break; // left
			case 0x40: value = 1; break; // middle
			case 0x80: value = 0; break; // right
			}
		}
		if (drv) drv->SetStereoPan_SOP(t, value);
		break;
	case SOP_EVNT_MVOL:
		if (track[t].pos >= track[t].size) break;
		value = track[t].data[track[t].pos++];
		// process this event only on control track
		if (t < nTracks) break;
		master_vol = value;
		for (int i = 0; i < nTracks; i++)
		{
			value = lastvol[i] * master_vol / SOP_MAX_VOL;
			if (value != volume[i])
			{
				if (drv) drv->SetVoiceVolume_SOP(i, value);
				volume[i] = value;
			}
		}
		break;
	default:
		track[t].pos++;
		break;
	}
}

bool CsopPlayer::update()
{
	songend = true;
	for (uint8_t i = 0; i < nTracks + 1; i++)
	{
		if (track[i].dur)
		{
			songend = false; // there are active notes
			if (drv && !--track[i].dur) drv->NoteOff_SOP(i);
		}
		if (track[i].pos >= track[i].size)
			continue;
		songend = false; // track is not finished
		if (!track[i].counter)
		{
			track[i].ticks  = track[i].data[track[i].pos++];
			track[i].ticks |= track[i].data[track[i].pos++] << 8;
			if (track[i].pos == 2 && track[i].ticks)
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
				else if (!track[i].data[track[i].pos] && !track[i].data[track[i].pos + 1]) // if next delay is zero
				{
					track[i].pos += 2;
				}
				else break;
			}
		}
	}
	return !songend;
}

/*
 *  Source code parts from free Ad262Sop Library (c) 1996-2005, Park Jeenhong
 */

const int Cad262Driver::fNumTbl[TABLE_SIZE]={
	0x0159, 0x015A, 0x015A, 0x015B, 0x015C, 0x015C, 0x015D, 0x015D, 0x015E, 0x015F, 0x015F, 0x0160,
	0x0161, 0x0161, 0x0162, 0x0162, 0x0163, 0x0164, 0x0164, 0x0165, 0x0166, 0x0166, 0x0167, 0x0168,
	0x0168, 0x0169, 0x016A, 0x016A, 0x016B, 0x016C, 0x016C, 0x016D, 0x016E, 0x016E, 0x016F, 0x016F,
	0x0170, 0x0171, 0x0171, 0x0172, 0x0173, 0x0174, 0x0174, 0x0175, 0x0176, 0x0176, 0x0177, 0x0178,
	0x0178, 0x0179, 0x017A, 0x017A, 0x017B, 0x017C, 0x017C, 0x017D, 0x017E, 0x017E, 0x017F, 0x0180,
	0x0180, 0x0181, 0x0182, 0x0183, 0x0183, 0x0184, 0x0185, 0x0185, 0x0186, 0x0187, 0x0187, 0x0188,
	0x0189, 0x018A, 0x018A, 0x018B, 0x018C, 0x018C, 0x018D, 0x018E, 0x018F, 0x018F, 0x0190, 0x0191,
	0x0191, 0x0192, 0x0193, 0x0194, 0x0194, 0x0195, 0x0196, 0x0197, 0x0197, 0x0198, 0x0199, 0x019A,
	0x019A, 0x019B, 0x019C, 0x019D, 0x019D, 0x019E, 0x019F, 0x019F, 0x01A0, 0x01A1, 0x01A2, 0x01A3,
	0x01A3, 0x01A4, 0x01A5, 0x01A6, 0x01A6, 0x01A7, 0x01A8, 0x01A9, 0x01A9, 0x01AA, 0x01AB, 0x01AC,
	0x01AC, 0x01AD, 0x01AE, 0x01AF, 0x01B0, 0x01B0, 0x01B1, 0x01B2, 0x01B3, 0x01B3, 0x01B4, 0x01B5,
	0x01B6, 0x01B7, 0x01B7, 0x01B8, 0x01B9, 0x01BA, 0x01BB, 0x01BB, 0x01BC, 0x01BD, 0x01BE, 0x01BF,
	0x01BF, 0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C3, 0x01C4, 0x01C5, 0x01C6, 0x01C7, 0x01C8, 0x01C8,
	0x01C9, 0x01CA, 0x01CB, 0x01CC, 0x01CD, 0x01CD, 0x01CE, 0x01CF, 0x01D0, 0x01D1, 0x01D2, 0x01D2,
	0x01D3, 0x01D4, 0x01D5, 0x01D6, 0x01D7, 0x01D7, 0x01D8, 0x01D9, 0x01DA, 0x01DB, 0x01DC, 0x01DD,
	0x01DD, 0x01DE, 0x01DF, 0x01E0, 0x01E1, 0x01E2, 0x01E3, 0x01E4, 0x01E4, 0x01E5, 0x01E6, 0x01E7,
	0x01E8, 0x01E9, 0x01EA, 0x01EB, 0x01EB, 0x01EC, 0x01ED, 0x01EE, 0x01EF, 0x01F0, 0x01F1, 0x01F2,
	0x01F3, 0x01F3, 0x01F4, 0x01F5, 0x01F6, 0x01F7, 0x01F8, 0x01F9, 0x01FA, 0x01FB, 0x01FC, 0x01FD,
	0x01FE, 0x01FE, 0x01FF, 0x0200, 0x0201, 0x0202, 0x0203, 0x0204, 0x0205, 0x0206, 0x0207, 0x0208,
	0x0209, 0x020A, 0x020B, 0x020B, 0x020C, 0x020D, 0x020E, 0x020F, 0x0210, 0x0211, 0x0212, 0x0213,
	0x0214, 0x0215, 0x0216, 0x0217, 0x0218, 0x0219, 0x021A, 0x021B, 0x021C, 0x021D, 0x021E, 0x021F,
	0x0220, 0x0221, 0x0222, 0x0223, 0x0224, 0x0225, 0x0226, 0x0227, 0x0228, 0x0229, 0x022A, 0x022B,
	0x022C, 0x022D, 0x022E, 0x022F, 0x0230, 0x0231, 0x0232, 0x0233, 0x0234, 0x0235, 0x0236, 0x0237,
	0x0238, 0x0239, 0x023A, 0x023B, 0x023C, 0x023D, 0x023E, 0x023F, 0x0240, 0x0241, 0x0242, 0x0243,
	0x0244, 0x0245, 0x0246, 0x0247, 0x0248, 0x0249, 0x024B, 0x024C, 0x024D, 0x024E, 0x024F, 0x0250,
	0x0251, 0x0252, 0x0253, 0x0254, 0x0255, 0x0256, 0x0257, 0x0258, 0x025A, 0x025B, 0x025C, 0x025D,
	0x025E, 0x025F, 0x0260, 0x0261, 0x0262, 0x0263, 0x0265, 0x0266, 0x0267, 0x0268, 0x0269, 0x026A,
	0x026B, 0x026C, 0x026D, 0x026F, 0x0270, 0x0271, 0x0272, 0x0273, 0x0274, 0x0275, 0x0276, 0x0278,
	0x0279, 0x027A, 0x027B, 0x027C, 0x027D, 0x027E, 0x0280, 0x0281, 0x0282, 0x0283, 0x0284, 0x0285,
	0x0287, 0x0288, 0x0289, 0x028A, 0x028B, 0x028C, 0x028E, 0x028F, 0x0290, 0x0291, 0x0292, 0x0294,
	0x0295, 0x0296, 0x0297, 0x0298, 0x029A, 0x029B, 0x029C, 0x029D, 0x029E, 0x02A0, 0x02A1, 0x02A2,
	0x02A3, 0x02A4, 0x02A6, 0x02A7, 0x02A8, 0x02A9, 0x02AB, 0x02AC, 0x02AD, 0x02AE, 0x02B0, 0x02B1
};

const unsigned char Cad262Driver::SlotX[maxVoices * 2]={
	0, 1, 2, 8, 9, 10, 16, 17, 18,  0,  0, 0, 1, 2, 8, 9, 10, 16, 17, 18,
	0, 1, 2, 8, 9, 10, 16, 20, 18, 21, 17, 0, 1, 2, 8, 9, 10, 16, 17, 18
};

const unsigned char Cad262Driver::VolReg[11 * 2]={
	0x43, 0x44, 0x45, 0x4B, 0x4C, 0x4D, 0x53, 0x54, 0x55, 0, 0,
	0x43, 0x44, 0x45, 0x4B, 0x4C, 0x4D, 0x53, 0x54, 0x52, 0x55, 0x51
};

const unsigned Cad262Driver::MOD12[OCTAVE * 11]={
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

const unsigned Cad262Driver::DIV12[OCTAVE * 8]={
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

void Cad262Driver::SetMode_SOP(int mode)
{
	if (mode) {
		/* set the frequency for the last 4 percussion voices: */
		voiceNote[TOM] = SOP_TOM_PITCH;
		vPitchBend[TOM] = 100;
		UpdateFNums(TOM);

		voiceNote[SD] = SOP_SD_PITCH;
		vPitchBend[SD] = 100;
		UpdateFNums(SD);
	}

	percussion = mode;

	SndOutput1(0xBD, (percussion ? 0x20 : 0));
} /* SetMode() */

void Cad262Driver::SoundWarmInit()
{
	int i, j;

	for (i = 0; i < 64; i++) {
		for (j = 0; j < 128; j++)
			VolumeTable[i * 128 + j] = (i * j + (MAX_VOLUME + 1) / 2) >> LOG2_VOLUME;
	}

	for (i = 1; i <= 0xF5; i++) {
		SndOutput1(i, 0); /* clear all registers */
		SndOutput3(i, 0);
	}

	for (i = 0; i < YMB_SIZE; i++) {
		ymbuf[i] = 0;
		ymbuf[i + YMB_SIZE] = 0;
	}

	for (i = 0; i < 20; i++) {
		vPitchBend[i] = 100;
		voiceKeyOn[i] = 0;
		voiceNote[i] = MID_C;
		VoiceVolume[i] = 0;
		Ksl[i] = 0;
		Ksl2[i] = 0;
		Ksl2V[i] = 0;
		OP4[i] = 0;
		Stereo[i] = 0x30;
	}

	OP_MASK = 0;

	SndOutput1(4, 6); /* mask T1 & T2 */

	SndOutput3(5, 1); /* YMF-262M Mode */
	SndOutput3(4, 0);

	SetMode_SOP(0); /* melodic mode */

	SndOutput1(0x08, 0);
	SndOutput1(1, 0x20);
} /* SoundWarmInit() */

void Cad262Driver::UpdateFNums(int chan)
{
	if (chan >= maxVoices)
		return;

	SetFreq_SOP(chan, voiceNote[chan], vPitchBend[chan], 0);
}

void Cad262Driver::SndOutput1(int addr, int value)
{
	if (addr >= 0xB0)
		ymbuf[addr - 0xB0] = value;

	if (opl->getchip() != 0)
		opl->setchip(0);
	opl->write(addr, value);
}

void Cad262Driver::SndOutput3(int addr, int value)
{
	if (addr >= 0xB0)
		ymbuf[YMB_SIZE - 0xB0 + addr] = value;

	if (opl->getchip() != 1)
		opl->setchip(1);
	opl->write(addr, value);
}

void Cad262Driver::SEND_INS(int base_addr, unsigned char* value, int mode)
{
	int i;
	if (opl->getchip() != mode)
		opl->setchip(mode);

	for (i = 0; i < 4; i++) {
		opl->write(base_addr, *value++);
		base_addr += 0x20;
	}

	base_addr += 0x40;
	opl->write(base_addr, (*value) & 0x07);
}

void Cad262Driver::SetYM_262_SOP(int VX)
{
	SndOutput3(5, VX);
	SndOutput3(4, 0);
}

void Cad262Driver::SetStereoPan_SOP(int chan, int value)
{
	int PAN[] = { 0xa0, 0x30, 0x50 };
	int port, abs_port;

	if (chan >= maxVoices)
		return;

	Stereo[chan] = (unsigned char)(value = PAN[value]);

	port = 0;

	if (chan < 9)
		abs_port = chan;
	else {
		if (chan < 11)
			abs_port = 17 - chan;
		else {
			abs_port = chan - 11;
			port = 1;
		}
	}

	value |= ((chan >= 11) ? ymbuf[YMB_SIZE + 0x10 + abs_port] & 0x0F : ymbuf[abs_port + 0x10] & 0x0F);
	if (opl->getchip() != port)
		opl->setchip(port);

	if (OP4[chan]) {
		opl->write(abs_port + 0xC3, (value & 0xF0) | (int)(((chan >= 11) ? ymbuf[YMB_SIZE + 0x13 + abs_port] : ymbuf[abs_port + 0x13]) & 0x0F));
	}

	opl->write(abs_port + 0xC0, value);
}

void Cad262Driver::SetVoiceVolume_SOP(unsigned chan, unsigned vol)
{
	int volume;
	unsigned char KSL_value;

	if (chan >= maxVoices)
		return;

	if (chan > 2 && OP4[chan - 3])
		return;

	if (vol > MAX_VOLUME)
		vol = MAX_VOLUME;

	VoiceVolume[chan] = vol;

	if (Ksl2V[chan]) {
		volume = 63 - VolumeTable[((63 - ((KSL_value = Ksl2[chan]) & 0x3F)) << 7) + vol];

		if (chan >= 11)
			SndOutput3(VolReg[chan - 11] - 3, (KSL_value & 0xC0) | volume);
		else
			SndOutput1((percussion ? VolReg[chan + 11] : VolReg[chan]) - 3, (KSL_value & 0xC0) | volume);

		if (OP4[chan]) {
			chan += 3;
			volume = 63 - VolumeTable[((63 - ((KSL_value = Ksl[chan]) & 0x3F)) << 7) + vol];

			if (chan >= 11)
				SndOutput3(VolReg[chan - 11], (KSL_value & 0xC0) | volume);
			else
				SndOutput1(VolReg[chan], (KSL_value & 0xC0) | volume);

			if (Ksl2V[chan]) {
				volume = 63 - VolumeTable[((63 - ((KSL_value = Ksl2[chan]) & 0x3F)) << 7) + vol];

				if (chan >= 11)
					SndOutput3(VolReg[chan - 11] - 3, (KSL_value & 0xC0) | volume);
				else
					SndOutput1(VolReg[chan] - 3, (KSL_value & 0xC0) | volume);
			}
		}
		else {
			volume = 63 - VolumeTable[((63 - ((KSL_value = Ksl[chan]) & 0x3F)) << 7) + vol];

			if (chan >= 11)
				SndOutput3(VolReg[chan - 11], (KSL_value & 0xC0) | volume);
			else
				SndOutput1((percussion ? VolReg[chan + 11] : VolReg[chan]), (KSL_value & 0xC0) | volume);
		}
	}
	else {
		if (OP4[chan]) {
			volume = 63 - VolumeTable[((63 - ((KSL_value = Ksl[chan + 3]) & 0x3F)) << 7) + vol];

			if (chan >= 11)
				SndOutput3(VolReg[chan + 3 - 11], (KSL_value & 0xC0) | volume);
			else
				SndOutput1(VolReg[chan + 3], (KSL_value & 0xC0) | volume);

			if (Ksl2V[chan + 3]) {
				volume = 63 - VolumeTable[((63 - ((KSL_value = Ksl[chan]) & 0x3F)) << 7) + vol];

				if (chan >= 11)
					SndOutput3(VolReg[chan - 11], (KSL_value & 0xC0) | volume);
				else
					SndOutput1(VolReg[chan], (KSL_value & 0xC0) | volume);
			}
		}
		else {
			volume = 63 - VolumeTable[((63 - ((KSL_value = Ksl[chan]) & 0x3F)) << 7) + vol];

			if (chan >= 11)
				SndOutput3(VolReg[chan - 11], (KSL_value & 0xC0) | volume);
			else
				SndOutput1((percussion ? VolReg[chan + 11] : VolReg[chan]), (KSL_value & 0xC0) | volume);
		}
	}
}

void Cad262Driver::SetVoiceTimbre_SOP(unsigned chan, unsigned char* array)
{
	int i;
	int Slot_Number, KSL_value;

	if (chan >= maxVoices || (chan > 2 && OP4[chan - 3]))
		return;

	if (!percussion)
		Slot_Number = SlotX[chan];
	else
		Slot_Number = SlotX[chan + 20];

	Ksl2V[chan] = ((KSL_value = (array[5] & 0x0F)) & 0x01);

	if (chan > 10) {
		i = chan + 0xC0 - 11;

		SndOutput3(i, 0);

		SEND_INS(0x20 + Slot_Number, array, 1);
		SEND_INS(0x23 + Slot_Number, &array[6], 1);

		if (OP4[chan]) {
			SndOutput3(i + 3, 0);

			SEND_INS(0x28 + Slot_Number, &((unsigned char*)array)[11], 1);
			SEND_INS(0x2B + Slot_Number, &((unsigned char*)array)[17], 1);

			Ksl[chan + 3] = *(array + 18);
			Ksl2[chan + 3] = *(array + 12);
			Ksl2V[chan + 3] = *(array + 16) & 1;

			SndOutput3(i + 3, (*(array + 16) & 0x0F) | Stereo[chan]);
		}

		Ksl[chan] = *(array + 7);
		Ksl2[chan] = *(array + 1);
		Ksl2V[chan] = *(array + 5) & 1;

		SetVoiceVolume_SOP(chan, VoiceVolume[chan]);
		SndOutput3(i, KSL_value | Stereo[chan]);
	}
	else {
		if (chan > 8)
			i = 0xC0 + 17 - chan;
		else
			i = chan + 0xC0;

		SndOutput1(i, 0);

		SEND_INS(0x20 + Slot_Number, array, 0);

		if (percussion && chan > BD) {
			Ksl[chan] = *(array + 1);
			Ksl2V[chan] = 0;
		}
		else {
			SEND_INS(0x23 + Slot_Number, &array[6], 0);

			Ksl[chan] = *(array + 7);
			Ksl2[chan] = *(array + 1);
			Ksl2V[chan] = *(array + 5) & 1;
		}

		if (OP4[chan]) {
			SndOutput1(i + 3, 0);

			SEND_INS(0x28 + Slot_Number, &((unsigned char*)array)[11], 0);
			SEND_INS(0x2B + Slot_Number, &((unsigned char*)array)[17], 0);

			Ksl[chan + 3] = *(array + 18);
			Ksl2[chan + 3] = *(array + 12);
			Ksl2V[chan + 3] = *(array + 16) & 1;

			SndOutput1(i + 3, (*(array + 16) & 0x0F) | Stereo[chan]);
		}

		SetVoiceVolume_SOP(chan, VoiceVolume[chan]);
		SndOutput1(i, KSL_value | Stereo[chan]);
	}
}

void Cad262Driver::SetFreq_SOP(int voice, unsigned note, int pitch, int keyOn)
{
	int temp, fN, divFactor, fNIndex;

	temp = (int)((pitch - 100) / 3.125) + ((note - 12) << LOG_NB_STEP_PITCH);

	if (temp < 0)
		temp = 0;
	else {
		if (temp >= ((NB_NOTES << LOG_NB_STEP_PITCH) - 1))
			temp = (NB_NOTES << LOG_NB_STEP_PITCH) - 1;
	}

	fNIndex = (MOD12[(temp >> 5)] << 5) + (temp & (NB_STEP_PITCH - 1));

	fN = fNumTbl[fNIndex];

	divFactor = DIV12[(temp >> 5)];

	if (voice <= 10)
		SndOutput1(0xA0 + voice, (fN & 0xFF));
	else
		SndOutput3(0xA0 + voice - 11, (fN & 0xFF));

	fN = (((fN >> 8) & 0x03) | (divFactor << 2) | keyOn) & 0xFF;

	if (voice <= 10)
		SndOutput1(0xB0 + voice, fN);
	else
		SndOutput3(0xB0 + voice - 11, fN);
}

void Cad262Driver::SetVoicePitch_SOP(unsigned chan, int pitch)
{
	if (chan >= maxVoices || pitch < 0 || pitch > 200)
		return;

	vPitchBend[chan] = pitch;

	if (!percussion)
		SetFreq_SOP(chan, voiceNote[chan], pitch, voiceKeyOn[chan]);
	else {
		if (chan <= BD || chan > HIHAT)
			SetFreq_SOP(chan, voiceNote[chan], pitch, voiceKeyOn[chan]);
	}
}

void Cad262Driver::NoteOn_SOP(unsigned chan, unsigned pitch)
{
	if (chan >= maxVoices)
		return;

	if (percussion && chan >= BD && chan <= HIHAT) {
		if (chan == BD) {
			voiceNote[BD] = pitch;
			SetFreq_SOP(BD, voiceNote[BD], vPitchBend[BD], 0);
		}
		else {
			if (chan == TOM && (unsigned)voiceNote[TOM] != pitch) {
				voiceNote[SD] = (voiceNote[TOM] = pitch) + TOM_TO_SD;
				SetFreq_SOP(TOM, voiceNote[TOM], 100, 0);
				SetFreq_SOP(SD, voiceNote[SD], 100, 0);
			}
		}
		SndOutput1(0xBD, ymbuf[0x0D] | (0x10 >> (chan - BD)));
	}
	else {
		voiceNote[chan] = pitch;
		voiceKeyOn[chan] = 0x20;

		SetFreq_SOP(chan, pitch, vPitchBend[chan], 0x20);
	}
}

void Cad262Driver::NoteOff_SOP(unsigned chan)
{
	if (chan >= maxVoices)
		return;

	voiceKeyOn[chan] = 0;

	if (percussion && chan >= BD && chan <= HIHAT)
		SndOutput1(0xBD, ymbuf[0x0D] & (0xFF - (0x10 >> (chan - BD))));
	else {
		if (chan < HIHAT)
			SndOutput1(0xB0 + chan, ymbuf[chan] & 0xDF);
		else
			SndOutput3(0xB0 - 11 + chan, ymbuf[chan - 11 + YMB_SIZE] & 0xDF);
	}
}

int Cad262Driver::Set_4OP_Mode(unsigned chan, unsigned value)
{
	if (chan >= maxVoices)
		return 1;

	if (SlotX[chan + 20] <= 2) {
		OP4[chan] = value;

		if (value) {
			if (chan > 10)
				OP_MASK |= (0x01 << (chan - 11 + 3));
			else
				OP_MASK |= (0x01 << chan);
		}
		else {
			if (chan > 10)
				OP_MASK &= (0xFF - (0x01 << (chan - 11 + 3)));
			else
				OP_MASK &= (0xFF - (0x01 << chan));
		}

		SndOutput3(0x04, OP_MASK);

		return 1;
	}

	return 0;
}
