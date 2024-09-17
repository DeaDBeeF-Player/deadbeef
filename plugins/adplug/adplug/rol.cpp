/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * rol.cpp - ROL Player by OPLx <oplx@yahoo.com>
 */
#include <cstring>
#include <algorithm>

#include "rol.h"
#include "debug.h"

//---------------------------------------------------------
static int16_t  const skVersionMajor         = 4;
static int16_t  const skVersionMinor         = 0;
//---------------------------------------------------------
static inline float fmin(int const a, int const b)
{
    return static_cast<float>(a < b ? a : b);
}
//---------------------------------------------------------
int   const CrolPlayer::kMaxTickBeat         = 60;
float const CrolPlayer::kDefaultUpdateTme    = 18.2f;

/*** public methods **************************************/

CPlayer * CrolPlayer::factory(Copl * pNewOpl)
{
  return new CrolPlayer(pNewOpl);
}
//---------------------------------------------------------
CrolPlayer::CrolPlayer(Copl * const pNewOpl)
    : CcomposerBackend   (pNewOpl)
    , mpROLHeader        (NULL)
    , mTempoEvents       ()
    , mVoiceData         ()
    , mRefresh           (kDefaultUpdateTme)
    , mNextTempoEvent    (0)
    , mCurrTick          (0)
    , mTimeOfLastNote    (0)
{
}
//---------------------------------------------------------
CrolPlayer::~CrolPlayer()
{
    if (mpROLHeader != NULL)
    {
        delete mpROLHeader;
        mpROLHeader = NULL;
    }
}
//---------------------------------------------------------
bool CrolPlayer::load(const std::string & filename, const CFileProvider & fp)
{
    binistream *f = fp.open(filename);

    if (!f)
    {
        return false;
    }

    char *fn = new char[filename.length()+13];
    int i;
    std::string bnk_filename;

    AdPlug_LogWrite("*** CrolPlayer::load(f, \"%s\") ***\n", filename.c_str());
    strcpy(fn,filename.data());
    for (i = strlen(fn) - 1; i >= 0; i--)
    {
        if (fn[i] == '/' || fn[i] == '\\')
        {
            break;
        }
    }
    strcpy(fn+i+1,"standard.bnk");
    bnk_filename = fn;
    delete [] fn;
    AdPlug_LogWrite("bnk_filename = \"%s\"\n",bnk_filename.c_str());

    mpROLHeader = new SRolHeader;
    memset(mpROLHeader, 0, sizeof(SRolHeader));

    mpROLHeader->version_major = static_cast<uint16_t>(f->readInt(2));
    mpROLHeader->version_minor = static_cast<uint16_t>(f->readInt(2));

    // Version check
    if ((mpROLHeader->version_major != skVersionMinor) || (mpROLHeader->version_minor != skVersionMajor))
    {
        AdPlug_LogWrite("Unsupported file version %d.%d or not a ROL file!\n",
                        mpROLHeader->version_major, mpROLHeader->version_minor);
        AdPlug_LogWrite("--- CrolPlayer::load ---\n");
        fp.close(f);
        return false;
    }

    f->readString(mpROLHeader->comment, ROL_COMMENT_SIZE);
    mpROLHeader->comment[ROL_COMMENT_SIZE - 1] = 0;
    mpROLHeader->ticks_per_beat    = static_cast<uint16_t>(f->readInt(2));
    mpROLHeader->beats_per_measure = static_cast<uint16_t>(f->readInt(2));
    mpROLHeader->edit_scale_y      = static_cast<uint16_t>(f->readInt(2));
    mpROLHeader->edit_scale_x      = static_cast<uint16_t>(f->readInt(2));

    f->seek(ROL_UNUSED1_SIZE, binio::Add); // Seek past 'SRolHeader.(unused1)' field of the header.

    mpROLHeader->mode = static_cast<uint8_t>(f->readInt(1));

    f->seek(ROL_UNUSED2_SIZE + ROL_FILLER0_SIZE + ROL_FILLER1_SIZE, binio::Add); // Seek past 'SRolHeader.(unused2, filler0, filler1)' field of header

    mpROLHeader->basic_tempo = static_cast<float>(f->readFloat(binio::Single));

    load_tempo_events(f);

    mTimeOfLastNote = 0;

    if (load_voice_data(f, bnk_filename, fp) != true)
    {
      AdPlug_LogWrite("CrolPlayer::load_voice_data(f) failed!\n");
      AdPlug_LogWrite("--- CrolPlayer::load ---\n");

      fp.close(f);
      return false;
    }

    fp.close(f);

    rewind(0);
    AdPlug_LogWrite("--- CrolPlayer::load ---\n");
    return true;
}
//---------------------------------------------------------
bool CrolPlayer::update()
{
    if ((mNextTempoEvent < mTempoEvents.size()) &&
        (mTempoEvents[mNextTempoEvent].time == mCurrTick))
    {
        SetRefresh(mTempoEvents[mNextTempoEvent].multiplier);
        ++mNextTempoEvent;
    }

    TVoiceData::iterator curr = mVoiceData.begin();
    TVoiceData::iterator end  = mVoiceData.end();
    int voice                 = 0;

    while(curr != end)
    {
        UpdateVoice(voice, *curr);
        ++curr;
        ++voice;
    }

    ++mCurrTick;

    if (mCurrTick > mTimeOfLastNote)
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------
void CrolPlayer::frontend_rewind(int subsong)
{
    TVoiceData::iterator curr = mVoiceData.begin();
    TVoiceData::iterator end  = mVoiceData.end();

    while(curr != end)
    {
        CVoiceData & voice = *curr;

        voice.Reset();
        ++curr;
    }

    mNextTempoEvent = 0;
    mCurrTick = 0;

    SetRhythmMode(mpROLHeader->mode ^ 1);

    SetRefresh(1.0f);
}
//---------------------------------------------------------
void CrolPlayer::SetRefresh( float const multiplier )
{
    float const tickBeat = static_cast<float>(fmin(kMaxTickBeat, mpROLHeader->ticks_per_beat));

    mRefresh = (tickBeat*mpROLHeader->basic_tempo*multiplier) / 60.0f;
}
//---------------------------------------------------------
float CrolPlayer::getrefresh()
{
    return mRefresh;
}
//---------------------------------------------------------
void CrolPlayer::UpdateVoice(int const voice, CVoiceData & voiceData)
{
    TNoteEvents const & nEvents = voiceData.note_events;

    if (nEvents.empty() || (voiceData.mEventStatus & CVoiceData::kES_NoteEnd))
    {
        return; // no note data to process, don't bother doing anything.
    }

    TInstrumentEvents const & iEvents = voiceData.instrument_events;
    TVolumeEvents     const & vEvents = voiceData.volume_events;
    TPitchEvents      const & pEvents = voiceData.pitch_events;

    if ((voiceData.mEventStatus & CVoiceData::kES_InstrEnd) == 0)
    {
        if (voiceData.next_instrument_event < iEvents.size())
        {
            if (iEvents[voiceData.next_instrument_event].time == mCurrTick)
            {
                SetInstrument(voice, iEvents[voiceData.next_instrument_event].ins_index);
                ++voiceData.next_instrument_event;
            }
        }
        else
        {
            voiceData.mEventStatus |= CVoiceData::kES_InstrEnd;
        }
    }

    if ((voiceData.mEventStatus & CVoiceData::kES_VolumeEnd) == 0)
    {
        if (voiceData.next_volume_event < vEvents.size())
        {
            if (vEvents[voiceData.next_volume_event].time == mCurrTick)
            {
                SVolumeEvent const & volumeEvent = vEvents[voiceData.next_volume_event];

                uint8_t const volume = (uint8_t)(kMaxVolume * volumeEvent.multiplier);

                SetVolume(voice, volume);

                ++voiceData.next_volume_event; // move to next volume event
            }
        }
        else
        {
            voiceData.mEventStatus |= CVoiceData::kES_VolumeEnd;
        }        
    }

    if (voiceData.mForceNote || (voiceData.current_note_duration > voiceData.mNoteDuration-1))
    {
        if (mCurrTick != 0)
        {
            ++voiceData.current_note;
        }

        if (voiceData.current_note < nEvents.size())
        {
            SNoteEvent const & noteEvent = nEvents[voiceData.current_note];

            NoteOn(voice, noteEvent.number);
            voiceData.current_note_duration = 0;
            voiceData.mNoteDuration         = noteEvent.duration;
            voiceData.mForceNote            = false;
        }
        else
        {
            NoteOff(voice);
            voiceData.mEventStatus |= CVoiceData::kES_NoteEnd;
            return;
        }
    }

    if ((voiceData.mEventStatus & CVoiceData::kES_PitchEnd) == 0)
    {
        if ( voiceData.next_pitch_event < pEvents.size() )
        {
            if (pEvents[voiceData.next_pitch_event].time == mCurrTick)
            {
                SetPitch(voice, pEvents[voiceData.next_pitch_event].variation);
                ++voiceData.next_pitch_event;
            }
        }
        else
        {
            voiceData.mEventStatus |= CVoiceData::kES_PitchEnd;
        }
    }

    ++voiceData.current_note_duration;
}
//---------------------------------------------------------
void CrolPlayer::SetPitch(int const voice, float const variation)
{
    uint16_t const pitchBend = (variation == 1.0f) ? kMidPitch : static_cast<uint16_t>((0x3fff >> 1) * variation);

    ChangePitch(voice, pitchBend);
}
//---------------------------------------------------------
void CrolPlayer::load_tempo_events(binistream *f)
{
    uint16_t const num_tempo_events = static_cast<uint16_t>(f->readInt(2));

    mTempoEvents.reserve(num_tempo_events);

    for (uint16_t i=0; i<num_tempo_events; ++i)
    {
        STempoEvent event;

        event.time       = static_cast<int16_t>(f->readInt(2));
        event.multiplier = static_cast<float>(f->readFloat(binio::Single));
        mTempoEvents.push_back(event);
    }
}
//---------------------------------------------------------
bool CrolPlayer::load_voice_data(binistream *f, std::string const &bnk_filename, const CFileProvider &fp)
{
    SBnkHeader bnk_header;
    binistream *bnk_file = fp.open(bnk_filename.c_str());

    if (bnk_file)
    {
        load_bnk_info(bnk_file, bnk_header);

        int const numVoices = mpROLHeader->mode ? kNumMelodicVoices : kNumPercussiveVoices;

        mVoiceData.reserve(numVoices);
        for (int i=0; i<numVoices; ++i)
        {
            CVoiceData voice;

            load_note_events(f, voice);
            load_instrument_events(f, voice, bnk_file, bnk_header);
            load_volume_events(f, voice);
            load_pitch_events(f, voice);

            mVoiceData.push_back(voice);
        }

        fp.close(bnk_file);

        return true;
    }

    return false;
}
//---------------------------------------------------------
void CrolPlayer::load_note_events(binistream *f, CVoiceData & voice)
{
    f->seek(ROL_FILLER_SIZE, binio::Add);

    int16_t const time_of_last_note = static_cast<int16_t>(f->readInt(2));

    if (time_of_last_note != 0)
    {
        TNoteEvents & note_events = voice.note_events;
        int16_t total_duration    = 0;

        do
        {
            SNoteEvent event;

            event.number   = static_cast<int16_t>(f->readInt(2));
            event.duration = static_cast<int16_t>(f->readInt(2));

            note_events.push_back(event);

            total_duration += event.duration;
        } while (total_duration < time_of_last_note && !f->error());

        if (time_of_last_note > mTimeOfLastNote)
        {
            mTimeOfLastNote = time_of_last_note;
        }
    }

    f->seek(ROL_FILLER_SIZE, binio::Add);
}
//---------------------------------------------------------
void CrolPlayer::load_instrument_events(binistream *f, CVoiceData & voice,
                                        binistream *bnk_file, SBnkHeader const & bnk_header)
{
    uint16_t const number_of_instrument_events = static_cast<uint16_t>(f->readInt(2));

    TInstrumentEvents & instrument_events = voice.instrument_events;

    instrument_events.reserve(number_of_instrument_events);

    for (uint16_t i = 0; i < number_of_instrument_events; ++i)
    {
        SInstrumentEvent event;
        event.time = static_cast<int16_t>(f->readInt(2));
        f->readString(event.name, INS_MAX_NAME_SIZE);
        event.name[INS_MAX_NAME_SIZE - 1] = 0;

        std::string event_name = event.name;
        if (std::find(usedInstruments.begin(), usedInstruments.end(), event_name) == usedInstruments.end())
            usedInstruments.push_back(event_name);
        event.ins_index = load_bnk_instrument(bnk_file, bnk_header, event_name);

        instrument_events.push_back(event);

        f->seek(ROL_INSTRUMENT_EVENT_FILLER_SIZE, binio::Add);
    }

    f->seek(ROL_FILLER_SIZE, binio::Add);
}
//---------------------------------------------------------
void CrolPlayer::load_volume_events(binistream *f, CVoiceData & voice)
{
    uint16_t const number_of_volume_events = static_cast<uint16_t>(f->readInt(2));

    TVolumeEvents & volume_events = voice.volume_events;

    volume_events.reserve(number_of_volume_events);

    for (uint16_t i=0; i<number_of_volume_events; ++i)
    {
        SVolumeEvent event;
        event.time       = static_cast<int16_t>(f->readInt(2));
        event.multiplier = static_cast<float>(f->readFloat(binio::Single));

        volume_events.push_back(event);
    }

    f->seek(ROL_FILLER_SIZE, binio::Add);
}
//---------------------------------------------------------
void CrolPlayer::load_pitch_events(binistream *f, CVoiceData & voice)
{
    uint16_t const number_of_pitch_events = static_cast<uint16_t>(f->readInt(2));

    TPitchEvents & pitch_events = voice.pitch_events;

    pitch_events.reserve(number_of_pitch_events);

    for (uint16_t i=0; i<number_of_pitch_events; ++i)
    {
        SPitchEvent event;
        event.time      = static_cast<int16_t>(f->readInt(2));
        event.variation = static_cast<float>(f->readFloat(binio::Single));

        pitch_events.push_back(event);
    }
}
