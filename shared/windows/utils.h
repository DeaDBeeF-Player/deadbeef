/*
    shared/windows/utils.h
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

// convert from UTF-8 and UTF-16 (WCHAR_T) and vice-versa
// works as junk_iconv() and has same argument strucutre
int win_charset_conv (const void *in, int inlen, void *out, int outlen, const char *cs_in, const char *cs_out);

// path_short converts path to DOS path (8.3 naming, ASCII)
int path_short(const char * path_in, char * path_out, int len);

// path_long expands given path
int path_long(const char * path_in, char * path_out, int len);