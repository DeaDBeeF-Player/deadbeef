/*
    shared/windows/utils.c
    Copyright (C) 2018-2023 Jakub Wasylków

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
#include <string.h>
#include <shlwapi.h>

// backend selection (only one, first recommended)
#define WIN_CHARSET_CONV_WIN
//#define WIN_CHARSET_CONV_ICONV

#ifdef WIN_CHARSET_CONV_WIN
#include <windows.h>
#elif defined(WIN_CHARSET_CONV_ICONV)
#include <iconv.h>
#endif

#ifdef WIN_CHARSET_CONV_WIN
int
win_charset_conv (const void *in, int inlen, void *out, int outlen, const char *cs_in, const char *cs_out) {
    int ret = 0;
    // Only UTF-8 <-> UTF-16 allowed
    if (strncmp(cs_in, "UTF-8", 5) == 0 && strncmp (cs_out, "WCHAR_T", 7) == 0) {
        // MultiByte to WideChar
        ret = MultiByteToWideChar (CP_UTF8, /*FLAGS*/ 0, (char *) in, inlen, (wchar_t *) out, outlen);
    }
    else if (strncmp(cs_out, "UTF-8", 5) == 0 && strncmp (cs_in, "WCHAR_T", 7) == 0) {
        // WideChar to MultiByte
        ret = WideCharToMultiByte (CP_UTF8, /*FLAGS*/ 0, (wchar_t *) in, inlen, (char *) out, outlen, NULL, NULL);
    }
    return ret;
}
#elif defined(WIN_CHARSET_CONV_ICONV)
int
win_charset_conv (const void *in, int inlen, void *out, int outlen, const char *cs_in, const char *cs_out) {
// NOTE: this function must support utf8->utf8 conversion, used for validation
    iconv_t cd = iconv_open (cs_out, cs_in);
    if (cd == (iconv_t)-1) {
        return -1;
    }
#if defined(__linux__) || defined(__OpenBSD__)
    char *pin = (char*)in;
#else
    const char *pin = in;
#endif

    size_t inbytesleft = inlen;
    size_t outbytesleft = outlen;

    char *pout = out;

    size_t res = iconv (cd, (char **) &pin, &inbytesleft, &pout, &outbytesleft);
    int err = errno;
    iconv_close (cd);

    //trace ("iconv -f %s -t %s '%s': returned %d, inbytes %d/%d, outbytes %d/%d, errno=%d\n", cs_in, cs_out, in, (int)res, inlen, (int)inbytesleft, outlen, (int)outbytesleft, err);
    if (res == -1) {
        return -1;
    }
    out[pout-out] = 0;
    //trace ("iconv out: %s (len=%d)\n", out, pout - out);
    return pout - out;
}
#endif

// path_short converts path to DOS path (8.3 naming, ASCII)
int path_short(const char * path_in, char * path_out, int len) {
    // ensure correct slashes
    char in_c[strlen(path_in)+1];
    strcpy (in_c, path_in);
    {
        char * slash = in_c;
        while ((slash = strchr(slash, '/')))
            *slash = '\\';
    }
    // convert to wchar_t
    wchar_t in_w[strlen(in_c) * 2 + 1];
    int iconv_ret = win_charset_conv (in_c, strlen(in_c) + 1, (char *) in_w, strlen(in_c) * 2 + 1, "UTF-8", "WCHAR_T");
    // convert path to shortpath
    wchar_t out_w[strlen(in_c)];
    int win_ret = GetShortPathNameW (in_w, out_w, 256);
    // convert to UTF-8 :D
    int iconv2_ret = win_charset_conv ((char *) out_w, wcslen(out_w)*2 + 2, (char *) path_out, len, "WCHAR_T", "UTF-8");
    return iconv2_ret;
}

unsigned char path_long_last_path_exists;

// path_long expands given path
int path_long(const char * path_in, char * path_out, int len) {
    // ensure correct slashes
    char in_c[strlen(path_in)+1];
    strcpy (in_c, path_in);
    {
        char * slash = in_c;
        while ((slash = strchr(slash, '/')))
            *slash = '\\';
    }
    // convert to wchar_t
    wchar_t in_w[strlen(in_c) * 2 + 1];
    int iconv_ret = win_charset_conv (in_c, strlen(in_c) + 1, (char *) in_w, strlen(in_c) * 2 + 1, "UTF-8", "WCHAR_T");
    // convert path to longpath
    wchar_t out_w[PATH_MAX/*strlen(in_c)*/];
    int win_ret = GetLongPathNameW (in_w, out_w, 256);

    // GetLongPathNameW can fail if path is like "/c/dir/..."" (msys2 style)
    // this is not checked and will later resolve to deadbeef.exe location


    // convert to absolute path
    wchar_t abs_path_w[PATH_MAX*2];
    _wfullpath(abs_path_w, out_w, PATH_MAX*2);


    // convert to UTF-8 :D
    int iconv2_ret = win_charset_conv ((char *) abs_path_w, wcslen(abs_path_w)*2 + 2, (char *) path_out, len, "WCHAR_T", "UTF-8");

    // check if file exists in reality
    if (!PathFileExistsW(abs_path_w)) {
        path_long_last_path_exists = 0;
    }
    else {
        path_long_last_path_exists = 1;
    }
    return iconv2_ret;
}

wchar_t *default_working_dir = NULL;
wchar_t *deadbeef_working_dir = NULL;

