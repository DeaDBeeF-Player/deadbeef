/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <neaacdec.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <stdlib.h>
#include "../../deadbeef.h"
#include "aac_parser.h"

#ifdef USE_MP4FF
#include "mp4ff/mp4ff.h"
#else
#warning linking mp4v2 to faad2 is illegal
#include <mp4v2/mp4v2.h>
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

#define AAC_BUFFER_SIZE 50000
#define OUT_BUFFER_SIZE 100000

#ifdef USE_MP4FF
#define MP4FILE mp4ff_t *
#define MP4FILE_CB mp4ff_callback_t
#else
#define MP4FILE MP4FileHandle
#define MP4FILE_CB MP4FileProvider
#endif

typedef struct {
    DB_fileinfo_t info;
    NeAACDecHandle dec;
    DB_FILE *file;
    MP4FILE mp4file;
    MP4FILE_CB mp4reader;
    int32_t timescale;
    u_int32_t maxSampleSize;
    int mp4track;
    int mp4samples;
    int mp4sample;
    int mp4framesize;
    int skipsamples;
    int startsample;
    int endsample;
    int currentsample;
    char buffer[AAC_BUFFER_SIZE];
    int remaining;
    int faad_channels;
    char out_buffer[OUT_BUFFER_SIZE];
    int out_remaining;
    int num_errors;
    char *samplebuffer;
} aac_info_t;

// allocate codec control structure
static DB_fileinfo_t *
aac_open (void) {
    DB_fileinfo_t *_info = malloc (sizeof (aac_info_t));
    aac_info_t *info = (aac_info_t *)_info;
    memset (info, 0, sizeof (aac_info_t));
    return _info;
}

#ifdef USE_MP4FF
static uint32_t
aac_fs_read (void *user_data, void *buffer, uint32_t length) {
//    trace ("aac_fs_read %d\n", length);
    DB_FILE *fp = (DB_FILE *)user_data;
    return deadbeef->fread (buffer, 1, length, fp);
}
static uint32_t
aac_fs_seek (void *user_data, uint64_t position) {
//    trace ("aac_fs_seek\n");
    DB_FILE *fp = (DB_FILE *)user_data;
    return deadbeef->fseek (fp, position, SEEK_SET);
}
#else
static void *
aac_fs_open (const char *fname, MP4FileMode mode) {
    return deadbeef->fopen (fname);
}
static int
aac_fs_seek (void* handle, int64_t pos) {
    return deadbeef->fseek ((DB_FILE*)handle, pos, SEEK_SET);
}

static int
aac_fs_read (void *handle, void *buffer, int64_t length, int64_t *nin, int64_t maxChunkSize) {
    if (deadbeef->fread (buffer, length, 1, (DB_FILE*)handle) != 1) {
        return 1;
    }
    *nin = length;
    return 0;
}
static int
aac_fs_close (void *handle) {
    deadbeef->fclose ((DB_FILE*)handle);
    return 1;
}
#endif

