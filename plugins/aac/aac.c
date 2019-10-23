/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <neaacdec.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <stdlib.h>
#include <math.h>
#include "../../deadbeef.h"
#include "../../strdupa.h"
#include "aac_parser.h"

#include "mp4ff.h"

#include "../../shared/mp4tagutil.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

#define AAC_BUFFER_SIZE 768*8
#define OUT_BUFFER_SIZE 100000

#define MP4FILE mp4ff_t *
#define MP4FILE_CB mp4ff_callback_t


// aac channel mapping
// 0: Defined in AOT Specifc Config
// 1: 1 channel: front-center
// 2: 2 channels: front-left, front-right
// 3: 3 channels: front-center, front-left, front-right
// 4: 4 channels: front-center, front-left, front-right, back-center
// 5: 5 channels: front-center, front-left, front-right, back-left, back-right
// 6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
// 7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
// 8-15: Reserved


typedef struct {
    DB_fileinfo_t info;
    NeAACDecHandle dec;
    DB_FILE *file;
    MP4FILE mp4;
    MP4FILE_CB mp4reader;
    NeAACDecFrameInfo frame_info; // last frame info
    int mp4track;
    int mp4samples;
    int mp4sample;
    int mp4framesize;
    int skipsamples;
    int64_t startsample;
    int64_t endsample;
    int64_t currentsample;
    uint8_t buffer[AAC_BUFFER_SIZE];
    int remaining;
    uint8_t out_buffer[OUT_BUFFER_SIZE];
    int out_remaining;
    int num_errors;
    char *samplebuffer;
    int remap[10];
    int noremap;
    int eof;
    int junk;
} aac_info_t;

// allocate codec control structure
static DB_fileinfo_t *
aac_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (aac_info_t));
    aac_info_t *info = (aac_info_t *)_info;
    memset (info, 0, sizeof (aac_info_t));
    return _info;
}

static uint32_t
aac_fs_read (void *user_data, void *buffer, uint32_t length) {
//    trace ("aac_fs_read %d\n", length);
    aac_info_t *info = user_data;
    return (uint32_t)deadbeef->fread (buffer, 1, length, info->file);
}
static uint32_t
aac_fs_seek (void *user_data, uint64_t position) {
    aac_info_t *info = user_data;
//    trace ("aac_fs_seek %lld (%lld)\n", position, position + info->junk);
    return deadbeef->fseek (info->file, position+info->junk, SEEK_SET);
}


static int64_t
parse_aac_stream(DB_FILE *fp, int *psamplerate, int *pchannels, float *pduration, int64_t *ptotalsamples)
{
    size_t framepos = deadbeef->ftell (fp);
    int64_t firstframepos = -1;
    int64_t fsize = -1;
    int offs = 0;
    if (!fp->vfs->is_streaming ()) {
        int skip = deadbeef->junk_get_leading_size (fp);
        if (skip >= 0) {
            deadbeef->fseek (fp, skip, SEEK_SET);
        }
        fsize = deadbeef->fgetlength (fp);
        if (skip > 0) {
            fsize -= skip;
        }
    }

    uint8_t buf[ADTS_HEADER_SIZE*8];

    int nsamples = 0;
    int stream_sr = 0;
    int stream_ch = 0;

    int bufsize = 0;

    int frame = 0;
    int scanframes = 1000;
    if (fp->vfs->is_streaming ()) {
        scanframes = 1;
    }

    do {
        int size = sizeof (buf) - bufsize;
        if (deadbeef->fread (buf + bufsize, 1, size, fp) != size) {
            trace ("parse_aac_stream: eof\n");
            break;
        }
        bufsize = sizeof (buf);

        int channels, samplerate, bitrate, samples;
        size = aac_sync (buf, &channels, &samplerate, &bitrate, &samples);
        if (size == 0) {
            memmove (buf, buf+1, sizeof (buf)-1);
            bufsize--;
            trace ("aac_sync fail, framepos: %d\n", framepos);
            framepos++;
            continue;
        }
        else {
            trace ("aac: frame #%d sync: %dch %d %d %d %d\n", frame, channels, samplerate, bitrate, samples, size);
            frame++;
            nsamples += samples;
            if (!stream_sr) {
                stream_sr = samplerate;
            }
            if (!stream_ch) {
                stream_ch = channels;
            }
            if (firstframepos == -1) {
                firstframepos = framepos;
            }
//            if (fp->vfs->streaming) {
//                *psamplerate = stream_sr;
//                *pchannels = stream_ch;
//            }
            framepos += size;
            if (deadbeef->fseek (fp, size-(int)sizeof(buf), SEEK_CUR) == -1) {
                trace ("parse_aac_stream: invalid seek %d\n", size-sizeof(buf));
                break;
            }
            bufsize = 0;
        }
    } while (ptotalsamples || frame < scanframes);

    if (!frame || !stream_sr || !nsamples) {
        return -1;
    }

    *psamplerate = stream_sr;

    *pchannels = stream_ch;

    if (ptotalsamples) {
        *ptotalsamples = nsamples;
        *pduration = nsamples / (float)stream_sr;
        trace ("aac: duration=%f (%d samples @ %d Hz), fsize=%d, nframes=%d\n", *pduration, *ptotalsamples, stream_sr, fsize, frame);
    }
    else {
        int64_t pos = deadbeef->ftell (fp);
        int totalsamples = (double)fsize / (pos-offs) * nsamples;
        *pduration = totalsamples / (float)stream_sr;
        trace ("aac: duration=%f (%d samples @ %d Hz), fsize=%d\n", *pduration, totalsamples, stream_sr, fsize);
    }

    if (*psamplerate <= 24000) {
        *psamplerate *= 2;
        if (ptotalsamples) {
            *ptotalsamples *= 2;
        }
    }
    return firstframepos;
}

