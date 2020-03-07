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
    strncat (output, "&", size);

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
    return 0;
}

