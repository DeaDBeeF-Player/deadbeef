#include <stdio.h>
#include <dlfcn.h>
#include "junk_iconv2.h"

char dlopened = 0;
void *dlhandle =0;
FILE *(*fopen_sys)(const char*, const char *);

FILE * fopen (const char *filename, const char *mode) {
    char filename_c[strlen(filename)+1];
    strcpy(filename_c, filename);
    // convert any '/' characters to '\\'
    {
        char * slash;
        while (slash = strrchr(filename_c, '/'))
            *slash = '\\';
    }
    // convert filename to wchar_t
    wchar_t filename_w[strlen(filename_c)*2+2];
    wchar_t mode_w[strlen(mode)*2+2];
    int ret = junk_iconv2 (filename_c, strlen(filename_c)+1, (char *) filename_w, strlen(filename_c)*2+2, "UTF-8", "WCHAR_T");
    int ret2 = junk_iconv2 (mode, strlen(mode)+1, (char *) mode_w, strlen(mode)*2+2, "UTF-8", "WCHAR_T");
    if (!dlopened) {
        dlhandle = dlopen ("msvcrt.dll", RTLD_LAZY);
        dlopened = 1;
        fopen_sys = (FILE *(*)(const char*, const char*))(dlsym (dlhandle, "fopen"));
    }
    if (ret == -1 || ret2 == -1) {
        if (fopen_sys)
            return (fopen_sys) (filename_c, mode);
        return NULL;
    }
    else {
        FILE *file = _wfopen (filename_w, mode_w);
        if (!file) {
            if (fopen_sys)
                return (fopen_sys) (filename_c, mode);
            return NULL;
        }
        else
            return file;
    }
}
