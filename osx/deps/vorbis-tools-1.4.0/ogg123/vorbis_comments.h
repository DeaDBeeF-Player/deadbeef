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
 
 last mod: $Id: vorbis_comments.h 16825 2010-01-27 04:14:08Z xiphmont $
 
********************************************************************/


#ifndef __VORBIS_COMMENTS_H__
#define __VORBIS_COMMENTS_H__

#include "format.h"


void print_vorbis_comment (char *comment, decoder_callbacks_t *cb, 
			   void *callback_arg);

#endif /* __VORBIS_COMMENTS_H__ */