static int
mp4_track_get_info(mp4ff_t *mp4, int track, float *duration, int *samplerate, int *channels, int64_t *totalsamples, int *mp4framesize) {
    int sr = -1;
    unsigned char*  buff = 0;
    unsigned int    buff_size = 0;
    mp4AudioSpecificConfig mp4ASC;
    mp4ff_get_decoder_config(mp4, track, &buff, &buff_size);
    if (buff) {
        int rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
        sr = (int)mp4ASC.samplingFrequency;
        if(rc < 0) {
            free (buff);
            trace ("aac: AudioSpecificConfig returned result=%d\n", rc);
            return -1;
        }
    }

    unsigned long srate;
    unsigned char ch;
    int samples;

    // init mp4 decoding
    NeAACDecHandle dec = NeAACDecOpen ();
    if (NeAACDecInit2(dec, buff, buff_size, &srate, &ch) < 0) {
        trace ("NeAACDecInit2 returned error\n");
        goto error;
    }
    *samplerate = (int)srate;
    *channels = ch;
    samples = mp4ff_num_samples(mp4, track);
    
    NeAACDecClose (dec);
    dec = NULL;

    if (samples <= 0) {
        goto error;
    }

    int i_sample_count = samples;
    int i_sample;

    int64_t total_dur = 0;
    for( i_sample = 0; i_sample < i_sample_count; i_sample++ )
    {
        total_dur += mp4ff_get_sample_duration (mp4, track, i_sample);
    }
    if (totalsamples) {
        *totalsamples = total_dur * (*samplerate) / mp4ff_time_scale (mp4, track);
        *mp4framesize = (int)((*totalsamples) / i_sample_count);
    }
    *duration = total_dur / (float)mp4ff_time_scale (mp4, track);

    return 0;
error:
    if (dec) {
        NeAACDecClose (dec);
    }
    free (buff);
    return -1;
}

// returns -1 for error, 0 for aac
static int
aac_probe (DB_FILE *fp, float *duration, int *samplerate, int *channels, int64_t *totalsamples) {

    deadbeef->rewind (fp);
    if (parse_aac_stream (fp, samplerate, channels, duration, totalsamples) == -1) {
        trace ("aac stream not found\n");
        return -1;
    }
    trace ("found aac stream\n");
    return 0;
}


