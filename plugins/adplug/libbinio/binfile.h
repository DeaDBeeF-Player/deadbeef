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
 * binfile.h - Binary file I/O
 * Copyright (C) 2002, 2003 Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_BINIO_BINFILE
#define H_BINIO_BINFILE

#include <stdio.h>

#include "binio.h"

#include <deadbeef/deadbeef.h>

class binfbase: virtual public binio
{
public:
  typedef enum {
    Append	= 1 << 0,
    NoCreate	= 1 << 1
  } ModeFlags;

  typedef int Mode;

  binfbase();
  virtual ~binfbase();

  virtual void open(const char *filename, const Mode mode) = 0;
#if BINIO_ENABLE_STRING
  virtual void open(const std::string &filename, const Mode mode) = 0;
#endif
  void close();

  virtual void seek(long pos, Offset offs = Set);
  virtual long pos();

protected:
  DB_FILE *f;
};

class binifstream: public binistream, virtual public binfbase
{
public:
  binifstream();
  binifstream(const char *filename, const Mode mode = NoCreate);
#if BINIO_ENABLE_STRING
  binifstream(const std::string &filename, const Mode mode = NoCreate);
#endif

  virtual ~binifstream();

  virtual void open(const char *filename, const Mode mode = NoCreate);
#if BINIO_ENABLE_STRING
  virtual void open(const std::string &filename, const Mode mode = NoCreate);
#endif

protected:
  virtual Byte getByte();
  virtual void getBuf(char *buf, int size);
};

class binofstream: public binostream, virtual public binfbase
{
public:
  binofstream();
  binofstream(const char *filename, const Mode mode = 0);
#if BINIO_ENABLE_STRING
  binofstream(const std::string &filename, const Mode mode = 0);
#endif

  virtual ~binofstream();

  virtual void open(const char *filename, const Mode mode = 0);
#if BINIO_ENABLE_STRING
  virtual void open(const std::string &filename, const Mode mode = 0);
#endif

protected:
  virtual void putByte(Byte b);
};

class binfstream: public binifstream, public binofstream
{
public:
  binfstream();
  binfstream(const char *filename, const Mode mode = 0);
#if BINIO_ENABLE_STRING
  binfstream(const std::string &filename, const Mode mode = 0);
#endif

  virtual ~binfstream();

  virtual void open(const char *filename, const Mode mode = 0);
#if BINIO_ENABLE_STRING
  virtual void open(const std::string &filename, const Mode mode = 0);
#endif
};

#endif
