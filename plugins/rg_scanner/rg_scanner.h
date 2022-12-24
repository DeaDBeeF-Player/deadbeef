/*
 * ReplayGain Scanner plugin for DeaDBeeF Player
 *
 * Copyright (c) 2016 Oleksiy Yakovenko
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef __RG_SCANNER_H
#define __RG_SCANNER_H

#include "../../deadbeef.h"

enum {
    DDB_RG_SCAN_MODE_TRACK = 1,
    DDB_RG_SCAN_MODE_SINGLE_ALBUM = 2,
    DDB_RG_SCAN_MODE_ALBUMS_FROM_TAGS = 3,
};

enum {
    DDB_RG_SCAN_RESULT_SUCCESS = 0,
    DDB_RG_SCAN_RESULT_FILE_NOT_FOUND = -1,
    DDB_RG_SCAN_RESULT_INVALID_FILE = -2,
};

#define DDB_RG_SCAN_DEFAULT_LOUDNESS 89.f

typedef struct {
    float track_gain;
    float album_gain;
    float track_peak;
    float album_peak;
    int scan_result;
} ddb_rg_scanner_result_t;

typedef struct {
    // Size of this structure
    int _size;

    // The scanning mode, one of DDB_RG_SCAN_MODE_*
    int mode;

    // The list of tracks and results.
    // The caller is responsible to allocate and free these buffers.
    DB_playItem_t **tracks;
    ddb_rg_scanner_result_t *results;

    int num_tracks;

    // Requested reference loudness, will be automatically set to DDB_RG_SCAN_DEFAULT_LOUDNESS
    // Preferred config variable: rg_scanner.target_db=89
    float ref_loudness;

    // Max number of concurrent threads
    int num_threads;

    // Optional pointer to the abort flag; the scanner will abort if the pointed value is non-zero
    int *pabort;

    // Optional progress callback, with the current track index.
    void (*progress_callback) (int current_track, void *user_data);

    // An additional user-defined parameter, which will be passed to the progress_callback.
    void *progress_cb_user_data;

    // How many 44.1kHz samples of PCM data have been processed.
    // Set by the scanner, can be used in the progress callback, to calculate scanning speed.
    uint64_t cd_samples_processed;
} ddb_rg_scanner_settings_t;

typedef struct {
    DB_misc_t misc;

    int (*scan) (ddb_rg_scanner_settings_t *settings);

    // flags specify which fields must be set / added
    // each bit is 1 shifted left by DDB_REPLAYGAIN_* constant
    // Example (1<<DDB_REPLAYGAIN_ALBUMGAIN)|(1<<DDB_REPLAYGAIN_ALBUMPEAK) would set album values, but not track values
    int (*apply) (DB_playItem_t *track, uint32_t flags, float track_gain, float track_peak, float album_gain, float album_peak);

    int (*remove) (DB_playItem_t *track);
} ddb_rg_scanner_t;

#endif //__RG_SCANNER_H
