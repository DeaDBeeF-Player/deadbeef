#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tftintutil.h"
#include "utf8.h"

static size_t
get_tint_from_string(const char *string, size_t len, int *tint) {
    const char *p = string;
    const char marker[] = "\0331;";

    if (len < sizeof(marker)+1) {
        return 0;
    }

    if (strncmp (p, marker, sizeof(marker)-1)) {
        return 0;
    }

    p += sizeof (marker)-1;

    const char *amount = p;

    if (*p == '-' || *p == '+') {
        p++;
    }

    if (!isdigit (*p)) {
        return 0;
    }

    while (isdigit (*p)) {
        p++;
    }

    if (*p != 'm') {
        return 0;
    }
    p++;

    *tint = atoi (amount);
    return p - string;
}

unsigned
calculate_tint_stops_from_string (const char *inputString, tint_stop_t *tintStops, unsigned maxStops, char** plainString) {
    unsigned numTintStops = 0;

    const char *p = inputString;
    char *out = calloc (strlen(inputString) + 1, 1);
    *plainString = out;
    size_t remaining = strlen (inputString);

    int currentTint = 0;

    int index = 0;
    while (*p) {
        int tint = 0;
        size_t len = get_tint_from_string(p, remaining, &tint);

        if (len != 0) {
            if (numTintStops < maxStops) {
                currentTint += tint;
                tintStops[numTintStops].tint = currentTint;
                tintStops[numTintStops].index = index;
                numTintStops += 1;
            }
            p += len;
            remaining -= len;
        }
        if (remaining == 0) {
            break;
        }
        uint32_t i = 0;
        u8_nextchar(p, &i);
        memcpy (out, p, i);
        out += i;
        p += i;
        remaining -= i;
        index++;
    }
    *out = 0;
    return numTintStops;
}
