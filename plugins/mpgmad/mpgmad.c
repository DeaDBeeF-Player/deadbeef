/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <stdio.h>
#include <mad.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include "../../deadbeef.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

//#define WRITE_DUMP 1

#if WRITE_DUMP
FILE *out;
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

#define READBUFFER 0x2800 // 10k is enough for single frame

// vbrmethod constants
#define XING_CBR  1 
#define XING_ABR  2
#define XING_VBR1 3
#define XING_VBR2 4
#define XING_VBR3 5
#define XING_VBR4 6
#define XING_CBR2 8
#define XING_ABR2 9
#define DETECTED_VBR 100

// xing header flags
#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

typedef struct {
    DB_FILE *file;
    DB_playItem_t *it;

    // input buffer, for MPEG data
    char input[READBUFFER];
    int remaining;

    // output buffer, supplied by player
    int readsize;
    int decode_remaining; // number of decoded samples of current mpeg frame
    char *out;

    // information, filled by cmp3_scan_stream
    int version;
    int layer;
    int bitrate;
    int samplerate;
    int packetlength;
    int bitspersample;
    int channels;
    float duration;

    // currentsample and totalsamples are in the entire file scope (delay/padding inclusive)
    int currentsample;
    int totalsamples;

    int skipsamples;

    int startoffset; // in bytes (id3v2, xing/lame)
    int endoffset; // in bytes (apev2, id3v1)

    // startsample and endsample exclude delay/padding
    int startsample;
    int endsample;

    // number of samples to skip at the start/end of file
    int delay;
    int padding;

    float avg_packetlength;
    int avg_samplerate;
    int avg_samples_per_frame;
    int nframes;
    int last_comment_update;
    int vbr;
    int have_xing_header;
    int lead_in_frames;
} buffer_t;

typedef struct {
    DB_fileinfo_t info;
    buffer_t buffer;
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
} mpgmad_info_t;


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

