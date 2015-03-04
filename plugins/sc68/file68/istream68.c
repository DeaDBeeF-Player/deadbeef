/*
 *                         sc68 - stream operations
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

#include "file68/istream68_def.h"

const char * istream_filename(istream_t * istream)
{
  return (!istream || !istream->name)
    ? 0
    : istream->name(istream);
}

int istream_open(istream_t *istream)
{
  return (!istream || !istream->open)
    ? -1
    : istream->open(istream);
}

int istream_close(istream_t *istream)
{
  return (!istream || !istream->close)
    ? -1
    : istream->close(istream);
}

int istream_read(istream_t *istream, void * data, int len)
{
  return (!istream || !istream->read)
    ? -1
    : istream->read(istream, data, len);
}

int istream_write(istream_t *istream, const void * data, int len)
{
  return (!istream || !istream->write)
    ? -1
    : istream->write(istream, data, len);
}

int istream_length(istream_t *istream)
{
  return (!istream || !istream->length)
    ? -1
    : istream->length(istream);
}

int istream_tell(istream_t *istream)
{
  return (!istream || !istream->tell)
    ? -1
    : istream->tell(istream);
}

static int isseek(istream_t *istream, int pos, int offset)
{
  if (pos != -1) {
    if (offset) {
      istream_seek_t seek = (offset > 0) ? istream->seekf : istream->seekb;
      if (seek && seek(istream, offset) != -1) {
	pos += offset;
      } else {
	pos = -1;
      }
    }
  }
  return pos;
}

int istream_seek(istream_t *istream, int offset)
{
  return isseek(istream, istream_tell(istream), offset);
}

int istream_seek_to(istream_t *istream, int pos)
{
  int cur = istream_tell(istream);

  return isseek(istream, cur, pos-cur);
}

void istream_destroy(istream_t *istream)
{
  if (istream) {
    if (istream->close) {
      istream->close(istream);
    }
    if (istream->destroy) {
      istream->destroy(istream);
    }
  }
}

int istream_gets(istream_t *istream, char * buffer, int max)
{
  int i;

  if (!istream || !istream->read || !buffer || max <= 0) {
    return -1;
  }
  
  for (i=0, --max; i<max; ) {
    char c;
    int err;
    err = istream->read(istream, &c, 1);
    if (err == -1) {
      return -1;
    }
    if (err != 1) {
      break;
    }
    buffer[i++] = c;
    if (c == '\n') {
      break;
    }
  }
  buffer[i] = 0;
  return i;
}
