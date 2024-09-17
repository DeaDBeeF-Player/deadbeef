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
 * composer.h - AdLib Visual Composer synth class by OPLx <oplx@yahoo.com>
 *              with improvements by Stas'M <binarymaster@mail.ru> and Jepael
 *
 * Source references ADLIB.C from Adlib MSC SDK.
 */
#ifndef H_VISUALCOMPOSER
#define H_VISUALCOMPOSER

#include <vector>
#include <string>

#include "player.h"

// These are here since Visual C 6 doesn't support statics declared and defined in class.
#define INS_MAX_NAME_SIZE  9U
#define BNK_SIGNATURE_SIZE 6U
#define MAX_VOICES         11
#define ADLIB_OPER_LEN     13		/* operator length, sizeof(SFMOperator) */
#define ADLIB_INST_LEN     (ADLIB_OPER_LEN * 2 + 2)	/* modulator, carrier, mod/car wave select */

#include <stdint.h> // for uintxx_t

#ifdef __x86_64__
    typedef signed   int      int32;
#else
    typedef signed long int   int32;
#endif

class CcomposerBackend: public CPlayer
{
public:
    static CPlayer *factory(Copl * pNewOpl);

    CcomposerBackend(Copl * const pNewOpl);

    ~CcomposerBackend()
    {
    };

    virtual bool load(const std::string &filename, const CFileProvider &fp)
    {
        return false;
    };
    virtual bool update()
    {
        return false;
    };
    virtual void frontend_rewind(int subsong) = 0;
    virtual void rewind(int subsong);	// rewinds to specified subsong
    virtual float getrefresh()			// returns needed timer refresh rate
    {
        return 1.0f;
    };

    virtual std::string gettype() { return std::string("AdLib Visual Composer"); }
    virtual unsigned int getinstruments() { return 0; };
    virtual std::string getinstrument(unsigned int n) { return std::string(); };
    virtual std::string getdesc() { return std::string(); };

    typedef struct
    {
        uint16_t index;
        uint8_t  record_used;
        char     name[INS_MAX_NAME_SIZE];
    } SInstrumentName;

    typedef std::vector<SInstrumentName> TInstrumentNames;

    typedef struct
    {
        uint8_t  version_major;
        uint8_t  version_minor;
        char     signature[BNK_SIGNATURE_SIZE];
        uint16_t number_of_list_entries_used;
        uint16_t total_number_of_list_entries;
        int32    abs_offset_of_name_list;
        int32    abs_offset_of_data;
        bool     case_sensitive;

        TInstrumentNames ins_name_list;
    } SBnkHeader;

    static int   const kNumMelodicVoices;
    static int   const kNumPercussiveVoices;
    static uint32_t const kMidPitch;
    static uint8_t  const kMaxVolume;

    bool load_bnk_info         (binistream *f, SBnkHeader & header);
    int  load_bnk_instrument   (binistream *f, SBnkHeader const & header, std::string const & name);
    int  load_instrument_data  (uint8_t *data, size_t size);
    bool bnk_return_failure = false;	// do not add empty instruments in failure case (default: false)

    void NoteOn(int const voice, int const note);
    void NoteOff(int const voice);
    void SetRhythmMode(int const mode);
    void SetPitchRange(uint8_t pitchRange);
    void ChangePitch(int voice, uint16_t const pitchBend);
    void SetVolume(int const voice, uint8_t const volume);
    void SetInstrument(int const voice, int const ins_index);
    void SetDefaultInstrument(int const voice);

private:

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
    } SInstrumentData;

    typedef struct
    {
        std::string    name;
        SInstrumentData instrument;
    } SInstrument;

    void read_bnk_instrument   (binistream *f, SInstrumentData & ins, bool raw);
    void read_fm_operator      (binistream *f, SOPL2Op & opl2_op);
    int  get_ins_index         (std::string const & name) const;

    void SetNote(int const voice, int const note);
    void SetNoteMelodic(int const voice, int const note);
    void SetNotePercussive(int const voice, int const note);
    void SetFreq(int const voice, int const note, bool const keyOn=false);
    uint8_t GetKSLTL(int const voice) const;
    void send_operator(int const voice, SOPL2Op const & modulator, SOPL2Op const & carrier);

    class StringCompare
    {
    public:
        StringCompare(bool case_sensitive)
        {
            sens = case_sensitive;
        }

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
        bool sens;

        bool keyLess(char const * const lhs, char const * const rhs) const
        {
            if (sens)
                return strcmp(lhs, rhs) < 0;
            return stricmp(lhs, rhs) < 0;
        }
    };

    typedef uint16_t const *             TUint16ConstPtr;
    typedef std::vector<SInstrument>     TInstrumentList;
    typedef std::vector<TUint16ConstPtr> TUint16PtrVector;
    typedef std::vector<int16_t>         TInt16Vector;
    typedef std::vector<uint8_t>         TUInt8Vector;
    typedef std::vector<bool>            TBoolVector;

    TUint16ConstPtr   mpOldFNumFreqPtr;
    TInstrumentList   mInstrumentList;
    TUint16PtrVector  mFNumFreqPtrList;
    TInt16Vector      mHalfToneOffset;
    TUInt8Vector      mVolumeCache;
    TUInt8Vector      mKSLTLCache;
    TUInt8Vector      mNoteCache;
    TUInt8Vector      mKOnOctFNumCache;
    TBoolVector       mKeyOnCache;
    uint8_t           mRhythmMode;
    int32_t           mOldPitchBendLength;
    uint16_t          mPitchRangeStep;
    int16_t           mOldHalfToneOffset;
    uint8_t           mAMVibRhythmCache;

    static int   const kSizeofDataRecord;
    static int   const kSilenceNote;
    static int   const kBassDrumChannel;
    static int   const kSnareDrumChannel;
    static int   const kTomtomChannel;
    static int   const kTomTomNote;
    static int   const kTomTomToSnare;
    static int   const kSnareNote;
};

#endif