// sample=-1: scan entire stream, calculate precise duration
// sample=0: read headers/tags, calculate approximate duration
// sample>0: seek to the frame with the sample, update skipsamples
// return value: -1 on error
static int
cmp3_scan_stream (buffer_t *buffer, int sample) {
    trace ("cmp3_scan_stream %d\n", sample);

// {{{ prepare for scan - seek, reset averages, etc
    int initpos = deadbeef->ftell (buffer->file);
    trace ("initpos: %d\n", initpos);
    int packetlength = 0;
    int nframe = 0;
    int scansamples = 0;
    buffer->currentsample = 0;
    buffer->skipsamples = 0;
//    int avg_bitrate = 0;
    int valid_frames = 0;
    int prev_bitrate = -1;
    buffer->samplerate = 0;
    int64_t fsize = deadbeef->fgetlength (buffer->file);

    if (sample <= 0) {
        buffer->totalsamples = 0;
        if (fsize > 0) {
            fsize -= initpos;
            if (fsize < 0) {
                trace ("cmp3_scan_stream: invalid file: bad header\n");
                return -1;
            }
        }
    }
    if (sample <= 0 && buffer->avg_packetlength == 0) {
        buffer->avg_packetlength = 0;
        buffer->avg_samplerate = 0;
        buffer->avg_samples_per_frame = 0;
        buffer->nframes = 0;
        trace ("setting startoffset to %d\n", initpos);
        buffer->startoffset = initpos;
    }

    int had_invalid_frames = 0;
    int lastframe_valid = 0;
    int64_t offs = -1;
// }}}

    int64_t lead_in_frame_pos = buffer->startoffset;
    int64_t lead_in_frame_no = 0;

#define MAX_LEAD_IN_FRAMES 10
    int64_t frame_positions[MAX_LEAD_IN_FRAMES]; // positions of nframe-9, nframe-8, nframe-7, ...
    for (int i = 0; i < MAX_LEAD_IN_FRAMES; i++) {
        frame_positions[i] = buffer->startoffset;
    }

    for (;;) {
// {{{ parse frame header, sync stream
        // mp3 files often have some garbage in the beginning
        // try to skip it if this is the case
        if (!lastframe_valid && valid_frames > 5) {
            had_invalid_frames = 1;
        }
        if (!lastframe_valid && offs >= 0) {
            deadbeef->fseek (buffer->file, offs+1, SEEK_SET);
        }
        offs = deadbeef->ftell (buffer->file);
        //uint8_t fb[4+2+2]; // 4b frame header + 2b crc + 2b sideinfo main_data_begin
        uint8_t fb[4]; // just the header
        if (deadbeef->fread (fb, 1, sizeof(fb), buffer->file) != sizeof(fb)) {
            break; // eof
        }

        uint32_t hdr;
        uint8_t sync = fb[0];
        if (sync != 0xff) {
//            trace ("[1]frame %d didn't seek to frame end\n", nframe);
            lastframe_valid = 0;
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            sync = fb[1];
            if ((sync >> 5) != 7) {
//                trace ("[2]frame %d didn't seek to frame end\n", nframe);
                lastframe_valid = 0;
                continue;
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
        int ver = (hdr & (3<<19)) >> 19;
        ver = vertbl[ver];
        if (ver < 0) {
            trace ("frame %d bad mpeg version %d\n", nframe, (hdr & (3<<19)) >> 19);
            lastframe_valid = 0;
            continue; // invalid frame
        }

        // layer info
        static const int ltbl[] = { -1, 3, 2, 1 };
        int layer = (hdr & (3<<17)) >> 17;
        layer = ltbl[layer];
        if (layer < 0) {
            trace ("frame %d bad layer %d\n", nframe, (hdr & (3<<17)) >> 17);
            lastframe_valid = 0;
            continue; // invalid frame
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
        int bitrate = (hdr & (0x0f<<12)) >> 12;
        int idx = 0;
        if (ver == 1) {
            idx = layer - 1;
        }
        else {
            idx = layer == 1 ? 3 : 4;
        }
        bitrate = brtable[idx][bitrate];
        if (bitrate <= 0) {
            trace ("frame %d bad bitrate %d\n", nframe, (hdr & (0x0f<<12)) >> 12);
            lastframe_valid = 0;
            continue; // invalid frame
        }

        // samplerate
        static const int srtable[3][4] = {
            {44100, 48000, 32000, -1},
            {22050, 24000, 16000, -1},
            {11025, 12000, 8000, -1},
        };
        int samplerate = (hdr & (0x03<<10))>>10;
        samplerate = srtable[ver-1][samplerate];
        if (samplerate < 0) {
            trace ("frame %d bad samplerate %d\n", nframe, (hdr & (0x03<<10))>>10);
            lastframe_valid = 0;
            continue; // invalid frame
        }

        // padding
        int padding = (hdr & (0x1 << 9)) >> 9;

        static const int chantbl[4] = { 2, 2, 2, 1 };
        int nchannels = (hdr & (0x3 << 6)) >> 6;
        nchannels = chantbl[nchannels];

        // check if channel/bitrate combination is valid for layer2
        if (layer == 2) {
            if ((bitrate <= 56 || bitrate == 80) && nchannels != 1) {
                trace ("mpgmad: bad frame %d: layer %d, channels %d, bitrate %d\n", nframe, layer, nchannels, bitrate);
                lastframe_valid = 0;
                continue; // bad frame
            }
            if (bitrate >= 224 && nchannels == 1) {
                trace ("mpgmad: bad frame %d: layer %d, channels %d, bitrate %d\n", nframe, layer, nchannels, bitrate);
                lastframe_valid = 0;
                continue; // bad frame
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
        packetlength = 0;
        bitrate *= 1000;
        int samples_per_frame = 0;
        if (samplerate > 0 && bitrate > 0) {
            if (layer == 1) {
                samples_per_frame = 384;
            }
            else if (layer == 2) {
                samples_per_frame = 1152;
            }
            else if (layer == 3) {
                if (ver == 1) {
                    samples_per_frame = 1152;
                }
                else {
                    samples_per_frame = 576;
                }
            }
            packetlength = samples_per_frame / 8 * bitrate / samplerate + padding;
//            if (sample > 0) {
//                printf ("frame: %d, crc: %d, layer: %d, bitrate: %d, samplerate: %d, filepos: 0x%llX, dataoffs: 0x%X, size: 0x%X\n", nframe, prot, layer, bitrate, samplerate, deadbeef->ftell (buffer->file)-8, data_ptr, packetlength);
//            }
        }
        else {
            trace ("frame %d samplerate or bitrate is invalid\n", nframe);
            lastframe_valid = 0;
            continue;
        }
// }}}

// {{{ vbr adjustement
        if (!buffer->have_xing_header && prev_bitrate != -1 && prev_bitrate != bitrate) {
            buffer->vbr = DETECTED_VBR;
        }
        prev_bitrate = bitrate;
// }}}

        valid_frames++;

// {{{ update stream parameters, only when sample!=0 or 1st frame
        if (sample != 0 || nframe == 0)
        {
            if (sample == 0 && lastframe_valid) {
                return 0;
            }
            // don't get parameters from frames coming after any bad frame
            if (!had_invalid_frames) {
                buffer->version = ver;
                buffer->layer = layer;
                buffer->bitrate = bitrate;
                buffer->samplerate = samplerate;
                buffer->packetlength = packetlength;
                if (nchannels > buffer->channels) {
                    buffer->channels = nchannels;
                }
                buffer->bitspersample = 16;
//                trace ("frame %d mpeg v%d layer %d bitrate %d samplerate %d packetlength %d channels %d\n", nframe, ver, layer, bitrate, samplerate, packetlength, nchannels);
            }
        }
// }}}

        lastframe_valid = 1;

        int64_t framepos = deadbeef->ftell (buffer->file)-(int)sizeof(fb);

        // allow at least 10 lead-in frames, to fill bit-reservoir
        if (nframe - lead_in_frame_no > MAX_LEAD_IN_FRAMES) {
            lead_in_frame_pos = frame_positions[0];
            lead_in_frame_no++;
        }
        memmove (frame_positions, &frame_positions[1], sizeof (int64_t) * (MAX_LEAD_IN_FRAMES-1));
        frame_positions[MAX_LEAD_IN_FRAMES-1] = framepos;

// {{{ detect/load xing frame, only on 1st pass
        // try to read xing/info tag (only on initial scans)
        if (sample <= 0 && !buffer->have_xing_header)
        {
            if (!buffer->file->vfs->is_streaming ()) {
                //            trace ("trying to read xing header at pos %d\n", framepos);
                if (ver == 1) {
                    deadbeef->fseek (buffer->file, 32/*-2-2*prot*/, SEEK_CUR);
                }
                else if (ver == 2) {
                    // in mpeg2 streams, sideinfo is 17 bytes
                    deadbeef->fseek (buffer->file, 17/*-2-prot*2*/, SEEK_CUR);
                }
                const char xing[] = "Xing";
                const char info[] = "Info";
                char magic[4];
                if (deadbeef->fread (magic, 1, 4, buffer->file) != 4) {
                    trace ("cmp3_scan_stream: EOF while checking for Xing header\n");
                    return -1; // EOF
                }

                //            trace ("xing magic: %c%c%c%c\n", magic[0], magic[1], magic[2], magic[3]);

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
                        if (sample == 0) {
                            buffer->duration = (((uint64_t)nframes * (uint64_t)samples_per_frame) - buffer->delay - buffer->padding)/ (uint64_t)samplerate;
                        }
                        trace ("xing totalsamples: %d, nframes: %d, samples_per_frame: %d\n", nframes*samples_per_frame, nframes, samples_per_frame);
                        if (nframes <= 0 || samples_per_frame <= 0) {
                            trace ("bad xing header\n");
                            continue;
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
                    //                trace ("tell=%x, %c%c%c%c\n", deadbeef->ftell(buffer->file), buf[0], buf[1], buf[2], buf[3]);

                    deadbeef->fseek (buffer->file, 5, SEEK_CUR);
                    uint8_t rev = 0;
                    if (deadbeef->fread (&rev, 1, 1, buffer->file) != 1) {
                        trace ("cmp3_scan_stream: EOF while reading info tag revision / vbr method\n");
                    }
                    switch (rev & 0x0f) {
                    case XING_ABR ... XING_VBR4:
                    case XING_ABR2:
                        buffer->vbr = rev & 0x0f;
                        break;
                    default:
                        buffer->vbr = DETECTED_VBR;
                        break;
                    }
                    if (!memcmp (buf, "LAME", 4)) {
                        trace ("lame header found\n");

                        // FIXME: that can be optimized by single read
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
                        // skip
                        deadbeef->fseek (buffer->file, 2, SEEK_CUR);
                        // musiclen
                        deadbeef->fread (buf, 1, 4, buffer->file);
                        //                    uint32_t musiclen = extract_i32 (buf);

                        //trace ("lpf: %d, peaksignalamp: %f, radiogain: %d, audiophile: %d, delay: %d, padding: %d, mp3gain: %d, musiclen: %d\n", lpf, rg_peaksignalamp, rg_radio, rg_audiophile, delay, padding, mp3gain, musiclen);
                        // skip crc
                        //deadbeef->fseek (buffer->file, 4, SEEK_CUR);
                        trace ("lame totalsamples: %d\n", buffer->totalsamples);
                    }
                    if (sample <= 0 && (flags&FRAMES_FLAG)) {
                        buffer->have_xing_header = 1;
                        deadbeef->fseek (buffer->file, framepos+packetlength, SEEK_SET);
                        if (fsize >= 0) {
                            buffer->bitrate = (fsize - deadbeef->ftell (buffer->file))/ buffer->samplerate * 1000;
                        }
                        buffer->startoffset = deadbeef->ftell (buffer->file);
                    }
                }
            }
            if (sample == 0) {
                trace ("cmp3_scan_stream: trying to figure out duration from file size\n");
                buffer->samplerate = samplerate;
                if (buffer->file->vfs->is_streaming ()) {
                    // only suitable for cbr files, used if streaming
                    int sz = deadbeef->fgetlength (buffer->file);
                    if (sz > 0) {
                        sz -= buffer->startoffset + buffer->endoffset;
                        if (sz < 0) {
                            trace ("cmp3_scan_stream: bad file headers\n");
                            return -1;
                        }
                    }
                    if (sz < 0) {
                        // unable to determine duration
                        buffer->duration = -1;
                        buffer->totalsamples = -1;
                        if (sample == 0) {
                            trace ("check validity of the next frame...\n");
                            deadbeef->fseek (buffer->file, framepos+packetlength, SEEK_SET);
                            continue;
                        }
                        trace ("cmp3_scan_stream: unable to determine duration");
                        return 0;
                    }
                    buffer->nframes = sz / packetlength;
                    buffer->avg_packetlength = packetlength;
                    buffer->avg_samplerate = samplerate;
                    buffer->avg_samples_per_frame = samples_per_frame;
                    buffer->duration = (buffer->nframes * samples_per_frame - buffer->delay - buffer->padding) / samplerate;
                    buffer->totalsamples = buffer->nframes * samples_per_frame;
                    trace ("totalsamples: %d, samplesperframe: %d, fsize=%lld\n", buffer->totalsamples, samples_per_frame, fsize);
//                    trace ("bitrate=%d, layer=%d, packetlength=%d, fsize=%d, nframes=%d, samples_per_frame=%d, samplerate=%d, duration=%f, totalsamples=%d\n", bitrate, layer, packetlength, sz, nframe, samples_per_frame, samplerate, buffer->duration, buffer->totalsamples);

                    if (sample == 0) {
                        deadbeef->fseek (buffer->file, framepos, SEEK_SET);
                        return 0;
                    }
                }
                else {
                    deadbeef->fseek (buffer->file, framepos, SEEK_SET);
                }
            }
            else {
                deadbeef->fseek (buffer->file, framepos+packetlength, SEEK_SET);
                buffer->have_xing_header = 1;
            }
        }
// }}}

        if (sample == 0) {
// {{{ update averages, interrupt scan on frame #100
            if (fsize <= 0) {
                trace ("cmp3_scan_stream: negative file size\n");
                return -1;
            }
            // calculating apx duration based on 1st 100 frames
            buffer->avg_packetlength += packetlength;
            buffer->avg_samplerate += samplerate;
            buffer->avg_samples_per_frame += samples_per_frame;
            //avg_bitrate += bitrate;
            if (nframe >= 100) {
                goto end_scan;
            }
// }}}
        }
        else {
            // seeking to particular sample, interrupt if reached
            if (sample > 0 && scansamples + samples_per_frame >= sample) {
                deadbeef->fseek (buffer->file, lead_in_frame_pos, SEEK_SET);
                buffer->lead_in_frames = nframe-lead_in_frame_no;
                buffer->currentsample = sample;
                buffer->skipsamples = sample - scansamples;
                trace ("scan: cursample=%d, frame: %d, skipsamples: %d, filepos: %llX, lead-in frames: %d\n", buffer->currentsample, nframe, buffer->skipsamples, deadbeef->ftell (buffer->file), buffer->lead_in_frames);
                return 0;
            }
        }
        scansamples += samples_per_frame;
        nframe++;
        if (packetlength > 0) {
            deadbeef->fseek (buffer->file, packetlength-(int)sizeof(fb), SEEK_CUR);
        }
    }
end_scan:
    if (nframe == 0) {
        trace ("cmp3_scan_stream: couldn't find mpeg frames in file\n");
        return -1;
    }
    if (sample == 0) {
// {{{ calculate final averages
        buffer->avg_packetlength /= (float)valid_frames;
        buffer->avg_samplerate /= valid_frames;
        buffer->avg_samples_per_frame /= valid_frames;
//        avg_bitrate /= valid_frames;
        //trace ("valid_frames=%d, avg_bitrate=%d, avg_packetlength=%f, avg_samplerate=%d, avg_samples_per_frame=%d\n", valid_frames, avg_bitrate, buffer->avg_packetlength, buffer->avg_samplerate, buffer->avg_samples_per_frame);
        trace ("startoffs: %d, endoffs: %d\n",  buffer->startoffset, buffer->endoffset);

        buffer->nframes = (fsize - buffer->startoffset - buffer->endoffset) / buffer->avg_packetlength;
        if (!buffer->have_xing_header) {
            buffer->totalsamples = buffer->nframes * buffer->avg_samples_per_frame;
            buffer->duration = (buffer->totalsamples - buffer->delay - buffer->padding) / buffer->avg_samplerate;
        }
        buffer->bitrate = (fsize-buffer->startoffset-buffer->endoffset) / buffer->duration * 8;
        trace ("nframes: %d, fsize: %lld, spf: %d, smp: %d, totalsamples: %d\n", buffer->nframes, fsize, buffer->avg_samples_per_frame, buffer->avg_samplerate, buffer->totalsamples);
// }}}
        return 0;
    }

    buffer->totalsamples = scansamples;
    buffer->duration = (buffer->totalsamples - buffer->delay - buffer->padding) / buffer->samplerate;
//    printf ("nframes=%d, totalsamples=%d, samplerate=%d, dur=%f\n", nframe, scansamples, buffer->samplerate, buffer->duration);
    return 0;
}

int
cmp3_seek_stream (DB_fileinfo_t *_info, int sample) {
    mpgmad_info_t *info = (mpgmad_info_t *)_info;
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
    DB_fileinfo_t *_info = malloc (sizeof (mpgmad_info_t));
    mpgmad_info_t *info = (mpgmad_info_t *)_info;
    memset (info, 0, sizeof (mpgmad_info_t));
    return _info;
}

void
cmp3_set_extra_properties (buffer_t *buffer) {
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
    deadbeef->pl_replace_meta (buffer->it, ":BPS", "16");
    snprintf (s, sizeof (s), "%d", buffer->channels);
    deadbeef->pl_replace_meta (buffer->it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", buffer->samplerate);
    deadbeef->pl_replace_meta (buffer->it, ":SAMPLERATE", s);

    // set codec profile (cbr or vbr) and mp3 vbr method (guessed, or from Xing/Info header)

    switch (buffer->vbr) {
    case XING_ABR:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "VBR");
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "ABR");
        break;
    case XING_VBR1:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "VBR");
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 1");
        break;
    case XING_VBR2:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "VBR");
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 2");
        break;
    case XING_VBR3:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "VBR");
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 3");
        break;
    case XING_VBR4:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "VBR");
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "full VBR method 4");
        break;
    case XING_CBR2:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "CBR");
        break;
    case XING_ABR2:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "VBR");
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "ABR 2 pass");
        break;
    case DETECTED_VBR:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "VBR");
        deadbeef->pl_replace_meta (buffer->it, ":MP3_VBR_METHOD", "unspecified");
        break;
    default:
        deadbeef->pl_replace_meta (buffer->it, ":CODEC_PROFILE", "CBR");
        break;
    }
    const char *versions[] = {"1", "2", "2.5"};
    snprintf (s, sizeof (s), "MPEG%s layer%d", versions[buffer->version-1], buffer->layer);
    deadbeef->pl_replace_meta (buffer->it, ":MPEG_VERSION", s);
    deadbeef->pl_replace_meta (buffer->it, ":XING_HEADER", buffer->have_xing_header ? "Yes" : "No");
    deadbeef->pl_replace_meta (buffer->it, ":FILETYPE", "MP3");
}

