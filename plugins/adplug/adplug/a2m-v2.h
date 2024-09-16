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
 * a2m-v2.cpp - Adlib Tracker II Player by Dmitry Smagin <dmitry.s.smagin@gmail.com>
 *              Originally by Stanislav Baranec <subz3ro.altair@gmail.com>
 *
 * NOTES:
 * This player loads a2m and a2t modules versions 1 - 14.
 * The code is adapted directly from FreePascal sources of the Adlib Tracker II
 *
 * REFERENCES:
 * https://github.com/ijsf/at2
 * http://www.adlibtracker.net/
 *
 */

#ifndef H_ADPLUG_A2MPLAYER
#define H_ADPLUG_A2MPLAYER

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "player.h"
#include "depack.h"
#include "sixdepack.h"
#include "unlzh.h"
#include "unlzss.h"
#include "unlzw.h"

/* https://github.com/wc-duck/dbgtools/blob/master/include/dbgtools/static_assert.h */

#define STATIC_ASSERT(cond, msg)
#undef STATIC_ASSERT

// ... clang ...
#if defined( __clang__ )
    #if defined( __cplusplus ) && __has_feature(cxx_static_assert)
        #define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
    #elif __has_feature(c_static_assert)
        #define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
    #endif

// ... msvc ...
#elif defined(_MSC_VER) && ( defined(_MSC_VER) && (_MSC_VER >= 1600) )
    #define STATIC_ASSERT(cond, msg) static_assert(cond, msg)

// ... gcc ...
#elif defined(__cplusplus)
    #if __cplusplus >= 201103L || (defined(_MSC_VER) && (_MSC_VER >= 1600))
        #define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
    #endif
#elif defined( __STDC__ )
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201112L
            #define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
        #else
            #define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
        #endif
    #endif
#endif

/* if we couldn't detect a builtin static assert, lets define one! */
#ifndef STATIC_ASSERT
    #define STATIC_ASSERT(cond, msg) typedef char __STATIC_ASSERT__[(cond) ? 1 : -1]
#endif


// Macros for extracting little-endian integers from filedata
#define INT16LE(A) (int16_t)((A[0]) | (A[1] << 8))
#define UINT16LE(A) (uint16_t)((A[0]) | (A[1] << 8))
#define INT32LE(A) (int32_t)((A[0]) | (A[1] << 8) | (A[2] << 16) | (A[3] << 24))
#define UINT32LE(A) (uint32_t)((A[0]) | (A[1] << 8) | (A[2] << 16) | (A[3] << 24))

#define keyoff_flag         0x80
#define fixed_note_flag     0x90
#define pattern_loop_flag   0xe0
#define pattern_break_flag  0xf0

typedef enum {
    isPlaying = 0, isPaused, isStopped
} tPLAY_STATUS;

#define BYTE_NULL (uint8_t)(0xFFFFFFFF)

#define MIN_IRQ_FREQ        50
#define MAX_IRQ_FREQ        1000

/*
    When loading A2T/A2M, FreePascal structures (no padding and little-endian) should be emulated,
    because AdlibTracker 2 was saving structures directly from memory into the file.

    That's why:
    1) only chars are used in structs to avoid any padding or alignment (default C/C++ behaviour)
    2) ints and longs are represented as arrays of chars, little-endian order is implied
    3) static_assert is used to make sure structs have the correct size
*/

typedef struct {
    union {
        struct {
            uint8_t multipM: 4, ksrM: 1, sustM: 1, vibrM: 1, tremM : 1;
            uint8_t multipC: 4, ksrC: 1, sustC: 1, vibrC: 1, tremC : 1;
            uint8_t volM: 6, kslM: 2;
            uint8_t volC: 6, kslC: 2;
            uint8_t decM: 4, attckM: 4;
            uint8_t decC: 4, attckC: 4;
            uint8_t relM: 4, sustnM: 4;
            uint8_t relC: 4, sustnC: 4;
            uint8_t wformM: 3, : 5;
            uint8_t wformC: 3, : 5;
            uint8_t connect: 1, feedb: 3, : 4; // panning is not used here
        };
        uint8_t data[11];
    };
} tFM_INST_DATA;

