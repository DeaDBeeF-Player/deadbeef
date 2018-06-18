
#ifndef _MINGW32_LAYER_H_
#define _MINGW32_LAYER_H_

#include <errno.h>
#include <dirent.h>
#include <malloc.h>
#include <windows.h>
#include <stdio.h>
// min and max are defined in windows.h, but source files define them too
// undefine them to avoid redefinition warnings
#undef max
#undef min

#define mkdir(X,Y)    mkdir(X)
#define realpath(X,Y) _fullpath(Y,X,PATH_MAX)
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

#ifndef _POSIX_ARG_MAX
#define _POSIX_ARG_MAX 255
#endif

#ifndef NAME_MAX
#define NAME_MAX FILENAME_MAX
#endif

#ifndef fnmatch
#define  fnmatch(x,y,z) PathMatchSpec(y,x)
#endif

//int rename(const char * oldfile, const char * newfile) __attribute__((weak));
#undef rename
#define rename(X,Y) rename_windows(X,Y)

#include <pthread.h>
typedef pthread_t       db_thread_t;
typedef pthread_mutex_t *db_mutex_t;
typedef pthread_cond_t  *db_cond_t;

#define S_ISLNK(X) 0
/*
  _In_opt_   LPCTSTR lpBackupFileName,
  _In_       DWORD   dwReplaceFlags,
  _Reserved_ LPVOID  lpExclude,
  _Reserved_ LPVOID  lpReserved
);*/

#ifndef posix_memalign
#define posix_memalign(X, Y, Z) ({*X = __mingw_aligned_malloc (Z, Y); (*X) ? 1 : 0;})
#endif

int scandir (const char *__dir, struct dirent ***__namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **));
void *mmap(void *, size_t, int, int, int, off_t);
int munmap(void *, size_t);
char *strndup(char *, size_t);
char *strcasestr(const char *, const char *);

int rename_windows(const char *, const char *);
int junk_iconv2 (const char *in, int inlen, char *out, int outlen, const char *cs_in, const char *cs_out);
#define stat(X,Y) stat_windows(X,Y)
int path_short(char * path_in, char * path_out, int len);
char * argv0_windows (char * argv[]);
//FILE * fopen_utf8 (const char *filename, const char *mode);
//fopen = fopen_utf8;
#endif
