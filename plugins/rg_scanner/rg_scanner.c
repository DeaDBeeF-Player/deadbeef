/*
 * ReplayGain Scanner plugin for DeaDBeeF Player
 *
 * Copyright (c) 2016 Oleksiy Yakovenko
 *
 * Based on ddb_misc_replaygain_scan (c) 2015 Ivan Pilipenko
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

#include "rg_scanner.h"

#include <dispatch/dispatch.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <deadbeef/deadbeef.h>
#include "ebur128/ebur128.h"
#include <deadbeef/strdupa.h>

#ifndef DISPATCH_QUEUE_CONCURRENT
#define DISPATCH_QUEUE_CONCURRENT NULL
#endif

#define trace(...) { deadbeef->log_detailed (&plugin.misc.plugin, 0, __VA_ARGS__); }

static const char *album_signature = "$if2(%album artist% - %album%,%filename%)";

static ddb_rg_scanner_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    int track_index;
    ddb_rg_scanner_settings_t *settings;
    ebur128_state **gain_state;
    ebur128_state **peak_state;
    dispatch_queue_t sync_queue;
} track_state_t;

void
rg_calc_track(track_state_t *st) {
    DB_decoder_t *dec = NULL;
    DB_fileinfo_t *fileinfo = NULL;

    char *buffer = NULL;
    float *bufferf = NULL;

    if (st->settings->pabort && *(st->settings->pabort)) {
        return;
    }
    if (deadbeef->pl_get_item_duration (st->settings->tracks[st->track_index]) <= 0) {
        st->settings->results[st->track_index].scan_result = DDB_RG_SCAN_RESULT_INVALID_FILE;
        return;
    }


    deadbeef->pl_lock ();
    dec = (DB_decoder_t *)deadbeef->plug_get_for_id (deadbeef->pl_find_meta (st->settings->tracks[st->track_index], ":DECODER"));
    deadbeef->pl_unlock ();

    if (dec) {
        fileinfo = dec->open (DDB_DECODER_HINT_RAW_SIGNAL);

        if (!fileinfo || dec->init (fileinfo, DB_PLAYITEM (st->settings->tracks[st->track_index])) != 0) {
            st->settings->results[st->track_index].scan_result = DDB_RG_SCAN_RESULT_FILE_NOT_FOUND;
            goto error;
        }

        st->gain_state[st->track_index] = ebur128_init(fileinfo->fmt.channels, fileinfo->fmt.samplerate, EBUR128_MODE_I);
        st->peak_state[st->track_index] = ebur128_init(fileinfo->fmt.channels, fileinfo->fmt.samplerate, EBUR128_MODE_SAMPLE_PEAK);

        // speaker mask mapping from WAV to EBUR128
        static const int chmap[18] = {
            EBUR128_LEFT,
            EBUR128_RIGHT,
            EBUR128_CENTER,
            EBUR128_UNUSED,
            EBUR128_LEFT_SURROUND,
            EBUR128_RIGHT_SURROUND,
            EBUR128_LEFT_SURROUND,
            EBUR128_RIGHT_SURROUND,
            EBUR128_CENTER,
            EBUR128_LEFT_SURROUND,
            EBUR128_RIGHT_SURROUND,
            EBUR128_CENTER,
            EBUR128_LEFT_SURROUND,
            EBUR128_CENTER,
            EBUR128_RIGHT_SURROUND,
            EBUR128_LEFT_SURROUND,
            EBUR128_CENTER,
            EBUR128_RIGHT_SURROUND,
        };

        uint32_t channelmask = fileinfo->fmt.channelmask;

        // first 18 speaker positions are known, the rest will be marked as UNUSED
        int ch = 0;
        for (int i = 0; i < 32 && ch < fileinfo->fmt.channels; i++) {
            if (i < 18) {
                if (channelmask & (1<<i))
                {
                    ebur128_set_channel (st->gain_state[st->track_index], ch, chmap[i]);
                    ebur128_set_channel (st->peak_state[st->track_index], ch, chmap[i]);
                    ch++;
                }
            }
            else {
                ebur128_set_channel (st->gain_state[st->track_index], ch, EBUR128_UNUSED);
                ebur128_set_channel (st->peak_state[st->track_index], ch, EBUR128_UNUSED);
                ch++;
            }
        }

        int samplesize = fileinfo->fmt.channels * fileinfo->fmt.bps / 8;

        int bs = 2000 * samplesize;
        ddb_waveformat_t fmt;

        buffer = malloc (bs);

        if (!fileinfo->fmt.is_float) {
            bufferf = malloc (2000 * sizeof (float) * fileinfo->fmt.channels);
            memcpy (&fmt, &fileinfo->fmt, sizeof (fmt));
            fmt.bps = 32;
            fmt.is_float = 1;
        }
        else {
            bufferf = (float *)buffer;
        }

        int eof = 0;
        for (;;) {
            if (eof) {
                break;
            }
            if (st->settings->pabort && *(st->settings->pabort)) {
                break;
            }

            int sz = dec->read (fileinfo, buffer, bs); // read one block

            dispatch_sync(st->sync_queue, ^{
                int samplesize = fileinfo->fmt.channels * (fileinfo->fmt.bps >> 3);
                int numsamples = sz / samplesize;
                st->settings->cd_samples_processed += numsamples * 44100 / fileinfo->fmt.samplerate;
            });

            if (sz != bs) {
                eof = 1;
            }

            // convert from native output to float,
            // only if the input is not float already
            if (!fileinfo->fmt.is_float) {
                deadbeef->pcm_convert (&fileinfo->fmt, buffer, &fmt, (char *)bufferf, sz);
            }

            int frames = sz / samplesize;

            ebur128_add_frames_float (st->gain_state[st->track_index], bufferf, frames); // collect data
            ebur128_add_frames_float (st->peak_state[st->track_index], bufferf, frames); // collect data
        }

        if (!st->settings->pabort || !(*(st->settings->pabort))) {
            // calculating track peak
            // libEBUR128 calculates peak per channel, so we have to pick the highest value
            double tr_peak = 0;
            double ch_peak = 0;
            int res;
            for (int ch = 0; ch < fileinfo->fmt.channels; ++ch) {
                res = ebur128_sample_peak (st->peak_state[st->track_index], ch, &ch_peak);
                //trace ("rg_scanner: peak for ch %d: %f\n", ch, ch_peak);
                if (ch_peak > tr_peak) {
                    //trace ("rg_scanner: %f > %f\n", ch_peak, tr_peak);
                    tr_peak = ch_peak;
                }
            }

            st->settings->results[st->track_index].track_peak = (float) tr_peak;

            // calculate track loudness
            double loudness = st->settings->ref_loudness;
            ebur128_loudness_global (st->gain_state[st->track_index], &loudness);
            /*
             * EBUR128 sets the target level to -23 LUFS = 84dB
             * -> -23 - loudness = track gain to get to 84dB
             *
             * The old implementation of RG used 89dB, most people still use that
             * -> the above + (loudness - 84) = track gain to get to 89dB (or user specified)
             */
            if (loudness != -HUGE_VAL) {
                st->settings->results[st->track_index].track_gain = -23 - loudness + st->settings->ref_loudness - 84;
            }
        }

        ebur128_destroy(&st->gain_state[st->track_index]);
        ebur128_destroy(&st->peak_state[st->track_index]);
    }

