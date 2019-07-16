/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

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

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "aacdecoder_lib.h"
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <stdlib.h>
#include <math.h>
#include "../../deadbeef.h"
#include "../../strdupa.h"
#include "aac_parser.h"

#include "../../shared/mp4tagutil.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

#define OUT_BUFFER_SIZE 1024*8*2 // AAC frame can be 1024 or 960 samples, up to 8 channels, 2 bytes each
#define AAC_MAX_PACKET_SIZE 768*8 // setting max input packet size, to have some headroom

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
    HANDLE_AACDECODER dec;
    DB_FILE *file;

    mp4p_file_callbacks_t mp4reader;
    mp4p_atom_t *mp4file;
    mp4p_atom_t *trak;
    uint64_t mp4samples;

    int mp4sample;
    int mp4framesize;
    int skipsamples;
    int startsample;
    int endsample;
    int currentsample;

    // buffer with input packet data
    uint8_t buffer[AAC_MAX_PACKET_SIZE];
    int remaining;

    // buffer with decoded samples
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
    aac_info_t *info = calloc (sizeof (aac_info_t), 1);
    return (DB_fileinfo_t *)info;
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
            break;
        }
        bufsize = sizeof (buf);

        int channels, samplerate, bitrate, samples;
        size = aac_sync (buf, &channels, &samplerate, &bitrate, &samples);
        if (size == 0) {
            memmove (buf, buf+1, sizeof (buf)-1);
            bufsize--;
            framepos++;
            continue;
        }
        else {
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
    }
    else {
        int64_t pos = deadbeef->ftell (fp);
        int totalsamples = (double)fsize / (pos-offs) * nsamples;
        *pduration = totalsamples / (float)stream_sr;
    }

    if (*psamplerate <= 24000) {
        *psamplerate *= 2;
        if (ptotalsamples) {
            *ptotalsamples *= 2;
        }
    }
    return firstframepos;
}

// returns -1 for error, 0 for aac
static int
aac_probe (DB_FILE *fp, float *duration, int *samplerate, int *channels, int64_t *totalsamples) {

    deadbeef->rewind (fp);
    if (parse_aac_stream (fp, samplerate, channels, duration, totalsamples) == -1) {
        return -1;
    }
    return 0;
}

static int
_mp4_get_samplerate (aac_info_t *info) {
    CStreamInfo* stream_info = aacDecoder_GetStreamInfo(info->dec);
    if (stream_info->extSamplingRate) {
        info->info.fmt.samplerate = stream_info->extSamplingRate;
    }
    else if (stream_info->sampleRate) {
        info->info.fmt.samplerate = stream_info->sampleRate;
    }
    else if (stream_info->aacSampleRate) {
        info->info.fmt.samplerate = stream_info->aacSampleRate;
    }
    else {
        return -1;
    }
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

    info->mp4reader.ptrhandle = info->file;
    mp4_init_ddb_file_callbacks (&info->mp4reader);
    info->mp4file = mp4p_open(&info->mp4reader);

    int64_t totalsamples = -1;
    float duration = -1;

    if (info->mp4file) {
        mp4p_mp4a_t *aac = NULL;
        info->trak = mp4p_atom_find (info->mp4file, "moov/trak");
        while (info->trak) {
            mp4p_atom_t *aac_atom = mp4p_atom_find (info->trak, "trak/mdia/minf/stbl/stsd/mp4a");
            if (aac_atom) {
                aac = aac_atom->data;
                break;
            }
            info->trak = info->trak->next;
        }

        if (!aac) {
            return -1;
        }

        mp4p_atom_t *stts_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl/stts");
        mp4p_atom_t *mdhd_atom = mp4p_atom_find(info->trak, "trak/mdia/mdhd");

        mp4p_mdhd_t *mdhd = mdhd_atom->data;
        uint64_t total_sample_duration = mp4p_stts_total_sample_duration (stts_atom);
        totalsamples = total_sample_duration * aac->sample_rate / mdhd->time_scale;
        duration = total_sample_duration / (float)mdhd->time_scale;

        mp4p_atom_t *stsz_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl/stsz");
        mp4p_stsz_t *stsz = stsz_atom->data;

        // init mp4 decoding
        info->mp4samples = stsz->number_of_entries;
        info->dec = aacDecoder_Open (TT_MP4_RAW, 1);

        mp4p_atom_t *esds_atom = mp4p_atom_find (info->trak, "trak/mdia/minf/stbl/stsd/mp4a/esds");
        mp4p_esds_t *esds = esds_atom->data;

        uint8_t *asc = (uint8_t *)esds->asc;
        if (aacDecoder_ConfigRaw(info->dec, &asc, &esds->asc_size) != AAC_DEC_OK) {
            return -1;
        }

        if (_mp4_get_samplerate (info) < 0) {
            return -1;
        }
        _info->fmt.channels = aac->channel_count;
    }
    else {
        int samplerate = -1;
        int channels = -1;
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

        _info->fmt.channels = channels;
        _info->fmt.samplerate = samplerate;

        // FIXME: FDK-AAC likes to crash on unsupported/unrecognized data, so need to bail here if ADTS stream is not detected.
        info->dec = aacDecoder_Open(TT_MP4_ADTS, 1);
    }

    _info->fmt.bps = 16;
    _info->plugin = &plugin;

    if (!info->file->vfs->is_streaming ()) {
        if (it->endsample > 0) {
            info->startsample = it->startsample;
            info->endsample = it->endsample;
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
            mp4p_atom_free_list (info->mp4file);
        }
        if (info->dec) {
            aacDecoder_Close (info->dec);
        }
        free (info);
    }
}

