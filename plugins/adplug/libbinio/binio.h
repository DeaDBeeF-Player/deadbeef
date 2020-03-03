/* -*-C++-*-
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
 * binio.h - Binary stream I/O classes
 * Copyright (C) 2002, 2003, 2005 Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_BINIO_BINIO
#define H_BINIO_BINIO

/***** Configuration *****/

// BINIO_ENABLE_STRING - Build std::string supporting methods
//
// Set to 1 to build std::string supporting methods. You need the STL to
// do this.
#define BINIO_ENABLE_STRING	1

// BINIO_ENABLE_IOSTREAM - Build iostream wrapper classes
//
// Set to 1 to build the iostream wrapper classes. You need the standard
// C++ library to do this.
#define BINIO_ENABLE_IOSTREAM	1

// BINIO_ISO_STDLIB - Build with ISO C++ standard library compliance
//
// Set to 1 to build for the ISO standard C++ library (i.e. namespaces, STL and
// templatized iostream). Set to 0 to build for the traditional C++ library.
#define BINIO_ISO_STDLIB	1

// BINIO_WITH_MATH - Build with 'math.h' dependency to allow float conversions
//
// Set to 1 to also build routines that depend on the 'math.h' standard C header
// file (this sometimes also implies a 'libm' or 'libmath' dependency). These
// routines are needed in order to write IEEE-754 floating-point numbers on a
// system that doesn't support this format natively. For only reading these
// numbers, however, these routines are not needed. If set to 0, writing
// IEEE-754 numbers on an incompatible system will be disabled.
#define BINIO_WITH_MATH		1

/***** Implementation *****/

// Disable annoying multiple inheritance compiler warning on MSVC6
#ifdef _MSC_VER
#	pragma warning(disable: 4250)
#endif

#if BINIO_ENABLE_STRING
#include <string>
#endif

class binio
{
public:
  typedef enum {
    BigEndian	= 1 << 0,
    FloatIEEE	= 1 << 1
  } Flag;

  typedef enum {
    NoError	= 0,
    Fatal	= 1 << 0,
    Unsupported	= 1 << 1,
    NotOpen	= 1 << 2,
    Denied	= 1 << 3,
    NotFound	= 1 << 4,
    Eof		= 1 << 5
  } ErrorCode;

  typedef enum { Set, Add, End } Offset;
  typedef enum { Single, Double } FType;
  typedef int Error;

  binio();
  virtual ~binio();

  void setFlag(Flag f, bool set = true);
  bool getFlag(Flag f);

  Error error();
  bool eof();

  virtual void seek(long, Offset = Set) = 0;
  virtual long pos() = 0;

protected:
  typedef long long	Int;
  typedef long double	Float;
  typedef unsigned char	Byte;	// has to be unsigned!

  typedef int		Flags;

  Flags			my_flags;
  static const Flags	system_flags;
  Error			err;

  // Some math.h emulation functions...
#if !BINIO_WITH_MATH
  Float pow(Float base, signed int exp);
  Float ldexp(Float x, signed int exp) { return x * pow(2, exp); }
#endif

private:
  static const Flags detect_system_flags();
};

class binistream: virtual public binio
{
public:
  binistream();
  virtual ~binistream();

  Int readInt(unsigned int size);
  Float readFloat(FType ft);
  unsigned long readString(char *str, unsigned long maxlen);
  unsigned long readString(char *str, unsigned long maxlen, const char delim);
#if BINIO_ENABLE_STRING
  std::string readString(const char delim = '\0');
#endif

  Int peekInt(unsigned int size);
  Float peekFloat(FType ft);

  bool ateof();
  void ignore(unsigned long amount = 1);

protected:
  virtual Byte getByte() = 0;

private:
  Float ieee_single2float(Byte *data);
  Float ieee_double2float(Byte *data);
};

class binostream: virtual public binio
{
public:
  binostream();
  virtual ~binostream();

  void writeInt(Int val, unsigned int size);
  void writeFloat(Float f, FType ft);
  unsigned long writeString(const char *str, unsigned long amount = 0);
#if BINIO_ENABLE_STRING
  unsigned long writeString(const std::string &str);
#endif

protected:
  virtual void putByte(Byte) = 0;

private:
  void float2ieee_single(Float f, Byte *data);
  void float2ieee_double(Float f, Byte *data);
};

class binstream: public binistream, public binostream
{
public:
  binstream();
  virtual ~binstream();
};

#endif
