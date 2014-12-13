/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2005                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: flac_format.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <FLAC/all.h>
#include <ao/ao.h>
#include "audio.h"
#include "format.h"
#include "i18n.h"
#if !defined(FLAC_API_VERSION_CURRENT) || (FLAC_API_VERSION_CURRENT < 8)
#define NEED_EASYFLAC 1
#endif
#if NEED_EASYFLAC
#include "easyflac.h"
#endif
#include "vorbis_comments.h"

typedef struct {
#if NEED_EASYFLAC
  EasyFLAC__StreamDecoder *decoder;
#else
  FLAC__StreamDecoder *decoder;
  int is_oggflac;
#endif
  short channels;
  int rate;
  int bits_per_sample;
  long totalsamples; /* per channel, of course */
  long currentsample;  /* number of samples played, (not decoded, 
			   as those may still be buffered in buf) */

  /* For calculating bitrate stats */
  long samples_decoded;
  long samples_decoded_previous;
  long bytes_read;
  long bytes_read_previous;

  FLAC__StreamMetadata *comments;

  int bos; /* At beginning of stream */
  int eos;  /* End of stream read */


  /* Buffer for decoded audio */
  FLAC__int32 **buf;  /* channels by buf_len array */
  int buf_len;
  int buf_start; /* Offset to start of audio data */
  int buf_fill; /* Number of bytes of audio data in buffer */

  decoder_stats_t stats;

} flac_private_t;

/* Forward declarations */
format_t flac_format;
format_t oggflac_format;


/* Private functions declarations */
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

void resize_buffer(flac_private_t *flac, int newchannels, int newsamples);
/*void copy_comments (vorbis_comment *v_comments, FLAC__StreamMetadata_VorbisComment *f_comments);*/
void print_flac_stream_info (decoder_t *decoder);
void print_flac_comments (FLAC__StreamMetadata_VorbisComment *comments,
			  decoder_callbacks_t *cb, void *callback_arg);



int flac_can_decode (data_source_t *source)
{
  char buf[4];
  int len;

  len = source->transport->peek(source, buf, sizeof(char), 4);

  if (len >= 4 && memcmp(buf, "fLaC", 4) == 0) 
    return 1; /* Naked FLAC */
  else
    return 0;
}


int oggflac_can_decode (data_source_t *source)
{
  char buf[36];
  int len;

  len = source->transport->peek(source, buf, sizeof(char), 36);

  if (len >= 36 && memcmp(buf, "OggS", 4) == 0
                && memcmp(buf+28, "fLaC", 4) == 0) {
     /* old Ogg FLAC , pre flac 1.1.1 */
     return 1;
  }

  if (len >= 36 && memcmp(buf, "OggS", 4) == 0
		&& buf[28] == 0x7F
		&& memcmp(buf+29, "FLAC", 4) == 0
		&& buf[33] == 1
		&& buf[34] == 0) {
      /* Ogg FLAC >= 1.1.1, according to OggFlac mapping 1.0 */
      return 1;
  }

  return 0;
}


