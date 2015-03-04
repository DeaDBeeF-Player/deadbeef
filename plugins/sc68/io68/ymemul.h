/**
 * @ingroup   io68_devel
 * @file      io68/ymemul.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1998/06/24
 * @brief     YM-2149 emulator
 *
 * $Id: ymemul.h,v 2.1 2003/08/25 02:43:09 benjihan Exp $
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _YM_EMUL_H_
#define _YM_EMUL_H_

#include "emu68/type68.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YM_BASEPERL  0  /**< YM-2149 LSB period base (canal A) */
#define YM_BASEPERH  1  /**< YM-2149 MSB period base (canal A) */
#define YM_BASEVOL   8  /**< YM-2149 volume base register (canal A) */

#define YM_PERL(N) (YM_BASEPERL+(N)*2) /**< Canal #N LSB period */
#define YM_PERH(N) (YM_BASEPERH+(N)*2) /**< Canal #N MSB periodr */
#define YM_VOL(N)  (YM_BASEVOL+(N))    /**< Canal #N volume */

#define YM_NOISE     6  /**< Noise period */
#define YM_MIXER     7  /**< Mixer control */
#define YM_ENVL      11 /**< Volume envelop LSB period */
#define YM_ENVH      12 /**< Volume envelop MSB period */
#define YM_ENVTYPE   13 /**< Volume envelop wave form */
#define YM_ENVSHAPE  13 /**< Volume envelop wave form */

/** YM-2149 internal data structure */
typedef struct
{
  /* Internal YM register */
  u8 ctrl;        /**< Current control (working) register */
  u8 data[16];    /**< YM data register. */

  /* Envelop specific */
  unsigned int env_ct;      /**< Envelop period counter */
  unsigned int env_cont;    /**< Continue mask [0 or 0x1f] */
  unsigned int env_alt;     /**< Alternate mask [0 or 0x1f] */  
  unsigned int env_bit;     /**< Envelop level : 5 LSB are envelop level */
  unsigned int env_bitstp;  /**< Envelop level step : [0 or 1] */

  /* Noise specific */
  unsigned int noise_gen;  /**< Noise generator 17-bit shift register */
  unsigned int noise_ct;   /**< Noise generator period counter */

  /* Sound specific */
    signed int voice_ctA;  /**< Canal A sound period counter */
    signed int voice_ctB;  /**< Canal B sound period counter */
    signed int voice_ctC;  /**< Canal C sound period counter */
  unsigned int sq;         /**< Square level 0xABC */
} ym2149_t;

/** YM-2149 emulator internal data. */
extern ym2149_t ym;

/** @name  Initialization functions
 *  @{
 */

/** Set/get output buffer replay frequency
 *
 *    The YM_sampling_rate() function set YM-2149 emulator sampling rate.
 *    If f is 0 the current sampling rate is returned without change.
 *
 *  @param  f  Sampling rate (in Hz) in the range
 *             [SAMPLING_RATE_MIN..SAMPLING_RATE_MAX].
 *             0 is used to read current value.
 *
 *  @return sampling rate used clampled in the range
 *          [SAMPLING_RATE_MIN..SAMPLING_RATE_MAX].
 */
unsigned int YM_sampling_rate(unsigned int f);

/** Yamaha-2149 hardware reset.
 *
 *    The YM_reset() reset function perform a YM-2149 reset. It performs
 *    following operations :
 *    - all register zeroed
 *    - mixer is set to 077 (no sound and no noise)
 *    - envelop shape is set to 0xA (/\)
 *    - control register is set to 0
 *    - internal periods counter are zeroed
 *
 *    @return error-code (always success)
 *    @retval 0  Success
 */
int YM_reset(void);

/** Yamaha-2149 first one first initialization.
 *
 *    The YM_init() must be call before all other YM functions. It performs
 *    following operations :
 *    - Init envelop data.
 *    - Init output level (volume) table.
 *    - Hardware reset
 *    - Set replay frequency to default (44100 Hz)
 *
 *    @return error-code (always success)
 *    @retval 0  Success
 *
 *  @see YM_reset()
 */
