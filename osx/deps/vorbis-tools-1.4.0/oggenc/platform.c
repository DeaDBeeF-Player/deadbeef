/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** Copyright 2000, Michael Smith <msmith@xiph.org>
 **
 ** Portions from Vorbize, (c) Kenneth Arnold <kcarnold-xiph@arnoldnet.net>
 ** and libvorbis examples, (c) Monty <monty@xiph.org>
 **/

/* Platform support routines  - win32, OS/2, unix */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "platform.h"
#include "encode.h"
#include "i18n.h"
#include <stdlib.h>
#include <ctype.h>
#if defined(_WIN32) || defined(__EMX__) || defined(__WATCOMC__)
#include <getopt.h>
#include <fcntl.h>
#include <io.h>
#include <time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER)

void setbinmode(FILE *f)
{
    _setmode( _fileno(f), _O_BINARY );
}
#endif /* win32 */

#ifdef __EMX__
void setbinmode(FILE *f) 
{
            _fsetmode( f, "b");
}
#endif

#if defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__MINGW32__)
void setbinmode(FILE *f)
{
    setmode(fileno(f), O_BINARY);
}
#endif


#if defined(_WIN32) || defined(__EMX__) || defined(__WATCOMC__)
void *timer_start(void)
{
    time_t *start = malloc(sizeof(time_t));
    time(start);
    return (void *)start;
}

double timer_time(void *timer)
{
    time_t now = time(NULL);
    time_t start = *((time_t *)timer);

    if(now-start)
        return (double)(now-start);
    else
        return 1; /* To avoid division by zero later, for very short inputs */
}


void timer_clear(void *timer)
{
    free((time_t *)timer);
}

#else /* unix. Or at least win32 */

#include <sys/time.h>
#include <unistd.h>

void *timer_start(void)
{
    struct timeval *start = malloc(sizeof(struct timeval));
    gettimeofday(start, NULL);
    return (void *)start;
}

double timer_time(void *timer)
{
    struct timeval now;
    struct timeval start = *((struct timeval *)timer);

    gettimeofday(&now, NULL);

    return (double)now.tv_sec - (double)start.tv_sec + 
        ((double)now.tv_usec - (double)start.tv_usec)/1000000.0;
}

void timer_clear(void *timer)
{
    free((time_t *)timer);
}

#endif

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32

#include <direct.h>

#define PATH_SEPS "/\\"
#define mkdir(x,y) _mkdir((x))

/* MSVC does this, borland doesn't? */
#ifndef __BORLANDC__
#define stat _stat
#endif

#else

#define PATH_SEPS "/"

#endif

