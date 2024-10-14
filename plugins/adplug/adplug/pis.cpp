/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2023 Simon Peter, <dn.tlp@gmx.net>, et al.
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

#include "pis.h"
#include <cstring>

#define PIS_NONE -1

#define OPL_NOTE_FREQUENCY_LO_B 0x143
#define OPL_NOTE_FREQUENCY_LO_C 0x157
#define OPL_NOTE_FREQUENCY_HI_B 0x287
#define OPL_NOTE_FREQUENCY_HI_C 0x2ae

#define PIS_DEFAULT_SPEED 6

#define replay_reset_voice(v) replay_set_voice_volatiles(v, 0, 0, 0);
#define EFFECT_HI(r) ((r)->effect >> 8)
#define EFFECT_LO(r) ((r)->effect & 0xff)
#define EFFECT_MIDNIB(r) (((r)->effect >> 4) & 15)
#define EFFECT_LONIB(r) ((r)->effect & 15)
#define HAS_NOTE(r) ((r)->note < 12)
#define HAS_INSTRUMENT(r) ((r)->instrument > 0)
#define IS_NOTE(n) (n < 12)

const int CpisPlayer::opl_voice_offset_into_registers[9] = {
    0,1,2,8,9,10,16,17,18
};

const int CpisPlayer::frequency_table[12] = {
    0x157,0x16B,0x181,0x198,0x1B0,0x1CA,
    0x1E5,0x202,0x220,0x241,0x263,0x287
};


/*** private methods *************************************/

void CpisPlayer::init_replay_state(PisReplayState *pstate) {
    memset(pstate, 0, sizeof(PisReplayState));

    pstate->speed = PIS_DEFAULT_SPEED;
    pstate->count = PIS_DEFAULT_SPEED - 1;
    pstate->position_jump = PIS_NONE;
    pstate->pattern_break = PIS_NONE;

    for (int i = 0; i < 9; i++) {
        pstate->voice_state[i].instrument = PIS_NONE;
    }
}

void CpisPlayer::replay_frame_routine() {
    if (is_playing) {
        replay_state.count++;
        if (replay_state.count >= replay_state.speed) {

            unpack_row();

            for (int v = 0; v < 9; v++) {
                replay_voice(v);
            }

            advance_row();
        } else {
            replay_do_per_frame_effects();
        }
    }
}

void CpisPlayer::replay_voice(int v) {
    PisVoiceState *vs = &replay_state.voice_state[v];
    PisRowUnpacked r = replay_state.row_buffer[v];

    if (EFFECT_HI(&r) == 0x03) {
        //
        // With portamento
        //
        replay_enter_row_with_portamento(v, vs, &r);
    } else {
        if (HAS_INSTRUMENT(&r)) {
            if (HAS_NOTE(&r)) {
                //
                // Instrument + note
                //
                replay_enter_row_with_instrument_and_note(v, vs, &r);
            } else {
                //
                // Instrument only
                //
                replay_enter_row_with_instrument_only(v, vs, &r);
            }
        } else {
            if (HAS_NOTE(&r)) {
                //
                // Note only
                //
                replay_enter_row_with_note_only(v, vs, &r);
            } else {
                //
                // Possibly effect only
                //
                replay_enter_row_with_possibly_effect_only(v, vs, &r);
            }
        }
    }

    replay_handle_effect(v, vs, &r);

    if (r.effect) {
        vs->previous_effect = r.effect;
    } else {
        vs->previous_effect = PIS_NONE;
        replay_reset_voice(v);
    }
}

void CpisPlayer::replay_enter_row_with_portamento(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    if (HAS_INSTRUMENT(r)) {
        replay_set_instrument(v, r->instrument);
        if (vs->volume < 63) {
            replay_set_level(v, r->instrument, PIS_NONE, 0);
        }
    }
    if (HAS_NOTE(r)) {
        vs->porta_src_freq = vs->frequency;
        vs->porta_src_octave = vs->octave;
        vs->porta_dest_freq = frequency_table[r->note];
        vs->porta_dest_octave = r->octave;

        if (vs->porta_dest_octave < vs->octave) {
            vs->porta_sign = -1;
        } else if (vs->porta_dest_octave > vs->octave) {
            vs->porta_sign = 1;
        } else {
            vs->porta_sign = (vs->porta_dest_freq < vs->frequency) ? -1 : 1;
        }
    }
}

