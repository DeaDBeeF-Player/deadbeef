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
 *
 * Source references ADLIB.C from Adlib MSC SDK.
 */
#ifndef H_ROLPLAYER
#define H_ROLPLAYER

#include <vector>
#include <string>

#include "player.h"

// These are here since Visual C 6 doesn't support statics declared and defined in class.
#define ROL_COMMENT_SIZE 40U
#define ROL_UNUSED1_SIZE 1U
#define ROL_UNUSED2_SIZE 90U
#define ROL_FILLER0_SIZE 38U
#define ROL_FILLER1_SIZE 15U
#define ROL_FILLER_SIZE  15U
#define ROL_MAX_NAME_SIZE  9U
#define ROL_INSTRUMENT_EVENT_FILLER_SIZE 3U // 1 for filler, 2 for unused
#define ROL_BNK_SIGNATURE_SIZE 6U

class CrolPlayer: public CPlayer
{
public:
    static CPlayer *factory(Copl * pNewOpl);

    explicit CrolPlayer(Copl * const pNewOpl);

    virtual ~CrolPlayer();

    virtual bool  load      (const std::string &filename, const CFileProvider &fp);
    virtual bool  update    ();
    virtual void  rewind    (int subsong);	// rewinds to specified subsong
    virtual float getrefresh();			// returns needed timer refresh rate

    virtual std::string gettype() { return std::string("AdLib Visual Composer"); }
    virtual unsigned int getinstruments()
    {
        return usedInstruments.size();
    };
    virtual std::string getinstrument(unsigned int n)
    {
        return usedInstruments[n];
    };
    virtual std::string getdesc()
    {
        return std::string(mpROLHeader->comment);
    };

private:

#if !defined(UINT8_MAX)
    typedef signed char    int8_t;
    typedef short          int16_t;
    typedef int            int32_t;
    typedef unsigned char  uint8_t;
    typedef unsigned short uint16_t;
    typedef unsigned int   uint32_t;
#endif

#ifdef __x86_64__
    typedef signed   int      int32;
#else
    typedef signed long int   int32;
#endif

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
        char     name[ROL_MAX_NAME_SIZE];
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

    typedef struct
    {
        uint16_t index;
        uint8_t  record_used;
        char     name[ROL_MAX_NAME_SIZE];
    } SInstrumentName;

    typedef std::vector<SInstrumentName> TInstrumentNames;

    typedef struct
    {
        uint8_t  version_major;
        uint8_t  version_minor;
        char     signature[ROL_BNK_SIGNATURE_SIZE];
        uint16_t number_of_list_entries_used;
        uint16_t total_number_of_list_entries;
        int32    abs_offset_of_name_list;
        int32    abs_offset_of_data;

        TInstrumentNames ins_name_list;
    } SBnkHeader;

    typedef struct
    {
        uint8_t key_scale_level;
        uint8_t freq_multiplier;
        uint8_t feed_back;
        uint8_t attack_rate;
        uint8_t sustain_level;
        uint8_t sustaining_sound;
        uint8_t decay_rate;
        uint8_t release_rate;
        uint8_t output_level;
        uint8_t amplitude_vibrato;
        uint8_t frequency_vibrato;
        uint8_t envelope_scaling;
        uint8_t fm_type;
    } SFMOperator;

    typedef struct
    {
        uint8_t ammulti;
        uint8_t ksltl;
        uint8_t ardr;  
        uint8_t slrr;
        uint8_t fbc;
        uint8_t waveform;
    } SOPL2Op;

    typedef struct
    {
        uint8_t mode;
        uint8_t voice_number;
        SOPL2Op modulator;
        SOPL2Op carrier;
    } SRolInstrument;

    typedef struct
    {
        std::string    name;
        SRolInstrument instrument;
    } SInstrument;

    void load_tempo_events     (binistream *f);
    bool load_voice_data       (binistream *f, std::string const & bnk_filename, CFileProvider const & fp);
    void load_note_events      (binistream *f, CVoiceData & voice);
    void load_instrument_events(binistream *f, CVoiceData & voice,
                                binistream *bnk_file, SBnkHeader const & bnk_header);
    void load_volume_events    (binistream *f, CVoiceData & voice);
    void load_pitch_events     (binistream *f, CVoiceData & voice);

