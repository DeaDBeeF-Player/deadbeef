/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __CONVERTER_H
#define __CONVERTER_H

#include <stdint.h>
#include "../../deadbeef.h"

enum {
    DDB_ENCODER_METHOD_PIPE = 0,
    DDB_ENCODER_METHOD_FILE = 1,
};

enum {
    DDB_ENCODER_FMT_8BIT = 0x1,
    DDB_ENCODER_FMT_16BIT = 0x2,
    DDB_ENCODER_FMT_24BIT = 0x4,
    DDB_ENCODER_FMT_32BIT = 0x8,
    DDB_ENCODER_FMT_32BITFLOAT = 0x10,
};

typedef struct ddb_preset_s {
    char *title;
    struct ddb_preset_s *next;
} ddb_preset_t;

typedef struct ddb_encoder_preset_s {
    char *title;
    struct ddb_encoder_preset_s *next;
    char *ext;
    char *encoder;
    int method; // pipe or file
    int tag_id3v2;
    int tag_id3v1;
    int tag_apev2;
    int tag_flac;
    int tag_oggvorbis;
    int tag_mp3xing;
    int id3v2_version;
} ddb_encoder_preset_t;

typedef struct ddb_dsp_preset_s {
    char *title;
    struct ddb_dsp_preset_s *next;
    ddb_dsp_context_t *chain;
} ddb_dsp_preset_t;

typedef struct {
    DB_misc_t misc;

    /////////////////////////////
    // encoder preset management
    /////////////////////////////

    ddb_encoder_preset_t *
    (*encoder_preset_alloc) (void);

    void
    (*encoder_preset_free) (ddb_encoder_preset_t *p);

    ddb_encoder_preset_t *
    (*encoder_preset_load) (const char *fname);

    // @return -1 on path/write error, -2 if file already exists
    int
    (*encoder_preset_save) (ddb_encoder_preset_t *p, int overwrite);

    void
    (*encoder_preset_copy) (ddb_encoder_preset_t *to, ddb_encoder_preset_t *from);

    ddb_encoder_preset_t *
    (*encoder_preset_get_list) (void);

    ddb_encoder_preset_t *
    (*encoder_preset_get_for_idx) (int idx);

    void
    (*encoder_preset_append) (ddb_encoder_preset_t *p);

    void
    (*encoder_preset_remove) (ddb_encoder_preset_t *p);

    void
    (*encoder_preset_replace) (ddb_encoder_preset_t *from, ddb_encoder_preset_t *to);

    /////////////////////////////
    // dsp preset management
    /////////////////////////////

    ddb_dsp_preset_t *
    (*dsp_preset_alloc) (void);

    void
    (*dsp_preset_free) (ddb_dsp_preset_t *p);

    ddb_dsp_preset_t *
    (*dsp_preset_load) (const char *fname);

    // @return -1 on path/write error, -2 if file already exists
    int
    (*dsp_preset_save) (ddb_dsp_preset_t *p, int overwrite);

    void
    (*dsp_preset_copy) (ddb_dsp_preset_t *to, ddb_dsp_preset_t *from);

    ddb_dsp_preset_t *
    (*dsp_preset_get_list) (void);

    ddb_dsp_preset_t *
    (*dsp_preset_get_for_idx) (int idx);

    void
    (*dsp_preset_append) (ddb_dsp_preset_t *p);

    void
    (*dsp_preset_remove) (ddb_dsp_preset_t *p);

    void
    (*dsp_preset_replace) (ddb_dsp_preset_t *from, ddb_dsp_preset_t *to);

    /////////////////////////////
    // converter
    /////////////////////////////


    // this function is deprecated, please don't use directly
    void
    (*get_output_path_1_0) (DB_playItem_t *it, const char *outfolder, const char *outfile, ddb_encoder_preset_t *encoder_preset, char *out, int sz);

    // this function is deprecated, please don't use directly
    int
    (*convert_1_0) (DB_playItem_t *it, const char *outfolder, const char *outfile, int output_bps, int output_is_float, int preserve_folder_structure, const char *root_folder, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort);

    /////////////////////////////
    // new APIs for converter-1.1
    /////////////////////////////

    int
    (*load_encoder_presets) (void);
    int
    (*load_dsp_presets) (void);
    void
    (*free_encoder_presets) (void);
    void
    (*free_dsp_presets) (void);

    /////////////////////////////
    // new APIs for converter-1.2
    /////////////////////////////
    int
    (*convert) (
            DB_playItem_t *it, // track to be converted
            const char *outpath, // final path to write the file (should normally be obtained using get_output_path)
            int output_bps, // stream should be pre-converted to this resolution
            int output_is_float, // stream should be converted to float
            ddb_encoder_preset_t *encoder_preset, // encoder preset to use
            ddb_dsp_preset_t *dsp_preset, // dsp preset to use
            int *abort // *abort will be checked regularly, conversion will be interrupted if it's non-zero
    );
    void
    (*get_output_path) (DB_playItem_t *it, const char *outfolder, const char *outfile, ddb_encoder_preset_t *encoder_preset, int preserve_folder_structure, const char *root_folder, int write_to_source_folder, char *out, int sz);
} ddb_converter_t;

#endif