static int
aac_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    aac_info_t *info = (aac_info_t *)_info;

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    info->file = deadbeef->fopen (uri);
    if (!info->file) {
        return -1;
    }

    // probe
    float duration = -1;
    int samplerate = -1;
    int channels = -1;
    int64_t totalsamples = -1;

    if (!info->file->vfs->is_streaming ()) {
        info->junk = deadbeef->junk_get_leading_size (info->file);
        if (info->junk >= 0) {
            deadbeef->fseek (info->file, info->junk, SEEK_SET);
        }
        else {
            info->junk = 0;
        }
    }
    else {
        deadbeef->fset_track (info->file, it);
    }

    info->mp4track = -1;
    info->mp4reader.read = aac_fs_read;
    info->mp4reader.write = NULL;
    info->mp4reader.seek = aac_fs_seek;
    info->mp4reader.truncate = NULL;
    info->mp4reader.user_data = info;

    trace ("aac_init: mp4ff_open_read %s\n", deadbeef->pl_find_meta (it, ":URI"));
    info->mp4 = mp4ff_open_read (&info->mp4reader);
    if (info->mp4) {
        int ntracks = mp4ff_total_tracks (info->mp4);
        for (int i = 0; i < ntracks; i++) {
            if (mp4ff_get_track_type (info->mp4, i) != TRACK_AUDIO) {
                continue;
            }
            int res = mp4_track_get_info (info->mp4, i, &duration, &samplerate, &channels, &totalsamples, &info->mp4framesize);
            if (res >= 0 && duration > 0) {
                trace ("mp4_track_get_info: %d, dur: %f, samplerate: %d, channels: %d, totalsamples: %d\n", i, (float)duration, (int)samplerate, (int)channels, (int)totalsamples);
                info->mp4track = i;
                break;
            }
        }
        trace ("track: %d\n", info->mp4track);
        if (info->mp4track >= 0) {
            // init mp4 decoding
            info->mp4samples = mp4ff_num_samples(info->mp4, info->mp4track);
            info->dec = NeAACDecOpen ();
            unsigned long srate;
            unsigned char ch;
            unsigned char*  buff = 0;
            unsigned int    buff_size = 0;
            mp4ff_get_decoder_config (info->mp4, info->mp4track, &buff, &buff_size);
            if (NeAACDecInit2(info->dec, buff, buff_size, &srate, &ch) < 0) {
                trace ("NeAACDecInit2 returned error\n");
                free (buff);
                return -1;
            }

            if (buff) {
                free (buff);
            }
            trace ("aac: successfully initialized track %d\n", info->mp4track);
            _info->fmt.samplerate = samplerate;
            _info->fmt.channels = channels;
        }
        else {
            trace ("aac: track not found in mp4 container\n");
            mp4ff_close (info->mp4);
            info->mp4 = NULL;
            return -1;
        }
    }

    if (!info->mp4) {
        trace ("aac: looking for raw stream...\n");
        int64_t offs;
        if (!info->file->vfs->is_streaming ()) {
            if (info->junk >= 0) {
                deadbeef->fseek (info->file, info->junk, SEEK_SET);
            }
            else {
                deadbeef->rewind (info->file);
            }
            offs = parse_aac_stream (info->file, &samplerate, &channels, &duration, &totalsamples);
        }
        else {
            offs = parse_aac_stream (info->file, &samplerate, &channels, &duration, NULL);
        }
        if (offs == -1) {
            trace ("aac stream not found\n");
            return -1;
        }
        if (offs > info->junk) {
            info->junk = (int)offs;
        }
        if (!info->file->vfs->is_streaming ()) {
            if (info->junk >= 0) {
                deadbeef->fseek (info->file, info->junk, SEEK_SET);
            }
            else {
                deadbeef->rewind (info->file);
            }
        }
        if (info->file->vfs->is_streaming ()) {
            deadbeef->pl_replace_meta (it, "!FILETYPE", "AAC");
        }
        trace ("found aac stream (junk: %d, offs: %d)\n", info->junk, offs);

        trace ("NeAACDecOpen for raw stream\n");
        info->dec = NeAACDecOpen ();

        trace ("prepare for NeAACDecInit: fread %d from offs %lld\n", AAC_BUFFER_SIZE, deadbeef->ftell (info->file));

        NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration (info->dec);
        if (!NeAACDecSetConfiguration (info->dec, conf)) {
            return -1;
        }

        info->remaining = (int)deadbeef->fread (info->buffer, 1, AAC_BUFFER_SIZE, info->file);
        unsigned long srate;
        unsigned char ch;
        long err = NeAACDecInit (info->dec, (uint8_t *)info->buffer, info->remaining, &srate, &ch);
        if (err) {
            return -1;
        }

        _info->fmt.channels = ch;
        _info->fmt.samplerate = (int)srate;
    }

    _info->fmt.bps = 16;
    _info->plugin = &plugin;

    if (!info->file->vfs->is_streaming ()) {
        int64_t endsample = deadbeef->pl_item_get_endsample (it);
        if (endsample > 0) {
            info->startsample = deadbeef->pl_item_get_startsample (it);
            info->endsample = endsample;
            plugin.seek_sample (_info, 0);
        }
        else {
            info->startsample = 0;
            info->endsample = (int)totalsamples-1;
        }
    }
    if (_info->fmt.channels == 7) {
        _info->fmt.channels = 8;
    }

    char s[100];
    deadbeef->pl_replace_meta (it, ":BPS", "16");
    snprintf (s, sizeof (s), "%d", _info->fmt.channels);
    deadbeef->pl_replace_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", _info->fmt.samplerate);
    deadbeef->pl_replace_meta (it, ":SAMPLERATE", s);

    trace ("totalsamples: %d, endsample: %d, samples-from-duration: %d, samplerate %d, channels %d\n", (int)totalsamples, (int)info->endsample, (int)deadbeef->pl_get_item_duration (it)*44100, _info->fmt.samplerate, _info->fmt.channels);

    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    info->noremap = 0;
    for (int i = 0; i < sizeof (info->remap) / sizeof (int); i++) {
        info->remap[i] = -1;
    }

    trace ("init success\n");

    return 0;
}

