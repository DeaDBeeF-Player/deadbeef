/* sc68 platform dependant configuration */

#ifndef _SC68_CONFIG_PLATFORM68_H_
#define _SC68_CONFIG_PLATFORM68_H_

#ifdef HAVE_CONFIG_H

/* Breakpoint instruction */
#ifndef BREAKPOINT68
# define BREAKPOINT68 if (1) { *(int *)1 = 0x12345678; } else
#endif

/* CDECL keyword */
#define CDECL

/* Define if vsprintf() exists */
#if 1
# define HAVE_VSPRINTF 1
#else
# undef HAVE_VSPRINTF
#endif

/* Define if vsnprintf() exists */
#if 1
# define HAVE_VSNPRINTF 1
#else
# undef HAVE_VSNPRINTF
#endif

/* Define if getenv() exists */
#if 1
# define HAVE_GETENV 1
#else
# undef HAVE_GETENV
#endif

/* Define if <zlib.h> exists */
#if 1
# define HAVE_ZLIB_H 1
#else
# undef HAVE_ZLIB_H
#endif

/* Define if <readline/readline.h> exists */
#if 1
# define HAVE_READLINE_READLINE_H 1
#else
# undef HAVE_READLINE_READLINE_H
#endif

/* Define if <readline/history.h> exists */
#if 1
# define HAVE_READLINE_HISTORY_H 1
#else
# undef HAVE_READLINE_HISTORY_H
#endif

#endif /* #ifdef HAVE_CONFIG_H */

/* Fallback */
#ifndef NO_FALLBACK_CONFIG

#ifdef _MSC_VER
# define HAVE_GETENV 1
# define HAVE_ZLIB_H 1
# define BREAKPOINT68 __asm int 3
# define vsnprintf _vsnprintf
# define vsprintf _vsprintf
# ifndef CDECL
#  define CDECL __cdecl
# endif
#endif

#ifndef BREAKPOINT68
# define BREAKPOINT68
#endif

#ifndef CDECL
# define CDECL
#endif

#endif /* #ifndef NO_FALLBACK_CONFIG */

#endif /* #ifndef _SC68_CONFIG_PLATFORM68_H_ */
