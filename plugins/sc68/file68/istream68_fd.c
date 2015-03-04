/*
 *                         sc68 - FILE stream
 *         Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <config68.h>

/* define this if you don't want file descriptor support. */
#ifndef ISTREAM_NO_FD

#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
# include <stdio.h>
# include <io.h>
#else
# include <stdio.h>
# include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>

#include "file68/istream68_fd.h"
#include "file68/istream68_def.h"
#include "file68/alloc68.h"

/** istream file structure. */
typedef struct {
  istream_t istream; /**< istream function.            */
  int fd;            /**< File descriptor (-1:closed). */
  int org_fd;        /**< Original file descriptor.    */

  /** Open modes. */
  int mode;

  /* MUST BE at the end of the structure because supplemental bytes will
   * be allocated to store filename.
   */
  char name[1];       /**< filename. */

} istream_fd_t;

static const char * ifdname(istream_t * istream)
{
  istream_fd_t * isf = (istream_fd_t *)istream;

  return (!isf || !isf->name[0])
    ? 0
    : isf->name;
}

static int ifdopen(istream_t * istream)
{
  int imode;
  istream_fd_t * isf = (istream_fd_t *)istream;

  if (!isf || !isf->name || isf->fd != -1) {
    return -1;
  }

  if (isf->org_fd != -1) {
    return isf->fd = isf->org_fd;
  }

  switch (isf->mode) {
  case ISTREAM_OPEN_READ:
    imode = O_RDONLY;
    break;
  case ISTREAM_OPEN_WRITE:
    imode = O_WRONLY | O_CREAT | O_TRUNC;
    break;
  case ISTREAM_OPEN_READ|ISTREAM_OPEN_WRITE:
    imode = O_RDWR | O_CREAT;
    break;
  default:
    return -1;
  }
#ifdef _O_BINARY
  imode |= _O_BINARY;
#endif

  isf->fd = open(isf->name, imode);
  return -(isf->fd == -1);
}

static int ifdclose(istream_t * istream)
{
  istream_fd_t * isf = (istream_fd_t *)istream;
  int fd;

  if (!isf || isf->fd == -1) {
    return -1;
  }
  fd = isf->fd;
  isf->fd = -1;
  /* $$$ Close even if org_fd was given. Is it a good thing ? */
  return close(fd);
}

static int ifdread(istream_t * istream, void * data, int n)
{
  istream_fd_t * isf = (istream_fd_t *)istream;

  return (!isf || isf->fd == -1)
    ? -1
    : read(isf->fd, data, n);
}

static int ifdwrite(istream_t * istream, const void * data, int n)
{
  istream_fd_t * isf = (istream_fd_t *)istream;

  return (!isf || isf->fd == -1)
    ? -1
    : write(isf->fd, data, n);
}


/* We could have store the length value at opening, but this way it handles
 * dynamic changes of file size.
 */
static int ifdlength(istream_t * istream)
{
  istream_fd_t * isf = (istream_fd_t *)istream;
  off_t pos,len = -1;

  if (!isf || isf->fd == -1) {
    return -1;
  }

  /* save current position. */
  pos = lseek(isf->fd,0,SEEK_CUR);
  if (pos != (off_t)-1) {
    /* seek to end of file to get file position (AKA length) */
    len = lseek(isf->fd, 0, SEEK_END);
    /* restore saved position. */
    lseek(isf->fd, pos, SEEK_SET);
  }
  return len;
}

static int ifdtell(istream_t * istream)
{
  istream_fd_t * isf = (istream_fd_t *)istream;

  return (!isf || isf->fd == -1)
    ? -1 
    : lseek(isf->fd,0,SEEK_CUR);
}

static int ifdseek(istream_t * istream, int offset)
{
  istream_fd_t * isf = (istream_fd_t *)istream;

  return (!isf || isf->fd == -1)
    ? -1
    : lseek(isf->fd, offset, SEEK_CUR);
}

static void ifddestroy(istream_t * istream)
{
  SC68free(istream);
}

static const istream_t istream_fd = {
  ifdname, ifdopen, ifdclose, ifdread, ifdwrite,
  ifdlength, ifdtell, ifdseek, ifdseek, ifddestroy
};

istream_t * istream_fd_create(const char * fname, int fd, int mode)
{
  istream_fd_t *isf;
  int len;

  if (fd == -1 && (!fname || !fname[0])) {
    fname = ":fd:";
  }

  if (!fname || !fname[0]) {
    return 0;
  }

  /* Don't need 0, because 1 byte already allocated in the
   * istream_fd_t::fname.
   */
  len = strlen(fname);
  isf = SC68alloc(sizeof(istream_fd_t) + len);
  if (!isf) {
    return 0;
  }

  /* Copy istream functions. */
  memcpy(&isf->istream, &istream_fd, sizeof(istream_fd));
  /* Clean init. */
  isf->fd = -1;
  isf->org_fd = fd;
  isf->mode = mode & (ISTREAM_OPEN_READ|ISTREAM_OPEN_WRITE);
  
  /* Copy filename. */
  /* $$$ May be later, we should add a check for relative path and add
   * CWD ... or make it URL compatible */
  strcpy(isf->name, fname);
  return &isf->istream;
}

#else /* #ifndef ISTREAM_NO_FILE */

/* istream fd must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

istream_t * istream_fd_create(const char * fname, int fd, int mode)
{
  return 0;
}

#endif /* #ifndef ISTREAM_NO_FILE */
