/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * pis.cpp - PIS Player (Beni Tracker) by Jonas Santoso <jonas.santoso@gmail.com>
 *           adapted from 'pisplay' by Dmitry Smagin <dmitry.s.smagin@gmail.com>
 *
 * REFERENCES:
 * https://github.com/klubderkluebe/pisplay/
 *
 */

#include <stdint.h>

#include "player.h"

class CpisPlayer : public CPlayer {
public:
    static CPlayer *factory(Copl *newopl);

    CpisPlayer(Copl *newopl) : CPlayer(newopl){};
    ~CpisPlayer(){};

    bool load(const std::string &filename, const CFileProvider &fp);
    bool update();
    void rewind(int subsong);
    float getrefresh();

    std::string gettype() { return std::string("Beni Tracker PIS module"); };

private:
    typedef struct {
        uint8_t mul1, mul2; // multiplier
        uint8_t lev1, lev2; // level
        uint8_t atd1, atd2; // attack / decay
        uint8_t sur1, sur2; // sustain / release
        uint8_t wav1, wav2; // waveform
        uint8_t fbcon;      // feedback / connection_type
    } PisInstrument;

    typedef struct {
        uint8_t length; // length of order list
        uint8_t number_of_patterns; // # of patterns stored in module
        uint8_t number_of_instruments; // # of instruments stored in module
        uint8_t pattern_map[128]; // maps physical to logical pattern
        uint8_t instrument_map[32]; // maps physical to logical instrument
        uint8_t order[256][9]; // order list for each channel
        uint32_t pattern[128][64]; // pattern data
        PisInstrument instrument[64]; // instrument data
    } PisModule;

    typedef struct {
        int note;
        int octave;
        int instrument;
        int effect;
    } PisRowUnpacked;

    typedef struct {
        int instrument;
        int volume;
        int note;
        int frequency;
        int octave;
        int previous_effect;
        int slide_increment;
        int porta_increment;
        int porta_src_freq;
        int porta_src_octave;
        int porta_dest_freq;
        int porta_dest_octave;
        int porta_sign;
        int arpeggio_flag;
        int arpeggio_freq[3];
        int arpeggio_octave[3];
    } PisVoiceState;

    typedef struct {
        int speed;
        int count;
        int position;
        int row;
        int position_jump;
        int pattern_break;
        int arpeggio_index;
        int loop_flag;
        int loop_start_row;
        int loop_count;
        PisVoiceState voice_state[9];
        PisRowUnpacked row_buffer[9];
    } PisReplayState;

    // Player control
    void pisplay_init();
    void pisplay_shutdown();
    void pisplay_load_and_play(const char *path);
    void load_module(binistream *f, PisModule *module);
    void load_pattern(uint32_t *destination, binistream *f);
    void load_instrument(PisInstrument *pinstr, binistream *f);

    // Replay routine
    void init_replay_state(PisReplayState *pstate);
    void replay_frame_routine();
    void replay_voice(int);
    void unpack_row();
    void advance_row();
    void replay_enter_row_with_portamento(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_enter_row_with_instrument_and_note(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_enter_row_with_instrument_only(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_enter_row_with_note_only(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_enter_row_with_possibly_effect_only(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_handle_effect(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_handle_arpeggio(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_handle_posjmp(int v, PisRowUnpacked *r);
    void replay_handle_ptnbreak(int v, PisRowUnpacked *r);
    void replay_handle_speed(int v, PisRowUnpacked *r);
    void replay_handle_exx_command(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_handle_loop(int v, PisRowUnpacked *r);
    void replay_handle_volume_slide(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_do_per_frame_effects();
    void replay_do_per_frame_portamento(int v, PisVoiceState *vs);
    void replay_set_note(int v, PisVoiceState *vs, PisRowUnpacked *r);
    void replay_set_instrument(int v, int instr_index);
    void replay_set_level(int v, int instr_index, int gain, int do_apply_correction);
    void replay_set_voice_volatiles(int v, int arpeggio_flag, int slide_increment, int porta_increment);

    void opl_set_pitch(int v, int freq, int octave);
    void opl_note_off(int v);
    void opl_set_instrument(int v, PisInstrument *instr);
    void oplout(int r, int v);

    static const int opl_voice_offset_into_registers[9];

    static const int frequency_table[12];

    PisModule module;
    PisReplayState replay_state;
    int is_playing;
};
