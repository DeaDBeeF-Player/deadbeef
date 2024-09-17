/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * herad.h - Herbulot AdLib Player by Stas'M <binarymaster@mail.ru>
 *
 * Thanks goes to co-workers: 
 *   -=CHE@TER=- (SQX decompression)
 *   SynaMax (general documentation, reverse-engineering, testing)
 *   Jepael (timer code sample, DOS driver shell)
 *   Daniël van de Burgt "thatdutchguy" (pitch slides code sample)
 *
 * REFERENCES:
 * http://www.vgmpf.com/Wiki/index.php/HERAD
 */

#ifndef H_ADPLUG_HERADPLAYER
#define H_ADPLUG_HERADPLAYER

#include <stdint.h>
#include "player.h"

#define HERAD_MIN_SIZE		6		/* Minimum file size for compression detection */
#define HERAD_MAX_SIZE		75775		/* Maximum possible file size: 0xFFFF + 256 * HERAD_INST_SIZE */
#define HERAD_HEAD_SIZE		52		/* File header size */
#define HERAD_COMP_NONE		0
#define HERAD_COMP_HSQ		1
#define HERAD_COMP_SQX		2
#define HERAD_MAX_TRACKS	21
#define HERAD_INST_SIZE		40
#define HERAD_INSTMODE_SDB1	0		/* HERAD SDB version 1 instrument */
#define HERAD_INSTMODE_SDB2	1		/* HERAD SDB version 2 instrument */
#define HERAD_INSTMODE_AGD	4		/* HERAD AGD instrument */
#define HERAD_INSTMODE_KMAP	-1		/* HERAD version 2 keymap */
#define HERAD_FNUM_MIN		325		/* Minimum note frequency number */
#define HERAD_FNUM_MAX		685		/* Maximum note frequency number */
#define HERAD_BEND_CENTER	0x40	/* Pitch bend middle value */
#define HERAD_NOTE_OFF		0
#define HERAD_NOTE_ON		1
#define HERAD_NOTE_UPDATE	2
#define HERAD_NUM_VOICES	9
#define HERAD_NUM_NOTES		12
#define HERAD_MEASURE_TICKS	96
//#define HERAD_USE_LOOPING			/* Uncomment this to enable looping */

class CheradPlayer: public CPlayer
{
public:
	static CPlayer *factory(Copl *newopl);

	CheradPlayer(Copl *newopl)
		: CPlayer(newopl), track(0), chn(0), inst(0)
		{ }
	~CheradPlayer()
	{
		if (track)
		{
			for (int i = 0; i < nTracks; i++)
			{
				if (track[i].data) delete[] track[i].data;
			}
			delete[] track;
		}
		if (chn) delete[] chn;
		if (inst) delete[] inst;
	};

	bool load(const std::string &filename, const CFileProvider &fp);
	bool update();
	void rewind(int subsong);

	float getrefresh()
	{
		return (float)200.299;
	};

	unsigned int getspeed()
	{
		return wSpeed;
	};

	unsigned int getpatterns()
	{
		return total_ticks / HERAD_MEASURE_TICKS + (total_ticks % HERAD_MEASURE_TICKS ? 1 : 0);
	};

	unsigned int getpattern()
	{
		return (ticks_pos <= 0 ? 0: (ticks_pos - 1) / HERAD_MEASURE_TICKS + 1);
	};

	unsigned int getrow()
	{
		return (ticks_pos <= 0 ? 0 : (ticks_pos - 1) % HERAD_MEASURE_TICKS);
	};

	std::string gettype();

	unsigned int getinstruments()
	{
		return inst ? nInsts : 0;
	};

	std::string getinstrument(unsigned int n)
	{
		return std::string();
	};

private:
	bool validEvent(int i, uint16_t * offset, bool v2);
	uint8_t validTracks();
	uint32_t GetTicks(uint8_t t);
	void executeCommand(uint8_t t);
	void processEvents();
	void ev_noteOn(uint8_t ch, uint8_t note, uint8_t vel);
	void ev_noteOff(uint8_t ch, uint8_t note, uint8_t vel);
	void ev_programChange(uint8_t ch, uint8_t prog);
	void ev_aftertouch(uint8_t ch, uint8_t vel);
	void ev_pitchBend(uint8_t ch, uint8_t bend);
	void playNote(uint8_t c, uint8_t note, uint8_t state);
	void setFreq(uint8_t c, uint8_t oct, uint16_t freq, bool on);
	void changeProgram(uint8_t c, uint8_t i);
	void macroModOutput(uint8_t c, uint8_t i, int8_t sens, uint8_t level);
	void macroCarOutput(uint8_t c, uint8_t i, int8_t sens, uint8_t level);
	void macroFeedback(uint8_t c, uint8_t i, int8_t sens, uint8_t level);
	void macroTranspose(uint8_t * note, uint8_t i);
	void macroSlide(uint8_t c);

	static const uint8_t slot_offset[HERAD_NUM_VOICES];
	static const uint16_t FNum[HERAD_NUM_NOTES];
	static const uint8_t fine_bend[HERAD_NUM_NOTES + 1];
	static const uint8_t coarse_bend[10];

protected:
	bool songend;
	int16_t wTime;
	uint32_t ticks_pos;	/* current tick counter */
	uint32_t total_ticks;	/* total ticks in song */