static int
cmp3_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    mpgmad_info_t *info = (mpgmad_info_t *)_info;
    _info->plugin = &plugin;
    memset (&info->buffer, 0, sizeof (info->buffer));
    info->buffer.file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    if (!info->buffer.file) {
        return -1;
    }
    info->info.file = info->buffer.file;
    deadbeef->pl_item_ref (it);
    info->buffer.it = it;
    info->info.readpos = 0;
    if (!info->buffer.file->vfs->is_streaming ()) {
        int skip = deadbeef->junk_get_leading_size (info->buffer.file);
        if (skip > 0) {
            trace ("mpgmad: skipping %d(%xH) bytes of junk\n", skip, skip);
            deadbeef->fseek (info->buffer.file, skip, SEEK_SET);
        }
        int res = cmp3_scan_stream (&info->buffer, -1);
        if (res < 0) {
            trace ("mpgmad: cmp3_init: initial cmp3_scan_stream failed\n");
            return -1;
        }
        info->buffer.delay += 529;
        if (info->buffer.padding >= 529) {
            info->buffer.padding -= 529;
        }
        if (it->endsample > 0) {
            info->buffer.startsample = it->startsample + info->buffer.delay;
            info->buffer.endsample = it->endsample + info->buffer.delay;
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
            trace ("mpgmad: seeking to %d(%xH) start offset\n", info->buffer.startoffset, info->buffer.startoffset);
            deadbeef->fseek (info->buffer.file, info->buffer.startoffset, SEEK_SET);
        }
        plugin.seek_sample (_info, 0);
        trace ("mp3: startsample: %d, endsample: %d, currentsample: %d\n", info->buffer.startsample, info->buffer.endsample, info->buffer.currentsample);
    }
    else {
        deadbeef->fset_track (info->buffer.file, it);
        deadbeef->pl_delete_meta (info->buffer.it, ":FILETYPE");
        int64_t len = deadbeef->fgetlength (info->buffer.file);
        if (len > 0) {
            deadbeef->pl_delete_all_meta (it);
            int v2err = deadbeef->junk_id3v2_read (it, info->buffer.file);
            if (v2err != 0) {
                deadbeef->fseek (info->buffer.file, 0, SEEK_SET);
            }
        }
        deadbeef->pl_add_meta (it, "title", NULL);
        int res = cmp3_scan_stream (&info->buffer, 0);
        if (res < 0) {
            trace ("mpgmad: cmp3_init: initial cmp3_scan_stream failed\n");
            return -1;
        }
        deadbeef->fseek (info->buffer.file, 0, SEEK_SET);

        cmp3_set_extra_properties (&info->buffer);

        ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
        deadbeef->plt_set_item_duration (plt, it, info->buffer.duration);
        if (plt) {
            deadbeef->plt_unref (plt);
        }
        if (info->buffer.duration >= 0) {
            info->buffer.endsample = info->buffer.totalsamples - 1;
        }
        else {
//            info->buffer.duration = 200;
//            info->buffer.totalsamples = 10000000;
//            info->buffer.endsample = info->buffer.totalsamples-1;
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
        trace ("bad mpeg file: %f\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }
    _info->fmt.bps = info->buffer.bitspersample;
    _info->fmt.samplerate = info->buffer.samplerate;
    _info->fmt.channels = info->buffer.channels;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    trace ("mp3 format: bps:%d sr:%d channels:%d\n", _info->fmt.bps, _info->fmt.samplerate, _info->fmt.channels);

	mad_stream_init(&info->stream);
	mad_stream_options (&info->stream, MAD_OPTION_IGNORECRC);
	mad_frame_init(&info->frame);
	mad_synth_init(&info->synth);

    return 0;
}

/****************************************************************************
 * Converts a sample from libmad's fixed point number format to a signed	*
 * short (16 bits).															*
 ****************************************************************************/
static inline int16_t
MadFixedToSshort(mad_fixed_t Fixed)
{
	/* A fixed point number is formed of the following bit pattern:
	 *
	 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * MSB                          LSB
	 * S ==> Sign (0 is positive, 1 is negative)
	 * W ==> Whole part bits
	 * F ==> Fractional part bits
	 *
	 * This pattern contains MAD_F_FRACBITS fractional bits, one
	 * should alway use this macro when working on the bits of a fixed
	 * point number. It is not guaranteed to be constant over the
	 * different platforms supported by libmad.
	 *
	 * The signed short value is formed, after clipping, by the least
	 * significant whole part bit, followed by the 15 most significant
	 * fractional part bits. Warning: this is a quick and dirty way to
	 * compute the 16-bit number, madplay includes much better
	 * algorithms.
	 */

	/* Clipping */
	if(Fixed>=MAD_F_ONE)
		return(32767);
	if(Fixed<=-MAD_F_ONE)
		return(-32768);

	/* Conversion. */
	Fixed=Fixed>>(MAD_F_FRACBITS-15);
	return((signed short)Fixed);
}

static inline float
MadFixedToFloat (mad_fixed_t Fixed) {
    return (float)((Fixed) / (float)(1L << MAD_F_FRACBITS));
}

#define MadErrorString(x) mad_stream_errorstr(x)

static inline void
cmp3_skip (mpgmad_info_t *info) {
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
cmp3_decode_requested_int16 (mpgmad_info_t *info) {
    cmp3_skip (info);
    if (info->buffer.skipsamples > 0) {
        return;
    }
    // copy synthesized samples into readbuffer
    int idx = info->synth.pcm.length-info->buffer.decode_remaining;

    // stereo
    if (MAD_NCHANNELS(&info->frame.header) == 2 && info->info.fmt.channels == 2) {
        while (info->buffer.decode_remaining > 0 && info->buffer.readsize > 0) {
            *((int16_t*)info->buffer.out) = MadFixedToSshort (info->synth.pcm.samples[0][idx]);
            info->buffer.readsize -= 2;
            info->buffer.out += 2;
            *((int16_t*)info->buffer.out) = MadFixedToSshort (info->synth.pcm.samples[1][idx]);
            info->buffer.readsize -= 2;
            info->buffer.out += 2;
            info->buffer.decode_remaining--;
            idx++;
        }
    }
    // mono
    else if (MAD_NCHANNELS(&info->frame.header) == 1 && info->info.fmt.channels == 1){
        while (info->buffer.decode_remaining > 0 && info->buffer.readsize > 0) {
            *((int16_t*)info->buffer.out) = MadFixedToSshort (info->synth.pcm.samples[0][idx]);
            info->buffer.readsize -= 2;
            info->buffer.out += 2;
            info->buffer.decode_remaining--;
            idx++;
        }
    }
    // workaround for bad mp3s that have both mono and stereo frames
    else if (MAD_NCHANNELS(&info->frame.header) == 1 && info->info.fmt.channels == 2) {
        while (info->buffer.decode_remaining > 0 && info->buffer.readsize > 0) {
            int16_t sample = MadFixedToSshort (info->synth.pcm.samples[0][idx]);
            *((int16_t*)info->buffer.out) = sample;
            info->buffer.readsize -= 2;
            info->buffer.out += 2;
            *((int16_t*)info->buffer.out) = sample;
            info->buffer.readsize -= 2;
            info->buffer.out += 2;
            info->buffer.decode_remaining--;
            idx++;
        }
    }
    else if (MAD_NCHANNELS(&info->frame.header) == 2 && info->info.fmt.channels == 1) {
        while (info->buffer.decode_remaining > 0 && info->buffer.readsize > 0) {
            int16_t sample = MadFixedToSshort (info->synth.pcm.samples[0][idx]);
            *((int16_t*)info->buffer.out) = sample;
            info->buffer.readsize -= 2;
            info->buffer.out += 2;
            info->buffer.decode_remaining--;
            idx++;
        }
    }
    assert (info->buffer.readsize >= 0);
}

static int
cmp3_stream_frame (mpgmad_info_t *info) {
    int eof = 0;
    while (!eof && (info->stream.buffer == NULL || info->buffer.decode_remaining <= 0)) {
        // read more MPEG data if needed
        if(info->stream.buffer==NULL || info->stream.error==MAD_ERROR_BUFLEN) {
            // copy part of last frame to beginning
            if (info->stream.next_frame && info->stream.bufend <= info->stream.next_frame) {
                eof = 1;
                break;
            }
            if (info->stream.next_frame != NULL) {
                info->buffer.remaining = info->stream.bufend - info->stream.next_frame;
                memmove (info->buffer.input, info->stream.next_frame, info->buffer.remaining);
            }
            int size = READBUFFER - info->buffer.remaining;
            int bytesread = 0;
            uint8_t *bytes = info->buffer.input + info->buffer.remaining;
            bytesread = deadbeef->fread (bytes, 1, size, info->buffer.file);
            if (!bytesread) {
                // add guard
                eof = 1;
                memset (bytes, 0, 8);
                bytesread = 8;
            }
            if (bytesread < size) {
                // end of file
                size -= bytesread;
                bytes += bytesread;
            }
            bytesread += info->buffer.remaining;
            mad_stream_buffer(&info->stream,info->buffer.input,bytesread);
            if (info->stream.buffer==NULL) {
                // check sync bits
                if (bytes[0] != 0xff || (bytes[1]&(3<<5)) != (3<<5)) {
                    trace ("mpgmad: read didn't start at frame boundary!\ncmp3_scan_stream is broken\n");
                }
                else {
                    trace ("mpgmad: streambuffer=NULL\n");
                }
            }
        }
        info->stream.error=0;

        // decode next frame
        if(mad_frame_decode(&info->frame,&info->stream))
        {
            if(MAD_RECOVERABLE(info->stream.error))
            {
                if(info->stream.error!=MAD_ERROR_LOSTSYNC) {
//                    printf ("mpgmad: recoverable frame level error (%s)\n", MadErrorString(&info->stream));
                }
                if (info->buffer.lead_in_frames > 0) {
                    info->buffer.lead_in_frames--;
                }
                continue;
            }
            else {
                if(info->stream.error==MAD_ERROR_BUFLEN) {
//                    printf ("mpgmad: recoverable frame level error (%s)\n", MadErrorString(&info->stream));
                    continue;
                }
                else
                {
//                    printf ("mpgmad: unrecoverable frame level error (%s).\n", MadErrorString(&info->stream));
                    return -1; // fatal error
                }
            }
        }
        mad_synth_frame(&info->synth,&info->frame);
        if (info->buffer.lead_in_frames > 0) {
            info->buffer.lead_in_frames--;
            info->buffer.decode_remaining = 0;
            continue;
        }

        info->info.fmt.samplerate = info->frame.header.samplerate;

        // synthesize single frame
        info->buffer.decode_remaining = info->synth.pcm.length;
        deadbeef->streamer_set_bitrate (info->frame.header.bitrate/1000);
        break;
    }

    return eof;
}

static int
cmp3_decode_int16 (mpgmad_info_t *info) {
    int eof = 0;
    while (!eof) {
        eof = cmp3_stream_frame (info);
        if (info->buffer.decode_remaining > 0) {
            int readsize = info->buffer.readsize;
            cmp3_decode_requested_int16 (info);
            if (info->buffer.readsize == 0) {
                return 0;
            }
        }
    }
    return 0;
}

static void
cmp3_free (DB_fileinfo_t *_info) {
    mpgmad_info_t *info = (mpgmad_info_t *)_info;
    if (info->buffer.it) {
        deadbeef->pl_item_unref (info->buffer.it);
    }
    if (info->buffer.file) {
        deadbeef->fclose (info->buffer.file);
        info->buffer.file = NULL;
        info->info.file = NULL;
        mad_synth_finish (&info->synth);
        mad_frame_finish (&info->frame);
        mad_stream_finish (&info->stream);
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
    mpgmad_info_t *info = (mpgmad_info_t *)_info;
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
    info->buffer.readsize = size;
    info->buffer.out = bytes;
    cmp3_decode_int16 (info);
    info->buffer.currentsample += (size - info->buffer.readsize) / samplesize;
    _info->readpos = (float)(info->buffer.currentsample - info->buffer.startsample) / info->buffer.samplerate;
#if WRITE_DUMP
    if (size - info->buffer.readsize > 0) {
        fwrite (bytes, 1, size - info->buffer.readsize, out);
    }
#endif
//    if (initsize-info->buffer.readsize != size) {
//        printf ("\033[0;31meof at sample %d\033[37;0m\n", info->buffer.currentsample);
//    }
    return initsize - info->buffer.readsize;
}

static int
cmp3_seek_sample (DB_fileinfo_t *_info, int sample) {
    mpgmad_info_t *info = (mpgmad_info_t *)_info;
    if (!info->buffer.file) {
        return -1;
    }

// {{{ handle net streaming case
    if (info->buffer.file->vfs->is_streaming ()) {
        if (info->buffer.totalsamples > 0 && info->buffer.avg_samples_per_frame > 0 && info->buffer.avg_packetlength > 0) { // that means seekable remote stream, like podcast
            trace ("seeking is possible!\n");
            // get length excluding id3v2
            int64_t l = deadbeef->fgetlength (info->buffer.file) - info->buffer.startoffset - info->buffer.endoffset;
            
            int r;

            // seek to beginning of the frame
            int64_t frm = sample / info->buffer.avg_samples_per_frame;
            r = deadbeef->fseek (info->buffer.file, frm * info->buffer.avg_packetlength + info->buffer.startoffset, SEEK_SET);

//            l = l * sample / buffer.totalsamples;
//            r = deadbeef->fseek (buffer.file, l, SEEK_SET);

            if (!r) {
                info->buffer.skipsamples = sample - frm * info->buffer.avg_samples_per_frame;

                info->buffer.currentsample = sample;
                _info->readpos = (float)(info->buffer.currentsample - info->buffer.startsample) / info->buffer.samplerate;

                // reset mad
                mad_synth_finish (&info->synth);
                mad_frame_finish (&info->frame);
                mad_stream_finish (&info->stream);
                info->buffer.remaining = 0;
                info->buffer.decode_remaining = 0;
                mad_stream_init(&info->stream);
                mad_stream_options (&info->stream, MAD_OPTION_IGNORECRC);
                mad_frame_init(&info->frame);
                mad_synth_init(&info->synth);

                return 0;
            }
            trace ("seek failed!\n");
            return -1;
        }
        trace ("seek is impossible (avg_samples_per_frame=%d, avg_packetlength=%d)!\n", info->buffer.avg_samples_per_frame, info->buffer.avg_packetlength);
        return 0;
    }
// }}}

    sample += info->buffer.startsample;
    if (sample > info->buffer.endsample) {
        trace ("seek sample %d is beyond end of track (%d)\n", sample, info->buffer.endsample);
        return -1; // eof
    }
    // restart file, and load until we hit required pos
    deadbeef->fseek (info->buffer.file, info->buffer.startoffset, SEEK_SET);
    mad_synth_finish (&info->synth);
    mad_frame_finish (&info->frame);
    mad_stream_finish (&info->stream);
    info->buffer.remaining = 0;
    info->buffer.readsize = 0;
    info->buffer.decode_remaining = 0;
	mad_stream_init(&info->stream);
	mad_stream_options (&info->stream, MAD_OPTION_IGNORECRC);
	mad_frame_init(&info->frame);
	mad_synth_init(&info->synth);

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
    mpgmad_info_t *info = (mpgmad_info_t *)_info;
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
    int skip = deadbeef->junk_get_leading_size (buffer.file);
    if (skip > 0) {
        trace ("mpgmad: skipping %d bytes (tag)\n", skip);
        deadbeef->fseek(buffer.file, skip, SEEK_SET);
    }
    // calc approx. mp3 duration 
    int res = cmp3_scan_stream (&buffer, 0);
    if (res < 0) {
        trace ("mpgmad: cmp3_scan_stream returned error\n");
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
    cmp3_set_extra_properties (&buffer);

    deadbeef->plt_set_item_duration (plt, it, buffer.duration);
    deadbeef->fclose (fp);

    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        DB_playItem_t *last = deadbeef->plt_insert_cue_from_buffer (plt, after, it, cuesheet, strlen (cuesheet), buffer.totalsamples-buffer.delay-buffer.padding, buffer.samplerate);
        if (last) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (last);
            return last;
        }
    }


    // FIXME! bad numsamples passed to cue
    DB_playItem_t *cue_after = deadbeef->plt_insert_cue (plt, after, it, buffer.totalsamples-buffer.delay-buffer.padding, buffer.samplerate);
    if (cue_after) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue_after);
        return cue_after;
    }

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

int
cmp3_read_metadata (DB_playItem_t *it) {
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
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
    deadbeef->conf_get_str ("mp3.id3v1_encoding", "iso8859-1", id3v1_encoding, sizeof (id3v1_encoding));
    return deadbeef->junk_rewrite_tags (it, junk_flags, id3v2_version, id3v1_encoding);
}

static const char *exts[] = {
	"mp1", "mp2", "mp3", NULL
};

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stdmpg",
    .plugin.name = "MPEG decoder",
    .plugin.descr = "MPEG v1/2 layer1/2/3 decoder based on libmad",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
mpgmad_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
