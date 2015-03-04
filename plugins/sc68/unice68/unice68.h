/**
 * @ingroup   unice68_devel
 * @file      unice68/unice68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      2003/08/06
 * @brief     ICE depacker.
 *
 * $Id: unice68.h,v 2.2 2003/09/06 16:14:31 benjihan Exp $
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _UNICE68_H_
#define _UNICE68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup  unice68_devel  ICE 2.4 depacker library.
 *  @brief     ICE 2.4 depacker.
 *  @author    Benjamin Gerard <ben@sashipa.com>
 *  @{
 */

/** Get ICE depacker version.
 *
 *  @return version number (MAJOR*100+MINOR)
 */
int unice68_ice_version(void);

/** Test ICE and get compressed and uncompresed size.
 *
 *    The unice68_get_depacked_size() function returns the uncompressed
 *    size of a ICE compressed buffer. If p_size is not 0 it is fill with
 *    the size of the compressed data found in header (useful for stream
 *    operation). 
 *    If the value pointed by p_csize is not 0 the function assumes that it is
 *    an expected compressed size and compares it to header one. If it differs
 *    the function returns the bitwise NOT value of uncompressed data. This
 *    should be a minus value but not -1.
 *
 *  @param  buffer   buffer with at least 12 bytes of data (ice header).
 *  @param  p_csize  Optionnal pointer to store compressed size.
 *                   May be use to verify an expected compressed size.
 *                   See function documentation for more details.
 *
 *  @return uncompressed size
 *  @retval >0   Uncompressed size
 *  @retval -1   Error, not a valid ICE packed buffer
 *  @retval <0   Bitwise NOT of uncompressed size but verify failed.
 *
 */
int unice68_get_depacked_size(const void * buffer, int * p_csize);

/** Depack an ICE buffer to another.
 *
 *   The unice68_depacker() depack src ICE compressed buffer to dest.
 *   The dest buffer is assumed to be already allocated with enought room.
 *
 * @param  dest  destination buffer (uncompressed data).
 * @param  src   source buffer (compressed data).
 *
 * @return error code
 * @retval 0     succcess
 * @retval 1     failure
 *
 * @warning The original ICE depacker may allow to use the same buffer for
 *          compressed and uncompressed data. Anyway this has not been tested
 *          and you are encouraged to add guard bands.
 */
int unice68_depacker(void * dest, const void * src);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _UNICE68_H_ */

