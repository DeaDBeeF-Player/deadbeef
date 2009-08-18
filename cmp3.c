/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
#include <iconv.h>
//#include <sys/mman.h>
#include "codec.h"
#include "cmp3.h"
#include "playlist.h"
#include "common.h"

#define READBUFFER 5*8192

// FIXME: cache is bad name for this
#define CACHESIZE 81920

// vbrmethod constants
#define LAME_CBR  1 
#define LAME_CBR2 8
#define LAME_ABR  2
#define LAME_VBR1 3
#define LAME_VBR2 4
#define LAME_VBR3 5
#define LAME_VBR4 6

typedef struct {
    FILE *file;

    // cuesheet info
    float timestart;
    float timeend;

    // input buffer, for MPEG data
    mad_timer_t timer;
    char input[READBUFFER];
    int remaining;

    // output buffer, supplied by player
    char *output;
    int readsize;

    // cache, for extra decoded samples
    char cache[CACHESIZE];
    int cachefill;

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
    float trackduration;
    int startoffset;
    int endoffset;
#if 0
    // only for xing/lame header
    uint32_t frames;
    uint32_t flags;
    uint32_t vbr_scale;
    uint32_t bytes;
    uint8_t lame_revision;
    uint8_t vbrmethod;
    uint8_t vbrbitrate;
    uint8_t mp3gain;
    float peak_signal_amp;
    uint16_t radio_replay_gain;
    uint16_t audiophile_replay_gain;
    uint16_t delay_start;
    uint16_t delay_end;
    uint32_t musiclength;
#endif
} buffer_t;

static buffer_t buffer;
static struct mad_stream stream;
static struct mad_frame frame;
static struct mad_synth synth;

static int
cmp3_decode (void);

static int
cmp3_scan_stream (buffer_t *buffer, float position);

int
cmp3_init (struct playItem_s *it) {
    memset (&buffer, 0, sizeof (buffer));
    buffer.file = fopen (it->fname, "rb");
    buffer.startoffset = it->startoffset;
    buffer.endoffset = it->endoffset;
    if (!buffer.file) {
        return -1;
    }
    buffer.remaining = 0;
    buffer.output = NULL;
    buffer.readsize = 0;
    buffer.cachefill = 0;
    cmp3.info.readposition = 0;
	mad_timer_reset(&buffer.timer);

    fseek (buffer.file, buffer.startoffset, SEEK_SET);
	if (it->timeend > 0) {
        buffer.timestart = it->timestart;
        buffer.timeend = it->timeend;
        buffer.trackduration = it->duration;
        printf ("duration: %f\n", it->duration);
        // that comes from cue, don't calc duration, just seek and play
        cmp3_scan_stream (&buffer, it->timestart);
    }
    else {
        buffer.trackduration = it->duration = cmp3_scan_stream (&buffer, -1); // scan entire stream, calc duration
        timestart = 0;
        timeend = buffer.trackduration;
        fseek (buffer.file, buffer.startoffset, SEEK_SET);
    }
    cmp3.info.bitsPerSample = buffer.bitspersample;
    cmp3.info.samplesPerSecond = buffer.samplerate;
    cmp3.info.channels = buffer.channels;

	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);

    return 0;
}

/****************************************************************************
 * Converts a sample from libmad's fixed point number format to a signed	*
 * short (16 bits).															*
 ****************************************************************************/
static signed short MadFixedToSshort(mad_fixed_t Fixed)
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

#define MadErrorString(x) mad_stream_errorstr(x)

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

static uint32_t
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
static uint16_t
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

static float
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

