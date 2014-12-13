/*
 * vgfilter.c (c) 2007,2008 William Poetra Yoga Hadisoeseno
 * based on:
 * vgplay.c 1.0 (c) 2003 John Morton
*/

/* vgplay.c 1.0 (c) 2003 John Morton
 *
 * Portions of this file are (C) COPYRIGHT 1994-2002 by 
 * the XIPHOPHORUS Company http://www.xiph.org/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **********************************************************************
 *
 * vgfilter - a filter for ov_read_filter to enable replay gain.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "vgfilter.h"

/*
 * Initialize the replaygain parameters from vorbis comments.
 */
void vg_init(vgain_state *vg, vorbis_comment *vc) {
  float track_gain_db = 0.00, track_peak = 1.00;
  char *tag = NULL;

  if (vc) {
    if ((tag = vorbis_comment_query(vc, "replaygain_album_gain", 0))
        || (tag = vorbis_comment_query(vc, "rg_audiophile", 0)))
      track_gain_db = atof(tag);
    else if ((tag = vorbis_comment_query(vc, "replaygain_track_gain", 0))
        || (tag = vorbis_comment_query(vc, "rg_radio", 0)))
      track_gain_db = atof(tag);

    if ((tag = vorbis_comment_query(vc, "replaygain_album_peak", 0))
        || (tag = vorbis_comment_query(vc, "replaygain_track_peak", 0))
        || (tag = vorbis_comment_query(vc, "rg_peak", 0)))
      track_peak = atof(tag);
  }

  vg->scale_factor = pow(10.0, (track_gain_db + VG_PREAMP_DB)/20);
  vg->max_scale = 1.0 / track_peak;
}


/*
 * This is the filter function for the decoded Ogg Vorbis stream.
 */
void vg_filter(float **pcm, long channels, long samples, void *filter_param)
{
  int i, j;
  float cur_sample;
  vgain_state *param = filter_param;
  float scale_factor = param->scale_factor;
  float max_scale = param->max_scale;

  /* Apply the gain, and any limiting necessary */
  if (scale_factor > max_scale) {
    for(i = 0; i < channels; i++)
      for(j = 0; j < samples; j++) {
        cur_sample = pcm[i][j] * scale_factor;
        /* This is essentially the scaled hard-limiting algorithm */
        /* It looks like the soft-knee to me */
        /* I haven't found a better limiting algorithm yet... */
        if (cur_sample < -0.5)
          cur_sample = tanh((cur_sample + 0.5) / (1-0.5)) * (1-0.5) - 0.5;
        else if (cur_sample > 0.5)
          cur_sample = tanh((cur_sample - 0.5) / (1-0.5)) * (1-0.5) + 0.5;
        pcm[i][j] = cur_sample;
      }
  } else if (scale_factor > 0.0) {
    for(i = 0; i < channels; i++)
      for(j = 0; j < samples; j++)
        pcm[i][j] *= scale_factor;
  }
}