STATIC_ASSERT(sizeof(tFM_INST_DATA) == 11, "sizeof(tFM_INST_DATA) != 11");

typedef struct {
    tFM_INST_DATA fm;
    uint8_t panning;
    int8_t  fine_tune;
    uint8_t perc_voice;
} tINSTR_DATA;

STATIC_ASSERT(sizeof(tINSTR_DATA) == 14, "sizeof(tINSTR_DATA) != 14");

typedef struct {
    uint8_t length;
    uint8_t speed;
    uint8_t loop_begin;
    uint8_t loop_length;
    uint8_t keyoff_pos;
    uint8_t data[255];
} tARPEGGIO_TABLE;

typedef struct {
    uint8_t length;
    uint8_t speed;
    uint8_t delay;
    uint8_t loop_begin;
    uint8_t loop_length;
    uint8_t keyoff_pos;
    int8_t data[255]; // array[1..255] of Shortint;
} tVIBRATO_TABLE;

typedef struct {
    tFM_INST_DATA fm;
    uint8_t freq_slide[2]; // int16_t
    uint8_t panning;
    uint8_t duration;
} tREGISTER_TABLE_DEF;

typedef struct {
    uint8_t length;
    uint8_t loop_begin;
    uint8_t loop_length;
    uint8_t keyoff_pos;
    uint8_t arpeggio_table;
    uint8_t vibrato_table;
    tREGISTER_TABLE_DEF data[255];
} tFMREG_TABLE;

typedef struct {
    tARPEGGIO_TABLE arpeggio;
    tVIBRATO_TABLE vibrato;
} tARPVIB_TABLE;

STATIC_ASSERT(sizeof(tFMREG_TABLE) == 3831, "sizeof(tFMREG_TABLE) != 3831");
STATIC_ASSERT(sizeof(tARPVIB_TABLE) == 521, "sizeof(tARPVIB_TABLE) != 521");

typedef struct {
    uint8_t note;
    uint8_t instr_def; // TODO: rename to 'ins'
    struct {
        uint8_t def;
        uint8_t val;
    } eff[2];
} tADTRACK2_EVENT;

STATIC_ASSERT(sizeof(tADTRACK2_EVENT) == 6, "sizeof(tADTRACK2_EVENT) != 6");

typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT ev;
        } row[256];
    } ch[20];
} tPATTERN_DATA;

STATIC_ASSERT(sizeof(tPATTERN_DATA) == 20 * 256 * 6, "sizeof(tPATTERN_DATA) != 30720");

#define ef_Arpeggio            0
#define ef_FSlideUp            1
#define ef_FSlideDown          2
#define ef_TonePortamento      3
#define ef_Vibrato             4
#define ef_TPortamVolSlide     5
#define ef_VibratoVolSlide     6
#define ef_FSlideUpFine        7
#define ef_FSlideDownFine      8
#define ef_SetModulatorVol     9
#define ef_VolSlide            10
#define ef_PositionJump        11
#define ef_SetInsVolume        12
#define ef_PatternBreak        13
#define ef_SetTempo            14
#define ef_SetSpeed            15
#define ef_TPortamVSlideFine   16
#define ef_VibratoVSlideFine   17
#define ef_SetCarrierVol       18
#define ef_SetWaveform         19
#define ef_VolSlideFine        20
#define ef_RetrigNote          21
#define ef_Tremolo             22
#define ef_Tremor              23
#define ef_ArpggVSlide         24
#define ef_ArpggVSlideFine     25
#define ef_MultiRetrigNote     26
#define ef_FSlideUpVSlide      27
#define ef_FSlideDownVSlide    28
#define ef_FSlUpFineVSlide     29
#define ef_FSlDownFineVSlide   30
#define ef_FSlUpVSlF           31
#define ef_FSlDownVSlF         32
#define ef_FSlUpFineVSlF       33
#define ef_FSlDownFineVSlF     34
#define ef_Extended            35
#define ef_Extended2           36
#define ef_SetGlobalVolume     37
#define ef_SwapArpeggio        38
#define ef_SwapVibrato         39
#define ef_ForceInsVolume      40
#define ef_Extended3           41
#define ef_ExtraFineArpeggio   42
#define ef_ExtraFineVibrato    43
#define ef_ExtraFineTremolo    44
#define ef_SetCustomSpeedTab   45
#define ef_GlobalFSlideUp      46
#define ef_GlobalFSlideDown    47
#define ef_GlobalFreqSlideUpXF 48 // ef_fix2 replacement for >xx + ZFE
#define ef_GlobalFreqSlideDnXF 49 // ef_fix2 replacement for <xx + ZFE