// function scans file and calculates duration
// if position >= 0 -- scanning is stopped after duration is greater than position
static int
cmp3_scan_stream (buffer_t *buffer, float position) {
    int nframe = 0;
    int nskipped = 0;
    float duration = 0;
    int nreads = 0;
    int nseeks = 0;
    for (;;) {
        if (position >= 0 && duration > position) {
            // set decoder timer
            buffer->timer.seconds = (int)duration;
            buffer->timer.fraction = (int)((duration - (float)buffer->timer.seconds)*MAD_TIMER_RESOLUTION);
            return duration;
        }
        uint32_t hdr;
        uint8_t sync;
        if (fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        nreads++;
        if (sync != 0xff) {
            nskipped++;
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            if (fread (&sync, 1, 1, buffer->file) != 1) {
                break; // eof
            }
                nreads++;
            if ((sync >> 5) != 7) {
                nskipped++;
                continue;
            }
        }
        // found frame
        hdr = (0xff<<24) | (sync << 16);
        // read 2 bytes more
        if (fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        nreads++;
        hdr |= sync << 8;
        if (fread (&sync, 1, 1, buffer->file) != 1) {
            break; // eof
        }
        nreads++;
        hdr |= sync;
        nskipped = 0;

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
        if (bitrate < 0) {
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

        // packetlength
        int packetlength = 0;
        bitrate *= 1000;
        float dur = 0;
        int samples_per_frame = 0;
        if (samplerate > 0 && bitrate > 0) {
            if (layer == 1) {
                samples_per_frame = 384;
                dur = (float)384 / samplerate;
                packetlength = (12 * bitrate / samplerate + padding) * 4;
            }
            else if (layer == 2) {
                samples_per_frame = 1152;
                dur = (float)1152 / samplerate;
                packetlength = 144 * bitrate / samplerate + padding;
            }
            else if (layer == 3) {
                if (ver == 1) {
                    samples_per_frame = 1152;
                    dur = (float)1152 / samplerate;
                    packetlength = 144 * bitrate / samplerate + padding;
                }
                else {
                    samples_per_frame = 576;
                    dur = (float)576 / samplerate;
                    packetlength = 144 * bitrate / samplerate + padding;
                }
            }
        }
        else {
            continue;
        }
        buffer->version = ver;
        buffer->layer = layer;
        buffer->bitrate = bitrate;
        buffer->samplerate = samplerate;
        buffer->packetlength = packetlength;
        buffer->frameduration = dur;
        buffer->channels = nchannels;
        buffer->bitspersample = 16;
        duration += dur;
        if (position == 0) {
            // try to read xing/info tag
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
//                printf ("found xing header!\n");
                // read flags
                uint32_t flags;
                char buf[4];
                if (fread (buf, 1, 4, buffer->file) != 4) {
                    return -1; // EOF
                }
                flags = extract_i32 (buf);
//                printf ("xing header flags: 0x%x\n", flags);
                if (flags & 0x01) {
                    // read number of frames
                    if (fread (buf, 1, 4, buffer->file) != 4) {
                        return -1; // EOF
                    }
                    uint32_t nframes = extract_i32 (buf);
                    buffer->duration = (float)nframes * (float)samples_per_frame / (float)samplerate;
//                    printf ("mp3 duration calculated based on vbr header: %f\n", buffer->duration);
                    return 0;
                }
            }
            // xing header failed, calculate based on file size
            fseek (buffer->file, 0, SEEK_END);
            int sz = ftell (buffer->file) - buffer->startoffset - buffer->endoffset;
            int nframes = sz / packetlength;
            buffer->duration = nframes * samples_per_frame / samplerate;

            return 0;
        }
        nframe++;
        if (packetlength > 0) {
            fseek (buffer->file, packetlength-4, SEEK_CUR);
            nseeks++;
        }
    }
    if (position >= 0 && duration >= position) {
        // set decoder timer
        buffer->timer.seconds = (int)duration;
        buffer->timer.fraction = (int)((duration - (float)buffer->timer.seconds)*MAD_TIMER_RESOLUTION);
    }
    if (nframe == 0) {
        return -1;
    }
    return duration;
}

static int
cmp3_decode (void) {
    int nread = 0;
    int eof = 0;
    for (;;) {
        if (eof) {
            break;
        }
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
            stream.error=0;
        }
        // decode next frame
		if(mad_frame_decode(&frame,&stream))
		{
			if(MAD_RECOVERABLE(stream.error))
			{
				if(stream.error!=MAD_ERROR_LOSTSYNC)
				{
					//fprintf(stderr,"recoverable frame level error (%s)\n",
					//		MadErrorString(&stream));
					//fflush(stderr);
				}
				continue;
			}
			else {
				if(stream.error==MAD_ERROR_BUFLEN)
					continue;
				else
				{
					//fprintf(stderr,"unrecoverable frame level error (%s).\n",
					//		MadErrorString(&stream));
					break;
				}
            }
		}

		cmp3.info.samplesPerSecond = frame.header.samplerate;
		cmp3.info.channels = MAD_NCHANNELS(&frame.header);

		mad_timer_add(&buffer.timer,frame.header.duration);
		
		mad_synth_frame(&synth,&frame);

        char *cache = &buffer.cache[buffer.cachefill];
        int i;
		for(i=0;i<synth.pcm.length;i++)
		{
            if (buffer.readsize > 0) {
                *((int16_t*)buffer.output) = MadFixedToSshort(synth.pcm.samples[0][i]);
                buffer.output+=2;
                buffer.readsize-=2;
                nread += 2;

                if(MAD_NCHANNELS(&frame.header)==2) {
                    *((int16_t*)buffer.output) = MadFixedToSshort(synth.pcm.samples[1][i]);
                    buffer.output+=2;
                    buffer.readsize-=2;
                    nread += 2;
                }
            }
            else if (buffer.cachefill < CACHESIZE) {
                assert (buffer.cachefill < CACHESIZE-2);
                *((int16_t*)cache) = MadFixedToSshort(synth.pcm.samples[0][i]);
                cache+=2;
                buffer.cachefill+=2;
                if (MAD_NCHANNELS(&frame.header) == 2) {
                    assert (buffer.cachefill < CACHESIZE-2);
                    *((int16_t*)cache) = MadFixedToSshort(synth.pcm.samples[1][i]);
                    cache+=2;
                    buffer.cachefill+=2;
                }
            }
            else {
                printf ("cache overflow!\n");
                break;
            }
        }
        if (buffer.readsize == 0 || eof) {
            break;
        }
//        if (buffer.readsize > 0 && endoffile) {
//            // fill rest with zeroes, and return -1
//            memset (buffer.output, 0, buffer.readsize);
//            return -1;
//        }
    }
    return nread;
}

void
cmp3_free (void) {
    if (buffer.file) {
        fclose (buffer.file);
        buffer.file = NULL;
        mad_synth_finish (&synth);
        mad_frame_finish (&frame);
        mad_stream_finish (&stream);
    }
}

int
cmp3_read (char *bytes, int size) {
    int result;
    int ret = 0;
    if (buffer.cachefill > 0) {
        int sz = min (size, buffer.cachefill);
        memcpy (bytes, buffer.cache, sz);
        bytes += sz;
        size -= sz;
        ret += sz;
        if (buffer.cachefill > sz) {
            memmove (buffer.cache, &buffer.cache[sz], buffer.cachefill-sz);
            buffer.cachefill -= sz;
        }
        else {
            buffer.cachefill = 0;
        }
    }
    if (size > 0) {
        buffer.output = bytes;
        buffer.readsize = size;
        ret += cmp3_decode ();
        cmp3.info.readposition = (float)buffer.timer.seconds + (float)buffer.timer.fraction / MAD_TIMER_RESOLUTION;
    }
    if (cmp3.info.readposition >= buffer.trackduration) {
        return 0;
    }
    return ret;
}

int
cmp3_seek (float time) {
    time += buffer.timestart;
    if (!buffer.file) {
        return -1;
    }
    // restart file, and load until we hit required pos
    mad_synth_finish (&synth);
    mad_frame_finish (&frame);
    mad_stream_finish (&stream);
    fseek(buffer.file, buffer.startoffset, SEEK_SET);
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	mad_timer_reset(&buffer.timer);

	if (time == 0) { 
        cmp3.info.readposition = 0;
        return 0;
    }

    if (cmp3_scan_stream (&buffer, time) == -1) {
        cmp3.info.readposition = 0;
        return -1;
    }
    // fixup timer
    cmp3.info.readposition = (float)buffer.timer.seconds + (float)buffer.timer.fraction / MAD_TIMER_RESOLUTION;
    cmp3.info.readposition -= buffer.timestart;
    buffer.timer.seconds = (int)cmp3.info.readposition;
    buffer.timer.fraction = (cmp3.info.readposition - buffer.timer.seconds) * MAD_TIMER_RESOLUTION;
    return 0;
}

static const char *cmp3_genretbl[] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychadelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk-Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "Acapella",
    "Euro-House",
    "Dance Hall",
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",
    "Christian Gangsta",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary C",
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",
    "JPop",
    "SynthPop",
};

