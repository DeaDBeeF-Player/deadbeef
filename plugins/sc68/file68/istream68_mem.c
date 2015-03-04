/*
 *                         sc68 - Memory stream
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

/* define this if you don't want MEM support. */
#ifndef ISTREAM_NO_MEM

#include <stdio.h>
#include <string.h>

#include "file68/istream68_mem.h"
#include "file68/istream68_def.h"
#include "file68/alloc68.h"

/** istream file structure. */
typedef struct {
  istream_t istream; /**< istream function.   */
  char * buffer;     /**< memory buffer.      */
  int size;          /**< memory buffer size. */
  int pos;           /**< current position.   */

  /** Open modes. */
  int mode;          /**< Allowed open mode bit-0:read bit-1:write.   */
  int open;          /**< Currently open mode bit-0:read bit-1:write. */

  /* MUST BE at the end of the structure because supplemental bytes will
   * be allocated to store filename.
   */
  char name[64];     /**< filename (mem://start:end). */

} istream_mem_t;

static const char * ism_name(istream_t * istream)
{
  istream_mem_t * ism = (istream_mem_t *)istream;

  return (!ism || !ism->name[0])
    ? 0
    : ism->name;
}

static int ism_open(istream_t * istream)
{
  istream_mem_t * ism = (istream_mem_t *)istream;

  if (!ism || !ism->mode || ism->open) {
    return -1;
  }
  ism->open = ism->mode;
  ism->pos = 0;
  return 0;
}

static int ism_close(istream_t * istream)
{
  istream_mem_t * ism = (istream_mem_t *)istream;

  if (!ism || !ism->open) {
    return -1;
  }
  ism->open = 0;
  return 0;
}

static int ism_read_or_write(istream_t * istream, void * data, int n, int mode)
{
  istream_mem_t * ism = (istream_mem_t *)istream;
  int pos, endpos;

  if (!ism || !(ism->open & mode) || n < 0) {
    return -1;
  }
  if (!n) {
    return 0;
  }

  pos = ism->pos;
  endpos = pos + n;
  if (endpos > ism->size) {
    endpos = ism->size;
    n = endpos - pos;
  }
  if (n > 0) {
    void *src, *dst;
    if (mode == ISTREAM_OPEN_READ) {
      src = ism->buffer+pos;
      dst = data;
    } else {
      src = data;
      dst = ism->buffer+pos;
    }
    memcpy(dst, src, n);
  }
  ism->pos = endpos;
  return n;
}

static int ism_read(istream_t * istream, void * data, int n)
{
  return ism_read_or_write(istream, data, n, ISTREAM_OPEN_READ);
}

static int ism_write(istream_t * istream, const void * data, int n)
{
  return ism_read_or_write(istream, (void *)data, n, ISTREAM_OPEN_WRITE);
}

static int ism_length(istream_t * istream)
{
  istream_mem_t * ism = (istream_mem_t *)istream;

  return (ism && ism->open) ? ism->size : -1;
}

static int ism_tell(istream_t * istream)
{
  istream_mem_t * ism = (istream_mem_t *)istream;

  return (!ism || !ism->open)
    ? -1 
    : ism->pos;
}

static int ism_seek(istream_t * istream, int offset)
{
  istream_mem_t * ism = (istream_mem_t *)istream;
  int pos;

  if (!ism || !ism->open) {
    return -1;
  }
  pos = ism->pos + offset;
  if (pos < 0 || pos > ism->size) {
    return -1;
  }
  ism->pos = pos;
  return 0;
}

static void ism_destroy(istream_t * istream)
{
  SC68free(istream);
}

static const istream_t istream_mem = {
  ism_name,
  ism_open, ism_close,
  ism_read, ism_write,
  ism_length, ism_tell, ism_seek, ism_seek,
  ism_destroy
};

istream_t * istream_mem_create(const void * addr, int len, int mode)
{
  istream_mem_t *ism;

  if (len < 0 || (!addr && len)) {
    return 0;
  }

  ism = SC68alloc(sizeof(istream_mem_t));
  if (!ism) {
    return 0;
  }

  ism->istream = istream_mem;
  ism->buffer = (char *)addr;
  ism->size = len;
  ism->mode = mode & (ISTREAM_OPEN_READ|ISTREAM_OPEN_WRITE);
  ism->open = 0;
  ism->pos = 0;
  sprintf(ism->name,"mem://%p:%p", addr, (char *)addr+len);

  return &ism->istream;
}

#else /* #ifndef ISTREAM_NO_FILE */

/* istream mem must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

#include "file68/istream68_mem.h"

istream_t * istream_mem_create(const void * addr, int len, int mode)
{
  return 0;
}

#endif
