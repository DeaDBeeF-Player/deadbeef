/*
 * vgfilter.h (c) 2007,2008 William Poetra Yoga Hadisoeseno
 * based on:
 * vgplay.h 1.0 (c) 2003 John Morton 
*/

/* vgplay.h 1.0 (c) 2003 John Morton 
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


#ifndef __VGPLAY_H
#define __VGPLAY_H

/* Default pre-amp in dB */
#define VG_PREAMP_DB         0.0

typedef struct {
  float scale_factor;  /* The scale factor */
  float max_scale;     /* The maximum scale factor before clipping occurs */
} vgain_state;


/* Initializes the ReplayGain the vgain_state structure for a track. */
extern void vg_init(vgain_state *vg_state, vorbis_comment *vc);

/* The filter where VorbisGain is applied */
extern void vg_filter(float **pcm, long channels, long samples, void *filter_param);

#endif /* __VGPLAY_H */