static void
aac_free (DB_fileinfo_t *_info) {
    aac_info_t *info = (aac_info_t *)_info;
    if (info) {
        if (info->file) {
            deadbeef->fclose (info->file);
        }
        if (info->mp4) {
            mp4ff_close (info->mp4);
        }
        if (info->dec) {
            NeAACDecClose (info->dec);
        }
        free (info);
    }
}

static int
aac_read (DB_fileinfo_t *_info, char *bytes, int size) {
    aac_info_t *info = (aac_info_t *)_info;
    if (info->eof) {
        trace ("aac_read: received call after eof\n");
        return 0;
    }

    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (!info->file->vfs->is_streaming ()) {
        if (info->currentsample + size / samplesize > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * samplesize;
            if (size <= 0) {
                trace ("aac_read: eof (current=%d, total=%d)\n", info->currentsample, info->endsample);
                return 0;
            }
        }
    }

    int initsize = size;

    while (size > 0) {
        if (info->skipsamples > 0 && info->out_remaining > 0) {
            int skip = min (info->out_remaining, info->skipsamples);
//            trace ("skipping %d\n", skip);
            if (skip < info->out_remaining) {
                memmove (info->out_buffer, info->out_buffer + skip * samplesize, (info->out_remaining - skip) * samplesize);
            }
            info->out_remaining -= skip;
            info->skipsamples -= skip;
        }
        if (info->out_remaining > 0) {
            int n = size / samplesize;
            n = min (info->out_remaining, n);

            uint8_t *src = info->out_buffer;
            if (info->noremap) {
                memcpy (bytes, src, n * samplesize);
                bytes += n * samplesize;
                src += n * samplesize;
            }
            else {
                int i, j;
                if (info->remap[0] == -1) {
                    // build remap mtx
                    // FIXME: should build channelmask 1st; then remap based on channelmask
                    for (i = 0; i < _info->fmt.channels; i++) {
                        switch (info->frame_info.channel_position[i]) {
                        case FRONT_CHANNEL_CENTER:
                            trace ("FC->%d %d\n", i, 2);
                            info->remap[2] = i;
                            break;
                        case FRONT_CHANNEL_LEFT:
                            trace ("FL->%d %d\n", i, 0);
                            info->remap[0] = i;
                            break;
                        case FRONT_CHANNEL_RIGHT:
                            trace ("FR->%d %d\n", i, 1);
                            info->remap[1] = i;
                            break;
                        case SIDE_CHANNEL_LEFT:
                            trace ("SL->%d %d\n", i, 6);
                            info->remap[6] = i;
                            break;
                        case SIDE_CHANNEL_RIGHT:
                            trace ("SR->%d %d\n", i, 7);
                            info->remap[7] = i;
                            break;
                        case BACK_CHANNEL_LEFT:
                            trace ("RL->%d %d\n", i, 4);
                            info->remap[4] = i;
                            break;
                        case BACK_CHANNEL_RIGHT:
                            trace ("RR->%d %d\n", i, 5);
                            info->remap[5] = i;
                            break;
                        case BACK_CHANNEL_CENTER:
                            trace ("BC->%d %d\n", i, 8);
                            info->remap[8] = i;
                            break;
                        case LFE_CHANNEL:
                            trace ("LFE->%d %d\n", i, 3);
                            info->remap[3] = i;
                            break;
                        default:
                            trace ("aac: unknown ch(%d)->%d\n", info->frame_info.channel_position[i], i);
                            break;
                        }
                    }
                    for (i = 0; i < _info->fmt.channels; i++) {
                        trace ("%d ", info->remap[i]);
                    }
                    trace ("\n");
                    if (info->remap[0] == -1) {
                        info->remap[0] = 0;
                    }
                    if ((_info->fmt.channels == 1 && info->remap[0] == FRONT_CHANNEL_CENTER)
                        || (_info->fmt.channels == 2 && info->remap[0] == FRONT_CHANNEL_LEFT && info->remap[1] == FRONT_CHANNEL_RIGHT)) {
                        info->noremap = 1;
                    }
                }

                for (i = 0; i < n; i++) {
                    for (j = 0; j < _info->fmt.channels; j++) {
                        if (info->remap[j] == -1) {
                            ((int16_t *)bytes)[j] = 0;
                        }
                        else {
                            ((int16_t *)bytes)[j] = ((int16_t *)src)[info->remap[j]];
                        }
                    }
                    src += samplesize;
                    bytes += samplesize;
                }
            }
            size -= n * samplesize;

            if (n == info->out_remaining) {
                info->out_remaining = 0;
            }
            else {
                memmove (info->out_buffer, src, (info->out_remaining - n) * samplesize);
                info->out_remaining -= n;
            }
            continue;
        }

        char *samples = NULL;

        if (info->mp4) {
            if (info->mp4sample >= info->mp4samples) {
                trace ("aac: finished with the last mp4sample\n");
                break;
            }
            
            unsigned char *buffer = NULL;
            uint32_t buffer_size = 0;
            int rc = mp4ff_read_sample (info->mp4, info->mp4track, info->mp4sample, &buffer, &buffer_size);
            if (rc == 0) {
                trace ("mp4ff_read_sample failed\n");
                info->eof = 1;
                break;
            }
            info->mp4sample++;
            samples = NeAACDecDecode(info->dec, &info->frame_info, buffer, buffer_size);

            if (buffer) {
                free (buffer);
            }
            if (!samples) {
                trace ("aac: NeAACDecDecode returned NULL\n");
                break;
            }
        }
        else {
            if (info->remaining < AAC_BUFFER_SIZE) {
                trace ("fread from offs %lld\n", deadbeef->ftell (info->file));
                size_t res = deadbeef->fread (info->buffer + info->remaining, 1, AAC_BUFFER_SIZE-info->remaining, info->file);
                info->remaining += res;
                trace ("remain: %d\n", info->remaining);
                if (!info->remaining) {
                    break;
                }
            }

            trace ("NeAACDecDecode %d bytes\n", info->remaining)
            samples = NeAACDecDecode (info->dec, &info->frame_info, info->buffer, info->remaining);
            trace ("samples =%p\n", samples);
            if (!samples) {
                trace ("NeAACDecDecode failed with error %s (%d), consumed=%d\n", NeAACDecGetErrorMessage(info->frame_info.error), (int)info->frame_info.error, (int)info->frame_info.bytesconsumed);

                if (info->num_errors > 10) {
                    trace ("NeAACDecDecode failed %d times, interrupting\n", info->num_errors);
                    break;
                }
                info->num_errors++;
                info->remaining = 0;
                continue;
            }
            info->num_errors=0;
            unsigned long consumed = info->frame_info.bytesconsumed;
            if (consumed > info->remaining) {
                trace ("NeAACDecDecode consumed more than available! wtf?\n");
                break;
            }
            if (consumed == info->remaining) {
                info->remaining = 0;
            }
            else if (consumed > 0) {
                memmove (info->buffer, info->buffer + consumed, info->remaining - consumed);
                info->remaining -= consumed;
            }
        }

        if (info->frame_info.samples > 0) {
            memcpy (info->out_buffer, samples, info->frame_info.samples * 2);
            info->out_remaining = (int)(info->frame_info.samples / info->frame_info.channels);
        }
    }

    info->currentsample += (initsize-size) / samplesize;

    return initsize-size;
}