static int
parse_aac_stream(DB_FILE *fp, int *psamplerate, int *pchannels, float *pduration, int *ptotalsamples)
{
    size_t framepos = deadbeef->ftell (fp);
    size_t initfpos = framepos;
    int firstframepos = -1;
    int fsize = -1;
    int offs = 0;
    if (!fp->vfs->streaming) {
        int skip = deadbeef->junk_get_leading_size (fp);
        if (skip >= 0) {
            deadbeef->fseek (fp, skip, SEEK_SET);
        }
        int offs = deadbeef->ftell (fp);
        fsize = deadbeef->fgetlength (fp);
        if (skip > 0) {
            fsize -= skip;
        }
    }

    uint8_t buf[ADTS_HEADER_SIZE*8];

    int nsamples = 0;
    int stream_sr = 0;
    int stream_ch = 0;

    int eof = 0;
    int bufsize = 0;
    int remaining = 0;

    int frame = 0;
    int scanframes = 1000;
    if (fp->vfs->streaming) {
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
//            trace ("aac_sync fail, framepos: %d\n", framepos);
            if (deadbeef->ftell (fp) - initfpos > 2000) { // how many is enough to make sure?
                break;
            }
            framepos++;
            continue;
        }
        else {
            trace ("aac: frame #%d sync: %d %d %d %d %d\n", frame, channels, samplerate, bitrate, samples, size);
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
            if (deadbeef->fseek (fp, size-sizeof(buf), SEEK_CUR) == -1) {
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
        int pos = deadbeef->ftell (fp);
        int totalsamples = (double)fsize / (pos-offs) * nsamples;
        *pduration = totalsamples / (float)stream_sr;
        trace ("aac: duration=%f (%d samples @ %d Hz), fsize=%d\n", *pduration, totalsamples, stream_sr, fsize);
    }

    return firstframepos;
}

// returns -1 for error, 0 for mp4, 1 for aac
int
aac_probe (DB_FILE *fp, const char *fname, MP4FILE_CB *cb, float *duration, int *samplerate, int *channels, int *totalsamples, int *mp4track, MP4FILE *pmp4) {
    // try mp4

    if (mp4track) {
        *mp4track = -1;
    }
    if (*pmp4) {
        *pmp4 = NULL;
    }
    *duration = -1;
#ifdef USE_MP4FF
    mp4ff_t *mp4 = mp4ff_open_read (cb);
#else
    MP4FileHandle mp4 = MP4ReadProvider (fname, 0, cb);
#endif
    if (!mp4) {
        trace ("not an mp4 file\n");
        return -1;
    }
    if (pmp4) {
        *pmp4 = mp4;
    }
#ifdef USE_MP4FF
    int ntracks = mp4ff_total_tracks (mp4);
    if (ntracks > 0) {
        trace ("m4a container detected, ntracks=%d\n", ntracks);
        int i = -1;
        trace ("looking for mp4 data...\n");
        for (i = 0; i < ntracks; i++) {
            unsigned char*  buff = 0;
            unsigned int    buff_size = 0;
            mp4AudioSpecificConfig mp4ASC;
            mp4ff_get_decoder_config(mp4, i, &buff, &buff_size);
            if(buff){
                int rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
                free(buff);
                if(rc < 0)
                    continue;
                break;
            }
        }

        if (i != ntracks) 
        {
            trace ("mp4 track: %d\n", i);
            *samplerate = mp4ff_get_sample_rate (mp4, i);
            *channels = mp4ff_get_channel_count (mp4, i);
            int samples = mp4ff_num_samples(mp4, i) * 1024;
            trace ("mp4 nsamples=%d, samplerate=%d\n", samples, *samplerate);
            *duration = (float)samples / (*samplerate);

            if (totalsamples) {
                *totalsamples = samples;
            }
            if (mp4track) {
                *mp4track = i;
            }
            if (!*pmp4) {
                mp4ff_close (mp4);
            }
            return 0;
        }
    }
#else
    MP4FileHandle mp4File = mp4;
    MP4TrackId trackId = MP4FindTrackId(mp4File, 0, "audio", 0);
    trace ("trackid: %d\n", trackId);
    u_int32_t timeScale = MP4GetTrackTimeScale(mp4File, trackId);
    MP4Duration trackDuration = MP4GetTrackDuration(mp4File, trackId);
    MP4SampleId numSamples = MP4GetTrackNumberOfSamples(mp4File, trackId);
    u_int8_t* pConfig;
    u_int32_t configSize = 0;
    bool res = MP4GetTrackESConfiguration(mp4File, trackId, &pConfig, &configSize);
    if (res && pConfig) {
        mp4AudioSpecificConfig mp4ASC;
        int rc = AudioSpecificConfig(pConfig, configSize, &mp4ASC);
        free (pConfig);
        if (rc >= 0) {
            *samplerate = mp4ASC.samplingFrequency;
            *channels = MP4GetTrackAudioChannels (mp4File, trackId);
//            int64_t duration = MP4ConvertFromTrackDuration (mp4File, trackId, trackDuration, timeScale);
            int samples = MP4GetTrackNumberOfSamples (mp4File, trackId) * 1024 * (*channels);
            trace ("mp4 nsamples=%d, timescale=%d, samplerate=%d\n", samples, timeScale, *samplerate);
            *duration = (float)samples / (*samplerate);

            if (totalsamples) {
                *totalsamples = samples;
            }
            if (mp4track) {
                *mp4track = trackId;
            }
            if (!*pmp4) {
                MP4Close (mp4);
            }
            return 0;
        }
    }
#endif
    if (*pmp4) {
        *pmp4 = NULL;
    }

    if (mp4) {
#if USE_MP4FF
        mp4ff_close (mp4);
#else
        MP4Close (mp4);
#endif
        mp4 = NULL;
    }
    trace ("mp4 track not found, looking for aac stream...\n");

    // not an mp4, try raw aac
#if USE_MP4FF
    deadbeef->rewind (fp);
#endif
    if (parse_aac_stream (fp, samplerate, channels, duration, totalsamples) == -1) {
        trace ("aac stream not found\n");
        return -1;
    }
    trace ("found aac stream\n");
    return 1;
}


static int
aac_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    aac_info_t *info = (aac_info_t *)_info;

    info->file = deadbeef->fopen (it->fname);
    if (!info->file) {
        return -1;
    }

    // probe
    float duration = -1;
    int samplerate = -1;
    int channels = -1;
    int totalsamples = -1;

    int offs = -1;
    if (!info->file->vfs->streaming) {
        int skip = deadbeef->junk_get_leading_size (info->file);
        if (skip >= 0) {
            deadbeef->fseek (info->file, skip, SEEK_SET);
        }
        offs  =deadbeef->ftell (info->file);
    }

    info->mp4track = -1;
#if USE_MP4FF
    info->mp4reader.read = aac_fs_read;
    info->mp4reader.write = NULL;
    info->mp4reader.seek = aac_fs_seek;
    info->mp4reader.truncate = NULL;
    info->mp4reader.user_data = info->file;
#else
    info->mp4reader.open = aac_fs_open;
    info->mp4reader.seek = aac_fs_seek;
    info->mp4reader.read = aac_fs_read;
    info->mp4reader.write = NULL;
    info->mp4reader.close = aac_fs_close;
#endif

    if (!info->file->vfs->streaming) {
#ifdef USE_MP4FF
        trace ("aac_init: mp4ff_open_read %s\n", it->fname);
        info->mp4file = mp4ff_open_read (&info->mp4reader);
        if (info->mp4file) {
            int ntracks = mp4ff_total_tracks (info->mp4file);
            if (ntracks > 0) {
                trace ("m4a container detected, ntracks=%d\n", ntracks);
                int i = -1;
                unsigned char*  buff = 0;
                unsigned int    buff_size = 0;
                for (i = 0; i < ntracks; i++) {
                    mp4AudioSpecificConfig mp4ASC;
                    mp4ff_get_decoder_config (info->mp4file, i, &buff, &buff_size);
                    if(buff){
                        int rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
                        if(rc < 0)
                            continue;
                        break;
                    }
                }
                trace ("mp4 probe-buffer size: %d\n", buff_size);

                if (i != ntracks && buff) 
                {
                    trace ("mp4 track: %d\n", i);
                    int samples = mp4ff_num_samples(info->mp4file, i);
                    info->mp4samples = samples;
                    trace ("mp4 mp4samples=%d, nsamples=%d, samplerate=%d\n", samples, samples * 1024, samplerate);
                    totalsamples = samples;
                    info->mp4track = i;

                    // init mp4 decoding
                    info->dec = NeAACDecOpen ();
                    unsigned long srate;
                    unsigned char ch;
                    if (NeAACDecInit2(info->dec, buff, buff_size, &srate, &ch) < 0) {
                        trace ("NeAACDecInit2 returned error\n");
                        return -1;
                    }
                    info->faad_channels = ch;
                    samplerate = srate;
                    channels = ch;
                    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration (info->dec);
                    conf->dontUpSampleImplicitSBR = 1;
                    NeAACDecSetConfiguration (info->dec, conf);
                    mp4AudioSpecificConfig mp4ASC;
                    if (NeAACDecAudioSpecificConfig(buff, buff_size, &mp4ASC) >= 0)
                    {
                        info->mp4framesize = 1024;
                        if (mp4ASC.frameLengthFlag == 1) {
                            info->mp4framesize = 960;
                        }
                        if (mp4ASC.sbr_present_flag == 1) {
                            info->mp4framesize *= 2;
                        }
                    }
                    totalsamples *= info->mp4framesize;
                    duration = (float)totalsamples  / samplerate;
                    free (buff);
                }
                else {
                    mp4ff_close (info->mp4file);
                    info->mp4file = NULL;
                }
            }
            else {
                mp4ff_close (info->mp4file);
                info->mp4file = NULL;
            }
        }
#else
        trace ("aac_init: MP4ReadProvider %s\n", it->fname);
        info->mp4file = MP4ReadProvider (it->fname, 0, &info->mp4reader);
        info->mp4track = MP4FindTrackId(info->mp4file, 0, "audio", 0);
        trace ("aac_init: MP4FindTrackId returned %d\n", info->mp4track);
        if (info->mp4track >= 0) {
            info->timescale = MP4GetTrackTimeScale(info->mp4file, info->mp4track);

            u_int8_t* pConfig;
            u_int32_t configSize = 0;
            bool res = MP4GetTrackESConfiguration(info->mp4file, info->mp4track, &pConfig, &configSize);

            mp4AudioSpecificConfig mp4ASC;
            int rc = AudioSpecificConfig(pConfig, configSize, &mp4ASC);
            if (rc >= 0) {
                _info->samplerate = mp4ASC.samplingFrequency;
                _info->channels = MP4GetTrackAudioChannels (info->mp4file, info->mp4track);
                info->faad_channels = _info->channels;
                totalsamples = MP4GetTrackNumberOfSamples (info->mp4file, info->mp4track) * 1024 * _info->channels;

                // init mp4 decoding
                info->dec = NeAACDecOpen ();
                unsigned long srate;
                unsigned char ch;
                if (NeAACDecInit2(info->dec, pConfig, configSize, &srate, &ch) < 0) {
                    trace ("NeAACDecInit2 returned error\n");
                    return -1;
                }
                info->faad_channels = ch;
                samplerate = srate;
                channels = ch;
                NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration (info->dec);
                conf->dontUpSampleImplicitSBR = 1;
                NeAACDecSetConfiguration (info->dec, conf);
                mp4AudioSpecificConfig mp4ASC;
                if (NeAACDecAudioSpecificConfig(pConfig, configSize, &mp4ASC) >= 0)
                {
                    info->mp4framesize = 1024;
                    if (mp4ASC.frameLengthFlag == 1) {
                        info->mp4framesize = 960;
                    }
                    if (mp4ASC.sbr_present_flag == 1) {
                        info->mp4framesize *= 2;
                    }
                }
                //totalsamples *= info->mp4framesize;
                free (pConfig);
                info->maxSampleSize = MP4GetTrackMaxSampleSize(info->mp4file, info->mp4track);
                info->samplebuffer = malloc (info->maxSampleSize);
                info->mp4sample = 1;
            }
            else {
                MP4Close (info->mp4file);
                info->mp4file = NULL;
            }
        }
        else {
            MP4Close (info->mp4file);
            info->mp4file = NULL;
        }
#endif
        if (!info->mp4file) {
            trace ("mp4 track not found, looking for aac stream...\n");

            // not an mp4, try raw aac
            deadbeef->rewind (info->file);
            if (parse_aac_stream (info->file, &samplerate, &channels, &duration, &totalsamples) == -1) {
                trace ("aac stream not found\n");
                return -1;
            }
            deadbeef->rewind (info->file);
            trace ("found aac stream\n");
        }

        _info->channels = channels;
        _info->samplerate = samplerate;
    }
    else {
        // sync before attempting to init
        int samplerate, channels;
        float duration;
        offs = parse_aac_stream (info->file, &samplerate, &channels, &duration, NULL);
        if (offs < 0) {
            trace ("aac: parse_aac_stream failed\n");
            return -1;
        }
        _info->channels = channels;
        _info->samplerate = samplerate*2;
        trace("parse_aac_stream returned %x\n", offs);
    }
    if (offs >= 0) {
        deadbeef->fseek (info->file, offs, SEEK_SET);
    }
//    duration = (float)totalsamples / samplerate;
//    deadbeef->pl_set_item_duration (it, duration);

    _info->bps = 16;
    _info->plugin = &plugin;

    if (!info->mp4file) {
        //trace ("NeAACDecGetCapabilities\n");
        //unsigned long caps = NeAACDecGetCapabilities();

        trace ("NeAACDecOpen\n");
        info->dec = NeAACDecOpen ();

        info->remaining = deadbeef->fread (info->buffer, 1, AAC_BUFFER_SIZE, info->file);

        NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration (info->dec);
        conf->dontUpSampleImplicitSBR = 1;
        NeAACDecSetConfiguration (info->dec, conf);
        unsigned long srate;
        unsigned char ch;
        trace ("NeAACDecInit\n");
        int consumed = NeAACDecInit (info->dec, info->buffer, info->remaining, &srate, &ch);
        trace ("NeAACDecInit returned samplerate=%d, channels=%d\n", (int)srate, (int)ch);
        if (consumed < 0) {
            trace ("NeAACDecInit returned %d\n", consumed);
            return -1;
        }
        if (consumed > info->remaining) {
            trace ("NeAACDecInit consumed more than available! wtf?\n");
            return -1;
        }
        if (consumed == info->remaining) {
            info->remaining = 0;
        }
        else if (consumed > 0) {
            memmove (info->buffer, info->buffer + consumed, info->remaining - consumed);
            info->remaining -= consumed;
        }
        info->faad_channels = ch;
        _info->channels = ch;
    }

    if (!info->file->vfs->streaming) {
        if (it->endsample > 0) {
            info->startsample = it->startsample;
            info->endsample = it->endsample;
            plugin.seek_sample (_info, 0);
        }
        else {
            info->startsample = 0;
            info->endsample = totalsamples-1;
        }
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
        if (info->mp4file) {
#ifdef USE_MP4FF
            mp4ff_close (info->mp4file);
#else
            MP4Close (info->mp4file);
#endif
        }
        if (info->dec) {
            NeAACDecClose (info->dec);
        }
        free (info);
    }
}

