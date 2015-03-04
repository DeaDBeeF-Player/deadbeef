/**
 * @ingroup   io68_devel
 * @file      io68/shifter_io.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/06/10
 * @brief     IO plugin for shifter emulation (50/60hz probing)
 *
 * $Id: shifter_io.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _SHIFTER_IO_H_
#define _SHIFTER_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

/** EMU68 compatible plugin for Atari-ST shifter emulation.
 *
 *    The shifter emulation is limited to 50/60Hz detection used by some
 *    Atari-ST music player to adapte the replay speed. By default, this
 *    shifter always reply to be in 50hz.
 */
extern io68_t shifter_io;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SHIFTER_IO_H_ */
