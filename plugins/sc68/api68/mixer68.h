/**
 * @ingroup   api68_devel
 * @file      api68/mixer68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1999/05/17
 * @brief     audio mixer
 *
 * $Id: mixer68.h,v 2.1 2003/08/26 23:14:02 benjihan Exp $
 */


#ifndef _MIXER68_H_
#define _MIXER68_H_

#include "emu68/type68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup  api68_mixer  audio mixer
 *
 *  sc68 audio mixer provides functions for computing audio data PCM.
 *  Almost all functions work on 16 bit stereo PCM. Some functions
 *  require signed input to work properly. All functions allow to change
 *  PCM sign. Functions that require signed input perform a double sign change
 *  operation which allow to have any kind of input PCM.
 *
 *  Internaly PCM are read as 32bit integer. This implies that buffer must be
 *  properly aligned (depending on the architecture). Left channel is stored
 *  the less signifiant 16 bit and right the most signifiant 16 bit. Ensure
 *  the audio device use the same byte order.
 *
 *  @ingroup   api68_devel
 *  @{
 */

/** @name  Constants for PCM sign transformation.
 *  @{
 */
#define MIXER68_SAME_SIGN                 0x00000000 /**< No channel.    */
#define MIXER68_CHANGE_LEFT_CHANNEL_SIGN  0x00008000 /**< Left channel.  */
#define MIXER68_CHANGE_RIGHT_CHANNEL_SIGN 0x80000000 /**< Right channel. */
#define MIXER68_CHANGE_SIGN               0x80008000 /**< Both channels. */
/**@}*/

/** Copy 16-bit-stereo PCM with optionnal sign change.
 *
 * @param  dest  Destination PCM buffer
 * @param  src   Source PCM buffer (can be same as dest).
 * @param  nb    Number of PCM
 * @param  sign  Sign transformation.
 */
void SC68mixer_stereo_16_LR(u32 *dest, u32 *src, int nb,
			    const u32 sign);

/** Copy 16-bit-stereo PCM with channel swapping and optionnal sign change.
 *
 * @note  Sign change occurs after channel swapping.
 *
 * @param  dest  Destination PCM buffer
 * @param  src   Source PCM buffer (can be same as dest).
 * @param  nb    Number of PCM
 * @param  sign  Sign transformation.
 */
void SC68mixer_stereo_16_RL(u32 *dest, u32 *src, int nb,
			    const u32 sign);

/** Copy 16-bit-stereo PCM into normalized float-stereo (-norm..+norm).
 *
 * @note     Sign change occurs before float transformation.
 * @warning  PCM are assumed to be signed after sign transform.
 *
 * @param  dest  Destination PCM buffer
 * @param  src   Source PCM buffer (can be same as dest).
 * @param  nb    Number of PCM
 * @param  sign  Sign transformation.
 * @param  norm  float absolute range (normalization).
 *
 * @note   Minus norm causes a sign/phase inversion.
 */
void SC68mixer_stereo_FL_LR(float *dest, u32 *src, int nb,
			    const u32 sign, const float norm);

/** Copy left channel of 16-bit stereo PCM into L/R channels
 *  with optionnal sign change.
 *
 * @param  dest  Destination PCM buffer
 * @param  src   Source PCM buffer (can be same as dest).
 * @param  nb    Number of PCM
 * @param  sign  Sign transformation.
 */
void SC68mixer_dup_L_to_R(u32 *dest, u32 *src, int nb, const u32 sign);

/** Copy right channel of 16-bit stereo PCM into L/R channels
 *  with optionnal sign change.
 *
 * @param  dest  Destination PCM buffer
 * @param  src   Source PCM buffer (can be same as dest).
 * @param  nb    Number of PCM
 * @param  sign  Sign transformation.
 */
void SC68mixer_dup_R_to_L(u32 *dest, u32 *src, int nb, const u32 sign);

/** Copy 16-bit-stereo PCM with L/R blending and optionnal sign change.
 *
 *  This function performs following transformations in this order :
 *  - Read 32 bit value from src.
 *  - Apply sign_r transformation; it is a bitwise EOR.
 *  - Apply blending (here PCM are asuumed to be signed).
 *  - Apply sign_w transformation; it is a bitwise EOR too.
 *  - Store 32 bit value.
 *
 * @warning  As the blending occurs on signed PCM the sign_r and
 *           the sign_w must be setted properly.
 *
 * @param  dest    Destination PCM buffer
 * @param  src     Source PCM buffer (can be same as dest).
 * @param  nb      Number of PCM
 * @param  factor  Blending factor from [0..65536].
 *                 - 0, blend nothing.
 *                 - 65536, Swap L/R.
 *                 - other value, linear blending.
 * @param  sign_r  sign transform for input PCM.
 * @param  sign_w  sign transform for output PCM.
 */
void SC68mixer_blend_LR(u32 *dest, u32 *src, int nb,
			int factor,
			const u32 sign_r, const u32 sign_w);

/** Copy 16-bit-stereo PCM with L/R amplitude factor and
 *  optionnal sign change.
 *
 *  This function performs following transformations in this order :
 *  - Read 32 bit value from src.
 *  - Apply sign_r transformation; it is a bitwise EOR.
 *  - Apply signed modulation to both channels.
 *  - Apply sign_w transformation; it is a bitwise EOR too.
 *  - Store 32 bit value.
 *
 * @warning  As the modulation occurs on signed PCM the sign_r and
 *           the sign_w must be setted properly.
 *
 * @note     Amplitude factors (ml and mr) can be minus to invert phase.
 *
 * @param  dest    Destination PCM buffer
 * @param  src     Source PCM buffer (can be same as dest).
 * @param  nb      Number of PCM
 * @param  ml      Left channel factor from [-65536..65536].
 * @param  mr      Like ml but for right channel.
 * @param  sign_r  sign transform for input PCM.
 * @param  sign_w  sign transform for output PCM.
 */
void SC68mixer_mult_LR(u32 * dest, u32 * src, int nb,
		       const int ml, const int mr,
		       const u32 sign_r, const u32 sign_w);

/** Fill 16-bit-stereo buffer with sign value (RRRRLLLL).
 *
 * @param  dest    Destination PCM buffer
 * @param  nb      Number of PCM
 * @param  sign    PCM value written (right channel is MSW).
 */
void SC68mixer_fill(u32 *dest, int nb,
		    const u32 sign);

/** 
 *  @}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MIXER68_H_ */
