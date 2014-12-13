/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** Copyright 2000-2002, Michael Smith <msmith@xiph.org>
 **           2010, Monty <monty@xiph.org>
 **
 ** AIFF/AIFC support from OggSquish, (c) 1994-1996 Monty <xiphmont@xiph.org>
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

#include "audio.h"
#include "platform.h"
#include "i18n.h"
#include "resample.h"

#ifdef HAVE_LIBFLAC
#include "flac.h"
#endif

/* Macros to read header data */
#define READ_U32_LE(buf) \
    (((buf)[3]<<24)|((buf)[2]<<16)|((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U16_LE(buf) \
    (((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U32_BE(buf) \
    (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|((buf)[3]&0xff))

#define READ_U16_BE(buf) \
    (((buf)[0]<<8)|((buf)[1]&0xff))

/* Define the supported formats here */
input_format formats[] = {
    {wav_id, 12, wav_open, wav_close, "wav", N_("WAV file reader")},
    {aiff_id, 12, aiff_open, wav_close, "aiff", N_("AIFF/AIFC file reader")},
#ifdef HAVE_LIBFLAC
    {flac_id,     4, flac_open, flac_close, "flac", N_("FLAC file reader")},
    {oggflac_id, 32, flac_open, flac_close, "ogg", N_("Ogg FLAC file reader")},
#endif
    {NULL, 0, NULL, NULL, NULL, NULL}
};

input_format *open_audio_file(FILE *in, oe_enc_opt *opt)
{
    int j=0;
    unsigned char *buf=NULL;
    int buf_size=0, buf_filled=0;
    int size,ret;

    while(formats[j].id_func)
    {
        size = formats[j].id_data_len;
        if(size >= buf_size)
        {
            buf = realloc(buf, size);
            buf_size = size;
        }

        if(size > buf_filled)
        {
            ret = fread(buf+buf_filled, 1, buf_size-buf_filled, in);
            buf_filled += ret;

            if(buf_filled < size)
            { /* File truncated */
                j++;
                continue;
            }
        }

        if(formats[j].id_func(buf, buf_filled))
        {
            /* ok, we now have something that can handle the file */
            if(formats[j].open_func(in, opt, buf, buf_filled)) {
                free(buf);
                return &formats[j];
            }
        }
        j++;
    }

    free(buf);

    return NULL;
}

static int seek_forward(FILE *in, unsigned int length)
{
    if(fseek(in, length, SEEK_CUR))
    {
        /* Failed. Do it the hard way. */
        unsigned char buf[1024];
        unsigned int seek_needed = length;
        int seeked;
        while(seek_needed > 0)
        {
            seeked = fread(buf, 1, seek_needed>1024?1024:seek_needed, in);
            if(!seeked)
                return 0; /* Couldn't read more, can't read file */
            else
                seek_needed -= seeked;
        }
    }
    return 1;
}


static int find_wav_chunk(FILE *in, char *type, unsigned int *len)
{
    unsigned char buf[8];

    while(1)
    {
        if(fread(buf,1,8,in) < 8) /* Suck down a chunk specifier */
        {
            fprintf(stderr, _("Warning: Unexpected EOF in reading WAV header\n"));
            return 0; /* EOF before reaching the appropriate chunk */
        }

        if(memcmp(buf, type, 4))
        {
            *len = READ_U32_LE(buf+4);
            if(!seek_forward(in, *len))
                return 0;

            buf[4] = 0;
            fprintf(stderr, _("Skipping chunk of type \"%s\", length %d\n"), buf, *len);
        }
        else
        {
            *len = READ_U32_LE(buf+4);
            return 1;
        }
    }
}

static int find_aiff_chunk(FILE *in, char *type, unsigned int *len)
{
    unsigned char buf[8];
    int restarted = 0;

    while(1)
    {
        if(fread(buf,1,8,in) <8)
        {
            if(!restarted) {
                /* Handle out of order chunks by seeking back to the start
                 * to retry */
                restarted = 1;
                fseek(in, 12, SEEK_SET);
                continue;
            }
            fprintf(stderr, _("Warning: Unexpected EOF in AIFF chunk\n"));
            return 0;
        }

        *len = READ_U32_BE(buf+4);

        if(memcmp(buf,type,4))
        {
            if((*len) & 0x1)
                (*len)++;

            if(!seek_forward(in, *len))
                return 0;
        }
        else
            return 1;
    }
}



double read_IEEE80(unsigned char *buf)
{
    int s=buf[0]&0xff;
    int e=((buf[0]&0x7f)<<8)|(buf[1]&0xff);
    double f=((unsigned long)(buf[2]&0xff)<<24)|
        ((buf[3]&0xff)<<16)|
        ((buf[4]&0xff)<<8) |
         (buf[5]&0xff);

    if(e==32767)
    {
        if(buf[2]&0x80)
            return HUGE_VAL; /* Really NaN, but this won't happen in reality */
        else
        {
            if(s)
                return -HUGE_VAL;
            else
                return HUGE_VAL;
        }
    }

    f=ldexp(f,32);
    f+= ((buf[6]&0xff)<<24)|
        ((buf[7]&0xff)<<16)|
        ((buf[8]&0xff)<<8) |
         (buf[9]&0xff);

    return ldexp(f, e-16446);
}

/* AIFF/AIFC support adapted from the old OggSQUISH application */
int aiff_id(unsigned char *buf, int len)
{
    if(len<12) return 0; /* Truncated file, probably */

    if(memcmp(buf, "FORM", 4))
        return 0;

    if(memcmp(buf+8, "AIF",3))
        return 0;

    if(buf[11]!='C' && buf[11]!='F')
        return 0;

    return 1;
}

static int aiff_permute_matrix[6][6] = 
{
  {0},              /* 1.0 mono   */
  {0,1},            /* 2.0 stereo */
  {0,2,1},          /* 3.0 channel ('wide') stereo */
  {0,1,2,3},        /* 4.0 discrete quadraphonic (WARN) */
  {0,2,1,3,4},      /* 5.0 surround (WARN) */
  {0,1,2,3,4,5},    /* 5.1 surround (WARN)*/
};


int aiff_open(FILE *in, oe_enc_opt *opt, unsigned char *buf, int buflen)
{
    int aifc; /* AIFC or AIFF? */
    unsigned int len;
    unsigned char *buffer;
    unsigned char buf2[8];
    aiff_fmt format;
    aifffile *aiff = malloc(sizeof(aifffile));
    int i;

    if(buf[11]=='C')
        aifc=1;
    else
        aifc=0;

    if(!find_aiff_chunk(in, "COMM", &len))
    {
        fprintf(stderr, _("Warning: No common chunk found in AIFF file\n"));
        return 0; /* EOF before COMM chunk */
    }

    if(len < 18) 
    {
        fprintf(stderr, _("Warning: Truncated common chunk in AIFF header\n"));
        return 0; /* Weird common chunk */
    }

    buffer = alloca(len);

    if(fread(buffer,1,len,in) < len)
    {
        fprintf(stderr, _("Warning: Unexpected EOF in reading AIFF header\n"));
        return 0;
    }

    format.channels = READ_U16_BE(buffer);
    format.totalframes = READ_U32_BE(buffer+2);
    format.samplesize = READ_U16_BE(buffer+6);
    format.rate = (int)read_IEEE80(buffer+8);

    aiff->bigendian = 1;

    if(aifc)
    {
        if(len < 22)
        {
            fprintf(stderr, _("Warning: AIFF-C header truncated.\n"));
            return 0;
        }

        if(!memcmp(buffer+18, "NONE", 4)) 
        {
            aiff->bigendian = 1;
        }
        else if(!memcmp(buffer+18, "sowt", 4)) 
        {
            aiff->bigendian = 0;
        }
        else
        {
            fprintf(stderr, _("Warning: Can't handle compressed AIFF-C (%c%c%c%c)\n"), *(buffer+18), *(buffer+19), *(buffer+20), *(buffer+21));
            return 0; /* Compressed. Can't handle */
        }
    }

    if(!find_aiff_chunk(in, "SSND", &len))
    {
        fprintf(stderr, _("Warning: No SSND chunk found in AIFF file\n"));
        return 0; /* No SSND chunk -> no actual audio */
    }

    if(len < 8) 
    {
        fprintf(stderr, _("Warning: Corrupted SSND chunk in AIFF header\n"));
        return 0; 
    }

    if(fread(buf2,1,8, in) < 8)
    {
        fprintf(stderr, _("Warning: Unexpected EOF reading AIFF header\n"));
        return 0;
    }

    format.offset = READ_U32_BE(buf2);
    format.blocksize = READ_U32_BE(buf2+4);

    if( format.blocksize == 0 &&
        (format.samplesize == 16 || format.samplesize == 8))
    {
        /* From here on, this is very similar to the wav code. Oh well. */
        
        opt->rate = format.rate;
        opt->channels = format.channels;
        opt->read_samples = wav_read; /* Similar enough, so we use the same */
        opt->total_samples_per_channel = format.totalframes;

        aiff->f = in;
        aiff->samplesread = 0;
        aiff->channels = format.channels;
        aiff->samplesize = format.samplesize;
        aiff->totalsamples = format.totalframes;

        if(aiff->channels>3)
          fprintf(stderr,"WARNING: AIFF[-C] files with greater than three channels use\n"
                  "speaker locations incompatable with Vorbis suppound definitions.\n"
                  "Not performaing channel location mapping.\n");

        opt->readdata = (void *)aiff;

        aiff->channel_permute = malloc(aiff->channels * sizeof(int));
        if (aiff->channels <= 6)
            /* Where we know the mappings, use them. */
            memcpy(aiff->channel_permute, aiff_permute_matrix[aiff->channels-1], 
                    sizeof(int) * aiff->channels);
        else
            /* Use a default 1-1 mapping */
            for (i=0; i < aiff->channels; i++)
                aiff->channel_permute[i] = i;

        seek_forward(in, format.offset); /* Swallow some data */
        return 1;
    }
    else
    {
        fprintf(stderr, 
                _("Warning: OggEnc does not support this type of AIFF/AIFC file\n"
                " Must be 8 or 16 bit PCM.\n"));
        return 0;
    }
}


int wav_id(unsigned char *buf, int len)
{
    unsigned int flen;
    
    if(len<12) return 0; /* Something screwed up */

    if(memcmp(buf, "RIFF", 4))
        return 0; /* Not wave */

    flen = READ_U32_LE(buf+4); /* We don't use this */

    if(memcmp(buf+8, "WAVE",4))
        return 0; /* RIFF, but not wave */

    return 1;
}

static int wav_permute_matrix[8][8] = 
{
  {0},              /* 1.0 mono   */
  {0,1},            /* 2.0 stereo */
  {0,2,1},          /* 3.0 channel ('wide') stereo */
  {0,1,2,3},        /* 4.0 discrete quadraphonic */
  {0,2,1,3,4},      /* 5.0 surround */
  {0,2,1,4,5,3},    /* 5.1 surround */
  {0,2,1,4,5,6,3},  /* 6.1 surround */
  {0,2,1,6,7,4,5,3} /* 7.1 surround (classic theater 8-track) */
};


int wav_open(FILE *in, oe_enc_opt *opt, unsigned char *oldbuf, int buflen)
{
    unsigned char buf[40];
    unsigned int len;
    int samplesize;
    wav_fmt format;
    wavfile *wav = malloc(sizeof(wavfile));
    int i;

    /* Ok. At this point, we know we have a WAV file. Now we have to detect
     * whether we support the subtype, and we have to find the actual data
     * We don't (for the wav reader) need to use the buffer we used to id this
     * as a wav file (oldbuf)
     */

    if(!find_wav_chunk(in, "fmt ", &len))
        return 0; /* EOF */

    if(len < 16) 
    {
        fprintf(stderr, _("Warning: Unrecognised format chunk in WAV header\n"));
        return 0; /* Weird format chunk */
    }

    /* A common error is to have a format chunk that is not 16, 18 or
     * 40 bytes in size.  This is incorrect, but not fatal, so we only
     * warn about it instead of refusing to work with the file.
     * Please, if you have a program that's creating format chunks of
     * sizes other than 16 or 18 bytes in size, report a bug to the
     * author.
     */
    if(len!=16 && len!=18 && len!=40)
        fprintf(stderr, 
                _("Warning: INVALID format chunk in wav header.\n"
                " Trying to read anyway (may not work)...\n"));

    if(len>40)len=40;

    if(fread(buf,1,len,in) < len)
    {
        fprintf(stderr, _("Warning: Unexpected EOF in reading WAV header\n"));
        return 0;
    }

    format.format =      READ_U16_LE(buf);
    format.channels =    READ_U16_LE(buf+2);
    format.samplerate =  READ_U32_LE(buf+4);
    format.bytespersec = READ_U32_LE(buf+8);
    format.align =       READ_U16_LE(buf+12);
    format.samplesize =  READ_U16_LE(buf+14);

    if(format.format == -2) /* WAVE_FORMAT_EXTENSIBLE */
    {
      if(len<40)
      {
        fprintf(stderr,"ERROR: Extended WAV format header invalid (too small)\n");
        return 0;
      }

      format.mask = READ_U32_LE(buf+20);
      /* warn the user if the format mask is not a supported/expected type */
      switch(format.mask){
      case 1539: /* 4.0 using side surround instead of back */
        fprintf(stderr,"WARNING: WAV file uses side surround instead of rear for quadraphonic;\n"
                "remapping side speakers to rear in encoding.\n");
        break;
      case 1551: /* 5.1 using side instead of rear */
        fprintf(stderr,"WARNING: WAV file uses side surround instead of rear for 5.1;\n"
                "remapping side speakers to rear in encoding.\n");
        break;
      case 319:  /* 6.1 using rear instead of side */
        fprintf(stderr,"WARNING: WAV file uses rear surround instead of side for 6.1;\n"
                "remapping rear speakers to side in encoding.\n");
        break;
      case 255:  /* 7.1 'Widescreen' */
        fprintf(stderr,"WARNING: WAV file is a 7.1 'Widescreen' channel mapping;\n"
                "remapping speakers to Vorbis 7.1 format.\n");
        break;
      case 0:    /* default/undeclared */
      case 1:    /* mono */
      case 3:    /* stereo */
      case 51:   /* quad */
      case 55:   /* 5.0 */
      case 63:   /* 5.1 */
      case 1807: /* 6.1 */
      case 1599: /* 7.1 */
        break;
      default:
        fprintf(stderr,"WARNING: Unknown WAV surround channel mask: %d\n"
                "blindly mapping speakers using default SMPTE/ITU ordering.\n",
                format.mask);
        break;
      }
      format.format = READ_U16_LE(buf+24);
    }

    if(!find_wav_chunk(in, "data", &len))
        return 0; /* EOF */

    if(format.format == 1)
    {
        samplesize = format.samplesize/8;
        opt->read_samples = wav_read;
    }
    else if(format.format == 3)
    {
        samplesize = 4;
        opt->read_samples = wav_ieee_read;
    }
    else
    {
        fprintf(stderr, 
                _("ERROR: Wav file is unsupported type (must be standard PCM\n"
                " or type 3 floating point PCM\n"));
        return 0;
    }

    if(format.align != format.channels * samplesize) {
        /* This is incorrect according to the spec. Warn loudly, then ignore
         * this value.
         */
        fprintf(stderr, _("Warning: WAV 'block alignment' value is incorrect, "
                    "ignoring.\n" 
                    "The software that created this file is incorrect.\n"));
    }

    if(format.samplesize == samplesize*8 && 
            (format.samplesize == 24 || format.samplesize == 16 || 
             format.samplesize == 8 ||
         (format.samplesize == 32 && format.format == 3)))
    {
        /* OK, good - we have the one supported format,
           now we want to find the size of the file */
        opt->rate = format.samplerate;
        opt->channels = format.channels;

        wav->f = in;
        wav->samplesread = 0;
        wav->bigendian = 0;
        wav->channels = format.channels; /* This is in several places. The price
                                            of trying to abstract stuff. */
        wav->samplesize = format.samplesize;

        if(len)
        {
            opt->total_samples_per_channel = len/(format.channels*samplesize);
        }
        else
        {
            long pos;
            pos = ftell(in);
            if(fseek(in, 0, SEEK_END) == -1)
            {
                opt->total_samples_per_channel = 0; /* Give up */
            }
            else
            {
                opt->total_samples_per_channel = (ftell(in) - pos)/
                    (format.channels*samplesize);
                fseek(in,pos, SEEK_SET);
            }
        }
        wav->totalsamples = opt->total_samples_per_channel;

        opt->readdata = (void *)wav;

        wav->channel_permute = malloc(wav->channels * sizeof(int));
        if (wav->channels <= 8)
            /* Where we know the mappings, use them. */
            memcpy(wav->channel_permute, wav_permute_matrix[wav->channels-1], 
                    sizeof(int) * wav->channels);
        else
            /* Use a default 1-1 mapping */
            for (i=0; i < wav->channels; i++)
                wav->channel_permute[i] = i;

        return 1;
    }
    else
    {
        fprintf(stderr, 
                _("ERROR: Wav file is unsupported subformat (must be 8,16, or 24 bit PCM\n"
                "or floating point PCM\n"));
        return 0;
    }
}

long wav_read(void *in, float **buffer, int samples)
{
    wavfile *f = (wavfile *)in;
    int sampbyte = f->samplesize / 8;
    signed char *buf = alloca(samples*sampbyte*f->channels);
    long bytes_read = fread(buf, 1, samples*sampbyte*f->channels, f->f);
    int i,j;
    long realsamples;
    int *ch_permute = f->channel_permute;

    if(f->totalsamples && f->samplesread + 
            bytes_read/(sampbyte*f->channels) > f->totalsamples) {
        bytes_read = sampbyte*f->channels*(f->totalsamples - f->samplesread);
    }

    realsamples = bytes_read/(sampbyte*f->channels);
    f->samplesread += realsamples;

    if(f->samplesize==8)
    {
        unsigned char *bufu = (unsigned char *)buf;
        for(i = 0; i < realsamples; i++)
        {
            for(j=0; j < f->channels; j++)
            {
                buffer[j][i]=((int)(bufu[i*f->channels + ch_permute[j]])-128)/128.0f;
            }
        }
    }
    else if(f->samplesize==16)
    {
        if(!f->bigendian)
        {
            for(i = 0; i < realsamples; i++)
            {
                for(j=0; j < f->channels; j++)
                {
                    buffer[j][i] = ((buf[i*2*f->channels + 2*ch_permute[j] + 1]<<8) |
                                    (buf[i*2*f->channels + 2*ch_permute[j]] & 0xff))/32768.0f;
                }
            }
        }
        else
        {
            for(i = 0; i < realsamples; i++)
            {
                for(j=0; j < f->channels; j++)
                {
                    buffer[j][i]=((buf[i*2*f->channels + 2*ch_permute[j]]<<8) |
                                  (buf[i*2*f->channels + 2*ch_permute[j] + 1] & 0xff))/32768.0f;
                }
            }
        }
    }
    else if(f->samplesize==24) 
    {
        if(!f->bigendian) {
            for(i = 0; i < realsamples; i++)
            {
                for(j=0; j < f->channels; j++) 
                {
                    buffer[j][i] = ((buf[i*3*f->channels + 3*ch_permute[j] + 2] << 16) |
                      (((unsigned char *)buf)[i*3*f->channels + 3*ch_permute[j] + 1] << 8) |
                      (((unsigned char *)buf)[i*3*f->channels + 3*ch_permute[j]] & 0xff)) 
                        / 8388608.0f;

                }
            }
        }
        else {
            fprintf(stderr, _("Big endian 24 bit PCM data is not currently "
                              "supported, aborting.\n"));
            return 0;
        }
    }
    else {
        fprintf(stderr, _("Internal error: attempt to read unsupported "
                          "bitdepth %d\n"), f->samplesize);
        return 0;
    }

    return realsamples;
}

long wav_ieee_read(void *in, float **buffer, int samples)
{
    wavfile *f = (wavfile *)in;
    float *buf = alloca(samples*4*f->channels); /* de-interleave buffer */
    long bytes_read = fread(buf,1,samples*4*f->channels, f->f);
    int i,j;
    long realsamples;


    if(f->totalsamples && f->samplesread +
            bytes_read/(4*f->channels) > f->totalsamples)
        bytes_read = 4*f->channels*(f->totalsamples - f->samplesread);
    realsamples = bytes_read/(4*f->channels);
    f->samplesread += realsamples;

    for(i=0; i < realsamples; i++)
        for(j=0; j < f->channels; j++)
            buffer[j][i] = buf[i*f->channels + f->channel_permute[j]];

    return realsamples;
}


void wav_close(void *info)
{
    wavfile *f = (wavfile *)info;
    free(f->channel_permute);

    free(f);
}

int raw_open(FILE *in, oe_enc_opt *opt, unsigned char *buf, int buflen)
{
    wav_fmt format; /* fake wave header ;) */
    wavfile *wav = malloc(sizeof(wavfile));
    int i;

    /* construct fake wav header ;) */
    format.format =      2; 
    format.channels =    opt->channels;
    format.samplerate =  opt->rate;
    format.samplesize =  opt->samplesize;
    format.bytespersec = opt->channels * opt->rate * opt->samplesize / 8;
    format.align =       format.bytespersec;
    wav->f =             in;
    wav->samplesread =   0;
    wav->bigendian =     opt->endianness;
    wav->channels =      format.channels;
    wav->samplesize =    opt->samplesize;
    wav->totalsamples =  0;
    wav->channel_permute = malloc(wav->channels * sizeof(int));
    for (i=0; i < wav->channels; i++)
      wav->channel_permute[i] = i;

    opt->read_samples = wav_read;
    opt->readdata = (void *)wav;
    opt->total_samples_per_channel = 0; /* raw mode, don't bother */
    return 1;
}

typedef struct {
    res_state resampler;
    audio_read_func real_reader;
    void *real_readdata;
    float **bufs;
    int channels;
    int bufsize;
    int done;
} resampler;

static long read_resampled(void *d, float **buffer, int samples)
{
    resampler *rs = d;
    long in_samples;
    int out_samples;

    in_samples = res_push_max_input(&rs->resampler, samples);
    if(in_samples > rs->bufsize)
        in_samples = rs->bufsize;

    in_samples = rs->real_reader(rs->real_readdata, rs->bufs, in_samples);

    if(in_samples <= 0) {
        if(!rs->done) {
            rs->done = 1;
            out_samples = res_drain(&rs->resampler, buffer);
            return out_samples;
        }
        return 0;
    }

    out_samples = res_push(&rs->resampler, buffer, (float const **)rs->bufs, in_samples);

    if(out_samples <= 0) {
        fprintf(stderr, _("BUG: Got zero samples from resampler: your file will be truncated. Please report this.\n"));
    }

    return out_samples;
}

int setup_resample(oe_enc_opt *opt) {
    resampler *rs = calloc(1, sizeof(resampler));
    int c;

    rs->bufsize = 4096; /* Shrug */
    rs->real_reader = opt->read_samples;
    rs->real_readdata = opt->readdata;
    rs->bufs = malloc(sizeof(float *) * opt->channels);
    rs->channels = opt->channels;
    rs->done = 0;
    if(res_init(&rs->resampler, rs->channels, opt->resamplefreq, opt->rate, RES_END))
    {
        fprintf(stderr, _("Couldn't initialise resampler\n"));
        return -1;
    }

    for(c=0; c < opt->channels; c++)
        rs->bufs[c] = malloc(sizeof(float) * rs->bufsize);

    opt->read_samples = read_resampled;
    opt->readdata = rs;
    if(opt->total_samples_per_channel)
        opt->total_samples_per_channel = (int)((float)opt->total_samples_per_channel * 
            ((float)opt->resamplefreq/(float)opt->rate));
    opt->rate = opt->resamplefreq;

    return 0;
}

void clear_resample(oe_enc_opt *opt) {
    resampler *rs = opt->readdata;
    int i;

    opt->read_samples = rs->real_reader;
    opt->readdata = rs->real_readdata;
    res_clear(&rs->resampler);

    for(i = 0; i < rs->channels; i++)
        free(rs->bufs[i]);

    free(rs->bufs);

    free(rs);
}

typedef struct {
    audio_read_func real_reader;
    void *real_readdata;
    int channels;
    float scale_factor;
} scaler;

static long read_scaler(void *data, float **buffer, int samples) {
    scaler *d = data;
    long in_samples = d->real_reader(d->real_readdata, buffer, samples);
    int i,j;

    for(i=0; i < d->channels; i++) {
        for(j=0; j < in_samples; j++) {
            buffer[i][j] *= d->scale_factor;
        }
    }

    return in_samples;
}


void setup_scaler(oe_enc_opt *opt, float scale) {
    scaler *d = calloc(1, sizeof(scaler));

    d->real_reader = opt->read_samples;
    d->real_readdata = opt->readdata;

    opt->read_samples = read_scaler;
    opt->readdata = d;
    d->channels = opt->channels;
    d->scale_factor = scale;
}

void clear_scaler(oe_enc_opt *opt) {
    scaler *d = opt->readdata;

    opt->read_samples = d->real_reader;
    opt->readdata = d->real_readdata;

    free(d);
}

typedef struct {
    audio_read_func real_reader;
    void *real_readdata;
    float **bufs;
} downmix;

static long read_downmix(void *data, float **buffer, int samples)
{
    downmix *d = data;
    long in_samples = d->real_reader(d->real_readdata, d->bufs, samples);
    int i;

    for(i=0; i < in_samples; i++) {
        buffer[0][i] = (d->bufs[0][i] + d->bufs[1][i])*0.5f;
    }

    return in_samples;
}

void setup_downmix(oe_enc_opt *opt) {
    downmix *d = calloc(1, sizeof(downmix));

    if(opt->channels != 2) {
        fprintf(stderr, "Internal error! Please report this bug.\n");
        return;
    }
    
    d->bufs = malloc(2 * sizeof(float *));
    d->bufs[0] = malloc(4096 * sizeof(float));
    d->bufs[1] = malloc(4096 * sizeof(float));
    d->real_reader = opt->read_samples;

    d->real_readdata = opt->readdata;

    opt->read_samples = read_downmix;
    opt->readdata = d;

    opt->channels = 1;
}
void clear_downmix(oe_enc_opt *opt) {
    downmix *d = opt->readdata;

    opt->read_samples = d->real_reader;
    opt->readdata = d->real_readdata;
    opt->channels = 2; /* other things in cleanup rely on this */

    free(d->bufs[0]);
    free(d->bufs[1]);
    free(d->bufs);
    free(d);
}

