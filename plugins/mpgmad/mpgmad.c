/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../deadbeef.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

#define READBUFFER 0x2800 // 10k is enough for single frame

// vbrmethod constants
#define LAME_CBR  1 
#define LAME_CBR2 8
#define LAME_ABR  2
#define LAME_VBR1 3
#define LAME_VBR2 4
#define LAME_VBR3 5
#define LAME_VBR4 6

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
    float frameduration;
    int bitspersample;
    int channels;
    float duration;
    int currentsample;
    int totalsamples;
    int skipsamples;
    int startoffset;
    int endoffset;
    int startsample;
    int endsample;
    int startdelay;
    int enddelay;
} buffer_t;

static buffer_t buffer;
static struct mad_stream stream;
static struct mad_frame frame;
static struct mad_synth synth;

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
//    if (sample == 0) {
//        sample = -1;
//    }
    int packetlength = 0;
    int nframe = 0;
    int got_xing_header = 0;
    buffer->duration = 0;
    int scansamples = 0;
    buffer->currentsample = 0;
    buffer->skipsamples = 0;
    int fsize = 0;
    int avg_packetlength = 0;
    int avg_samplerate = 0;
    int avg_samples_per_frame = 0;

    if (sample <= 0) {
        buffer->totalsamples = 0;
        fsize = deadbeef->fgetlength (buffer->file);
    }

    for (;;) {
        uint32_t hdr;
        uint8_t sync;
        //size_t pos = deadbeef->ftell (buffer->file);
        if (deadbeef->fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        if (sync != 0xff) {
            trace ("[1]frame %d didn't seek to frame end\n", nframe);
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            if (deadbeef->fread (&sync, 1, 1, buffer->file) != 1) {
                break; // eof
            }
            if ((sync >> 5) != 7) {
                trace ("[2]frame %d didn't seek to frame end\n", nframe);
                continue;
            }
        }
        // found frame
        hdr = (0xff<<24) | (sync << 16);
        // read 2 bytes more
        if (deadbeef->fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        hdr |= sync << 8;
        if (deadbeef->fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        hdr |= sync;

        // parse header
        
        // sync bits
        int usync = hdr & 0xffe00000;
        if (usync != 0xffe00000) {
            fprintf (stderr, "fatal error: mp3 header parser is broken\n");
        }

        // mpeg version
        static int vertbl[] = {3, -1, 2, 1}; // 3 is 2.5
        int ver = (hdr & (3<<19)) >> 19;
        ver = vertbl[ver];
        if (ver < 0) {
            trace ("frame %d bad mpeg version %d\n", nframe, (hdr & (3<<19)) >> 19);
            continue; // invalid frame
        }

        // layer info
        static int ltbl[] = { -1, 3, 2, 1 };
        int layer = (hdr & (3<<17)) >> 17;
        layer = ltbl[layer];
        if (layer < 0) {
            trace ("frame %d bad layer %d\n", nframe, (hdr & (3<<17)) >> 17);
            continue; // invalid frame
        }

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
            continue; // invalid frame
        }

        // padding
        int padding = (hdr & (0x1 << 9)) >> 9;

        static int chantbl[4] = { 2, 2, 2, 1 };
        int nchannels = (hdr & (0x3 << 6)) >> 6;
        nchannels = chantbl[nchannels];

        // check if channel/bitrate combination is valid for layer2
        if (layer == 2) {
            if ((bitrate <= 56 || bitrate == 80) && nchannels != 1) {
                trace ("[1]frame %d channel/bitrate combination is bad\n", nframe);
                continue; // bad frame
            }
            if (bitrate >= 224 && nchannels == 1) {
                trace ("[2]frame %d channel/bitrate combination is bad\n", nframe);
                continue; // bad frame
            }
        }

        // packetlength
        packetlength = 0;
        bitrate *= 1000;
        float dur = 0;
        int samples_per_frame = 0;
        if (samplerate > 0 && bitrate > 0) {
            if (layer == 1) {
                samples_per_frame = 384;
                dur = (float)384 / samplerate;
            }
            else if (layer == 2) {
                samples_per_frame = 1152;
                dur = (float)1152 / samplerate;
            }
            else if (layer == 3) {
                if (ver == 1) {
                    samples_per_frame = 1152;
                    dur = (float)1152 / samplerate;
                }
                else {
                    samples_per_frame = 576;
                    dur = (float)576 / samplerate;
                }
            }
            packetlength = samples_per_frame / 8 * bitrate / samplerate + padding;
        }
        else {
            trace ("frame %d samplerate or bitrate is invalid\n", nframe);
            continue;
        }

        if (sample != 0 || nframe == 0)
        {
            buffer->version = ver;
            buffer->layer = layer;
            buffer->bitrate = bitrate;
            buffer->samplerate = samplerate;
            buffer->packetlength = packetlength;
            buffer->frameduration = dur;
            buffer->channels = nchannels;
            buffer->bitspersample = 16;
            //trace ("frame %d(@%d) mpeg v%d layer %d bitrate %d samplerate %d packetlength %d framedur %f channels %d\n", nframe, pos, ver, layer, bitrate, samplerate, packetlength, dur, nchannels);
        }
        // try to read xing/info tag (only on initial scans)
        if (sample <= 0 && !got_xing_header)
        {
            size_t framepos = deadbeef->ftell (buffer->file);
            trace ("trying to read xing header\n");
            if (ver == 1) {
                deadbeef->fseek (buffer->file, 32, SEEK_CUR);
            }
            else {
                deadbeef->fseek (buffer->file, 17, SEEK_CUR);
            }
            const char xing[] = "Xing";
            const char info[] = "Info";
            char magic[4];
            if (deadbeef->fread (magic, 1, 4, buffer->file) != 4) {
                return -1; // EOF
            }
            // add information to skip this frame
            int startoffset = deadbeef->ftell (buffer->file) + packetlength;
            if (startoffset > buffer->startoffset) {
                buffer->startoffset = startoffset;
            }

            trace ("xing magic: %c%c%c%c\n", magic[0], magic[1], magic[2], magic[3]);

            if (!strncmp (xing, magic, 4) || !strncmp (info, magic, 4)) {
                trace ("xing/info frame found\n");
                // read flags
                uint32_t flags;
                uint8_t buf[4];
                if (deadbeef->fread (buf, 1, 4, buffer->file) != 4) {
                    return -1; // EOF
                }
                flags = extract_i32 (buf);
                if (flags & FRAMES_FLAG) {
                    // read number of frames
                    if (deadbeef->fread (buf, 1, 4, buffer->file) != 4) {
                        return -1; // EOF
                    }
                    uint32_t nframes = extract_i32 (buf);
                    buffer->duration = (float)nframes * (float)samples_per_frame / (float)samplerate;
                    buffer->totalsamples = nframes * samples_per_frame;
                    trace ("xing totalsamples: %d\n", buffer->totalsamples);
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
                    return -1; // EOF
                }
                trace ("tell=%x, %c%c%c%c\n", deadbeef->ftell(buffer->file), buf[0], buf[1], buf[2], buf[3]);
                if (!memcmp (buf, "LAME", 4)) {
                    trace ("lame header found\n");
                    deadbeef->fseek (buffer->file, 6, SEEK_CUR);

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
                    uint32_t startdelay = (((uint32_t)buf[0]) << 4) | ((((uint32_t)buf[1]) & 0xf0)>>4);
                    uint32_t enddelay = ((((uint32_t)buf[1])&0x0f)<<8) | ((uint32_t)buf[2]);
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

                    trace ("lpf: %d, peaksignalamp: %f, radiogain: %d, audiophile: %d, startdelay: %d, enddelay: %d, mp3gain: %d, musiclen: %d\n", lpf, rg_peaksignalamp, rg_radio, rg_audiophile, startdelay, enddelay, mp3gain, musiclen);
                    // skip crc
                    //deadbeef->fseek (buffer->file, 4, SEEK_CUR);
                    buffer->startdelay = startdelay;
                    buffer->enddelay = enddelay;
                }
                if (sample <= 0 && (flags&FRAMES_FLAG)) {
                    buffer->totalsamples -= buffer->enddelay;
                    trace ("lame totalsamples: %d\n", buffer->totalsamples);
                    deadbeef->fseek (buffer->file, framepos+packetlength-4, SEEK_SET);
                    return 0;
                }
            }
            if (sample == 0) {
                // xing header failed, calculate based on file size
                trace ("xing header failed\n");
                buffer->samplerate = samplerate;
                if (buffer->file->vfs->streaming) {
                    // only suitable for cbr files, used if streaming
                    int sz = deadbeef->fgetlength (buffer->file) - buffer->startoffset - buffer->endoffset;
                    if (sz < 0) {
                        buffer->duration = -1;
                        buffer->totalsamples = -1;
                        if (sample == 0) {
                            deadbeef->fseek (buffer->file, framepos+packetlength-4, SEEK_SET);
                        }
                        return 0;
                    }
                    int nframes = sz / packetlength;
                    buffer->duration = nframes * samples_per_frame / samplerate;
                    buffer->totalsamples = nframes * samples_per_frame;
//                    trace ("bitrate=%d, layer=%d, packetlength=%d, fsize=%d, nframes=%d, samples_per_frame=%d, samplerate=%d, duration=%f, totalsamples=%d\n", bitrate, layer, packetlength, sz, nframes, samples_per_frame, samplerate, buffer->duration, buffer->totalsamples);

                    if (sample == 0) {
                        deadbeef->fseek (buffer->file, framepos+packetlength-4, SEEK_SET);
                        return 0;
                    }
                }
            }
            else {
                deadbeef->fseek (buffer->file, framepos+packetlength-4, SEEK_SET);
                got_xing_header = 1;
            }
        }

        if (sample == 0) {
            if (fsize <= 0) {
                return -1;
            }
            // calculating apx duration based on 1st 100 frames
            avg_packetlength += packetlength;
            avg_samplerate += samplerate;
            avg_samples_per_frame += samples_per_frame;
            if (nframe >= 100) {
                avg_packetlength /= nframe;
                avg_samplerate /= nframe;
                avg_samples_per_frame /= nframe;

                trace ("avg_packetlength=%d, avg_samplerate=%d, avg_samples_per_frame=%d\n", avg_packetlength, avg_samplerate, avg_samples_per_frame);

                int nframes = fsize / avg_packetlength;
                buffer->duration = nframes * avg_samples_per_frame / avg_samplerate;
                buffer->totalsamples = nframes * avg_samples_per_frame;
                return 0;
            }
        }
        else {
            if (sample > 0 && scansamples + samples_per_frame >= sample) {
                deadbeef->fseek (buffer->file, -4, SEEK_CUR);
                buffer->currentsample = sample;
                buffer->skipsamples = sample - scansamples;
                return 0;
            }
        }
        scansamples += samples_per_frame;
        buffer->duration += dur;
        nframe++;
        if (packetlength > 0) {
            deadbeef->fseek (buffer->file, packetlength-4, SEEK_CUR);
        }
    }
    if (nframe == 0) {
        return -1;
    }
    buffer->totalsamples = scansamples;
//    buffer->duration = buffer->totalsamples / buffer->samplerate;
    trace ("nframes=%d, totalsamples=%d, samplerate=%d, dur=%f\n", nframe, scansamples, buffer->samplerate, buffer->duration);
    return 0;
}


static int
cmp3_init (DB_playItem_t *it) {
    memset (&buffer, 0, sizeof (buffer));
    buffer.file = deadbeef->fopen (it->fname);
    if (!buffer.file) {
        return -1;
    }
    buffer.it = it;
    plugin.info.readpos = 0;
    if (!buffer.file->vfs->streaming) {
        int skip = deadbeef->junk_get_leading_size (buffer.file);
        if (skip > 0) {
            deadbeef->fseek(buffer.file, skip, SEEK_SET);
        }
        cmp3_scan_stream (&buffer, -1); // scan entire stream, calc duration
        if (it->endsample > 0) {
            buffer.startsample = it->startsample;
            buffer.endsample = it->endsample;
            // that comes from cue, don't calc duration, just seek and play
            plugin.seek_sample (0);
        }
        else {
            deadbeef->pl_set_item_duration (it, buffer.duration);
            buffer.startsample = 0;
            buffer.endsample = buffer.totalsamples-1;
            buffer.skipsamples = buffer.startdelay;
            buffer.currentsample = buffer.startdelay;
            deadbeef->fseek (buffer.file, buffer.startoffset, SEEK_SET);
        }
    }
    else {
        buffer.it->filetype = NULL;
        int len = deadbeef->fgetlength (buffer.file);
        const char *name = deadbeef->fget_content_name (buffer.file);
        const char *genre = deadbeef->fget_content_genre (buffer.file);
        if (len > 0) {
            deadbeef->pl_delete_all_meta (it);
            int v2err = deadbeef->junk_read_id3v2 (it, buffer.file);
            deadbeef->pl_add_meta (it, "title", NULL);
            if (v2err != 0) {
                deadbeef->fseek (buffer.file, 0, SEEK_SET);
            }
        }
        else {
            deadbeef->pl_delete_all_meta (it);
            if (name) {
                deadbeef->pl_add_meta (it, "title", name);
            }
            else {
                deadbeef->pl_add_meta (it, "title", NULL);
            }
            if (genre) {
                deadbeef->pl_add_meta (it, "genre", genre);
            }
        }
        int res = cmp3_scan_stream (&buffer, 0);
        if (res < 0) {
            trace ("mpgmad: cmp3_init: initial cmp3_scan_stream failed\n");
            plugin.free ();
            return -1;
        }
        deadbeef->pl_set_item_duration (it, buffer.duration);
        if (buffer.duration >= 0) {
            buffer.endsample = buffer.totalsamples - 1;
        }
        else {
//            buffer.duration = 200;
//            buffer.totalsamples = 10000000;
//            buffer.endsample = buffer.totalsamples-1;
            buffer.endsample = -1;
            buffer.totalsamples = -1;
        }
        buffer.skipsamples = 0;
        buffer.currentsample = 0;
        if (buffer.duration < 0) {
            buffer.duration = -1;
            buffer.totalsamples = -1;
            buffer.endsample = -1;
        }
        trace ("duration=%f, endsample=%d, totalsamples=%d\n", buffer.duration, buffer.endsample, buffer.totalsamples);
    }
    if (buffer.samplerate == 0) {
        trace ("bad mpeg file: %f\n", it->fname);
        plugin.free ();
        return -1;
    }
    plugin.info.bps = buffer.bitspersample;
    plugin.info.samplerate = buffer.samplerate;
    plugin.info.channels = buffer.channels;

	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);

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

// cuts readsize if it's beyond boundaries
static int
cmp3_decode_cut (int framesize) {
    if (buffer.duration >= 0) {
        if (buffer.currentsample + buffer.readsize / (framesize * buffer.channels) > buffer.endsample) {
            int sz = (buffer.endsample - buffer.currentsample + 1) * framesize * buffer.channels;
            trace ("size truncated to %d bytes, cursample=%d, endsample=%d, totalsamples=%d\n", buffer.readsize, buffer.currentsample, buffer.endsample, buffer.totalsamples);
            if (sz <= 0) {
                return 1;
            }
            buffer.readsize = sz;
        }
    }
    return 0;
}

static inline void
cmp3_skip (void) {
    if (buffer.skipsamples > 0) {
        int skip = min (buffer.skipsamples, buffer.decode_remaining);
        buffer.skipsamples -= skip;
        buffer.decode_remaining -= skip;
    }
}

// decoded requested number of samples to int16 format
static void
cmp3_decode_requested_int16 (void) {
    cmp3_skip ();
    // copy synthesized samples into readbuffer
    int idx = synth.pcm.length-buffer.decode_remaining;
    while (buffer.decode_remaining > 0 && buffer.readsize > 0) {
        *((int16_t*)buffer.out) = MadFixedToSshort (synth.pcm.samples[0][idx]);
        buffer.readsize -= 2;
        buffer.out += 2;
        if (MAD_NCHANNELS(&frame.header) == 2) {
            *((int16_t*)buffer.out) = MadFixedToSshort (synth.pcm.samples[1][idx]);
            buffer.readsize -= 2;
            buffer.out += 2;
        }
        buffer.decode_remaining--;
        idx++;
    }
    assert (buffer.readsize >= 0);
}

// decoded requested number of samples to int16 format
static void
cmp3_decode_requested_float32 (void) {
    cmp3_skip ();
    // copy synthesized samples into readbuffer
    int idx = synth.pcm.length-buffer.decode_remaining;
    while (buffer.decode_remaining > 0 && buffer.readsize > 0) {
        *((float*)buffer.out) = MadFixedToFloat (synth.pcm.samples[0][idx]);
        buffer.readsize -= 4;
        buffer.out += 4;
        if (MAD_NCHANNELS(&frame.header) == 2) {
            *((float*)buffer.out) = MadFixedToFloat (synth.pcm.samples[1][idx]);
            buffer.readsize -= 4;
            buffer.out += 4;
        }
        buffer.decode_remaining--;
        idx++;
    }
    assert (buffer.readsize >= 0);
}

static int
cmp3_stream_frame (void) {
    int eof = 0;
    while (!eof && (stream.buffer == NULL || buffer.decode_remaining <= 0)) {
        // read more MPEG data if needed
        if(stream.buffer==NULL || stream.error==MAD_ERROR_BUFLEN) {
            // copy part of last frame to beginning
            if (stream.next_frame != NULL) {
                buffer.remaining = stream.bufend - stream.next_frame;
                memmove (buffer.input, stream.next_frame, buffer.remaining);
            }
            int size = READBUFFER - buffer.remaining;
            int bytesread = 0;
            uint8_t *bytes = buffer.input + buffer.remaining;
            bytesread = deadbeef->fread (bytes, 1, size, buffer.file);
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
            bytesread += buffer.remaining;
            mad_stream_buffer(&stream,buffer.input,bytesread);
            if (stream.buffer==NULL) {
                // check sync bits
                if (bytes[0] != 0xff || (bytes[1]&(3<<5)) != (3<<5)) {
                    trace ("mpgmad: read didn't start at frame boundary!\ncmp3_scan_stream is broken\n");
                }
            }
        }
        stream.error=0;
        // decode next frame
        if(mad_frame_decode(&frame,&stream))
        {
            if(MAD_RECOVERABLE(stream.error))
            {
#if 0
                if(stream.error!=MAD_ERROR_LOSTSYNC) {
                    trace ("mpgmad: recoverable frame level error (%s)\n", MadErrorString(&stream));
                }
#endif
                continue;
            }
            else {
                if(stream.error==MAD_ERROR_BUFLEN) {
                    continue;
                }
                else
                {
                    trace ("mpgmad: unrecoverable frame level error (%s).\n", MadErrorString(&stream));
                    return -1; // fatal error
                }
            }
        }

        if (!buffer.it->filetype) {
            int layer = frame.header.layer;
            if (layer >= 1 && layer <= 3) {
                buffer.it->filetype = plugin.filetypes[layer-1];
            }
        }

        plugin.info.samplerate = frame.header.samplerate;
        plugin.info.channels = MAD_NCHANNELS(&frame.header);

        // synthesize single frame
        mad_synth_frame(&synth,&frame);
        buffer.decode_remaining = synth.pcm.length;
        deadbeef->streamer_set_bitrate (frame.header.bitrate/1000);
        break;
    }
    return eof;
}

static int
cmp3_decode_int16 (void) {
    if (cmp3_decode_cut (4)) {
        return 0;
    }
    int eof = 0;
    while (!eof) {
        eof = cmp3_stream_frame ();
        if (buffer.decode_remaining > 0) {
            cmp3_decode_requested_int16 ();
            if (buffer.readsize == 0) {
                return 0;
            }
        }
    }
    return 0;
}

static int
cmp3_decode_float32 (void) {
    if (cmp3_decode_cut (8)) {
        trace ("read request ignored (end of track passed)\n");
        return 0;
    }
    int eof = 0;
    while (!eof) {
        eof = cmp3_stream_frame ();
        if (buffer.decode_remaining > 0) {
            cmp3_decode_requested_float32 ();
            if (buffer.readsize == 0) {
                return 0;
            }
        }
    }
    return 0;
}

static void
cmp3_free (void) {
    if (buffer.file) {
        deadbeef->fclose (buffer.file);
        buffer.file = NULL;
        mad_synth_finish (&synth);
        mad_frame_finish (&frame);
        mad_stream_finish (&stream);
    }
}

static int
cmp3_read_int16 (char *bytes, int size) {
    buffer.readsize = size;
    buffer.out = bytes;
    cmp3_decode_int16 ();
    buffer.currentsample += (size - buffer.readsize) / 4;
    plugin.info.readpos = (float)(buffer.currentsample - buffer.startsample) / buffer.samplerate;
    return size - buffer.readsize;
}

static int
cmp3_read_float32 (char *bytes, int size) {
    buffer.readsize = size;
    buffer.out = bytes;
    cmp3_decode_float32 ();
    buffer.currentsample += (size - buffer.readsize) / 8;
    plugin.info.readpos = (float)(buffer.currentsample - buffer.startsample) / buffer.samplerate;
    return size - buffer.readsize;
}

static int
cmp3_seek_sample (int sample) {
    if (!buffer.file) {
        return -1;
    }

    if (buffer.file->vfs->streaming) {
        if (buffer.totalsamples > 0) {
            // approximation
            int64_t l = deadbeef->fgetlength (buffer.file);
            l = l * sample / buffer.totalsamples;
            int r = deadbeef->fseek (buffer.file, l, SEEK_SET);
            if (!r) {
                buffer.currentsample = sample;
                plugin.info.readpos = (float)(buffer.currentsample - buffer.startsample) / buffer.samplerate;

                mad_synth_finish (&synth);
                mad_frame_finish (&frame);
                mad_stream_finish (&stream);
                buffer.remaining = 0;
                buffer.decode_remaining = 0;
                mad_stream_init(&stream);
                mad_frame_init(&frame);
                mad_synth_init(&synth);

                return 0;
            }
            return -1;
        }
        return 0;
    }

    sample += buffer.startsample + buffer.startdelay;
    if (sample > buffer.endsample) {
        trace ("seek sample %d is beyond end of track (%d)\n", sample, buffer.endsample);
        return -1; // eof
    }
    // restart file, and load until we hit required pos
    deadbeef->fseek (buffer.file, 0, SEEK_SET);
    int skip = deadbeef->junk_get_leading_size (buffer.file);
    if (skip > 0) {
        deadbeef->fseek(buffer.file, skip, SEEK_SET);
    }
    mad_synth_finish (&synth);
    mad_frame_finish (&frame);
    mad_stream_finish (&stream);
    buffer.remaining = 0;
    buffer.readsize = 0;
    buffer.decode_remaining = 0;

	if (sample == 0) { 
        plugin.info.readpos = 0;
        buffer.currentsample = 0;
        buffer.skipsamples = buffer.startdelay;
        return 0;
    }

    if (cmp3_scan_stream (&buffer, sample) == -1) {
        trace ("failed to seek to sample %d\n", sample);
        plugin.info.readpos = 0;
        return -1;
    }
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
    plugin.info.readpos = (float)(buffer.currentsample - buffer.startsample) / buffer.samplerate;
    return 0;
}

static int
cmp3_seek (float time) {
    int sample = time * buffer.samplerate;
    return cmp3_seek_sample (sample);
}

static const char *filetypes[] = {
    "MPEG 1.0 layer I", "MPEG 1.0 layer II", "MPEG 1.0 layer III", "MPEG 2.0 layer I", "MPEG 2.0 layer II", "MPEG 2.0 layer III", "MPEG 2.5 layer I", "MPEG 2.5 layer II", "MPEG 2.5 layer III", NULL
};

static DB_playItem_t *
cmp3_insert (DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    if (fp->vfs->streaming) {
        DB_playItem_t *it = deadbeef->pl_item_alloc ();
        it->decoder = &plugin;
        it->fname = strdup (fname);
        deadbeef->fclose (fp);
        deadbeef->pl_add_meta (it, "title", NULL);
        deadbeef->pl_set_item_duration (it, -1);
        it->filetype = NULL;//filetypes[0];
        after = deadbeef->pl_insert_item (after, it);
        return after;
    }
    buffer_t buffer;
    memset (&buffer, 0, sizeof (buffer));
    buffer.file = fp;
    int skip = deadbeef->junk_get_leading_size (buffer.file);
    if (skip > 0) {
        deadbeef->fseek(buffer.file, skip, SEEK_SET);
    }
    // calc approx. mp3 duration 
    int res = cmp3_scan_stream (&buffer, 0);
    if (res < 0) {
        deadbeef->fclose (fp);
        return NULL;
    }

    const char *ftype = NULL;
    if (buffer.version == 1) {
        switch (buffer.layer) {
        case 1:
            ftype = filetypes[0];
            break;
        case 2:
            ftype = filetypes[1];
            break;
        case 3:
            ftype = filetypes[2];
            break;
        }
    }
    else if (buffer.version == 2) {
        switch (buffer.layer) {
        case 1:
            ftype = filetypes[3];
            break;
        case 2:
            ftype = filetypes[4];
            break;
        case 3:
            ftype = filetypes[5];
            break;
        }
    }
    else {
        switch (buffer.layer) {
        case 1:
            ftype = filetypes[6];
            break;
        case 2:
            ftype = filetypes[7];
            break;
        case 3:
            ftype = filetypes[8];
            break;
        }
    }
    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);

    deadbeef->rewind (fp);
    /*int apeerr = */deadbeef->junk_read_ape (it, fp);
    /*int v2err = */deadbeef->junk_read_id3v2 (it, fp);
    /*int v1err = */deadbeef->junk_read_id3v1 (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->pl_set_item_duration (it, buffer.duration);
    it->filetype = ftype;
    deadbeef->fclose (fp);

    // FIXME! bad numsamples passed to cue
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, it, buffer.duration*buffer.samplerate, buffer.samplerate);
    if (cue_after) {
        deadbeef->pl_item_free (it);
        return cue_after;
    }

    after = deadbeef->pl_insert_item (after, it);
    return after;
}

static const char *exts[] = {
	"mp1", "mp2", "mp3", NULL
};

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "MPEG decoder",
    .plugin.descr = "MPEG v1/2 layer1/2/3 decoder based on libmad",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = cmp3_init,
    .free = cmp3_free,
    .read_int16 = cmp3_read_int16,
    .read_float32 = cmp3_read_float32,
    .seek = cmp3_seek,
    .seek_sample = cmp3_seek_sample,
    .insert = cmp3_insert,
    .exts = exts,
    .id = "stdmpg",
    .filetypes = filetypes
};

DB_plugin_t *
mpgmad_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
