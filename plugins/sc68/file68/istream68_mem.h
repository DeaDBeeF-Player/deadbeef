/**
 * @ingroup file68_devel
 * @file    file68/istream68_mem.h
 * @author  Benjamin Gerard
 * @date    2003/08/08
 * @brief   Memory buffer stream operation.
 *
 * $Id: istream68_mem.h,v 2.3 2003/09/30 06:29:57 benjihan Exp $
 *
 *    isteam68_mem implements istream_t for memory buffer.
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ISTREAM68_MEM_H_
#define _ISTREAM68_MEM_H_

#include "file68/istream68.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Creates a stream for memory buffer.
 *
 *  @param  addr   Buffer base address.
 *  @param  len    Buffer length.
 *  @param  mode   Allowed open mode.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note   filename is build with memory range.
 */
istream_t * istream_mem_create(const void * addr, int len, int mode);

#ifdef __cplusplus
}
#endif


#endif /* #define _ISTREAM68_MEM_H_ */
