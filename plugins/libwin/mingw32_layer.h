
#ifndef _MINGW32_LAYER_H_
#define _MINGW32_LAYER_H_

#include <errno.h>
#include <dirent.h>
#include <malloc.h>


#define mkdir(X,Y)    mkdir(X)
#define realpath(X,Y) _fullpath(Y,X,PATH_MAX)
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

int scandir (const char *__dir, struct dirent ***__namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **));
void *mmap(void *, size_t, int, int, int, off_t);
int munmap(void *, size_t);
char *strndup(char *, size_t);
char *strcasestr(const char *, const char *);

#endif
