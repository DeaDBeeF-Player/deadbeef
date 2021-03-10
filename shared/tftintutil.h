#ifndef __TFTINTUTIL_H
#define __TFTINTUTIL_H

typedef struct {
    int tint;

    int index;

    uint8_t r, g, b;

    unsigned has_rgb: 1;
    unsigned reset_rgb : 1;
} tint_stop_t;

unsigned
calculate_tint_stops_from_string (const char *inputString, tint_stop_t *tintRanges, unsigned maxRanges, char** plainString);

#endif
