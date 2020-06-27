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
 * adlib.cpp - AdLib Sound Driver by Stas'M <binarymaster@mail.ru> and Jepael
 *
 * Based on ADLIB.C by Marc Savary and Dale Glowinski, Ad Lib Inc.
 */

#include "adlib.h"

const uint8_t CadlibDriver::percMasks[5] =
	{0x10, 0x08, 0x04, 0x02, 0x01};

/* definition of the ELECTRIC-PIANO voice (opr0 & opr1) */
uint8_t CadlibDriver::pianoParamsOp0[nbLocParam] =
	{ 1, 1, 3, 15, 5, 0, 1, 3, 15, 0, 0, 0, 1, 0 };
uint8_t CadlibDriver::pianoParamsOp1[nbLocParam] =
	{ 0, 1, 1, 15, 7, 0, 2, 4,  0, 0, 0, 1, 0, 0 };

/* definition of default percussive voices: */
uint8_t CadlibDriver::bdOpr0[nbLocParam] =
	{ 0,  0, 0, 10,  4, 0, 8, 12, 11, 0, 0, 0, 1, 0 };
uint8_t CadlibDriver::bdOpr1[nbLocParam] =
	{ 0,  0, 0, 13,  4, 0, 6, 15,  0, 0, 0, 0, 1, 0 };
uint8_t CadlibDriver::sdOpr[nbLocParam] =
	{ 0, 12, 0, 15, 11, 0, 8,  5,  0, 0, 0, 0, 0, 0 };
uint8_t CadlibDriver::tomOpr[nbLocParam] =
	{ 0,  4, 0, 15, 11, 0, 7,  5,  0, 0, 0, 0, 0, 0 };
uint8_t CadlibDriver::cymbOpr[nbLocParam] =
	{ 0,  1, 0, 15, 11, 0, 5,  5,  0, 0, 0, 0, 0, 0 };
uint8_t CadlibDriver::hhOpr[nbLocParam] =
	{ 0,  1, 0, 15, 11, 0, 7,  5,  0, 0, 0, 0, 0, 0 };

/* Slot numbers as a function of the voice and the operator.
	( melodic only)
*/
uint8_t CadlibDriver::slotVoice[9][2] = {
	{ 0, 3 },	/* voix 0 */
	{ 1, 4 },	/* 1 */
	{ 2, 5 },	/* 2 */
	{ 6, 9 },	/* 3 */
	{ 7, 10 },	/* 4 */
	{ 8, 11 },	/* 5 */
	{ 12, 15 },	/* 6 */
	{ 13, 16 },	/* 7 */
	{ 14, 17 }	/* 8 */
};

/* Slot numbers for the percussive voices.
	0 indicates that there is only one slot.
*/
uint8_t CadlibDriver::slotPerc[5][2] = {
	{ 12, 15 },		/* Bass Drum: slot 12 et 15 */
	{ 16, 0 },		/* SD: slot 16 */
	{ 14, 0 },		/* TOM: slot 14 */
	{ 17, 0 },		/* TOP-CYM: slot 17 */
	{ 13, 0 }		/* HH: slot 13 */
};

/*
	This table gives the offset of each slot within the chip.
	offset = fn( slot)
*/
uint8_t CadlibDriver::offsetSlot[18] = {
	0,  1,  2,  3,  4,  5,
	8,  9, 10, 11, 12, 13,
	16, 17, 18, 19, 20, 21
};

/* This table indicates if the slot is a modulator (0) or a carrier (1).
	opr = fn( slot)
*/
uint8_t CadlibDriver::operSlot[18] = {
	0, 0, 0,		/* 1 2 3 */
	1, 1, 1,		/* 4 5 6 */
	0, 0, 0, 		/* 7 8 9 */
	1, 1, 1, 		/* 10 11 12 */
	0, 0, 0, 		/* 13 14 15 */
	1, 1, 1,		/* 16 17 18 */
};

