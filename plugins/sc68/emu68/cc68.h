/**
 * @ingroup   emu68_devel
 * @file      cc68.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      13/03/1999
 * @brief     Condition code function table.
 * @version   $Id: cc68.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 *
 *    Condition code function table is used by EMU68 for conditional
 *    instructions emulation including Bcc, Scc and DCcc. The 4 condition
 *    bits of instruction is used as index to call corresponding test
 *    function. Each test function performs suitable SR bits operations and
 *    return 0 if condition is false and other if condition is satisfied.
 *
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _CC68_H_
#define _CC68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Code condition testing function table */
extern int (*is_cc[8])(void );

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CC68_H_ */
