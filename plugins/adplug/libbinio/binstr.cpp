/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * binstr.cpp - Binary I/O on standard C strings in memory
 * Copyright (C) 2003, 2010 Simon Peter <dn.tlp@gmx.net>
 */

#include "binstr.h"

/***** binsbase *****/

binsbase::binsbase(void *str, unsigned long len)
  : data((Byte *)str), spos((Byte *)str), length(len)
{
}

binsbase::~binsbase()
{
}

void binsbase::seek(long p, Offset offs)
{
  switch(offs) {
  case Set: spos = data + p; break;
  case Add: spos += p; break;
  case End: spos = data + length + p; break;
  }

  // Seek before start of data
  if(spos < data) {
    spos = data;
    return;
  }

  // Seek after end of data
  if(spos - data > length) {
    err |= Eof;
    spos = data + length;
  }
}

long binsbase::pos()
{
  return (long)(spos - data);
}

/***** binisstream *****/

binisstream::binisstream(void *str, unsigned long len)
  : binsbase(str, len)
{
}

binisstream::~binisstream()
{
}

binisstream::Byte binisstream::getByte()
{
  Byte in = 0;

  if(spos - data >= length)
    err |= Eof;
  else {
    in = *spos;
    spos++;
  }

  return in;
}

/***** binosstream *****/

binosstream::binosstream(void *str, unsigned long len)
  : binsbase(str, len)
{
}

binosstream::~binosstream()
{
}

void binosstream::putByte(Byte b)
{
  if(spos - data >= length) {
    err |= Eof;
  } else {
    *spos = b;
    spos++;
  }
}

/***** binsstream *****/

binsstream::binsstream(void *str, unsigned long len)
  : binsbase(str, len), binisstream(str, len), binosstream(str, len)
{
}

binsstream::~binsstream()
{
}
