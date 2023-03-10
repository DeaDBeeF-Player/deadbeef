
/*
    shared/windows/rmdir.c
    Copyright (C) 2020 Keith Cancel <admin@keith.pro>

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

#include <windows.h>
#include <wchar.h>

int
win_rmdir (const char *path) {
    // get length includeing NULL terminater
    int utf16_points = MultiByteToWideChar (CP_UTF8, /*Flags*/ 0, path, -1, NULL, 0);
    if (utf16_points < 1) {
        return -1;
    }
    wchar_t wPath[utf16_points];
    memset(wPath, 0, sizeof(wchar_t) * utf16_points);
    if(MultiByteToWideChar (CP_UTF8, /*Flags*/ 0, path, -1, wPath, utf16_points) < 1) {
        return -1;
    }
    return _wrmdir (wPath);
}