static const char *
convstr_id3v2_2to3 (const char* str, int sz) {
    static char out[2048];
    const char *enc = "iso8859-1";
    char *ret = out;

    // hack to add limited cp1251 recoding support

    if (*str == 1) {
        enc = "UCS-2";
    }
    else {
        int latin = 0;
        int rus = 0;
        for (int i = 1; i < sz; i++) {
            if ((str[i] >= 'A' && str[i] <= 'Z')
                    || str[i] >= 'a' && str[i] <= 'z') {
                latin++;
            }
            else if (str[i] < 0) {
                rus++;
            }
        }
        if (rus > latin/2) {
            // might be russian
            enc = "cp1251";
        }
    }
    str++;
    sz--;
    iconv_t cd = iconv_open ("utf8", enc);
    if (!cd) {
        printf ("unknown encoding: %s\n", enc);
        return NULL;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
        char *pin = (char*)str;
        char *pout = out;
        memset (out, 0, sizeof (out));
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
        ret = out;
    }
    return ret;
}

static const char *
convstr_id3v2_4 (const char* str, int sz) {
    static char out[2048];
    const char *enc = "iso8859-1";
    char *ret = out;

    // hack to add limited cp1251 recoding support

    if (*str == 3) {
        // utf8
        strncpy (out, str+1, 2047);
        sz--;
        out[min (sz, 2047)] = 0;
        return out;
    }
    else if (*str == 0) {
        // iso8859-1
        enc = "iso8859-1";
    }
    else if (*str == 1) {
        enc = "UTF-16";
    }
    else if (*str == 2) {
        enc = "UTF-16BE";
    }
    else {
        return "";
    }
// {{{ cp1251 detection (disabled)
#if 0
    else {
        int latin = 0;
        int rus = 0;
        for (int i = 1; i < sz; i++) {
            if ((str[i] >= 'A' && str[i] <= 'Z')
                    || str[i] >= 'a' && str[i] <= 'z') {
                latin++;
            }
            else if (str[i] < 0) {
                rus++;
            }
        }
        if (rus > latin/2) {
            // might be russian
            enc = "cp1251";
        }
    }
#endif
// }}}
    str++;
    sz--;
    iconv_t cd = iconv_open ("utf8", enc);
    if (!cd) {
        // printf ("unknown encoding: %s\n", enc);
        return NULL;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
        char *pin = (char*)str;
        char *pout = out;
        memset (out, 0, sizeof (out));
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
        ret = out;
    }
    //printf ("decoded %s\n", out+3);
    return ret;
}

