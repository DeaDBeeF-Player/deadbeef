/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * rol.h - ROL Player by OPLx <oplx@yahoo.com>
 */
#ifndef H_ROLPLAYER
#define H_ROLPLAYER

#include <vector>
#include <string>

#include "composer.h"

// These are here since Visual C 6 doesn't support statics declared and defined in class.
#define ROL_COMMENT_SIZE 40U
#define ROL_UNUSED1_SIZE 1U
#define ROL_UNUSED2_SIZE 90U
#define ROL_FILLER0_SIZE 38U
#define ROL_FILLER1_SIZE 15U
#define ROL_FILLER_SIZE  15U
#define ROL_INSTRUMENT_EVENT_FILLER_SIZE 3U // 1 for filler, 2 for unused

class CrolPlayer: public CcomposerBackend
{
public:
    static CPlayer *factory(Copl * pNewOpl);

    explicit CrolPlayer(Copl * const pNewOpl);

    ~CrolPlayer();

    bool  load      (const std::string &filename, const CFileProvider &fp);
    bool  update    ();
    void  frontend_rewind(int subsong);	// rewinds to specified subsong
    float getrefresh();			// returns needed timer refresh rate

    unsigned int getinstruments()
    {
        return usedInstruments.size();
    };
    std::string getinstrument(unsigned int n)
    {
        return n < usedInstruments.size() ? usedInstruments[n] : std::string();
    };
    std::string getdesc()
    {
        return strcmp(mpROLHeader->comment, "\\roll\\default") ? std::string(mpROLHeader->comment) : std::string();
    };

private:

    typedef struct
    {
        uint16_t version_major;
        uint16_t version_minor;
        char     comment[ROL_COMMENT_SIZE];
        uint16_t ticks_per_beat;
        uint16_t beats_per_measure;
        uint16_t edit_scale_y;
        uint16_t edit_scale_x;
        char     unused1;
        char     mode;
        char     unused2[ROL_UNUSED2_SIZE];
        char     filler0[ROL_FILLER0_SIZE];
        char     filler1[ROL_FILLER1_SIZE];
        float    basic_tempo;
    } SRolHeader;

    typedef struct
    {
        int16_t time;
        float   multiplier;
    } STempoEvent;

    typedef struct
    {
        int16_t number;
        int16_t duration;
    } SNoteEvent;

    typedef struct
    {
        int16_t time;
        char     name[INS_MAX_NAME_SIZE];
        int16_t ins_index;
    } SInstrumentEvent;

    typedef struct
    {
        int16_t  time;
        float    multiplier;
    } SVolumeEvent;

    typedef struct
    {
        int16_t  time;
        float    variation;
    } SPitchEvent;

    typedef std::vector<SNoteEvent>       TNoteEvents;
    typedef std::vector<SInstrumentEvent> TInstrumentEvents;
    typedef std::vector<SVolumeEvent>     TVolumeEvents;
    typedef std::vector<SPitchEvent>      TPitchEvents;

#define BIT_POS(pos) (1<<pos)

    class CVoiceData
    {
    public:
        enum EEventStatus
        {
            kES_NoteEnd   = BIT_POS(0),
            kES_PitchEnd  = BIT_POS(1),
            kES_InstrEnd  = BIT_POS(2),
            kES_VolumeEnd = BIT_POS(3),

            kES_None      = 0
        };

        explicit CVoiceData()
            :mEventStatus         (kES_None)
            ,mNoteDuration        (0)
            ,current_note_duration(0)
            ,current_note         (0)
            ,next_instrument_event(0)
            ,next_volume_event    (0)
            ,next_pitch_event     (0)
            ,mForceNote           (true)
        {
        }

        void Reset()
        {
            mEventStatus          = kES_None;
            mNoteDuration         = 0;
            current_note_duration = 0;
            current_note          = 0;
            next_instrument_event = 0;
            next_volume_event     = 0;
            next_pitch_event      = 0;
            mForceNote            = true;
        }

        TNoteEvents       note_events;
        TInstrumentEvents instrument_events;
        TVolumeEvents     volume_events;
        TPitchEvents      pitch_events;
        int               mEventStatus;
        int16_t           mNoteDuration;
        int16_t           current_note_duration;
        uint16_t          current_note;
        uint16_t          next_instrument_event;
        uint16_t          next_volume_event;
        uint16_t          next_pitch_event;
        bool              mForceNote;
    };

    void load_tempo_events     (binistream *f);
    bool load_voice_data       (binistream *f, std::string const & bnk_filename, CFileProvider const & fp);
    void load_note_events      (binistream *f, CVoiceData & voice);
    void load_instrument_events(binistream *f, CVoiceData & voice,
                                binistream *bnk_file, SBnkHeader const & bnk_header);
    void load_volume_events    (binistream *f, CVoiceData & voice);
    void load_pitch_events     (binistream *f, CVoiceData & voice);

    void UpdateVoice(int const voice, CVoiceData & voiceData);
    void SetPitch  (int const voice, float const variation);
    void SetRefresh(float const multiplier);

    typedef std::vector<STempoEvent>     TTempoEvents;
    typedef std::vector<CVoiceData>      TVoiceData;
    typedef std::vector<std::string>     TStringVector;

    SRolHeader      * mpROLHeader;
    TTempoEvents      mTempoEvents;
    TVoiceData        mVoiceData;
    float             mRefresh;
    uint16_t          mNextTempoEvent;
    int16_t           mCurrTick;
    int16_t           mTimeOfLastNote;
    TStringVector     usedInstruments;

    static int   const kMaxTickBeat;
    static float const kDefaultUpdateTme;
};

#endif
