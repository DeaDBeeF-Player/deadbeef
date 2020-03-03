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
 * binwrap.cpp - Binary I/O wrapper, using standard iostreams library
 * Copyright (C) 2002, 2003, 2009 Simon Peter <dn.tlp@gmx.net>
 */

#include "binwrap.h"

#if BINIO_ENABLE_IOSTREAM

/***** biniwstream *****/

biniwstream::biniwstream(istream *istr)
  : in(istr)
{
}

biniwstream::~biniwstream()
{
}

void biniwstream::seek(long pos, Offset offs)
{
  if(!in) { err = NotOpen; return; }

  switch(offs) {
  case Set: in->seekg(pos, ios::beg); break;
  case Add: in->seekg(pos, ios::cur); break;
  case End: in->seekg(pos, ios::end); break;
  }
}

biniwstream::Byte biniwstream::getByte()
{
  if(!in) { err = NotOpen; return 0; }

  if(!in->eof()) {
    return (Byte)in->get();
  } else {
    err |= Eof;
    return 0;
  }
}

long biniwstream::pos()
{
  if(!in) { err = NotOpen; return 0; }
  return (long)in->tellg();
}

/***** binowstream *****/

binowstream::binowstream(ostream *ostr)
  : out(ostr)
{
}

binowstream::~binowstream()
{
}

void binowstream::seek(long pos, Offset offs)
{
  if(!out) { err = NotOpen; return; }

  switch(offs) {
  case Set: out->seekp(pos, ios::beg); break;
  case Add: out->seekp(pos, ios::cur); break;
  case End: out->seekp(pos, ios::end); break;
  }
}

void binowstream::putByte(binio::Byte b)
{
  if(!out) { err = NotOpen; return; }
  out->put((char)b);
}

long binowstream::pos()
{
  if(!out) { err = NotOpen; return 0; }
  return (long)out->tellp();
}

/***** binwstream *****/

binwstream::binwstream(iostream *str)
  : biniwstream(str), binowstream(str), io(str)
{
}

binwstream::~binwstream()
{
}

void binwstream::seek(long pos, Offset offs)
{
  biniwstream::seek(pos, offs);
  binowstream::seek(pos, offs);
}

long binwstream::pos()
{
  if(!io) { err = NotOpen; return 0; }
  return (long)io->tellg();
}

binwstream::Byte binwstream::getByte()
{
  Byte in = biniwstream::getByte();
  binowstream::seek(biniwstream::pos(), Set);	// sync stream position
  return in;
}

void binwstream::putByte(Byte b)
{
  binowstream::putByte(b);
  biniwstream::seek(binowstream::pos(), Set);	// sync stream position
}

#endif
