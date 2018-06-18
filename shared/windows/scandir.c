#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include "junk_iconv2.h"

#define DIRENT_CHUNK 64
// TODO: no selector or cmp support
int scandir (const char      *dirname_o,
             struct dirent ***namelist_to,
             int            (*selector) (const struct dirent *),
             int            (*cmp) (const struct dirent **, const struct dirent **)) {

    struct dirent ** namelist = malloc (sizeof(struct dirent *) * DIRENT_CHUNK);
    if (!namelist) {
        return -1;
    }


    char dirname[strlen(dirname_o)+1];
    strcpy(dirname, dirname_o);
    // convert any '/' characters to '\\'
    {
        char * slash = dirname;
        while (slash = strchr(slash, '/'))
            *slash = '\\';
    }
    // convert dirname to wchar
    int dirname_w_len = strlen(dirname) * 2 + 8;
    wchar_t dirname_w[dirname_w_len];
    int iconv_ret = junk_iconv2 (dirname, strlen(dirname) + 1, (char *) dirname_w, dirname_w_len, "UTF-8", "WCHAR_T");
    wcscat (dirname_w,L"\\*.*");
    // FindFirstFileW: P:\ATH\*.*

    WIN32_FIND_DATAW fData;
    HANDLE hFind = FindFirstFileW (dirname_w, &fData);
    if (INVALID_HANDLE_VALUE == hFind) {
        return -1;
    }
    else {
        int struct_count = 0, alloc_multiplier = 1;
        // for each file
        while (1) {
                // skip dots
                if (wcscmp(fData.cFileName, L".") != 0 && wcscmp(fData.cFileName, L"..") != 0) {
                    // mem
                    if (struct_count == DIRENT_CHUNK) {
                        namelist = realloc (namelist, sizeof(struct dirent *) * DIRENT_CHUNK * ++alloc_multiplier);
                        if (!namelist)
                            break;
                    }
                    // entry
                    struct dirent * tmp = (struct dirent *) malloc (sizeof(struct dirent));
                    if (!tmp) {
                        break;
                    }
                    size_t len_l = (wcslen (fData.cFileName) + 1) * 2; // 16-bit => 2-byte
                    size_t len_r = len_l * 2;
                    char string_tmp[len_r];
                    int ret = junk_iconv2 ((char *) fData.cFileName, len_l, string_tmp, len_r, "WCHAR_T", "UTF-8");
                    if (ret == -1) {
                        // failed to UTF-8-fy string, abort entry
                        free (tmp);
                        continue;
                    }
                    else {
                        // entry
                        strcpy (tmp->d_name,string_tmp);
                        // no d_type on windows
                        //tmp->d_type = DT_REG; // treat everything as file (TODO)
                        namelist[struct_count++] = tmp;
                    }
                }
                // next file, stop if last
                if (FindNextFileW(hFind, &fData) == 0) {
                    break;
                }
        }
        FindClose(hFind);
        *namelist_to = namelist;
        //printf("scandir: %d\n",struct_count);
        return struct_count;
    }
    return -1;
}
