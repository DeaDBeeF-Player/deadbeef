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

 last mod: $Id: compat.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __COMPAT_H__
#define __COMPAT_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __sun
#include <alloca.h>
#endif

/* SunOS 4 does on_exit() and everything else does atexit() */
#ifdef HAVE_ATEXIT
#define ATEXIT(x) (atexit(x))
#else
#ifdef HAVE_ON_EXIT
#define ATEXIT(x) (on_exit( (void (*)(int, void*))(x) , NULL)
#else
#define ATEXIT(x)
#warning "Neither atexit() nor on_exit() is present.  Bad things may happen when the application terminates."
#endif
#endif

#endif