/* This table gives the voice number associated with each slot.
	(melodic mode only)
	voice = fn( slot)
*/
uint8_t CadlibDriver::voiceSlot[18] = {
	0, 1, 2,
	0, 1, 2,
	3, 4, 5,
	3, 4, 5,
	6, 7, 8,
	6, 7, 8,
};

/*** public methods *************************************/

void CadlibDriver::SoundWarmInit()
{
	/* init variables */
	for (int i = 0; i < MAX_VOICES; i++)
	{
		fNumFreqPtr[i] = 0;
		voiceKeyOn[i] = 0;
		notePitch[i] = 0;
	}
	amDepth = 0;
	vibDepth = 0;
	noteSel = 0;
	InitSlotVolume();
	InitFNums();
	SetMode(0);				/* melodic mode */
	SetGParam(0, 0, 0);		/* init global parameters */
	for (int i = 0; i < 9; i++)
		SoundChut(i);
	SetPitchRange(1);		/* default pitch range is 1 half-tone */
	SetWaveSel(1);
}

/*
---------------------------------------------
	Put the chip in melodic mode (mode == 0),
	or in percussive mode ( mode != 0).

	If the melodic mode is chosen, all voices are
	set to electric-piano, else the first 5 are set
	to electric-piano, and the percussion voices
	to their default timbres.
---------------------------------------------
*/
void CadlibDriver::SetMode(int mode)
{
	if (mode)
	{
		SoundChut(BD);
		SoundChut(SD);
		SoundChut(TOM);

		/* set the frequency for the last 4 percussion voices: */
		SetFreq(TOM, TOM_PITCH, 0);
		SetFreq(SD, SD_PITCH, 0);
	}
	percussion = mode;
	percBits = 0;

	InitSlotParams();
	SndSAmVibRhythm();
}

/*
	Enable (state != 0) / disable (state == 0)
	the wave-select parameters.

	If you do not want to use the wave-select parameters, call
	this function with a value of 0 AFTER calling SoundColdInit()
	or SoundWarmInit().
*/
void CadlibDriver::SetWaveSel(int state)
{
	modeWaveSel = state ? 0x20 : 0;
	for (int i = 0; i < 18; i++)
		opl->write(0xE0 + offsetSlot[i], 0);
	opl->write(1, modeWaveSel);
}

/*
	Routine to change the pitch bend range. The value can be from
	1 to 12 (in half-tones).

	For example, the value 12 means that the pitch bend will
	range from -12 (pitchBend == 0, see function 'SetVoicePitch()')
	to +12 (pitchBend == 0x3fff) half-tones.

	The change will be effective as of the next call to
	'SetVoicePitch()'.
*/
void CadlibDriver::SetPitchRange(uint8_t pR)
{
	if (pR > 12)
		pR = 12;
	if (pR < 1)
		pR = 1;
	pitchRange = pR;
	pitchRangeStep = pitchRange * NR_STEP_PITCH;
}

/*
----------------------------------------------
	Set the 3 global parameters AmDepth,
	VibDepth & NoteSel

	The change takes place immediately.
----------------------------------------------
*/
void CadlibDriver::SetGParam(int amD, int vibD, int nSel)
{
	amDepth = amD;
	vibDepth = vibD;
	noteSel = nSel;

	SndSAmVibRhythm();
	SndSNoteSel();
}