decoder_t* flac_init (data_source_t *source, ogg123_options_t *ogg123_opts,
		     audio_format_t *audio_fmt,
		     decoder_callbacks_t *callbacks, void *callback_arg)
{
  decoder_t *decoder;
  flac_private_t *private;
  FLAC__bool ret;


  /* Allocate data source structures */
  decoder = malloc(sizeof(decoder_t));
  private = malloc(sizeof(flac_private_t));

  if (decoder != NULL && private != NULL) {
    decoder->source = source;
    decoder->actual_fmt = decoder->request_fmt = *audio_fmt;
    /* Set format below when we distinguish which we are doing */
    decoder->callbacks = callbacks;
    decoder->callback_arg = callback_arg;
    decoder->private = private;

    private->stats.total_time = 0.0;
    private->stats.current_time = 0.0;
    private->stats.instant_bitrate = 0;
    private->stats.avg_bitrate = 0;
  } else {
    fprintf(stderr, _("Error: Out of memory.\n"));
    exit(1);
  }

  private->bos = 1;
  private->eos = 0;
  private->comments = NULL;
  private->samples_decoded = private->samples_decoded_previous = 0;
  private->bytes_read = private->bytes_read_previous = 0;
  private->currentsample = 0;

  /* Setup empty audio buffer that will be resized on first frame 
     callback */
  private->channels = 0;
  private->buf = NULL;
  private->buf_len = 0;
  private->buf_fill = 0;
  private->buf_start = 0;

  /* Setup FLAC decoder */
#if NEED_EASYFLAC
  if (oggflac_can_decode(source)) {
    decoder->format = &oggflac_format;
    private->decoder = EasyFLAC__stream_decoder_new(1);
  } else {
    decoder->format = &flac_format;
    private->decoder = EasyFLAC__stream_decoder_new(0);
  }


  EasyFLAC__set_client_data(private->decoder, decoder);
  EasyFLAC__set_read_callback(private->decoder, &easyflac_read_callback);
  EasyFLAC__set_write_callback(private->decoder, &easyflac_write_callback);
  EasyFLAC__set_metadata_callback(private->decoder, &easyflac_metadata_callback);
  EasyFLAC__set_error_callback(private->decoder, &easyflac_error_callback);
  EasyFLAC__set_metadata_respond(private->decoder, FLAC__METADATA_TYPE_STREAMINFO);
  EasyFLAC__set_metadata_respond(private->decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
  EasyFLAC__init(private->decoder);
#else
  if (oggflac_can_decode(source)) {
    private->is_oggflac = 1;
    decoder->format = &oggflac_format;
  } else {
    private->is_oggflac = 0;
    decoder->format = &flac_format;
  }
  private->decoder = FLAC__stream_decoder_new();

  FLAC__stream_decoder_set_md5_checking(private->decoder, false);
  FLAC__stream_decoder_set_metadata_respond(private->decoder, FLAC__METADATA_TYPE_STREAMINFO);
  FLAC__stream_decoder_set_metadata_respond(private->decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
  /*FLAC__stream_decoder_init(private->decoder);*/
  if(private->is_oggflac)
    FLAC__stream_decoder_init_ogg_stream(private->decoder, read_callback, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, eof_callback, write_callback, metadata_callback, error_callback, decoder);
  else
    FLAC__stream_decoder_init_stream(private->decoder, read_callback, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, eof_callback, write_callback, metadata_callback, error_callback, decoder);
#endif

  /* Callback will set the total samples and sample rate */
#if NEED_EASYFLAC
  EasyFLAC__process_until_end_of_metadata(private->decoder);
#else
  FLAC__stream_decoder_process_until_end_of_metadata(private->decoder);
#endif

  /* Callback will set the number of channels and resize the 
     audio buffer */
#if NEED_EASYFLAC
  EasyFLAC__process_single(private->decoder);
#else
  FLAC__stream_decoder_process_single(private->decoder);
#endif

  /* FLAC API returns signed samples on all streams */
  decoder->actual_fmt.signed_sample = 1;
  decoder->actual_fmt.big_endian = ao_is_big_endian();

  return decoder;
}


int flac_read (decoder_t *decoder, void *ptr, int nbytes, int *eos,
	      audio_format_t *audio_fmt)
{
  flac_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;
  FLAC__int8 *buf8 = (FLAC__int8 *) ptr;
  FLAC__int16 *buf16 = (FLAC__int16 *) ptr;
  long samples, realsamples = 0;
  FLAC__bool ret;
  int i,j;

  /* Read comments and audio info at the start of a logical bitstream */
  if (priv->bos) {
    decoder->actual_fmt.rate = priv->rate;
    decoder->actual_fmt.channels = priv->channels;
    decoder->actual_fmt.word_size = ((priv->bits_per_sample + 7) / 8);

    switch(decoder->actual_fmt.channels){
    case 1:
      decoder->actual_fmt.matrix="M";
      break;
    case 2:
      decoder->actual_fmt.matrix="L,R";
      break;
    case 3:
      decoder->actual_fmt.matrix="L,R,C";
      break;
    case 4:
      decoder->actual_fmt.matrix="L,R,BL,BR";
      break;
    case 5:
      decoder->actual_fmt.matrix="L,R,C,BL,BR";
      break;
    case 6:
      decoder->actual_fmt.matrix="L,R,C,LFE,BL,BR";
      break;
    case 7:
      decoder->actual_fmt.matrix="L,R,C,LFE,SL,SR,BC";
      break;
    case 8:
      decoder->actual_fmt.matrix="L,R,C,LFE,SL,SR,BL,BR";
      break;
    default:
      decoder->actual_fmt.matrix=NULL;
      break;
    }

    print_flac_stream_info(decoder);
    if (priv->comments != NULL) 
      print_flac_comments(&priv->comments->data.vorbis_comment, cb,
			  decoder->callback_arg);

    priv->bos = 0;
  }

  *audio_fmt = decoder->actual_fmt;

  /* Validate channels and word_size to avoid div by zero */
  if(!(audio_fmt->channels && audio_fmt->word_size)) {
    fprintf(stderr, _("Error: Corrupt input.\n"));
    exit(1);
  }

  /* Only return whole samples (no channel splitting) */
  samples = nbytes / (audio_fmt->channels * audio_fmt->word_size);

  while (realsamples < samples) {
    if (priv->buf_fill > 0) {
      int copy = priv->buf_fill < (samples - realsamples) ?
	priv->buf_fill : (samples - realsamples);

      /* Need sample mangling code here! */

      if (audio_fmt->word_size == 1) {
	for (i = 0; i < priv->channels; i++)
	  for (j = 0; j < copy; j++)
           buf8[(j+realsamples)*audio_fmt->channels+i] = (FLAC__int8) (0xFF & priv->buf[i][j+priv->buf_start]);
      } else if (audio_fmt->word_size == 2) {
	for (i = 0; i < priv->channels; i++)
	  for (j = 0; j < copy; j++)
           buf16[(j+realsamples)*audio_fmt->channels+i] = (FLAC__int16) (0xFFFF & priv->buf[i][j+priv->buf_start]);
      }	

      priv->buf_start += copy;
      priv->buf_fill -= copy;
      realsamples += copy;
    }
    else if (!priv->eos) {
#if NEED_EASYFLAC
      ret = EasyFLAC__process_single(priv->decoder);
      if (!ret ||
	  EasyFLAC__get_state(priv->decoder)
	  == FLAC__STREAM_DECODER_END_OF_STREAM)
	priv->eos = 1;  /* Bail out! */
#else
      ret = FLAC__stream_decoder_process_single(priv->decoder);
      if (!ret ||
	  FLAC__stream_decoder_get_state(priv->decoder)
	  == FLAC__STREAM_DECODER_END_OF_STREAM)
	priv->eos = 1;  /* Bail out! */
#endif
    } else
      break;
  }

  priv->currentsample += realsamples;

  return realsamples * audio_fmt->channels * audio_fmt->word_size;
}


int flac_seek (decoder_t *decoder, double offset, int whence)
{
  return 0;  /* No seeking at this time */
}


#define AVG_FACTOR 0.5

decoder_stats_t *flac_statistics (decoder_t *decoder)
{
  flac_private_t *priv = decoder->private;
  long instant_bitrate;

  /* ov_time_tell() doesn't work on non-seekable streams, so we use
     ov_pcm_tell()  */
  priv->stats.total_time = (double) priv->totalsamples /
    (double) decoder->actual_fmt.rate;
  priv->stats.current_time = (double) priv->currentsample / 
    (double) decoder->actual_fmt.rate;

  /* Need this test to prevent averaging in false zeros. */
  if ((priv->bytes_read - priv->bytes_read_previous) != 0 && 
      (priv->samples_decoded - priv->samples_decoded_previous) != 0) {

    instant_bitrate = 8.0 * (priv->bytes_read - priv->bytes_read_previous) 
    * decoder->actual_fmt.rate 
    / (double) (priv->samples_decoded - priv->samples_decoded_previous);

    /* A little exponential averaging to smooth things out */
    priv->stats.instant_bitrate = AVG_FACTOR * instant_bitrate
      + (1.0 - AVG_FACTOR) * priv->stats.instant_bitrate;

    priv->bytes_read_previous = priv->bytes_read;
    priv->samples_decoded_previous = priv->samples_decoded;
  }

  // priv->stats.instant_bitrate = 8.0 * priv->bytes_read * decoder->actual_fmt.rate / (double) priv->samples_decoded; 

  /* Don't know unless we seek the stream */
  priv->stats.avg_bitrate = 0;


  return malloc_decoder_stats(&priv->stats);
}


void flac_cleanup (decoder_t *decoder)
{
  flac_private_t *priv = decoder->private;
  int i;

  for (i = 0; i < priv->channels; i++)
    free(priv->buf[i]);

  free(priv->buf);
#if NEED_EASYFLAC
  EasyFLAC__finish(priv->decoder);
  EasyFLAC__stream_decoder_delete(priv->decoder);
#else
  FLAC__stream_decoder_finish(priv->decoder);
  FLAC__stream_decoder_delete(priv->decoder);
#endif

  free(decoder->private);
  free(decoder);
}


format_t flac_format = {
  "flac",
  &flac_can_decode,
  &flac_init,
  &flac_read,
  &flac_seek,
  &flac_statistics,
  &flac_cleanup,
};


format_t oggflac_format = {
  "oggflac",
  &oggflac_can_decode,
  &flac_init,
  &flac_read,
  &flac_seek,
  &flac_statistics,
  &flac_cleanup,
};



#if NEED_EASYFLAC
FLAC__StreamDecoderReadStatus easyflac_read_callback(const EasyFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
#else
FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
#endif
{
  decoder_t *e_decoder = client_data;
  flac_private_t *priv = e_decoder->private;

  int read = 0;
	
  read = e_decoder->source->transport->read(e_decoder->source, buffer, 
					    sizeof(FLAC__byte), *bytes);

  *bytes = read;
  priv->bytes_read += read;

  /* Immediately return if errors occured */
  if(read == 0)
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
  else
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;	 
}


#if NEED_EASYFLAC
FLAC__StreamDecoderWriteStatus easyflac_write_callback(const EasyFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
#else
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
#endif
{
  decoder_t *e_decoder = client_data;
  flac_private_t *priv = e_decoder->private;

  int samples = frame->header.blocksize;
  int channels = frame->header.channels;
  int bits_per_sample = priv->bits_per_sample = frame->header.bits_per_sample;
  int i, j;

  resize_buffer(priv, channels, samples);

  for (i = 0; i < channels; i++)
    for (j = 0; j < samples; j++)
      priv->buf[i][j] = buffer[i][j];

  priv->buf_start = 0;
  priv->buf_fill = samples;


  priv->samples_decoded += samples;

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


#if NEED_EASYFLAC
void easyflac_metadata_callback(const EasyFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
#else
void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
#endif
{
  decoder_t *e_decoder = client_data;
  flac_private_t *priv = e_decoder->private;

  switch (metadata->type) {
  case FLAC__METADATA_TYPE_STREAMINFO:
    priv->totalsamples = metadata->data.stream_info.total_samples;
    priv->rate = metadata->data.stream_info.sample_rate;
    break;

  case FLAC__METADATA_TYPE_VORBIS_COMMENT:
    priv->comments = FLAC__metadata_object_clone(metadata);
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


}

#if !NEED_EASYFLAC
FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data)
{
  decoder_t *e_decoder = client_data;
  flac_private_t *priv = e_decoder->private;

  return priv->eos;
}
#endif


void resize_buffer(flac_private_t *flac, int newchannels, int newsamples)
{
  int i;

  if (newchannels == flac->channels && newsamples == flac->buf_len) {
    flac->buf_start = 0;
    flac->buf_fill = 0;
    return;
  }


  /* Not the most efficient approach, but it is easy to follow */
  if(newchannels != flac->channels) {
    /* Deallocate all of the sample vectors */
    for (i = 0; i < flac->channels; i++)
      free(flac->buf[i]);

    flac->buf = realloc(flac->buf, sizeof(FLAC__int32*) * newchannels);
    flac->channels = newchannels;
  }

  for (i = 0; i < newchannels; i++)
    flac->buf[i] = malloc(sizeof(FLAC__int32) * newsamples);

  flac->buf_len = newsamples;
  flac->buf_start = 0;
  flac->buf_fill = 0;
}


void print_flac_stream_info (decoder_t *decoder)
{
  flac_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;


  if (cb == NULL || cb->printf_metadata == NULL)
    return;



#if NEED_EASYFLAC
  if (EasyFLAC__is_oggflac(priv->decoder))
#else
  if (priv->is_oggflac)
#endif
    cb->printf_metadata(decoder->callback_arg, 2,
			_("Ogg FLAC stream: %d bits, %d channel, %ld Hz"),
			priv->bits_per_sample,
			priv->channels,
			priv->rate);
  else
    cb->printf_metadata(decoder->callback_arg, 2,
			_("FLAC stream: %d bits, %d channel, %ld Hz"),
			priv->bits_per_sample,
			priv->channels,
			priv->rate);  
}

void print_flac_comments (FLAC__StreamMetadata_VorbisComment *f_comments,
			  decoder_callbacks_t *cb, void *callback_arg)
{
  int i;
  char *temp = NULL;
  int temp_len = 0;

  for (i = 0; i < f_comments->num_comments; i++) {

    /* Gotta null terminate these things */
    if (temp_len < f_comments->comments[i].length + 1) {
      temp_len = f_comments->comments[i].length + 1;
      temp = realloc(temp, sizeof(char) * temp_len);
    }

    strncpy(temp, f_comments->comments[i].entry, 
	    f_comments->comments[i].length);
    temp[f_comments->comments[i].length] = '\0';

    print_vorbis_comment(temp, cb, callback_arg);
  }

  free(temp);
}
