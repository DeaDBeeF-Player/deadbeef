/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: ogg123.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __OGG123_H__
#define __OGG123_H__

#include <ogg/os_types.h>
#include "audio.h"
#include "playlist.h"

typedef struct ogg123_options_t {
  int verbosity;              /* Verbose output if > 1, quiet if 0 */

  int shuffle;                /* Should we shuffle playing? */
  int repeat;                 /* Repeat playlist indefinitely? */
  ogg_int64_t delay;          /* delay (in millisecs) for skip to next song */
  int nth;                    /* Play every nth chunk */
  int ntimes;                 /* Play every chunk n times */
  double seekoff;             /* Offset to seek to */
  double endpos;              /* Position in file to play to (greater than seekpos) */
  int seekmode;               /* DECODER_SEEK_[NONE | START | CUR */

  long buffer_size;           /* Size of audio buffer */
  float prebuffer;            /* Percent of buffer to fill before playing */
  long input_buffer_size;     /* Size of input audio buffer */
  float input_prebuffer;

  char *default_device;       /* Name of default driver to use */

  audio_device_t *devices;    /* Audio devices to use */

  double status_freq;         /* Number of status updates per second */

  int remote;                 /* Remotely controlled */

  playlist_t *playlist;       /* List of files to play */

} ogg123_options_t;

typedef struct signal_request_t {
  int cancel;
  int skipfile;
  int exit;
  int pause;
  ogg_int64_t last_ctrl_c;
} signal_request_t;

#endif /* __OGG123_H__ */