const char *convstr_id3v1 (const char* str, int sz) {
    static char out[2048];
    int i;
    for (i = 0; i < sz; i++) {
        if (str[i] != ' ') {
            break;
        }
    }
    if (i == sz) {
        out[0] = 0;
        return out;
    }

    // check for utf8 (hack)
    iconv_t cd;
    cd = iconv_open ("utf8", "utf8");
    size_t inbytesleft = sz;
    size_t outbytesleft = 2047;
    char *pin = (char*)str;
    char *pout = out;
    memset (out, 0, sizeof (out));
    size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
    iconv_close (cd);
    if (res == 0) {
        strncpy (out, str, 2047);
        out[min (sz, 2047)] = 0;
        return out;
    }

    const char *enc = "iso8859-1";
    int latin = 0;
    int rus = 0;
    for (int i = 0; i < sz; i++) {
        if ((str[i] >= 'A' && str[i] <= 'Z')
                || str[i] >= 'a' && str[i] <= 'z') {
            latin++;
        }
        else if (str[i] < 0) {
            rus++;
        }
    }
    if (rus > latin/2) {
        // might be russian
        enc = "cp1251";
    }
    cd = iconv_open ("utf8", enc);
    if (!cd) {
        // printf ("unknown encoding: %s\n", enc);
        return NULL;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
        char *pin = (char*)str;
        char *pout = out;
        memset (out, 0, sizeof (out));
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
    }
    return out;
}