#define ef_ex_SetTremDepth     0
#define ef_ex_SetVibDepth      1
#define ef_ex_SetAttckRateM    2
#define ef_ex_SetDecayRateM    3
#define ef_ex_SetSustnLevelM   4
#define ef_ex_SetRelRateM      5
#define ef_ex_SetAttckRateC    6
#define ef_ex_SetDecayRateC    7
#define ef_ex_SetSustnLevelC   8
#define ef_ex_SetRelRateC      9
#define ef_ex_SetFeedback      10
#define ef_ex_SetPanningPos    11
#define ef_ex_PatternLoop      12
#define ef_ex_PatternLoopRec   13
#define ef_ex_ExtendedCmd      14
#define ef_ex_cmd_MKOffLoopDi  0
#define ef_ex_cmd_MKOffLoopEn  1
#define ef_ex_cmd_TPortaFKdis  2
#define ef_ex_cmd_TPortaFKenb  3
#define ef_ex_cmd_RestartEnv   4
#define ef_ex_cmd_4opVlockOff  5
#define ef_ex_cmd_4opVlockOn   6
#define ef_ex_cmd_ForceBpmSld  7
#define ef_ex_ExtendedCmd2     15
#define ef_ex_cmd2_RSS         0
#define ef_ex_cmd2_ResetVol    1
#define ef_ex_cmd2_LockVol     2
#define ef_ex_cmd2_UnlockVol   3
#define ef_ex_cmd2_LockVP      4
#define ef_ex_cmd2_UnlockVP    5
#define ef_ex_cmd2_VSlide_mod  6
#define ef_ex_cmd2_VSlide_car  7
#define ef_ex_cmd2_VSlide_def  8
#define ef_ex_cmd2_LockPan     9
#define ef_ex_cmd2_UnlockPan   10
#define ef_ex_cmd2_VibrOff     11
#define ef_ex_cmd2_TremOff     12
#define ef_ex_cmd2_FVib_FGFS   13
#define ef_ex_cmd2_FTrm_XFGFS  14
#define ef_ex_cmd2_NoRestart   15
#define ef_ex2_PatDelayFrame   0
#define ef_ex2_PatDelayRow     1
#define ef_ex2_NoteDelay       2
#define ef_ex2_NoteCut         3
#define ef_ex2_FineTuneUp      4
#define ef_ex2_FineTuneDown    5
#define ef_ex2_GlVolSlideUp    6
#define ef_ex2_GlVolSlideDn    7
#define ef_ex2_GlVolSlideUpF   8
#define ef_ex2_GlVolSlideDnF   9
#define ef_ex2_GlVolSldUpXF    10
#define ef_ex2_GlVolSldDnXF    11
#define ef_ex2_VolSlideUpXF    12
#define ef_ex2_VolSlideDnXF    13
#define ef_ex2_FreqSlideUpXF   14
#define ef_ex2_FreqSlideDnXF   15
#define ef_ex3_SetConnection   0
#define ef_ex3_SetMultipM      1
#define ef_ex3_SetKslM         2
#define ef_ex3_SetTremoloM     3
#define ef_ex3_SetVibratoM     4
#define ef_ex3_SetKsrM         5
#define ef_ex3_SetSustainM     6
#define ef_ex3_SetMultipC      7
#define ef_ex3_SetKslC         8
#define ef_ex3_SetTremoloC     9
#define ef_ex3_SetVibratoC     10
#define ef_ex3_SetKsrC         11
#define ef_ex3_SetSustainC     12

#define EFGR_ARPVOLSLIDE 1
#define EFGR_FSLIDEVOLSLIDE 2
#define EFGR_TONEPORTAMENTO 3
#define EFGR_VIBRATO 4
#define EFGR_TREMOLO 5
#define EFGR_VIBRATOVOLSLIDE 6
#define EFGR_PORTAVOLSLIDE 7
#define EFGR_RETRIGNOTE 8

