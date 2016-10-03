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

typedef struct {
    int _size;

    int mode; // one of DDB_RG_SCAN_MODE_*

    DB_playItem_t **tracks;
    int num_tracks;
    float *out_track_gain;
    float *out_track_peak;
    float *out_album_gain;
    float *out_album_peak;

    float targetdb;                // requested reference gain
    int num_threads;               // max number of concurrent threads
    int *pabort;                   // abort execution if set to 1

    void (*progress_callback) (int current_track, void *user_data);
    void *progress_cb_user_data;
} ddb_rg_scanner_settings_t;

typedef struct {
    DB_misc_t misc;

    int (*scan) (ddb_rg_scanner_settings_t *settings);

    void (*clear_settings) (ddb_rg_scanner_settings_t *settings);

    int (*apply) (DB_playItem_t *track, float track_gain, float album_gain, float track_peak, float album_peak);

    void (*remove) (DB_playItem_t **work_items, int num_tracks);

} ddb_rg_scanner_t;

#endif //__RG_SCANNER_H
