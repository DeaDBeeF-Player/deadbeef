/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  playlist metadata management

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/

#include <string.h>
#include <stdlib.h>
#include "playlist.h"
#include <deadbeef/deadbeef.h>
#include "metacache.h"
#include "pltmeta.h"

#define LOCK \
    { pl_lock (); }
#define UNLOCK \
    { pl_unlock (); }

void
plt_add_meta (playlist_t *it, const char *key, const char *value) {
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
    if (!value || !*value) {
        UNLOCK;
        return;
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
plt_append_meta (playlist_t *it, const char *key, const char *value) {
    pl_lock ();
    const char *old = plt_find_meta (it, key);
    size_t newlen = strlen (value);
    if (!old) {
        plt_add_meta (it, key, value);
    }
    else {
        // check for duplicate data
        const char *str = old;
        size_t len;
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
                pl_unlock ();
                return;
            }

            str = next;
        }
        size_t sz = strlen (old) + newlen + 2;
        char out[sz];
        snprintf (out, sz, "%s\n%s", old, value);
        plt_replace_meta (it, key, out);
    }
    pl_unlock ();
}

void
plt_replace_meta (playlist_t *it, const char *key, const char *value) {
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
        plt_add_meta (it, key, value);
    }
    UNLOCK;
}

void
plt_set_meta_int (playlist_t *it, const char *key, int value) {
    char s[20];
    snprintf (s, sizeof (s), "%d", value);
    plt_replace_meta (it, key, s);
}

void
plt_set_meta_float (playlist_t *it, const char *key, float value) {
    char s[20];
    snprintf (s, sizeof (s), "%f", value);
    plt_replace_meta (it, key, s);
}

void
plt_delete_meta (playlist_t *it, const char *key) {
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
plt_find_meta (playlist_t *it, const char *key) {
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
plt_find_meta_int (playlist_t *it, const char *key, int def) {
    pl_lock ();
    const char *val = plt_find_meta (it, key);
    int res = val ? atoi (val) : def;
    pl_unlock ();
    return res;
}

float
plt_find_meta_float (playlist_t *it, const char *key, float def) {
    pl_lock ();
    const char *val = plt_find_meta (it, key);
    float res = val ? (float)atof (val) : def;
    pl_unlock ();
    return res;
}

DB_metaInfo_t *
plt_get_metadata_head (playlist_t *it) {
    return it->meta;
}

void
plt_delete_metadata (playlist_t *it, DB_metaInfo_t *meta) {
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

void
plt_delete_all_meta (playlist_t *it) {
    LOCK;
    DB_metaInfo_t *m = it->meta;
    DB_metaInfo_t *prev = NULL;
    while (m) {
        DB_metaInfo_t *next = m->next;
        if (m->key[0] == ':') {
            prev = m;
        }
        else {
            if (prev) {
                prev->next = next;
            }
            else {
                it->meta = next;
            }
            metacache_remove_string (m->key);
            metacache_remove_string (m->value);
            free (m);
        }
        m = next;
    }
    UNLOCK;
}

int
plt_get_meta (playlist_t *handle, const char *key, char *val, int size) {
    *val = 0;
    LOCK;
    const char *v = plt_find_meta (handle, key);
    if (!v) {
        UNLOCK;
        return 0;
    }
    strncpy (val, v, size);
    UNLOCK;
    return 1;
}