/* Data for importing A2T format */
typedef struct {
    char id[15];	// '_a2tiny_module_'
    uint8_t crc[4]; // uint32_t
    uint8_t ffver;
    uint8_t npatt;
    uint8_t tempo;
    uint8_t speed;
} A2T_HEADER;

STATIC_ASSERT(sizeof(A2T_HEADER) == 23, "sizeof(A2T_HEADER) != 23");

typedef struct {
    char id[10];	// '_a2module_'
    uint8_t crc[4]; // uint32_t
    uint8_t ffver;
    uint8_t npatt;
} A2M_HEADER;

STATIC_ASSERT(sizeof(A2M_HEADER) == 16, "sizeof(A2M_HEADER) != 16");

typedef struct {
    uint8_t len[6][2]; // uint16_t
} A2T_VARHEADER_V1234;

typedef struct {
    uint8_t common_flag;
    uint8_t len[10][2]; // uint16_t
} A2T_VARHEADER_V5678;

typedef struct {
    uint8_t common_flag;
    uint8_t patt_len[2]; // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2]; // uint16_t
    uint8_t len[20][4]; // uint32_t
} A2T_VARHEADER_V9;

typedef struct {
    uint8_t common_flag;
    uint8_t patt_len[2]; // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2]; // uint16_t
    uint8_t flag_4op;
    uint8_t lock_flags[20];
    uint8_t len[20][4]; // uint32_t
} A2T_VARHEADER_V10;

typedef struct {
    uint8_t common_flag;
    uint8_t patt_len[2]; // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2]; // uint16_t
    uint8_t flag_4op;
    uint8_t lock_flags[20];
    uint8_t len[21][4]; // uint32_t
} A2T_VARHEADER_V11;

typedef union {
    A2T_VARHEADER_V1234 v1234;
    A2T_VARHEADER_V5678 v5678;
    A2T_VARHEADER_V9    v9;
    A2T_VARHEADER_V10   v10;
    A2T_VARHEADER_V11   v11;
} A2T_VARHEADER;

STATIC_ASSERT(sizeof(A2T_VARHEADER_V1234) == 12, "sizeof(A2T_VARHEADER_V1234) != 12");
STATIC_ASSERT(sizeof(A2T_VARHEADER_V5678) == 21, "sizeof(A2T_VARHEADER_V5678) != 21");
STATIC_ASSERT(sizeof(A2T_VARHEADER_V9) == 86, "sizeof(A2T_VARHEADER_V9) != 86");
STATIC_ASSERT(sizeof(A2T_VARHEADER_V10) == 107, "sizeof(A2T_VARHEADER_V10) != 107");
STATIC_ASSERT(sizeof(A2T_VARHEADER_V11) == 111, "sizeof(A2T_VARHEADER_V11) != 111");
STATIC_ASSERT(sizeof(A2T_VARHEADER) == 111, "sizeof(A2T_VARHEADER) != 111");

// only for importing v 1,2,3,4,5,6,7,8
typedef struct {
    uint8_t note;
    uint8_t instr_def;
    uint8_t effect_def;
    uint8_t effect;
} tADTRACK2_EVENT_V1234;

// for importing v 1,2,3,4 patterns
typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT_V1234 ev;
        } ch[9];
    } row[64];
} tPATTERN_DATA_V1234;

// for importing v 5,6,7,8 patterns
typedef struct {
    struct {
        struct {
            tADTRACK2_EVENT_V1234 ev;
        } row[64];
    } ch[18];
} tPATTERN_DATA_V5678;

STATIC_ASSERT(sizeof(tADTRACK2_EVENT_V1234) == 4, "sizeof(tADTRACK2_EVENT_V1234) != 4");
STATIC_ASSERT(sizeof(tPATTERN_DATA_V1234) == 2304, "sizeof(tPATTERN_DATA_V1234) != 2304");
STATIC_ASSERT(sizeof(tPATTERN_DATA_V5678) == 4608, "sizeof(tPATTERN_DATA_V5678) != 4608");