// should read both id3v1 and id3v1.1
int
cmp3_read_id3v1 (playItem_t *it, FILE *fp) {
    if (!it || !fp) {
        printf ("bad call to cmp3_read_id3v1!\n");
        return -1;
    }
    uint8_t buffer[128];
    // try reading from end
    fseek (fp, -128, SEEK_END);
    if (fread (buffer, 1, 128, fp) != 128) {
        return -1;
    }
    if (strncmp (buffer, "TAG", 3)) {
        return -1; // no tag
    }
    char title[31];
    char artist[31];
    char album[31];
    char year[5];
    char comment[31];
    uint8_t genreid;
    uint8_t tracknum;
    const char *genre;
    memset (title, 0, 31);
    memset (artist, 0, 31);
    memset (album, 0, 31);
    memset (year, 0, 5);
    memset (comment, 0, 31);
    memcpy (title, &buffer[3], 30);
    memcpy (artist, &buffer[3+30], 30);
    memcpy (album, &buffer[3+60], 30);
    memcpy (year, &buffer[3+90], 4);
    memcpy (comment, &buffer[3+94], 30);
    genreid = buffer[3+124];
    tracknum = 0xff;
    if (comment[28] == 0) {
        tracknum = comment[29];
    }
//    255 = "None",
//    "CR" = "Cover" (id3v2)
//    "RX" = "Remix" (id3v2)

    if (genreid == 0xff) {
        genre = "None";
    }
    else if (genreid <= 147) {
        genre = cmp3_genretbl[genreid];
    }

    // add meta
//    printf ("%s - %s - %s - %s - %s - %s\n", title, artist, album, year, comment, genre);
    pl_add_meta (it, "title", convstr_id3v1 (title, strlen (title)));
    pl_add_meta (it, "artist", convstr_id3v1 (artist, strlen (artist)));
    pl_add_meta (it, "album", convstr_id3v1 (album, strlen (album)));
    pl_add_meta (it, "year", year);
    pl_add_meta (it, "comment", convstr_id3v1 (comment, strlen (comment)));
    pl_add_meta (it, "genre", convstr_id3v1 (genre, strlen (genre)));
    if (tracknum != 0xff) {
        char s[4];
        snprintf (s, 4, "%d", tracknum);
        pl_add_meta (it, "track", s);
    }

    if (it->endoffset < 128) {
        it->endoffset = 128;
    }

    return 0;
}

int
cmp3_read_ape (playItem_t *it, FILE *fp) {
//    printf ("trying to read ape tag\n");
    // try to read footer, position must be already at the EOF right before
    // id3v1 (if present)
    uint8_t header[32];
    if (fseek (fp, -32, SEEK_CUR) == -1) {
        return -1; // something bad happened
    }

    if (fread (header, 1, 32, fp) != 32) {
        return -1; // something bad happened
    }

    if (strncmp (header, "APETAGEX", 8)) {
        return -1; // no ape tag here
    }

    // end of footer must be 0
    if (memcmp (&header[24], "\0\0\0\0\0\0\0\0", 8)) {
        return -1;
    }

    uint32_t version = extract_i32_le (&header[8]);
    uint32_t size = extract_i32_le (&header[12]);
    uint32_t numitems = extract_i32_le (&header[16]);
    uint32_t flags = extract_i32_le (&header[20]);

//    printf ("APEv%d, size=%d, items=%d, flags=%x\n", version, size, numitems, flags);
    // now seek to beginning of the tag (exluding header)
    if (fseek (fp, -size, SEEK_CUR) == -1) {
        printf ("4\n");
        return -1;
    }

    for (int i = 0; i < numitems; i++) {
        uint8_t buffer[8];
        if (fread (buffer, 1, 8, fp) != 8) {
            return -1;
        }
        uint32_t itemsize = extract_i32_le (&buffer[0]);
        uint32_t itemflags = extract_i32_le (&buffer[4]);
        // read key until 0 (stupid and slow)
        char key[256];
        int keysize = 0;
        while (keysize <= 255) {
            if (fread (&key[keysize], 1, 1, fp) != 1) {
                return -1;
            }
            if (key[keysize] == 0) {
                break;
            }
            if (key[keysize] < 0x20) {
                return -1; // non-ascii chars and chars with coded 0..0x1f not allowed in ape item keys
            }
            keysize++;
        }
        key[255] = 0;
        // read value
        char value[itemsize+1];
        if (fread (value, 1, itemsize, fp) != itemsize) {
            return -1;
        }
        value[itemsize] = 0;
        // add metainfo only if it's textual
        int valuetype = ((itemflags & (0x3<<1)) >> 1);
        if (valuetype == 0) {
            if (!strcasecmp (key, "artist")) {
                pl_add_meta (it, "artist", value);
            }
            else if (!strcasecmp (key, "title")) {
                pl_add_meta (it, "title", value);
            }
            else if (!strcasecmp (key, "album")) {
                pl_add_meta (it, "album", value);
            }
            else if (!strcasecmp (key, "track")) {
                pl_add_meta (it, "track", value);
            }
        }
    }

    return 0;
}