// returns -1 on error, 0 on success
int
seek_raw_aac (aac_info_t *info, int sample) {
    uint8_t buf[ADTS_HEADER_SIZE*8];

    int bufsize = 0;

    int frame = 0;

    int frame_samples = 0;
    int curr_sample = 0;

    do {
        curr_sample += frame_samples;
        int size = sizeof (buf) - bufsize;
        if (deadbeef->fread (buf + bufsize, 1, size, info->file) != size) {
            trace ("seek_raw_aac: eof\n");
            break;
        }
        bufsize = sizeof (buf);

        int channels, samplerate, bitrate;
        size = aac_sync (buf, &channels, &samplerate, &bitrate, &frame_samples);
        if (size == 0) {
            memmove (buf, buf+1, sizeof (buf)-1);
            bufsize--;
            continue;
        }
        else {
            //trace ("aac: frame #%d(%d/%d) sync: %d %d %d %d %d\n", frame, curr_sample, sample, channels, samplerate, bitrate, frame_samples, size);
            frame++;
            if (deadbeef->fseek (info->file, size-(int)sizeof(buf), SEEK_CUR) == -1) {
                trace ("seek_raw_aac: invalid seek %d\n", size-sizeof(buf));
                break;
            }
            bufsize = 0;
        }
        if (samplerate <= 24000) {
            frame_samples *= 2;
        }
    } while (curr_sample + frame_samples < sample);

    if (curr_sample + frame_samples < sample) {
        return -1;
    }

    return sample - curr_sample;
}

