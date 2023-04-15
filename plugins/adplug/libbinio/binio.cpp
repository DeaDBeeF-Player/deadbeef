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
 * binio.cpp - Binary stream I/O classes
 * Copyright (C) 2002, 2003 Simon Peter <dn.tlp@gmx.net>
 */

#include <string.h>

#include "binio.h"

#if BINIO_WITH_MATH

#include <math.h>

#ifdef __QNXNTO__
   #define pow std::powf
#endif // __QNXNTO__

// If 'math.h' doesn't define HUGE_VAL, we try to use HUGE instead.
#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif

#endif

/***** Defines *****/

#if BINIO_ENABLE_STRING
// String buffer size for std::string readString() method
#define STRINGBUFSIZE	256
#endif

/***** binio *****/

const binio::Flags binio::system_flags = binio::detect_system_flags();

const binio::Flags binio::detect_system_flags()
{
  Flags f = 0;

  // Endian test
  union {
    int word;
    Byte byte;
  } endian_test;

  endian_test.word = 1;
  if(endian_test.byte != 1) f |= BigEndian;

  // IEEE-754 floating-point test
  float fl = 6.5;
  Byte	*dat = (Byte *)&fl;

  if(sizeof(float) == 4 && sizeof(double) == 8)
    if(f & BigEndian) {
      if(dat[0] == 0x40 && dat[1] == 0xD0 && !dat[2] && !dat[3])
	f |= FloatIEEE;
    } else
      if(dat[3] == 0x40 && dat[2] == 0xD0 && !dat[1] && !dat[0])
      f |= FloatIEEE;

  return f;
}

binio::binio()
  : my_flags(system_flags), err(NoError)
{
}

binio::~binio()
{
}

void binio::setFlag(Flag f, bool set)
{
  if(set)
    my_flags |= f;
  else
    my_flags &= !f;
}

bool binio::getFlag(Flag f)
{
  return (my_flags & f ? true : false);
}

binio::Error binio::error()
{
  Error e = err;

  err = NoError;
  return e;
}

bool binio::eof()
{
  return (err & Eof ? true : false);
}

/***** binistream *****/

binistream::binistream()
{
}

binistream::~binistream()
{
}

binistream::Int binistream::readInt(unsigned int size)
{
  unsigned int	i;
  Int		val = 0, in;

  // Check if 'size' doesn't exceed our system's biggest type.
  if(size > sizeof(Int)) {
    err |= Unsupported;
    return 0;
  }

  for(i = 0; i < size; i++) {
    in = getByte();
    if(getFlag(BigEndian))
      val <<= 8;
    else
      in <<= i * 8;
    val |= in;
  }

  return val;
}

void binistream::readBuf(char *buf, int size) {
    getBuf (buf, size);
}


binistream::Float binistream::readFloat(FType ft)
{
  if(getFlag(FloatIEEE)) {	// Read IEEE-754 floating-point value
    unsigned int	i, size;
    Byte		in[8];
    bool		swap;

    // Determine appropriate size for given type.
    switch(ft) {
    case Single: size = 4; break;	// 32 bits
    case Double: size = 8; break;	// 64 bits
    }

    // Determine byte ordering, depending on what we do next
    if(system_flags & FloatIEEE)
      swap = getFlag(BigEndian) ^ (system_flags & BigEndian);
    else
      swap = !getFlag(BigEndian);

    // Read the float byte by byte, converting endianess
    for(i = 0; i < size; i++)
      if(swap)
	in[size - i - 1] = getByte();
      else
	in[i] = getByte();

    if(system_flags & FloatIEEE) {
      // Compatible system, let the hardware do the conversion
      switch(ft) {
      case Single: return *(float *)in;
      case Double: return *(double *)in;
      }
    } else {	// Incompatible system, convert manually
      switch(ft) {
      case Single: return ieee_single2float(in);
      case Double: return ieee_double2float(in);
      }
    }
  }

  // User tried to read a (yet) unsupported floating-point type. Bail out.
  err |= Unsupported; return 0.0;
}