void
id3v2_string_read (int version, char *out, int sz, int unsync, uint8_t **pread) {
    if (!unsync) {
        memcpy (out, *pread, sz);
        *pread += sz;
        out[sz] = 0;
        out[sz+1] = 0;
        return;
    }
    if (version == 3 && *(*pread) == 1) {
        *out = *(*pread);
        (*pread)++;
        out++;
        sz--;
        while (sz >= 2) {
            if (unsync && !(*pread)[0] && !(*pread)[1]) {
                (*pread) += 2;
                continue;
            }
            *out++ = *(*pread)++;
            *out++ = *(*pread)++;
            sz -= 2;
        }
        *out++ = 0;
        *out++ = 0;
    }
    else {
        *out = *(*pread);
        (*pread)++;
        out++;
        sz--;
        while (sz > 0) {
            *out = *(*pread);
            (*pread)++;
            if (unsync && !(*out)) {
                continue;
            }
            out++;
            sz--;
        }
        *out = 0;
    }
}

int
cmp3_read_id3v2 (playItem_t *it, FILE *fp) {
    int title_added = 0;
    if (!it || !fp) {
        printf ("bad call to cmp3_read_id3v2!\n");
        return -1;
    }
    rewind (fp);
    uint8_t header[10];
    if (fread (header, 1, 10, fp) != 10) {
        return -1; // too short
    }
    if (strncmp (header, "ID3", 3)) {
        return -1; // no tag
    }
    uint8_t version_major = header[3];
    uint8_t version_minor = header[4];
    if (version_major > 4 || version_major < 2) {
//        printf ("id3v2.%d.%d is unsupported\n", version_major, version_minor);
        return -1; // unsupported
    }
    uint8_t flags = header[5];
    if (flags & 15) {
        return -1; // unsupported
    }
    int unsync = (flags & (1<<7)) ? 1 : 0;
    int extheader = (flags & (1<<6)) ? 1 : 0;
    int expindicator = (flags & (1<<5)) ? 1 : 0;
    int footerpresent = (flags & (1<<4)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);
    int startoffset = size + 10 + 10 * footerpresent;
    if (startoffset > it->startoffset) {
        it->startoffset = startoffset;
        //printf ("id3v2 end: %x\n", startoffset);
    }

//    printf ("tag size: %d\n", size);


    // try to read full tag if size is small enough
    if (size > 1000000) {
        return -1;
    }
    uint8_t tag[size];
    if (fread (tag, 1, size, fp) != size) {
        return -1; // bad size
    }
    uint8_t *readptr = tag;
    int crcpresent = 0;
//    printf ("version: 2.%d.%d, unsync: %d, extheader: %d, experimental: %d\n", version_major, version_minor, unsync, extheader, expindicator);
    
    if (extheader) {
        if (size < 6) {
            return -1; // bad size
        }
        uint32_t sz = (readptr[3] << 0) | (header[2] << 8) | (header[1] << 16) | (header[0] << 24);
        readptr += 4;
        if (size < sz) {
            return -1; // bad size
        }
        uint16_t extflags = (readptr[1] << 0) | (readptr[0] << 8);
        readptr += 2;
        uint32_t pad = (readptr[3] << 0) | (header[2] << 8) | (header[1] << 16) | (header[0] << 24);
        readptr += 4;
        if (extflags & 0x80000000) {
            crcpresent = 1;
        }
        if (crcpresent && sz != 10) {
            return -1; // bad header
        }
        readptr += 4; // skip crc
    }
    const char * (*convstr)(const char *, int);
    if (version_major == 3) {
        convstr = convstr_id3v2_2to3;
    }
    else {
        convstr = convstr_id3v2_4;
    }
    char *artist = NULL;
    char *album = NULL;
    char *band = NULL;
    char *track = NULL;
    char *title = NULL;
    char *vendor = NULL;
    int err = 0;
    while (readptr - tag <= size - 4) {
        if (version_major == 3 || version_major == 4) {
            char frameid[5];
            memcpy (frameid, readptr, 4);
            frameid[4] = 0;
            readptr += 4;
            if (readptr - tag >= size - 4) {
                err = 1;
                break;
            }
            uint32_t sz = (readptr[3] << 0) | (readptr[2] << 8) | (readptr[1] << 16) | (readptr[0] << 24);
            readptr += 4;
            //printf ("got frame %s, size %d, pos %d, tagsize %d\n", frameid, sz, readptr-tag, size);
            if (readptr - tag >= size - sz) {
                err = 1;
                break; // size of frame is more than size of tag
            }
            if (sz < 1) {
//                err = 1;
                break; // frame must be at least 1 byte long
            }
            uint16_t flags = (readptr[1] << 0) | (readptr[0] << 8);
            readptr += 2;
//            printf ("found id3v2.3 frame: %s, size=%d\n", frameid, sz);
            if (!strcmp (frameid, "TPE1")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                artist = strdup (convstr (str, sz));
            }
            else if (!strcmp (frameid, "TPE2")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                band = strdup (convstr (str, sz));
            }
            else if (!strcmp (frameid, "TRCK")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                track = strdup (convstr (str, sz));
            }
            else if (!strcmp (frameid, "TIT2")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                title = strdup (convstr (str, sz));
            }
            else if (!strcmp (frameid, "TALB")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                id3v2_string_read (version_major, &str[0], sz, unsync, &readptr);
                album = strdup (convstr (str, sz));
            }
            else {
                readptr += sz;
            }
        }
        else if (version_major == 2) {
            char frameid[4];
            memcpy (frameid, readptr, 3);
            frameid[3] = 0;
            readptr += 3;
            if (readptr - tag >= size - 3) {
                err = 1;
                break;
            }
            uint32_t sz = (readptr[2] << 0) | (readptr[1] << 8) | (readptr[0] << 16);
            readptr += 3;
            if (readptr - tag >= size - sz) {
                err = 1;
                break; // size of frame is less than size of tag
            }
            //sz -= 6;
            if (sz < 1) {
                err = 1;
                break; // frame must be at least 1 byte long
            }
//            printf ("found id3v2.2 frame: %s, size=%d\n", frameid, sz);
            if (!strcmp (frameid, "TEN")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                vendor = strdup (convstr (str, sz));
            }
            else if (!strcmp (frameid, "TT2")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                title = strdup (convstr (str, sz));
            }
            else if (!strcmp (frameid, "TAL")) {
                if (sz > 1000) {
                    err = 1;
                    break; // too large
                }
                char str[sz+2];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                album = strdup (convstr (str, sz));
            }
            readptr += sz;
        }
        else {
            printf ("id3v2.%d (unsupported!)\n", version_minor);
        }
    }
    if (!err) {
        if (artist) {
            pl_add_meta (it, "artist", artist);
            free (artist);
        }
        if (album) {
            pl_add_meta (it, "album", album);
            free (album);
        }
        if (band) {
            pl_add_meta (it, "band", band);
            free (band);
        }
        if (track) {
            pl_add_meta (it, "track", track);
            free (track);
        }
        if (title) {
            pl_add_meta (it, "title", title);
            free (title);
        }
        if (vendor) {
            pl_add_meta (it, "vendor", vendor);
            free (vendor);
        }
        if (!title) {
            pl_add_meta (it, "title", NULL);
        }
        return 0;
    }
    return -1;
}