// Old v1234 effects
enum {
    fx_Arpeggio          = 0x00,
    fx_FSlideUp          = 0x01,
    fx_FSlideDown        = 0x02,
    fx_FSlideUpFine      = 0x03,
    fx_FSlideDownFine    = 0x04,
    fx_TonePortamento    = 0x05,
    fx_TPortamVolSlide   = 0x06,
    fx_Vibrato           = 0x07,
    fx_VibratoVolSlide   = 0x08,
    fx_SetOpIntensity    = 0x09,
    fx_SetInsVolume      = 0x0a,
    fx_PatternBreak      = 0x0b,
    fx_PatternJump       = 0x0c,
    fx_SetTempo          = 0x0d,
    fx_SetTimer          = 0x0e,
    fx_Extended          = 0x0f,
    fx_ex_DefAMdepth     = 0x00,
    fx_ex_DefVibDepth    = 0x01,
    fx_ex_DefWaveform    = 0x02,
    fx_ex_ManSlideUp     = 0x03,
    fx_ex_ManSlideDown   = 0x04,
    fx_ex_VSlideUp       = 0x05,
    fx_ex_VSlideDown     = 0x06,
    fx_ex_VSlideUpFine   = 0x07,
    fx_ex_VSlideDownFine = 0x08,
    fx_ex_RetrigNote     = 0x09,
    fx_ex_SetAttckRate   = 0x0a,
    fx_ex_SetDecayRate   = 0x0b,
    fx_ex_SetSustnLevel  = 0x0c,
    fx_ex_SetReleaseRate = 0x0d,
    fx_ex_SetFeedback    = 0x0e,
    fx_ex_ExtendedCmd    = 0x0f
};

/* Data for importing A2M format */
typedef struct {
    tFM_INST_DATA fm;
    uint8_t panning;
    int8_t  fine_tune;
} tINSTR_DATA_V1_8;

STATIC_ASSERT(sizeof(tINSTR_DATA_V1_8) == 13, "sizeof(tINSTR_DATA_V1_8) != 13");

typedef struct {
    char songname[43];
    char composer[43];
    char instr_names[250][33];
    tINSTR_DATA_V1_8 instr_data[250];
    uint8_t pattern_order[128];
    uint8_t tempo;
    uint8_t speed;
    uint8_t common_flag; // A2M_SONGDATA_V5678
} A2M_SONGDATA_V1_8;

STATIC_ASSERT(sizeof(A2M_SONGDATA_V1_8) == 11717, "sizeof(A2M_SONGDATA_V1_8) != 11717");

typedef struct {
    uint8_t num_4op;
    uint8_t idx_4op[128];
} tINS_4OP_FLAGS;

typedef uint8_t tRESERVED[1024];

typedef struct {
    uint8_t rows_per_beat;
    int8_t tempo_finetune[2]; // int16_t
} tBPM_DATA;

typedef struct {
    char songname[43];
    char composer[43];
    char instr_names[255][43];
    tINSTR_DATA instr_data[255];
    tFMREG_TABLE fmreg_table[255];
    tARPVIB_TABLE arpvib_table[255];
    uint8_t pattern_order[128];
    uint8_t tempo;
    uint8_t speed;
    uint8_t common_flag;
    uint8_t patt_len[2];           // uint16_t
    uint8_t nm_tracks;
    uint8_t macro_speedup[2];      // uint16_t
    uint8_t flag_4op;              // A2M_SONGDATA_V10
    uint8_t lock_flags[20];        // A2M_SONGDATA_V10
    char pattern_names[128][43];   // A2M_SONGDATA_V11
    bool dis_fmreg_col[255][28];   // A2M_SONGDATA_V11
    tINS_4OP_FLAGS ins_4op_flags;  // A2M_SONGDATA_V12_13
    tRESERVED reserved_data;       // A2M_SONGDATA_V12_13
    tBPM_DATA bpm_data;            // A2M_SONGDATA_V14
} A2M_SONGDATA_V9_14;

STATIC_ASSERT(sizeof(A2M_SONGDATA_V9_14) == 1138338, "sizeof(A2M_SONGDATA_V9_14) != 1138338");

/* Player data */

typedef struct {
    tINSTR_DATA instr_data;
    uint8_t vibrato;
    uint8_t arpeggio;
    tFMREG_TABLE *fmreg;
    uint32_t dis_fmreg_cols;
} tINSTR_DATA_EXT;

