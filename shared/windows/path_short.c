#include <string.h>
#include "junk_iconv2.h"

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
    wchar_t in_w[strlen(in_c) * 2 + 2];
    int iconv_ret = junk_iconv2 (in_c, strlen(in_c) + 1, (char *) in_w, strlen(in_c) * 2 + 2, "UTF-8", "WCHAR_T");
    // convert path to shortpath
    wchar_t out_w[strlen(in_c)];
    int win_ret = GetShortPathNameW (in_w, out_w, 256);
    // convert to UTF-8 :D
    int iconv2_ret = junk_iconv2 ((char *) out_w, wcslen(out_w)*2 + 2, (char *) path_out, len, "WCHAR_T", "UTF-8");
    return iconv2_ret;
}

char *argv0_windows (char * argv[]) {
	wchar_t argv_win[PATH_MAX];
    GetModuleFileNameW(NULL,argv_win,PATH_MAX);
    int argv_win_len = wcslen(argv_win);
    // feel sorry, this boy is not getting freed :(
    char * argv0 = malloc (argv_win_len*2+2);
    if (argv0) {
	    int ret = junk_iconv2 ((char *) argv_win, (wcslen(argv_win)+1)*2, argv0, argv_win_len*2+2, "WCHAR_T", "UTF-8");
	    if (ret == -1) {
	    	return argv[0];
	    }
    	path_short (argv0, argv0, argv_win_len*2+2);
    }
    return argv0;
}