error:
    // clean up
    if (fileinfo) {
        dec->free (fileinfo);
    }

    if (buffer && buffer != (char *)bufferf) {
        free (buffer);
        buffer = NULL;
    }

    if (bufferf) {
        free (bufferf);
        bufferf = NULL;
    }
}

static int
_update_album_gain (ddb_rg_scanner_settings_t *settings, int i, int album_start, char *current_album, char *album, double loudness, ebur128_state **gain_state) {
    if (strcmp (album, current_album)) {
        if (i > 0) {
            // update current album gain/peak
            float album_peak = 0;

            for (int n = album_start; n < i; ++n) {
                if (album_peak < settings->results[n].track_peak) {
                    album_peak = settings->results[n].track_peak;
                }
            }

            // calculate gain of all tracks of the current album
            ebur128_loudness_global_multiple(&gain_state[album_start], (size_t)i-album_start, &loudness);

            float album_gain = -23 - (float)loudness + settings->ref_loudness - 84;

            for (int n = album_start; n < i; ++n) {
                settings->results[n].album_gain = album_gain;
                settings->results[n].album_peak = album_peak;
            }
        }

        // proceed to next album
        strcpy (current_album, album);

        //trace ("next album found %d -> %d\n", album_start, i);
        return i;
    }
    return album_start;
}