    bool load_bnk_info         (binistream *f, SBnkHeader & header);
    int  load_rol_instrument   (binistream *f, SBnkHeader const & header, std::string const & name);
    void read_rol_instrument   (binistream *f, SRolInstrument & ins);
    void read_fm_operator      (binistream *f, SOPL2Op & opl2_op);
    int  get_ins_index         (std::string const & name) const;

    void UpdateVoice(int const voice, CVoiceData & voiceData);
    void SetNote(int const voice, int const note);
    void SetNoteMelodic(int const voice, int const note);
    void SetNotePercussive(int const voice, int const note);
    void SetFreq   (int const voice, int const note, bool const keyOn=false);
    void ChangePitch(int voice, uint16_t const pitchBend);
    void SetPitch  (int const voice, float const variation);
    void SetVolume (int const voice, uint8_t const volume);
    void SetRefresh(float const multiplier);
    uint8_t GetKSLTL(int const voice) const;
    void send_ins_data_to_chip(int const voice, int const ins_index);
    void send_operator(int const voice, SOPL2Op const & modulator, SOPL2Op const & carrier);

    class StringCompare
    {
    public:
        bool operator()(SInstrumentName const & lhs, SInstrumentName const & rhs) const
        {
            return keyLess(lhs.name, rhs.name);
        }

        bool operator()(SInstrumentName const & lhs, std::string const &rhs) const
        {
            return keyLess(lhs.name, rhs.c_str());
        }

        bool operator()(std::string const & lhs, SInstrumentName const & rhs) const
        {
            return keyLess(lhs.c_str(), rhs.name);
        }
    private:
        bool keyLess(char const * const lhs, char const * const rhs) const
        {
            return stricmp(lhs, rhs) < 0;
        }
    };

    typedef uint16_t const *             TUint16ConstPtr;
    typedef std::vector<STempoEvent>     TTempoEvents;
    typedef std::vector<CVoiceData>      TVoiceData;
    typedef std::vector<SInstrument>     TInstrumentList;
    typedef std::vector<TUint16ConstPtr> TUint16PtrVector;
    typedef std::vector<int16_t>         TInt16Vector;
    typedef std::vector<uint8_t>         TUInt8Vector;
    typedef std::vector<bool>            TBoolVector;
    typedef std::vector<std::string>     TStringVector;

    SRolHeader      * mpROLHeader;
    TUint16ConstPtr   mpOldFNumFreqPtr;
    TTempoEvents      mTempoEvents;
    TVoiceData        mVoiceData;
    TInstrumentList   mInstrumentList;
    TUint16PtrVector  mFNumFreqPtrList;
    TInt16Vector      mHalfToneOffset;
    TUInt8Vector      mVolumeCache;
    TUInt8Vector      mKSLTLCache;
    TUInt8Vector      mNoteCache;
    TUInt8Vector      mKOnOctFNumCache;
    TBoolVector       mKeyOnCache;
    float             mRefresh;
    int32_t           mOldPitchBendLength;
    uint16_t          mPitchRangeStep;
    uint16_t          mNextTempoEvent;
    int16_t           mCurrTick;
    int16_t           mTimeOfLastNote;
    int16_t           mOldHalfToneOffset;
    uint8_t           mAMVibRhythmCache;
    TStringVector     usedInstruments;

    static int   const kSizeofDataRecord;
    static int   const kMaxTickBeat;
    static int   const kSilenceNote;
    static int   const kNumMelodicVoices;
    static int   const kNumPercussiveVoices;
    static int   const kBassDrumChannel;
    static int   const kSnareDrumChannel;
    static int   const kTomtomChannel;
    static int   const kTomTomNote;
    static int   const kTomTomToSnare;
    static int   const kSnareNote;
    static float const kDefaultUpdateTme;
};

#endif
