/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** Copyright 2002, Stan Seibert <volsung@xiph.org>
 **
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <FLAC/metadata.h>
#include "audio.h"
#include "flac.h"
#include "i18n.h"
#include "platform.h"
#include "resample.h"

#if !defined(FLAC_API_VERSION_CURRENT) || (FLAC_API_VERSION_CURRENT < 8)
#define NEED_EASYFLAC 1
#endif

#if NEED_EASYFLAC
static FLAC__StreamDecoderReadStatus easyflac_read_callback(const EasyFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus easyflac_write_callback(const EasyFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void easyflac_metadata_callback(const EasyFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void easyflac_error_callback(const EasyFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
#else
static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
#endif

static void resize_buffer(flacfile *flac, int newchannels, int newsamples);
static void copy_comments (vorbis_comment *v_comments, FLAC__StreamMetadata_VorbisComment *f_comments);


int flac_id(unsigned char *buf, int len)
{
    if (len < 4) return 0;

    return memcmp(buf, "fLaC", 4) == 0;
}


int oggflac_id(unsigned char *buf, int len)
{
    if (len < 33) return 0;

    return memcmp(buf, "OggS", 4) == 0 &&
	   (memcmp (buf+28, "\177FLAC", 5) == 0 || flac_id(buf+28, len - 28));
}


int flac_open(FILE *in, oe_enc_opt *opt, unsigned char *oldbuf, int buflen)
{
    flacfile *flac = malloc(sizeof(flacfile));

    flac->decoder = NULL;
    flac->channels = 0;
    flac->rate = 0;
    flac->totalsamples = 0;
    flac->comments = NULL;
    flac->in = NULL;
    flac->eos = 0;

    /* Setup empty audio buffer that will be resized on first frame 
       callback */
    flac->buf = NULL;
    flac->buf_len = 0;
    flac->buf_start = 0;
    flac->buf_fill = 0;

    /* Copy old input data over */
    flac->oldbuf = malloc(buflen);
    flac->oldbuf_len = buflen;
    memcpy(flac->oldbuf, oldbuf, buflen);
    flac->oldbuf_start = 0;

    /* Need to save FILE pointer for read callback */
    flac->in = in;

    /* Setup FLAC decoder */
#if NEED_EASYFLAC
    flac->decoder = EasyFLAC__stream_decoder_new(oggflac_id(oldbuf, buflen));
    EasyFLAC__set_client_data(flac->decoder, flac);
    EasyFLAC__set_read_callback(flac->decoder, &easyflac_read_callback);
    EasyFLAC__set_write_callback(flac->decoder, &easyflac_write_callback);
    EasyFLAC__set_metadata_callback(flac->decoder, &easyflac_metadata_callback);
    EasyFLAC__set_error_callback(flac->decoder, &easyflac_error_callback);
    EasyFLAC__set_metadata_respond(flac->decoder, FLAC__METADATA_TYPE_STREAMINFO);
    EasyFLAC__set_metadata_respond(flac->decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
    EasyFLAC__init(flac->decoder);
#else
    flac->decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(flac->decoder, false);
    FLAC__stream_decoder_set_metadata_respond(flac->decoder, FLAC__METADATA_TYPE_STREAMINFO);
    FLAC__stream_decoder_set_metadata_respond(flac->decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
    if(oggflac_id(oldbuf, buflen))
        FLAC__stream_decoder_init_ogg_stream(flac->decoder, read_callback, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, eof_callback, write_callback, metadata_callback, error_callback, flac);
    else
        FLAC__stream_decoder_init_stream(flac->decoder, read_callback, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, eof_callback, write_callback, metadata_callback, error_callback, flac);
#endif

    /* Callback will set the total samples and sample rate */
#if NEED_EASYFLAC
    EasyFLAC__process_until_end_of_metadata(flac->decoder);
#else
    FLAC__stream_decoder_process_until_end_of_metadata(flac->decoder);
#endif

    /* Callback will set the number of channels and resize the 
       audio buffer */
#if NEED_EASYFLAC
    EasyFLAC__process_single(flac->decoder);
#else
    FLAC__stream_decoder_process_single(flac->decoder);
#endif

    /* Copy format info for caller */
    opt->rate = flac->rate;
    opt->channels = flac->channels;
    /* flac->total_samples_per_channel was already set by metadata
       callback when metadata was processed. */
    opt->total_samples_per_channel = flac->totalsamples;
    /* Copy Vorbis-style comments from FLAC file (read in metadata 
       callback)*/
    if (flac->comments != NULL && opt->copy_comments)
        copy_comments(opt->comments, &flac->comments->data.vorbis_comment);
    opt->read_samples = flac_read;
    opt->readdata = (void *)flac;

    return 1;
}

/* FLAC follows the WAV channel ordering pattern; we must permute to
   put things in Vorbis channel order */
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

long flac_read(void *in, float **buffer, int samples)
{
    flacfile *flac = (flacfile *)in;
    long realsamples = 0;
    FLAC__bool ret;
    int i,j;
    while (realsamples < samples)
    {
        if (flac->buf_fill > 0)
        {
            int copy = flac->buf_fill < (samples - realsamples) ?
                flac->buf_fill : (samples - realsamples);

            for (i = 0; i < flac->channels; i++){
              int permute = wav_permute_matrix[flac->channels-1][i];
                for (j = 0; j < copy; j++)
                    buffer[i][j+realsamples] =
                        flac->buf[permute][j+flac->buf_start];
            }
            flac->buf_start += copy;
            flac->buf_fill -= copy;
            realsamples += copy;
        }
        else if (!flac->eos)
        {
#if NEED_EASYFLAC
            ret = EasyFLAC__process_single(flac->decoder);
            if (!ret ||
                EasyFLAC__get_state(flac->decoder)
                == FLAC__STREAM_DECODER_END_OF_STREAM)
                flac->eos = 1;  /* Bail out! */
#else
            ret = FLAC__stream_decoder_process_single(flac->decoder);
            if (!ret ||
                FLAC__stream_decoder_get_state(flac->decoder)
                == FLAC__STREAM_DECODER_END_OF_STREAM)
                flac->eos = 1;  /* Bail out! */
#endif
        } else
            break;
    }

    return realsamples;
}

void flac_close(void *info)
{
    int i;
    flacfile *flac =  (flacfile *) info;

    for (i = 0; i < flac->channels; i++)
        free(flac->buf[i]);

    free(flac->buf);
    free(flac->oldbuf);
    free(flac->comments);
#if NEED_EASYFLAC
    EasyFLAC__finish(flac->decoder);
    EasyFLAC__stream_decoder_delete(flac->decoder);
#else
    FLAC__stream_decoder_finish(flac->decoder);
    FLAC__stream_decoder_delete(flac->decoder);
#endif
    free(flac);
}

#if NEED_EASYFLAC
FLAC__StreamDecoderReadStatus easyflac_read_callback(const EasyFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
#else
FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
#endif
{
    flacfile *flac = (flacfile *) client_data;
    int i = 0;
    int oldbuf_fill = flac->oldbuf_len - flac->oldbuf_start;

    /* Immediately return if errors occured */
    if(feof(flac->in))
    {
        *bytes = 0;
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    else if(ferror(flac->in))
    {
        *bytes = 0;
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }


    if(oldbuf_fill > 0) 
    {
        int copy;

        copy = oldbuf_fill < (*bytes - i) ? oldbuf_fill : (*bytes - i);
        memcpy(buffer + i, flac->oldbuf, copy);
        i += copy;
        flac->oldbuf_start += copy;
    }

    if(i < *bytes)
        i += fread(buffer+i, sizeof(FLAC__byte), *bytes - i, flac->in);

    *bytes = i;

    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

#if NEED_EASYFLAC
FLAC__StreamDecoderWriteStatus easyflac_write_callback(const EasyFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
#else
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
#endif
{
    flacfile *flac = (flacfile *) client_data;
    int samples = frame->header.blocksize;
    int channels = frame->header.channels;
    int bits_per_sample = frame->header.bits_per_sample;
    int i, j;

    resize_buffer(flac, channels, samples);

    for (i = 0; i < channels; i++)
        for (j = 0; j < samples; j++)
            flac->buf[i][j] = buffer[i][j] / 
                 (float) (1 << (bits_per_sample - 1));

    flac->buf_start = 0;
    flac->buf_fill = samples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

#if NEED_EASYFLAC
void easyflac_metadata_callback(const EasyFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
#else
void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
#endif
{
    flacfile *flac = (flacfile *) client_data;

    switch (metadata->type)
    {
    case FLAC__METADATA_TYPE_STREAMINFO:
        flac->totalsamples = metadata->data.stream_info.total_samples;
        flac->rate = metadata->data.stream_info.sample_rate;
        break;

    case FLAC__METADATA_TYPE_VORBIS_COMMENT:
        flac->comments = FLAC__metadata_object_clone(metadata);
        break;
    default:
        break;
    }
}

#if NEED_EASYFLAC
void easyflac_error_callback(const EasyFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
#else
void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
#endif
{
    flacfile *flac = (flacfile *) client_data;

}

#if !NEED_EASYFLAC
FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data)
{
    flacfile *flac = (flacfile *) client_data;

    return feof(flac->in)? true : false;
}
#endif

void resize_buffer(flacfile *flac, int newchannels, int newsamples)
{
    int i;

    if (newchannels == flac->channels && newsamples == flac->buf_len)
    {
        flac->buf_start = 0;
        flac->buf_fill = 0;
        return;
    }


    /* Not the most efficient approach, but it is easy to follow */
    if(newchannels != flac->channels)
    {
        /* Deallocate all of the sample vectors */
        for (i = 0; i < flac->channels; i++)
            free(flac->buf[i]);

        flac->buf = realloc(flac->buf, sizeof(float*) * newchannels);
        flac->channels = newchannels;

    }

    for (i = 0; i < newchannels; i++)
        flac->buf[i] = malloc(sizeof(float) * newsamples);

    flac->buf_len = newsamples;
    flac->buf_start = 0;
    flac->buf_fill = 0;
}

void copy_comments (vorbis_comment *v_comments, FLAC__StreamMetadata_VorbisComment *f_comments)
{
    int i;

    for (i = 0; i < f_comments->num_comments; i++)
    {
        char *comment = malloc(f_comments->comments[i].length + 1);
        memset(comment, '\0', f_comments->comments[i].length + 1);
        strncpy(comment, f_comments->comments[i].entry, f_comments->comments[i].length);
        vorbis_comment_add(v_comments, comment);
        free(comment);
    }
}