typedef struct {
    char            songname[43];        // pascal String[42];
    char            composer[43];        // pascal String[42];
    char            instr_names[255][43];// array[1..255] of String[42];
    uint8_t         pattern_order[0x80]; // array[0..0x7f] of Byte;
    uint8_t         tempo;
    uint8_t         speed;
    uint8_t         common_flag;
    uint16_t        patt_len;
    uint8_t         nm_tracks;
    uint16_t        macro_speedup;
    uint8_t         flag_4op;
    uint8_t         lock_flags[20];
} tSONGINFO;

typedef struct {
    uint16_t freq;
    uint8_t speed;
} tPORTA_TABLE;

typedef struct {
    uint8_t state, note, add1, add2;
} tARPGG_TABLE;

typedef struct {
    int8_t pos;
    uint8_t volM, volC;
} tTREMOR_TABLE;

typedef struct {
    uint8_t pos, dir, speed, depth;
    bool fine;
} tVIBRTREM_TABLE;

typedef struct {
    uint8_t def, val;
} tEFFECT_TABLE;

typedef struct {
    uint16_t fmreg_pos,
         arpg_pos,
         vib_pos;
    uint8_t
         fmreg_duration,
         arpg_count,
         vib_count,
         vib_delay,
         fmreg_ins, // fmreg_table
         arpg_table,
         vib_table,
         arpg_note;
    bool vib_paused;
    uint16_t vib_freq;
} tCH_MACRO_TABLE;

typedef struct {
    tFM_INST_DATA fmpar_table[20]; // TODO: rename to 'fm'
    bool volume_lock[20];
    bool vol4op_lock[20];
    bool peak_lock[20];
    bool pan_lock[20];
    uint8_t modulator_vol[20];
    uint8_t carrier_vol[20];
    // note/instr_def - memorized across rows
    // effects - change each row
    tADTRACK2_EVENT event_table[20];
    uint8_t voice_table[20];
    uint16_t freq_table[20];
    uint16_t zero_fq_table[20];
    tEFFECT_TABLE effect_table[2][20];
    uint8_t fslide_table[2][20];
    tEFFECT_TABLE glfsld_table[2][20];
    tPORTA_TABLE porta_table[2][20];
    bool portaFK_table[20];
    tARPGG_TABLE arpgg_table[2][20];
    tVIBRTREM_TABLE vibr_table[2][20];
    tVIBRTREM_TABLE trem_table[2][20];
    uint8_t retrig_table[2][20];
    tTREMOR_TABLE tremor_table[2][20];
    uint8_t panning_table[20];
    tEFFECT_TABLE last_effect[2][20];
    uint8_t volslide_type[20];
    uint8_t notedel_table[20];
    uint8_t notecut_table[20];
    int8_t ftune_table[20];
    bool keyoff_loop[20];
    uint8_t loopbck_table[20];
    uint8_t loop_table[20][256];
    bool reset_chan[20];
    tCH_MACRO_TABLE macro_table[20];
} tCHDATA;

typedef struct {
    unsigned int count;
    size_t size;
    tINSTR_DATA_EXT *instruments;
} tINSTR_INFO;

typedef struct {
    int patterns, rows, channels;
    size_t size;
    tADTRACK2_EVENT *events;
} tEVENTS_INFO;

typedef struct _4op_data {
    uint32_t mode: 1, conn: 3, ch1: 4, ch2: 4, ins1: 8, ins2: 8;
} t4OP_DATA;

class Ca2mv2Player : public CPlayer
{
public:
    static CPlayer *factory(Copl *newopl);

    Ca2mv2Player(Copl *newopl);
    ~Ca2mv2Player();

    bool load(const std::string &filename, const CFileProvider &fp);
    bool update();
    void rewind(int subsong);
    float getrefresh();

    std::string gettype();
    std::string gettitle() { return std::string(songinfo->songname); }
    std::string getauthor() { return std::string(songinfo->composer); }
    unsigned int getinstruments() { return instrinfo->count; }
    std::string getinstrument(unsigned int n) { return std::string(n < instrinfo->count ? songinfo->instr_names[n] : ""); }

private:
    uint8_t current_order = 0;
    uint8_t current_pattern = 0;
    uint8_t current_line = 0; // TODO: rename to current_row

