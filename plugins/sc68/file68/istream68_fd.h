/**
 * @ingroup file68_devel
 * @file    file68/istream68_fd.h
 * @author  benjamin gerard
 * @date    2003/08/08
 * @brief   file descriptor stream operation.
 *
 * $Id: istream68_fd.h,v 1.2 2003/09/30 06:29:57 benjihan Exp $
 *
 *    isteam68_fd implements istream_t for file descriptor.
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ISTREAM68_FD_H_
#define _ISTREAM68_FD_H_

#include "file68/istream68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a stream for "UNIX" file descriptor.
 *
 *  If fd parameters is not -1, it is used to as file descriptor for
 *  the stream and fname is used for naming the stream. Else the file
 *  is open as a regular file with fname as path.
 *
 *  @param  fname  path of file or 0.
 *  @param  fd     file decriptor or -1.
 *  @param  mode   bit-0: read access, bit-1: write access.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note     filename is internally copied.
 *  @note     Even if fd is given the istream_open() must be call.
 *  @warning  When opening a stream with an already opened descriptor the
 *            mode should match the real open mode but since no tests are
 *            performed before calling r/w access, it should not failed in
 *            case of wrong access on given mode.
 */
istream_t * istream_fd_create(const char * fname, int fd, int mode);

#ifdef __cplusplus
}
#endif

#endif /* #define _ISTREAM68_FILE_H_ */