static int
aac_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
    aac_info_t *info = (aac_info_t *)_info;
    int out_ch = min (_info->channels, 2);
    if (!info->file->vfs->streaming) {
        if (info->currentsample + size / (2 * out_ch) > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * 2 * out_ch;
            if (size <= 0) {
                return 0;
            }
        }
    }

    int initsize = size;
    int eof = 0;
    int sample_size = out_ch * (_info->bps >> 3);

    while (size > 0) {
        if (info->skipsamples > 0 && info->out_remaining > 0) {
            int skip = min (info->out_remaining, info->skipsamples);
            if (skip < info->out_remaining) {
                memmove (info->out_buffer, info->out_buffer + skip * 2 * info->faad_channels, (info->out_remaining - skip) * 2 * info->faad_channels);
            }
            info->out_remaining -= skip;
            info->skipsamples -= skip;
        }
        if (info->out_remaining > 0) {
            int n = size / sample_size;
            n = min (info->out_remaining, n);

            char *src = info->out_buffer;
            for (int i = 0; i < n; i++) {
                memcpy (bytes, src, sample_size);
                bytes += sample_size;
                src += info->faad_channels * 2;
            }

            size -= n * sample_size;
            if (n == info->out_remaining) {
                info->out_remaining = 0;
            }
            else {
                memmove (info->out_buffer, src, (info->out_remaining - n) * info->faad_channels * 2);
                info->out_remaining -= n;
            }
            continue;
        }

        char *samples = NULL;
        NeAACDecFrameInfo frame_info;

        if (info->mp4file) {
            unsigned char *buffer = NULL;
            int buffer_size = 0;
#ifdef USE_MP4FF
            int rc = mp4ff_read_sample (info->mp4file, info->mp4track, info->mp4sample, &buffer, &buffer_size);
            if (rc == 0) {
                break;
            }
#else

            buffer = info->samplebuffer;
            buffer_size = info->maxSampleSize;
            MP4Timestamp sampleTime;
            MP4Duration sampleDuration;
            MP4Duration sampleRenderingOffset;
            bool isSyncSample;
            MP4ReadSample (info->mp4file, info->mp4track, info->mp4sample, &buffer, &buffer_size, &sampleTime, &sampleDuration, &sampleRenderingOffset, &isSyncSample);
            // convert timestamp and duration from track time to milliseconds
            u_int64_t myTime = MP4ConvertFromTrackTimestamp (info->mp4file, info->mp4track, 
                    sampleTime, MP4_MSECS_TIME_SCALE);

            u_int64_t myDuration = MP4ConvertFromTrackDuration (info->mp4file, info->mp4track,
                    sampleDuration, MP4_MSECS_TIME_SCALE);
#endif
            if (info->mp4sample >= info->mp4samples) {
                break;
            }
            info->mp4sample++;
            samples = NeAACDecDecode(info->dec, &frame_info, buffer, buffer_size);
#ifdef USE_MP4FF
            free (buffer);
#endif
            if (!samples) {
                break;
            }
        }
        else {
            if (info->remaining < AAC_BUFFER_SIZE) {
                size_t res = deadbeef->fread (info->buffer + info->remaining, 1, AAC_BUFFER_SIZE-info->remaining, info->file);
                if (res == 0) {
                    eof = 1;
                }
                info->remaining += res;
            }

            samples = NeAACDecDecode (info->dec, &frame_info, info->buffer, info->remaining);
            if (!samples) {
                trace ("NeAACDecDecode failed, consumed=%d\n", frame_info.bytesconsumed);
                if (info->num_errors > 10) {
                    trace ("NeAACDecDecode failed 10 times, interrupting\n");
                    break;
                }
                info->num_errors++;
                info->remaining = 0;
                continue;
            }
            info->num_errors=0;
            int consumed = frame_info.bytesconsumed;
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

        if (frame_info.samples > 0) {
            memcpy (info->out_buffer, samples, frame_info.samples * 2);
            info->out_remaining = frame_info.samples / frame_info.channels;
        }
    }

    info->currentsample += (initsize-size) / sample_size;
    return initsize-size;
}

