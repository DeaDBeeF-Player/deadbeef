/*
    $Id: cddb_regex.h,v 1.14 2007/08/07 03:12:53 jcaratzas Exp $

    Copyright (C) 2003, 2004, 2005 Kris Verbeeck <airborne@advalvas.be>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the
    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA  02111-1307, USA.
*/

#ifndef CDDB_REGEX_H
#define CDDB_REGEX_H 1

#ifdef HAVE_REGEX_H
#ifdef __cplusplus
    extern "C" {
#endif

#include <cddb/cddb_config.h>
#include <stdlib.h>

#ifdef CDDB_NEED_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>          /* need for MacOS X */
#include <regex.h>


extern regex_t *REGEX_TRACK_FRAME_OFFSETS;
extern regex_t *REGEX_TRACK_FRAME_OFFSET;
extern regex_t *REGEX_DISC_LENGTH;
extern regex_t *REGEX_DISC_REVISION;
extern regex_t *REGEX_DISC_TITLE;
extern regex_t *REGEX_DISC_YEAR;
extern regex_t *REGEX_DISC_GENRE;
extern regex_t *REGEX_DISC_EXT;
extern regex_t *REGEX_TRACK_TITLE;
extern regex_t *REGEX_TRACK_EXT;
extern regex_t *REGEX_PLAY_ORDER;
extern regex_t *REGEX_QUERY_MATCH;
extern regex_t *REGEX_SITE;
extern regex_t *REGEX_TEXT_SEARCH;


void cddb_regex_init(void);

void cddb_regex_destroy(void);

int cddb_regex_get_int(const char *s, regmatch_t matches[], int idx);

unsigned long cddb_regex_get_hex(const char *s, regmatch_t matches[], int idx);

double cddb_regex_get_float(const char *s, regmatch_t matches[], int idx);

char *cddb_regex_get_string(const char *s, regmatch_t matches[], int idx);


#ifdef __cplusplus
    }
#endif
#endif /* HAVE_REGEX_H */

#endif /* CDDB_REGEX_H */
