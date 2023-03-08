#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tftintutil.h"
#include "../utf8.h"

static size_t
get_stop_from_string(const char *string, size_t len, tint_stop_t *stop) {
    const char *p = string;
    const char *end = p + len;
    const char marker_tint[] = "\0331;";
    const char marker_rgb[] = "\0332;";

    if (len < sizeof(marker_tint)+1) {
        return 0;
    }

    if (!strncmp (p, marker_tint, sizeof(marker_tint)-1)) {
        p += sizeof (marker_tint)-1;

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

        stop->tint = atoi (amount);
    }
    else if (!strncmp (p, marker_rgb, sizeof(marker_rgb)-1)) {
        p += sizeof (marker_rgb)-1;
        int r = 0;
        int g = 0;
        int b = 0;
        int sign = 1;
        if (*p == '-') {
            sign = -1;
            p += 1;
        }
        while (p < end && isdigit(*p)) {
            r = r * 10 + (*p)-'0';
            p += 1;
        }
        if (*p != ';') {
            return 0;
        }
        r *= sign;
        p += 1;
        if (*p == '-') {
            sign = -1;
            p += 1;
        }
        while (p < end && isdigit(*p)) {
            g = g * 10 + (*p)-'0';
            p += 1;
        }
        if (*p != ';') {
            return 0;
        }
        g *= sign;
        p += 1;
        if (*p == '-') {
            sign = -1;
            p += 1;
        }
        while (p < end && isdigit(*p)) {
            b = b * 10 + (*p)-'0';
            p += 1;
        }
        if (*p != 'm') {
            return 0;
        }
        p += 1;
        b *= sign;

        if (r < 0 || g < 0 || b < 0) {
            stop->reset_rgb = 1;
        }
        else {
            stop->has_rgb = 1;
            stop->r = r <= 255 ? (uint8_t)r : 255;
            stop->g = g <= 255 ? (uint8_t)g : 255;
            stop->b = b <= 255 ? (uint8_t)b : 255;
        }
    }
    else {
        return 0;
    }

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
    uint8_t currentR = 0;
    uint8_t currentG = 0;
    uint8_t currentB = 0;
    unsigned has_rgb = 0;

    int index = 0;
    int byteindex = 0;
    while (*p) {
        tint_stop_t stop = {0};
        size_t len = get_stop_from_string(p, remaining, &stop);

        if (len != 0) {
            if (numTintStops < maxStops) {
                currentTint += stop.tint;
                if (stop.has_rgb) {
                    has_rgb = 1;
                    currentR = stop.r;
                    currentG = stop.g;
                    currentB = stop.b;
                }
                if (stop.reset_rgb) {
                    has_rgb = 0;
                    currentR = currentG = currentB = 0;
                }
                tintStops[numTintStops].tint = currentTint;
                tintStops[numTintStops].index = index;
                tintStops[numTintStops].byteindex = byteindex;
                tintStops[numTintStops].has_rgb = has_rgb;
                tintStops[numTintStops].r = currentR;
                tintStops[numTintStops].g = currentG;
                tintStops[numTintStops].b = currentB;
                numTintStops += 1;
            }
            p += len;
            remaining -= len;
            continue;
        }
        if (remaining == 0) {
            break;
        }
        uint32_t i = 0;
        u8_nextchar(p, (int32_t *)&i);
        memcpy (out, p, i);
        out += i;
        p += i;
        remaining -= i;
        index++;
        byteindex+=i;
    }
    *out = 0;
    return numTintStops;
}