    uint8_t tempo = 50;
    uint8_t speed = 6;

    uint16_t macro_speedup = 1;
    bool irq_mode = false;

    int16_t IRQ_freq = 50;
    int IRQ_freq_shift = 0;
    bool irq_initialized = false;
    bool timer_fix = true;

    bool pattern_break = false;
    bool pattern_delay = false;
    uint8_t next_line = 0;

    int playback_speed_shift = 0;
    tPLAY_STATUS play_status = isStopped;
    uint8_t overall_volume = 63;
    uint8_t global_volume = 63;

    const uint8_t def_vibtrem_speed_factor = 1;
    const uint8_t def_vibtrem_table_size = 32;

    uint8_t vibtrem_speed_factor;
    uint8_t vibtrem_table_size;
    uint8_t vibtrem_table[256];

    uint8_t misc_register;

    uint8_t current_tremolo_depth = 0;
    uint8_t current_vibrato_depth = 0;

    bool speed_update, lockvol, panlock, lockVP;
    uint8_t tremolo_depth, vibrato_depth;
    bool volume_scaling, percussion_mode;
    uint8_t last_order;

    bool editor_mode = false; // true to allocate max resources

    tSONGINFO *songinfo;
    tINSTR_INFO *instrinfo;
    unsigned int arpvib_count;
    tVIBRATO_TABLE **vibrato_table = 0;
    tARPEGGIO_TABLE **arpeggio_table = 0;
    tEVENTS_INFO *eventsinfo;
    tCHDATA *ch;

    // Timer
    int ticks, tickD, tickXF;
    int ticklooper, macro_ticklooper;

    // Loader
    int type = 0; // 0 - a2m, 1 - a2t
    int ffver = 1;
    unsigned int len[21];
    bool adsr_carrier[9]; // For importing from a2m v1234

    bool songend = false;
    int chip = 0;

    // Helpers for instruments
    void instruments_free();
    void instruments_allocate(size_t number);
    tINSTR_DATA_EXT *get_instr(uint8_t ins);
    int8_t get_instr_fine_tune(uint8_t ins);
    tINSTR_DATA *get_instr_data_by_ch(int chan);
    tINSTR_DATA *get_instr_data(uint8_t ins);

    // Helpers for macro tables
    void fmreg_table_allocate(size_t n, tFMREG_TABLE rt[]);
    void disabled_fmregs_import(size_t n, bool dis_fmregs[255][28]);
    void arpvib_tables_free();
    void arpvib_tables_allocate(size_t n, tARPVIB_TABLE mt[]);
    tARPEGGIO_TABLE *get_arpeggio_table(uint8_t arp_table);
    tVIBRATO_TABLE *get_vibrato_table(uint8_t vib_table);
    tFMREG_TABLE *get_fmreg_table(uint8_t fmreg_ins);

    // Helpers for patterns
    tADTRACK2_EVENT *get_event_p(int pattern, int channel, int row);
    void patterns_free();
    void patterns_allocate(int patterns, int channels, int rows);

    uint16_t regoffs_n(int chan);
    uint16_t regoffs_m(int chan);
    uint16_t regoffs_c(int chan);
    bool is_4op_chan_hi(int chan);
    bool is_4op_chan_lo(int chan);
    void opl2out(uint16_t reg, uint16_t data);
    void opl3out(uint16_t reg, uint8_t data);
    void opl3exp(uint16_t data);
    void change_freq(int chan, uint16_t freq);
    bool is_chan_adsr_data_empty(int chan);
    bool is_ins_adsr_data_empty(int ins);
    void change_frequency(int chan, uint16_t freq);
    uint16_t _macro_speedup();
    void set_clock_rate(uint8_t clock_rate);
    void update_timer(int Hz);
    void update_playback_speed(int speed_shift);
    void key_on(int chan);
    void key_off(int chan);
    void release_sustaining_sound(int chan);

