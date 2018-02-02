
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../../deadbeef.h"
extern DB_functions_t *deadbeef;

int scandir (const char      *dirname,
             struct dirent ***namelist_to,
             int            (*selector) (const struct dirent *),
             int            (*cmp) (const struct dirent **, const struct dirent **)) {
    #define DIRENT_CHUNK 64
    struct dirent ** namelist = malloc (sizeof(struct dirent *) * DIRENT_CHUNK);
    *namelist_to = namelist;
    if (!namelist)
        return -1;
    WIN32_FIND_DATAW fData;
    int dirname_w_len = strlen(dirname) * 2 + 8;
    wchar_t dirname_w[dirname_w_len];
    int iconv_ret = deadbeef->junk_iconv (dirname, strlen(dirname) + 1, (char *) dirname_w, dirname_w_len, "UTF-8", "WCHAR_T");
    wcscat (dirname_w,L"\\*.*");
    HANDLE hFind = FindFirstFileW (dirname_w, &fData);
    if (INVALID_HANDLE_VALUE == hFind) {
        return -1;
    }
    else {
        int struct_count = 0, alloc_multiplier = 1;
        while (1) {
                // skip dots
                if (wcscmp(fData.cFileName, L".") != 0 && wcscmp(fData.cFileName, L"..") != 0) {
                    if (struct_count == DIRENT_CHUNK) {
                        namelist = realloc (namelist, sizeof(struct dirent *) * DIRENT_CHUNK * ++alloc_multiplier);
                        if (!namelist)
                            break;
                    }
                    struct dirent * tmp = (struct dirent *) malloc (sizeof(struct dirent));
                    if (!tmp) {
                        break;
                    }
                    size_t len_l = wcslen (fData.cFileName)*2;
                    size_t len_r = len_l * 2;
                    char string_tmp[len_r];
                    int ret = deadbeef->junk_iconv ((char *) fData.cFileName, len_l, string_tmp, len_r, "WCHAR_T", "UTF-8");
                    if (ret == -1) {
                        free (tmp);
                    }
                    else {
                        strcpy (tmp->d_name,string_tmp);
                        namelist[struct_count++] = tmp;
                    }
                }
                if (FindNextFileW(hFind, &fData) == 0) {
                    break;
                }
        }
        FindClose(hFind);
        return struct_count;
    }
    return -1;
}
