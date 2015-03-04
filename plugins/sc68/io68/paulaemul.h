/**
 * @ingroup   io68_devel
 * @file      io68/paulaemul.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1998/07/18
 * @brief     Paula emulator (Amiga soundchip).
 *
 * $Id: paulaemul.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 *
 * @par About Amiga hardware registers
 *
 * Amiga hardware registers could have a different address for
 * read and write access. It is the case for DMA control register (DMACON).
 *
 * Registers could be (B)yte or (W)ord wide.
 *
 * @par SET/CLR bit
 *
 * Some hardware registers work with a SET/CLEAR bit. When writing these
 * registers the value is not copied as is. The operation depends on the
 * value of S the most signifiant bit (#15).
 * - If S is SET then all others setted bits are setted (bitwise OR).
 * - If S is CLEAR then all others setted bits are cleared (bitwise NAND).
 *
 */

/*
 *
 * @par Register map
 *
 * @code {
 *
 * // NAME SZ  ADDR    
 *
 * VHPOSR  (B) DFF006 [xxxxxxxx] Vertical/Horizontal electron bean position.
 *
 * INTREQR (W) DFF01E [-M---DCBA--------] Interrupt request read (M=Master).
 * INTREQW (W) DFF09C [-M---DCBA--------] Interrupt request write (M=Master).
 *
 * INTENAR (W) DFF01C Interrupt enable read.
 * INTENAW (W) DFF09A Interrupt enable write.
 *
 * DMACONW (W) DFF096 [S00000E00000DBCA] DMA control register write (E=Enable).
 * DMACONR (W) DFF002 [000000E00000DBCA] DMA control register read (E=Enable).
 *
 * VOICEA      DFF0A0
 * VOICEB      DFF0B0
 * VOICEC      DFF0C0
 * VOICED      DFF0D0
 *
 * VOICEX
 * +0 (L) START  [00000000 00000xxx xxxxxxxx xxxxxxx0] start address (even).
 * +4 (W) LENGHT [xxxxxxxx xxxxxxxx] Length in word (0=010000).
 * +6 (W) PERIOD [0000xxxx xxxxxxxx] Period (in paula cycle).
 * +8 (B) VOLUME [0xxxxxxx] Volume [0-64] (presume value > 64 => 64).
 * }
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _PAULA_EMUL_H_
#define _PAULA_EMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

/** Amiga Vertical/Horizontal electron bean position */
#define PAULA_VHPOSR    0x06

/** @name  Amiga interruption registers
 *  @{
 *
 *  All hardware registers involved with interruption handling use the
 *  same bit organisation : 
 *   - bit 7   Audio channel A
 *   - bit 8   Audio channel B
 *   - bit 9   Audio channel C
 *   - bit 10  Audio channel D
 *   - bit 14  Master interrupt
 *
 */

#define PAULA_INTREQR   0x1E  /**< Interruption request read */
#define PAULA_INTREQRH  0x1E  /**< Interruption request read MSB */
#define PAULA_INTREQRL  0x1F  /**< Interruption request read LSB */

#define PAULA_INTREQ    0x9C  /**< Interruption request write */
#define PAULA_INTREQH   0x9C  /**< Interruption request write MSB */
#define PAULA_INTREQL   0x9D  /**< Interruption request write LSB */

#define PAULA_INTENAR   0x1C  /**< Interruption enable read */
#define PAULA_INTENARH  0x1C  /**< Interruption enable read MSB */
#define PAULA_INTENARL  0x1D  /**< Interruption enable read LSB */

#define PAULA_INTENA    0x9A  /**< Interruption enable write */
#define PAULA_INTENAH   0x9A  /**< Interruption enable write MSB */
#define PAULA_INTENAL   0x9B  /**< Interruption enable write LSB */

/**@}*/


/** @name  Amiga DMA registers.
 *  @{
 *
 *   Amiga DMA control register bits :
 *   - bit 0  Audio DMA channel A
 *   - bit 1  Audio DMA channel B
 *   - bit 2  Audio DMA channel C
 *   - bit 3  Audio DMA channel D
 *   - bit 9  General DMA
 */

#define PAULA_DMACONR   0x02  /**< DMA control read */
#define PAULA_DMACONRH  0x02  /**< DMA control read MSB */
#define PAULA_DMACONRL  0x03  /**< DMA control read LSB */

#define PAULA_DMACON    0x96  /**< DMA control write */
#define PAULA_DMACONH   0x96  /**< DMA control write MSB */
#define PAULA_DMACONL   0x97  /**< DMA control write LSB */

/**@}*/