/*
-------------------------------------------------
	Set the parameters of the voice 'voice'.

	In melodic mode, 'voice' varies from 0 to 8.
	In percussive mode, voices 0 to 5 are melodic
	and 6 to 10 are percussive.

	A timbre (melodic or percussive) is defined as follows:
	the 13 first parameters of operator 0 ( ksl, multi, feedBack,
	attack, sustain, eg-typem decay, release, level, am, vib, ksr, fm)
	followed by the 13 parameters of operator 1 (if a percussive voice, all
	the parameters are zero), followed by the wave-select parameter for
	the operators 0 and 1.

	'paramArray' is structured as follows:
		struct {
			int opr0Prm[ 13];		first 13 parameters
			int opr1Prm[ 13];		must be 0 if percussive timbre
			int	opr0WaveSel;		last parameter
			int opr1WaveSel;		must be 0 if percussive timbre
		} TimbreDef;

	The old timbre files (*.INS) do not contain the parameters
	'opr0WaveSel' and 'opr1WaveSel'. 
	Set these two parameters to zero if you are using the old file
	format.
-------------------------------------------------
*/
void CadlibDriver::SetVoiceTimbre(uint8_t voice, int16_t * paramArray)
{
	int16_t wave0, wave1;
	int16_t * prm1, * wavePtr;

	wavePtr = paramArray + 2 * (nbLocParam - 1);
	wave0 = *wavePtr++;
	wave1 = *wavePtr;
	prm1 = paramArray + nbLocParam - 1;

	if (!percussion || voice < BD) {	/* melodic only */
		SetSlotParam(slotVoice[voice][0], paramArray, wave0);
		SetSlotParam(slotVoice[voice][1], prm1, wave1);
	}
	else if (voice == BD) {	/* Bass Drum */
		SetSlotParam(slotPerc[0][0], paramArray, wave0);
		SetSlotParam(slotPerc[0][1], prm1, wave1);
	}
	else	/* percussion, 1 slot */
		SetSlotParam(slotPerc[voice - BD][0], paramArray, wave0);
}

/*
--------------------------------------------------
	Set the volume of the voice 'voice' to 'volume'.

	The resulting output level is (timbreVolume * volume / 127).
	The change takes place immediately.

	0 <= volume <= 127
--------------------------------------------------
*/
void CadlibDriver::SetVoiceVolume(uint8_t voice, uint8_t volume)
{
	uint8_t slot;

#if 1
	if (!percussion || voice < BD)
		slot = slotVoice[voice][1];
	else
		slot = slotPerc[voice - BD][voice == BD ? 1 : 0];

	if (volume > MAX_VOLUME)
		volume = MAX_VOLUME;
	slotRelVolume[slot] = volume;
	SndSKslLevel(slot);
#else	/* code that modify the two oper. volume of an additive sound: */
	if (volume > MAX_VOLUME)
		volume = MAX_VOLUME;

	if (!percussion || voice <= BD) {
		slots = slotVoice[voice];
		slotRelVolume[slots[1]] = volume;
		SndSKslLevel(slots[1]);
		if (!GetLocPrm(slots[0], prmFm)) {
			/* additive syntesis: set volume of first slot too */
			slotRelVolume[slots[0]] = volume;
			SndSKslLevel(slots[0]);
		}
	}
	else {
		slot = slotPerc[voice - BD][0];
		slotRelVolume[slot] = volume;
		SndSKslLevel(slot);
	}
#endif
}

/*
-------------------------------------------------
	Change the pitch value of a voice.

	The variation in pitch is a function of the previous call
	to 'SetPitchRange()' and the value of 'pitchBend'.
	A value of 0 means -half-tone * pitchRange,
	0x2000 means no variation (exact pitch) and
	0x3fff means +half-tone * pitchRange.

	Does not affect the percussive voices, except for the bass drum.

	The change takes place immediately.

	0 <= pitchBend <= 0x3fff, 0x2000 == exact tuning
-------------------------------------------------
*/
void CadlibDriver::SetVoicePitch(uint8_t voice, uint16_t pitchBend)
{
	if (!percussion || voice <= BD) {
		/* melodic + bass-drum */
		if (pitchBend > MAX_PITCH)
			pitchBend = MAX_PITCH;
		ChangePitch(voice, pitchBend);
		SetFreq(voice, notePitch[voice], voiceKeyOn[voice]);
	}
}

