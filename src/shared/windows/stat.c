/*
    shared/windows/stat.c
    Copyright (C) 2018 Jakub Wasylków

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
#include <wchar.h>
#include "utils.h"

int stat_windows (const char *path, struct stat *buffer) {
    wchar_t path_w[strlen(path) * 2 + 2];
    int iconv_ret = win_charset_conv (path, strlen(path) + 1, (char *) path_w, strlen(path) * 2 + 2, "UTF-8", "WCHAR_T");
    return _wstat (path_w, (struct _stat64i32 *) buffer);
}
