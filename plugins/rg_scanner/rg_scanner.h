/*
 * rg_scanner.h - libEBUR128-based ReplayGain scanner plugin
 *                      for the DeaDBeeF audio player
 *
 * Copyright (c) 2015 Ivan Pilipenko
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

typedef struct {
    int _size;
    DB_playItem_t **scan_items;    // tracks to scan
    int num_tracks;                // how many tracks
    float *out_track_rg;           // individual track ReplayGain
    float *out_track_pk;           // indivirual track peak
    float out_album_rg;           // album track ReplayGain
    float out_album_pk;           // album peak
    float targetdb;                // our target loudness
    int num_threads;               // number of threads
    int *pabort;                   // abort execution if set to 1
} ddb_rg_scanner_settings_t;

typedef struct {
    DB_misc_t misc;

    int (*scan) (ddb_rg_scanner_settings_t *settings);

    int (*apply) (DB_playItem_t *track, float track_gain, float album_gain, float track_peak, float album_peak);

    void (*remove) (DB_playItem_t **work_items, int num_tracks);
} ddb_rg_scanner_t;

#endif //__RG_SCANNER_H
