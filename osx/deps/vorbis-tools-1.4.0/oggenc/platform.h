#ifndef __PLATFORM_H
#define __PLATFORM_H

#include <stdio.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef __OS2__
#define INCL_DOS
#define INCL_NOPMAPI
#include <os2.h>
#endif

#if defined(_WIN32) || defined(__OS2__)
#include <malloc.h>

void setbinmode(FILE *);

#define DEFAULT_NAMEFMT_REMOVE "/\\:<>|"
#define DEFAULT_NAMEFMT_REPLACE ""

#else /* Unix, mostly */

#define setbinmode(x) {}
#define DEFAULT_NAMEFMT_REMOVE "/"
#define DEFAULT_NAMEFMT_REPLACE ""

#endif

#ifdef _WIN32

extern FILE *oggenc_fopen(char *fn, char *mode, int isutf8);
extern void get_args_from_ucs16(int *argc, char ***argv);

#else

#define oggenc_fopen(x,y,z) fopen(x,y)
#define get_args_from_ucs16(x,y) { }

#endif

#endif /* __PLATFORM_H */