/*
-----------------------------------------------------------
	Routine to start a note playing.

	0 <= voice <= 8	in melodic mode,
	0 <= voice <= 10 in percussive mode;
	0 <= pitch <= 127, 60 == MID_C ( the card can play between 12 and 107 )
-----------------------------------------------------------
*/
void CadlibDriver::NoteOn(uint8_t voice, int pitch)
{
	pitch -= (MID_C - CHIP_MID_C);
	if (pitch < 0)
		pitch = 0;
	if (pitch > 127)
		pitch = 127;

	if (voice < BD || !percussion)
		/* this is a melodic voice */
		SetFreq(voice, pitch, 1);
	else {
		/* this is a percussive voice */
		if (voice == BD)
			SetFreq(BD, pitch, 0);
		else if (voice == TOM) {
			/* for the last 4 percussions, only the TOM may change in frequency,
			modifying the three others: */
			SetFreq(TOM, pitch, 0);
			SetFreq(SD, pitch + TOM_TO_SD, 0);	/* f7 = 3 * f8 */
		}

		percBits |= percMasks[voice - BD];
		SndSAmVibRhythm();
	}
}

/*
	Routine to stop playing the note which was started in 'NoteOn()'.

	0 <= voice <= 8	in melodic mode,
	0 <= voice <= 10 in percussive mode;
*/
void CadlibDriver::NoteOff(uint8_t voice)
{
	if (!percussion || voice < BD)
		SetFreq(voice, notePitch[voice], 0); /* shut off */
	else {
		percBits &= ~percMasks[voice - BD];
		SndSAmVibRhythm();
	}
}

/*
	Set the volume of all slots.
*/
void CadlibDriver::InitSlotVolume()
{
	for (int i = 0; i < 18; i++)
		slotRelVolume[i] = MAX_VOLUME;
}

/*
	Initialize all lines of the frequency table. Each line represents
	12 half-tones shifted by (n/NR_STEP_PITCH), where 'n' is the line number
	and ranges from 1 to NR_STEP_PITCH.
*/
void CadlibDriver::InitFNums()
{
	uint8_t i, j, k, num, numStep, pas;

	numStep = 100 / NR_STEP_PITCH;
	for (num = pas = 0; pas < NR_STEP_PITCH; pas++, num += numStep)
		SetFNum(fNumNotes[pas], num, 100);
	for (i = 0; i < MAX_VOICES; i++) {
		fNumFreqPtr[i] = (uint16_t *)fNumNotes[0];
		halfToneOffset[i] = 0;
	}

	k = 0;
	for (i = 0; i < 8; i++)
		for (j = 0; j < 12; j++, k++) {
			noteDIV12[k] = i;
			noteMOD12[k] = j;
		}
}

/*
	Set the frequency of voice 'voice' to 0 hertz.
*/
void CadlibDriver::SoundChut(int voice)
{
	opl->write(0xA0 + voice, 0);
	opl->write(0xB0 + voice, 0);
}

/*
	Change pitch of voices 0 to 8, for melodic or percussive mode.
*/
void CadlibDriver::SetFreq(uint8_t voice, int pitch, uint8_t keyOn)
{
	uint16_t fNbr, t1;

	voiceKeyOn[voice] = keyOn;
	notePitch[voice] = pitch;
	pitch += halfToneOffset[voice];
	if (pitch > 95)
		pitch = 95;
	if (pitch < 0)
		pitch = 0;
	fNbr = *(fNumFreqPtr[voice] + noteMOD12[pitch]);
	opl->write(0xA0 + voice, fNbr & 0xFF);
	t1 = keyOn ? 32 : 0;
	t1 += (noteDIV12[pitch] << 2) + (0x3 & (fNbr >> 8));
	opl->write(0xB0 + voice, t1);
}

/*
	Set the values: AM Depth, VIB depth & Rhythm
*/
void CadlibDriver::SndSAmVibRhythm()
{
	uint8_t t1;

	t1 = amDepth ? 0x80 : 0;
	t1 |= vibDepth ? 0x40 : 0;
	t1 |= percussion ? 0x20 : 0;
	t1 |= percBits;
	opl->write(0xBD, t1);
}

/* --------------------------------------------
	Note sel
*/
void CadlibDriver::SndSNoteSel()
{
	opl->write(0x08, noteSel ? 64 : 0);
}