/** @name The Audio & Disk Control Registers.
 *  @{
 *
 * Bits:
 * - 07   USE3PN    Use audio channel 3 to modulate nothing.
 * - 06   USE2P3    Use audio channel 2 to modulate period of channel 3.
 * - 05   USE1P2    Use audio channel 1 to modulate period of channel 2.
 * - 04   USE0P1    Use audio channel 0 to modulate period of channel 1.
 * - 03   USE3VN    Use audio channel 3 to modulate nothing.
 * - 02   USE2V3    Use audio channel 2 to modulate volume of channel 3.
 * - 01   USE1V2    Use audio channel 1 to modulate volume of channel 2.
 * - 00   USE0V1    Use audio channel 0 to modulate volume of channel 1.
 *
 * @note   If both period/volume are modulated on the same channel,
 * the period and volume will be alternated. First word xxxxxxxx V6-V0,
 * Second word P15-P0 (etc).
 */

#define PAULA_ADKCON    0x9E  /**< Audio, disk, control write. */
#define PAULA_ADKCONR   0x10  /**< Audio, disk, control read.  */
#define PAULA_ADKCONRH  0x10
#define PAULA_ADKCONRL  0x11

/**@}*/

/** @name  Amiga Paula registers.
 *  @{
 */
#define PAULA_VOICE(I) ((0xA+(I))<<4) /**< Paula channel I register base. */
#define PAULA_VOICEA   0xA0           /**< Paula channel A register base. */
#define PAULA_VOICEB   0xB0           /**< Paula channel B register base. */
#define PAULA_VOICEC   0xC0           /**< Paula channel C register base. */
#define PAULA_VOICED   0xD0           /**< Paula channel D register base. */
/**@}*/


/** @name  Amiga Paula frequencies (PAL).
 *  @{
 */
#define PAULA_PER 2.79365E-7      /**< Paula period (1 cycle duration) */
#define PAULA_FRQ 3579610.53837   /**< Paula frequency (1/PAULA_PER) */
  /**@}*/


/** Counter fixed point precision 13+19(512kb)=>32 bit */
#define PAULA_CT_FIX		13

/** @name  Internal Paula emulation data.
 *  @{
 */

/** Paula voice information data structure. */
typedef struct
{
  u32 adr;   /**< current sample counter (<<PAULA_CT_FIX) */
  u32 start; /**< loop address */
  u32 end;   /**< end address (<<PAULA_CT_FIX) */

/*  int v[16]; //$$$ */

} paulav_t;

extern u8 paula[];        /**< Paula regiter data storage */
extern paulav_t paulav[]; /**< Paula voices(channel) table (4 voices) */
extern int paula_dmacon;  /**< Shadow DMACON. */
extern int paula_intena;  /**< Shadow INTENA. */
extern int paula_intreq;  /**< Shadow INTREQ. */
extern int paula_adkcon;  /**< Shadow ADKCON. */

/**@}*/


/** @name  Initialization functions.
 *  @{
 */

/** Set/Get sampling rate.
 *
 *    The PL_sampling_rate() function set Paula emulator sampling rate.
 *    f is given in hz. If f is 0 the current replay is return.
 *
 *  @param  f  sampling rate in hz (0 to get current value).
 *
 *  @return new sampling rate
 */
unsigned int PL_sampling_rate(unsigned int f);

/** Paula hardware reset.
 *
 *    The PL_reset() reset function perform a Paula reset. It performs
 *    following operations :
 *    - all registers zeroed
 *    - all internal voices set to dummy 2 samples len address.
 *    - general DMA enabled
 *    - all audio DMA disabled
 *    - interrupt master enabled
 *    - all audio interrupt disbled
 *
 *    @return error-code (always success)
 *    @return  0  Success
 */
int PL_reset(void);

/** Paula first one first initialization.
 *
 *    The PL_init() must be call before all other PL functions. It performs
 *    following operations:
 *    - Init output level (volume) table.
 *    - Hardware reset
 *    - Set replay frequency to default (44100 Hz) if not already set 
 *
 *    @return error-code (always success)
 *    @return  0  Success
 *
 *  @see PL_reset()
 */
int PL_init(void);

/**@}*/


/** @name  Emulation functions
 *  @{
 */

/** Execute Paula emulation.
 *
 *    The PL_mix() function processes sample mixing with current internal
 *    parameters for n samples. Mixed samples are stored in a large enough
 *    (at least n) 32 bit pcm buffer pointed by b. mem68 is a pointer to
 *    the 68K memory buffer. The Paula emulator assume that this buffer is
 *    at least the size of the Amiga "chip" RAM. This implies at leat 512Kb
 *    and PCM data must be in the first 512Kb.
 *
 *    @param  b         Pointer to destination 32-bit data buffer
 *    @param  mem68     Pointer to 68K memory buffer start address
 *    @param  n         Number of sample to mix in b buffer
 *
 */
void PL_mix(u32 *b, u8 *mem68, int n);

/**@}*/


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _PAULA_EMUL_H_ */
