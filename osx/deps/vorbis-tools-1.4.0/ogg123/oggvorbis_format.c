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

 last mod: $Id: oggvorbis_format.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "transport.h"
#include "format.h"
#include "vorbis_comments.h"
#include "utf8.h"
#include "i18n.h"

#ifdef HAVE_OV_READ_FILTER
#include "vgfilter.h"
#endif

typedef struct ovf_private_t {
  OggVorbis_File vf;
  vorbis_comment *vc;
  vorbis_info *vi;
  int current_section;

  int bos; /* At beginning of logical bitstream */

  decoder_stats_t stats;
#ifdef HAVE_OV_READ_FILTER
  vgain_state vg;
#endif
} ovf_private_t;

/* Forward declarations */
format_t oggvorbis_format;
ov_callbacks vorbisfile_callbacks;


void print_vorbis_stream_info (decoder_t *decoder);
void print_vorbis_comments (vorbis_comment *vc, decoder_callbacks_t *cb, 
			    void *callback_arg);


/* ----------------------------------------------------------- */


int ovf_can_decode (data_source_t *source)
{
  return 1;  /* The file transport is tested last, so always try it */
}


decoder_t* ovf_init (data_source_t *source, ogg123_options_t *ogg123_opts,
		     audio_format_t *audio_fmt,
		     decoder_callbacks_t *callbacks, void *callback_arg)
{
  decoder_t *decoder;
  ovf_private_t *private;
  int ret;


  /* Allocate data source structures */
  decoder = malloc(sizeof(decoder_t));
  private = malloc(sizeof(ovf_private_t));

  if (decoder != NULL && private != NULL) {
    decoder->source = source;
    decoder->actual_fmt = decoder->request_fmt = *audio_fmt;
    decoder->format = &oggvorbis_format;
    decoder->callbacks = callbacks;
    decoder->callback_arg = callback_arg;
    decoder->private = private;

    private->bos = 1;
    private->current_section = -1;

    private->stats.total_time = 0.0;
    private->stats.current_time = 0.0;
    private->stats.instant_bitrate = 0;
    private->stats.avg_bitrate = 0;

#ifdef HAVE_OV_READ_FILTER
    private->vg.scale_factor = 1.0;
    private->vg.max_scale = 1.0;
#endif
  } else {
    fprintf(stderr, _("ERROR: Out of memory.\n"));
    exit(1);
  }

  /* Initialize vorbisfile decoder */

  ret = ov_open_callbacks (decoder, &private->vf, NULL, 0, 
			   vorbisfile_callbacks);

  if (ret < 0) {
    free(private);
/*    free(source);     nope.  caller frees. */
    return NULL;
  }

  return decoder;
}


int ovf_read (decoder_t *decoder, void *ptr, int nbytes, int *eos,
	      audio_format_t *audio_fmt)
{
  ovf_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;
  int bytes_read = 0;
  int ret;
  int old_section;

  /* Read comments and audio info at the start of a logical bitstream */
  if (priv->bos) {
    priv->vc = ov_comment(&priv->vf, -1);
    priv->vi = ov_info(&priv->vf, -1);

    decoder->actual_fmt.rate = priv->vi->rate;
    decoder->actual_fmt.channels = priv->vi->channels;

    switch(decoder->actual_fmt.channels){
    case 1:
      decoder->actual_fmt.matrix="M";
      break;
    case 2:
      decoder->actual_fmt.matrix="L,R";
      break;
    case 3:
      decoder->actual_fmt.matrix="L,C,R";
      break;
    case 4:
      decoder->actual_fmt.matrix="L,R,BL,BR";
      break;
    case 5:
      decoder->actual_fmt.matrix="L,C,R,BL,BR";
      break;
    case 6:
      decoder->actual_fmt.matrix="L,C,R,BL,BR,LFE";
      break;
    case 7:
      decoder->actual_fmt.matrix="L,C,R,SL,SR,BC,LFE";
      break;
    case 8:
      decoder->actual_fmt.matrix="L,C,R,SL,SR,BL,BR,LFE";
      break;
    default:
      decoder->actual_fmt.matrix=NULL;
      break;
    }

#ifdef HAVE_OV_READ_FILTER
    vg_init(&priv->vg, priv->vc);
#endif

    print_vorbis_stream_info(decoder);
    print_vorbis_comments(priv->vc, cb, decoder->callback_arg);
    priv->bos = 0;
  }

  *audio_fmt = decoder->actual_fmt;

  /* Attempt to read as much audio as is requested */
  while (nbytes >= audio_fmt->word_size * audio_fmt->channels) {

    old_section = priv->current_section;
#ifdef HAVE_OV_READ_FILTER
    ret = ov_read_filter(&priv->vf, ptr, nbytes, audio_fmt->big_endian,
			 audio_fmt->word_size, audio_fmt->signed_sample,
			 &priv->current_section,
			 vg_filter, &priv->vg);
#else
    ret = ov_read(&priv->vf, ptr, nbytes, audio_fmt->big_endian,
		  audio_fmt->word_size, audio_fmt->signed_sample,
		  &priv->current_section);
#endif

    if (ret == 0) {

      /* EOF */
      *eos = 1;
      break;

    } else if (ret == OV_HOLE) {

      if (cb->printf_error != NULL)
	cb->printf_error(decoder->callback_arg, INFO,
			   _("--- Hole in the stream; probably harmless\n"));

    } else if (ret < 0) {

      if (cb->printf_error != NULL)
	cb->printf_error(decoder->callback_arg, ERROR,
			 _("=== Vorbis library reported a stream error.\n"));

      /* EOF */
      *eos = 1;
      break;
    } else {

      bytes_read += ret;
      ptr = (void *)((unsigned char *)ptr + ret);
      nbytes -= ret;

      /* did we enter a new logical bitstream? */
      if (old_section != priv->current_section && old_section != -1) {
	
	*eos = 1;
	priv->bos = 1; /* Read new headers next time through */
	break;
      }
    }

  }

  return bytes_read;
}


