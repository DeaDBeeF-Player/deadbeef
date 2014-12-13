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

 last mod: $Id: cmdline_options.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __CMDLINE_OPTIONS_H__
#define __CMDLINE_OPTIONS_H__

#include "ogg123.h"
#include "cfgfile_options.h"
#include "audio.h"

int parse_cmdline_options (int argc, char **argv,
			   ogg123_options_t *ogg123_opts,
			   file_option_t    *file_opts);
void cmdline_usage (void);


#endif /* __CMDLINE_OPTIONS_H__ */
