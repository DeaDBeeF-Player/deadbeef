#ifndef __LYRICS_H
#define __LYRICS_H

#include <stddef.h>
#ifdef HAVE_KATE
#include <kate/kate.h>
#endif

typedef struct oe_lyrics_item {
    char *text;
    size_t len;
    double t0;
    double t1;
#ifdef HAVE_KATE
    kate_motion *km;
#endif
} oe_lyrics_item;

typedef struct oe_lyrics {
    size_t count;
    oe_lyrics_item *lyrics;
    int karaoke;
} oe_lyrics;

extern oe_lyrics *load_lyrics(const char *filename);
extern void free_lyrics(oe_lyrics *lyrics);
extern const oe_lyrics_item *get_lyrics(const oe_lyrics *lyrics, double t, size_t *idx);

#endif
