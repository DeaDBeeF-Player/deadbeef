/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <string.h>
#include <stdlib.h>
#include "playlist.h"
#include "deadbeef.h"
#include "metacache.h"

#define LOCK {pl_lock();}
#define UNLOCK {pl_unlock();}

void
pl_add_meta (playItem_t *it, const char *key, const char *value) {
    LOCK;
    // check if it's already set
    DB_metaInfo_t *tail = NULL;
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            // duplicate key
            UNLOCK;
            return;
        }
        tail = m;
        m = m->next;
    }
    // add
    char str[256];
    if (!value || !*value) {
        UNLOCK;
        return;
#if 0
        if (!strcasecmp (key, "title")) {
            // cut filename without path and extension
            const char *pext = pl_find_meta (it, ":URI") + strlen (pl_find_meta (it, ":URI")) - 1;
            while (pext >= pl_find_meta (it, ":URI") && *pext != '.') {
                pext--;
            }
            const char *pname = pext;
            while (pname >= pl_find_meta (it, ":URI") && *pname != '/') {
                pname--;
            }
            if (*pname == '/') {
                pname++;
            }
            strncpy (str, pname, pext-pname);
            str[pext-pname] = 0;
            value = str;
        }
        else {
            UNLOCK;
            return;
        }
#endif
    }
    m = malloc (sizeof (DB_metaInfo_t));
    memset (m, 0, sizeof (DB_metaInfo_t));
    m->key = metacache_add_string (key); //key;
    m->value = metacache_add_string (value); //strdup (value);

    if (tail) {
        tail->next = m;
    }
    else {
        it->meta = m;
    }
    UNLOCK;
}

void
pl_append_meta (playItem_t *it, const char *key, const char *value) {
    const char *old = pl_find_meta (it, key);
    size_t newlen = strlen (value);
    if (!old) {
        pl_add_meta (it, key, value);
    }
    else {
        // check for duplicate data
        const char *str = old;
        int len;
        while (str) {
            char *next = strchr (str, '\n');

            if (next) {
                len = next - str;
                next++;
            }
            else {
                len = strlen (str);
            }

            if (len == newlen && !memcmp (str, value, len)) {
                return;
            }

            str = next;
        }
        int sz = strlen (old) + newlen + 2;
        char out[sz];
        snprintf (out, sz, "%s\n%s", old, value);
        pl_replace_meta (it, key, out);
    }
}

void
pl_replace_meta (playItem_t *it, const char *key, const char *value) {
    LOCK;
    // check if it's already set
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            break;
        }
        m = m->next;
    }
    if (m) {
        metacache_remove_string (m->value);
        m->value = metacache_add_string (value);
        UNLOCK;
        return;
    }
    else {
        pl_add_meta (it, key, value);
    }
    UNLOCK;
}

void
pl_set_meta_int (playItem_t *it, const char *key, int value) {
    char s[20];
    snprintf (s, sizeof (s), "%d", value);
    pl_replace_meta (it, key, s);
}

void
pl_set_meta_float (playItem_t *it, const char *key, float value) {
    char s[20];
    snprintf (s, sizeof (s), "%f", value);
    pl_replace_meta (it, key, s);
}

void
pl_delete_meta (playItem_t *it, const char *key) {
    DB_metaInfo_t *prev = NULL;
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            if (prev) {
                prev->next = m->next;
            }
            else {
                it->meta = m->next;
            }
            metacache_remove_string (m->key);
            metacache_remove_string (m->value);
            free (m);
            break;
        }
        prev = m;
        m = m->next;
    }
}

const char *
pl_find_meta (playItem_t *it, const char *key) {
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            return m->value;
        }
        m = m->next;
    }
    return NULL;
}

int
pl_find_meta_int (playItem_t *it, const char *key, int def) {
    const char *val = pl_find_meta (it, key);
    return val ? atoi (val) : def;
}

float
pl_find_meta_float (playItem_t *it, const char *key, float def) {
    const char *val = pl_find_meta (it, key);
    return val ? atof (val) : def;
}

DB_metaInfo_t *
pl_get_metadata_head (playItem_t *it) {
    return it->meta;
}

void
pl_delete_metadata (playItem_t *it, DB_metaInfo_t *meta) {
    DB_metaInfo_t *prev = NULL;
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (m == meta) {
            if (prev) {
                prev->next = m->next;
            }
            else {
                it->meta = m->next;
            }
            metacache_remove_string (m->key);
            metacache_remove_string (m->value);
            free (m);
            break;
        }
        prev = m;
        m = m->next;
    }
}


