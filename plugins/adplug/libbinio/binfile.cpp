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

#include <stdio.h>
#include <errno.h>

#include "binfile.h"

/***** binfbase *****/

binfbase::binfbase()
  : f(NULL)
{
}

binfbase::~binfbase()
{
  if(f != NULL) close();
}

void binfbase::close()
{
  if(f != NULL) {
    if(fclose(f) == EOF) err |= Fatal; else f = NULL;
  } else
    err |= NotOpen;
}

void binfbase::seek(long pos, Offset offs)
{
  int error;

  if(f == NULL) { err |= NotOpen; return; }

  switch(offs) {
  case Set: error = fseek(f, pos, SEEK_SET); break;
  case Add: error = fseek(f, pos, SEEK_CUR); break;
  case End: error = fseek(f, pos, SEEK_END); break;
  }

  if(error == -1) err |= Fatal;
}

long binfbase::pos()
{
  long pos;

  if(f == NULL) { err |= NotOpen; return 0; }

  pos = ftell(f);

  if(pos == -1) {
    err |= Fatal;
    return 0;
  } else
    return pos;
}

/***** binifstream *****/

binifstream::binifstream()
{
}

binifstream::binifstream(const char *filename, const Mode mode)
{
  open(filename, mode);
}

#if BINIO_ENABLE_STRING
binifstream::binifstream(const std::string &filename, const Mode mode)
{
  open(filename, mode);
}
#endif

binifstream::~binifstream()
{
}

void binifstream::open(const char *filename, const Mode mode)
{
  f = fopen(filename, "rb");

  if(f == NULL)
    switch(errno) {
    case ENOENT: err |= NotFound; break;
    case EACCES: err |= Denied; break;
    default: err |= NotOpen; break;
    }
}

#if BINIO_ENABLE_STRING
void binifstream::open(const std::string &filename, const Mode mode)
{
  open(filename.c_str(), mode);
}
#endif

binifstream::Byte binifstream::getByte()
{
  int read;

  if(f != NULL) {
    read = fgetc(f);
    if(read == EOF) err |= Eof;
    return (Byte)read;
  } else {
    err |= NotOpen;
    return 0;
  }
}

/***** binofstream *****/

binofstream::binofstream()
{
}

binofstream::binofstream(const char *filename, const Mode mode)
{
  open(filename, mode);
}

#if BINIO_ENABLE_STRING
binofstream::binofstream(const std::string &filename, const Mode mode)
{
  open(filename, mode);
}
#endif

binofstream::~binofstream()
{
}

void binofstream::open(const char *filename, const Mode mode)
{
  char *modestr = "wb";

  // Check if append mode is desired
  if(mode & Append) modestr = "ab";

  f = fopen(filename, modestr);

  if(f == NULL)
    switch(errno) {
    case EEXIST:
    case EACCES:
    case EROFS:
      err |= Denied;
      break;
    case ENOENT: err |= NotFound; break;
    default: err |= NotOpen; break;
    }
}

#if BINIO_ENABLE_STRING
void binofstream::open(const std::string &filename, const Mode mode)
{
  open(filename.c_str(), mode);
}
#endif

void binofstream::putByte(Byte b)
{
  if(f == NULL) { err |= NotOpen; return; }

  if(fputc(b, f) == EOF)
    err |= Fatal;
}

/***** binfstream *****/

binfstream::binfstream()
{
}

binfstream::binfstream(const char *filename, const Mode mode)
{
  open(filename, mode);
}

#if BINIO_ENABLE_STRING
binfstream::binfstream(const std::string &filename, const Mode mode)
{
  open(filename, mode);
}
#endif

binfstream::~binfstream()
{
}

void binfstream::open(const char *filename, const Mode mode)
{
  char	*modestr = "w+b";	// Create & at beginning
  int	ferror = 0;

  // Apply desired mode
  if(mode & NoCreate) {
    if(!(mode & Append))
      modestr[0] = 'r';	// NoCreate & at beginning
  } else
    if(mode & Append)	// Create & append
      modestr[0] = 'a';

  f = fopen(filename, modestr);

  // NoCreate & append (emulated -- not possible with standard C fopen())
  if(f != NULL && (mode & Append) && (mode & NoCreate))
    ferror = fseek(f, 0, SEEK_END);

  if(f == NULL || ferror == -1) {
    switch(errno) {
    case EEXIST:
    case EACCES:
    case EROFS:
      err |= Denied;
      break;
    case ENOENT: err |= NotFound; break;
    default: err |= NotOpen; break;
    }
  }
}

#if BINIO_ENABLE_STRING
void binfstream::open(const std::string &filename, const Mode mode)
{
  open(filename.c_str(), mode);
}
#endif
