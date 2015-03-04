/**
 * @ingroup file68_devel
 * @file    file68/error68.h
 * @author  benjamin gerard
 * @date    2003/08/08
 * @brief   error message stack.
 *
 * $Id: error68.h,v 2.1 2003/08/30 01:27:08 benjihan Exp $
 *
 *    SC68 error module consists on a fixed size stack of fixed
 *    length strings. It provides functions for both error pushing and
 *    poping.
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ERROR68_H_
#define _ERROR68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Push a formatted error message.
 *
 *    The SC68error_add() function format error string into stack buffer. If
 *    stack is full, the oldest error message is removed.
 *
 *  @param  format  printf() like format string
 *
 *  @return error-code
 *  @retval -1
 */
int SC68error_add(const char *format, ... );

/** Get last error message.
 *
 *    The SC68error_get() function retrieves last error message and removes
 *    it from error message stack.
 *
 *  @return  Static string to last error message
 *  @retval  0  No stacked error message
 */
const char * SC68error_get(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ERROR68_H_ */