void CpisPlayer::replay_enter_row_with_instrument_and_note(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    vs->previous_effect = PIS_NONE;

    opl_note_off(v);
    if (EFFECT_HI(r) != 0x0c) {
        //
        // Volume is not set
        //
        if (r->instrument != vs->instrument) {
            //
            // Is new instrument
            //
            replay_set_instrument(v, r->instrument);
        } else if (vs->volume < 63) {
            replay_set_level(v, r->instrument, PIS_NONE, 0);
        }

    } else {
        //
        // Volume is set
        //
        if (r->instrument != vs->instrument) {
            //
            // Is new instrument
            //
            replay_set_instrument(v, r->instrument);
        }
        replay_set_level(v, r->instrument, EFFECT_LO(r), 1);
    }
    //
    // Trigger new note
    //
    replay_set_note(v, vs, r);
}

void CpisPlayer::replay_enter_row_with_instrument_only(int v, PisVoiceState *vs, PisRowUnpacked *r) {

    if (r->instrument != vs->instrument) {
        //
        // Is new instrument
        //
        replay_set_instrument(v, r->instrument);

        //
        // Set operator level according to instrument and possibly Cxx effect
        //
        if (EFFECT_HI(r) == 0x0c) {
            replay_set_level(v, r->instrument, EFFECT_LO(r), 1);
        } else if (vs->volume < 63) {
            replay_set_level(v, r->instrument, PIS_NONE, 0);
        }

        if ((vs->previous_effect != PIS_NONE) && ((vs->previous_effect & 0xF00) == 0)) {
            //
            // Reset to base tone after arpeggio
            //
            opl_set_pitch(v, vs->frequency, vs->octave);
        }
    }
}

void CpisPlayer::replay_enter_row_with_note_only(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    vs->previous_effect = PIS_NONE;

    if (vs->instrument != PIS_NONE) {
        //
        // Set operator level according to instrument and possibly Cxx effect
        //
        if (EFFECT_HI(r) == 0x0c) {
            replay_set_level(v, vs->instrument, EFFECT_LO(r), 1);
        } else if (vs->volume < 63) {
            replay_set_level(v, vs->instrument, PIS_NONE, 0);
        }
    }
    //
    // Trigger new note
    //
    replay_set_note(v, vs, r);
}

void CpisPlayer::replay_enter_row_with_possibly_effect_only(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    //
    // Set operator level according to instrument and Cxx effect
    //
    if (vs->instrument != PIS_NONE && EFFECT_HI(r) == 0x0c) {
        replay_set_level(v, vs->instrument, EFFECT_LO(r), 1);
    }

    if ((vs->previous_effect != PIS_NONE) && ((vs->previous_effect & 0xF00) == 0)) {
        //
        // Reset to base tone after arpeggio
        //
        opl_set_pitch(v, vs->frequency, vs->octave);
    }
}

void CpisPlayer::replay_handle_effect(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    int effect_hi = EFFECT_HI(r);
    switch (effect_hi) {
    case 0x00: // arpeggio
        if (EFFECT_LO(r)) {
            replay_handle_arpeggio(v, vs, r);
        } else {
            vs->arpeggio_flag = 0;
        }
        break;
    case 0x01: // slide up
        vs->slide_increment = EFFECT_LO(r);
        break;
    case 0x02: // slide down
        vs->slide_increment = -EFFECT_LO(r);
        break;
    case 0x03: // tone portamento
        replay_set_voice_volatiles(v, 0, 0, EFFECT_LO(r));
        break;
    case 0x0b: // position jump
        replay_handle_posjmp(v, r);
        break;
    case 0x0d: // pattern break
        replay_handle_ptnbreak(v, r);
        break;
    case 0x0e: // Exx commands
        replay_handle_exx_command(v, vs, r);
        break;
    case 0x0f: // set speed
        replay_handle_speed(v, r);
        break;
    }
}

void CpisPlayer::replay_handle_exx_command(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    switch (EFFECT_MIDNIB(r)) {
    case 0x06: // loop
        replay_handle_loop(v, r);
        break;
    case 0x0a: // volume slide up
    case 0x0b: // volume slide down
        replay_handle_volume_slide(v, vs, r);
        break;
    }
}

