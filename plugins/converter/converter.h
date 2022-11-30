/*
    Converter for DeaDBeeF Player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#ifndef __CONVERTER_H
#define __CONVERTER_H

#include <stdint.h>

// changes in 1.5:
//   added mp4 tagging support
//   added converter option to copy files without conversion, if file format isn't changing
//   added encoder preset option to pass source file name for %i without any processing
//   added `converter2` function
// changes in 1.4:
//   changed escaping rules:
//   now get_output_path returns unescaped path, and doesn't create folders
//   added get_output_path2
// changes in 1.3:
//   readonly preset support

enum {
    DDB_ENCODER_METHOD_PIPE = 0,
    DDB_ENCODER_METHOD_FILE = 1,
    DDB_ENCODER_METHOD_FILENAME = 2, // added in converter-1.5
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
    int method; // DDB_ENCODER_METHOD_*
    int tag_id3v2;
    int tag_id3v1;
    int tag_apev2;
    int tag_flac;
    int tag_oggvorbis;
    int tag_mp3xing;
    int tag_mp4;
    int id3v2_version;

    // added in converter-1.3
    int readonly; // this means the preset cannot be edited
} ddb_encoder_preset_t;

typedef struct ddb_dsp_preset_s {
    char *title;
    struct ddb_dsp_preset_s *next;
    ddb_dsp_context_t *chain;
} ddb_dsp_preset_t;

typedef struct ddb_converter_settings_s {
    // stream should be pre-converted to this resolution
    int output_bps;

    // stream should be converted to float
    int output_is_float;

    // encoder preset to use
    ddb_encoder_preset_t *encoder_preset;

    // dsp preset to use
    ddb_dsp_preset_t *dsp_preset;

    // bypass conversion, and copy the file as-is, if the output file extension is the same
    int bypass_conversion_on_same_format;

    // rewrite tags after copy, if the bypass_conversion_on_same_format is true
    int rewrite_tags_after_copy;
} ddb_converter_settings_t;

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
    (*convert_1_0) (DB_playItem_t *it, const char *outfolder, const char *outfile, int output_bps, int output_is_float, int preserve_folder_structure, const char *root_folder, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *pabort);

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

    // `convert` is deprecated in converter-1.5, use `convert2` instead
    int
    (*convert) (
        // track to be converted
        DB_playItem_t *it,
        // final path to write the file (should normally be obtained using get_output_path)
        const char *outpath,
        // stream should be pre-converted to this resolution
        int output_bps,
        // stream should be converted to float
        int output_is_float,
        // encoder preset to use
        ddb_encoder_preset_t *encoder_preset,
        // dsp preset to use
        ddb_dsp_preset_t *dsp_preset,
        // *pabort will be checked regularly, conversion will be interrupted if it's non-zero
        int *pabort
    );

    // The 'get_output_path' function should be used to get the fully
    // qualified output file path, to be passed to 'convert' function.
    // It is commonly used by converter GUI.
    // Parameters:
    //  it: the track
    //  outfolder: the folder to write the file to (usually specified in GUI)
    //  outfile: the filename pattern, which may include additional folder
    //           structure, and title formatting; without extension.
    //           examples: "%artist% - %title%", "subfolder/%title%"
    //  encoder_preset: an existing encoder preset.
    //  preserve_folder_structure: set to 1 to recreate the existing folder
    //                             structure, when converting multiple files
    //  root_folder: common root path of all the tracks being converted in 1 go.
    //  write_to_source_folder: set to 1 to write output to the same folders
    //                          where input files are located.
    //  out: the buffer for the output file path,
    //       which will come out not escaped, will include the file extension.
    //  sz: size of the out buffer.
    void
    (*get_output_path) (DB_playItem_t *it, const char *outfolder, const char *outfile, ddb_encoder_preset_t *encoder_preset, int preserve_folder_structure, const char *root_folder, int write_to_source_folder, char *out, int sz);

    // since 1.4
    // same as get_output_path, but takes playlist as argument, and used the new title formatting
    // plt: the playlist which contains the track 'it'
    void
    (*get_output_path2) (DB_playItem_t *it, ddb_playlist_t *plt, const char *outfolder, const char *outfile, ddb_encoder_preset_t *encoder_preset, int preserve_folder_structure, const char *root_folder, int write_to_source_folder, char *out, int sz);

    // since 1.5
    int
    (*convert2) (
         // converter settings
         ddb_converter_settings_t *settings,

         // track to convert
         DB_playItem_t *it,

         // fully qualified output path with filename and extension
         const char *outpath,

         // *pabort will be checked regularly, conversion will be interrupted if it's non-zero
         int *pabort
    );

    // for multi-threaded calls since 1.9.3
    int
    (*convert3) (
         // converter settings
         ddb_converter_settings_t *settings,

         // track to convert
         DB_playItem_t *it,

         // fully qualified output path with filename and extension
         const char *outpath,

         // read lock for accessing pabort
         pthread_rwlock_t *abort_lock,

         // *pabort will be checked regularly, conversion will be interrupted if it's non-zero
         int *pabort
    );
} ddb_converter_t;

#endif