binistream::Float binistream::ieee_single2float(Byte *data)
{
  signed int	sign = data[0] >> 7 ? -1 : 1;
  unsigned int	exp = ((data[0] << 1) & 0xff) | ((data[1] >> 7) & 1),
    fracthi7 = data[1] & 0x7f;
  Float		fract = fracthi7 * 65536.0 + data[2] * 256.0 + data[3];

  // Signed and unsigned zero
  if(!exp && !fracthi7 && !data[2] && !data[3]) return sign * 0.0;

  // Signed and unsigned infinity (maybe unsupported on non-IEEE systems)
  if(exp == 255)
    if(!fracthi7 && !data[2] && !data[3]) {
#ifdef HUGE_VAL
      if(sign == -1) return -HUGE_VAL; else return HUGE_VAL;
#else
      err |= Unsupported;
      if(sign == -1) return -1.0; else return 1.0;
#endif
    } else {	  // Not a number (maybe unsupported on non-IEEE systems)
#ifdef NAN
      return NAN;
#else
      err |= Unsupported; return 0.0;
#endif
    }

  if(!exp)	// Unnormalized float values
    return sign * pow(2, -126) * fract * pow(2, -23);
  else		// Normalized float values
    return sign * pow(2, exp - 127) * (fract * pow(2, -23) + 1);

  err |= Fatal; return 0.0;
}

binistream::Float binistream::ieee_double2float(Byte *data)
{
  signed int	sign = data[0] >> 7 ? -1 : 1;
  unsigned int	exp = ((unsigned int)(data[0] & 0x7f) << 4) | (data[1] >> 4),
    fracthi4 = data[1] & 0xf;
  Float		fract = fracthi4 * pow(2, 48) + data[2] * pow(2, 40) + data[3] *
    pow(2, 32) + data[4] * pow(2, 24) + data[5] * pow(2, 16) + data[6] *
    pow(2, 8) + data[7];

  // Signed and unsigned zero
  if(!exp && !fracthi4 && !data[2] && !data[3] && !data[4] && !data[5] &&
     !data[6] && !data[7]) return sign * 0.0;

  // Signed and unsigned infinity  (maybe unsupported on non-IEEE systems)
  if(exp == 2047)
    if(!fracthi4 && !data[2] && !data[3] && !data[4] && !data[5] && !data[6] &&
       !data[7]) {
#ifdef HUGE_VAL
      if(sign == -1) return -HUGE_VAL; else return HUGE_VAL;
#else
      err |= Unsupported;
      if(sign == -1) return -1.0; else return 1.0;
#endif
    } else {	  // Not a number (maybe unsupported on non-IEEE systems)
#ifdef NAN
      return NAN;
#else
      err |= Unsupported; return 0.0;
#endif
    }

  if(!exp)	// Unnormalized float values
    return sign * pow(2, -1022) * fract * pow(2, -52);
  else		// Normalized float values
    return sign * pow(2, exp - 1023) * (fract * pow(2, -52) + 1);

  err |= Fatal; return 0.0;
}

#if !BINIO_WITH_MATH
binio::Float binio::pow(Float base, signed int exp)
/* Our own, stripped-down version of pow() for not having to depend on 'math.h'.
 * This one handles float values for the base and an integer exponent, both
 * positive and negative.
 */
{
  int	i;
  Float	val = base;

  if(!exp) return 1.0;

  for(i = 1; i < (exp < 0 ? -exp : exp); i++)
    val *= base;

  if(exp < 0) val = 1.0 / val;

  return val;
}
#endif

unsigned long binistream::readString(char *str, unsigned long maxlen)
{
  unsigned long	i;

  for(i = 0; i < maxlen; i++) {
    str[i] = (char)getByte();
    if(err) { str[i] = '\0'; return i; }
  }

  return maxlen;
}

unsigned long binistream::readString(char *str, unsigned long maxlen,
				     const char delim)
{
  unsigned long i;

  for(i = 0; i < maxlen; i++) {
    str[i] = (char)getByte();
    if(str[i] == delim || err) { str[i] = '\0'; return i; }
  }

  str[maxlen] = '\0';
  return maxlen;
}

#if BINIO_ENABLE_STRING
std::string binistream::readString(const char delim)
{
  char buf[STRINGBUFSIZE + 1];
  std::string tempstr;
  unsigned long read;

  do {
    read = readString(buf, STRINGBUFSIZE, delim);
    tempstr.append(buf, read);
  } while(read == STRINGBUFSIZE);

  return tempstr;
}
#endif

binistream::Int binistream::peekInt(unsigned int size)
{
  Int val = readInt(size);
  if(!err) seek(-(long)size, Add);
  return val;
}

