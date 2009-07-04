#include <string.h>
#include <stdio.h>
#include <mad.h>
#include <assert.h>
#include "codec.h"
#include "cmp3.h"

FILE *outf;

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

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
static int frame_count;
unsigned char *GuardPtr = NULL;

int
cmp3_init (const char *fname) {
    buffer.file = fopen (fname, "rb");
    if (!buffer.file) {
        printf ("failed to read %s\n", fname);
        return -1;
    }
    cmp3.info.bitsPerSample = 16;
    cmp3.info.dataSize = -1;
    cmp3.info.channels = 2;
    cmp3.info.samplesPerSecond = 44100;
    buffer.remaining = 0;
    buffer.output = NULL;
    buffer.readsize = 0;
    buffer.cachefill = 0;
    frame_count = 0;
    GuardPtr = NULL;
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	mad_timer_reset(&timer);

    outf = fopen ("out.raw", "w+b");

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

void
cmp3_decode (void) {
    for (;;) {
        // read more MPEG data if needed
        if(stream.buffer==NULL || stream.error==MAD_ERROR_BUFLEN) {
            // copy part of last frame to beginning
            if (stream.next_frame != NULL) {
                buffer.remaining = stream.bufend - stream.next_frame;
                memmove (buffer.input, stream.next_frame, buffer.remaining);
            }
            int size = READBUFFER - buffer.remaining;
            char *bytes = buffer.input + buffer.remaining;
            for (;;) {
                int bytesread = fread (bytes, 1, size, buffer.file);
                if (bytesread < size) {
                    fseek (buffer.file, 0, SEEK_SET);
                    size -= bytesread;
                    bytes += bytesread;
                }
                else {
                    break;
                }
            }
            mad_stream_buffer(&stream,buffer.input,READBUFFER);
            stream.error=0;
        }
        // decode next frame
		if(mad_frame_decode(&frame,&stream))
		{
			if(MAD_RECOVERABLE(stream.error))
			{
				if(stream.error!=MAD_ERROR_LOSTSYNC ||
				   stream.this_frame!=GuardPtr)
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

		if(frame_count==0) {
            // all info about stream is here, set params
            cmp3.info.bitsPerSample = 16;
            cmp3.info.dataSize = -1;
            cmp3.info.channels = MAD_NCHANNELS(&frame.header);
            cmp3.info.samplesPerSecond = frame.header.samplerate;
//            break;
        }

        frame_count++;
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

                if(MAD_NCHANNELS(&frame.header)==2) {
                    *((int16_t*)buffer.output) = MadFixedToSshort(synth.pcm.samples[1][i]);
                    buffer.output+=2;
                    buffer.readsize-=2;
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
        if (buffer.readsize == 0) {
            break;
        }
    }
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
    if (outf) {
        fclose (outf);
        outf = NULL;
    }
}

int
cmp3_read (char *bytes, int size) {
    int result;
    int totalread = 0;
    if (buffer.cachefill > 0) {
        int sz = min (size, buffer.cachefill);
        memcpy (bytes, buffer.cache, sz);
        bytes += sz;
        size -= sz;
        totalread += sz;
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
        cmp3_decode ();//mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
        totalread += size;
    }
    return 0;
}

codec_t cmp3 = {
    .init = cmp3_init,
    .free = cmp3_free,
    .read = cmp3_read
};


