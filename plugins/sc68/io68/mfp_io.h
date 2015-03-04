/**
 * @ingroup   io68_devel
 * @file      io68/mfp_io.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/03/20
 * @brief     MFP-68901 emulator plugin.
 *
 * $Id: mfp_io.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MFP_IO_H_
#define _MFP_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

/** EMU68 compatible IO plugin for MFP emulation. */
extern io68_t mfp_io;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MFP_IO_H_ */