binistream::Float binistream::peekFloat(FType ft)
{
  Float val = readFloat(ft);

  if(!err)
    switch(ft) {
    case Single: seek(-4, Add); break;
    case Double: seek(-8, Add); break;
    }

  return val;
}

bool binistream::ateof()
{
  Error	olderr = err;	// Save current error state
  bool	eof_then;

  peekInt(1);
  eof_then = eof();	// Get error state of next byte
  err = olderr;		// Restore original error state
  return eof_then;
}

void binistream::ignore(unsigned long amount)
{
  unsigned long i;

  for(i = 0; i < amount; i++)
    getByte();
}

/***** binostream *****/

binostream::binostream()
{
}

binostream::~binostream()
{
}

void binostream::writeInt(Int val, unsigned int size)
{
  unsigned int	i;

  // Check if 'size' doesn't exceed our system's biggest type.
  if(size > sizeof(Int)) { err |= Unsupported; return; }

  for(i = 0; i < size; i++) {
    if(getFlag(BigEndian))
      putByte((val >> (size - i - 1) * 8) & 0xff);
    else {
      putByte(val & 0xff);
      val >>= 8;
    }
  }
}

void binostream::writeFloat(Float f, FType ft)
{
  if(getFlag(FloatIEEE)) {	// Write IEEE-754 floating-point value
    unsigned int	i, size;
    Byte		*out;
    bool		swap;

    if(system_flags & FloatIEEE) {
      // compatible system, let the hardware do the conversion
      float	outf = f;
      double	outd = f;

      // Hardware could be big or little endian, convert appropriately
      swap = getFlag(BigEndian) ^ (system_flags & BigEndian);

      // Determine appropriate size for given type and convert by hardware
      switch(ft) {
      case Single: size = 4; out = (Byte *)&outf; break;	// 32 bits
      case Double: size = 8; out = (Byte *)&outd; break;	// 64 bits
      }
    } else {
#if BINIO_WITH_MATH
      // incompatible system, do the conversion manually
      Byte	buf[8];

      // Our own value is always big endian, just check whether we have to
      // convert for a different stream format.
      swap = !getFlag(BigEndian);

      // Convert system's float to requested IEEE-754 float
      switch(ft) {
      case Single: size = 4; float2ieee_single(f, buf); break;
      case Double: size = 8; float2ieee_double(f, buf); break;
      }

      out = buf;	// Make the value ready for writing
#else
      // No necessary support routines to do the conversion, bail out!
      err |= Unsupported; return;
#endif
    }

    // Write the float byte by byte, converting endianess
    if(swap) out += size - 1;
    for(i = 0; i < size; i++) {
      putByte(*out);
      if(swap) out--; else out++;
    }

    return;	// We're done.
  }

  // User tried to write an unsupported floating-point type. Bail out.
  err |= Unsupported;
}

#ifdef BINIO_WITH_MATH

/*
 * Single and double floating-point to IEEE-754 equivalent conversion functions
 * courtesy of Ken Turkowski.
 *
 * Copyright (C) 1989-1991 Ken Turkowski. <turk@computer.org>
 *
 * All rights reserved.
 *
 * Warranty Information
 *  Even though I have reviewed this software, I make no warranty
 *  or representation, either express or implied, with respect to this
 *  software, its quality, accuracy, merchantability, or fitness for a
 *  particular purpose.  As a result, this software is provided "as is,"
 *  and you, its user, are assuming the entire risk as to its quality
 *  and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the above warranty information.
 */

/****************************************************************
 * The following two routines make up for deficiencies in many
 * compilers to convert properly between unsigned integers and
 * floating-point.  Some compilers which have this bug are the
 * THINK_C compiler for the Macintosh and the C compiler for the
 * Silicon Graphics MIPS-based Iris.
 ****************************************************************/

#ifdef applec	/* The Apple C compiler works */
# define FloatToUnsigned(f)	((unsigned long)(f))
#else
# define FloatToUnsigned(f)	((unsigned long)(((long)((f) - 2147483648.0)) + 2147483647L + 1))
#endif

#define SEXP_MAX	255
#define SEXP_OFFSET	127
#define SEXP_SIZE	8
#define SEXP_POSITION	(32-SEXP_SIZE-1)

