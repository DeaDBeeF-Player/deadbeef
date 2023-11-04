/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id: shorten.h,v 1.4 2001/12/30 05:12:04 jason Exp $
 */

#ifndef _SHORTEN_H
#define _SHORTEN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <pthread.h>

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#else
#  if SIZEOF_UNSIGNED_LONG == 4
#    define uint32_t unsigned long
#    define int32_t long
#  else
#    define uint32_t unsigned int
#    define int32_t int
#  endif
#  define uint16_t unsigned short
#  define uint8_t unsigned char
#  define int16_t short
#  define int8_t char
#endif

#undef  ulong
#undef  ushort
#undef  uchar
#undef  slong
#undef  sshort
#undef  schar
#undef  uint
#define uint    uint32_t
#define ulong   uint32_t
#define ushort  uint16_t
#define uchar   uint8_t
#define slong   int32_t
#define sshort  int16_t
#define schar   int8_t

#include "shn.h"

extern shn_file *shnfile;

#define MAGIC			"ajkg"
#define FORMAT_VERSION		2
#define MIN_SUPPORTED_VERSION	1
#define MAX_SUPPORTED_VERSION	3
#define MAX_VERSION		7

#define UNDEFINED_UINT		-1
#define DEFAULT_BLOCK_SIZE	256
#define DEFAULT_V0NMEAN	0
#define DEFAULT_V2NMEAN	4
#define DEFAULT_MAXNLPC	0
#define DEFAULT_NCHAN		1
#define DEFAULT_NSKIP		0
#define DEFAULT_NDISCARD	0
#define NBITPERLONG		32
#define DEFAULT_MINSNR          256
#define DEFAULT_MAXRESNSTR	"32.0"
#define DEFAULT_QUANTERROR	0
#define MINBITRATE		2.5

#define MAX_LPC_ORDER	64
#define CHANSIZE	0
#define ENERGYSIZE	3
#define BITSHIFTSIZE	2
#define NWRAP		3

#define FNSIZE		2
#define FN_DIFF0	0
#define FN_DIFF1	1
#define FN_DIFF2	2
#define FN_DIFF3	3
#define FN_QUIT		4
#define FN_BLOCKSIZE	5
#define FN_BITSHIFT	6
#define FN_QLPC		7
#define FN_ZERO		8
#define FN_VERBATIM     9

#define VERBATIM_CKSIZE_SIZE 5	/* a var_put code size */
#define VERBATIM_BYTE_SIZE 8	/* code size 8 on single bytes means
				 * no compression at all */
#define VERBATIM_CHUNK_MAX 256	/* max. size of a FN_VERBATIM chunk */

#define ULONGSIZE	2
#define NSKIPSIZE	1
#define LPCQSIZE	2
#define LPCQUANT	5
#define XBYTESIZE	7

#define TYPESIZE	4
#define TYPE_AU1	0	/* original lossless ulaw                    */
#define TYPE_S8	        1	/* signed 8 bit characters                   */
#define TYPE_U8         2	/* unsigned 8 bit characters                 */
#define TYPE_S16HL	3	/* signed 16 bit shorts: high-low            */
#define TYPE_U16HL	4	/* unsigned 16 bit shorts: high-low          */
#define TYPE_S16LH	5	/* signed 16 bit shorts: low-high            */
#define TYPE_U16LH	6	/* unsigned 16 bit shorts: low-high          */
#define TYPE_ULAW	7	/* lossy ulaw: internal conversion to linear */
#define TYPE_AU2	8	/* new ulaw with zero mapping                */
#define TYPE_AU3	9	/* lossless alaw                             */
#define TYPE_ALAW 	10	/* lossy alaw: internal conversion to linear */
#define TYPE_RIFF_WAVE  11	/* Microsoft .WAV files                      */
#define TYPE_EOF	12
#define TYPE_GENERIC_ULAW 128
#define TYPE_GENERIC_ALAW 129

#define POSITIVE_ULAW_ZERO 0xff
#define NEGATIVE_ULAW_ZERO 0x7f

#ifndef MAX_PATH
#define MAX_PATH 2048
#endif

#ifndef	MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef	MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#if defined(unix) && !defined(linux)
#define labs abs
#endif

#define ROUNDEDSHIFTDOWN(x, n) (((n) == 0) ? (x) : ((x) >> ((n) - 1)) >> 1)

#ifndef M_LN2
#define	M_LN2	0.69314718055994530942
#endif

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

/* BUFSIZ must be a multiple of four to contain a whole number of words */
#ifdef BUFSIZ
#undef BUFSIZ
#endif

#define BUFSIZ 512

#define V2LPCQOFFSET (1 << LPCQUANT);

#define UINT_GET(nbit, shnfile) \
  ((version == 0) ? uvar_get(nbit, shnfile) : ulong_get(shnfile))

#define putc_exit(val, stream)\
{ char rval;\
  if((rval = putc((val), (stream))) != (char) (val))\
    complain("FATALERROR: write failed: putc returns EOF");\
}

extern int getc_exit_val;
#define getc_exit(stream)\
(((getc_exit_val = getc(stream)) == EOF) ? \
  complain("FATALERROR: read failed: getc returns EOF"), 0: getc_exit_val)

/************************/
/* defined in shorten.c */
extern void	init_offset(slong**, int, int, int);
extern int	shorten(FILE*, FILE*, int, char**);

/**************************/
/* defined in Sulawalaw.c */
extern int Sulaw2lineartab[];
#define Sulaw2linear(i) (Sulaw2lineartab[i])
#ifndef Sulaw2linear
extern int	Sulaw2linear(uchar);
#endif
extern uchar	Slinear2ulaw(int);

extern int Salaw2lineartab[];
#define Salaw2linear(i) (Salaw2lineartab[i])
#ifndef Salaw2linear
extern int	Salaw2linear(uchar);
#endif
extern uchar	Slinear2alaw(int);

/**********************/
/* defined in fixio.c */
extern void	init_sizeof_sample(void);
extern void     fwrite_type_init(shn_file*);
extern void     fwrite_type(slong**,int,int,int,shn_file*);
extern void     fwrite_type_quit(shn_file*);
extern void	fix_bitshift(slong*, int, int, int);

/**********************/
/* defined in vario.c */
extern void	var_get_init(shn_file*);
extern slong	uvar_get(int, shn_file*);
extern slong	var_get(int, shn_file*);
extern ulong	ulong_get(shn_file*);
extern void	var_get_quit(shn_file*);

extern int	sizeof_uvar(ulong, int);
extern int	sizeof_var(slong, int);

extern void	mkmasktab(void);
extern ulong	word_get(shn_file*);

/**********************/
/* defined in array.c */
extern void* 	pmalloc(ulong, shn_file*);
extern slong**	long2d(ulong, ulong, shn_file*);

#endif