void CpisPlayer::replay_handle_loop(int v, PisRowUnpacked *r) {
    if (!replay_state.loop_flag) {
        //
        // Playing for the first time
        //
        if (EFFECT_LONIB(r) == 0) {
            //
            // Set loop start row
            //
            replay_state.loop_start_row = replay_state.row;
        } else {
            //
            // Initialize loop counter
            //
            replay_state.loop_count = EFFECT_LONIB(r);
            replay_state.loop_flag = 1;
        }
    }

    if ((replay_state.loop_flag) && (EFFECT_LONIB(r))) {
        //
        // Repeating
        //
        replay_state.loop_count--;

        if (replay_state.loop_count >= 0) {
            replay_state.row = replay_state.loop_start_row - 1;
        } else {
            replay_state.loop_flag = 0;
        }
    }
}

void CpisPlayer::replay_handle_volume_slide(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    int level;

    if (vs->instrument != PIS_NONE) {
        level = (EFFECT_MIDNIB(r) == 0x0a)
                    ? (vs->volume + EFFECT_LONIB(r))
                    : (vs->volume - EFFECT_LONIB(r));

        if (level < 2)
            level = 2;
        else if (level > 63)
            level = 63;

        replay_set_level(v, vs->instrument, level, 0);
    }
}

void CpisPlayer::replay_do_per_frame_effects() {

    replay_state.arpeggio_index++;
    if (replay_state.arpeggio_index == 3)
        replay_state.arpeggio_index = 0;

    for (int v = 0; v < 8; v++) {
        PisVoiceState *vs = &replay_state.voice_state[v];
        if (vs->slide_increment) {
            vs->frequency += vs->slide_increment;
            opl_set_pitch(v, vs->frequency, vs->octave);
        } else if (vs->porta_increment) {
            replay_do_per_frame_portamento(v, vs);
        } else if (vs->arpeggio_flag) {
            int freq = vs->arpeggio_freq[replay_state.arpeggio_index];
            opl_set_pitch(v, freq, vs->arpeggio_octave[replay_state.arpeggio_index]);
        }
    }
}

void CpisPlayer::replay_do_per_frame_portamento(int v, PisVoiceState *vs) {
    if (vs->porta_sign == 1) {
        vs->frequency += vs->porta_increment;
        if ((vs->octave == vs->porta_dest_octave) && (vs->frequency > vs->porta_dest_freq)) {
            vs->frequency = vs->porta_dest_freq;
            vs->porta_increment = 0;
        }
        if (vs->frequency > OPL_NOTE_FREQUENCY_HI_B) {
            vs->frequency = OPL_NOTE_FREQUENCY_LO_B + (vs->frequency - OPL_NOTE_FREQUENCY_HI_B);
            vs->octave++;
        }
    } else {
        vs->frequency -= vs->porta_increment;
        if ((vs->octave == vs->porta_dest_octave) && (vs->frequency < vs->porta_dest_freq)) {
            vs->frequency = vs->porta_dest_freq;
            vs->porta_increment = 0;
        }
        if (vs->frequency < OPL_NOTE_FREQUENCY_LO_C) {
            vs->frequency = OPL_NOTE_FREQUENCY_HI_C - (OPL_NOTE_FREQUENCY_LO_C - vs->frequency);
            vs->octave--;
        }
    }
    opl_set_pitch(v, vs->frequency, vs->octave);
}

void CpisPlayer::replay_handle_arpeggio(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    int an1, an2;
    if (EFFECT_LO(r) != (vs->previous_effect & 0xff)) {
        vs->arpeggio_freq[0] = frequency_table[vs->note];
        vs->arpeggio_octave[0] = vs->octave;
        an1 = vs->note + EFFECT_MIDNIB(r);
        an2 = vs->note + EFFECT_LONIB(r);
        if (IS_NOTE(an1)) {
            vs->arpeggio_freq[1] = frequency_table[an1];
            vs->arpeggio_octave[1] = vs->octave;
        } else {
            vs->arpeggio_freq[1] = frequency_table[an1 - 12];
            vs->arpeggio_octave[1] = vs->octave + 1;
        }
        if (IS_NOTE(an2)) {
            vs->arpeggio_freq[2] = frequency_table[an2];
            vs->arpeggio_octave[2] = vs->octave;
        } else {
            vs->arpeggio_freq[2] = frequency_table[an2 - 12];
            vs->arpeggio_octave[2] = vs->octave + 1;
        }
        vs->arpeggio_flag = 1;
    }

    vs->slide_increment = 0;
    vs->porta_increment = 0;
}

