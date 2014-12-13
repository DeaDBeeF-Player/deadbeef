/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2003                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: speex_format.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ogg/ogg.h>
#include <speex/speex.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>
#include "transport.h"
#include "format.h"
#include "vorbis_comments.h"
#include "utf8.h"
#include "i18n.h"

/* Note that this file contains gratuitous cut and paste from speexdec.c. */


/* Use speex's audio enhancement feature */
#define ENHANCE_AUDIO 1


typedef struct speex_private_t {
  ogg_sync_state oy;
  ogg_page og;
  ogg_packet op;
  ogg_stream_state os;
  SpeexBits bits;
  SpeexStereoState stereo;
  SpeexHeader *header;
  void *st;

  char *comment_packet;
  int  comment_packet_len;

  float *output; /* Has frame_size * [number of channels] * frames_per_packet 
		    elements */ 
  int output_start;
  int output_left;

  int frames_per_packet;
  int frame_size;
  int vbr;

  int bos;  /* If we are at the beginning (before headers) of a stream */
  int eof;  /* If we've read the end of the data source (but there still might
	       be data in the buffers */
  /* For calculating bitrate stats */
  long samples_decoded;
  long samples_decoded_previous;
  long bytes_read;
  long bytes_read_previous;

  long totalsamples;
  long currentsample;

  decoder_stats_t stats;
} speex_private_t;

/* Forward declarations */
format_t speex_format;



/* Private functions declarations */
void print_speex_info(SpeexHeader *header, decoder_callbacks_t *cb, 
		      void *callback_arg);
void print_speex_comments(char *comments, int length, 
			  decoder_callbacks_t *cb, void *callback_arg);
void *process_header(ogg_packet *op, int *frame_size,
		     SpeexHeader **header,
		     SpeexStereoState *stereo, decoder_callbacks_t *cb,
		     void *callback_arg);
int read_speex_header(decoder_t *decoder);

/* ----------------------------------------------------------- */


int speex_can_decode (data_source_t *source)
{
  char buf[36];
  int len;

  len = source->transport->peek(source, buf, sizeof(char), 36);

  if (len >= 32 && memcmp(buf, "OggS", 4) == 0
      && memcmp(buf+28, "Speex   ", 8) == 0) /* 3 trailing spaces */
    return 1;
  else
    return 0;
}


decoder_t* speex_init (data_source_t *source, ogg123_options_t *ogg123_opts,
		       audio_format_t *audio_fmt,
		       decoder_callbacks_t *callbacks, void *callback_arg)
{
  decoder_t *decoder;
  speex_private_t *private;
  int ret;


  /* Allocate data source structures */
  decoder = malloc(sizeof(decoder_t));
  private = malloc(sizeof(speex_private_t));

  if (decoder != NULL && private != NULL) {
    decoder->source = source;
    decoder->actual_fmt = decoder->request_fmt = *audio_fmt;
    decoder->format = &speex_format;
    decoder->callbacks = callbacks;
    decoder->callback_arg = callback_arg;
    decoder->private = private;


    private->bos = 1;
    private->eof = 0;
    private->samples_decoded = private->samples_decoded_previous = 0;
    private->bytes_read = private->bytes_read_previous = 0;
    private->currentsample = 0;
    private->comment_packet = NULL;
    private->comment_packet_len = 0;
    private->header = NULL;

    private->stats.total_time = 0.0;
    private->stats.current_time = 0.0;
    private->stats.instant_bitrate = 0;
    private->stats.avg_bitrate = 0;
  } else {
    fprintf(stderr, _("ERROR: Out of memory.\n"));
    exit(1);
  }

  /* Initialize structures  */
  ogg_sync_init(&private->oy);
  speex_bits_init(&private->bits);
  ret = 1;

  if (ret < 0) {
    free(private);
/*    free(source);     nope.  caller frees. */ 
    return NULL;
  }

  return decoder;
}


