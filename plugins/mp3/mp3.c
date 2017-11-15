/*
    MPEG decoder plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Alexey Yakovenko

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
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include "../../deadbeef.h"
#include "mp3.h"
#ifdef USE_LIBMAD
#include "mp3_mad.h"
#endif
#ifdef USE_LIBMPG123
#include "mp3_mpg123.h"
#endif

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

//#define WRITE_DUMP 1

#if WRITE_DUMP
FILE *out;
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

static uint32_t
extract_i32 (unsigned char *buf)
{
    uint32_t x;
    // big endian extract

    x = buf[0];
    x <<= 8;
    x |= buf[1];
    x <<= 8;
    x |= buf[2];
    x <<= 8;
    x |= buf[3];

    return x;
}

// useful for parsing lame header, but unused right now
#if 0
static inline uint32_t
extract_i32_le (unsigned char *buf)
{
    uint32_t x;
    // little endian extract

    x = buf[3];
    x <<= 8;
    x |= buf[2];
    x <<= 8;
    x |= buf[1];
    x <<= 8;
    x |= buf[0];

    return x;
}
static inline uint16_t
extract_i16 (unsigned char *buf)
{
    uint16_t x;
    // big endian extract

    x = buf[0];
    x <<= 8;
    x |= buf[1];
    x <<= 8;

    return x;
}

static inline float
extract_f32 (unsigned char *buf) {
    float f;
    uint32_t *x = (uint32_t *)&f;
    *x = buf[0];
    *x <<= 8;
    *x |= buf[1];
    *x <<= 8;
    *x |= buf[2];
    *x <<= 8;
    *x |= buf[3];
    return f;
}
#endif

static int
mp3_check_xing_header (buffer_t *buffer, int packetlength, int sample, int samples_per_frame, int samplerate, int64_t framepos, int64_t fsize) {
    const char xing[] = "Xing";
    const char info[] = "Info";
    char magic[4];

    // ignore mpeg version, and try both 17 and 32 byte offsets
    deadbeef->fseek (buffer->file, 17, SEEK_CUR);

    if (deadbeef->fread (magic, 1, 4, buffer->file) != 4) {
        trace ("cmp3_scan_stream: EOF while checking for Xing header\n");
        return -1; // EOF
    }

    if (strncmp (xing, magic, 4) && strncmp (info, magic, 4)) {
        deadbeef->fseek (buffer->file, 11, SEEK_CUR);
        if (deadbeef->fread (magic, 1, 4, buffer->file) != 4) {
            trace ("cmp3_scan_stream: EOF while checking for Xing header\n");
            return -1; // EOF
        }
    }

    if (!strncmp (xing, magic, 4) || !strncmp (info, magic, 4)) {
        trace ("xing/info frame found\n");
        buffer->startoffset += packetlength;
        // read flags
        uint32_t flags;
        uint8_t buf[4];
        if (deadbeef->fread (buf, 1, 4, buffer->file) != 4) {
            trace ("cmp3_scan_stream: EOF while parsing Xing header\n");
            return -1; // EOF
        }
        flags = extract_i32 (buf);
        if (flags & FRAMES_FLAG) {
            // read number of frames
            if (deadbeef->fread (buf, 1, 4, buffer->file) != 4) {
                trace ("cmp3_scan_stream: EOF while parsing Xing header\n");
                return -1; // EOF
            }
            uint32_t nframes = extract_i32 (buf);
            if (sample <= 0) {
                buffer->duration = (((uint64_t)nframes * (uint64_t)samples_per_frame) - buffer->delay - buffer->padding)/ (float)samplerate;
            }
            trace ("xing totalsamples: %d, nframes: %d, samples_per_frame: %d\n", nframes*samples_per_frame, nframes, samples_per_frame);
            if (nframes <= 0 || samples_per_frame <= 0) {
                trace ("bad xing header\n");
                return -1;
            }
            buffer->totalsamples = nframes * samples_per_frame;
            buffer->samplerate = samplerate;
        }
        if (flags & BYTES_FLAG) {
            deadbeef->fseek (buffer->file, 4, SEEK_CUR);
        }
        if (flags & TOC_FLAG) {
            deadbeef->fseek (buffer->file, 100, SEEK_CUR);
        }
        if (flags & VBR_SCALE_FLAG) {
            deadbeef->fseek (buffer->file, 4, SEEK_CUR);
        }
        // lame header
        if (deadbeef->fread (buf, 1, 4, buffer->file) != 4) {
            trace ("cmp3_scan_stream: EOF while reading LAME header\n");
            return -1; // EOF
        }

        deadbeef->fseek (buffer->file, 5, SEEK_CUR);
        uint8_t rev = 0;
        if (deadbeef->fread (&rev, 1, 1, buffer->file) != 1) {
            trace ("cmp3_scan_stream: EOF while reading info tag revision / vbr method\n");
        }
        switch (rev & 0x0f) {
        case XING_CBR ... XING_ABR2:
            buffer->vbr = rev & 0x0f;
            break;
            break;
        }
        if (!memcmp (buf, "LAME", 4)) {
            trace ("lame header found\n");

            // FIXME: that can be optimized with a single read
            // FIXME: add error handling
            uint8_t lpf;
            deadbeef->fread (&lpf, 1, 1, buffer->file);
            //3 floats: replay gain
            deadbeef->fread (buf, 1, 4, buffer->file);
            // float rg_peaksignalamp = extract_f32 (buf);
            deadbeef->fread (buf, 1, 2, buffer->file);
            // uint16_t rg_radio = extract_i16 (buf);

            deadbeef->fread (buf, 1, 2, buffer->file);
            // uint16_t rg_audiophile = extract_i16 (buf);

            // skip
            deadbeef->fseek (buffer->file, 2, SEEK_CUR);
            deadbeef->fread (buf, 1, 3, buffer->file);

            buffer->delay = (((uint32_t)buf[0]) << 4) | ((((uint32_t)buf[1]) & 0xf0)>>4);
            buffer->padding = ((((uint32_t)buf[1])&0x0f)<<8) | ((uint32_t)buf[2]);
            // skip
            deadbeef->fseek (buffer->file, 1, SEEK_CUR);
            // mp3gain
            uint8_t mp3gain;
            deadbeef->fread (&mp3gain, 1, 1, buffer->file);
            // lame preset nr
            uint8_t lamepreset_bytes[2];
            deadbeef->fread (lamepreset_bytes, 2, 1, buffer->file);
            buffer->lamepreset = lamepreset_bytes[1] | (lamepreset_bytes[0] << 8);
            // musiclen
            deadbeef->fread (buf, 1, 4, buffer->file);
            trace ("lame totalsamples: %d\n", buffer->totalsamples);
        }
    }

    return 0;
}

static void
_scan_init (buffer_t *buffer, int sample) {
    buffer->currentsample = 0;
    buffer->skipsamples = 0;
    buffer->samplerate = 0;

    if (sample <= 0) { // rescanning the stream, reset the xing header flag
        buffer->have_xing_header = 0;
    }

    if (sample <= 0) {
        buffer->totalsamples = 0;
    }
    if (sample <= 0 && buffer->avg_packetlength == 0) {
        buffer->avg_packetlength = 0;
        buffer->avg_samplerate = 0;
        buffer->avg_samples_per_frame = 0;
        buffer->nframes = 0;
        int64_t initpos = deadbeef->ftell (buffer->file);
        trace ("scan initpos: %lld\n", initpos);
        buffer->startoffset = initpos;
    }
}

typedef struct {
    int samplerate;
    int bitrate;
    int nchannels;
    int samples_per_frame;
    int ver;
    int layer;
    int packetlength;
} mpeg_frame_info_t;

// returns the new `offs` (header position)
// if the frame is invalid, returns -1
// if EOF reached, returns -2
static int64_t
_scan_mpeg_header (buffer_t *buffer, int64_t offs, int64_t fsize, mpeg_frame_info_t * restrict mpeg_frame) {
    uint32_t hdr;
    uint8_t sync;
    uint8_t fb[4]; // just the header
    if (deadbeef->fread (fb, 1, sizeof(fb), buffer->file) != sizeof(fb)) {
        return -2;
    }

    sync = fb[0];
    if (sync != 0xff) {
        return -1;
    }
    else {
        // 2nd sync byte
        sync = fb[1];
        if ((sync >> 5) != 7) {
            return -1;
        }
    }
    // found frame
    hdr = (0xff<<24) | (sync << 16);
    sync = fb[2];
    hdr |= sync << 8;
    sync = fb[3];
    hdr |= sync;

    // parse header

    // sync bits
    int usync = hdr & 0xffe00000;
    if (usync != 0xffe00000) {
        fprintf (stderr, "fatal error: mp3 header parser is broken\n");
    }

    // mpeg version
    static const int vertbl[] = {3, -1, 2, 1}; // 3 is 2.5
    mpeg_frame->ver = (hdr & (3<<19)) >> 19;
    mpeg_frame->ver = vertbl[mpeg_frame->ver];
    if (mpeg_frame->ver < 0) {
//        trace ("frame %d bad mpeg version %d\n", nframe, (hdr & (3<<19)) >> 19);
        return -1;
    }

    // layer info
    static const int ltbl[] = { -1, 3, 2, 1 };
    mpeg_frame->layer = (hdr & (3<<17)) >> 17;
    mpeg_frame->layer = ltbl[mpeg_frame->layer];
    if (mpeg_frame->layer < 0) {
//        trace ("frame %d bad layer %d\n", nframe, (hdr & (3<<17)) >> 17);
        return -1;
    }

    // protection bit (crc)
    //        int prot = (hdr & (1<<16)) >> 16;

    // bitrate
    static const int brtable[5][16] = {
        { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 },
        { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1 },
        { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1 },
        { 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, -1 },
        { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 }
    };
    mpeg_frame->bitrate = (hdr & (0x0f<<12)) >> 12;
    int idx = 0;
    if (mpeg_frame->ver == 1) {
        idx = mpeg_frame->layer - 1;
    }
    else {
        idx = mpeg_frame->layer == 1 ? 3 : 4;
    }
    mpeg_frame->bitrate = brtable[idx][mpeg_frame->bitrate];
    if (mpeg_frame->bitrate <= 0) {
//        trace ("frame %d bad bitrate %d\n", nframe, (hdr & (0x0f<<12)) >> 12);
        return -1;
    }

    // samplerate
    static const int srtable[3][4] = {
        {44100, 48000, 32000, -1},
        {22050, 24000, 16000, -1},
        {11025, 12000, 8000, -1},
    };
    mpeg_frame->samplerate = (hdr & (0x03<<10))>>10;
    mpeg_frame->samplerate = srtable[mpeg_frame->ver-1][mpeg_frame->samplerate];
    if (mpeg_frame->samplerate < 0) {
//        trace ("frame %d bad samplerate %d\n", nframe, (hdr & (0x03<<10))>>10);
        return -1;
    }

    // padding
    int padding = (hdr & (0x1 << 9)) >> 9;

    static const int chantbl[4] = { 2, 2, 2, 1 };
    mpeg_frame->nchannels = (hdr & (0x3 << 6)) >> 6;
    mpeg_frame->nchannels = chantbl[mpeg_frame->nchannels];

    // check if channel/bitrate combination is valid for layer2
    if (mpeg_frame->layer == 2) {
        if ((mpeg_frame->bitrate <= 56 || mpeg_frame->bitrate == 80) && mpeg_frame->nchannels != 1) {
//            trace ("mp3: bad frame %d: layer %d, channels %d, bitrate %d\n", nframe, layer, nchannels, bitrate);
            return -1;
        }
        if (mpeg_frame->bitrate >= 224 && mpeg_frame->nchannels == 1) {
//            trace ("mp3: bad frame %d: layer %d, channels %d, bitrate %d\n", nframe, layer, nchannels, bitrate);
            return -1;
        }
    }
    // }}}

    // {{{ get side info ptr
    //        uint8_t *si = fb + 4;
    //        if (prot) {
    //            si += 2;
    //        }
    //        int data_ptr = ((uint16_t)si[1]) | (((uint16_t)si[1]&0)<<8);
    // }}}

    // {{{ calc packet length, number of samples in a frame
    // packetlength
    mpeg_frame->packetlength = 0;
    mpeg_frame->bitrate *= 1000;
    mpeg_frame->samples_per_frame = 0;
    if (mpeg_frame->samplerate > 0 && mpeg_frame->bitrate > 0) {
        if (mpeg_frame->layer == 1) {
            mpeg_frame->samples_per_frame = 384;
        }
        else if (mpeg_frame->layer == 2) {
            mpeg_frame->samples_per_frame = 1152;
        }
        else if (mpeg_frame->layer == 3) {
            if (mpeg_frame->ver == 1) {
                mpeg_frame->samples_per_frame = 1152;
            }
            else {
                mpeg_frame->samples_per_frame = 576;
            }
        }
        mpeg_frame->packetlength = mpeg_frame->samples_per_frame / 8 * mpeg_frame->bitrate / mpeg_frame->samplerate + padding;

        // stop if the packet size is larger than remaining data
        if (fsize >= 0 && offs + mpeg_frame->packetlength > fsize) {
            return -2;
        }
        //            if (sample > 0) {
        //                printf ("frame: %d, crc: %d, layer: %d, bitrate: %d, samplerate: %d, filepos: 0x%llX, dataoffs: 0x%X, size: 0x%X\n", nframe, prot, layer, bitrate, samplerate, deadbeef->ftell (buffer->file)-8, data_ptr, packetlength);
        //            }
    }
    else {
///        trace ("frame %d samplerate or bitrate is invalid\n", nframe);
        return -1;
    }
    return offs + mpeg_frame->packetlength;
}

// sample=-1: scan entire stream, calculate precise duration
// sample=0: read headers/tags, calculate approximate duration
// sample>0: seek to the frame with the sample, update skipsamples
// return value: -1 on error
static int
cmp3_scan_stream (buffer_t *buffer, int sample) {
    trace ("cmp3_scan_stream %d (offs: %lld)\n", sample, deadbeef->ftell (buffer->file));

    _scan_init (buffer, sample);
    int lastframe_valid = 0;
    int64_t offs = -1;
    int nframe = 0;
    int scansamples = 0;
    int valid_frames = 0;
    int prev_bitrate = -1;
    int64_t valid_frame_pos = -1;
    // this flag is used to make sure we only check the 1st frame for xing info
    int checked_xing_header = buffer->have_xing_header;
    int reached_eof = 0;

    int64_t fsize = deadbeef->fgetlength (buffer->file) - buffer->endoffset;

    if (fsize < 0) {
        buffer->duration = -1;
    }

    int64_t lead_in_frame_pos = buffer->startoffset;
    int64_t lead_in_frame_no = 0;

#define MAX_LEAD_IN_FRAMES 10
    int64_t frame_positions[MAX_LEAD_IN_FRAMES]; // positions of nframe-9, nframe-8, nframe-7, ...
    for (int i = 0; i < MAX_LEAD_IN_FRAMES; i++) {
        frame_positions[i] = buffer->startoffset;
    }

    for (;;) {
        if (offs <= 0) {
            offs = deadbeef->ftell (buffer->file);
        }

        if (!buffer->file->vfs->is_streaming () && offs >= fsize) {
            reached_eof = 1;
            goto end_scan;
        }

        deadbeef->fseek (buffer->file, offs, SEEK_SET);

        mpeg_frame_info_t frame;
        int64_t framepos = offs;
        int64_t new_offs = _scan_mpeg_header(buffer, offs, fsize, &frame);
        if (new_offs == -1) {
            valid_frame_pos = -1;
            lastframe_valid = 0;
            if (sample == 0) {
                valid_frames = 0;
            }
            offs++;
            continue;
        }
        // end of file?
        if (new_offs == -2) {
            if (!valid_frames) {
                break;
            }
            goto end_scan;
        }
        if (valid_frame_pos == -1) {
            valid_frame_pos = framepos;
        }
        offs = new_offs;
// {{{ vbr adjustement
        if (buffer->vbr != DETECTED_VBR && (!buffer->have_xing_header || !buffer->vbr) && prev_bitrate != -1 && prev_bitrate != frame.bitrate) {
            buffer->vbr = DETECTED_VBR;
        }
        prev_bitrate = frame.bitrate;
// }}}

        valid_frames++;

        trace ("valid: %d, sr: %d, ch: %d\n", valid_frames, frame.samplerate, frame.nchannels);

// {{{ update stream parameters, only when sample!=0 or 1st frame
        if (sample != 0 || nframe == 0)
        {
            if (sample == 0 && lastframe_valid) {
                return 0;
            }
            // don't get parameters from frames coming after any bad frame
            buffer->version = frame.ver;
            buffer->layer = frame.layer;
            buffer->bitrate = frame.bitrate;
            buffer->samplerate = frame.samplerate;
            buffer->packetlength = frame.packetlength;
            if (frame.nchannels > buffer->channels) {
                buffer->channels = frame.nchannels;
            }
//            trace ("frame %d mpeg v%d layer %d bitrate %d samplerate %d packetlength %d channels %d\n", nframe, ver, layer, bitrate, samplerate, packetlength, nchannels);
        }
// }}}

        lastframe_valid = 1;

        // allow at least 10 lead-in frames, to fill bit-reservoir
        if (nframe - lead_in_frame_no > MAX_LEAD_IN_FRAMES) {
            lead_in_frame_pos = frame_positions[0];
            lead_in_frame_no++;
        }
        memmove (frame_positions, &frame_positions[1], sizeof (int64_t) * (MAX_LEAD_IN_FRAMES-1));
        frame_positions[MAX_LEAD_IN_FRAMES-1] = framepos;

// {{{ detect/load xing frame, only on 1st pass
        // try to read xing/info tag (only on initial scans)
        if (sample <= 0 && !buffer->have_xing_header && !checked_xing_header)
        {
            checked_xing_header = 1;
            mp3_check_xing_header (buffer, frame.packetlength, sample, frame.samples_per_frame, frame.samplerate, framepos, fsize);

            if (buffer->have_xing_header) {
                // trust the xing header -- even if requested to scan for precise duration
                buffer->startoffset = offs;
                if (sample <= 0) {
                    // parameters have been discovered from xing header, no need to continue
                    deadbeef->fseek (buffer->file, buffer->startoffset, SEEK_SET);
                    return 0;
                }
                else {
                    // skip to the next frame
                    continue;
                }
            }
        }
// }}}

        if (sample == 0) {
// {{{ update averages, interrupt scan on frame #100
            // calculating apx duration based on 1st 100 frames
            buffer->avg_packetlength += frame.packetlength;
            buffer->avg_samplerate += frame.samplerate;
            buffer->avg_samples_per_frame += frame.samples_per_frame;

            buffer->version = frame.ver;
            buffer->layer = frame.layer;
            buffer->bitrate = frame.bitrate;
            buffer->samplerate = frame.samplerate;
            buffer->packetlength = frame.packetlength;
            if (frame.nchannels > buffer->channels) {
                buffer->channels = frame.nchannels;
            }

            if (buffer->file->vfs->is_streaming ()) {
                if (valid_frames >= 20) {
                    deadbeef->fseek (buffer->file, valid_frame_pos, SEEK_SET);
                    buffer->duration = -1;
                    buffer->totalsamples = 0;
                    return 0;
                }
            }
            else if (valid_frames >= 100) {
                deadbeef->fseek (buffer->file, valid_frame_pos, SEEK_SET);
                goto end_scan;
            }
// }}}
        }
        else {
            // seeking to particular sample, interrupt if reached
            if (sample > 0 && scansamples + frame.samples_per_frame >= sample) {
                deadbeef->fseek (buffer->file, lead_in_frame_pos, SEEK_SET);
                buffer->lead_in_frames = (int)(nframe-lead_in_frame_no);
                buffer->currentsample = sample;
                buffer->skipsamples = sample - scansamples;
                trace ("scan: cursample=%d, frame: %d, skipsamples: %d, filepos: %llX, lead-in frames: %d\n", buffer->currentsample, nframe, buffer->skipsamples, deadbeef->ftell (buffer->file), buffer->lead_in_frames);
                return 0;
            }
        }
        scansamples += frame.samples_per_frame;
        nframe++;
    }
end_scan:
    if (nframe == 0 || valid_frames == 0) {
        trace ("cmp3_scan_stream: couldn't find mpeg frames in file\n");
        return -1;
    }
    if (sample == 0) {
// {{{ calculate final averages
        buffer->avg_packetlength /= (float)valid_frames;
        buffer->avg_samplerate /= valid_frames;
        buffer->avg_samples_per_frame /= valid_frames;

        buffer->nframes = (fsize - buffer->startoffset - buffer->endoffset) / buffer->avg_packetlength;
        if (!buffer->have_xing_header) {
            if (reached_eof) {
                buffer->totalsamples = scansamples;
                buffer->duration = (buffer->totalsamples - buffer->delay - buffer->padding) / (float)buffer->samplerate;
            }
            else {
                buffer->totalsamples = buffer->nframes * buffer->avg_samples_per_frame;
                buffer->duration = (buffer->totalsamples - buffer->delay - buffer->padding) / (float)buffer->avg_samplerate;
            }
        }
        buffer->bitrate = (fsize - buffer->startoffset - buffer->endoffset) / buffer->duration * 8;
        trace ("nframes: %d, fsize: %lld, spf: %d, smp: %d, totalsamples: %d\n", buffer->nframes, fsize, buffer->avg_samples_per_frame, buffer->avg_samplerate, buffer->totalsamples);
// }}}
        return 0;
    }

    buffer->totalsamples = scansamples;
    buffer->duration = (buffer->totalsamples - buffer->delay - buffer->padding) / (float)buffer->samplerate;
//    printf ("nframes=%d, totalsamples=%d, samplerate=%d, dur=%f\n", nframe, scansamples, buffer->samplerate, buffer->duration);
    return 0;
}

int
cmp3_seek_stream (DB_fileinfo_t *_info, int sample) {
    mp3_info_t *info = (mp3_info_t *)_info;
    if (sample == 0) {
        _info->readpos = 0;
        info->buffer.currentsample = 0;
        info->buffer.skipsamples = 0;
        return 0;

    }
#if WRITE_DUMP
    if (out) {
        fclose (out);
        out = fopen ("out.raw", "w+b");
    }
#endif

    int res = cmp3_scan_stream (&info->buffer, sample);
    return res;
}


static DB_fileinfo_t *
cmp3_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (mp3_info_t));
    mp3_info_t *info = (mp3_info_t *)_info;
    memset (info, 0, sizeof (mp3_info_t));

    if (hints & DDB_DECODER_HINT_RAW_SIGNAL) {
        info->raw_signal = 1;
    }

#ifndef ANDROID // force 16 bit on android
    if ((hints & DDB_DECODER_HINT_16BIT) || deadbeef->conf_get_int ("mp3.force16bit", 0))
#endif
    {
        info->want_16bit = 1;
    }
    return _info;
}

void
cmp3_set_extra_properties (buffer_t *buffer, int fake) {
    char s[100];
    int64_t size = deadbeef->fgetlength (buffer->file);
    if (size >= 0) {
        snprintf (s, sizeof (s), "%lld", size);
        deadbeef->pl_replace_meta (buffer->it, ":FILE_SIZE", s);
    }
    else {
        deadbeef->pl_replace_meta (buffer->it, ":FILE_SIZE", "∞");
    }
    if (buffer->bitrate > 0) {
        snprintf (s, sizeof (s), "%d", buffer->bitrate/1000);
        deadbeef->pl_replace_meta (buffer->it, ":BITRATE", s);
    }
    snprintf (s, sizeof (s), "%d", buffer->channels);
    deadbeef->pl_replace_meta (buffer->it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", buffer->samplerate);
    deadbeef->pl_replace_meta (buffer->it, ":SAMPLERATE", s);

    // set codec profile (cbr or vbr) and mp3 vbr method (guessed, or from Xing/Info header)

    char codec_profile[100];
    snprintf (codec_profile, sizeof (codec_profile), "MP3 %s", (!buffer->vbr || buffer->vbr == XING_CBR || buffer->vbr == XING_CBR2) ?  "CBR" : "VBR");
    if (buffer->vbr != XING_CBR && buffer->vbr != XING_CBR2 && (buffer->lamepreset & 0x7ff)) {
        const static struct {
            int v;
            const char *name;
        } presets[] = {
            { 8, "ABR_8" },
            { 320, "ABR_320" },
            { 410, "V9" },
            { 420, "V8" },
            { 430, "V7" },
            { 440, "V6" },
            { 450, "V5" },
            { 460, "V4" },
            { 470, "V3" },
            { 480, "V2" },
            { 490, "V1" },
            { 500, "V0" },
            { 1000, "R3MIX" },
            { 1001, "STANDARD" },
            { 1002, "EXTREME" },
            { 1003, "INSANE" },
            { 1004, "STANDARD_FAST" },
            { 1005, "EXTREME_FAST" },
            { 1006, "MEDIUM" },
            { 1007, "MEDIUM_FAST" },
            { 0, NULL },
        };

        for (int i = 0; presets[i].name; i++) {
            if (presets[i].v == (buffer->lamepreset&0x7ff)) {
                size_t l = strlen (codec_profile);
                char *preset = codec_profile + l;
                snprintf (preset, sizeof (codec_profile) - l, " %s", presets[i].name);
                break;
            }
        }
    }

    deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", codec_profile);

    switch (buffer->vbr) {
    case XING_ABR:
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "ABR");
        break;
    case XING_VBR1:
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 1");
        break;
    case XING_VBR2:
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 2");
        break;
    case XING_VBR3:
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 3");
        break;
    case XING_VBR4:
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 4");
        break;
    case XING_ABR2:
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "ABR 2 pass");
        break;
    case DETECTED_VBR:
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "unspecified");
        break;
    }
    const char *versions[] = {"1", "2", "2.5"};
    snprintf (s, sizeof (s), "MPEG%s layer%d", versions[buffer->version-1], buffer->layer);
    deadbeef->pl_replace_meta (buffer->it, ":MPEG_VERSION", s);
    deadbeef->pl_replace_meta (buffer->it, ":XING_HEADER", buffer->have_xing_header ? "Yes" : "No");
    deadbeef->pl_replace_meta (buffer->it, fake ? "!FILETYPE" : ":FILETYPE", "MP3");
}

static int
cmp3_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    mp3_info_t *info = (mp3_info_t *)_info;

#if defined(USE_LIBMAD) && defined(USE_LIBMPG123)
    int backend = deadbeef->conf_get_int ("mp3.backend", 0);
    switch (backend) {
    case 0:
        info->dec = &mpg123_api;
        break;
    case 1:
        info->dec = &mad_api;
        break;
    default:
        info->dec = &mpg123_api;
        break;
    }
#else
#if defined(USE_LIBMAD)
    info->dec = &mad_api;
#else
    info->dec = &mpg123_api;
#endif
#endif

    _info->plugin = &plugin;
    memset (&info->buffer, 0, sizeof (info->buffer));
    deadbeef->pl_lock ();
    info->buffer.file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!info->buffer.file) {
        return -1;
    }
    info->info.file = info->buffer.file;
    deadbeef->pl_item_ref (it);
    info->buffer.it = it;
    info->info.readpos = 0;
    if (!info->buffer.file->vfs->is_streaming ()) {
        uint32_t start;
        uint32_t end;
        deadbeef->junk_get_tag_offsets (info->buffer.file, &start, &end);
        info->buffer.startoffset = start;
        info->buffer.endoffset = end;
        if (start > 0) {
            trace ("mp3: skipping %d(%xH) bytes of junk\n", start, start);
            deadbeef->fseek (info->buffer.file, start, SEEK_SET);
        }
#if 0
        deadbeef->conf_get_int ("mp3.disable_gapless", 0) ? 0 : -1;
#endif
        int scan_mode = -1;
        int res = cmp3_scan_stream (&info->buffer, scan_mode);
        if (res < 0) {
            trace ("mp3: cmp3_init: initial cmp3_scan_stream failed\n");
            return -1;
        }
        info->buffer.delay += 529;
        if (info->buffer.padding >= 529) {
            info->buffer.padding -= 529;
        }
        int64_t endsample = deadbeef->pl_item_get_endsample (it);
        if (endsample > 0) {
            info->buffer.startsample = deadbeef->pl_item_get_startsample (it) + info->buffer.delay;
            info->buffer.endsample = endsample + info->buffer.delay;
            // that comes from cue, don't calc duration, just seek and play
            trace ("mp3 totalsamples: %d\n", info->buffer.endsample-info->buffer.startsample+1);
        }
        else {
            ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
            deadbeef->plt_set_item_duration (plt, it, info->buffer.duration);
            if (plt) {
                deadbeef->plt_unref (plt);
            }
            info->buffer.startsample = info->buffer.delay;
            info->buffer.endsample = info->buffer.totalsamples-info->buffer.padding-1;
            trace ("mp3 totalsamples: %d (%d, %d, %d | %d %d)\n", info->buffer.endsample-info->buffer.startsample+1, info->buffer.totalsamples, info->buffer.delay, info->buffer.padding, info->buffer.startsample, info->buffer.endsample);
            trace ("mp3: seeking to %d(%xH) start offset\n", info->buffer.startoffset, info->buffer.startoffset);
            deadbeef->fseek (info->buffer.file, info->buffer.startoffset, SEEK_SET);
        }
        trace ("mp3: startsample: %d, endsample: %d, currentsample: %d\n", info->buffer.startsample, info->buffer.endsample, info->buffer.currentsample);
    }
    else {
        deadbeef->fset_track (info->buffer.file, it);
        deadbeef->pl_add_meta (it, "title", NULL);
        int res = cmp3_scan_stream (&info->buffer, 0);
        if (res < 0) {
            trace ("mp3: cmp3_init: initial cmp3_scan_stream failed\n");
            return -1;
        }

        cmp3_set_extra_properties (&info->buffer, 1);

        ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
        deadbeef->plt_set_item_duration (plt, it, info->buffer.duration);
        if (plt) {
            deadbeef->plt_unref (plt);
        }
        if (info->buffer.duration >= 0) {
            info->buffer.endsample = info->buffer.totalsamples - 1;
        }
        else {
            info->buffer.endsample = -1;
            info->buffer.totalsamples = -1;
        }
        info->buffer.skipsamples = 0;
        info->buffer.currentsample = 0;
        if (info->buffer.duration < 0) {
            info->buffer.duration = -1;
            info->buffer.totalsamples = -1;
            info->buffer.endsample = -1;
        }
        trace ("duration=%f, endsample=%d, totalsamples=%d\n", info->buffer.duration, info->buffer.endsample, info->buffer.totalsamples);
    }
    if (info->buffer.samplerate == 0) {
        trace ("bad mpeg file: %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }
    if (info->want_16bit && !info->raw_signal) {
        _info->fmt.bps = 16;
        _info->fmt.is_float = 0;
    }
    else {
        _info->fmt.bps = 32;
        _info->fmt.is_float = 1;
    }
    _info->fmt.samplerate = info->buffer.samplerate;
    _info->fmt.channels = info->buffer.channels;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    trace ("mp3 format: bps:%d sr:%d channels:%d\n", _info->fmt.bps, _info->fmt.samplerate, _info->fmt.channels);

    if (info->want_16bit) {
        deadbeef->pl_replace_meta (it, ":BPS", "16");
    }
    else {
        deadbeef->pl_replace_meta (it, ":BPS", "32");
    }

    info->dec->init (info);
    if (!info->buffer.file->vfs->is_streaming ()) {
        plugin.seek_sample (_info, 0);
    }
    return 0;
}

static inline void
cmp3_skip (mp3_info_t *info) {
    if (info->buffer.skipsamples > 0) {
        int skip = min (info->buffer.skipsamples, info->buffer.decode_remaining);
//        printf ("skip %d / %d\n", skip, info->buffer.skipsamples);
        info->buffer.skipsamples -= skip;
        info->buffer.decode_remaining -= skip;
    }
}

#if 0
static void
dump_buffer (buffer_t *buffer) {
    printf ("*** DUMP ***\n");
    printf ("remaining: %d\n", buffer->remaining);

    printf ("readsize: %d\n", buffer->readsize);
    printf ("decode_remaining: %d\n", buffer->decode_remaining);

    // information, filled by cmp3_scan_stream
    printf ("%d\n", buffer->version);
    printf ("%d\n", buffer->layer);
    printf ("%d\n", buffer->bitrate);
    printf ("%d\n", buffer->samplerate);
    printf ("%d\n", buffer->packetlength);
    printf ("%d\n", buffer->bitspersample);
    printf ("%d\n", buffer->channels);
    printf ("%f\n", buffer->duration);

    printf ("%d\n", buffer->currentsample);
    printf ("%d\n", buffer->totalsamples);
    printf ("%d\n", buffer->skipsamples);
    printf ("%d\n", buffer->startoffset);
    printf ("%d\n", buffer->endoffset);
    printf ("%d\n", buffer->startsample);
    printf ("%d\n", buffer->endsample);
    printf ("%d\n", buffer->delay);
    printf ("%d\n", buffer->padding);

    printf ("%f\n", buffer->avg_packetlength);
    printf ("%d\n", buffer->avg_samplerate);
    printf ("%d\n", buffer->avg_samples_per_frame);
    printf ("%d\n", buffer->nframes);
    printf ("%d\n", buffer->last_comment_update);
    printf ("%d\n", buffer->vbr);
    printf ("%d\n", buffer->have_xing_header);
    printf ("%d\n", buffer->current_decode_frame);
    printf ("%lld\n", buffer->lastframe_filepos);

    printf ("*** END ***\n");
}
#endif

// decoded requested number of samples to int16 format
static void
cmp3_decode_requested (mp3_info_t *info) {
    cmp3_skip (info);
    if (info->buffer.skipsamples > 0) {
        return;
    }
    info->dec->decode (info);

    assert (info->buffer.readsize >= 0);
}

static int
cmp3_stream_frame (mp3_info_t *info) {
    return info->dec->stream_frame (info);
}

static int
cmp3_decode (mp3_info_t *info) {
    int eof = 0;
    while (!eof) {
        eof = cmp3_stream_frame (info);
        if (info->buffer.decode_remaining > 0) {
            cmp3_decode_requested (info);
            if (info->buffer.readsize == 0) {
                return 0;
            }
        }
    }
    return 0;
}

static void
cmp3_free (DB_fileinfo_t *_info) {
    mp3_info_t *info = (mp3_info_t *)_info;
    if (info->buffer.it) {
        deadbeef->pl_item_unref (info->buffer.it);
    }
    if (info->conv_buf) {
        free (info->conv_buf);
    }
    if (info->buffer.file) {
        deadbeef->fclose (info->buffer.file);
        info->buffer.file = NULL;
        info->info.file = NULL;
        info->dec->free (info);
    }
    free (info);
}

static int
cmp3_read (DB_fileinfo_t *_info, char *bytes, int size) {
#if WRITE_DUMP
    if (!out) {
        out = fopen ("out.raw", "w+b");
    }
#endif
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    mp3_info_t *info = (mp3_info_t *)_info;
    if (info->buffer.duration >= 0 && !info->buffer.file->vfs->is_streaming ()) {
        int curr = info->buffer.currentsample;
        //printf ("curr: %d -> end %d, padding: %d\n", curr, info->buffer.endsample, info->buffer.padding);
        if (size / samplesize + curr > info->buffer.endsample) {
            size = (info->buffer.endsample - curr + 1) * samplesize;
            trace ("\033[0;32mmp3: size truncated to %d bytes (%d samples), cursample=%d, endsample=%d\033[37;0m\n", size, info->buffer.endsample - curr + 1, curr, info->buffer.endsample);
            if (size <= 0) {
                return 0;
            }
        }
    }

    int initsize = size;

    int req_size;
    if (info->want_16bit && !info->raw_signal) {
        req_size = size * 2;
        // decode in 32 bit temp buffer, then convert to 16 below
        if (info->conv_buf_size < req_size) {
            info->conv_buf_size = req_size;
            if (info->conv_buf) {
                free (info->conv_buf);
            }
            info->conv_buf = malloc (info->conv_buf_size);
        }
        info->buffer.readsize = req_size;
        info->buffer.out = info->conv_buf;
    }
    else {
        req_size = size;
        // decode straight to 32 bit
        info->buffer.readsize = size;
        info->buffer.out = bytes;
    }

    cmp3_decode (info);

    if (!info->raw_signal) {
        ddb_waveformat_t fmt;
        memcpy (&fmt, &info->info.fmt, sizeof (fmt));
        fmt.bps = 32;
        fmt.is_float = 1;

        // apply replaygain, before clipping
        deadbeef->replaygain_apply (&fmt, info->want_16bit ? info->conv_buf : bytes, req_size - info->buffer.readsize);

        // convert to 16 bit, if needed
        if (info->want_16bit) {
            int sz = req_size - info->buffer.readsize;
            int ret = deadbeef->pcm_convert (&fmt, info->conv_buf, &_info->fmt, bytes, sz);
            info->buffer.readsize = size-ret;
        }
    }

    info->buffer.currentsample += (size - info->buffer.readsize) / samplesize;
    _info->readpos = (float)(info->buffer.currentsample - info->buffer.startsample) / info->buffer.samplerate;
#if WRITE_DUMP
    if (size - info->buffer.readsize > 0) {
        fwrite (bytes, 1, size - info->buffer.readsize, out);
    }
#endif
    return initsize - info->buffer.readsize;
}

static int
cmp3_seek_sample (DB_fileinfo_t *_info, int sample) {
    mp3_info_t *info = (mp3_info_t *)_info;
    if (!info->buffer.file) {
        return -1;
    }

// {{{ handle net streaming case
    if (info->buffer.file->vfs->is_streaming ()) {
        if (info->buffer.totalsamples > 0 && info->buffer.avg_samples_per_frame > 0 && info->buffer.avg_packetlength > 0) { // that means seekable remote stream, like podcast
            trace ("seeking is possible!\n");

            int r;

            // seek to beginning of the frame
            int64_t frm = sample / info->buffer.avg_samples_per_frame;
            r = deadbeef->fseek (info->buffer.file, frm * info->buffer.avg_packetlength + info->buffer.startoffset, SEEK_SET);

            if (!r) {
                info->buffer.skipsamples = (int)(sample - frm * info->buffer.avg_samples_per_frame);

                info->buffer.currentsample = sample;
                _info->readpos = (float)(info->buffer.currentsample - info->buffer.startsample) / info->buffer.samplerate;

                info->dec->free (info);
                info->buffer.remaining = 0;
                info->buffer.decode_remaining = 0;
                info->dec->init (info);
                return 0;
            }
            trace ("seek failed!\n");
            return -1;
        }
        trace ("seek is impossible (avg_samples_per_frame=%d, avg_packetlength=%f)!\n", info->buffer.avg_samples_per_frame, info->buffer.avg_packetlength);
        return 0;
    }
// }}}

    sample += info->buffer.startsample;
    if (sample > info->buffer.endsample) {
        sample = info->buffer.endsample;
    }
    // restart file, and load until we hit required pos
    deadbeef->fseek (info->buffer.file, info->buffer.startoffset, SEEK_SET);

    info->buffer.remaining = 0;
    info->buffer.readsize = 0;
    info->buffer.decode_remaining = 0;

    // force flush the decoder by reinitializing it
    info->dec->free (info);
    info->dec->init (info);

//    struct timeval tm1;
//    gettimeofday (&tm1, NULL);
    if (cmp3_seek_stream (_info, sample) == -1) {
        trace ("failed to seek to sample %d\n", sample);
        _info->readpos = 0;
        return -1;
    }
//    struct timeval tm2;
//    gettimeofday (&tm2, NULL);
//    int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
//    printf ("cmp3_scan_stream took %d ms\n", ms);
	trace ("seeked to %d\n", info->buffer.currentsample);
    _info->readpos = (float)(info->buffer.currentsample - info->buffer.startsample) / info->buffer.samplerate;
    return 0;
}

static int
cmp3_seek (DB_fileinfo_t *_info, float time) {
    mp3_info_t *info = (mp3_info_t *)_info;
    int sample = time * info->buffer.samplerate;
    return cmp3_seek_sample (_info, sample);
}

static DB_playItem_t *
cmp3_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("cmp3_insert %s\n", fname);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    if (fp->vfs->is_streaming ()) {
        DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
        deadbeef->fclose (fp);
        deadbeef->pl_add_meta (it, "title", NULL);
        deadbeef->plt_set_item_duration (plt, it, -1);
        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
        return after;
    }
    buffer_t buffer;
    memset (&buffer, 0, sizeof (buffer));
    buffer.file = fp;
    uint32_t start;
    uint32_t end;
    deadbeef->junk_get_tag_offsets (buffer.file, &start, &end);
    buffer.startoffset = start;
    buffer.endoffset = end;
    if (start > 0) {
        trace ("mp3: skipping %d bytes (tag)\n", start);
        deadbeef->fseek(buffer.file, start, SEEK_SET);
    }
    // calc approx. mp3 duration 
    int res = cmp3_scan_stream (&buffer, 0);
    if (res < 0) {
        trace ("mp3: cmp3_scan_stream returned error\n");
        deadbeef->fclose (fp);
        return NULL;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);

    deadbeef->rewind (fp);
    // reset tags
    uint32_t f = deadbeef->pl_get_item_flags (it);
    f &= ~DDB_TAG_MASK;
    deadbeef->pl_set_item_flags (it, f);
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
    /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
    deadbeef->pl_set_meta_int (it, ":MP3_DELAY", buffer.delay);
    deadbeef->pl_set_meta_int (it, ":MP3_PADDING", buffer.padding);

    buffer.it = it;
    cmp3_set_extra_properties (&buffer, 0);

    deadbeef->plt_set_item_duration (plt, it, buffer.duration);
    deadbeef->fclose (fp);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, buffer.totalsamples-buffer.delay-buffer.padding, buffer.samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

int
cmp3_read_metadata (DB_playItem_t *it) {
    deadbeef->pl_lock ();
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
    // FIXME: reload and apply the Xing header
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
    /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->fclose (fp);
    return 0;
}

int
cmp3_write_metadata (DB_playItem_t *it) {
    // get options

    int strip_id3v2 = deadbeef->conf_get_int ("mp3.strip_id3v2", 0);
    int strip_id3v1 = deadbeef->conf_get_int ("mp3.strip_id3v1", 0);
    int strip_apev2 = deadbeef->conf_get_int ("mp3.strip_apev2", 0);
    int write_id3v2 = deadbeef->conf_get_int ("mp3.write_id3v2", 1);
    int write_id3v1 = deadbeef->conf_get_int ("mp3.write_id3v1", 1);
    int write_apev2 = deadbeef->conf_get_int ("mp3.write_apev2", 0);

    uint32_t junk_flags = 0;
    if (strip_id3v2) {
        junk_flags |= JUNK_STRIP_ID3V2;
    }
    if (strip_id3v1) {
        junk_flags |= JUNK_STRIP_ID3V1;
    }
    if (strip_apev2) {
        junk_flags |= JUNK_STRIP_APEV2;
    }
    if (write_id3v2) {
        junk_flags |= JUNK_WRITE_ID3V2;
    }
    if (write_id3v1) {
        junk_flags |= JUNK_WRITE_ID3V1;
    }
    if (write_apev2) {
        junk_flags |= JUNK_WRITE_APEV2;
    }

    int id3v2_version = deadbeef->conf_get_int ("mp3.id3v2_version", 3);
    if (id3v2_version != 3 && id3v2_version != 4) {
        id3v2_version = 3;
    }
    char id3v1_encoding[50];
    deadbeef->conf_get_str ("mp3.id3v1_encoding", "cp1252", id3v1_encoding, sizeof (id3v1_encoding));
    return deadbeef->junk_rewrite_tags (it, junk_flags, id3v2_version, id3v1_encoding);
}

static const char *exts[] = {
	"mp1", "mp2", "mp3", "mpga", NULL
};

static const char settings_dlg[] =
    "property \"Force 16 bit output\" checkbox mp3.force16bit 0;\n"
//    "property \"Disable gapless playback (faster scanning)\" checkbox mp3.disable_gapless 0;\n"
#if defined(USE_LIBMAD) && defined(USE_LIBMPG123)
    "property \"Backend\" select[2] mp3.backend 0 mpg123 mad;\n"
#endif
;

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 10, // requires API level 10 for logger and replaygain support
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.flags = DDB_PLUGIN_FLAG_REPLAYGAIN,
    .plugin.id = "stdmpg",
    .plugin.name = "MP3 player",
    .plugin.descr = "MPEG v1/2 layer1/2/3 decoder\n\n"
#if defined(USE_LIBMPG123) && defined(USE_LIBMAD)
    "Can use libmad and libmpg123 backends.\n"
    "Changing the backend will take effect when the next track starts.\n"
#elif defined(USE_LIBMAD)
    "Using libmad backend.\n"
#elif defined(USE_LIBMPG123)
    "Using libmpg123 backend.\n"
#endif
    ,
    .plugin.copyright = 
        "MPEG decoder plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Alexey Yakovenko\n"
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
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.configdialog = settings_dlg,
    .open = cmp3_open,
    .init = cmp3_init,
    .free = cmp3_free,
    .read = cmp3_read,
    .seek = cmp3_seek,
    .seek_sample = cmp3_seek_sample,
    .insert = cmp3_insert,
    .read_metadata = cmp3_read_metadata,
    .write_metadata = cmp3_write_metadata,
    .exts = exts,
};

DB_plugin_t *
mp3_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