int
rg_scan (ddb_rg_scanner_settings_t *settings) {
    if (settings->_size != sizeof (ddb_rg_scanner_settings_t)) {
        return -1;
    }

    if (settings->num_threads <= 0) {
        settings->num_threads = 4;
    }

    char *album_signature_tf = NULL;
    if (settings->mode == DDB_RG_SCAN_MODE_ALBUMS_FROM_TAGS) {
        album_signature_tf = deadbeef->tf_compile (album_signature);
        deadbeef->sort_track_array (NULL, settings->tracks, settings->num_tracks, album_signature, DDB_SORT_ASCENDING);
    }

    //trace ("rg_scanner: using %d thread(s)\n", settings->num_threads);

    ebur128_state **gain_state = NULL;
    ebur128_state **peak_state = NULL;

    if (settings->ref_loudness == 0) {
        settings->ref_loudness = DDB_RG_SCAN_DEFAULT_LOUDNESS;
    }

    double loudness = settings->ref_loudness;

    // allocate status array
    gain_state = calloc (settings->num_tracks, sizeof (ebur128_state *));
    peak_state = calloc (settings->num_tracks, sizeof (ebur128_state *));

    track_state_t *track_states = calloc (settings->num_tracks, sizeof (track_state_t));

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(settings->num_threads);
    dispatch_queue_t queue = dispatch_queue_create("rg_scanner", DISPATCH_QUEUE_CONCURRENT);
    dispatch_queue_t sync_queue = dispatch_queue_create("rg_scanner_sync", 0);

    // calculate gain for each track and album
    for (int i = 0; i < settings->num_tracks; ++i) {
        if (settings->progress_callback) {
            settings->progress_callback (i, settings->progress_cb_user_data);
        }

        if (settings->pabort && *(settings->pabort)) {
            // wait for remaining jobs / close semaphore
            for (int i = 0; i < settings->num_threads; i++) {
                dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
            }
            goto cleanup;
        }

        // close semaphore before starting the next job
        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);

        // initialize arguments
        track_states[i].track_index = i;
        track_states[i].settings = settings;
        track_states[i].gain_state = gain_state;
        track_states[i].peak_state = peak_state;
        track_states[i].sync_queue = sync_queue;

        dispatch_async(queue, ^{
            rg_calc_track(&track_states[i]);

            // open semaphore when the job is done
            dispatch_semaphore_signal(semaphore);
        });
    }

    // wait for remaining jobs / close semaphore
    for (int i = 0; i < settings->num_threads; i++) {
        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    }

    if (settings->mode == DDB_RG_SCAN_MODE_ALBUMS_FROM_TAGS) {
        int album_start = -1;
        char current_album[1000] = "";
        char album[1000];

        ddb_tf_context_t ctx;
        memset (&ctx, 0, sizeof (ctx));

        ctx._size = sizeof (ctx);
        ctx.plt = NULL;
        ctx.idx = -1;
        ctx.id = -1;

        for (int i = 0; i <= settings->num_tracks; i++) {
            // set album gain when the album change is detected
            if (i < settings->num_tracks) {
                ctx.it = settings->tracks[i];
                deadbeef->tf_eval(&ctx, album_signature_tf, album, sizeof (album));
            }
            else {
                *album = 0;
            }
            album_start = _update_album_gain (settings, i, album_start, current_album, album, loudness, gain_state);
        }
    }

    if (settings->mode == DDB_RG_SOURCE_MODE_ALBUM) {
        float album_peak = 0;

        for (int i = 0; i < settings->num_tracks; ++i) {
            if (album_peak < settings->results[i].track_peak) {
                album_peak = settings->results[i].track_peak;
            }
        }

        // calculate gain of all tracks combined
        ebur128_loudness_global_multiple(gain_state, (size_t)settings->num_tracks, &loudness);

        float album_gain = -23 - (float)loudness + settings->ref_loudness - 84;

        for (int i = 0; i < settings->num_tracks; ++i) {
            settings->results[i].album_gain = album_gain;
            settings->results[i].album_peak = album_peak;
        }
    }

cleanup:
    // It is assumed that semaphore is closed at this point,
    // need to open before calling release.
    for (int i = 0; i < settings->num_threads; i++) {
        dispatch_semaphore_signal (semaphore);
    }
    dispatch_release(semaphore);
    semaphore = NULL;
    dispatch_release(queue);
    queue = NULL;
    dispatch_release(sync_queue);
    sync_queue = NULL;

    if (track_states) {
        free (track_states);
        track_states = NULL;
    }

    if (gain_state) {
        for (int i = 0; i < settings->num_tracks; ++i) {
            if (gain_state[i]) {
                ebur128_destroy (&gain_state[i]);
            }
        }
        free (gain_state);
        gain_state = NULL;
    }

    if (peak_state) {
        for (int i = 0; i < settings->num_tracks; ++i) {
            if (peak_state[i]) {
                ebur128_destroy (&peak_state[i]);
            }
        }
        free (peak_state);
        peak_state = NULL;
    }

    if (album_signature_tf) {
        deadbeef->tf_free (album_signature_tf);
        album_signature_tf = NULL;
    }

    return 0;
}

