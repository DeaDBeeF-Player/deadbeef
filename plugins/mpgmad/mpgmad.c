/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

#define READBUFFER 0x10000
#define READBUFFER_MASK 0xffff

// FIXME: cache is bad name for this
#define CACHE_SIZE 0x20000
#define CACHE_MASK 0x1ffff

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
    FILE *file;

//    // input buffer, for MPEG data
//    // FIXME: this should go away if reading happens per-frame
    char input[READBUFFER];
    int remaining;

    // NOTE: both "output" and "cache" buffers store sampels in libmad fixed point format

//    // output buffer, supplied by player
    int readsize;

    // cache, for extra decoded samples
    char cache[CACHE_SIZE];
    int cachefill;
    int cachepos;

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

static int
cmp3_decode (void);

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
    int nframe = 0;
    int got_xing_header = 0;
    buffer->duration = 0;
    int scansamples = 0;
    buffer->currentsample = 0;
    buffer->skipsamples = 0;

    if (sample <= 0) {
        buffer->totalsamples = 0;
    }

    for (;;) {
        uint32_t hdr;
        uint8_t sync;
        size_t pos = ftell (buffer->file);
        if (fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        if (sync != 0xff) {
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            if (fread (&sync, 1, 1, buffer->file) != 1) {
                break; // eof
            }
            if ((sync >> 5) != 7) {
                continue;
            }
        }
        // found frame
        hdr = (0xff<<24) | (sync << 16);
        // read 2 bytes more
        if (fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        hdr |= sync << 8;
        if (fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        hdr |= sync;

        // parse header
        
        // sync bits
        int usync = hdr & 0xffe00000;
        if (usync != 0xffe00000) {
            printf ("fatal error: mp3 header parser is broken\n");
        }

        // mpeg version
        static int vertbl[] = {3, -1, 2, 1}; // 3 is 2.5
        int ver = (hdr & (3<<19)) >> 19;
        ver = vertbl[ver];
        if (ver < 0) {
            continue; // invalid frame
        }

        // layer info
        static int ltbl[] = { -1, 3, 2, 1 };
        int layer = (hdr & (3<<17)) >> 17;
        layer = ltbl[layer];
        if (layer < 0) {
            continue; // invalid frame
        }

        // bitrate
        static int brtable[5][16] = {
            { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 },
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1 },
            { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 },
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1 },
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1 }
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
            continue; // invalid frame
        }

        // samplerate
        static int srtable[3][4] = {
            {44100, 48000, 32000, -1},
            {22050, 24000, 16000, -1},
            {11025, 12000, 8000, -1},
        };
        int samplerate = (hdr & (0x03<<10))>>10;
        samplerate = srtable[ver-1][samplerate];
        if (samplerate < 0) {
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
                continue; // bad frame
            }
            if (bitrate >= 224 && nchannels == 1) {
                continue; // bad frame
            }
        }

        // check if emphasis is valid
        if ((hdr & 3) == 2) {
            continue; // 10 is reserved
        }

        // packetlength
        int packetlength = 0;
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
//            fprintf (stderr, "frame %d(@%d) mpeg v%d layer %d bitrate %d samplerate %d packetlength %d framedur %f channels %d\n", nframe, pos, ver, layer, bitrate, samplerate, packetlength, dur, nchannels);
        }
        // try to read xing/info tag (only on initial scans)
        if (sample <= 0 && !got_xing_header)
        {
            size_t framepos = ftell (buffer->file);
            trace ("trying to read xing header\n");
            if (ver == 1) {
                fseek (buffer->file, 32, SEEK_CUR);
            }
            else {
                fseek (buffer->file, 17, SEEK_CUR);
            }
            const char xing[] = "Xing";
            const char info[] = "Info";
            char magic[4];
            if (fread (magic, 1, 4, buffer->file) != 4) {
                return -1; // EOF
            }
            // add information to skip this frame
            int startoffset = ftell (buffer->file) + packetlength;
            if (startoffset > buffer->startoffset) {
                buffer->startoffset = startoffset;
            }

            if (!strncmp (xing, magic, 4) || !strncmp (info, magic, 4)) {
                trace ("xing/info frame found\n");
                // read flags
                uint32_t flags;
                uint8_t buf[4];
                if (fread (buf, 1, 4, buffer->file) != 4) {
                    return -1; // EOF
                }
                flags = extract_i32 (buf);
                if (flags & FRAMES_FLAG) {
                    // read number of frames
                    if (fread (buf, 1, 4, buffer->file) != 4) {
                        return -1; // EOF
                    }
                    uint32_t nframes = extract_i32 (buf);
                    buffer->duration = (float)nframes * (float)samples_per_frame / (float)samplerate;
                    buffer->totalsamples = nframes * samples_per_frame;
                    trace ("xing totalsamples: %d\n", buffer->totalsamples);
                    buffer->samplerate = samplerate;
                }
                if (flags & BYTES_FLAG) {
                    fseek (buffer->file, 4, SEEK_CUR);
                }
                if (flags & TOC_FLAG) {
                    fseek (buffer->file, 100, SEEK_CUR);
                }
                if (flags & VBR_SCALE_FLAG) {
                    fseek (buffer->file, 4, SEEK_CUR);
                }
                // lame header
                if (fread (buf, 1, 4, buffer->file) != 4) {
                    return -1; // EOF
                }
                trace ("tell=%x, %c%c%c%c\n", ftell(buffer->file), buf[0], buf[1], buf[2], buf[3]);
                if (!memcmp (buf, "LAME", 4)) {
                    trace ("lame header found\n");
                    fseek (buffer->file, 6, SEEK_CUR);

                    // FIXME: that can be optimized by single read
                    uint8_t lpf;
                    fread (&lpf, 1, 1, buffer->file);
                    //3 floats: replay gain
                    fread (buf, 1, 4, buffer->file);
                    float rg_peaksignalamp = extract_f32 (buf);
                    fread (buf, 1, 2, buffer->file);
                    uint16_t rg_radio = extract_i16 (buf);
                    fread (buf, 1, 2, buffer->file);
                    uint16_t rg_audiophile = extract_i16 (buf);
                    // skip
                    fseek (buffer->file, 2, SEEK_CUR);
                    fread (buf, 1, 3, buffer->file);
                    uint32_t startdelay = (((uint32_t)buf[0]) << 4) | ((((uint32_t)buf[1]) & 0xf0)>>4);
                    uint32_t enddelay = ((((uint32_t)buf[1])&0x0f)<<8) | ((uint32_t)buf[2]);
                    // skip
                    fseek (buffer->file, 1, SEEK_CUR);
                    // mp3gain
                    uint8_t mp3gain;
                    fread (&mp3gain, 1, 1, buffer->file);
                    // skip
                    fseek (buffer->file, 2, SEEK_CUR);
                    // musiclen
                    fread (buf, 1, 4, buffer->file);
                    uint32_t musiclen = extract_i32 (buf);
                    trace ("lpf: %d, peaksignalamp: %f, radiogain: %d, audiophile: %d, startdelay: %d, enddelay: %d, mp3gain: %d, musiclen: %d\n", lpf, rg_peaksignalamp, rg_radio, rg_audiophile, startdelay, enddelay, mp3gain, musiclen);
                    // skip crc
                    //fseek (buffer->file, 4, SEEK_CUR);
                    buffer->startdelay = startdelay;
                    buffer->enddelay = enddelay;
                }
                if (sample <= 0 && (flags&FRAMES_FLAG)) {
                    buffer->totalsamples -= buffer->enddelay;
                    trace ("lame totalsamples: %d\n", buffer->totalsamples);
                    fseek (buffer->file, framepos+packetlength-4, SEEK_SET);
                    return 0;
                }
            }
            if (sample == 0) {
                // xing header failed, calculate based on file size
                fseek (buffer->file, 0, SEEK_END);
                int sz = ftell (buffer->file) - buffer->startoffset - buffer->endoffset;
                int nframes = sz / packetlength;
                buffer->duration = nframes * samples_per_frame / samplerate;
                buffer->totalsamples = nframes * samples_per_frame;
                buffer->samplerate = samplerate;
//                trace ("packetlength=%d, fsize=%d, nframes=%d, samples_per_frame=%d, samplerate=%d, duration=%f, totalsamples=%d\n", packetlength, sz, nframes, samples_per_frame, samplerate, buffer->duration, buffer->totalsamples);

                if (sample == 0) {
                    fseek (buffer->file, framepos+packetlength-4, SEEK_SET);
                    return 0;
                }
            }
            fseek (buffer->file, framepos+packetlength-4, SEEK_SET);
            got_xing_header = 1;
        }

        if (sample >= 0 && scansamples + samples_per_frame >= sample) {
            fseek (buffer->file, -4, SEEK_CUR);
            buffer->currentsample = sample;
            buffer->skipsamples = sample - scansamples;
            return 0;
        }
        scansamples += samples_per_frame;

        buffer->duration += dur;
        nframe++;
        if (packetlength > 0) {
            fseek (buffer->file, packetlength-4, SEEK_CUR);
        }
    }
    if (nframe == 0) {
        return -1;
    }
    buffer->totalsamples = scansamples;
    buffer->duration = buffer->totalsamples / buffer->samplerate;
    return 0;
}