/*
	KSL, LEVEL
*/
void CadlibDriver::SndSKslLevel(uint8_t slot)
{
	unsigned t1;

	t1 = 63 - (GetLocPrm(slot, prmLevel) & 0x3f);	/* amplitude */
	t1 = slotRelVolume[slot] * t1;
	t1 += t1 + MAX_VOLUME;	/* round off to 0.5 */
	t1 = 63 - t1 / (2 * MAX_VOLUME);

	t1 |= GetLocPrm(slot, prmKsl) << 6;
	opl->write(0x40 + offsetSlot[slot], t1 & 0xFF);
}

/* --------------------------------------------
	FEED-BACK and FM (connection).
	Applicable only to operator 0 in melodic mode.
*/
void CadlibDriver::SndSFeedFm(uint8_t slot)
{
	uint8_t t1;

	if (operSlot[slot])
		return;
	t1 = GetLocPrm(slot, prmFeedBack) << 1;
	t1 |= GetLocPrm(slot, prmFm) ? 0 : 1;
	opl->write(0xC0 + (int)voiceSlot[slot], t1);
}

/*
	ATTACK, DECAY
*/
void CadlibDriver::SndSAttDecay(uint8_t slot)
{
	uint8_t t1;

	t1 = GetLocPrm(slot, prmAttack) << 4;
	t1 |= GetLocPrm(slot, prmDecay) & 0xf;
	opl->write(0x60 + (int)offsetSlot[slot], t1);
}

/*
	SUSTAIN, RELEASE
*/
void CadlibDriver::SndSSusRelease(uint8_t slot)
{
	uint8_t t1;

	t1 = GetLocPrm(slot, prmSustain) << 4;
	t1 |= GetLocPrm(slot, prmRelease) & 0xf;
	opl->write(0x80 + (int)offsetSlot[slot], t1);
}

/*
	AM, VIB, EG-TYP (Sustaining), KSR, MULTI
*/
void CadlibDriver::SndSAVEK(uint8_t slot)
{
	uint8_t t1;

	t1 = GetLocPrm(slot, prmAm) ? 0x80 : 0;
	t1 += GetLocPrm(slot, prmVib) ? 0x40 : 0;
	t1 += GetLocPrm(slot, prmStaining) ? 0x20 : 0;
	t1 += GetLocPrm(slot, prmKsr) ? 0x10 : 0;
	t1 += GetLocPrm(slot, prmMulti) & 0xf;
	opl->write(0x20 + (int)offsetSlot[slot], t1);
}

/*
	Set the wave-select parameter.
*/
void CadlibDriver::SndWaveSelect(uint8_t slot)
{
	uint8_t wave;

	if (modeWaveSel)
		wave = GetLocPrm(slot, prmWaveSel) & 0x03;
	else
		wave = 0;
	opl->write(0xE0 + offsetSlot[slot], wave);
}

/*-------------------------------------------------
	Transfer all the parameters from slot 'slot' to
	the chip.
*/
void CadlibDriver::SndSetAllPrm(uint8_t slot)
{
	SndSAmVibRhythm();
	SndSNoteSel();
	SndSKslLevel(slot);
	SndSFeedFm(slot);
	SndSAttDecay(slot);
	SndSSusRelease(slot);
	SndSAVEK(slot);
	SndWaveSelect(slot);
}

/*
------------------------------------------------------
	Set the 14 parameters ( 13 in 'param', 1 in 'waveSel')
	of slot 'slot'. Update the parameter array and the chip.
------------------------------------------------------
*/
void CadlibDriver::SetSlotParam(uint8_t slot, int16_t * param, uint8_t waveSel)
{
	for (int i = 0; i < nbLocParam - 1; i++)
		paramSlot[slot][i] = *param++;
	paramSlot[slot][nbLocParam - 1] = waveSel &= 0x3;
	SndSetAllPrm(slot);
}

