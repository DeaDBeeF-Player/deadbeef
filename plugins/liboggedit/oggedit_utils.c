/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library miscellaneous functions

  Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "oggedit_internal.h"

uint8_t *oggedit_vorbis_channel_map(const unsigned channel_count)
{
    unsigned map_size = channel_count * sizeof(uint8_t);
    uint8_t *map = malloc(map_size);
    if (!map)
        return NULL;
    switch(channel_count) {
        case 3:
            return memcpy(map, &((uint8_t[]){0,2,1}), map_size);
        case 5:
            return memcpy(map, &((uint8_t[]){0,2,1,3,4}), map_size);
        case 6:
            return memcpy(map, &((uint8_t[]){0,2,1,4,5,3}), map_size);
        case 7:
            return memcpy(map, &((uint8_t[]){0,2,1,4,5,6,3}), map_size);
        case 8:
            return memcpy(map, &((uint8_t[]){0,2,1,6,7,4,5,3}), map_size);
        default:
            free(map);
            return NULL;
    }
}

// FIXME: oggedit_map_tag expects key to be writable, and uppercases it in-place, returning the result
const char *oggedit_map_tag(char *key, const char *in_or_out)
{
    typedef struct {
        const char *tag;
        const char *meta;
    } key_t;
    const key_t keys[] = {
        /* Permanent named tags in DeaDBeef */
//        {.tag = "ARTIST",         .meta = "artist"},
//        {.tag = "TITLE",          .meta = "title"},
//        {.tag = "ALBUM",          .meta = "album"},
        {.tag = "DATE",           .meta = "year"},
        {.tag = "TRACKNUMBER",    .meta = "track"},
        {.tag = "TRACKTOTAL",     .meta = "numtracks"},
        {.tag = "TOTALTRACKS",    .meta = "numtracks"},
//        {.tag = "GENRE",          .meta = "genre"},
//        {.tag = "COMPOSER",       .meta = "composer"},
        {.tag = "DISCNUMBER",     .meta = "disc"},
        {.tag = "TOTALDISCS",     .meta = "numdiscs"},
        {.tag = "DISCTOTAL",      .meta = "numdiscs"},
//        {.tag = "COMMENT",        .meta = "comment"},
        /* Vorbis standard tags */
//        {.tag = "ARRANGER",       .meta = "Arranger"},
//        {.tag = "AUTHOR",         .meta = "Author"},
//        {.tag = "CONDUCTOR",      .meta = "Conductor"},
//        {.tag = "ENSEMBLE",       .meta = "Ensemble"},
//        {.tag = "LYRICIST",       .meta = "Lyricist"},
//        {.tag = "PERFORMER",      .meta = "Performer"},
//        {.tag = "PUBLISHER",      .meta = "Publisher"},
//        {.tag = "OPUS",           .meta = "Opus"},
//        {.tag = "PART",           .meta = "Part"},
//        {.tag = "PARTNUMBER",     .meta = "Partnumber"},
//        {.tag = "VERSION",        .meta = "Version"},
//        {.tag = "DESCRIPTION",    .meta = "Description"},
//        {.tag = "COPYRIGHT",      .meta = "Copyright"},
//        {.tag = "LICENSE",        .meta = "License"},
//        {.tag = "CONTACT",        .meta = "Contact"},
//        {.tag = "ORGANIZATION",   .meta = "Organization"},
//        {.tag = "LOCATION",       .meta = "Location"},
//        {.tag = "EAN/UPN",        .meta = "EAN/UPN"},
//        {.tag = "ISRC",           .meta = "ISRC"},
//        {.tag = "LABEL",          .meta = "Label"},
//        {.tag = "LABELNO",        .meta = "Labelno"},
//        {.tag = "ENCODER",        .meta = "Encoder"},
//        {.tag = "ENCODED-BY",     .meta = "Encoded-by"},
//        {.tag = "ENCODING",       .meta = "Encoding"},
        /* Other tags */
//        {.tag = "ALBUMARTIST",    .meta = "Albumartist"},
//        {.tag = "ALBUM ARTIST",   .meta = "Album artist"},
//        {.tag = "BAND",           .meta = "Band"},
//        {.tag = "COMPILATION",    .meta = "Compilation"},
//        {.tag = "ENCODED_BY",     .meta = "Encoded_by"},
//        {.tag = "ENCODER_OPTIONS",.meta = "Encoder_options"},
        {.tag = "ORIGINALDATE",   .meta = "original_release_time"},
        {.tag = "ORIGINALYEAR",   .meta = "original_release_year"},
        {.tag = NULL}
    };

    /* Mapping for special Deadbeef internal metadata */
    for (const key_t *match = keys; match->tag; match++)
        if (!strcasecmp(*in_or_out == 't' ? match->tag : match->meta, key))
            return *in_or_out == 't' ? match->meta : match->tag;

    /* Upper-case all Vorbis Comment tag names */
    if (*in_or_out == 'm')
        for (unsigned i = 0; key[i]; i++)
            key[i] = toupper(key[i]);

    return key;
}