// {{{ separate xing/lame header reader (unused)
#if 0
// read xing/lame header
// based on Xing example code
#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#define FRAMES_AND_BYTES (FRAMES_FLAG | BYTES_FLAG)
int
cmp3_read_info_tag (buffer_t *buffer, playItem_t *it, FILE *fp) {
    rewind (fp);
    int h_id, h_mode, h_sr_index, h_ly_index;
    static int sr_table[4] = { 44100, 48000, 32000, -1 };
    uint8_t header[1024];
    if (fread (header, 1, 1024, fp) != 1024) {
        return -1;
    }
    uint8_t *buf = header;

    // check sync
    if (buf[0] != 0xff || (buf[1]&(3<<5)) != (3<<5)) {
        printf ("sync bits not set, not a mpeg frame\n");
        return -1;
    }

    // get selected MPEG header data
    h_id       = (buf[1] >> 3) & 1;
    h_sr_index = (buf[2] >> 2) & 3;
    h_mode     = (buf[3] >> 6) & 3;
    h_ly_index = (buf[1] >> 1) & 3;

    static int ltbl[] = { -1, 3, 2, 1 };
    int layer = ltbl[h_ly_index];
    printf ("layer%d\n", layer);
    if (layer < 0) {
        printf ("invalid layer\n");
        return -1;
    }

    // determine offset of header
    if (h_id) { // mpeg1
        if (h_mode != 3) {
            buf += (32+4);
        }
        else {
            buf += (17+4);
        }
    }
    else { // mpeg2
        if (h_mode != 3) {
            buf += (17+4);
        }
        else {
            buf += (9+4);
        }
    }

    int is_vbr = 0;
    if (!strncmp (buf, "Xing", 4)) {
        is_vbr = 1;
    }
    else if (!strncmp (buf, "Info", 4)) {
        is_vbr = 0;
    }
    else {
        return -1; // no xing header found
    }

    buf+=4;

    if (h_id) {
        buffer->version = 1;
    }
    else {
        buffer->version = 2;
    }

    buffer->samplerate = sr_table[h_sr_index];
    if (!h_id) {
        buffer->samplerate >>= 1;
    }

    uint32_t flags, frames, bytes, vbr_scale;
    // get flags
    flags = extract_i32 (buf);
    buf+=4;

    if (flags & FRAMES_FLAG) {
        frames = extract_i32(buf);
        buf+=4;
    }
    if (flags & BYTES_FLAG) {
        bytes = extract_i32(buf);
        buf+=4;
    }

    if (flags & TOC_FLAG) {
        // read toc
        buf+=100;
    }

    vbr_scale = -1;
    if (flags & VBR_SCALE_FLAG) {
        vbr_scale = extract_i32(buf);
        buf+=4;
    }

    // check for lame header
    if( buf[0] != 'L' ) return 0;
    if( buf[1] != 'A' ) return 0;
    if( buf[2] != 'M' ) return 0;
    if( buf[3] != 'E' ) return 0;
    printf ("found LAME header\n");
    buf += 20;
    buf += 140;

    uint8_t vbrmethod = header[0xa5];
    if (vbrmethod & 0xf0) {
        buffer->lame_revision = 1;
    }
    else {
        buffer->lame_revision = 0;
    }
    buffer->vbrmethod = vbrmethod & 0x0f;

    buffer->peak_signal_amp = extract_f32 (&header[0xa7]);
    buffer->radio_replay_gain = extract_i16 (&header[0xab]);
    buffer->audiophile_replay_gain = extract_i16 (&header[0xad]);
    buffer->vbrbitrate = header[0xb0];
    buffer->delay_start = (((uint16_t)header[0xb1]) << 4) | (((uint16_t)header[0xb2])>>4);
    buffer->delay_end = ((((uint16_t)header[0xb2]) & 0x0f) << 8) | ((uint16_t)header[0xb3]);
    buffer->mp3gain = header[0xb5];
    buffer->musiclength = extract_i32 (&header[0xb8]);
    return 0;
}
#endif
// }}}

