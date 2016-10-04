/*
 * ReplayGain Scanner plugin for DeaDBeeF Player
 *
 * Copyright (c) 2015 Ivan Pilipenko
 * Copyright (c) 2016 Alexey Yakovenko
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
    DDG_RG_SCAN_RESULT_SUCCESS = 0,
    DDG_RG_SCAN_RESULT_FILE_NOT_FOUND = -1,
};

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

    // Requested reference gain.
    // FIXME: it is questionable if this is necessary
    float targetdb;

    // Max number of concurrent threads
    int num_threads;

    // Optional pointer to the abort flag; the scanner will abort if the pointed value is non-zero
    int *pabort;

    // Optional progress callback, with the current track index.
    void (*progress_callback) (int current_track, void *user_data);

    // An additional user-defined parameter, which will be passed to the progress_callback.
    void *progress_cb_user_data;
} ddb_rg_scanner_settings_t;

typedef struct {
    DB_misc_t misc;

    int (*scan) (ddb_rg_scanner_settings_t *settings);

    int (*apply) (DB_playItem_t *track, float track_gain, float album_gain, float track_peak, float album_peak);

    void (*remove) (DB_playItem_t **work_items, int num_tracks);
} ddb_rg_scanner_t;

#endif //__RG_SCANNER_H