    t4OP_DATA get_4op_data(uint8_t chan);
    bool _4op_vol_valid_chan(int chan);
    void set_ins_volume(uint8_t modulator, uint8_t carrier, uint8_t chan);
    void set_volume(uint8_t modulator, uint8_t carrier, uint8_t chan);
    void set_ins_volume_4op(uint8_t volume, uint8_t chan);
    void reset_ins_volume(int chan);
    void set_global_volume();
    void set_overall_volume(unsigned char level);

    void init_macro_table(int chan, uint8_t note, uint8_t ins, uint16_t freq);
    void set_ins_data(uint8_t ins, int chan);
    void update_modulator_adsrw(int chan);
    void update_carrier_adsrw(int chan);
    void update_fmpar(int chan);
    bool is_4op_chan(int chan);
    void output_note(uint8_t note, uint8_t ins, int chan, bool restart_macro, bool restart_adsr);
    bool no_loop(uint8_t current_chan, uint8_t current_line);
    void update_effect_table(int slot, int chan, int eff_group, uint8_t def, uint8_t val);
    void process_effects(tADTRACK2_EVENT *event, int slot, int chan);
    void new_process_note(tADTRACK2_EVENT *event, int chan);
    void play_line();
    void generate_custom_vibrato(uint8_t value);
    void check_swap_arp_vibr(tADTRACK2_EVENT *event, int slot, int chan);
    void portamento_up(int chan, uint16_t slide, uint16_t limit);
    void portamento_down(int chan, uint16_t slide, uint16_t limit);
    void macro_vibrato__porta_up(int chan, uint8_t depth);
    void macro_vibrato__porta_down(int chan, uint8_t depth);
    void tone_portamento(int slot, int chan);
    void slide_carrier_volume_up(uint8_t chan, uint8_t slide, uint8_t limit);
    void slide_modulator_volume_up(uint8_t chan, uint8_t slide, uint8_t limit);
    void slide_volume_up(int chan, uint8_t slide);
    void slide_carrier_volume_down(uint8_t chan, uint8_t slide);
    void slide_modulator_volume_down(uint8_t chan, uint8_t slide);
    void slide_volume_down(int chan, uint8_t slide);
    void volume_slide(int chan, uint8_t up_speed, uint8_t down_speed);
    void global_volume_slide(uint8_t up_speed, uint8_t down_speed);
    void arpeggio(int slot, int chan);
    void vibrato(int slot, int chan);
    void tremolo(int slot, int chan);
    int chanvol(int chan);
    void update_effects_slot(int slot, int chan);
    void update_effects();
    void update_fine_effects(int slot, int chan);
    void update_extra_fine_effects_slot(int slot, int chan);
    void update_extra_fine_effects();

    void set_current_order(uint8_t new_order);
    int calc_following_order(uint8_t order);
    int calc_order_jump();
    void update_song_position();
    void poll_proc();
    void macro_poll_proc();
    void newtimer();
    void init_irq();
    void done_irq();

    void init_buffers();
    void init_player();
    bool a2t_play(char *tune, unsigned long size);
    void a2t_stop();
    void init_songdata();

    // Loader
    void a2t_depack(char *src, int srcsize, char *dst, int dstsize);
    int a2t_read_varheader(char *blockptr, unsigned long size);
    void instrument_import_v1_8(int ins, tINSTR_DATA_V1_8 *instr_s);
    void instrument_import(int ins, tINSTR_DATA *instr_s);
    int a2t_read_instruments(char *src, unsigned long size);
    int a2t_read_fmregtable(char *src, unsigned long size);
    int a2t_read_arpvibtable(char *src, unsigned long size);
    int a2t_read_disabled_fmregs(char *src, unsigned long size);
    int a2t_read_order(char *src, unsigned long size);
    void convert_v1234_event(tADTRACK2_EVENT_V1234 *ev, int chan);
    int a2_read_patterns(char *src, int s, unsigned long size);
    int a2t_read_patterns(char *src, unsigned long size);
    bool a2t_import(char *tune, unsigned long size);

    int a2m_read_varheader(char *blockptr, int npatt, unsigned long size);
    int a2m_read_songdata(char *src, unsigned long size);
    int a2m_read_patterns(char *src, unsigned long size);
    bool a2m_import(char *tune, unsigned long size);
    bool a2_import(char *tune, unsigned long size);
};

#endif
