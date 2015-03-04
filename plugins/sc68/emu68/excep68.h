/**
 * @ingroup   emu68_devel
 * @file      excep68.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/13/03
 * @brief     68k exception vector definitions.
 * @version   $Id: excep68.h,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 *
 *   68K interruptions are defined by a vector and a level. The interrupt
 *   vector is a long word stored in memory at vector address. It is loaded
 *   when position loaded when this interruption occurs. The level is the
 *   value transfered to the IPL field of SR, so that no interruption from
 *   lower level could occur.
 *
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _EXCEP68_H_
#define _EXCEP68_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RESET_VECTOR     0x00 /**< RESET vector address */
#define RESET_LVL        7    /**< RESET interruption level */

#define BUSERROR_VECTOR  0x08 /**< BUSERROR vector address */
#define BUSERROR_LVL     7    /**< BUSERROR interruption level */

#define ADRERROR_VECTOR  0x0C /**< ADRERROR vector address */
#define ADRERROR_LVL     7    /**< ADRERROR interruption level */

#define ILLEGAL_VECTOR   0x10 /**< ILLEGAL vector address */
#define ILLEGAL_LVL      7    /**< ILLEGAL interruption level */

#define DIVIDE_VECTOR    0x14 /**< DIVIDE vector address */
#define DIVIDE_LVL       7    /**< DIVIDE interruption level */

#define CHK_VECTOR       0x18 /**< CHK vector address */
#define CHK_LVL          7    /**< CHK interruption level */

#define TRAPV_VECTOR     0x1C /**< TRAPV vector address */
#define TRAPV_LVL        7    /**< TRAPV interruption level */

#define LINEA_VECTOR     0x28 /**< LINEA vector address */
#define LINEA_LVL        7    /**< LINEA interruption level */

#define LINEF_VECTOR     0x28 /**< LINEF vector address */
#define LINEF_LVL        7    /**< LINEF interruption level */

#define TRAP_VECTOR(N)   (0x80+(4*(N))) /**< TRAP #N vector address */
#define TRAP_LVL         7              /**< TRAP #N interruption level */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _EXCEP68_H_ */