void CadlibDriver::SetCharSlotParam(uint8_t slot, uint8_t * cParam, uint8_t waveSel)
{
	int16_t param[nbLocParam];

	for (int i = 0; i < nbLocParam - 1; i++)
		param[i] = *cParam++;
	SetSlotParam(slot, param, waveSel);
}

/*
	In melodic mode, initialize all voices to electric-pianos.

	In percussive mode, initialize the first 6 voices to electric-pianos
	and the percussive voices to their default timbres.
*/
void CadlibDriver::InitSlotParams()
{
	for (int i = 0; i < 18; i++)
		if (operSlot[i])
			SetCharSlotParam(i, pianoParamsOp1, 0);
		else
			SetCharSlotParam(i, pianoParamsOp0, 0);

	if (percussion)
	{
		SetCharSlotParam(12, bdOpr0, 0);
		SetCharSlotParam(15, bdOpr1, 0);
		SetCharSlotParam(16, sdOpr, 0);
		SetCharSlotParam(14, tomOpr, 0);
		SetCharSlotParam(17, cymbOpr, 0);
		SetCharSlotParam(13, hhOpr, 0);
	}
}

/*
	Initialize a line in the frequency table with shifted frequency values.
	The values are shifted a fraction (num/den) of a half-tone.
	See following routine.
*/
void CadlibDriver::SetFNum(uint16_t * fNumVec, int num, int den)
{
	int i;
	long val;

	*fNumVec++ = (uint16_t)(4 + (val = CalcPremFNum(num, den))) >> 3;
	for (i = 1; i < 12; i++) {
		val *= 106;
		*fNumVec++ = (uint16_t)(4 + (val /= 100)) >> 3;
	}
}

/*
	Routine to set 'halfToneOffset[]' & 'fNumFreqPtr[]'.
	These two global variables are used to determine the frequency
	variation to use when a note is played.

	0 <= pitchBend <= 3fffH
*/
void CadlibDriver::ChangePitch(int voice, int pitchBend)
{
	int l, t1, t2, delta;
	static int oldL = -1;
	static int oldHt;
	static uint16_t * oldPtr;

	l = (int)(pitchBend - MID_PITCH) * pitchRangeStep;
	if (oldL == l) {	/* optimisation ... */
		fNumFreqPtr[voice] = oldPtr;
		halfToneOffset[voice] = oldHt;
	}
	else {
		t1 = l / MID_PITCH;
		if (t1 < 0) {
			t2 = NR_STEP_PITCH - 1 - t1;
			oldHt = halfToneOffset[voice] = -(t2 / NR_STEP_PITCH);
			delta = (t2 - NR_STEP_PITCH + 1) % NR_STEP_PITCH;
			if (delta)
				delta = NR_STEP_PITCH - delta;
		}
		else {
			oldHt = halfToneOffset[voice] = t1 / NR_STEP_PITCH;
			delta = t1 % NR_STEP_PITCH;
		}
		oldPtr = fNumFreqPtr[voice] = (uint16_t *)fNumNotes[delta];
		oldL = l;
	}
}

/*
	Return binary value of the frequency 260.44 (C)
	shifted by +/- numdeltaDemiTon/denDeltaDemiTon multiplied by 8.

	If the numerator (numDeltaDemiTon) is positive, the frequency is
	increased; if negative, it is decreased.

	Fo = Fb( 1 + 0.06 num /den)
	Fnum8 = Fo * 65536 * 72 / 3.58e6
	
	-100 <= numDeltaDemiTon <= +100
	1 <= denDeltaDemiTon <= 100
*/
long CadlibDriver::CalcPremFNum(int numDeltaDemiTon, int denDeltaDemiTon)
{
	long	f8, fNum8, d100;

	d100 = denDeltaDemiTon * 100;
	f8 = (d100 + 6 * numDeltaDemiTon) * (26044L * 2L);	/* 260.44 * 100 * 2 */
	f8 /= d100 * 25;
	fNum8 = f8 * 16384;		/*( 16384L * 9L);	*/
	fNum8 *= 9L;
	fNum8 /= 179L * 625L;
	return fNum8;
}