// returns -1 on error, 0 on success
int
seek_raw_aac (aac_info_t *info, int sample) {
    deadbeef->rewind (info->file);
    int skip = deadbeef->junk_get_leading_size (info->file);
    if (skip >= 0) {
        deadbeef->fseek (info->file, skip, SEEK_SET);
    }

    int offs = deadbeef->ftell (info->file);
    uint8_t buf[ADTS_HEADER_SIZE*8];

    int nsamples = 0;
    int stream_sr = 0;
    int stream_ch = 0;

    int eof = 0;
    int bufsize = 0;
    int remaining = 0;

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
            //trace ("aac: frame #%d sync: %d %d %d %d %d\n", frame, channels, samplerate, bitrate, samples, size);
            frame++;
            if (deadbeef->fseek (info->file, size-sizeof(buf), SEEK_CUR) == -1) {
                trace ("seek_raw_aac: invalid seek %d\n", size-sizeof(buf));
                break;
            }
            bufsize = 0;
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

    sample += info->startsample;
    if (info->mp4file) {
        info->mp4sample = sample / (info->mp4framesize-1);
        info->skipsamples = sample - info->mp4sample * (info->mp4framesize-1);
    }
    else {
        int res = seek_raw_aac (info, sample);
        if (res < 0) {
            return -1;
        }
        info->skipsamples = res;
    }
    info->remaining = 0;
    info->out_remaining = 0;
    info->currentsample = sample;
    _info->readpos = (float)(info->currentsample - info->startsample) / _info->samplerate;
    return 0;
}