void set_deadbeef_working_dir() {
    wchar_t deadbeef_filepath[PATH_MAX];
    GetModuleFileNameW(NULL, deadbeef_filepath, PATH_MAX);
    // remove '\deadbeef.exe' from the end
    wchar_t *ll = wcsrchr (deadbeef_filepath, '\\');
    if (ll) {
        *ll = 0;
    }
    deadbeef_working_dir = wcsdup(deadbeef_filepath);
}

void set_default_working_dir() {
    wchar_t current_dir[PATH_MAX];
    GetCurrentDirectoryW(PATH_MAX, current_dir);
    default_working_dir = wcsdup(current_dir);
}

void startup_fixes() {
    // FIX 1 moved to realpath

    // FIX 2: GTK to disable client-side decorations
    // since gtk 3.24.12 on windows (msys) client-side decorations are not forced, we need to disable them
    putenv ("GTK_CSD=0");

    // FIX 3: set path to certs for curl
    // curl can't find certs by default
    if (getenv("CURL_CA_BUNDLE") == NULL) {
        char capath[PATH_MAX + strlen("CURL_CA_BUNDLE=\\share\\ssl\\certs\\ca-bundle.crt")];
        strcpy (capath,"CURL_CA_BUNDLE=");
        // append deadbeef path location
        {
            size_t deadbeef_working_dir_byte_len = wcslen(deadbeef_working_dir)*2+1;
            size_t ddb_path_utf_byte_len =  wcslen(deadbeef_working_dir)*4+1;
            char deadbeef_filepath[ddb_path_utf_byte_len];
            win_charset_conv(deadbeef_working_dir, deadbeef_working_dir_byte_len, deadbeef_filepath, ddb_path_utf_byte_len, "WCHAR_T", "UTF-8");
            strcat(capath, deadbeef_filepath);
        }
        strcat (capath, "share\\ssl\\certs\\ca-bundle.crt");
        putenv (capath);
    }

    // End of fixes
}

void windows_arg_fix(int *argc, char **argv) {
    wchar_t *wcmd = GetCommandLineW();
    wchar_t **wargv = CommandLineToArgvW(wcmd, argc);
    for(int i = 0; i < *argc; i++) {
        size_t warg_byte_len = wcslen(wargv[i])*2+2;
        size_t arg_byte_len = wcslen(wargv[i])*4+1;
        char *arg_str = malloc(arg_byte_len);
        if (arg_str) {
            win_charset_conv(wargv[i], warg_byte_len, arg_str, arg_byte_len, "WCHAR_T", "UTF-8");
            argv[i] = arg_str;
        }
    }
}

unsigned char first_call = 1;

// realpath windows implementation
char *realpath (const char *path, char *resolved_path) {
    // HACK: save current working dir and deadbeef dir
    // Explaination: working dir has to be deadbeef's dir to enable finding
    //     lib*.dll, current working dir is used only during realpath
    // HACK2: first realpath call uses path_short (8.3 naming)
    // Explaination: first call is for deadbeef directory location, if the
    //     path includes non-ascii characters all plugins will fail to load
    if (first_call) {
        set_default_working_dir();
        set_deadbeef_working_dir();
        startup_fixes();
    }

    char *out_p;

    // documentation states that we have to alloc memory if resolved_path is NULL
    if (!resolved_path) {
        out_p = malloc (PATH_MAX);
    }
    else {
        out_p = resolved_path;
    }
    if (!out_p)
        return NULL;

    int ret;
    // This function is called first in main() to get main dir path.
    // To ensure that plugins will load sucessfully, we need to return DOS path (as it is ASCII-only)
    if (first_call) {
        SetCurrentDirectoryW(deadbeef_working_dir);
    }
    else {
        SetCurrentDirectoryW(default_working_dir);
    }

    // standard realpath implementation

    // convert path to WCHAR_T
    wchar_t wpath[strlen(path)+1];
    win_charset_conv(path, strlen(path)+1, wpath, strlen(path)*2+2, "UTF-8", "WCHAR_T");
    // find resolved path
    wchar_t wresolved_path[PATH_MAX];
    GetFullPathNameW(wpath, PATH_MAX, wresolved_path, NULL);
    // convert resolved path to UTF-8
    ret = win_charset_conv(wresolved_path, wcslen(wresolved_path)*2+2, out_p, PATH_MAX, "WCHAR_T", "UTF-8");

    if (first_call) {
        first_call = 0;
        // HACK for deadbeef.exe realpath: use short path for dlopen compatibility
        path_short(out_p, out_p, strlen(out_p));
    }

    // Always keep deadbeef path as working dir to allow lib*.dll files to be found
    SetCurrentDirectoryW(deadbeef_working_dir);

    // fail if getting resolved path fails or if the path does not exist
    if (ret <= 0 || GetFileAttributesW(wresolved_path) == INVALID_FILE_ATTRIBUTES) {
        // free if we allocated memory
        if (!resolved_path) {
            free (out_p);
        }
        return NULL;
    }

    // replace backslashes with normal slashes
    {
        char *slash_p = out_p;
        while ((slash_p = strchr(slash_p, '\\'))) {
            *slash_p = '/';
        }
    }
    return out_p;
}

int fnmatch(const char *pattern, const char *string, int flags) {
    return !PathMatchSpec(string,pattern);
}
