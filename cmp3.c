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
#include <sys/mman.h>
#include "codec.h"
#include "cmp3.h"
#include "playlist.h"
#include "common.h"

#define READBUFFER 5*8192
#define CACHESIZE 81920
struct buffer {
    FILE *file;

    // input buffer, for MPEG data
    char input[READBUFFER];
    int remaining;

    // output buffer, supplied by player
    char *output;
    int readsize;

    // cache, for extra decoded samples
    char cache[CACHESIZE];
    int cachefill;
};

static struct buffer buffer;
static struct mad_stream stream;
static struct mad_frame frame;
static struct mad_synth synth;
static mad_timer_t timer;

static int
cmp3_decode (void);

static int
cmp3_scan_stream (float position);

int
cmp3_init (const char *fname, int track, float start, float end) {
    buffer.file = fopen (fname, "rb");
    if (!buffer.file) {
        return -1;
    }
    buffer.remaining = 0;
    buffer.output = NULL;
    buffer.readsize = 0;
    buffer.cachefill = 0;
    cmp3.info.position = 0;
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	mad_timer_reset(&timer);

#if 0
    if (cmp3_scan_stream (-1) < 0) {
        return -1;
    }
    rewind (buffer.file);
#endif

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

// function scans file and calculates duration
// if position >= 0 -- scanning is stopped after duration is greater than position
static int
cmp3_scan_stream (float position) {
    int nframe = 0;
    int nskipped = 0;
    float duration = 0;
    int nreads = 0;
    int nseeks = 0;
    for (;;) {
        if (position >= 0 && duration > position) {
            // set decoder timer
            timer.seconds = (int)duration;
            timer.fraction = (int)((duration - (float)timer.seconds)*MAD_TIMER_RESOLUTION);
            return duration;
        }
        uint32_t hdr;
        uint8_t sync;
        if (fread (&sync, 1, 1, buffer.file) != 1) {
            break; // eof
        }
        nreads++;
        if (sync != 0xff) {
            nskipped++;
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            if (fread (&sync, 1, 1, buffer.file) != 1) {
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
        if (fread (&sync, 1, 1, buffer.file) != 1) {
            break; // eof
        }
        nreads++;
        hdr |= sync << 8;
        if (fread (&sync, 1, 1, buffer.file) != 1) {
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
        int vertbl[] = {3, -1, 2, 1}; // 3 is 2.5
        int ver = (hdr & (3<<19)) >> 19;
        ver = vertbl[ver];
        if (ver < 0) {
            continue; // invalid frame
        }

        // layer info
        int ltbl[] = { -1, 3, 2, 1 };
        int layer = (hdr & (3<<17)) >> 17;
        layer = ltbl[layer];
        if (layer < 0) {
            continue; // invalid frame
        }

        // bitrate
        int brtable[5][16] = {
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
        int srtable[3][4] = {
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

        int chantbl[4] = { 2, 2, 2, 1 };
        int nchannels = (hdr & (0x3 << 6)) >> 6;
        nchannels = chantbl[nchannels];

        if (nframe == 0 || cmp3.info.samplesPerSecond == -1)
        {
            cmp3.info.bitsPerSample = 16;
            cmp3.info.channels = nchannels;
            cmp3.info.samplesPerSecond = samplerate;
        }

        // packetlength
        int packetlength = 0;
        bitrate *= 1000;
        if (samplerate > 0 && bitrate > 0) {
            if (layer == 1) {
                duration += (float)384 / samplerate;
                packetlength = (12 * bitrate / samplerate + padding) * 4;
            }
            else if (layer == 2) {
                duration += (float)1152 / samplerate;
                packetlength = 144 * bitrate / samplerate + padding;
            }
            else if (layer == 3) {
                duration += (float)1152 / samplerate;
                packetlength = 144 * bitrate / samplerate + padding;
            }
        }
        else {
            packetlength = 0;
        }
        nframe++;
        if (packetlength > 0) {
            fseek (buffer.file, packetlength-4, SEEK_CUR);
            nseeks++;
        }
    }
//    cmp3.info.duration = duration;
    if (position >= 0 && duration > position) {
        // set decoder timer
        timer.seconds = (int)duration;
        timer.fraction = (int)((duration - (float)timer.seconds)*MAD_TIMER_RESOLUTION);
    }
    if (nframe == 0) {
        //printf ("file doesn't looks like mpeg stream\n");
        return -1;
    }
    printf ("mp3 scan stats: %d reads, %d seeks\n", nreads, nseeks);
    return duration;
}

static int
cmp3_scan_stream2 (float position) {
    fseek (buffer.file, 0, SEEK_END);
    size_t len = ftell (buffer.file);
    uint8_t *map = mmap (NULL, len, PROT_READ, MAP_PRIVATE, buffer.file->_fileno, 0);
    assert (map);
    uint8_t *curr = map;
    int nframe = 0;
    int nskipped = 0;
    float duration = 0;
    int nreads = 0;
    int nseeks = 0;

#define read_byte(x) \
    if ((curr-map)>=len) break; \
    x = *curr; \
    curr++;

    for (;;) {
        if (position >= 0 && duration > position) {
            // set decoder timer
            timer.seconds = (int)duration;
            timer.fraction = (int)((duration - (float)timer.seconds)*MAD_TIMER_RESOLUTION);
            break;
        }
        uint32_t hdr;
        uint8_t sync;
        read_byte (sync);
        nreads++;
        if (sync != 0xff) {
            nskipped++;
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            read_byte(sync);
            nreads++;
            if ((sync >> 5) != 7) {
                nskipped++;
                continue;
            }
        }
        // found frame
        hdr = (0xff<<24) | (sync << 16);
        // read 2 bytes more
        read_byte (sync);
        nreads++;
        hdr |= sync << 8;
        read_byte (sync);
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
        int vertbl[] = {3, -1, 2, 1}; // 3 is 2.5
        int ver = (hdr & (3<<19)) >> 19;
        ver = vertbl[ver];
        if (ver < 0) {
            continue; // invalid frame
        }

        // layer info
        int ltbl[] = { -1, 3, 2, 1 };
        int layer = (hdr & (3<<17)) >> 17;
        layer = ltbl[layer];
        if (layer < 0) {
            continue; // invalid frame
        }

        // bitrate
        int brtable[5][16] = {
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
        int srtable[3][4] = {
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

        int chantbl[4] = { 2, 2, 2, 1 };
        int nchannels = (hdr & (0x3 << 6)) >> 6;
        nchannels = chantbl[nchannels];

        if (nframe == 0 || cmp3.info.samplesPerSecond == -1)
        {
            cmp3.info.bitsPerSample = 16;
            cmp3.info.channels = nchannels;
            cmp3.info.samplesPerSecond = samplerate;
        }

        // packetlength
        int packetlength = 0;
        bitrate *= 1000;
        if (samplerate > 0 && bitrate > 0) {
            if (layer == 1) {
                duration += (float)384 / samplerate;
                packetlength = (12 * bitrate / samplerate + padding) * 4;
            }
            else if (layer == 2) {
                duration += (float)1152 / samplerate;
                packetlength = 144 * bitrate / samplerate + padding;
            }
            else if (layer == 3) {
                duration += (float)1152 / samplerate;
                packetlength = 144 * bitrate / samplerate + padding;
            }
        }
        else {
            packetlength = 0;
        }
        nframe++;
        if (packetlength > 0) {
            curr += packetlength-4;
            if ((curr - map) >= len) {
                break;
            }
            nseeks++;
        }
    }
//    cmp3.info.duration = duration;
    if (position >= 0 && duration > position) {
        // set decoder timer
        timer.seconds = (int)duration;
        timer.fraction = (int)((duration - (float)timer.seconds)*MAD_TIMER_RESOLUTION);
    }
    if (nframe == 0) {
        //printf ("file doesn't looks like mpeg stream\n");
        duration = -1;
    }
    printf ("mp3 scan stats: %d reads, %d seeks\n", nreads, nseeks);
    munmap (map, len);
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

		mad_timer_add(&timer,frame.header.duration);
		
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
    cmp3.info.position = (float)timer.seconds + (float)timer.fraction / MAD_TIMER_RESOLUTION;
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
    }
    return ret;
}

int
cmp3_seek (float time) {
    if (!buffer.file) {
        return -1;
    }
    // restart file, and load until we hit required pos
    mad_synth_finish (&synth);
    mad_frame_finish (&frame);
    mad_stream_finish (&stream);
    fseek(buffer.file, 0, SEEK_SET);
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	mad_timer_reset(&timer);

    if (cmp3_scan_stream (time) == -1) {
        return -1;
    }
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
    ps_add_meta (it, "title", convstr_id3v1 (title, strlen (title)));
    ps_add_meta (it, "artist", convstr_id3v1 (artist, strlen (artist)));
    ps_add_meta (it, "album", convstr_id3v1 (album, strlen (album)));
    ps_add_meta (it, "year", year);
    ps_add_meta (it, "comment", convstr_id3v1 (comment, strlen (comment)));
    ps_add_meta (it, "genre", convstr_id3v1 (genre, strlen (genre)));
    if (tracknum != 0xff) {
        char s[4];
        snprintf (s, 4, "%d", tracknum);
        ps_add_meta (it, "track", s);
    }

    return 0;
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
        printf ("id3v2.%d.%d is unsupported (crap)\n", version_major, version_minor);
        return -1; // unsupported
    }
    uint8_t flags = header[5];
    if (flags & 15) {
        return -1; // unsupported
    }
    int unsync = (flags & (1<<7)) ? 1 : 0;
    int extheader = (flags & (1<<6)) ? 1 : 0;
    int expindicator = (flags & (1<<5)) ? 1 : 0;
    // check for bad size
    if ((header[9] & 0x80) || (header[8] & 0x80) || (header[7] & 0x80) || (header[6] & 0x80)) {
        return -1; // bad header
    }
    uint32_t size = (header[9] << 0) | (header[8] << 7) | (header[7] << 14) | (header[6] << 21);
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
    while (readptr - tag < size - 4) {
        if (version_major == 3 || version_major == 4) {
            char frameid[5];
            memcpy (frameid, readptr, 4);
            frameid[4] = 0;
            readptr += 4;
            if (readptr - tag >= size - 4) {
                break;
            }
            uint32_t sz = (readptr[3] << 0) | (readptr[2] << 8) | (readptr[1] << 16) | (readptr[0] << 24);
            readptr += 4;
            if (readptr - tag >= size - sz) {
                break; // size of frame is less than size of tag
            }
            if (sz < 1) {
                break; // frame must be at least 1 byte long
            }
            uint16_t flags = (readptr[1] << 0) | (readptr[0] << 8);
            readptr += 2;
//            printf ("found id3v2.3 frame: %s, size=%d\n", frameid, sz);
            if (!strcmp (frameid, "TPE1")) {
                if (sz > 1000) {
                    break; // too large
                }
                char str[sz+1];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                ps_add_meta (it, "artist", convstr (str, sz));
            }
            else if (!strcmp (frameid, "TPE2")) {
                if (sz > 1000) {
                    break; // too large
                }
                char str[sz+1];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                ps_add_meta (it, "band", convstr (str, sz));
            }
            else if (!strcmp (frameid, "TRCK")) {
                if (sz > 1000) {
                    break; // too large
                }
                char str[sz+1];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                ps_add_meta (it, "track", convstr (str, sz));
            }
            else if (!strcmp (frameid, "TIT2")) {
                if (sz > 1000) {
                    break; // too large
                }
                char str[sz+1];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                ps_add_meta (it, "title", convstr (str, sz));
                title_added = 1;
            }
            readptr += sz;
        }
        else if (version_major == 2) {
            char frameid[4];
            memcpy (frameid, readptr, 3);
            frameid[3] = 0;
            readptr += 3;
            if (readptr - tag >= size - 3) {
                break;
            }
            uint32_t sz = (readptr[2] << 0) | (readptr[1] << 8) | (readptr[0] << 16);
            readptr += 3;
            if (readptr - tag >= size - sz) {
                break; // size of frame is less than size of tag
            }
            //sz -= 6;
            if (sz < 1) {
                break; // frame must be at least 1 byte long
            }
//            printf ("found id3v2.2 frame: %s, size=%d\n", frameid, sz);
            if (!strcmp (frameid, "TEN")) {
                if (sz > 1000) {
                    break; // too large
                }
                char str[sz+1];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                ps_add_meta (it, "vendor", convstr (str, sz));
            }
            else if (!strcmp (frameid, "TT2")) {
                if (sz > 1000) {
                    break; // too large
                }
                char str[sz+1];
                memcpy (str, readptr, sz);
                str[sz] = 0;
                ps_add_meta (it, "title", convstr (str, sz));
                title_added = 1;
            }
            readptr += sz;
        }
        else {
            printf ("id3v2.%d (unsupported!)\n", version_minor);
        }
    }
    if (!title_added) {
        ps_add_meta (it, "title", NULL);
    }

    return 0;
}

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
    it->tracknum = 0;
    it->timestart = 0;
    it->timeend = 0;
    it->filetype = "MP3";
    if (cmp3_read_id3v2 (it, fp) < 0) {
        if (cmp3_read_id3v1 (it, fp) < 0) {
            ps_add_meta (it, "title", NULL);
        }
    }
    rewind (fp);


    // calculate duration
    buffer.file = fp;
    buffer.remaining = 0;
    buffer.output = NULL;
    buffer.readsize = 0;
    buffer.cachefill = 0;
    cmp3.info.position = 0;
	mad_timer_reset(&timer);

    float dur = 0;
    if ((dur = cmp3_scan_stream2 (-1)) >= 0) {
        it->duration = dur;
        //printf ("duration: %f\n", dur);
        after = ps_insert_item (after, it);
    }
    else {
        ps_item_free (it);
    }
    memset (&buffer, 0, sizeof (buffer));
    fclose (fp);
    return after;
}

static const char * exts[]=
{
	"mp2","mp3",NULL
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
    .getexts = cmp3_getexts
};


