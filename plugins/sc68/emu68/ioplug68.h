/**
 * @ingroup   emu68_devel
 * @file      ioplug68.h
 * @date      1999/03/13
 * @brief     68k IO plugger.
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: ioplug68.h,v 2.1 2003/09/30 06:29:57 benjihan Exp $
 *
 *   EMU68ioplug is user interface for EMU68 IO pluging and mapping
 *   facilities. It provides function for warm plugging/unplugging of IO
 *   processors. Limitations are explained in mem68.h detailled description.
 *
 */
 
/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _IOPLUG68_H_
#define _IOPLUG68_H_

#include "emu68/struct68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Unplug all IO.
 *
 *    Process EMU68ioplug_unplug() function for all plugged IO.
 *
 *  @ingroup emu68_devel
*/
void EMU68ioplug_unplug_all(void);

/**  Unplug an IO.
 *
 *     The EMU68ioplug_unplug() function removes an IO from plugged IO list
 *     and reset memory access handler for its area.
 *
 *  @ingroup emu68_devel
 *
 *  @param   io  Address of IO structure to unplug.
 *
 *  @return   error-code
 *  @retval   0   Success
 *  @retval   <0  Error (probably no matching IO)
 */
int EMU68ioplug_unplug(io68_t *io);

/**  Plug an IO.
 *
 *     The EMU68ioplug() function add an IO to plugged IO list and add
 *     suitable memory access handlers.
 *
 *  @ingroup emu68_devel
 *
 *  @param  io  Address of IO structure to plug.
 */
void EMU68ioplug(io68_t *io);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _IOPLUG68_H_ */
