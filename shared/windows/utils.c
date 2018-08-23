#include <string.h>

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
int path_short(char * path_in, char * path_out, int len) {
    // ensure correct slashes
    char in_c[strlen(path_in)+1];
    strcpy (in_c, path_in);
    {
        char * slash = in_c;
        while (slash = strchr(slash, '/'))
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

char *argv0_windows (char * argv[]) {
    wchar_t argv_win[PATH_MAX];
    GetModuleFileNameW(NULL,argv_win,PATH_MAX);
    int argv_win_len = wcslen(argv_win);
    // feel sorry, this boy is not getting freed :(
    char * argv0 = malloc (argv_win_len*2+2);
    if (argv0) {
        int ret = win_charset_conv ((char *) argv_win, (wcslen(argv_win)+1)*2, argv0, argv_win_len*2+2, "WCHAR_T", "UTF-8");
        if (ret == -1) {
            return argv[0];
        }
        path_short (argv0, argv0, argv_win_len*2+2);
    }
    return argv0;
}