int ovf_seek (decoder_t *decoder, double offset, int whence)
{
  ovf_private_t *priv = decoder->private;
  int ret;
  double cur;

  if (whence == DECODER_SEEK_CUR) {
    cur = ov_time_tell(&priv->vf);
    if (cur >= 0.0)
      offset += cur;
    else
      return 0;
  }

  ret = ov_time_seek(&priv->vf, offset);
  if (ret == 0)
    return 1;
  else
    return 0;
}


decoder_stats_t *ovf_statistics (decoder_t *decoder)
{
  ovf_private_t *priv = decoder->private;
  long instant_bitrate;
  long avg_bitrate;

  /* ov_time_tell() doesn't work on non-seekable streams, so we use
     ov_pcm_tell()  */
  priv->stats.total_time = (double) ov_pcm_total(&priv->vf, -1) /
    (double) decoder->actual_fmt.rate;
  priv->stats.current_time = (double) ov_pcm_tell(&priv->vf) / 
    (double) decoder->actual_fmt.rate;

  /* vorbisfile returns 0 when no bitrate change has occurred */
  instant_bitrate = ov_bitrate_instant(&priv->vf);
  if (instant_bitrate > 0)
    priv->stats.instant_bitrate = instant_bitrate;

  avg_bitrate = ov_bitrate(&priv->vf, priv->current_section);
  /* Catch error case caused by non-seekable stream */
  priv->stats.avg_bitrate = avg_bitrate > 0 ? avg_bitrate : 0;


  return malloc_decoder_stats(&priv->stats);
}


void ovf_cleanup (decoder_t *decoder)
{
  ovf_private_t *priv = decoder->private;

  ov_clear(&priv->vf);

  free(decoder->private);
  free(decoder);
}


format_t oggvorbis_format = {
  "oggvorbis",
  &ovf_can_decode,
  &ovf_init,
  &ovf_read,
  &ovf_seek,
  &ovf_statistics,
  &ovf_cleanup,
};


/* ------------------- Vorbisfile Callbacks ----------------- */

size_t vorbisfile_cb_read (void *ptr, size_t size, size_t nmemb, void *arg)
{
  decoder_t *decoder = arg;

  return decoder->source->transport->read(decoder->source, ptr, size, nmemb);
}

int vorbisfile_cb_seek (void *arg, ogg_int64_t offset, int whence)
{
  decoder_t *decoder = arg;

  return decoder->source->transport->seek(decoder->source, offset, whence);
}

int vorbisfile_cb_close (void *arg)
{
  return 1; /* Ignore close request so transport can be closed later */
}

long vorbisfile_cb_tell (void *arg)
{
  decoder_t *decoder = arg;

  return decoder->source->transport->tell(decoder->source);
}


ov_callbacks vorbisfile_callbacks = {
  &vorbisfile_cb_read,
  &vorbisfile_cb_seek,
  &vorbisfile_cb_close,
  &vorbisfile_cb_tell
};


/* ------------------- Private functions -------------------- */


void print_vorbis_stream_info (decoder_t *decoder)
{
  ovf_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;


  if (cb == NULL || cb->printf_metadata == NULL)
    return;

  cb->printf_metadata(decoder->callback_arg, 2,
		      _("Ogg Vorbis stream: %d channel, %ld Hz"),
		      priv->vi->channels,
		      priv->vi->rate);

  cb->printf_metadata(decoder->callback_arg, 3,
		      _("Vorbis format: Version %d"), 
		      priv->vi->version);

  cb->printf_metadata(decoder->callback_arg, 3,
		      _("Bitrate hints: upper=%ld nominal=%ld lower=%ld "
		      "window=%ld"), 
		      priv->vi->bitrate_upper,
		      priv->vi->bitrate_nominal,
		      priv->vi->bitrate_lower,
		      priv->vi->bitrate_window);

  cb->printf_metadata(decoder->callback_arg, 3,
		      _("Encoded by: %s"), priv->vc->vendor);
}

void print_vorbis_comments (vorbis_comment *vc, decoder_callbacks_t *cb, 
			    void *callback_arg)
{
  int i;
  char *temp = NULL;
  int temp_len = 0;

  for (i = 0; i < vc->comments; i++) {

    /* Gotta null terminate these things */
    if (temp_len < vc->comment_lengths[i] + 1) {
      temp_len = vc->comment_lengths[i] + 1;
      temp = realloc(temp, sizeof(char) * temp_len);
    }

    strncpy(temp, vc->user_comments[i], vc->comment_lengths[i]);
    temp[vc->comment_lengths[i]] = '\0';

    print_vorbis_comment(temp, cb, callback_arg);
  }

  free(temp);
}
