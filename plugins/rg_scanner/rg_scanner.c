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

#include "rg_scanner.h"

#include <string.h>
#include <stdlib.h>

#include "../../deadbeef.h"
#include "ebur128/ebur128.h"

// Initial changes, compared to the original plugin:
// * Use the new logging API
// * Better plugin API, using a shared structure instead of many arguments
// * Better format conversion / buffer management
// * Code reformatted
// * Use the new DDB_DECODER_HINT_RAW_SIGNAL for getting the signal with "headroom"

#define trace(...) { deadbeef->log_detailed (&plugin.misc.plugin, 0, __VA_ARGS__); }

static ddb_rg_scanner_t plugin;
static DB_functions_t *deadbeef;

DB_plugin_t *rg_scanner_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

struct rg_thread_arg {
    int result; // result of this thread
    int thread_id; // number of this thread
    ddb_rg_scanner_settings_t *settings;
    ebur128_state **status_gain;
    ebur128_state **status_peak;
    double loudness;
};

void rg_calc_thread(void *_args) {
    char *buffer = NULL;
    char *bufferf = NULL;

    struct rg_thread_arg *args = (struct rg_thread_arg *)_args;
    if (args->settings->pabort && args->settings->pabort) {
        trace ("rg scan: user asked to abort, main loop aborted.\n");
        args->result = -2;
        return;
    }
    if (deadbeef->pl_get_item_duration (args->settings->scan_items[args->thread_id]) <= 0) {
        deadbeef->pl_lock ();
        trace ("rg scan: stream %s doesn't have finite length, skipped\n", deadbeef->pl_find_meta (args->settings->scan_items[args->thread_id], ":URI"));
        deadbeef->pl_unlock ();
        args->result = -1;
        return;
    }

    DB_decoder_t *dec = NULL;
    DB_fileinfo_t *fileinfo = NULL;

    deadbeef->pl_lock ();
    dec = (DB_decoder_t *)deadbeef->plug_get_for_id (deadbeef->pl_find_meta (args->settings->scan_items[args->thread_id], ":DECODER"));
    deadbeef->pl_unlock ();

    if (dec) { // we have our decoder
        fileinfo = dec->open (DDB_DECODER_HINT_RAW_SIGNAL);
        if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (args->settings->scan_items[args->thread_id])) != 0) {
            deadbeef->pl_lock ();
            trace ("rg scan: failed to decode file %s\n", deadbeef->pl_find_meta (args->settings->scan_items[args->thread_id], ":URI"));
            deadbeef->pl_unlock ();
            args->result = -1;
            return;
        }

        if (fileinfo) { // we have all info needed to scan
            // this is a status object for ebur128 gain scanning
            args->status_gain[args->thread_id] = ebur128_init(fileinfo->fmt.channels,   // channels
                                          fileinfo->fmt.samplerate, // samplerate
                                          EBUR128_MODE_I);          // mode: Integrated (over the length of the track)

            // this is a status object for ebur128 peak scanning - needs a different mode, so separate
            args->status_peak[args->thread_id] = ebur128_init(fileinfo->fmt.channels,   // channels
                                          fileinfo->fmt.samplerate, // samplerate
                                          EBUR128_MODE_SAMPLE_PEAK);// mode: find sample peak
            if(args->status_gain[args->thread_id] == NULL || args->status_peak[args->thread_id] == NULL) {
                deadbeef->pl_lock ();
                trace ("rg scan: failed to init libebur128 object for file %s, aborting\n", deadbeef->pl_find_meta (args->settings->scan_items[args->thread_id], ":URI"));
                deadbeef->pl_unlock ();
                args->result = -1;
                return;
            }

            // setting channel map
            switch(fileinfo->fmt.channels) {
                case 1: // mono
                    ebur128_set_channel (args->status_gain[args->thread_id], 0, EBUR128_CENTER);

                    ebur128_set_channel (args->status_peak[args->thread_id], 0, EBUR128_CENTER);
                    break;
                case 2: // stereo
                    ebur128_set_channel (args->status_gain[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel (args->status_gain[args->thread_id], 1, EBUR128_RIGHT);

                    ebur128_set_channel (args->status_peak[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel (args->status_peak[args->thread_id], 1, EBUR128_RIGHT);
                    break;
                case 3: // 3.1
                    ebur128_set_channel(args->status_gain[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 2, EBUR128_CENTER);

                    ebur128_set_channel(args->status_peak[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 2, EBUR128_CENTER);
                    break;
                case 4:
                    ebur128_set_channel(args->status_gain[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 2, EBUR128_LEFT_SURROUND);
                    ebur128_set_channel(args->status_gain[args->thread_id], 3, EBUR128_RIGHT_SURROUND);

                    ebur128_set_channel(args->status_peak[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 2, EBUR128_LEFT_SURROUND);
                    ebur128_set_channel(args->status_peak[args->thread_id], 3, EBUR128_RIGHT_SURROUND);
                    break;
                case 5:
                    ebur128_set_channel(args->status_gain[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 2, EBUR128_CENTER);
                    ebur128_set_channel(args->status_gain[args->thread_id], 3, EBUR128_LEFT_SURROUND);
                    ebur128_set_channel(args->status_gain[args->thread_id], 4, EBUR128_RIGHT_SURROUND);

                    ebur128_set_channel(args->status_peak[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 2, EBUR128_CENTER);
                    ebur128_set_channel(args->status_peak[args->thread_id], 3, EBUR128_LEFT_SURROUND);
                    ebur128_set_channel(args->status_peak[args->thread_id], 4, EBUR128_RIGHT_SURROUND);
                    break;
                case 6:
                    ebur128_set_channel(args->status_gain[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_gain[args->thread_id], 2, EBUR128_CENTER);
                    // LFE is not being taken into account when scanning
                    // see R128 spec at https://tech.ebu.ch/docs/tech/tech3341.pdf
                    ebur128_set_channel(args->status_gain[args->thread_id], 3, EBUR128_UNUSED);
                    ebur128_set_channel(args->status_gain[args->thread_id], 4, EBUR128_LEFT_SURROUND);
                    ebur128_set_channel(args->status_gain[args->thread_id], 5, EBUR128_RIGHT_SURROUND);

                    ebur128_set_channel(args->status_peak[args->thread_id], 0, EBUR128_LEFT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 1, EBUR128_RIGHT);
                    ebur128_set_channel(args->status_peak[args->thread_id], 2, EBUR128_CENTER);
                    ebur128_set_channel(args->status_peak[args->thread_id], 3, EBUR128_UNUSED);
                    ebur128_set_channel(args->status_peak[args->thread_id], 4, EBUR128_LEFT_SURROUND);
                    ebur128_set_channel(args->status_peak[args->thread_id], 5, EBUR128_RIGHT_SURROUND);
                    break;
                default:
                    deadbeef->pl_lock ();
                    trace ("rg scan: file %s has %d channels - libebur128 only supports up to 6. Aborting.\n",
                                     deadbeef->pl_find_meta (args->settings->scan_items[args->thread_id], ":URI"),
                                     fileinfo->fmt.channels);
                    deadbeef->pl_unlock ();
                    args->result = -1;
                    return;
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
                bufferf = buffer;
            }

            int eof = 0;
            for (;;) {
                if (eof) {
                    break;
                }
                if (args->settings->pabort && *args->settings->pabort) {
                    trace ("rg scan: user asked to abort, scanning aborted.\n");
                    break;
                }

                int sz = dec->read (fileinfo, buffer, bs); // read one block

                if (sz != bs) {
                    eof = 1;
                }

                // convert from native output to float,
                // only if the input is not float already
                if (!fileinfo->fmt.is_float) {
                    deadbeef->pcm_convert (&fileinfo->fmt, buffer, &fmt, bufferf, sz);
                }

                int frames = sz / samplesize;

                ebur128_add_frames_float (args->status_gain[args->thread_id], (float*) bufferf, frames); // collect data
                ebur128_add_frames_float (args->status_peak[args->thread_id], (float*) bufferf, frames); // collect data
            }
        }
    }

    // calculating track peak
    // libEBUR128 calculates peak per channel, so we have to pick the highest value
    double tr_peak = 0;
    double ch_peak = 0;
    int res;
    for (int ch = 0; ch < fileinfo->fmt.channels; ++ch) {
        res = ebur128_sample_peak (args->status_peak[args->thread_id], ch, &ch_peak);
        if (res == EBUR128_ERROR_INVALID_MODE) {
            trace ("rg scan: internal error: invalid mode set\n");
            *args->settings->pabort = 1;
            args->result = -1;
            return;
        }
        trace ("rg scan: peak for ch %d: %f\n", ch, ch_peak);
        if (ch_peak > tr_peak) {
            trace ("rg scan: %f > %f\n", ch_peak, tr_peak);
            tr_peak = ch_peak;
        }
    }

    args->settings->out_track_pk[args->thread_id] = (float) tr_peak;

    // calculate track loudness
    ebur128_loudness_global (args->status_gain[args->thread_id], &args->loudness);

    /*
     * EBUR128 sets the target level to -23 LUFS = 84dB
     * -> -23 - loudness = track gain to get to 84dB
     *
     * The old implementation of RG used 89dB, most people still use that
     * -> the above + (targetdb - 84) = track gain to get to 89dB (or user specified)
     */
    args->settings->out_track_rg[args->thread_id] = (float)(-23 - args->loudness + args->settings->targetdb - 84);

    // clean up
    if (buffer) {
        free (buffer);
        buffer = NULL;
    }
    if (bufferf) {
        free (bufferf);
        bufferf = NULL;
    }
}

int rg_scan (ddb_rg_scanner_settings_t *settings) {
    if (settings->num_threads <= 0) {
        settings->num_threads = 1;
    }

    trace ("rg scan: using %d thread(s)\n", settings->num_threads);

    ebur128_state **status_gain = NULL;
    ebur128_state **status_peak = NULL;
    double loudness;

    settings->out_album_pk = 0;
    settings->out_album_rg = 0;

    // allocate status array
    status_gain = malloc (settings->num_tracks * sizeof (ebur128_state *));
    status_peak = malloc (settings->num_tracks * sizeof (ebur128_state *));

    // used for joining threads
    intptr_t *rg_threads = NULL;
    rg_threads = malloc (settings->num_tracks * sizeof (intptr_t));
    struct rg_thread_arg *args = NULL;
    args = malloc (settings->num_tracks * sizeof (struct rg_thread_arg));

    // calculate gain for each track
    for (int i = 0; i < settings->num_tracks; ++i) {
        // limit number of parallel threads
        if (i >= settings->num_threads) {
            // simple blocking mechanism: join 'oldest' thread
            deadbeef->thread_join(rg_threads[i - settings->num_threads]);
        }

        // initialize arguments
        args[i].result = 0;
        args[i].thread_id = i;
        args[i].settings = settings;
        args[i].status_gain = status_gain;
        args[i].status_peak = status_peak;
        args[i].loudness = loudness;

        // run thread
        rg_threads[i] = deadbeef->thread_start(&rg_calc_thread, (void*)(&args[i]));
    }

    /* wait for remaining threads to join */
    int remaining_thread_id = settings->num_tracks - settings->num_threads;
    if (remaining_thread_id < 0) {
        remaining_thread_id = 0;
    }
    for (int i = remaining_thread_id; i < settings->num_tracks; ++i) {
        deadbeef->thread_join(rg_threads[i]);
    }

    // update album peak if necessary
    for (int i = 0; i < settings->num_tracks; ++i) {
        if (settings->out_album_pk < args[i].settings->out_track_pk[i]) {
            settings->out_album_pk = args[i].settings->out_track_pk[i];
        }
    }

    /* free thread storage */
    if (rg_threads) {
        free (rg_threads);
        rg_threads = NULL;
    }
    if (args) {
        free (args);
        args = NULL;
    }

    // calculate album loudness
    ebur128_loudness_global_multiple(status_gain, (size_t)settings->num_tracks, &loudness);
    settings->out_album_rg = -23 - (float)loudness + settings->targetdb - 84; // see above

    // clean up
    if (status_gain) {
        for (int i = 0; i < settings->num_tracks; ++i) {
            ebur128_destroy(&status_gain[i]);
        }
        free(status_gain);
    }
    if (status_peak) {
        for (int i = 0; i < settings->num_tracks; ++i) {
            ebur128_destroy(&status_peak[i]);
        }
        free(status_peak);
    }
    return 0;
}

int rg_write_meta (DB_playItem_t *track) {
    deadbeef->pl_lock ();
    const char *dec = deadbeef->pl_find_meta_raw (track, ":DECODER");
    char decoder_id[100];
    if (dec) {
        strncpy (decoder_id, dec, sizeof (decoder_id));
    }
    int match = track && dec;
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
                    dec->write_metadata (track);
                }
                break;
            }
        }
    }
    else {
        deadbeef->pl_lock ();
        trace ("rg scan: could not find matching decoder for %s\n", deadbeef->pl_find_meta (track, ":URI"));
        deadbeef->pl_unlock ();
        return -1;
    }
    return 0;
}

int rg_apply (DB_playItem_t *track, float track_gain, float track_peak, float album_gain, float album_peak) {

    // set RG tags 
    deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_ALBUMGAIN, album_gain);
    deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_ALBUMPEAK, album_peak);
    deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_TRACKGAIN, track_gain);
    deadbeef->pl_set_item_replaygain (track, DDB_REPLAYGAIN_TRACKPEAK, track_peak);

    // tags are NOT written yet - they are merely data in the playlist item, so "flush" them to file
    return rg_write_meta (track);
}

void rg_remove (DB_playItem_t **work_items, int num_tracks) {
    for (int it = 0; it < num_tracks; ++it) {
        deadbeef->pl_delete_meta (work_items[it], ":REPLAYGAIN_ALBUMGAIN");
        deadbeef->pl_delete_meta (work_items[it], ":REPLAYGAIN_ALBUMPEAK");
        deadbeef->pl_delete_meta (work_items[it], ":REPLAYGAIN_TRACKGAIN");
        deadbeef->pl_delete_meta (work_items[it], ":REPLAYGAIN_TRACKPEAK");

        DB_playItem_t *track = work_items[it];
        deadbeef->pl_lock ();
        const char *dec = deadbeef->pl_find_meta_raw (track, ":DECODER");
        char decoder_id[100];
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = track && dec;
        deadbeef->pl_unlock ();
        if (match) {
            int is_subtrack = deadbeef->pl_get_item_flags (track) & DDB_IS_SUBTRACK;
            if (is_subtrack) {
                continue;
            }
            // find decoder
            DB_decoder_t *dec = NULL;
            DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                    dec = decoders[i];
                    if (dec->write_metadata) {
                        dec->write_metadata (track);
                    }
                    break;
                }
            }
        }
        deadbeef->pl_item_unref (work_items[it]);
    }
}

// plugin structure and info
static ddb_rg_scanner_t plugin = {
    .misc.plugin.api_vmajor = 1,
    .misc.plugin.api_vminor = 10,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 0,
    .misc.plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.name = "ReplayGain Scanner",
    .misc.plugin.id = "rg_scanner",
    .misc.plugin.descr =
        "Calculates and writes ReplayGain tags, based on the EBUR128 spec.\n"
        "Requires a GUI plugin, e.g. the GTK2 RG GUI plugin, to work.\n",
    .misc.plugin.copyright =
        "ReplayGain Scanner plugin for DeaDBeeF Player\n"
        "\n"
        "Copyright (c) 2015 Ivan Pilipenko\n"
        "Copyright (c) 2016 Alexey Yakovenko\n"
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