int YM_init(void);

/**@}*/


/** @name  Emulation functions
 *  @{
 */

/** Execute Yamaha-2149 emulation.
*
*    The YM_mix() function execute Yamaha-2149 emulation for a given number
*    of cycle. Clock frequency is based on Atari-ST 68K at 8MHz. This
*    function fill internal mix-buffer, that could be retrieve by
*    YM_get_buffer() function and returns the number of sample mixed. The
*    register update lists are flushed to perform a cycle precision
*    emulation.
*
*  @warning  The emulator used an internal round error counter, so the
*            return value could be different from one call to another even
*            the number of cycle is the same.
*
*  @return  Number of sample in output mix-buffer
*
*  @see YM_get_buffer()
*/
unsigned int YM_mix(cycle68_t cycle2mix);

/** Yamaha get buffer.
 *
 *    The YM_get_buffer() function returns a pointer to the current PCM
 *    ouput buffer. Currently it returns a static buffer so the address is
 *    always the same but this could change in the future and it is safer to
 *    call it after each YM_mix() function call.
 *
 *    Output buffer samples are unsigned 16 bit machine endian values stored
 *    in a 32 bit data. Copying LSW value to the MSW transform it in a
 *    stereo sample.
 *
 *  @return  Pointer to a 32-bit value output buffer.
 *  @retval 0  No output buffer (currently impossible).
 *
 *  @see YM_mix()
 */
u32 *YM_get_buffer(void);

/** Change YM cycle counter base.
 *
 *    The YM_subcycle() function allow to corrige the internal cycle counter
 *    to prevent overflow. Because the number of cycle could grow very
 *    quickly, it is neccessary to get it down from time to time.
 *
 *  @param subcycle  Number of cycle to substract to current cycle counter.
 *
 */
void YM_subcycle(cycle68_t subcycle);

/**@}*/


/** @name  YM-2149 register access functions
 *  @{
 */

/** Write in YM register.
 *
 *   The YM_writereg() function must be call to write an YM-2149 register.
 *   The YM-2149 emulator do not really write register, but store changes in
 *   separate list depending of the register nature and dependencies. There
 *   are 3 list of update (sound, noise and envelop). This method allow to
 *   perform a very efficient cycle precise emulation. For this reason, the
 *   YM-2149 must not be read directly but toward the YM_readreg() function.
 *
 *  @param  reg    YM-2149 register to write
 *  @param  v      Value to write
 *  @param  cycle  Cycle number this access occurs
 *
 *  @see YM_readreg();
 */
void YM_writereg(u8 reg, u8 v, cycle68_t cycle);

/** Read a YM-2119 register.
 *
 *   The YM_readreg() function must be call to read an YM-2149 register. For
 *   the reasons explained in YM_writereg(), register must not be read
 *   directly.
 *
 *  @param  reg    YM-2149 register to read
 *  @param  cycle  Cycle number this access occurs
 *
 *  @return  Register value at given cycle
 *
 *  @see YM_writereg();
 */
u8 YM_readreg(u8 reg, cycle68_t cycle);

/** Get voices status.
 *
 *   The YM_get_activevoices() function return activation status
 *   for each voices (canals) of the YM.
 *
 *  @return voices activation status.
 *          -bit#0: canal A (0:off)
 *          -bit#1: canal B (0:off)
 *          -bit#2: canal C (0:off)
 *
 *  @see void YM_set_activeVoices(int v);
 */
int YM_get_activevoices(void);

/** Set voices status.
 *
 *   The YM_set_activevoices() function activates or desactivates
 *   each voices (canals) of the YM.
 *
 *  @param  v   new voices activation status.
 *              -bit#0: canal A (0:off)
 *              -bit#1: canal B (0:off)
 *              -bit#2: canal C (0:off)
 *
 *  @see void YM_get_activeVoices(int v);
 */
void YM_set_activeVoices(int v);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _YM_EMUL_H_ */