static int
cmp3_init (DB_playItem_t *it) {
    memset (&buffer, 0, sizeof (buffer));
    buffer.file = fopen (it->fname, "rb");
    if (!buffer.file) {
        return -1;
    }
    int skip = deadbeef->junk_get_leading_size (buffer.file);
    if (skip > 0) {
        fseek(buffer.file, skip, SEEK_SET);
    }
    buffer.remaining = 0;
    //buffer.output = NULL;
    buffer.readsize = 0;
    buffer.cachefill = 0;
    buffer.cachepos = 0;
    plugin.info.readpos = 0;

    cmp3_scan_stream (&buffer, -1); // scan entire stream, calc duration
	if (it->endsample > 0) {
        buffer.startsample = it->startsample;
        buffer.endsample = it->endsample;
        // that comes from cue, don't calc duration, just seek and play
        plugin.seek_sample (0);
    }
    else {
        it->duration = buffer.duration;
        buffer.startsample = 0;
        buffer.endsample = buffer.totalsamples-1;
        buffer.skipsamples = buffer.startdelay;
        buffer.currentsample = buffer.startdelay;
        fseek (buffer.file, buffer.startoffset, SEEK_SET);
    }
    if (buffer.samplerate == 0) {
        //fprintf (stderr, "bad mpeg file: %f\n", it->fname);
        fclose (buffer.file);
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

static int
cmp3_decode (void) {
    if (buffer.currentsample + buffer.readsize / (4 * buffer.channels) > buffer.endsample) {
        buffer.readsize = (buffer.endsample - buffer.currentsample + 1) * 4 * buffer.channels;
        trace ("size truncated to %d bytes, cursample=%d, endsample=%d, totalsamples=%d\n", buffer.readsize, buffer.currentsample, buffer.endsample, buffer.totalsamples);
        if (buffer.readsize <= 0) {
            return 0;
        }
    }
    int eof = 0;
    for (;;) {
        if (eof) {
            break;
        }
        // FIXME: read single frame here
        // read more MPEG data if needed
        if(stream.buffer==NULL || stream.error==MAD_ERROR_BUFLEN) {
            // copy part of last frame to beginning
            if (stream.next_frame != NULL) {
                buffer.remaining = stream.bufend - stream.next_frame;
                memmove (buffer.input, stream.next_frame, buffer.remaining);
            }
            int size = READBUFFER - buffer.remaining;
            int bytesread = 0;
            char *bytes = buffer.input + buffer.remaining;
            bytesread = fread (bytes, 1, size, buffer.file);
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
				if(stream.error!=MAD_ERROR_LOSTSYNC) {
					trace ("mpgmad: recoverable frame level error (%s)\n", MadErrorString(&stream));
				}
				continue;
			}
			else {
				if(stream.error==MAD_ERROR_BUFLEN) {
					continue;
                }
				else
				{
					trace ("mpgmad: unrecoverable frame level error (%s).\n", MadErrorString(&stream));
					break;
				}
            }
		}

		plugin.info.samplerate = frame.header.samplerate;
		plugin.info.channels = MAD_NCHANNELS(&frame.header);
		
		mad_synth_frame(&synth,&frame);

        int cachepos = (buffer.cachefill + buffer.cachepos) & CACHE_MASK;
        int len = synth.pcm.length;
        if (buffer.currentsample + len > buffer.endsample) {
            len = buffer.endsample - buffer.currentsample + 1;
        }
        int i = min (synth.pcm.length, buffer.skipsamples);
        if (buffer.skipsamples > 0) {
            buffer.skipsamples -= i;
            trace ("skipped %d samples\n", i);
        }
        buffer.currentsample += len-i;
		for(;i<len;i++)
		{
            if (buffer.cachefill >= CACHE_SIZE) {
                printf ("cache overflow!\n");
                break;
            }
//            if (buffer.cachefill >= CACHE_SIZE - sizeof (mad_fixed_t)) {
//                printf ("readsize=%d, pcm.length=%d(%d)\n", buffer.readsize, synth.pcm.length, i);
//            }
            assert (buffer.cachefill < CACHE_SIZE - sizeof (mad_fixed_t));
            memcpy (buffer.cache+cachepos, &synth.pcm.samples[0][i], sizeof (mad_fixed_t));
            cachepos = (cachepos + sizeof (mad_fixed_t)) & CACHE_MASK;
            buffer.cachefill += sizeof (mad_fixed_t);
            buffer.readsize -= sizeof (mad_fixed_t);
            if (MAD_NCHANNELS(&frame.header) == 2) {
                if (buffer.cachefill >= CACHE_SIZE - sizeof (mad_fixed_t)) {
                    trace ("readsize=%d, pcm.length=%d(%d), cachefill=%d, cachepos=%d(%d)\n", buffer.readsize, synth.pcm.length, i, buffer.cachefill, buffer.cachepos, cachepos);
                }
                assert (buffer.cachefill < CACHE_SIZE - sizeof (mad_fixed_t));
                memcpy (buffer.cache+cachepos, &synth.pcm.samples[1][i], sizeof (mad_fixed_t));
                cachepos = (cachepos + sizeof (mad_fixed_t)) & CACHE_MASK;
                buffer.cachefill += sizeof (mad_fixed_t);
                buffer.readsize -= sizeof (mad_fixed_t);
            }
        }
        if (buffer.currentsample > buffer.totalsamples) {
            trace ("mpgmad: warning: extra samples were read after end of stream\n");
        }
        if (buffer.readsize <= 0 || eof || buffer.currentsample > buffer.endsample) {
            if (buffer.currentsample > buffer.endsample) {
                trace ("finished at sample %d (%d)\n", buffer.currentsample, buffer.totalsamples);
            }
            break;
        }
    }
    return 0;
}

static void
cmp3_free (void) {
    if (buffer.file) {
        fclose (buffer.file);
        buffer.file = NULL;
        mad_synth_finish (&synth);
        mad_frame_finish (&frame);
        mad_stream_finish (&stream);
    }
}

static int
cmp3_read (char *bytes, int size) {
    int result;
    int ret = 0;
    int nsamples = size / 2 / plugin.info.channels;
    size *= 2; // convert to mad sample size
    if (buffer.cachefill < size) {
        buffer.readsize = (size - buffer.cachefill);
        cmp3_decode ();
        plugin.info.readpos = (float)(buffer.currentsample-buffer.startsample) / buffer.samplerate;
    }
    if (buffer.cachefill > 0) {
        int sz = min (size, buffer.cachefill);
        int cachepos = buffer.cachepos;
        for (int i = 0; i < nsamples; i++) {
            mad_fixed_t sample = *((mad_fixed_t*)(buffer.cache + cachepos));
            cachepos = (cachepos + sizeof (mad_fixed_t)) & CACHE_MASK;
            *((int16_t*)bytes) = MadFixedToSshort (sample);
            bytes += 2;
            size -= 2;
            ret += 2;
            if (plugin.info.channels == 2) {
                sample = *((mad_fixed_t*)(buffer.cache + cachepos));
                cachepos = (cachepos + sizeof (mad_fixed_t)) & CACHE_MASK;
                *((int16_t*)bytes) = MadFixedToSshort (sample);
                bytes += 2;
                size -= 2;
                ret += 2;
            }
        }
        if (buffer.cachefill > sz) {
            buffer.cachepos = (buffer.cachepos + sz) & CACHE_MASK;
            buffer.cachefill -= sz;
        }
        else {
            buffer.cachefill = 0;
            buffer.cachepos = 0;
        }
    }
    return ret;
}

static int
cmp3_read_float32 (char *bytes, int size) {
    int result;
    int ret = 0;
    int nsamples = size / 4 / plugin.info.channels;
    if (buffer.cachefill < size) {
        buffer.readsize = (size - buffer.cachefill);
        //printf ("decoding %d bytes using read_float32\n", buffer.readsize);
        cmp3_decode ();
        plugin.info.readpos = (float)(buffer.currentsample - buffer.startsample) / buffer.samplerate;
    }
    if (buffer.cachefill > 0) {
        int sz = min (size, buffer.cachefill);
        int cachepos = buffer.cachepos;
        for (int i = 0; i < nsamples; i++) {
            mad_fixed_t sample = *((mad_fixed_t*)(buffer.cache + cachepos));
            cachepos = (cachepos + sizeof (mad_fixed_t)) & CACHE_MASK;
            *((float*)bytes) = MadFixedToFloat (sample);
            bytes += 4;
            size -= 4;
            ret += 4;
            if (plugin.info.channels == 2) {
                sample = *((mad_fixed_t*)(buffer.cache + cachepos));
                cachepos = (cachepos + sizeof (mad_fixed_t)) & CACHE_MASK;
                *((float*)bytes) = MadFixedToFloat (sample);
                bytes += 4;
                size -= 4;
                ret += 4;
            }
        }
        if (buffer.cachefill > sz) {
            buffer.cachepos = (buffer.cachepos + sz) & CACHE_MASK;
            buffer.cachefill -= sz;
        }
        else {
            buffer.cachefill = 0;
        }
    }
    return ret;
}

static int
cmp3_seek_sample (int sample) {
    if (!buffer.file) {
        return -1;
    }
    sample += buffer.startsample + buffer.startdelay;
    if (sample > buffer.endsample) {
        trace ("seek sample %d is beyond end of track (%d)\n", sample, buffer.endsample);
        return -1; // eof
    }
    // restart file, and load until we hit required pos
    fseek (buffer.file, 0, SEEK_SET);
    int skip = deadbeef->junk_get_leading_size (buffer.file);
    if (skip > 0) {
        fseek(buffer.file, skip, SEEK_SET);
    }
    mad_synth_finish (&synth);
    mad_frame_finish (&frame);
    mad_stream_finish (&stream);
    buffer.remaining = 0;
    buffer.readsize = 0;
    buffer.cachefill = 0;
    buffer.cachepos = 0;

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
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return NULL;
    }
    buffer_t buffer;
    memset (&buffer, 0, sizeof (buffer));
    buffer.file = fp;
    int skip = deadbeef->junk_get_leading_size (buffer.file);
    if (skip > 0) {
        fseek(buffer.file, skip, SEEK_SET);
    }
    // calc approx. mp3 duration 
    int res = cmp3_scan_stream (&buffer, 0);
    if (res < 0) {
        fclose (fp);
        return NULL;
    }

    const char *ftype;
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
    // FIXME! bad numsamples passed to cue
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, fname, &plugin, ftype, buffer.duration*buffer.samplerate, buffer.samplerate);
    if (cue_after) {
        fclose (fp);
        return cue_after;
    }

    rewind (fp);

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);

    int apeerr = deadbeef->junk_read_ape (it, fp);
    int v2err = deadbeef->junk_read_id3v2 (it, fp);
    int v1err = deadbeef->junk_read_id3v1 (it, fp);
    fclose (fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    it->duration = buffer.duration;
    it->filetype = ftype;

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
    .plugin.name = "MPEG v1,2 layer1,2,3 decoder",
    .plugin.descr = "based on libmad",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = cmp3_init,
    .free = cmp3_free,
    .read_int16 = cmp3_read,
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
