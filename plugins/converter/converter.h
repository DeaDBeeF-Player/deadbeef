/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>
    Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>

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

#include "../../deadbeef.h"

enum {
    DDB_ENCODER_METHOD_PIPE = 0,
    DDB_ENCODER_METHOD_FILE = 1
};

enum {
    DDB_PRESET_CUSTOM = 0,
    DDB_PRESET_BUILTIN,
    DDB_PRESET_MODIFIED
};

enum ddb_convert_api {
    DDB_CONVERT_API_CONTINUE = 0,
    DDB_CONVERT_API_ABORT
};

typedef struct {
    char *title;
    char *extension;
    char *encoder;
    int method;
    int tag_id3v2;
    int tag_id3v1;
    int tag_apev2;
    int tag_flac;
    int tag_oggvorbis;
    int tag_mp3xing;
    int id3v2_version;
    int builtin;
} ddb_encoder_preset_t;

typedef struct ddb_dsp_preset_s {
    char *title;
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
    (*encoder_preset_duplicate) (ddb_encoder_preset_t *old);

    ddb_encoder_preset_t *
    (*encoder_preset_get) (const char *title);

    ddb_encoder_preset_t *
    (*encoder_preset_get_for_idx) (int idx);

    ddb_encoder_preset_t *
    (*encoder_preset_get_next) (const ddb_encoder_preset_t *);

    int
    (*encoder_preset_get_idx) (const char *title);

    ddb_encoder_preset_t *
    (*encoder_preset_load_builtin) (const char *title);

    ddb_encoder_preset_t *
    (*encoder_preset_save) (ddb_encoder_preset_t *p);

    void
    (*encoder_preset_remove) (ddb_encoder_preset_t *p);

    /////////////////////////////
    // dsp preset management
    /////////////////////////////

    ddb_dsp_context_t *
    (*dsp_plugin_duplicate) (ddb_dsp_context_t *old);

    ddb_dsp_preset_t *
    (*dsp_preset_alloc) (void);

    void
    (*dsp_preset_free) (ddb_dsp_preset_t *p);

    ddb_dsp_preset_t *
    (*dsp_preset_duplicate) (ddb_dsp_preset_t *old);

    ddb_dsp_preset_t *
    (*dsp_preset_get) (const char *title);

    ddb_dsp_preset_t *
    (*dsp_preset_get_for_idx) (int idx);

    ddb_dsp_preset_t *
    (*dsp_preset_get_next) (const ddb_dsp_preset_t *);

    int
    (*dsp_preset_get_idx) (const char *title);

    ddb_dsp_preset_t *
    (*dsp_preset_save) (ddb_dsp_preset_t *p);

    void
    (*dsp_preset_remove) (ddb_dsp_preset_t *p);

    /////////////////////////////
    // converter
    /////////////////////////////

    int // 0 = success, -1 = error, 1 = aborted
    (*convert) (DB_playItem_t *it, // track to be converted
                const ddb_encoder_preset_t *encoder_preset, // preset defining how to encode the converted file
                const char *outpath, // final path to write the file (should normally be obtained using get_output_path)
                const ddb_dsp_preset_t *dsp_preset, // preset defining DSP operations to be performed, may be null or empty
                const int output_bps, // stream should be pre-converted to this resolution, -1 means no pre-convert
                const int output_is_float, // stream should be pre-converted to float (output_bps should always be 32)
                enum ddb_convert_api *api, // will be checked regularly during conversion
                char **message, // message reported back after an error
                void (* convert_callback)(const time_t, const time_t, const float, void *), // callback to update progress
                void *user_data); // opaque pointer for the callback

    void
    (*get_output_path) (DB_playItem_t *it, // the track playitem
                        const ddb_encoder_preset_t *encoder_preset, // preset defining the extension
                        const char *rootfolder, // common path root of all the tracks being converted (if preserve_folder_structure is set)
                        const char *outfolder, // the root folder to write the converted files to
                        const char *outfile, // pattern defining a file path (excluding extension) to be appended to outfolder
                        const int use_source_folder, // use the folder containing the track instead of outfolder
                        char *out, // buffer for the final path, includes file extension, not escaped
                        int sz); // number of bytes available in the out buffer

    char * // the common root path of all the playitems passed in
    (*get_root_folder) (DB_playItem_t **items);

    void
    (*load) (void);

    void
    (*unload) (void);
} ddb_converter_t;

#endif