static int
aac_seek (DB_fileinfo_t *_info, float t) {
    return aac_seek_sample (_info, t * _info->samplerate);
}

#ifdef USE_MP4FF
static const char *metainfo[] = {
    "artist", "artist",
    "title", "title",
    "album", "album",
    "track", "track",
    "date", "year",
    "genre", "genre",
    "comment", "comment",
    "performer", "performer",
    "album_artist", "band",
    "writer", "composer",
    "vendor", "vendor",
    "disc", "disc",
    "compilation", "compilation",
    "totaldiscs", "numdiscs",
    "copyright", "copyright",
    "totaltracks", "numtracks",
    "tool", "tool",
    NULL
};


/* find a metadata item by name */
/* returns 0 if item found, 1 if no such item */
int32_t mp4ff_meta_find_by_name(const mp4ff_t *f, const char *item, char **value);


void
aac_load_tags (DB_playItem_t *it, mp4ff_t *mp4) {
    char *s;
    for (int i = 0; metainfo[i]; i += 2) {
        if (mp4ff_meta_find_by_name(mp4, metainfo[i], &s)) {
            deadbeef->pl_add_meta (it, metainfo[i+1], s);
            free (s);
        }
    }
    it->replaygain_track_gain = 0;
    it->replaygain_track_peak = 1;
    it->replaygain_album_gain = 0;
    it->replaygain_album_peak = 1;
    if (mp4ff_meta_find_by_name(mp4, "replaygain_track_gain", &s)) {
        it->replaygain_track_gain = atof (s);
    }
    if (mp4ff_meta_find_by_name(mp4, "replaygain_track_peak", &s)) {
        it->replaygain_track_peak = atof (s);
    }
    if (mp4ff_meta_find_by_name(mp4, "replaygain_album_gain", &s)) {
        it->replaygain_album_gain = atof (s);
    }
    if (mp4ff_meta_find_by_name(mp4, "replaygain_album_peak", &s)) {
        it->replaygain_album_peak = atof (s);
    }
    deadbeef->pl_add_meta (it, "title", NULL);
}
#endif


