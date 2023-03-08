/*
    shared/windows/mingw32_layer.h
    Copyright (C) 2016 Elio Blanca

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _MINGW32_LAYER_H_
#define _MINGW32_LAYER_H_

#include <errno.h>
#include <dirent.h>
#include <malloc.h>
//#include <windows.h>
#include <stdio.h>
// min and max are defined in windows.h, but source files define them too
// undefine them to avoid redefinition warnings
#undef max
#undef min

// make mkdir and rmdir compatible with windows
#define mkdir(X,Y) win_mkdir(X, Y)
#define rmdir(X)   win_rmdir(X)

// Most of these values come from Elio's port, these values should be checked if they are still needed

#undef  EWOULDBLOCK
#define EWOULDBLOCK   EAGAIN
#define SHUT_WR       1
#define lstat         stat

#define PROT_READ     0x1
#define PROT_WRITE    0x2
/* This flag is only available in WinXP+ */
#ifdef FILE_MAP_EXECUTE
#define PROT_EXEC     0x4
#else
#define PROT_EXEC        0x0
#define FILE_MAP_EXECUTE 0
#endif

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_ANON      MAP_ANONYMOUS
#define MAP_FAILED    ((void *) -1)

////

// used by plugin shellexec / check if needed
#ifndef _POSIX_ARG_MAX
#define _POSIX_ARG_MAX 255
#endif

// todo check
#ifndef NAME_MAX
#define NAME_MAX FILENAME_MAX
#endif

// used by plugin artwork / defined in libwin
#ifndef fnmatch
#include <fnmatch.h>
#endif

// used by plugin converter
# define WEXITSTATUS(x) (x)

// used by shn
/*
 * File type macros.  Note that block devices, sockets and links cannot be
 * distinguished on Windows and the macros S_ISBLK, S_ISSOCK and S_ISLNK are
 * only defined for compatibility.  These macros should always return false
 * on Windows.
 */
#ifndef S_IFMT /* File type mask */
#define S_IFMT _S_IFMT
#endif
#ifndef S_IFSOCK /* Socket */
#define S_IFSOCK 0
#endif
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#define S_ISCHR(mode)  (((mode) & S_IFMT) == S_IFCHR)
#define S_ISBLK(mode)  (((mode) & S_IFMT) == S_IFBLK)

// ?
#include <pthread.h>
typedef pthread_t       db_thread_t;
typedef pthread_mutex_t *db_mutex_t;
typedef pthread_cond_t  *db_cond_t;

// ?
#define S_ISLNK(X) 0
/*
  _In_opt_   LPCTSTR lpBackupFileName,
  _In_       DWORD   dwReplaceFlags,
  _Reserved_ LPVOID  lpExclude,
  _Reserved_ LPVOID  lpReserved
);*/

#ifndef posix_memalign
#define posix_memalign(X, Y, Z) ({*X = __mingw_aligned_malloc (Z, Y); (*X) ? 0 : 1;})
#endif


// Included implementations:

// rename replacement which works with UTF-8 strings and when using as atomic write
#undef rename
#define rename(X,Y) rename_windows(X,Y)
int rename_windows(const char *, const char *);

int win_mkdir (const char *path, mode_t mode);
int win_rmdir (const char *path);

// Own scandir implementation
int scandir (const char *__dir, struct dirent ***__namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **));

// stat implementation which should work with non-ASCII paths
#define stat(X,Y) stat_windows(X,Y)
//int stat_windows (const char *path, struct stat *buffer);

// mmap.c
void *mmap(void *, size_t, int, int, int, off_t);
int munmap(void *, size_t);

// these functions are not implemented in mingw (yet)
// strndup.c
char *strndup(const char *, size_t);
// strcasestr.c
char *strcasestr(const char *, const char *);

////

// Utility functions: TODO DESCRIBE
// utils.c

// convert from UTF-8 and UTF-16 (WCHAR_T) and vice-versa
// works as junk_iconv() and has same argument strucutre
int win_charset_conv (const void *in, int inlen, void *out, int outlen, const char *cs_in, const char *cs_out);

// convert path to DOS path (8.3 naming, ASCII)
int path_short(const char * path_in, char * path_out, int len);

// realpath implementation
char *realpath (const char *path, char *resolved_path);

#endif
