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
 * binstr.h - Binary I/O on standard C strings in memory
 * Copyright (C) 2003 Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_BINIO_BINSTR
#define H_BINIO_BINSTR

#include "binio.h"

class binsbase: virtual public binio
{
public:
  binsbase(void *str, unsigned long len);
  virtual ~binsbase();

  virtual void seek(long p, Offset offs = Set);
  virtual long pos();

protected:
  Byte	*data, *spos;
  long	length;
};

class binisstream: public binistream, virtual public binsbase
{
public:
  binisstream(void *str, unsigned long len);
  virtual ~binisstream();

protected:
  virtual Byte getByte();
};

class binosstream: public binostream, virtual public binsbase
{
public:
  binosstream(void *str, unsigned long len);
  virtual ~binosstream();

protected:
  virtual void putByte(Byte b);
};

class binsstream: public binisstream, public binosstream
{
public:
  binsstream(void *str, unsigned long len);
  virtual ~binsstream();
};

#endif