int create_directories(char *fn, int isutf8)
{
    char *end, *start;
    struct stat statbuf;
    char *segment = malloc(strlen(fn)+1);
#ifdef _WIN32
    wchar_t seg[MAX_PATH+1];
#endif

    start = fn;
#ifdef _WIN32
    if(strlen(fn) >= 3 && isalpha(fn[0]) && fn[1]==':')
        start = start+2;
#endif

    while((end = strpbrk(start+1, PATH_SEPS)) != NULL)
    {
        int rv;
        memcpy(segment, fn, end-fn);
        segment[end-fn] = 0;

#ifdef _WIN32
        if (isutf8) {
            MultiByteToWideChar(CP_UTF8, 0, segment, -1, seg, MAX_PATH+1);
            rv = _wstat(seg,&statbuf);
        } else
#endif
            rv = stat(segment,&statbuf);
        if(rv) {
            if(errno == ENOENT) {
#ifdef _WIN32
                if (isutf8)
                    rv = _wmkdir(seg);
                else
#endif
                    rv = mkdir(segment, 0777);
                if(rv) {
                    fprintf(stderr, _("Couldn't create directory \"%s\": %s\n"),
                            segment, strerror(errno));
                    free(segment);
                    return -1;
                }
            }
            else {
                fprintf(stderr, _("Error checking for existence of directory %s: %s\n"), 
                            segment, strerror(errno));
                free(segment);
                return -1;
            }
        }
#if defined(_WIN32) && !defined(__BORLANDC__)
        else if(!(_S_IFDIR & statbuf.st_mode)) {
#elif defined(__BORLANDC__)
        else if(!(S_IFDIR & statbuf.st_mode)) {
#else
        else if(!S_ISDIR(statbuf.st_mode)) {
#endif
            fprintf(stderr, _("Error: path segment \"%s\" is not a directory\n"),
                    segment);
            free(segment);
            return -1;
        }

        start = end+1;
    }

    free(segment);
    return 0;

}

#ifdef _WIN32

FILE *oggenc_fopen(char *fn, char *mode, int isutf8)
{
    if (isutf8) {
        wchar_t wfn[MAX_PATH+1];
        wchar_t wmode[32];
        MultiByteToWideChar(CP_UTF8, 0, fn, -1, wfn, MAX_PATH+1);
        MultiByteToWideChar(CP_ACP, 0, mode, -1, wmode, 32);
        return _wfopen(wfn, wmode);
    } else
        return fopen(fn, mode);
}

static int
parse_for_utf8(int argc, char **argv)
{
    extern struct option long_options[];
    int ret;
    int option_index = 1;

    while((ret = getopt_long(argc, argv, "A:a:b:B:c:C:d:G:hkl:m:M:n:N:o:P:q:QrR:s:t:vX:",
                    long_options, &option_index)) != -1)
    {
        switch(ret)
        {
            case 0:
                if(!strcmp(long_options[option_index].name, "utf8")) {
                    return 1;
                }
                break;
            default:
                break;
        }
    }

    return 0;
}

typedef WINSHELLAPI LPWSTR *  (APIENTRY *tCommandLineToArgvW)(LPCWSTR lpCmdLine, int*pNumArgs);

void get_args_from_ucs16(int *argc, char ***argv)
{
    OSVERSIONINFO vi;
    int utf8;

    utf8 = parse_for_utf8(*argc, *argv);
    optind = 1; /* Reset getopt_long */

    /* If command line is already UTF-8, don't convert */
    if (utf8)
        return;

    vi.dwOSVersionInfoSize = sizeof(vi);
    GetVersionEx(&vi);

    /* We only do NT4 and more recent.*/
    /* It would be relatively easy to add NT3.5 support. Is anyone still using NT3? */
    /* It would be relatively hard to add 9x support. Fortunately, 9x is
       a lost cause for unicode support anyway. */
    if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT && vi.dwMajorVersion >= 4) {
        const char utf8flag[] = "--utf8";
        int newargc;
        int sizeofargs = 0;
        int a, count;
        char *argptr;
        char **newargv = NULL;
       LPWSTR *ucs16argv = NULL;
        tCommandLineToArgvW pCommandLineToArgvW = NULL;
        HMODULE hLib = NULL;

        hLib = LoadLibrary("shell32.dll");
        if (!hLib)
            goto bail;
        pCommandLineToArgvW = (tCommandLineToArgvW)GetProcAddress(hLib, "CommandLineToArgvW");
        if (!pCommandLineToArgvW)
            goto bail;

        ucs16argv = pCommandLineToArgvW(GetCommandLineW(), &newargc);
        if (!ucs16argv)
            goto bail;

        for (a=0; a<newargc; a++) {
            count = WideCharToMultiByte(CP_UTF8, 0, ucs16argv[a], -1,
                NULL, 0, NULL, NULL);
            if (count == 0)
                goto bail;
            sizeofargs += count;
        }

        sizeofargs += strlen(utf8flag) + 1;

        newargv = malloc(((newargc + 2) * sizeof(char *)) + sizeofargs);
        argptr = (char *)(&newargv[newargc+2]);

        for (a=0; a<newargc; a++) {
            count = WideCharToMultiByte(CP_UTF8, 0, ucs16argv[a], -1,
                argptr, sizeofargs, NULL, NULL);
            if (count == 0)
                goto bail;

            newargv[a] = argptr;
            argptr += count;
            sizeofargs -= count;
        }

        count = strlen(utf8flag) + 1;
        strcpy(argptr, utf8flag);
        newargv[a] = argptr;
        argptr += count;
        sizeofargs -= count;

        newargv[a+1] = NULL;

        *argc = newargc + 1;
        *argv = newargv;

bail:
        if (hLib != NULL)
            FreeLibrary(hLib);
        if (ucs16argv != NULL)
            GlobalFree(ucs16argv);
    }
}

#endif
