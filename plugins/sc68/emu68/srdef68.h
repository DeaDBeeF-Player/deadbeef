/**
 * @ingroup   emu68_devel
 * @file      srdef68.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/13/03
 * @brief     Status Register (SR) definitions.
 * @version   $Id: srdef68.h,v 2.1 2003/09/30 06:29:57 benjihan Exp $
 *
 *   68K status register include condition code register (CCR) located in
 *   the LSB of SR, and privileged processor status in MSB. EMU68 does not
 *   currently handle supervisor and trace mode. Internal processor level is
 *   partially managed. Each SR bit is defined by its bit number (SR_x_BIT)
 *   and the corresponding value (SR_x) where x is one of C,V,Z,N,X,S or T. 
 *   SR_IPL_BIT is used to locate the less significant bit position of the 3
 *   IPL bits. Macros are available to help SR bit manipulations.
 *
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _SRDEF68_H_
#define _SRDEF68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @name  SR bit definitions.
 *  @{
 */

#define SR_C_BIT 0    /**< Carry bit number */
#define SR_V_BIT 1    /**< Overflow bit number */
#define SR_Z_BIT 2    /**< Zero bit number */
#define SR_N_BIT 3    /**< Negative bit number */
#define SR_X_BIT 4    /**< eXtended carry bit number */
#define SR_IPL_BIT 8  /**< Internal Processor Level bit number */
#define SR_S_BIT 13   /**< Superuser bit number */
#define SR_T_BIT 15   /**< Trace bit number */

#define SR_C (1<<SR_C_BIT)  /**< Carry value */
#define SR_V (1<<SR_V_BIT)  /**< Overflow value */
#define SR_Z (1<<SR_Z_BIT)  /**< Zero value */
#define SR_N (1<<SR_N_BIT)  /**< Negative value */
#define SR_X (1<<SR_X_BIT)  /**< eXtended carry value */
#define SR_S (1<<SR_S_BIT)  /**< Superuser value */
#define SR_T (1<<SR_T_BIT)  /**< Trace value */

/**@}*/


/** @name  Condition tests.
 *  @{
 */

#define IS_CS(sr) ((sr)&SR_C)
#define IS_CC(sr) (!IS_CS(sr))

#define IS_EQ(sr) ((sr)&SR_Z)
#define IS_NE(sr) (!IS_EQ(sr))

#define IS_VS(sr) ((sr)&SR_V)
#define IS_VC(sr) (!IS_VS(sr))

#define IS_MI(sr) ((sr)&SR_N)
#define IS_PL(sr) (!IS_MI(sr))

#define IS_LS(sr) ((sr)&(SR_C|SR_Z))
#define IS_LT(sr) (((sr)^((sr)>>2))&SR_V)
#define IS_LE(sr) (IS_LT(sr)|IS_EQ(sr))

#define IS_GE(sr) (!IS_LT(sr))
#define IS_GT(sr) (!IS_LE(sr))
#define IS_HI(sr) (!IS_LS(sr))

#define IS_T(sr)  1
#define IS_F(sr)  0

/**@}*/


/** @name  SR manipulations.
 *  @{
 */

/** Set SR Z and N bit for SRC value */
#define MOVESR(SR,SRC) (SR) = \
	( ((SR)&~(SR_Z|SR_N|SR_V|SR_C)) | \
	(((SRC)==0)<<SR_Z_BIT ) | (( (SRC)<0)<<SR_N_BIT ) )

/** Get CCR value */
#define GET_CCR(SR) ((u8)(SR))

/** Set CCR in SR value */
#define SET_CCR(SR,CCR) (SR) = (((SR)&0xFF00) | (CCR))

/** Get IPL in SR value */
#define GET_IPL(n) (((n)>>SR_IPL)&7)

/** Change IPL for SR value */
#define SET_IPL(sr,n) (sr) = (((sr)&(7<<SR_IPL_BIT)) | ((n)<<SR_IPL_BIT)))

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SRDEF68_H_ */
