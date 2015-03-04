/**
 * @ingroup   emu68_devel
 * @file      emu68/struct68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      13/03/1999
 * @brief     Struture definitions.
 * @version   $Id: struct68.h,v 2.2 2003/09/30 06:29:57 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _STRUCT68_H_
#define _STRUCT68_H_

#include "emu68/type68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** IO no pending interruption return value.
 *
 *    The next_int function of IO plugin must return IO68_NO_INT when no
 *    interruption are expected.
 */
#define IO68_NO_INT (0x80000000)

#ifndef NULL
# define NULL 0L /**< Default NULL constant. */
#endif

/** Mask for memory overflow.
 */
#define MEM68MSK ((1<<19)-1)   /* 512 Kb memory */
/* #define MEM68MSK (reg68.memmsk) */

/** @name  Memory access caller type
 *  @{
 */

/** Read memory function. */
typedef u32 (*memrfunc68_t)(u32 addr, cycle68_t cycle);

/** Write memory function. */
typedef void (*memwfunc68_t)(u32 addr, u32 value, cycle68_t cycle);

/**@}*/

/** First level (16 lines) decoder function. */
typedef void (linefunc68_t)(int, int );

/**  68K interruption exception structure.
 */
typedef struct
{
  int vector;                  /**< Interrupt vector.       */
  int level;                   /**< Interrupt level [0..7]. */
} int68_t;

/** IO emulator pluggin structure.
 *
 *      All 68K IO must have a filled io68_t structure to be warm plug or
 *      unplug with ioplug interface.
 *
 */
typedef struct _io68_t
{
  struct _io68_t * next;            /**< IO list; pointer to next.       */
  char name[32];                    /**< IO identifier name.             */
  u32 addr_low;                     /**< IO mapping area start address.  */
  u32 addr_high;                    /**< IO mapping area end address.    */
  memrfunc68_t Rfunc[3];            /**< IO read functions (B,W,L).      */
  memwfunc68_t Wfunc[3];            /**< IO write functions (B,W,L).     */
  int68_t *(*interrupt)(cycle68_t); /**< IO interruption function claim. */
  cycle68_t (*next_int)(cycle68_t); /**< IO get next interruption cycle. */
  void (*adjust_cycle)(cycle68_t);  /**< IO adjust cycle function.       */
  int (*reset)(void);               /**< IO reset function.              */
  cycle68_t rcycle_penalty;         /**< Read cycle penalty .            */
  cycle68_t wcycle_penalty;         /**< Write cycle penalty.            */
} io68_t;

/** 68K emulator registers, memory and IO.
 */
typedef struct
{
  /* Registers. */
  s32 d[8];                    /**< 68000 data registers.      */
  s32 a[8];                    /**< 68000 address registers.   */
  s32 usp;                     /**< 68000 User Stack Pointers. */
  s32 pc;                      /**< 68000 Program Counter.     */
  u32 sr;                      /**< 68000 Status Register.     */

  cycle68_t cycle;             /**< Internal cycle counter. */
  int status;                  /**< Unused !!!              */

  /* Memory. */
  u8  *mem;                    /**< Memory buffer.       */
  u32 memmsk;                  /**< Memory address mask. */
  unsigned int memsz;          /**< Memory size in byte. */

  /* Debug mode only. */
  u8  *chk;                    /**< Access-Control-Memory buffer.      */
  int framechk;                /**< ORed chk change for current frame. */

  /* IO chips. */
  int nio;                     /**< # IO plug in IO-list. */
  io68_t *iohead;              /**< Head of IO-list.      */

} reg68_t;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _STRUCT68_H_ */
