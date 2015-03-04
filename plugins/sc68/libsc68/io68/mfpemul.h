/**
 * @ingroup   lib_io68
 * @file      io68/mfpemul.h
 * @brief     MFP-68901 emulator header.
 * @author    Benjamin Gerard
 * @date      1999/03/20
 */

/* Copyright (c) 1998-2015 Benjamin Gerard */

#ifndef IO68_MFPEMUL_H
#define IO68_MFPEMUL_H

#include "io68_api.h"
#include "emu68/struct68.h"

/**
 * @defgroup  lib_io68_mfp  MFP-68901 emulator
 * @ingroup   lib_io68
 * @brief     MFP-68901 (Atari-ST timers) emulator.
 *
 *  Motorola Multi Function Peripheral is a multi purpose IO chip:
 * - 8 bit parallele port.
 * - Each bit has indepedant direction
 * - Each bit can be a interruption source
 * - 16 interruption sources
 * - 4 universal timers (most part of this emulator)
 * - Integrated serial interface.
 * - 24 registers
 *
 * @p Registers
 *
 * - 00 @b GPIP (General Purpose I/O Interrupt port):
 *
 *         Data Register for the 8 bit port for reading or writing.
 *         - bit#0 : Centronics (parrallele port)
 *         - bit#1 : Rs232 carrier detection
 *         - bit#1 : Rs232 CTS (clear to send)
 *         - bit#3 : Blitter
 *         - bit#4 : MIDI/Keyboard irq
 *         - bit#5 : FDC/DMA interrupt
 *         - bit#6 : Rs232 ring
 *         - bit#7 : Monochorm monitor detect
 *
 * - 01 @b AER (Active Edge Register):
 *
 *         If port's bit are an interruption source this register
 *         selects interruption direction (0:descending 1:ascending).
 *
 * - 02 @b DDR (Data Direction Register):
 *
 *         Port's bit direction select. 0 => input / 1 => output.
 *
 * - 03 @b IERA (Interrupt Enable Register A)
 * - 04 @b IERB (Interrupt Enable Register B)
 *
 *         Control the 16 interrupt source. 0 => Disable / 1 => Enable.
 *     - iera bit#7 : bit 7 of I/O port (highest priority)
 *     - iera bit#6 : bit 6 of I/O port
 *     - iera bit#5 : Timer A
 *     - iera bit#4 : Receive Buffer Full
 *     - iera bit#3 : Receive Error
 *     - iera bit#2 : Send Buffer Empty
 *     - iera bit#1 : Send Error
 *     - iera bit#0 : Timer B
 *     - ierb bit#7 : bit 5 of I/O port
 *     - ierb bit#6 : bit 4 of I/O port
 *     - ierb bit#5 : Timer C
 *     - ierb bit#4 : Timer D
 *     - ierb bit#3 : bit 3 of I/O port
 *     - ierb bit#2 : bit 2 of I/O port
 *     - ierb bit#1 : bit 1 of I/O port
 *     - ierb bit#0 : bit 0 of I/O port (lowest priority)
 *
 * - 05 @b IPRA (Interrupt Pending Register A)
 * - 06 @b IPRB (Interrupt Pending Register B)
 *
 *         Bit is set if an interruption occurs. Interruption have to
 *         be enable (see IER). If the MFP can produce an interruption
 *         vector the bit is cleared automatically at this moment. In
 *         the contrary the bit should be clear by writing this
 *         register (~1<<bit).
 *
 * - 07 @b ISRA (Interrupt In Service Register A)
 * - 08 @b ISRB (Interrupt In Service Register B)
 *
 *         In AEI mode the bit is cleared after the MFP has produced
 *         the interruption vector and a new event can trigger a new
 *         interruption while another is been proced by the CPU. In
 *         SEI mode the bit is set and the interrupt routine have to
 *         clear it by writing this register (~1<<bit). While ISR bit
 *         is 1 all less prioritary bit are unmasked (?). When IPR is
 *         cleared (vector has been produced) a similar interrupt may
 *         occur. Anyway this interruption and the one less prioritary
 *         can be process only if ISR is disabled.
 *
 * - 09 @b IMRA (Interrupt Mask Register A)
 * - 0A @b IMRB (Interrupt Mask Register B)
 *
 *         If register allow ennable interrupt event to occur but not
 *         to produce an interrupt if corresponding bit in IMR is
 *         clear.
 *
 * - 0B @b VR (Vector Register)
 *
 *         - bit#0-2 : unknown ?
 *         - bit#3   : 0:AEI 1:SEI (automatic/software end of interrupt)
 *         - bit#4-7 : 4 MSB of interrupt vector
 *
 * - 0C @b TACR  (Timer A Control Register)
 * - 0D @b TBCR  (Timer B Control Register)
 * - 0E @b TCDCR (Timer C/D Control Register)
 * - 0F @b TADR  (Timer A Data Register)
 * - 10 @b TBDR  (Timer B Data Register)
 * - 11 @b TCDR  (Timer C Data Register)
 * - 12 @b TDDR  (Timer D Data Register)
 *
 *         @see Programming MFP Timers
 *
 * - 13 @b SCR (Synchronous Character Register)
 *
 *         Within a syncronous data transfer writing a specific value
 *         in this register use to start transfer as soon has it is
 *         recieved.
 *
 * - 14 UCR,USART (Control Register)
 *
 *         USART is Universal Synchronous/Asynchronous Receiver/Transmitter.
 *         -bit#0   : unused
 *         -bit#1   : 0:odd 1:even
 *         -bit#2   : 0:parity-off 1:parity-on
 *         -bit#3-4 : Transfert mode
 *                    -00 Start 0, Stop 0, Synchronous
 *                    -01 Start 1, Stop 1, Asynchronous
 *                    -10 Start 1, Stop 1.5, Asynchronous
 *                    -11 Start 1, Stop 2, Asynchronous
 *         -bit#5-6 : Data Length (8-value)
 *         -bit#7   : 0 use Timer-C frequency directly (synchornous only),
 *                    1 use predivisor by 16
 *
 * - 15 RSR (Receiver Status Register)
 *
 *      -bit#0 : Receiver Enable Bit
 *      -bit#1 : Synchronous Strip Enable (sync)
 *      -bit#2 : Match (sync)/Character in progress (async)
 *      -bit#3 : Found-Search(sync)/Break Detect(async)
 *      -bit#4 : Frame Error
 *      -bit#5 : Parity Error
 *      -bit#6 : Overrun Error
 *      -bit#7 : Buffer Full
 *
 * - 16 TSR (Tranmitter Status Register)
 *
 *      -bit#0 : Transmitter Enable
 *      -bit#1 : High bit
 *      -bit#2 : Low bit
 *      -bit#3 : Break
 *      -bit#4 : End of transmission
 *      -bit#5 : Auto Turnaround
 *      -bit#6 : Underrun Error
 *      -bit#7 : Buffer Empty
 *
 * - 17 UDR,USART (DataRegister)
 *
 *      Data to send (write access) or recieve (read access)
 *
 * @p MFP interrupt vector table
 *
 *   MSB of the vector number (the x value below) is stored in the MFP
 *   vector register (register #11) bit#4 to bit#7.
 *
 * - x0 (initially disabled)-Parallel port interrupt handler
 * - x1 (initially disabled)-RS-232 carrier detect pin handler
 * - x2 (initially disabled)-RS-232 clear to send pin handler
 * - x3 (initially disabled)-Graphics blitter chip done interrupt
 *      handler (see below!)
 * - x4 (initially disabled) MFP Timer D done handler
 * - x5 200Hz System Clock (MFP Timer C) Handler
 * - x6 Keyboard or MIDI interrupt handler
 * - x7 (initially disabled) Floppy/hard disk data request handler
 * - x8 (initially disabled) Horizontal blank counter/MFP Timer B
 * - x9 RS-232 transmit error handler
 * - xA RS-232 transmit buffer empty handler
 * - xB RS-232 receive error handler
 * - xC RS-232 receive buffer full handler
 * - xD (initially disabled) MFP timer A
 * - xE (initially disabled) RS-232 ring detect pin
 * - xF (initially disabled) Monochrome/color monitor change detecter
 *
 * @p Programming MFP Timers
 *
 *     TODO
 *
 * @{
 */