void binostream::float2ieee_single(Float num, Byte *bytes)
{
  long		sign;
  register long	bits;

  if (num < 0) {	/* Can't distinguish a negative zero */
    sign = 0x80000000;
    num *= -1;
  } else {
    sign = 0;
  }

  if (num == 0) {
    bits = 0;
  } else {
    Float	fMant;
    int		expon;

    fMant = frexp(num, &expon);

    if ((expon > (SEXP_MAX-SEXP_OFFSET+1)) || !(fMant < 1)) {
      /* NaN's and infinities fail second test */
      bits = sign | 0x7F800000;		/* +/- infinity */
    }

    else {
      long mantissa;

      if (expon < -(SEXP_OFFSET-2)) {	/* Smaller than normalized */
	int shift = (SEXP_POSITION+1) + (SEXP_OFFSET-2) + expon;
	if (shift < 0) {	/* Way too small: flush to zero */
	  bits = sign;
	}
	else {			/* Nonzero denormalized number */
	  mantissa = (long)(fMant * (1L << shift));
	  bits = sign | mantissa;
	}
      }

      else {				/* Normalized number */
	mantissa = (long)floor(fMant * (1L << (SEXP_POSITION+1)));
	mantissa -= (1L << SEXP_POSITION);			/* Hide MSB */
	bits = sign | ((long)((expon + SEXP_OFFSET - 1)) << SEXP_POSITION) | mantissa;
      }
    }
  }

  bytes[0] = bits >> 24;	/* Copy to byte string */
  bytes[1] = bits >> 16;
  bytes[2] = bits >> 8;
  bytes[3] = bits;
}

#define DEXP_MAX	2047
#define DEXP_OFFSET	1023
#define DEXP_SIZE	11
#define DEXP_POSITION	(32-DEXP_SIZE-1)

void binostream::float2ieee_double(Float num, Byte *bytes)
{
  long	sign;
  long	first, second;

  if (num < 0) {	/* Can't distinguish a negative zero */
    sign = 0x80000000;
    num *= -1;
  } else {
    sign = 0;
  }

  if (num == 0) {
    first = 0;
    second = 0;
  } else {
    Float	fMant, fsMant;
    int		expon;

    fMant = frexp(num, &expon);

    if ((expon > (DEXP_MAX-DEXP_OFFSET+1)) || !(fMant < 1)) {
      /* NaN's and infinities fail second test */
      first = sign | 0x7FF00000;		/* +/- infinity */
      second = 0;
    }

    else {
      long mantissa;

      if (expon < -(DEXP_OFFSET-2)) {	/* Smaller than normalized */
	int shift = (DEXP_POSITION+1) + (DEXP_OFFSET-2) + expon;
	if (shift < 0) {	/* Too small for something in the MS word */
	  first = sign;
	  shift += 32;
	  if (shift < 0) {	/* Way too small: flush to zero */
	    second = 0;
	  }
	  else {			/* Pretty small demorn */
	    second = FloatToUnsigned(floor(ldexp(fMant, shift)));
	  }
	}
	else {			/* Nonzero denormalized number */
	  fsMant = ldexp(fMant, shift);
	  mantissa = (long)floor(fsMant);
	  first = sign | mantissa;
	  second = FloatToUnsigned(floor(ldexp(fsMant - mantissa, 32)));
	}
      }

      else {				/* Normalized number */
	fsMant = ldexp(fMant, DEXP_POSITION+1);
	mantissa = (long)floor(fsMant);
	mantissa -= (1L << DEXP_POSITION);			/* Hide MSB */
	fsMant -= (1L << DEXP_POSITION);
	first = sign | ((long)((expon + DEXP_OFFSET - 1)) << DEXP_POSITION) | mantissa;
	second = FloatToUnsigned(floor(ldexp(fsMant - mantissa, 32)));
      }
    }
  }
	
  bytes[0] = first >> 24;
  bytes[1] = first >> 16;
  bytes[2] = first >> 8;
  bytes[3] = first;
  bytes[4] = second >> 24;
  bytes[5] = second >> 16;
  bytes[6] = second >> 8;
  bytes[7] = second;
}

#endif // BINIO_WITH_MATH

unsigned long binostream::writeString(const char *str, unsigned long amount)
{
  unsigned int i;

  if(!amount) amount = strlen(str);

  for(i = 0; i < amount; i++) {
    putByte(str[i]);
    if(err) return i;
  }

  return amount;
}

#if BINIO_ENABLE_STRING
unsigned long binostream::writeString(const std::string &str)
{
  return writeString(str.c_str());
}
#endif