int speex_read (decoder_t *decoder, void *ptr, int nbytes, int *eos,
	      audio_format_t *audio_fmt)
{
  speex_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;
  transport_t *trans = decoder->source->transport;
  int bytes_requested = nbytes;
  int ret;
  short *out = (short *) ptr;

  /* Read comments and audio info at the start of a logical bitstream */
  if (priv->bos) {

    ret = read_speex_header(decoder);

    if (!ret) {
      *eos = 1;
      return 0; /* Bail out! */
    }

    print_speex_info(priv->header, cb, decoder->callback_arg);
    if (priv->comment_packet != NULL)
      print_speex_comments(priv->comment_packet, priv->comment_packet_len,
			   cb, decoder->callback_arg);

    priv->bos = 0;
  }

  *audio_fmt = decoder->actual_fmt;

  while (nbytes) {
    char *data;
    int i, j, nb_read;

    /* First see if there is anything left in the output buffer and 
       empty it out */
    if (priv->output_left > 0) {
      int to_copy = nbytes / (2 * audio_fmt->channels);

      to_copy *= audio_fmt->channels;

      to_copy = priv->output_left < to_copy ? priv->output_left : to_copy;

      /* Integerify it! */
      for (i = 0; i < to_copy; i++)
	out[i]=(ogg_int16_t)priv->output[i+priv->output_start];

      out += to_copy;
      priv->output_start += to_copy;
      priv->output_left -= to_copy;

      priv->currentsample += to_copy / audio_fmt->channels;

      nbytes -= to_copy * 2;
    } else if (ogg_stream_packetout(&priv->os, &priv->op) == 1) {
      float *temp_output = priv->output;
      /* Decode some more samples */

      /*Copy Ogg packet to Speex bitstream*/
      speex_bits_read_from(&priv->bits, (char*)priv->op.packet, 
			   priv->op.bytes);

      for (j = 0;j < priv->frames_per_packet; j++) {
	/*Decode frame*/
	speex_decode(priv->st, &priv->bits, temp_output);

	if (audio_fmt->channels==2)
	  speex_decode_stereo(temp_output, priv->frame_size, &priv->stereo);
	
	priv->samples_decoded += priv->frame_size;

	/*PCM saturation (just in case)*/
	for (i=0;i < priv->frame_size * audio_fmt->channels; i++) {
	  if (temp_output[i]>32000.0)
	    temp_output[i]=32000.0;
	  else if (temp_output[i]<-32000.0)
	    temp_output[i]=-32000.0;
	}

	temp_output += priv->frame_size * audio_fmt->channels;
      }

      priv->output_start = 0;
      priv->output_left = priv->frame_size * audio_fmt->channels *
	priv->frames_per_packet;
    } else if (ogg_sync_pageout(&priv->oy, &priv->og) == 1) {
      /* Read in another ogg page */
      ogg_stream_pagein(&priv->os, &priv->og);
    } else if (!priv->eof) {
      /* Finally, pull in some more data and try again on the next pass */

      /*Get the ogg buffer for writing*/
      data = ogg_sync_buffer(&priv->oy, 200);
      /*Read bitstream from input file*/
      nb_read = trans->read(decoder->source, data, sizeof(char), 200);

      if (nb_read == 0)
	priv->eof = 1;  /* We've read the end of the file */

      ogg_sync_wrote(&priv->oy, nb_read);

      priv->bytes_read += nb_read;
    } else {
      *eos = 1;
      break;
    }
  }

  return bytes_requested - nbytes;
}


int speex_seek (decoder_t *decoder, double offset, int whence)
{
  speex_private_t *priv = decoder->private;

    return 0;
}


#define AVG_FACTOR 0.7

decoder_stats_t *speex_statistics (decoder_t *decoder)
{
  speex_private_t *priv = decoder->private;
  long instant_bitrate;
  long avg_bitrate;

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

  /* Don't know unless we seek the stream */
  priv->stats.avg_bitrate = 0;


  return malloc_decoder_stats(&priv->stats);
}


void speex_cleanup (decoder_t *decoder)
{
  speex_private_t *priv = decoder->private;

  free(priv->comment_packet);
  free(priv->output);
  free(decoder->private);
  free(decoder);
}


format_t speex_format = {
  "speex",
  &speex_can_decode,
  &speex_init,
  &speex_read,
  &speex_seek,
  &speex_statistics,
  &speex_cleanup,
};


/* ------------------- Private functions -------------------- */

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
  	           	    (buf[base]&0xff))

void print_speex_info(SpeexHeader *header, decoder_callbacks_t *cb, 
		      void *callback_arg)
{
  int modeID = header->mode;

  if (header->vbr)
    cb->printf_metadata(callback_arg, 2, 
			_("Ogg Speex stream: %d channel, %d Hz, %s mode (VBR)"),
			header->nb_channels,
			header->rate,
			speex_mode_list[modeID]->modeName);
  else
    cb->printf_metadata(callback_arg, 2,
			_("Ogg Speex stream: %d channel, %d Hz, %s mode"),
			header->nb_channels,
			header->rate,
			speex_mode_list[modeID]->modeName);

  cb->printf_metadata(callback_arg, 3, 
		      _("Speex version: %s"),
		      header->speex_version);

}


void print_speex_comments(char *comments, int length, 
			  decoder_callbacks_t *cb, void *callback_arg)
{
  char *c = comments;
  int len, i, nb_fields;
  char *end;
  char *temp = NULL;
  int temp_len = 0;

  if (length<8) {
    cb->printf_error(callback_arg, WARNING, _("Invalid/corrupted comments"));
    return;
  }

  /* Parse out vendor string */

  end = c + length;
  len = readint(c, 0);
  c += 4;

  if (c+len>end) {
    cb->printf_error(callback_arg, WARNING, _("Invalid/corrupted comments"));
    return;
  }


  temp_len = len + 1;
  temp = malloc(sizeof(char) * temp_len);

  strncpy(temp, c, len);
  temp[len] = '\0';

  cb->printf_metadata(callback_arg, 3, _("Encoded by: %s"), temp);


  /* Parse out user comments */

  c += len;

  if (c + 4>end) {
    cb->printf_error(callback_arg, WARNING, _("Invalid/corrupted comments"));
    free(temp);
    return;
  }

  nb_fields = readint(c, 0);
  c += 4;

  for (i = 0; i < nb_fields; i++) {
    if (c + 4 > end) {
      cb->printf_error(callback_arg, WARNING, _("Invalid/corrupted comments"));
      free(temp);
      return;
    }
    len = readint(c, 0);
    c += 4;
    if (c + len > end) {
      cb->printf_error(callback_arg, WARNING, _("Invalid/corrupted comments"));

      free(temp);
      return;
    }

    if (temp_len < len + 1) {
      temp_len = len + 1;
      temp = realloc(temp, sizeof(char) * temp_len);
    }


    strncpy(temp, c, len);
    temp[len] = '\0';

    print_vorbis_comment(temp, cb, callback_arg);

    c += len;
  }

  free(temp);
}