static int
_rg_write_meta (DB_playItem_t *track) {
    const char *path = NULL;
    const char *decoder_id = NULL;

    deadbeef->pl_lock ();
    path = strdupa (deadbeef->pl_find_meta_raw (track, ":URI"));
    int is_subtrack = deadbeef->pl_get_item_flags (track) & DDB_IS_SUBTRACK;
    if (is_subtrack) {
        trace ("rg_scanner: Can't write to subtrack of file: %s\n", path);
        deadbeef->pl_unlock ();
        return -1;
    }
    decoder_id = deadbeef->pl_find_meta_raw (track, ":DECODER");
    if (!decoder_id) {
        trace ("rg_scanner: Invalid decoder in track %s\n", path);
        deadbeef->pl_unlock ();
        return -1;
    }
    decoder_id = strdupa (decoder_id);

    int match = track && decoder_id;
    deadbeef->pl_unlock ();

    if (match) {
        int is_subtrack = deadbeef->pl_get_item_flags (track) & DDB_IS_SUBTRACK;
        if (is_subtrack) {
            return 0; // only write tags for actual tracks
        }
        // find decoder
        DB_decoder_t *dec = NULL;
        DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
        for (int i = 0; decoders[i]; i++) {
            if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                dec = decoders[i];
                if (dec->write_metadata) {
                    if (dec->write_metadata (track)) {
                        trace ("rg_scanner: Failed to write tag to %s\n", path);
                        return -1;
                    }
                }
                else {
                    trace ("rg_scanner: Writing tags is not supported for the file %s\n", path);
                }
                break;
            }
        }
    }
    else {
        trace ("rg_scanner: Could not find matching decoder for %s\n", path);
        return -1;
    }
    return 0;
}

static void
_rg_remove_meta (DB_playItem_t *track) {
    deadbeef->pl_delete_meta (track, ":REPLAYGAIN_ALBUMGAIN");
    deadbeef->pl_delete_meta (track, ":REPLAYGAIN_ALBUMPEAK");
    deadbeef->pl_delete_meta (track, ":REPLAYGAIN_TRACKGAIN");
    deadbeef->pl_delete_meta (track, ":REPLAYGAIN_TRACKPEAK");
}

int
rg_apply (DB_playItem_t *track, uint32_t flags, float track_gain, float track_peak, float album_gain, float album_peak) {
    _rg_remove_meta(track);

    if (flags & (1<<DDB_REPLAYGAIN_TRACKGAIN)) {
        deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_TRACKGAIN, track_gain);
    }
    if (flags & (1<<DDB_REPLAYGAIN_TRACKPEAK)) {
        deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_TRACKPEAK, track_peak);
    }
    if (flags & (1<<DDB_REPLAYGAIN_ALBUMGAIN)) {
        deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_ALBUMGAIN, album_gain);
    }
    if (flags & (1<<DDB_REPLAYGAIN_ALBUMPEAK)) {
        deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_ALBUMPEAK, album_peak);
    }

    return _rg_write_meta (track);
}

int
rg_remove (DB_playItem_t *track) {
    _rg_remove_meta (track);

    return _rg_write_meta (track);
}

// plugin structure and info
static ddb_rg_scanner_t plugin = {
    .misc.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .misc.plugin.api_vminor = DB_API_VERSION_MINOR,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 0,
    .misc.plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.name = "ReplayGain Scanner",
    .misc.plugin.id = "rg_scanner",
    .misc.plugin.descr =
        "Calculates and writes ReplayGain tags, according to EBUR128 spec.",
    .misc.plugin.copyright =
        "ReplayGain Scanner plugin for DeaDBeeF Player\n"
        "\n"
        "Copyright (c) 2016 Oleksiy Yakovenko\n"
        "\n"
        "Based on ddb_misc_replaygain_scan (c) 2015 Ivan Pilipenko\n"
        "\n"
        "libEBUR128\n"
        "Copyright (c) 2011 Jan Kokemüller\n"
        "\n"
        "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
        "of this software and associated documentation files (the \"Software\"), to deal\n"
        "in the Software without restriction, including without limitation the rights\n"
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
        "copies of the Software, and to permit persons to whom the Software is\n"
        "furnished to do so, subject to the following conditions:\n"
        "\n"
        "The above copyright notice and this permission notice shall be included in\n"
        "all copies or substantial portions of the Software.\n"
        "\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
        "THE SOFTWARE.\n",
    .misc.plugin.website = "http://deadbeef.sf.net",
    .scan = rg_scan,
    .apply = rg_apply,
    .remove = rg_remove
};

DB_plugin_t *
rg_scanner_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