void CpisPlayer::replay_handle_posjmp(int v, PisRowUnpacked *r) {
    replay_reset_voice(v);
    replay_state.position_jump = EFFECT_LO(r);
}

void CpisPlayer::replay_handle_ptnbreak(int v, PisRowUnpacked *r) {
    replay_reset_voice(v);
    replay_state.pattern_break = EFFECT_LO(r);
}

void CpisPlayer::replay_handle_speed(int v, PisRowUnpacked *r) {
    replay_reset_voice(v);
    if (EFFECT_LO(r)) {
        replay_state.speed = EFFECT_LO(r);
    } else {
        is_playing = 0;
    }
}

void CpisPlayer::replay_set_note(int v, PisVoiceState *vs, PisRowUnpacked *r) {
    int frequency = frequency_table[r->note];
    opl_set_pitch(v, frequency, r->octave);
    vs->note = r->note;
    vs->octave = r->octave;
    vs->frequency = frequency;
}

void CpisPlayer::replay_set_instrument(int v, int instr_index) {
    PisInstrument *pinstr = &module.instrument[instr_index];
    opl_set_instrument(v, pinstr);
    replay_state.voice_state[v].instrument = instr_index;
}

void CpisPlayer::replay_set_level(int v, int instr_index, int gain, int do_apply_correction) {
    int base, l1, l2;
    PisInstrument *instr = &module.instrument[instr_index];

    base = do_apply_correction
               ? 62
               : 64;

    if (gain == PIS_NONE) {
        gain = 64;
        replay_state.voice_state[v].volume = 63;
    } else {
        replay_state.voice_state[v].volume = gain;
    }

    l1 = base - (gain * (64 - instr->lev1) >> 6);
    l2 = base - (gain * (64 - instr->lev2) >> 6);

    oplout(0x40 + opl_voice_offset_into_registers[v], l1);
    oplout(0x43 + opl_voice_offset_into_registers[v], l2);
}

void CpisPlayer::replay_set_voice_volatiles(int v, int arpeggio_flag, int slide_increment, int porta_increment) {
    PisVoiceState *vs = &replay_state.voice_state[v];
    vs->arpeggio_flag = arpeggio_flag;
    vs->slide_increment = slide_increment;
    vs->porta_increment = porta_increment;
}

void CpisPlayer::unpack_row() {
    int pattern_index;
    uint32_t *pptn;
    uint32_t packed;
    uint8_t b1, b2, el;

    for (int v = 0; v < 9; v++) {
        pattern_index = module.order[replay_state.position][v];
        pptn = module.pattern[pattern_index];
        packed = pptn[replay_state.row];

        el = packed & 0xff;
        packed >>= 8;
        b2 = packed & 0xff;
        packed >>= 8;
        b1 = packed & 0xff;

        replay_state.row_buffer[v].note = b1 >> 4;
        replay_state.row_buffer[v].octave = (b1 >> 1) & 7;
        replay_state.row_buffer[v].instrument = ((b1 & 1) << 4) | (b2 >> 4);
        replay_state.row_buffer[v].effect = ((b2 & 15) << 8) | el;
    }
}

void CpisPlayer::advance_row() {
    if (replay_state.position_jump >= 0) {
        replay_state.position = replay_state.position_jump;
        is_playing = 0; // Treat pattern jump as stop

        if (replay_state.pattern_break == PIS_NONE) {
            //
            // Position jump without pattern break
            //
            replay_state.row = 0;
        } else {
            //
            // Position jump with pattern break
            //
            replay_state.row = replay_state.pattern_break;
            replay_state.pattern_break = PIS_NONE;
        }
        replay_state.position_jump = PIS_NONE;
    } else if (replay_state.pattern_break >= 0) {
        //
        // Pattern break
        //
        replay_state.position++;
        if (replay_state.position == module.length) {
            replay_state.position = 0;
            is_playing = 0;
        }
        replay_state.row = replay_state.pattern_break;
        replay_state.pattern_break = PIS_NONE;
    } else {
        //
        // Simple row advance
        //
        replay_state.row++;
        if (replay_state.row == 64) {
            replay_state.row = 0;
            replay_state.position++;
            if (replay_state.position == module.length) {
                replay_state.position = 0;
                is_playing = 0;
            }
        }
    }

    replay_state.count = 0;
}

