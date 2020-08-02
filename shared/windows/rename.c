/*
    shared/windows/rename.c
    Copyright (C) 2018-2020 Jakub Wasylków

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
#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include "utils.h"

int rename_windows(const char * oldfile_c, const char * newfile_c){
    // early stage validation
    if (!oldfile_c || !newfile_c || !oldfile_c[0] || !newfile_c[0]) {
        return -1;
    }
    char oldfile[strlen(oldfile_c)+1];
    strcpy(oldfile, oldfile_c);
    // convert any '/' characters to '\\'
    {
        char * slash = oldfile;
        while (slash = strchr(slash, '/'))
            *slash = '\\';
    }
    char newfile[strlen(newfile_c)+1];
    strcpy(newfile, newfile_c);
    // convert any '/' characters to '\\'
    {
        char * slash = newfile;
        while (slash = strchr(slash, '/'))
            *slash = '\\';
    }

    wchar_t oldfile_w[strlen(oldfile_c) * 2 + 2];
    int iconv_ret = win_charset_conv (oldfile_c, strlen(oldfile_c) + 1, (char *) oldfile_w, strlen(oldfile_c) * 2 + 2, "UTF-8", "WCHAR_T");
    wchar_t newfile_w[strlen(newfile_c) * 2 + 2];
    int iconv2_ret = win_charset_conv (newfile, strlen(newfile) + 1, (char *) newfile_w, strlen(newfile_c) * 2 + 2, "UTF-8", "WCHAR_T");

    struct _stat st;
    if (_wstat (newfile_w, &st)) {
        // not exists
        {
            wchar_t dir[wcslen(newfile_w)+1];
            wcscpy (dir, newfile_w);
            wchar_t * last = wcsrchr (dir, '\\');
            if (!last) {
                // fail?
                return -1;
            }
            *last = 0;
            _wmkdir (dir);
        }
        HANDLE hFile = CreateFileW (newfile_w, (GENERIC_READ | GENERIC_WRITE), 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        }
    }
    int ret = ReplaceFileW (newfile_w, oldfile_w, NULL, 0, /*reserved for future use ...*/0,/*xd*/0);
    if (!ret){
        return -1;
    }
    return 0;
}