static int
aac_read (DB_fileinfo_t *_info, char *bytes, int size) {
    // this needs to persist until aacDecoder_DecodeFrame ends
    unsigned char *mp4packet = NULL;

    aac_info_t *info = (aac_info_t *)_info;
    if (info->eof) {
        return 0;
    }

    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (!info->file->vfs->is_streaming ()) {
        if (info->currentsample + size / samplesize > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * samplesize;
            if (size <= 0) {
                return 0;
            }
        }
    }

    int initsize = size;
    unsigned long consumed = 0;

    while (size > 0) {
        if (info->skipsamples > 0 && info->out_remaining > 0) {
            int skip = min (info->out_remaining, info->skipsamples);
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

        if (info->mp4file) {
            if (info->mp4sample >= info->mp4samples) {
                break;
            }

            mp4p_atom_t *stbl_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl");
            uint64_t offs = mp4p_sample_offset (stbl_atom, info->mp4sample);
            UINT size = mp4p_sample_size (stbl_atom, info->mp4sample);
            printf ("%08X %d\n", (int)offs, size);

            mp4packet = malloc (size);
            deadbeef->fseek (info->file, offs+info->junk, SEEK_SET);
            if (size != deadbeef->fread (mp4packet, 1, size, info->file)) {
                trace ("aac: failed to read sample\n");
                info->eof = 1;
                break;
            }

            info->mp4sample++;

            UINT valid = size;
            AAC_DECODER_ERROR err = aacDecoder_Fill(info->dec, &mp4packet, &size, &valid);

            if (err != AAC_DEC_OK) {
                goto error;
            }
        }
        else {
            uint8_t *packet = info->buffer;
            size_t n = deadbeef->fread (packet, 1, 2, info->file);
            if (n != 2) {
                goto error;
            }
            if (packet[0] != 0xff || (packet[1] & 0xf0) != 0xf0) {
                trace ("aac: ADTS out of sync, resyncing\n");
                if (info->num_errors > 10) {
                    break;
                }
                info->num_errors++;
                info->remaining = 0;
                continue;
            }
            n = deadbeef->fread (packet+2, 1, 5, info->file);
            if (n != 5) {
                goto error;
            }

            info->num_errors=0;
            UINT packet_size = ((packet[3] & 0x03) << 11) | (packet[4] << 3) | (packet[5] >> 5);
            n = deadbeef->fread(packet + 7, 1, packet_size - 7, info->file);
            if (n != packet_size - 7) {
                goto error;
            }
            UINT valid = packet_size;
            AAC_DECODER_ERROR err = aacDecoder_Fill(info->dec, &packet, &packet_size, &valid);
            if (err != AAC_DEC_OK) {
                goto error;
            }
            consumed = info->remaining-valid;
        }

        AAC_DECODER_ERROR err = aacDecoder_DecodeFrame(info->dec, (short *)info->out_buffer, OUT_BUFFER_SIZE/sizeof(short), 0);

        if (mp4packet) {
            free (mp4packet);
            mp4packet = NULL;
        }

        if (consumed == info->remaining) {
            info->remaining = 0;
        }
        else if (consumed > 0) {
            memmove (info->buffer, info->buffer + consumed, info->remaining - consumed);
            info->remaining -= consumed;
        }

        if (err != AAC_DEC_OK) {
            trace ("aacDecoder_DecodeFrame: error %d\n", err);
            break;
        }

        CStreamInfo *frame_info = aacDecoder_GetStreamInfo(info->dec);
        info->out_remaining = (int)frame_info->frameSize;

        if (info->remap[0] == -1) {
            // build remap mtx
            for (int i = 0; i < _info->fmt.channels; i++) {
                AUDIO_CHANNEL_TYPE idx = frame_info->pChannelIndices[i];
                info->remap[idx] = i;
            }
            if (info->remap[0] == -1) {
                info->remap[0] = 0;
            }
            if ((_info->fmt.channels == 1 && info->remap[0] == 0)
                || (_info->fmt.channels == 2 && info->remap[0] == 0 && info->remap[1] == 1)) {
                info->noremap = 1;
            }
        }
    }

    info->currentsample += (initsize-size) / samplesize;

    return initsize-size;

error:
    if (mp4packet) {
        free (mp4packet);
    }
    return -1;
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
            frame++;
            if (deadbeef->fseek (info->file, size-(int)sizeof(buf), SEEK_CUR) == -1) {
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

    sample += info->startsample;
    if (info->mp4file) {
        mp4p_atom_t *mdhd_atom = mp4p_atom_find(info->trak, "trak/mdia/mdhd");
        mp4p_mdhd_t *mdhd = mdhd_atom->data;

        mp4p_atom_t *stts_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl/stts");

        int scale = _info->fmt.samplerate / mdhd->time_scale;
        int pos = 0;
        for (int i = 0; i < info->mp4samples; i++)
        {
            uint32_t thissample_duration = mp4p_stts_sample_duration(stts_atom, i);

            if (pos + thissample_duration > sample / scale) {
                info->skipsamples = sample - pos * scale;
                info->mp4sample = i;
                break;
            }
            pos += thissample_duration;
        }
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
aac_load_itunes_chapters (aac_info_t *info, mp4p_chap_t *chap, /* out */ int *num_chapters, int samplerate) {
    *num_chapters = 0;

    mp4p_atom_t *mp4 = info->mp4file;

    for (int i = 0; i < chap->count; i++)
    {
        mp4p_atom_t *text_atom = NULL;
        mp4p_atom_t *trak_atom = mp4p_atom_find(mp4, "moov/trak");
        while (trak_atom) {
            text_atom = NULL;
            if (!mp4p_atom_type_compare(trak_atom, "trak")) {
                text_atom = mp4p_atom_find(trak_atom, "trak/mdia/minf/stbl/stsd/text");
                mp4p_atom_t *tkhd_atom = mp4p_atom_find(trak_atom, "trak/tkhd");
                if (text_atom && tkhd_atom) {
                    mp4p_tkhd_t *tkhd = tkhd_atom->data;
                    if (tkhd->track_id == chap->track_id[i]) {
                        break;
                    }
                }
            }
            trak_atom = trak_atom->next;
        }

        if (!text_atom) {
            continue;
        }

        mp4p_atom_t *stts_atom = mp4p_atom_find(trak_atom, "trak/mdia/minf/stbl/stts");
        mp4p_atom_t *mdhd_atom = mp4p_atom_find(trak_atom, "trak/mdia/mdhd");
        mp4p_atom_t *stbl_atom = mp4p_atom_find(trak_atom, "trak/mdia/minf/stbl");
        if (!stts_atom || !mdhd_atom || !stbl_atom) {
            continue;
        }

        mp4p_mdhd_t *mdhd = mdhd_atom->data;

        uint64_t sample_count = mp4p_stts_total_num_samples (stts_atom);

        aac_chapter_t *chapters = malloc (sizeof (aac_chapter_t) * sample_count);
        memset (chapters, 0, sizeof (aac_chapter_t) * sample_count);
        *num_chapters = 0;

        int64_t total_dur = 0;
        int64_t curr_sample = 0;
        for (int sample = 0; sample < sample_count; sample++)
        {
            int32_t dur = (int64_t)1000 * mp4p_stts_sample_duration(stts_atom, sample) / mdhd->time_scale; // milliseconds
            total_dur += dur;
            unsigned char *buffer = NULL;
            uint32_t buffer_size = 0;

            uint64_t offs = mp4p_sample_offset (stbl_atom, sample);
            UINT size = mp4p_sample_size (stbl_atom, sample);

            buffer = malloc (size);
            deadbeef->fseek (info->file, offs+info->junk, SEEK_SET);
            if (size != deadbeef->fread (buffer, 1, size, info->file)) {
                free (buffer);
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
            free (buffer);
            (*num_chapters)++;
        }
        return chapters;
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
        it->startsample = chapters[i].startsample;
        it->endsample = chapters[i].endsample;
        deadbeef->pl_replace_meta (it, ":FILETYPE", ftype);
        deadbeef->plt_set_item_duration (plt, it, (float)(it->endsample - it->startsample + 1) / samplerate);
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
    // copy metadata from embedded tags
    uint32_t f = deadbeef->pl_get_item_flags (origin);
    f |= DDB_IS_SUBTRACK;
    deadbeef->pl_set_item_flags (origin, f);
    deadbeef->pl_items_copy_junk (origin, first, after);
    deadbeef->pl_item_unref (first);

    deadbeef->pl_unlock ();
    return after;
}


static int
_mp4_insert(DB_playItem_t **after, const char *fname, DB_FILE *fp, ddb_playlist_t *plt) {
    mp4p_atom_t *mp4 = NULL;
    aac_info_t info = {0};
    info.junk = deadbeef->junk_get_leading_size (fp);
    if (info.junk >= 0) {
        deadbeef->fseek (fp, info.junk, SEEK_SET);
    }
    else {
        info.junk = 0;
    }

    info.file = fp;
    info.mp4reader.ptrhandle = fp;
    mp4_init_ddb_file_callbacks (&info.mp4reader);
    mp4 = info.mp4file = mp4p_open(&info.mp4reader);

    if (!mp4) {
        return -1; // not mp4
    }
    const char *ftype = NULL;
    float duration = -1;

    info.trak = mp4p_atom_find (info.mp4file, "moov/trak");
    mp4p_mp4a_t *aac = NULL;
    while (info.trak) {
        mp4p_atom_t *aac_atom = mp4p_atom_find (info.trak, "trak/mdia/minf/stbl/stsd/mp4a");
        if (aac_atom) {
            aac = aac_atom->data;
            break;
        }
        info.trak = info.trak->next;
    }
    if (!aac) {
        mp4p_atom_free_list(info.mp4file);
        return -1; // mp4 without aac
    }

    // get audio format: samplerate, bps, channels
    mp4p_atom_t *esds_atom = mp4p_atom_find (info.trak, "trak/mdia/minf/stbl/stsd/mp4a/esds");
    mp4p_esds_t *esds = esds_atom->data;

    info.dec = aacDecoder_Open (TT_MP4_RAW, 1);

    uint8_t *asc = (uint8_t *)esds->asc;
    if (aacDecoder_ConfigRaw(info.dec, &asc, &esds->asc_size) != AAC_DEC_OK) {
        mp4p_atom_free_list(info.mp4file);
        return -1;
    }

    if (_mp4_get_samplerate(&info) < 0) {
        mp4p_atom_free_list(info.mp4file);
        aacDecoder_Close(info.dec);
        return -1;
    }
    aacDecoder_Close(info.dec);

    int channels = aac->channel_count;
    int64_t totalsamples = 0;

    mp4p_atom_t *stts_atom = mp4p_atom_find(info.trak, "trak/mdia/minf/stbl/stts");
    mp4p_atom_t *mdhd_atom = mp4p_atom_find(info.trak, "trak/mdia/mdhd");
    mp4p_mdhd_t *mdhd = mdhd_atom->data;

    uint64_t total_sample_duration = mp4p_stts_total_sample_duration (stts_atom);
    totalsamples = total_sample_duration * info.info.fmt.samplerate / mdhd->time_scale;
    duration = total_sample_duration / (float)mdhd->time_scale;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    ftype = "MP4 AAC";
    deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
    deadbeef->plt_set_item_duration (plt, it, duration);

    deadbeef->rewind (fp);
    mp4_read_metadata_file(it, &info.mp4reader);
    (void)deadbeef->junk_apev2_read (it, fp);
    (void)deadbeef->junk_id3v2_read (it, fp);
    (void)deadbeef->junk_id3v1_read (it, fp);

    int64_t fsize = deadbeef->fgetlength (fp);

    char s[100];
    snprintf (s, sizeof (s), "%lld", fsize);
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
    deadbeef->pl_add_meta (it, ":BPS", "16");
    snprintf (s, sizeof (s), "%d", channels);
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", info.info.fmt.samplerate);
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    int br = (int)roundf(fsize / duration * 8 / 1000);
    snprintf (s, sizeof (s), "%d", br);
    deadbeef->pl_add_meta (it, ":BITRATE", s);

    int num_chapters = 0;
    aac_chapter_t *chapters = NULL;

    mp4p_atom_t *chap_atom = mp4p_atom_find(info.trak, "trak/tref/chap");
    if (chap_atom) {
        mp4p_chap_t *chap = chap_atom->data;
        if (chap->count > 0) {
            chapters = aac_load_itunes_chapters (&info, chap, &num_chapters, info.info.fmt.samplerate);
        }
    }

    // embedded chapters
    deadbeef->pl_lock (); // FIXME: the lock can be eliminated, if subtracks are first appended "locally", and only appended to the real playlist at the end
    if (chapters && num_chapters > 0) {
        DB_playItem_t *cue = aac_insert_with_chapters (plt, *after, it, chapters, num_chapters, totalsamples, info.info.fmt.samplerate);
        for (int n = 0; n < num_chapters; n++) {
            if (chapters[n].title) {
                free (chapters[n].title);
            }
        }
        free (chapters);
        if (cue) {
            mp4p_atom_free_list(info.mp4file);
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            deadbeef->pl_unlock ();
            *after = cue;
            return 0;
        }
    }
    deadbeef->pl_unlock ();

    // embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    DB_playItem_t *cue = NULL;

    if (cuesheet) {
        cue = deadbeef->plt_insert_cue_from_buffer (plt, *after, it, (const uint8_t *)cuesheet, (int)strlen (cuesheet), (int)totalsamples, info.info.fmt.samplerate);
        if (cue) {
            mp4p_atom_free_list(info.mp4file);
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            *after = cue;
            return 0;
        }
    }

    cue  = deadbeef->plt_insert_cue (plt, *after, it, (int)totalsamples, info.info.fmt.samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        *after = cue;
        return 0;
    }

    *after = deadbeef->plt_insert_item (plt, *after, it);
    deadbeef->pl_item_unref (it);

    mp4p_atom_free_list(info.mp4file);
    return 0;
}

static DB_playItem_t *
aac_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    if (!fp->vfs->is_streaming ()) {
        int res = _mp4_insert(&after, fname, fp, plt);
        if (res == 0) {
            deadbeef->fclose (fp);
            return after;
        }
    }

    // If mp4 is not detected, try raw aac
    const char *ftype = "RAW AAC";
    int samplerate = 0;
    int channels = 0;
    float duration = -1;
    int64_t totalsamples = 0;
    int res = aac_probe (fp, &duration, &samplerate, &channels, &totalsamples);
    if (res == -1) {
        deadbeef->fclose (fp);
        return NULL;
    }
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
    deadbeef->plt_set_item_duration (plt, it, duration);

    // read tags
    (void)deadbeef->junk_apev2_read (it, fp);
    (void)deadbeef->junk_id3v2_read (it, fp);
    (void)deadbeef->junk_id3v1_read (it, fp);

    int64_t fsize = deadbeef->fgetlength (fp);

    deadbeef->fclose (fp);

    if (duration > 0) {
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
        // embedded cue
        deadbeef->pl_lock ();
        const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
        DB_playItem_t *cue = NULL;

        if (cuesheet) {
            cue = deadbeef->plt_insert_cue_from_buffer (plt, after, it, (uint8_t *)cuesheet, (int)strlen (cuesheet), (int)totalsamples, samplerate);
            if (cue) {
                deadbeef->pl_item_unref (it);
                deadbeef->pl_item_unref (cue);
                deadbeef->pl_unlock ();
                return cue;
            }
        }
        deadbeef->pl_unlock ();

        cue  = deadbeef->plt_insert_cue (plt, after, it, (int)totalsamples, samplerate);
        if (cue) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
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
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 2,
    .plugin.version_minor = 0,
//    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "aac",
    .plugin.name = "AAC player",
    .plugin.descr = "plays aac files, supports raw aac files, as well as mp4 container",
    .plugin.copyright = 
        "AAC DeaDBeeF Player Plugin\n"
        "Copyright (c) 2009-2016 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
        "\n"
        "\n"
        "Software License for The Fraunhofer FDK AAC Codec Library for Android\n"
        "\n"
        "© Copyright  1995 - 2012 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.\n"
        "  All rights reserved.\n"
        "\n"
        "1.    INTRODUCTION\n"
        "The Fraunhofer FDK AAC Codec Library for Android (\"FDK AAC Codec\") is software that implements\n"
        "the MPEG Advanced Audio Coding (\"AAC\") encoding and decoding scheme for digital audio.\n"
        "This FDK AAC Codec software is intended to be used on a wide variety of Android devices.\n"
        "\n"
        "AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual\n"
        "audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by\n"
        "independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part\n"
        "of the MPEG specifications.\n"
        "\n"
        "Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)\n"
        "may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners\n"
        "individually for the purpose of encoding or decoding bit streams in products that are compliant with\n"
        "the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license\n"
        "these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec\n"
        "software may already be covered under those patent licenses when it is used for those licensed purposes only.\n"
        "\n"
        "Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,\n"
        "are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional\n"
        "applications information and documentation.\n"
        "\n"
        "2.    COPYRIGHT LICENSE\n"
        "\n"
        "Redistribution and use in source and binary forms, with or without modification, are permitted without\n"
        "payment of copyright license fees provided that you satisfy the following conditions:\n"
        "\n"
        "You must retain the complete text of this software license in redistributions of the FDK AAC Codec or\n"
        "your modifications thereto in source code form.\n"
        "\n"
        "You must retain the complete text of this software license in the documentation and/or other materials\n"
        "provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.\n"
        "You must make available free of charge copies of the complete source code of the FDK AAC Codec and your\n"
        "modifications thereto to recipients of copies in binary form.\n"
        "\n"
        "The name of Fraunhofer may not be used to endorse or promote products derived from this library without\n"
        "prior written permission.\n"
        "\n"
        "You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec\n"
        "software or your modifications thereto.\n"
        "\n"
        "Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software\n"
        "and the date of any change. For modified versions of the FDK AAC Codec, the term\n"
        "\"Fraunhofer FDK AAC Codec Library for Android\" must be replaced by the term\n"
        "\"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android.\"\n"
        "\n"
        "3.    NO PATENT LICENSE\n"
        "\n"
        "NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,\n"
        "ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with\n"
        "respect to this software.\n"
        "\n"
        "You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized\n"
        "by appropriate patent licenses.\n"
        "\n"
        "4.    DISCLAIMER\n"
        "\n"
        "This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors\n"
        "\"AS IS\" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties\n"
        "of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR\n"
        "CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,\n"
        "including but not limited to procurement of substitute goods or services; loss of use, data, or profits,\n"
        "or business interruption, however caused and on any theory of liability, whether in contract, strict\n"
        "liability, or tort (including negligence), arising in any way out of the use of this software, even if\n"
        "advised of the possibility of such damage.\n"
        "\n"
        "5.    CONTACT INFORMATION\n"
        "\n"
        "Fraunhofer Institute for Integrated Circuits IIS\n"
        "Attention: Audio and Multimedia Departments - FDK AAC LL\n"
        "Am Wolfsmantel 33\n"
        "91058 Erlangen, Germany\n"
        "\n"
        "www.iis.fraunhofer.de/amm\n"
        "amm-info@iis.fraunhofer.de\n"
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