void *process_header(ogg_packet *op, int *frame_size,
		     SpeexHeader **header,
		     SpeexStereoState *stereo, decoder_callbacks_t *cb,
		     void *callback_arg)
{
   void *st;
   SpeexMode *mode;
   int modeID;
   SpeexCallback callback;
   int enhance = ENHANCE_AUDIO;

   *header = speex_packet_to_header((char*)op->packet, op->bytes);
   if (!*header) {
           cb->printf_error(callback_arg, ERROR, _("Cannot read header"));
     return NULL;
   }
   if ((*header)->mode >= SPEEX_NB_MODES || (*header)->mode < 0) {
     cb->printf_error(callback_arg, ERROR, 
		      _("Mode number %d does not (any longer) exist in this version"),
	      (*header)->mode);
     return NULL;
   }

   modeID = (*header)->mode;
   mode = speex_mode_list[modeID];

   if (mode->bitstream_version < (*header)->mode_bitstream_version) {
     cb->printf_error(callback_arg, ERROR, _("The file was encoded with a newer version of Speex.\n You need to upgrade in order to play it.\n"));
     return NULL;
   }
   if (mode->bitstream_version > (*header)->mode_bitstream_version) {
     cb->printf_error(callback_arg, ERROR, _("The file was encoded with an older version of Speex.\nYou would need to downgrade the version in order to play it."));
     return NULL;
   }

   st = speex_decoder_init(mode);
   speex_decoder_ctl(st, SPEEX_SET_ENH, &enhance);
   speex_decoder_ctl(st, SPEEX_GET_FRAME_SIZE, frame_size);

   callback.callback_id = SPEEX_INBAND_STEREO;
   callback.func = speex_std_stereo_request_handler;
   callback.data = stereo;
   speex_decoder_ctl(st, SPEEX_SET_HANDLER, &callback);

   speex_decoder_ctl(st, SPEEX_SET_SAMPLING_RATE, &(*header)->rate);

   return st;
}


int read_speex_header (decoder_t *decoder)
{
  speex_private_t *priv = decoder->private;
  transport_t *trans = decoder->source->transport;
  int packet_count = 0;
  int stream_init = 0;
  char *data;
  int nb_read;

  while (packet_count < 2) {
    /*Get the ogg buffer for writing*/
    data = ogg_sync_buffer(&priv->oy, 200);

    /*Read bitstream from input file*/
    nb_read = trans->read(decoder->source, data, sizeof(char), 200);
    ogg_sync_wrote(&priv->oy, nb_read);

    /*Loop for all complete pages we got (most likely only one)*/
    while (ogg_sync_pageout(&priv->oy, &priv->og)==1) {

      if (stream_init == 0) {
	ogg_stream_init(&priv->os, ogg_page_serialno(&priv->og));
	stream_init = 1;
      }

      /*Add page to the bitstream*/
      ogg_stream_pagein(&priv->os, &priv->og);

      /*Extract all available packets FIXME: EOS!*/
      while (ogg_stream_packetout(&priv->os, &priv->op)==1) {
	/*If first packet, process as Speex header*/
	if (packet_count==0) {
	  priv->st = process_header(&priv->op, 
				    &priv->frame_size,
				    &priv->header,
				    &priv->stereo,
				    decoder->callbacks,
				    decoder->callback_arg);

	  if (!priv->st)
	    return 0;

	  decoder->actual_fmt.rate = priv->header->rate;
	  priv->frames_per_packet = priv->header->frames_per_packet; 
	  decoder->actual_fmt.channels = priv->header->nb_channels;
	  priv->vbr = priv->header->vbr;

	  if (!priv->frames_per_packet)
	    priv->frames_per_packet=1;
	

	  priv->output = calloc(priv->frame_size * 
				decoder->actual_fmt.channels * 
				priv->frames_per_packet, sizeof(float));
	  priv->output_start = 0;
	  priv->output_left = 0;

	} else if (packet_count==1){
	  priv->comment_packet_len = priv->op.bytes;
	  priv->comment_packet = malloc(sizeof(char) * 
					priv->comment_packet_len);
	  memcpy(priv->comment_packet, priv->op.packet,
		 priv->comment_packet_len);
	}
	
	packet_count++;
      }
    }
  }

  return 1;
}
