#include "shellexecutil.h"
#include <string.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

extern DB_functions_t *deadbeef;

int
shellexec_eval_command (const char *shcommand, char *output, size_t size, DB_playItem_t *it) {
    int res = deadbeef->pl_format_title_escaped (it, -1, output, (int)(size - 2), -1, shcommand);
    if (res < 0) {
        trace ("shellexec: failed to format string for execution (too long?)\n");
        return -1;
    }
    // Run in background
    #ifndef __MINGW32__
    strncat (output, "&", size);
    #else
    {
        const char *cmd_prefix = "start /b \"\" cmd /c \"";
        size_t conv_len = strlen(output) + strlen(cmd_prefix) + 1;
        size_t len = strlen(output);
        if ( conv_len > size) {
            trace ("shellexec: string too long\n");
            return -1;
        }
        memmove (output + strlen(cmd_prefix), output, len);
        output[conv_len] = 0;
        output[conv_len - 1] = '\"';
        memcpy (output, cmd_prefix, strlen(cmd_prefix));
    }
    #endif

    // replace \' with '"'"'
    size_t l = strlen (output);
    size_t remaining = size - l - 1;
    for (int i = 0; output[i]; i++) {
        if (output[i] == '\\' && output[i+1] == '\'') {
            if (remaining < 3) {
                trace ("shellexec command is too long.\n");
                return -1;
            }
            memmove (&output[i+5], &output[i+2], l - i + 1 - 2);
            memcpy (&output[i], "'\"'\"'", 5);
            l += 3;
            remaining -= 3;
            i += 4;
        }
        else if (remaining < 3) {
            fprintf (stderr, "shellexec: command is too long.\n");
            return -1;
        }
    }
    #ifdef __MINGW32__
    // replace ' with "
    // Required for paths to work
    char *str_p = output;
    while(str_p = strchr(str_p, '\'')) {
        *str_p = '\"';
    }
    #endif
    return 0;
}

