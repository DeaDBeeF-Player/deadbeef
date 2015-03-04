/*
 *                      sc68 - gzip file loader
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

#include <stdlib.h>

#ifdef HAVE_ZLIB_H
# include <zlib.h>
#else
# define Z_DEFLATED   8 /* From zlib.h */
#endif

#include <string.h>

static char gz_magic[] = {0x1f, 0x8b, Z_DEFLATED}; /* gzip magic header */

int gzip_is_magic(const void * buffer)
{
  return !memcmp(gz_magic,buffer,sizeof(gz_magic));
}

#ifdef HAVE_ZLIB_H

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _MSC_VER
# include <stdio.h>
# include <io.h>
#else
# include <unistd.h>
#endif

#include "file68/gzip68.h"
#include "file68/error68.h"
#include "file68/alloc68.h"

static int is_gz(int fd, int len)
{
  char magic[sizeof(gz_magic)];
  int inflate_len = -1;
  
  /* header + inflate len */
  if (len < 10+4) {
    goto error;
  }

  /* Read and check magic. */
  if (read(fd, magic, sizeof(magic)) != sizeof(magic) ||
      memcmp(magic, gz_magic, sizeof(magic))) {
    goto error;
  }

  /* Get uncompressed size at the end of file.
   */
  if (lseek(fd, -4, SEEK_END) == (off_t) -1) {
    goto error;
  } else {
    unsigned char buffer[4];
    if(read(fd, buffer, 4) != 4) {
      goto error;
    }
    inflate_len =
      buffer[0]
      | (buffer[1] << 8)
      | (buffer[2] << 16)
      | (buffer[3] << 24);
    if (inflate_len < 0) {
      inflate_len = -1;
    }
  }
 error:

  /* Rewind */
  lseek(fd, 0, SEEK_SET);
  return inflate_len;
}


/* Windows opens streams in Text mode by default. */
#ifndef _O_BINARY
# define _O_BINARY 0
#endif

void *gzip_load(const char *fname, int *ptr_ulen)
{
  int fd, err;
  gzFile f = 0;
  int ulen = 0;
  void * uncompr = 0;
  off_t len;
  const int omode = O_RDONLY | _O_BINARY;

  fd = open(fname, omode);
  if (fd == -1) {
    SC68error_add("gzip_load(%s) : %s", fname, strerror(errno));
    goto error;
  }

  len = lseek(fd, 0, SEEK_END);
  if (len == (off_t) -1) {
    SC68error_add("gzip_load(%s) : %s", fname, strerror(errno));
    goto error;
  }

  if (lseek(fd, 0, SEEK_SET) != 0) {
    SC68error_add("gzip_load(%s) : %s", fname, strerror(errno));
    goto error;
  }

  ulen = is_gz(fd, len);
  if (ulen == -1) {
    ulen = len;
  }

  f = gzdopen(fd, "rb");
  if (!f) {
    SC68error_add("gzip_load(%s) :  %s", fname, gzerror(f, &err));
    goto error;
  }
  fd = 0; /* $$$ Closed by gzclose(). Verify fdopen() rules. */

  uncompr = SC68alloc(ulen);
  if (!uncompr) {
    SC68error_add("gzip_load(%s) : alloc (%d) failed", fname, ulen);
    goto error;
  }
  len = gzread(f, uncompr, ulen);

  if (len != ulen) {
    SC68error_add("gzip_load(%s) : read : %s",fname, gzerror (f, &err));
    goto error;
  }
  goto end;
  
 error:
  if (uncompr) {
    SC68free(uncompr);
    uncompr = 0;
    ulen = 0;
  }

 end:
  if (fd) {
    close(fd);
  }
  if (f) {
    gzclose(f);
  }
  if (ptr_ulen) {
    *ptr_ulen = ulen;
  }
     
  return uncompr;
}

#else

#include "file68/error68.h"

void *gzip_load(const char *fname, int *ptr_ulen)
{
  if (ptr_ulen) *ptr_ulen=0;
  SC68error_add("gzip_load(%s) : no zlib support", fname);
  return 0;
}

#endif /* #ifdef HAVE_ZLIB_H */