/**
 * @name MFP-68901 timers
 * @{
 */

/**
 * MFP-68901 timer identifers.
 */
enum mfp_timer_e {
  TIMER_A=0,   /**< MFP timer 'A' */
  TIMER_B,     /**< MFP timer 'B' */
  TIMER_C,     /**< MFP timer 'C' */
  TIMER_D      /**< MFP timer 'D' */
};

/**
 * bogo-cycle definition (1 Bogo = 192 "8mhz 68K" = 625 "mfp" cycle).
 */
typedef cycle68_t bogoc68_t;

/**
 * Timer definition struct.
 */
typedef struct {
  addr68_t vector; /**< Interrupt vector.                         */
  u8 level;        /**< Interrupt level.                          */
  u8 bit;          /**< Bit mask for Interrupt stuffes registers. */
  u8 channel;      /**< Channel A=0 or B=2.                       */
  u8 letter;       /**< 'A','B','C' or 'D'.                       */
} mfp_timer_def_t;

/**
 * MFP-68901 timer struct.
 */
typedef struct
{
  mfp_timer_def_t def; /**< Timer definition.                          */

  bogoc68_t cti;       /**< bogo-cycle to next timer interrupt.        */
  uint_t    tdr_cur;   /**< Timer Data register current value.         */
  uint_t    tdr_res;   /**< Timer Data register reset value.           */
  uint_t    tcr;       /**< Timer control register (prescaler) [0..7]. */

  bogoc68_t psc;       /**< prescale counter.                          */

  /* On Interrupt */
  uint_t   int_lost;   /**< Interrupts missed (should be 0).           */
  uint_t   int_mask;   /**< Interrupts ignored (masked or whatever)    */
  uint_t   int_fall;   /**< Interrupts counter.                        */

  interrupt68_t interrupt; /**< Interruption info.                     */
} mfp_timer_t;

