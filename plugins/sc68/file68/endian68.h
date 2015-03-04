/**
 * @ingroup   file68_devel
 * @file      file68/endian68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      2003/08/12
 * @brief     System byte order.
 *
 * $Id: endian68.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ENDIAN68_H_
#define _ENDIAN68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Get integer byte order.
 *
 *    The SC68byte_order() function returns an integer which gives the position
 *    of each byte in the memory.
 *
 *   Examples:
 *   - Intel little endian will return 0x03020100.
 *   - Motorola big endian will return 0x00010203.
 *
 *  @return byte order.
 */
int SC68byte_order(void);

/** Check if byte order is little endian.
 *
 *     The SC68little_endian() function checks if the byte order is little
 *     endian.
 *
 *  @return little endian test.
 *  @retval  1  byte order is little endian. 
 *  @retval  0  byte order is not little endian.
 *
 * @warning The function only test if the less signifiant byte is stored at
 *          offset 0.
 */
int SC68little_endian(void);

/** Check if byte order is big endian.
 *
 *     The SC68big_endian() function checks if the byte order is big
 *     endian. By the way it returns !SC68little_endian().
 *
 *  @return big endian test.
 *  @retval  1  byte order is big endian. 
 *  @retval  0  byte order is not big endian.
 *
 * @see SC68little_endian();
 */
int SC68big_endian(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ENDIAN68_H_ */
