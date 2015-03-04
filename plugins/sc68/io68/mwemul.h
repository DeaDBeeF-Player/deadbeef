/**
 * @ingroup   io68_devel
 * @file      io68/mwemul.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/03/20
 * @brief     MicroWire - STE sound emulator
 *
 * $Id: mwemul.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MWEMUL_H_
#define _MWEMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/type68.h"

/** @name  Micro-Wire registers.
 *  @{
 */

#define MW_ACTI 0x01        /**< Microwire enabled */

#define MW_BASH 0x03        /**< Microwire sample start address, bit 16-23 */
#define MW_BASM (MW_BASH+2) /**< Microwire sample start address, bit 8-15  */
#define MW_BASL (MW_BASH+4) /**< Microwire sample start address, bit 0-7   */

#define MW_CTH 0x09         /**< Microwire sample counter, bit 16-23 */
#define MW_CTM (MW_CTH+2)   /**< Microwire sample counter, bit 8-15  */
#define MW_CTL (MW_CTH+4)   /**< Microwire sample counter, bit 0-7   */

#define MW_ENDH 0x0f        /**< Microwire sample end address, bit 16-23 */
#define MW_ENDM (MW_ENDH+2) /**< Microwire sample end address, bit 8-15  */
#define MW_ENDL (MW_ENDH+4) /**< Microwire sample end address, bit 0-7   */

#define MW_MODE 0x21        /**< Microwire playing mode */

#define MW_DATA 0x22        /**< Microwire data register */
#define MW_CTRL 0x24        /**< Microwire control register */

/**@}*/


/** @name  Micro-Wire internal data.
 *  @{
 */

extern u8 mw[0x40]; /**< Micro-Wire register array.                  */
extern u32 mw_ct;   /**< DMA current location (8 bit fixed point).   */
extern u32 mw_end;  /**< DMA end point location (8 bit fixed point). */

/**@}*/


/** @name  Initialization functions.
 *  @{
 */

/** Set/Get sampling rate.
 *
 *    The MW_sampling_rate() function set Micro-Wire emulator sampling
 *    rate. f is given in hz. If f is 0 the current replay is return.
 *
 *  @param  f  sampling rate in hz (0 to get current value).
 *
 *  @return new sampling rate
 */
unsigned int MW_sampling_rate(unsigned int f);

/** Micro-Wire hardware reset.
 *
 *    The MW_reset() reset function perform a Micro-Wire reset. It performs
 *    following operations :
 *    - all registers zeroed
 *    - all internal counter zeroed
 *    - LMC reset
 *      - mixer mode YM2149+Micro-Wire
 *      - master volume to -40db
 *      - left and right volumes to -20db
 *      - low-pass filter to 0
 *
 *    @return error-code (always success)
 *    @retval 0  Success
 */
int MW_reset(void);

/** Micro-Wire first one first initialization.
 *
 *    The MW_init() must be call before all other PL functions.
 *    It performs following operations.
 *    - Init output level (volume) table.
 *    - Hardware reset
 *    - Set replay frequency to default (44100 Hz)
 *
 *    @return error-code (always success)
 *    @retval 0  Success
 *
 *  @see MW_reset()
 */
int MW_init(void);

/**@}*/

/** @name  Emulation functions.
 *  @{
 */

/** Execute Micro-Wire emulation.
 *
 *    The MW_mix() function processes sample mixing with current internal
 *    parameters for n samples. Mixed samples are stored in a large enough
 *    (at least n) 32 bit buffer pointed by b. This buffer have to be
 *    previously filled with the YM-2149 samples. Typically it is the YM-2149
 *    emulator output buffer. This allows Micro-Wire emulator to honnor the
 *    LMC mixer mode.iven LMC mode. This porocess include the mono to stereo
 *    expansion. The mem68 starting pointer locates the 68K memory buffer
 *    where samples are stored to allow DMA fetch emulation.
 *
 *    @param  b      Pointer to YM-2149 source sample directly used for
 *                   Micro-Wire output mixing.
 *    @param  mem68  Pointer to 68K memory buffer start address
 *    @param  n      Number of sample to mix in b buffer
 *
 *    @see YM_mix()  @see YM_get_buffer()
 */
void MW_mix(u32 *b, const u8 * mem68, int n);

/**@}*/


/** @name  Micro-Wire LMC control functions.
 *  @{
 */

/** Set LMC mixer type.
 *
 *   The MW_set_LMC_mixer() function choose the mixer mode :
 *   - 0  -12 Db
 *   - 1  YM+STE
 *   - 2  STE only
 *   - 3  reserved ???
 *
 *   @param n  New mixer mode (see above)
 */
void MW_set_LMC_mixer(unsigned int n);

/** Set LMC master volume.
 *
 *  @param  n  New volume in range [0..40]=>[-80Db..0Db]
 *
 *  @see MW_set_LMC_left()
 *  @see MW_set_LMC_right()
 */
void MW_set_LMC_master(unsigned int n);

/** Set LMC left channel volume.
 *
 *    Set LMC left channel volume in decibel.
 *
 *    @param  n  New volume in range [0..20]=>[-40Db..0Db]
 *
 *  @see MW_set_LMC_master()
 *  @see MW_set_LMC_right()
 */
void MW_set_LMC_left(unsigned int n);

/** Set LMC right channel volume.
 *
 *    @param  n  New volume in range [0..20]=>[-40Db..0Db]
 *
 *  @see MW_set_LMC_master()
 *  @see MW_set_LMC_left()
 */
void MW_set_LMC_right(unsigned int n);

/** Set high pass filter.
 *
 *  @param  n  New high pass filter [0..12]=>[-12Db..0Db]
 *
 *  @see MW_set_LMC_low()
 *
 *  @warning  Filters are not supported by MicroWire emulator.
 */
void MW_set_LMC_high(unsigned int n);

/** Set low pass filter.
 *
 *  @param  n  New low pass filter [0..12]=>[-12Db..0Db]
 *
 *  @see MW_set_LMC_high()
 *
 *  @warning  Unsupported by MicroWire emulator.
 */
void MW_set_LMC_low(unsigned int n);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MWEMUL_H_*/
