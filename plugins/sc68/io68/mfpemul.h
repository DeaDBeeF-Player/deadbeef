/**
 * @ingroup   io68_devel
 * @file      io68/mfpemul.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/03/20
 * @brief     MFP-68901 emulator.
 *
 * $Id: mfpemul.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MFPEMUL_H_
#define _MFPEMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

#define TIMER_A   0   /**< MFP timer 'A' */
#define TIMER_B   1   /**< MFP timer 'B' */
#define TIMER_C   2   /**< MFP timer 'C' */
#define TIMER_D   3   /**< MFP timer 'D' */

/** MFP shadow register array. */
extern u8 mfp[0x40];

/** MFP reset.
 */
int MFP_reset(void);

/** MFP init.
 */
int MFP_init(void);

/** MFP get Timer Data register.
 *
 *  @param  timer  Timer-id (0:A 1:B 2:C 3:D).
 *  @param  cycle  Current cycle.
 *
 *  @return timer data register (TDR) value
 */
u8 MFP_getTDR(int timer, cycle68_t cycle);

/** MFP write Timer data register.
 *
 *  @param  timer  Timer-id (0:A 1:B 2:C 3:D).
 *  @param  v      New timer data register (TDR) value.
 *  @param  cycle  current cycle.
 *
 */
void MFP_putTDR(int timer, u8 v, cycle68_t cycle);

/** MFP write Timer control register.
 *
 *  @param  timer  Timer-id (0:A 1:B 2:C 3:D).
 *  @param  v      New timer control register (TCR) value.
 *  @param  cycle  current cycle.
 *
 */
void MFP_putTCR(int timer, u8 v, cycle68_t cycle);

/** Get MFP pending interruption.
 *
 *  @param  cycle  Current cycle.
 *
 *  @return interruption info structure.
 *  @retval 0 no pending interruption.
 *
 */
int68_t * MFP_interrupt(cycle68_t cycle);

/** Get cycle for the next MFP interruption.
 *
 *  @param  cycle  Current cycle.
 *
 *  @return cycle when MFP will interrupt
 *  @retval IO68_NO_INT no interrupt will occur.
 */
cycle68_t MFP_nextinterrupt(cycle68_t cycle);

/** Change cycle count base
 *
 *  @param  subcycle  New base for internal cycle counter.
 */
void MFP_subcycle(cycle68_t subcycle);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MFPEMUL_H_ */