void CpisPlayer::load_module(binistream *f, PisModule *pmodule) {
    int i, j;

    memset(pmodule, 0, sizeof(PisModule));

    pmodule->length = f->readInt(1);
    pmodule->number_of_patterns = f->readInt(1);
    pmodule->number_of_instruments = f->readInt(1);

    for (i = 0; i < pmodule->number_of_patterns; i++) {
        pmodule->pattern_map[i] = f->readInt(1);
    }

    for (i = 0; i < pmodule->number_of_instruments; i++) {
        pmodule->instrument_map[i] = f->readInt(1);
    }

    f->readString((char *)pmodule->order, 9 * pmodule->length);

    for (i = 0; i < pmodule->number_of_patterns; i++) {
        j = pmodule->pattern_map[i];
        load_pattern(pmodule->pattern[j], f);
    }

    for (i = 0; i < pmodule->number_of_instruments; i++) {
        j = pmodule->instrument_map[i];
        load_instrument(&pmodule->instrument[j], f);
    }
}

void CpisPlayer::load_pattern(uint32_t *destination, binistream *f) {
    int row;
    uint32_t packed;
    for (row = 0; row < 64; row++) {
        packed = f->readInt(1);
        packed <<= 8;
        packed |= f->readInt(1);
        packed <<= 8;
        packed |= f->readInt(1);
        destination[row] = packed;
    }
}

void CpisPlayer::load_instrument(PisInstrument *pinstr, binistream *f) {
    pinstr->mul1 = f->readInt(1);
    pinstr->mul2 = f->readInt(1);
    pinstr->lev1 = f->readInt(1);
    pinstr->lev2 = f->readInt(1);
    pinstr->atd1 = f->readInt(1);
    pinstr->atd2 = f->readInt(1);
    pinstr->sur1 = f->readInt(1);
    pinstr->sur2 = f->readInt(1);
    pinstr->wav1 = f->readInt(1);
    pinstr->wav2 = f->readInt(1);
    pinstr->fbcon = f->readInt(1);
}

void CpisPlayer::opl_set_pitch(int v, int freq, int octave) {
    oplout(0xa0 + v, freq & 0xff);
    oplout(0xb0 + v, 0x20 | (octave << 2) | (freq >> 8));
}

void CpisPlayer::opl_note_off(int v) {
    oplout(0xb0 + v, 0);
}

void CpisPlayer::opl_set_instrument(int v, PisInstrument *instr) {
    int opl_register = 0x20 + opl_voice_offset_into_registers[v];
    oplout(opl_register, instr->mul1);
    opl_register += 3;
    oplout(opl_register, instr->mul2);
    opl_register += 0x1d;
    oplout(opl_register, instr->lev1);
    opl_register += 3;
    oplout(opl_register, instr->lev2);
    opl_register += 0x1d;
    oplout(opl_register, instr->atd1);
    opl_register += 3;
    oplout(opl_register, instr->atd2);
    opl_register += 0x1d;
    oplout(opl_register, instr->sur1);
    opl_register += 3;
    oplout(opl_register, instr->sur2);
    opl_register += 0x5d;
    oplout(opl_register, instr->wav1);
    opl_register += 3;
    oplout(opl_register, instr->wav2);
    opl_register += 0x1d;
    oplout(0xc0 + v, instr->fbcon);
}

void CpisPlayer::oplout(int r, int v) {
    opl->write(r, v);
}

/*** public methods *************************************/

CPlayer *CpisPlayer::factory(Copl *newopl) {
    return new CpisPlayer(newopl);
}

bool CpisPlayer::load(const std::string &filename, const CFileProvider &fp) {
    binistream *f = fp.open(filename);
    if (!f)
        return false;

    // file validation section
    if (!fp.extension(filename, ".pis")) {
        fp.close(f);
        return false;
    }

    load_module(f, &module);

    fp.close(f);

    rewind(0);
    is_playing = 1;

    return true;
}

bool CpisPlayer::update() {
    replay_frame_routine();

    return is_playing;
}

void CpisPlayer::rewind(int subsong) {
    init_replay_state(&replay_state);

    opl->init();
    opl->write(1, 0x20); // enable waveform control
    is_playing = 1;
}

float CpisPlayer::getrefresh() {
    return 50.0f;
}