	uint8_t comp;		/* File compression type (see HERAD_COMP_*) */
	bool AGD;			/* Whether this is HERAD AGD (OPL3) */
	bool v2;			/* Whether this is HERAD version 2 */
	uint8_t nTracks;		/* Number of tracks */
	uint8_t nInsts;		/* Number of instruments */

	uint16_t wLoopStart;	/* Loop starts at this measure (0 = don't loop) */
	uint16_t wLoopEnd;	/* Loop ends at this measure (0 = don't loop) */
	uint16_t wLoopCount;	/* Number of times the selected measures will play (0 = loop forever; >0 - play N times) */
	uint16_t wSpeed;		/* Fixed point value that controls music speed. Value range is 0x0100 - 0x8100 */

	struct herad_trk {
		// stored variables
		uint16_t size;			/* data size */
		uint8_t * data;			/* event data */

		// variables for playback
		uint16_t pos;			/* data position */
		uint32_t counter;			/* tick counter */
		uint16_t ticks;			/* ticks to wait for next event */
	};
	struct herad_chn {
		uint8_t program;			/* current instrument */
		uint8_t playprog;			/* current playing instrument (used for keymap) */
		uint8_t note;			/* current note */
		bool keyon;				/* note is active */
		uint8_t bend;			/* current pitch bend */
		uint8_t slide_dur;		/* pitch slide duration */
	};
	struct herad_inst_data {
		int8_t mode;			/* instrument mode (see HERAD_INSTMODE_*) */
		uint8_t voice;			/* voice number, unused */
		uint8_t mod_ksl;			/* modulator: KSL */
		uint8_t mod_mul;			/* modulator: frequency multiplier */
		uint8_t feedback;			/* feedback */
		uint8_t mod_A;			/* modulator: attack */
		uint8_t mod_S;			/* modulator: sustain */
		uint8_t mod_eg;			/* modulator: envelope gain */
		uint8_t mod_D;			/* modulator: decay */
		uint8_t mod_R;			/* modulator: release */
		uint8_t mod_out;			/* modulator: output level */
		uint8_t mod_am;			/* modulator: amplitude modulation (tremolo) */
		uint8_t mod_vib;			/* modulator: frequency vibrato */
		uint8_t mod_ksr;			/* modulator: key scaling/envelope rate */
		uint8_t con;			/* connector */
		uint8_t car_ksl;			/* carrier: KSL */
		uint8_t car_mul;			/* carrier: frequency multiplier */
		uint8_t pan;			/* panning */
		uint8_t car_A;			/* carrier: attack */
		uint8_t car_S;			/* carrier: sustain */
		uint8_t car_eg;			/* carrier: envelope gain */
		uint8_t car_D;			/* carrier: decay */
		uint8_t car_R;			/* carrier: release */
		uint8_t car_out;			/* carrier: output level */
		uint8_t car_am;			/* carrier: amplitude modulation (tremolo) */
		uint8_t car_vib;			/* carrier: frequency vibrato */
		uint8_t car_ksr;			/* carrier: key scaling/envelope rate */
		int8_t mc_fb_at;			/* macro: modify feedback with aftertouch */
		uint8_t mod_wave;			/* modulator: waveform select */
		uint8_t car_wave;			/* carrier: waveform select */
		int8_t mc_mod_out_vel;		/* macro: modify modulator output level with note velocity */
		int8_t mc_car_out_vel;		/* macro: modify carrier output level with note velocity */
		int8_t mc_fb_vel;			/* macro: modify feedback with note velocity */
		uint8_t mc_slide_coarse;	/* macro: pitch slide coarse tune */
		uint8_t mc_transpose;		/* macro: root note transpose */
		uint8_t mc_slide_dur;		/* macro: pitch slide duration (in ticks) */
		int8_t mc_slide_range;		/* macro: pitch slide range */
		uint8_t dummy;			/* unknown value */
		int8_t mc_mod_out_at;		/* macro: modify modulator output level with aftertouch */
		int8_t mc_car_out_at;		/* macro: modify carrier output level with aftertouch */
	};
	struct herad_keymap {
		int8_t mode;			/* same as herad_inst_data */
		uint8_t voice;			/* same as herad_inst_data */
		uint8_t offset;			/* keymap start note: 0 => C2 (24), 24 => C4 (48) */
		uint8_t dummy;			/* unknown value */
		uint8_t index[HERAD_INST_SIZE-4];	/* array of instruments */
	};
	union herad_inst {
		uint8_t data[HERAD_INST_SIZE];	/* array of 8-bit parameters */
		herad_inst_data param;			/* structure of parameters */
		herad_keymap keymap;			/* keymap structure */
	};
	herad_trk * track;				/* event tracks [nTracks] */
	herad_chn * chn;					/* active channels [nTracks] */
	herad_inst * inst;				/* instruments [nInsts] */

	uint32_t loop_pos;
	uint16_t loop_times;
	herad_trk loop_data[HERAD_MAX_TRACKS];
};

#endif
