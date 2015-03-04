/* Configured types for sc68. */

#ifndef _SC68_CONFIG_TYPE68_H_
#define _SC68_CONFIG_TYPE68_H_

#ifdef HAVE_CONFIG_H

#define SIZEOF_CHAR      1
#define SIZEOF_SHORT     2
#define SIZEOF_INT       4
#define SIZEOF_LONG      8
#define SIZEOF_LONG_LONG 8

#define TYPE_S8    signed char
#define TYPE_U8  unsigned char
#define TYPE_S16   signed short
#define TYPE_U16 unsigned short
#define TYPE_S32   signed int
#define TYPE_U32 unsigned int
#define TYPE_S64   signed long
#define TYPE_U64 unsigned long

#endif /* ifdef HAVE_CONFIG_H */

/* Fallback */
#ifndef NO_FALLBACK_CONFIG

#ifndef SIZEOF_CHAR
# define SIZEOF_CHAR      1
#endif

#ifndef SIZEOF_SHORT
# define SIZEOF_SHORT     2
#endif

#ifndef SIZEOF_INT
# define SIZEOF_INT       4
#endif

#ifndef SIZEOF_LONG
# define SIZEOF_LONG      4
#endif

#ifndef SIZEOF_LONG_LONG
# define  SIZEOF_LONG_LONG 8
#endif

#ifndef TYPE_S8
# define TYPE_S8    signed char
#endif

#ifndef TYPE_U8
# define TYPE_U8  unsigned char
#endif

#ifndef TYPE_S16
# define TYPE_S16   signed short
#endif

#ifndef TYPE_U16
# define TYPE_U16 unsigned short
#endif

#ifndef TYPE_S32
# define TYPE_S32   signed int
#endif

#ifndef TYPE_U32
# define TYPE_U32 unsigned int
#endif

#ifndef TYPE_S64
# ifdef  _MSC_VER
#  define TYPE_S64   signed __int64
# else
#  define TYPE_S64   signed long long
# endif
#endif

#ifndef TYPE_U64
# ifdef  _MSC_VER
#  define TYPE_U64 unsigned __int64
# else
#  define TYPE_U64 unsigned long long
# endif
#endif

#else /* #ifndef NO_FALLBACK_CONFIG */

#if ! defined(TYPE_U8)  || ! defined (TYPE_S8) ||\
    ! defined(TYPE_U16) || ! defined (TYPE_S16) ||\
    ! defined(TYPE_U32) || ! defined (TYPE_S32) ||\
    ! defined(TYPE_U64) || ! defined (TYPE_S64)
# error "Missing integer type configuration"
#endif

#endif /* #ifndef NO_FALLBACK_CONFIG */

#endif /* #ifndef _SC68_CONFIG_TYPE68_H_ */