playItem_t *
cmp3_insert (playItem_t *after, const char *fname) {
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return NULL;
    }
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = &cmp3;
    it->fname = strdup (fname);

    buffer_t buffer;
    memset (&buffer, 0, sizeof (buffer));
    buffer.file = fp;

#if 0
    if (cmp3_read_id3v2 (it, fp) < 0) {
        if (cmp3_read_id3v1 (it, fp) < 0) {
            pl_add_meta (it, "title", NULL);
        }
    }
#endif
    int v2err = cmp3_read_id3v2 (it, fp);
    int v1err = cmp3_read_id3v1 (it, fp);
    if (v1err >= 0) {
        fseek (fp, -128, SEEK_END);
    }
    else {
        fseek (fp, 0, SEEK_END);
    }
    int apeerr = cmp3_read_ape (it, fp);
    pl_add_meta (it, "title", NULL);

    buffer.startoffset = it->startoffset;
    fseek (fp, buffer.startoffset, SEEK_SET);
    // calc approx. mp3 duration 
    int res = cmp3_scan_stream (&buffer, 0);
    if (res < 0) {
        pl_item_free (it);
        return NULL;
    }
    it->startoffset = buffer.startoffset;
    it->duration = buffer.duration;
    switch (buffer.layer) {
    case 1:
        it->filetype = "MP1";
        break;
    case 2:
        it->filetype = "MP2";
        break;
    case 3:
        it->filetype = "MP3";
        break;
    }

    playItem_t *cue_after = pl_insert_cue (after, fname, &cmp3, it->filetype);
    if (cue_after) {
        cue_after->timeend = buffer.duration;
        cue_after->duration = cue_after->timeend - cue_after->timestart;
        pl_item_free (it);
        fclose (fp);
        return cue_after;
    }

    after = pl_insert_item (after, it);
    fclose (fp);
    return after;
}

static const char * exts[]=
{
	"mp1", "mp2", "mp3", NULL
};

const char **cmp3_getexts (void) {
    return exts;
}

codec_t cmp3 = {
    .init = cmp3_init,
    .free = cmp3_free,
    .read = cmp3_read,
    .seek = cmp3_seek,
    .insert = cmp3_insert,
    .getexts = cmp3_getexts,
    .id = "stdmp3",
    .filetypes = { "MP1", "MP2", "MP3", NULL }
};


