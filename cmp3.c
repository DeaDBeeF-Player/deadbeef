#include <string.h>
#include <stdio.h>
#include <mad.h>
#include <assert.h>
#include <stdlib.h>
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

//static int
//cmp3_get_stream_info (void);

static int
cmp3_get_stream_info2 (void);

int
cmp3_init (const char *fname, int track, float start, float end) {
    buffer.file = fopen (fname, "rb");
    if (!buffer.file) {
        printf ("failed to read %s\n", fname);
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

    if (cmp3_get_stream_info2 () == -1) {
        return -1;
    }
//    printf ("song %s samplerate %d\n", fname, cmp3.info.samplesPerSecond);
    fseek (buffer.file, 0, SEEK_SET);
//	cmp3_decode (); // this 1st run will read 1st frame, but will not decode anything except header

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

static int
cmp3_get_stream_info2 (void) {
    int nframe = 0;
    int nskipped = 0;
    float duration = 0;
    for (;;) {
        uint32_t hdr;
        uint8_t sync;
        if (fread (&sync, 1, 1, buffer.file) != 1) {
            break; // eof
        }
        if (sync != 0xff) {
            nskipped++;
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            if (fread (&sync, 1, 1, buffer.file) != 1) {
                break; // eof
            }
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
        hdr |= sync << 8;
        if (fread (&sync, 1, 1, buffer.file) != 1) {
            break; // eof
        }
        hdr |= sync;
        nskipped = 0;

        // parse header
        
        // sync bits
        int usync = hdr & 0xffe00000;
        if (usync != 0xffe00000) {
            printf ("fatal error: mp3 header parser is broken\n");
        }

        // layer info
        int layer = (hdr & (3<<17)) >> 17;

        // bitrate
        int brtable[16] = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 };
        int bitrate = (hdr & (0x0f<<12)) >> 12;
        bitrate = brtable[bitrate];
//        printf ("bitrate = %d\n", bitrate);

        // samplerate
        int srtable[4] = {44100, 48000, 32000, -1};
        int samplerate = (hdr & (0x03<<10))>>10;
        samplerate = srtable[samplerate];
//        printf ("samplerate = %d\n", samplerate);

        // padding
        int padding = (hdr & (0x1 << 9)) >> 9;
//        printf ("padding = %d\n", padding);

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
        if (samplerate > 0) {
            if (layer == 3) { // layer
                packetlength = (12 * bitrate / samplerate + padding) * 4;
            }
            else if (layer == 2) { // layer2
                packetlength = 144 * bitrate / samplerate + padding;
            }
            else if (layer == 1) { // layer3
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
        }
    }
    cmp3.info.duration = duration;
    printf ("song duration: %f\n", duration);
    return 0;
}

static int
cmp3_skip2 (float seconds) {
    int nframe = 0;
    int nskipped = 0;
    float duration = 0;
    for (;;) {
        uint32_t hdr;
        uint8_t sync;
        if (fread (&sync, 1, 1, buffer.file) != 1) {
            break; // eof
        }
        if (sync != 0xff) {
            nskipped++;
            continue; // not an mpeg frame
        }
        else {
            // 2nd sync byte
            if (fread (&sync, 1, 1, buffer.file) != 1) {
                break; // eof
            }
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
        hdr |= sync << 8;
        if (fread (&sync, 1, 1, buffer.file) != 1) {
            break; // eof
        }
        hdr |= sync;
//        printf ("header=%x, skipped %d\n", hdr, nskipped);
        nskipped = 0;

        // parse header
        
        // sync bits
        int usync = hdr & 0xffe00000;
//        printf ("sync = %xh\n", usync);
        if (usync != 0xffe00000) {
            printf ("fatal error: mp3 header parser is broken\n");
        //    exit (0);
        }

        // layer info
        int layer = (hdr & (3<<17)) >> 17;
//        printf ("layer = %xh\n", layer);

        // bitrate
        int brtable[16] = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 };
        int bitrate = (hdr & (0x0f<<12)) >> 12;
        bitrate = brtable[bitrate];
//        printf ("bitrate = %d\n", bitrate);

        // samplerate
        int srtable[4] = {44100, 48000, 32000, -1};
        int samplerate = (hdr & (0x03<<10))>>10;
        samplerate = srtable[samplerate];
//        printf ("samplerate = %d\n", samplerate);

        // padding
        int padding = (hdr & (0x1 << 9)) >> 9;
//        printf ("padding = %d\n", padding);

        int chantbl[4] = { 2, 2, 2, 1 };
        int nchannels = (hdr & (0x3 << 6)) >> 6;
        nchannels = chantbl[nchannels];


        // packetlength
        int packetlength = 0;
        bitrate *= 1000;
        if (samplerate > 0) {
            if (layer == 3) { // layer1
                packetlength = (12 * bitrate / samplerate + padding) * 4;
            }
            else if (layer == 2) { // layer2
                packetlength = 144 * bitrate / samplerate + padding;
            }
            else if (layer == 1) { // layer3
                duration += (float)1152 / samplerate;
                packetlength = 144 * bitrate / samplerate + padding;
            }
            if (duration > seconds) {
                fseek (buffer.file, -4, SEEK_SET);
                // set decoder timer
                timer.seconds = (int)duration;
                timer.fraction = (int)((duration - (float)timer.seconds)*MAD_TIMER_RESOLUTION);
                return 0;
            }
        }
        else {
            packetlength = 0;
        }
        nframe++;
        if (packetlength != 0) {
            fseek (buffer.file, packetlength-4, SEEK_CUR);
        }
    }
    return -1;
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
					fprintf(stderr,"recoverable frame level error (%s)\n",
							MadErrorString(&stream));
					fflush(stderr);
				}
				continue;
			}
			else {
				if(stream.error==MAD_ERROR_BUFLEN)
					continue;
				else
				{
					fprintf(stderr,"unrecoverable frame level error (%s).\n",
							MadErrorString(&stream));
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

    if (cmp3_skip2 (time) == -1) {
        return -1;
    }
    return 0;
}

int
cmp3_add (const char *fname) {
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = &cmp3;
    it->fname = strdup (fname);
    it->tracknum = 0;
    it->timestart = 0;
    it->timeend = 0;
    ps_append_item (it);
    return 0;
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
    .add = cmp3_add,
    .getexts = cmp3_getexts
};


