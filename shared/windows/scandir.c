/*
    shared/windows/scandir.c
    Copyright (C) 2018-2020 Jakub Wasylków and other contributors

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
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <windows.h>

//#include "common.h" // for trace_err()

static void set_convert_errno (void) {
    // Is there like a debug assert instead in this project?
    /*if (GetLastError() != ERROR_NO_UNICODE_TRANSLATION) {
        // Sad days =(
        trace_err ("The scandir implementation is using a bad argument for encoding conversion!\n");
    }*/
    _set_errno (EILSEQ);
}

static void free_namelist (struct dirent **names, int count) {
    for (int i = 0; i < count; i++) {
        free (names[i]);
    }
    free (names);
}

int scandir (const char      *dirname_o,
             struct dirent ***namelist_to,
             int            (*selector) (const struct dirent *),
             int            (*cmp) (const struct dirent **, const struct dirent **)) {
    if (dirname_o == NULL) {
        return -1;
    }

    int sErr = errno; // Don't disturb errno unless there is an error.

    // setting arg cchWideChar to zero will return the length in codepoints, and
    // it also includes str terminator in the length
    int utf16_points = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, dirname_o, -1, NULL, 0);
    if (utf16_points < 1) {
        set_convert_errno ();
        return -1;
    }

    // Add 4 for enough room for '\*.*'
    wchar_t dir_wide[utf16_points + 4];
    memset (dir_wide, 0, sizeof(wchar_t) * (utf16_points + 4));
    int ret = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, dirname_o, -1, dir_wide, utf16_points);
    if (ret < 1) {
        // I don't see how this could fail if the first call succeeded, but check anyways.
        set_convert_errno ();
        return -1;
    }

    // Windows can accept forward slashes since like windows 2000. However,
    // converting makes debugging on windows more canonical/idiomatic.
    wchar_t *slash = dir_wide;
    while ((slash = wcschr(slash, '/'))) {
        *slash = '\\';
    }

    // Append `\*.*`, and for such a simple append let's just do it here.
    dir_wide[utf16_points - 1] = '\\';
    dir_wide[utf16_points + 0] = '*';
    dir_wide[utf16_points + 1] = '.';
    dir_wide[utf16_points + 2] = '*';

    WIN32_FIND_DATAW fData = { 0 };
    HANDLE           hFind = FindFirstFileW (dir_wide, &fData);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (GetLastError () == ERROR_FILE_NOT_FOUND) {
            _set_errno (ENOENT);
        }
        else {
            _set_errno (EACCES);
        }
        return -1;
    }

    // Begin searching/scanning the directory
    struct dirent** names    = NULL;
    size_t          capacity = 0;
    int             count    = 0;
    do {
        wchar_t* name = fData.cFileName;
        // Skip the special file names `.` and `..`
        if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
            continue;
        }
        struct dirent tDir = { 0 };
        // cchWideChar can be -1 for null terminated
        int ret = WideCharToMultiByte (CP_UTF8, WC_ERR_INVALID_CHARS, name, -1, tDir.d_name, sizeof(tDir.d_name) - 1, NULL, NULL);
        // Encoding conversion for the unicode string failed
        if (ret < 1) {
            free_namelist (names, count);
            set_convert_errno ();
            FindClose (hFind); // Don't forget to close
            return -1;
        }
        if (selector && !selector (&tDir)) {
            continue;
        }
        //MessageBox(0, tDir.d_name, "Variable", 0);

        struct dirent* cDir = malloc (sizeof(struct dirent));
        if (cDir == NULL) {
            // Free before setting errno since it can set errno
            free_namelist (names, count);
            _set_errno (ENOMEM);
            FindClose (hFind);
            return -1;
        }
        memcpy (cDir, &tDir, sizeof(struct dirent));

        // Check we have room
        if (count == capacity) {
            capacity = capacity == 0 ? 16 : capacity << 1;
            struct dirent** new = realloc (names, capacity * sizeof(struct dirent*));
            if (new == NULL) {
                free (cDir);
                free_namelist (names, count);
                _set_errno (ENOMEM);
                FindClose(hFind);
                return -1;
            }
            names = new;
        }
        names[count] = cDir;
        count++;
    } while (FindNextFileW (hFind, &fData));
    FindClose (hFind);

    // Now sort if we have a comparison callback
    if (cmp != NULL) {
        qsort (names, count, sizeof(struct dirent*), (int (*)(const void*, const void*))cmp);
    }
    if (namelist_to) {
        *namelist_to = names;
    }
    _set_errno (sErr);
    return count;
}