int
aac_read_metadata (DB_playItem_t *it) {
#ifdef USE_MP4FF
    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }

    if (fp->vfs->streaming) {
        deadbeef->fclose (fp);
        return -1;
    }

    MP4FILE_CB cb = {
        .read = aac_fs_read,
        .write = NULL,
        .seek = aac_fs_seek,
        .truncate = NULL,
        .user_data = fp
    };

    deadbeef->pl_delete_all_meta (it);

    mp4ff_t *mp4 = mp4ff_open_read (&cb);
    if (mp4) {
        aac_load_tags (it, mp4);
        mp4ff_close (mp4);
    }
    else {
        /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
        /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
        /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
        deadbeef->pl_add_meta (it, "title", NULL);
    }
    deadbeef->fclose (fp);
#endif
}

static DB_playItem_t *
aac_insert (DB_playItem_t *after, const char *fname) {
    trace ("adding %s\n", fname);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("not found\n");
        return NULL;
    }

    const char *ftype = NULL;
    float duration = -1;
    int totalsamples = 0;
    int samplerate = 0;

    int mp4track = -1;
    MP4FILE mp4 = NULL;

    if (fp->vfs->streaming) {
        trace ("streaming aac (%s)\n", fname);
        ftype = plugin.filetypes[0];
    }
    else {
        int skip = deadbeef->junk_get_leading_size (fp);
        if (skip >= 0) {
            deadbeef->fseek (fp, skip, SEEK_SET);
        }

        int channels;

        // slowwww!
        MP4FILE_CB cb = {
#ifdef USE_MP4FF
            .read = aac_fs_read,
            .write = NULL,
            .seek = aac_fs_seek,
            .truncate = NULL,
            .user_data = fp
#else
            .open = aac_fs_open,
            .seek = aac_fs_seek,
            .read = aac_fs_read,
            .write = NULL,
            .close = aac_fs_close
#endif
        };

        int res = aac_probe (fp, fname, &cb, &duration, &samplerate, &channels, &totalsamples, &mp4track, &mp4);
        if (res == -1) {
            deadbeef->fclose (fp);
            return NULL;
        }
        else if (res == 0) {
            ftype = plugin.filetypes[1];
        }
        else if (res == 1) {
            ftype = plugin.filetypes[0];
        }
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = ftype;
    deadbeef->pl_set_item_duration (it, duration);
//    trace ("duration: %f sec\n", duration);

    // read tags
    if (mp4) {
#ifdef USE_MP4FF
        aac_load_tags (it, mp4);
        mp4ff_close (mp4);
#else
        const MP4Tags *tags = MP4TagsAlloc ();
        MP4TagsFetch (tags, mp4);

        deadbeef->pl_add_meta (it, "title", tags->name);
        deadbeef->pl_add_meta (it, "artist", tags->artist);
        deadbeef->pl_add_meta (it, "albumArtist", tags->albumArtist);
        deadbeef->pl_add_meta (it, "album", tags->album);
        deadbeef->pl_add_meta (it, "composer", tags->composer);
        deadbeef->pl_add_meta (it, "comments", tags->comments);
        deadbeef->pl_add_meta (it, "genre", tags->genre);
        deadbeef->pl_add_meta (it, "year", tags->releaseDate);
        char s[10];
        if (tags->track) {
            snprintf (s, sizeof (s), "%d", tags->track->index);
            deadbeef->pl_add_meta (it, "track", s);
            snprintf (s, sizeof (s), "%d", tags->track->total);
            deadbeef->pl_add_meta (it, "numtracks", s);
        }
        if (tags->disk) {
            snprintf (s, sizeof (s), "%d", tags->disk->index);
            deadbeef->pl_add_meta (it, "disc", s);
            snprintf (s, sizeof (s), "%d", tags->disk->total);
            deadbeef->pl_add_meta (it, "numdiscs", s);
        }
        deadbeef->pl_add_meta (it, "copyright", tags->copyright);
        deadbeef->pl_add_meta (it, "vendor", tags->encodedBy);
        deadbeef->pl_add_meta (it, "title", NULL);
        MP4TagsFree (tags);
        MP4Close (mp4);
#endif
    }
    else if (ftype == "aac") {
        int apeerr = deadbeef->junk_apev2_read (it, fp);
        int v2err = deadbeef->junk_id3v2_read (it, fp);
        int v1err = deadbeef->junk_id3v1_read (it, fp);
        deadbeef->pl_add_meta (it, "title", NULL);
    }

    deadbeef->fclose (fp);

    if (duration > 0) {
        // embedded cue
        deadbeef->pl_lock ();
        const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
        DB_playItem_t *cue = NULL;

        if (cuesheet) {
            cue = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, samplerate);
            if (cue) {
                deadbeef->pl_item_unref (it);
                deadbeef->pl_item_unref (cue);
                deadbeef->pl_unlock ();
                return cue;
            }
        }
        deadbeef->pl_unlock ();

        cue  = deadbeef->pl_insert_cue (after, it, totalsamples, samplerate);
        if (cue) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            return cue;
        }
    }

    deadbeef->pl_add_meta (it, "title", NULL);

    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static const char * exts[] = { "aac", "mp4", "m4a", NULL };
static const char *filetypes[] = { "RAW AAC", "MP4 AAC", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "aac",
    .plugin.name = "AAC decoder based on FAAD2",
    .plugin.descr = "aac (m4a, mp4, ...)  player",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = aac_open,
    .init = aac_init,
    .free = aac_free,
    .read_int16 = aac_read_int16,
    .seek = aac_seek,
    .seek_sample = aac_seek_sample,
    .insert = aac_insert,
    .read_metadata = aac_read_metadata,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
aac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
