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
 * binwrap.h - Binary I/O wrapper, using standard iostreams library
 * Copyright (C) 2002, 2003 Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_BINIO_BINWRAP
#define H_BINIO_BINWRAP

#include "binio.h"

#if BINIO_ENABLE_IOSTREAM

#if BINIO_ISO_STDLIB
#include <iostream>

using std::iostream;
using std::ostream;
using std::istream;
using std::ios;
using std::streambuf;
#else

#include <iostream.h>

#endif

class biniwstream: public binistream
{
public:
  biniwstream(istream *istr);
  virtual ~biniwstream();

  virtual void seek(long pos, Offset offs = Set);
  virtual long pos();

protected:
  virtual Byte getByte();

private:
  istream *in;
};

class binowstream: public binostream
{
public:
  binowstream(ostream *ostr);
  virtual ~binowstream();

  virtual void seek(long pos, Offset offs = Set);
  virtual long pos();

protected:
  virtual void putByte(Byte b);

private:
  ostream *out;
};

class binwstream: public biniwstream, public binowstream
{
public:
  binwstream(iostream *str);
  virtual ~binwstream();

  virtual void seek(long pos, Offset offs = Set);
  virtual long pos();

protected:
  virtual Byte getByte();
  virtual void putByte(Byte b);

private:
  iostream *io;
};

#endif

#endif
