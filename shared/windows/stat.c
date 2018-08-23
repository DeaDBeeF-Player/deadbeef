#include <wchar.h>
#include "utils.h"

int stat_windows (const char *path, struct stat *buffer) {
    wchar_t path_w[strlen(path) * 2 + 2];
    int iconv_ret = win_charset_conv (path, strlen(path) + 1, (char *) path_w, strlen(path) * 2 + 2, "UTF-8", "WCHAR_T");
    return _wstat (path_w, (struct _stat64i32 *) buffer);
}
