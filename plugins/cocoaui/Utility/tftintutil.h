#ifndef __TFTINTUTIL_H
#define __TFTINTUTIL_H

typedef struct {
    int tint;
    int index;
} tint_stop_t;

unsigned
calculate_tint_stops_from_string (const char *inputString, tint_stop_t *tintRanges, unsigned maxRanges, char** plainString);

#endif