/**
 * @}
 */

/**
 * MFP register name.
 */
enum {
  GPIP  = 0x01,
  AER   = 0x03,
  DDR   = 0x05,
  IERA  = 0x07,
  IERB  = 0x09,
  IPRA  = 0x0B,
  IPRB  = 0x0D,
  ISRA  = 0x0F,
  ISRB  = 0x11,
  IMRA  = 0x13,
  IMRB  = 0x15,
  VR    = 0x17,
  TACR  = 0x19,
  TBCR  = 0x1B,
  TCDCR = 0x1D,
  TADR  = 0x1F,
  TBDR  = 0x21,
  TCDR  = 0x23,
  TDDR  = 0x25,
  SCR   = 0x27,
  UCR   = 0x29,
  RSR   = 0x2B,
  TSR   = 0x2D,
  UDR   = 0x2F,
};

/**
 * MFP-68901 emulator. */
typedef struct {
  u8 map[0x40];                       /**< Registers map.        */
  mfp_timer_t timers[4];              /**< Timers.               */
} mfp_t;

/**
 * @name  MFP-68901 emulator library.
 * @{
 */

IO68_EXTERN
/**
 * MFP-68901 emulator library initialization.
 *
 *     The mfp_init() function intialize MFP emulator library.  It
 *     must be call prior to any other mfp_ function.
 *
 * @return  error-code
 * @retval  0  Success
 * @retval -1  Failure
 */
int mfp_init(void);

IO68_EXTERN
/**
 * MFP-68901 emulator library shutdown.
 */
void mfp_shutdown(void);

/**
 * @} */

/**
 * @name MFP-68901 emulator functions.
 * @{
 */

IO68_EXTERN
/**
 * Setup mfp instance.
 *
 * @param  mfp    mfp emulator instance.
 *
 * @return Error-code
 * @retval  0  Success
 * @retval -1  Failure
 */
int mfp_setup(mfp_t * const mfp);

IO68_EXTERN
/**
 * Cleanup mfp instance.
 *
 * @param  mfp    mfp emulator instance.
 *
 */
void mfp_cleanup(mfp_t * const mfp);


IO68_EXTERN
/**
 * Reset mfp instance.
 *
 * @param  mfp    mfp emulator instance.
 * @param  bogoc  bogo cycles
 *
 * @return Error-code
 * @retval  0  Success
 * @retval -1  Failure
 */
int mfp_reset(mfp_t * const mfp, const bogoc68_t bogoc);

IO68_EXTERN
/**
 * Destroy mfp instance.
 *
 * @param  mfp    mfp emulator instance.
 */
void mfp_destroy(mfp_t * const mfp);

/**
 * @}
 */

IO68_EXTERN
/**
 * MFP get Timer Data register.
 *
 * @param  mfp    mfp emulator instance.
 * @param  timer  Timer-id (see mfp_timer_e).
 * @param  bogoc  Current bogo-cycle.
 *
 * @return timer data register (TDR) value
 */
int68_t mfp_get_tdr(mfp_t * const mfp,
                    const int timer, const bogoc68_t bogoc);

IO68_EXTERN
/**
 * MFP write Timer data register.
 *
 * @param  mfp    mfp emulator instance.
 * @param  timer  Timer-id (see mfp_timer_e).
 * @param  v      New timer data register (TDR) value.
 * @param  bogoc  Current bogo-cycle.
 *
 */
void mfp_put_tdr(mfp_t * const mfp,
                 int timer, int68_t v, const bogoc68_t bogoc);

IO68_EXTERN
/**
 * MFP write Timer control register.
 *
 * @param  mfp    mfp emulator instance.
 * @param  timer  Timer-id (see mfp_timer_e).
 * @param  v      New timer control register (TCR) value.
 * @param  bogoc  Current bogo-cycle.
 *
 */
void mfp_put_tcr(mfp_t * const mfp,
                 int timer, int68_t v, const bogoc68_t bogoc);

IO68_EXTERN
/**
 * Get MFP pending interruption.
 *
 * @param  mfp    mfp emulator instance.
 * @param  bogoc  Current bogo-cycle.
 *
 * @return interruption info structure.
 * @retval 0 no pending interruption.
 *
 */
interrupt68_t * mfp_interrupt(mfp_t * const mfp, const bogoc68_t bogoc);

IO68_EXTERN
/**
 * Get cycle for the next MFP interruption.
 *
 * @param  mfp  mfp emulator instance.
 *
 * @return CPU-cycle when MFP will interrupt
 * @retval IO68_NO_INT no interrupt will occur.
 */
bogoc68_t mfp_nextinterrupt(const mfp_t * const mfp);

IO68_EXTERN
/**
 * Change cycle count base.
 *
 * @param  mfp    mfp emulator instance.
 * @param  bogoc  New base for internal cycle counter.
 */
void mfp_adjust_bogoc(mfp_t * const mfp, const bogoc68_t bogoc);

/**
 * @}
 */

#endif