static int
aac_seek_sample (DB_fileinfo_t *_info, int sample) {
    aac_info_t *info = (aac_info_t *)_info;

    trace ("seek: start %d + %d\n", info->startsample, sample);

    sample += info->startsample;
    if (info->mp4) {
        int totalsamples = 0;
        int i;
        int num_sample_byte_sizes = mp4ff_get_num_sample_byte_sizes (info->mp4, info->mp4track);
        int scale = _info->fmt.samplerate / mp4ff_time_scale (info->mp4, info->mp4track);
        for (i = 0; i < num_sample_byte_sizes; i++)
        {
            unsigned int thissample_duration = 0;
            unsigned int thissample_bytesize = 0;

            mp4ff_get_sample_info(info->mp4, info->mp4track, i, &thissample_duration,
                    &thissample_bytesize);

            if (totalsamples + thissample_duration > sample / scale) {
                info->skipsamples = sample - totalsamples * scale;
                break;
            }
            totalsamples += thissample_duration;
        }
//        i = sample / info->mp4framesize;
//        info->skipsamples = sample - info->mp4sample * info->mp4framesize;
        info->mp4sample = i;
        trace ("seek res: frame %d (old: %d*%d), skip %d\n", info->mp4sample, sample / info->mp4framesize, info->mp4framesize, info->skipsamples);
    }
    else {
        int skip = deadbeef->junk_get_leading_size (info->file);
        if (skip >= 0) {
            deadbeef->fseek (info->file, skip, SEEK_SET);
        }
        else {
            deadbeef->fseek (info->file, 0, SEEK_SET);
        }

        int res = seek_raw_aac (info, sample);
        if (res < 0) {
            return -1;
        }
        info->skipsamples = res;
    }
    info->remaining = 0;
    info->out_remaining = 0;
    info->currentsample = sample;
    _info->readpos = (float)(info->currentsample - info->startsample) / _info->fmt.samplerate;
    return 0;
}

static int
aac_seek (DB_fileinfo_t *_info, float t) {
    return aac_seek_sample (_info, t * _info->fmt.samplerate);
}

typedef struct {
    char *title;
    int32_t startsample;
    int32_t endsample;
} aac_chapter_t;

static aac_chapter_t *
aac_load_itunes_chapters (mp4ff_t *mp4, /* out */ int *num_chapters, int samplerate) {
    *num_chapters = 0;
    int i_entry_count = mp4ff_chap_get_num_tracks (mp4);
    int i_tracks = mp4ff_total_tracks (mp4);
    int i, j;
    for( i = 0; i < i_entry_count; i++ )
    {
        for( j = 0; j < i_tracks; j++ )
        {
            trace ("aac: i_tracks=%d found track id=%d type=%d (expected %d %d)\n", i_tracks, mp4ff_get_track_id (mp4, j), mp4ff_get_track_type (mp4, j), mp4ff_chap_get_track_id (mp4, i), TRACK_TEXT);
            if(mp4ff_chap_get_track_id (mp4, i)  == mp4ff_get_track_id (mp4, j) &&
                    mp4ff_get_track_type (mp4, j) == TRACK_TEXT) {
                trace ("aac: found subt track\n");
                break;
            }
        }
        if( j < i_tracks )
        {
            int i_sample_count = mp4ff_num_samples (mp4, j);
            int i_sample;

            aac_chapter_t *chapters = malloc (sizeof (aac_chapter_t) * i_sample_count);
            memset (chapters, 0, sizeof (aac_chapter_t) * i_sample_count);
            *num_chapters = 0;

            int64_t total_dur = 0;
            int64_t curr_sample = 0;
            for( i_sample = 0; i_sample < i_sample_count; i_sample++ )
            {
#if 0
                const int64_t i_dts = mp4ff_get_track_dts (mp4, j, i_sample);
                const int64_t i_pts_delta = mp4ff_get_track_pts_delta(mp4, j, i_sample);
                trace ("i_dts = %lld, i_pts_delta = %lld\n", i_dts, i_pts_delta);
                const unsigned int i_size = mp4ff_get_track_sample_size(mp4, j, i_sample);
                if (i_size <= 0) {
                    continue;
                }

                int64_t i_time_offset = i_dts + max (i_pts_delta, 0);
#endif
                int32_t dur = (int64_t)1000 * mp4ff_get_sample_duration (mp4, j, i_sample) / mp4ff_time_scale (mp4, j); // milliseconds
                total_dur += dur;
#if 0
                trace ("dur: %d %f min // offs: %lld %f (from currsample: %f)\n", dur, dur / 1000.f / 60.f, i_time_offset, i_time_offset / 1000000.f / 60.f, curr_sample * 1000.f/ samplerate);
#else
                trace ("dur: %d %f min\n", dur, dur / 1000.f / 60.f);
#endif
                unsigned char *buffer = NULL;
                uint32_t buffer_size = 0;

                int rc = mp4ff_read_sample (mp4, j, i_sample, &buffer, &buffer_size);

                if (rc == 0 || !buffer) {
                    continue;
                }
                int len = (buffer[0] << 8) | buffer[1];
                len = min (len, buffer_size - 2);
                if (len > 0) {
                    chapters[*num_chapters].title = strndup ((const char *)&buffer[2], len);
                }
                chapters[*num_chapters].startsample = (int)curr_sample;
                curr_sample += (int64_t)dur * samplerate / 1000.f;
                chapters[*num_chapters].endsample = (int)curr_sample - 1;
                trace ("aac: chapter %d: %s, s=%d e=%d\n", *num_chapters, chapters[*num_chapters].title, chapters[*num_chapters].startsample, chapters[*num_chapters].endsample);
                if (buffer) {
                    free (buffer);
                }
                (*num_chapters)++;
            }
            trace ("aac: total dur: %lld (%f min)\n", total_dur, total_dur / 1000.f / 60.f);
            return chapters;
        }
    }
    return NULL;
}

static DB_playItem_t *
aac_insert_with_chapters (ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *origin, aac_chapter_t *chapters, int num_chapters, int64_t totalsamples, int samplerate) {
    deadbeef->pl_lock ();
    DB_playItem_t *ins = after;
    for (int i = 0; i < num_chapters; i++) {
        const char *uri = deadbeef->pl_find_meta_raw (origin, ":URI");
        const char *dec = deadbeef->pl_find_meta_raw (origin, ":DECODER");
        const char *ftype= "MP4 AAC";//pl_find_meta_raw (origin, ":FILETYPE");

        DB_playItem_t *it = deadbeef->pl_item_alloc_init (uri, dec);
        deadbeef->pl_set_meta_int (it, ":TRACKNUM", i);
        deadbeef->pl_set_meta_int (it, "TRACK", i);
        // poor-man utf8 check
        if (!chapters[i].title || deadbeef->junk_detect_charset (chapters[i].title)) {
            char s[1000];
            snprintf (s, sizeof (s), "chapter %d", i+1);
            deadbeef->pl_add_meta (it, "title", s);
        }
        else {
            deadbeef->pl_add_meta (it, "title", chapters[i].title);
        }
        deadbeef->pl_item_set_startsample (it, chapters[i].startsample);
        deadbeef->pl_item_set_endsample (it, chapters[i].endsample);
        deadbeef->pl_replace_meta (it, ":FILETYPE", ftype);
        deadbeef->plt_set_item_duration (plt, it, (float)(chapters[i].endsample - chapters[i].startsample + 1) / samplerate);
        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_item_ref (after);
    
    DB_playItem_t *first = deadbeef->pl_get_next (ins, PL_MAIN);
    
    if (!first) {
        first = deadbeef->plt_get_first (plt, PL_MAIN);
    }

    if (!first) {
        deadbeef->pl_unlock ();
        return NULL;
    }
    trace ("aac: split by chapters success\n");
    // copy metadata from embedded tags
    uint32_t f = deadbeef->pl_get_item_flags (origin);
    f |= DDB_IS_SUBTRACK;
    deadbeef->pl_set_item_flags (origin, f);
    deadbeef->pl_items_copy_junk (origin, first, after);
    deadbeef->pl_item_unref (first);

    deadbeef->pl_unlock ();
    return after;
}

static DB_playItem_t *
aac_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("adding %s\n", fname);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("not found\n");
        return NULL;
    }
    aac_info_t info = {0};
    info.junk = deadbeef->junk_get_leading_size (fp);
    if (info.junk >= 0) {
        trace ("junk: %d\n", info.junk);
        deadbeef->fseek (fp, info.junk, SEEK_SET);
    }
    else {
        info.junk = 0;
    }

    const char *ftype = NULL;
    float duration = -1;
    int64_t totalsamples = 0;
    int samplerate = 0;
    int channels = 0;

    if (fp->vfs->is_streaming ()) {
        trace ("streaming aac (%s)\n", fname);
        ftype = "RAW AAC";
    }
    else {
        // slowwww!
        info.file = fp;
        MP4FILE_CB cb = {
            .read = aac_fs_read,
            .write = NULL,
            .seek = aac_fs_seek,
            .truncate = NULL,
            .user_data = &info
        };
        mp4ff_t *mp4 = mp4ff_open_read (&cb);
        if (mp4) {
            int ntracks = mp4ff_total_tracks (mp4);
            trace ("aac: numtracks=%d\n", ntracks);
            int i;
            for (i = 0; i < ntracks; i++) {
                if (mp4ff_get_track_type (mp4, i) != TRACK_AUDIO) {
                    trace ("aac: track %d is not audio\n", i);
                    continue;
                }
                int mp4framesize;
                int res = mp4_track_get_info (mp4, i, &duration, &samplerate, &channels, &totalsamples, &mp4framesize);
                if (res >= 0 && duration > 0) {
                    trace ("aac: found audio track %d (duration=%f, totalsamples=%d)\n", i, duration, totalsamples);

                    int num_chapters = 0;
                    aac_chapter_t *chapters = NULL;
                    if (mp4ff_chap_get_num_tracks(mp4) > 0) {
                        chapters = aac_load_itunes_chapters (mp4, &num_chapters, samplerate);
                    }

                    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
                    ftype = "MP4 AAC";
                    deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
                    deadbeef->pl_set_meta_int (it, ":TRACKNUM", i);
                    deadbeef->plt_set_item_duration (plt, it, duration);

                    deadbeef->rewind (fp);
                    mp4_read_metadata_file(it, fp);

                    int64_t fsize = deadbeef->fgetlength (fp);
                    deadbeef->fclose (fp);
                    fp = NULL;

                    char s[100];
                    snprintf (s, sizeof (s), "%lld", fsize);
                    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
                    deadbeef->pl_add_meta (it, ":BPS", "16");
                    snprintf (s, sizeof (s), "%d", channels);
                    deadbeef->pl_add_meta (it, ":CHANNELS", s);
                    snprintf (s, sizeof (s), "%d", samplerate);
                    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
                    int br = (int)roundf(fsize / duration * 8 / 1000);
                    snprintf (s, sizeof (s), "%d", br);
                    deadbeef->pl_add_meta (it, ":BITRATE", s);

                    // embedded chapters
                    deadbeef->pl_lock (); // FIXME: is it needed?
                    if (chapters && num_chapters > 0) {
                        DB_playItem_t *cue = aac_insert_with_chapters (plt, after, it, chapters, num_chapters, totalsamples, samplerate);
                        for (int n = 0; n < num_chapters; n++) {
                            if (chapters[n].title) {
                                free (chapters[n].title);
                            }
                        }
                        free (chapters);
                        if (cue) {
                            mp4ff_close (mp4);
                            deadbeef->pl_item_unref (it);
                            deadbeef->pl_item_unref (cue);
                            deadbeef->pl_unlock ();
                            return cue;
                        }
                    }

                    deadbeef->pl_unlock ();

                    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, totalsamples, samplerate);
                    if (cue) {
                        deadbeef->pl_item_unref (it);
                        return cue;
                    }

                    after = deadbeef->plt_insert_item (plt, after, it);
                    deadbeef->pl_item_unref (it);
                    break;
                }
            }
            mp4ff_close (mp4);
            if (fp) {
                deadbeef->fclose (fp);
            }
            if (i < ntracks) {
                return after;
            }
            // mp4 container found, but no valid aac tracks in it
            return NULL;
        }
    }
    trace ("aac: mp4 container failed, trying raw aac\n");
    int res = aac_probe (fp, &duration, &samplerate, &channels, &totalsamples);
    if (res == -1) {
        deadbeef->fclose (fp);
        return NULL;
    }
    ftype = "RAW AAC";
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
    deadbeef->plt_set_item_duration (plt, it, duration);
    trace ("duration: %f sec\n", duration);

    // read tags
    (void)deadbeef->junk_apev2_read (it, fp);
    (void)deadbeef->junk_id3v2_read (it, fp);
    (void)deadbeef->junk_id3v1_read (it, fp);

    int64_t fsize = deadbeef->fgetlength (fp);

    deadbeef->fclose (fp);

    char s[100];
    deadbeef->pl_add_meta (it, ":BPS", "16");
    snprintf (s, sizeof (s), "%d", channels);
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", samplerate);
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    if (duration > 0) {
        snprintf (s, sizeof (s), "%lld", fsize);
        deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
        int br = (int)roundf(fsize / duration * 8 / 1000);
        snprintf (s, sizeof (s), "%d", br);
        deadbeef->pl_add_meta (it, ":BITRATE", s);
        DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, totalsamples, samplerate);
        if (cue) {
            deadbeef->pl_item_unref (it);
            return cue;
        }
    }

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);

    return after;
}

static const char * exts[] = { "aac", "mp4", "m4a", "m4b", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
//    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "aac",
    .plugin.name = "AAC player",
    .plugin.descr = "plays aac files, supports raw aac files, as well as mp4 container",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses modified libmp4ff (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .open = aac_open,
    .init = aac_init,
    .free = aac_free,
    .read = aac_read,
    .seek = aac_seek,
    .seek_sample = aac_seek_sample,
    .insert = aac_insert,
    .read_metadata = mp4_read_metadata,
    .write_metadata = mp4_write_metadata,
    .exts = exts,
};

DB_plugin_t *
aac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
