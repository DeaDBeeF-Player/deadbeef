/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  track metadata management

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
#include "plmeta.h"
#include <deadbeef/deadbeef.h>
#include "metacache.h"

#define LOCK {pl_lock();}
#define UNLOCK {pl_unlock();}

DB_metaInfo_t *
pl_meta_for_key_with_override_needs_mutex_lock (playItem_t *it, const char *key, int needs_mutex_lock) {
    if (needs_mutex_lock) {
        pl_ensure_lock ();
    }
    DB_metaInfo_t *m = it->meta;

    // try to find an override
    while (m) {
        if (m->key[0] == '!' && !strcasecmp (key, m->key+1)) {
            return m;
        }
        m = m->next;
    }

    m = it->meta;
    while (m) {
        if (key && !strcasecmp (key, m->key)) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}

DB_metaInfo_t *
pl_meta_for_key_with_override (playItem_t *it, const char *key) {
    return pl_meta_for_key_with_override_needs_mutex_lock(it, key, 1);
}

DB_metaInfo_t *
pl_meta_for_cached_key (playItem_t *it, const char *key, int needs_mutex_lock) {
    if (needs_mutex_lock) {
        pl_ensure_lock ();
    }
    DB_metaInfo_t *m = it->meta;

    m = it->meta;
    while (m) {
        if (key == m->key) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}


DB_metaInfo_t *
pl_meta_for_key (playItem_t *it, const char *key) {
    pl_ensure_lock ();
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}

void
pl_meta_free_values (DB_metaInfo_t *meta) {
    metacache_remove_value (meta->value, meta->valuesize);
    meta->value = NULL;
    meta->valuesize = 0;
}

DB_metaInfo_t *
pl_add_empty_meta_for_key (playItem_t *it, const char *key) {
    // check if it's already set
    DB_metaInfo_t *normaltail = NULL;
    DB_metaInfo_t *propstart = NULL;
    DB_metaInfo_t *tail = NULL;
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            // duplicate key
            return NULL;
        }
        // find end of normal metadata
        if (!normaltail && (!m->next || m->key[0] == ':' || m->key[0] == '_' || m->key[0] == '!')) {
            normaltail = tail;
            propstart = m;
            if (key[0] != ':' && key[0] != '_' && key[0] != '!') {
                break;
            }
        }
        // find end of properties
        tail = m;
        m = m->next;
    }
    // add
    m = calloc (1, sizeof (DB_metaInfo_t));
    m->key = metacache_add_string (key);

    if (key[0] == ':' || key[0] == '_' || key[0] == '!') {
        if (tail) {
            tail->next = m;
        }
        else {
            it->meta = m;
        }
    }
    else {
        m->next = propstart;
        if (normaltail) {
            normaltail->next = m;
        }
        else {
            it->meta = m;
        }
    }

    return m;
}

static char *
_strip_empty (const char *value, int size, int *outsize) {
    char *data = malloc (size);
    if (!data) {
        return NULL;
    }

    *outsize = 0;
    const char *p = value;
    const char *e = value + size;
    char *out = data;
    while (p < e) {
        size_t l = strlen (p) + 1;
        if (l > 1) {
            memcpy (out, p, l);
            out += l;
            *outsize += l;
        }
        p += l;
    }

    return data;
}

static void
_meta_set_value (DB_metaInfo_t *m, const char *value, int size) {
    size_t len = strlen (value) + 1;
    if (len != size) {
        // multivalue -- need to strip empty parts
        char *data = _strip_empty (value, size, &m->valuesize);

        if (m->valuesize > 0) {
            m->value = metacache_add_value (data, m->valuesize);
        }
        else {
            m->value = metacache_add_value ("", 1);
            m->valuesize = 1;
        }
        free (data);
    }
    else {
        m->value = metacache_add_value (value, size);
        m->valuesize = size;
    }
}

void
pl_add_meta_full (playItem_t *it, const char *key, const char *value, int valuesize) {
    if (!value || !*value) {
        return;
    }

    DB_metaInfo_t *meta = pl_add_empty_meta_for_key (it, key);
    if (!meta) {
        return;
    }

    _meta_set_value (meta, value, valuesize);
}

void
pl_add_meta (playItem_t *it, const char *key, const char *value) {
    if (!value || !*value) {
        return;
    }

    pl_add_meta_full (it, key, value, (int)strlen (value) + 1);
}

static char *
_combine_into_unique_multivalue (const char *value1, int size1, const char *value2, int size2, int *outsize) {
    char *buf = NULL;
    size_t buflen = 0;

    const char *v = value2;
    const char *ve = value2 + size2;
    while (v < ve) {
        const char *p = value1;
        const char *e = value1 + size1;
        while (p < e) {
            if (!strcmp (p, v)) {
                // dupe
                break;
            }
            p += strlen (p) + 1;
        }
        size_t len = strlen (v);
        if (p >= e) {
            // append
            if (!buf) {
                buf = malloc (size1 + len + 1);
                buflen = size1;
                memcpy (buf, value1, size1);
            }
            else {
                buf = realloc (buf, buflen + len + 1);
            }
            memcpy (buf + buflen, v, len + 1);
            buflen += len + 1;
        }

        v += len + 1;
    }

    *outsize = (int)buflen;
    return buf;
}


// append zero-divided multivalue data to existing data
// skip duplicates
void
pl_append_meta_full (playItem_t *it, const char *key, const char *value, int size) {
    if (!value || size == 0 || *value == 0) {
        return;
    }
    pl_lock ();
    DB_metaInfo_t *m = pl_meta_for_key (it, key);
    if (!m) {
        m = pl_add_empty_meta_for_key(it, key);
    }

    if (!m->value) {
        _meta_set_value (m, value, size);
        pl_unlock ();
        return;
    }

    int buflen;
    char *buf = _combine_into_unique_multivalue(m->value, m->valuesize, value, size, &buflen);

    if (!buf) {
        pl_unlock ();
        return;
    }

    metacache_remove_value (m->value, m->valuesize);
    m->value = metacache_add_value (buf, buflen);
    m->valuesize = (int)buflen;
    free (buf);
    pl_unlock ();
}

void
pl_append_meta (playItem_t *it, const char *key, const char *value) {
    pl_append_meta_full(it, key, value, (int)strlen (value)+1);
}

void
pl_replace_meta (playItem_t *it, const char *key, const char *value) {
    LOCK;
    // check if it's already set
    DB_metaInfo_t *m = pl_meta_for_key (it, key);

    if (m) {
        pl_meta_free_values (m);
        int l = (int)strlen (value) + 1;
        m->value = metacache_add_value(value, l);
        m->valuesize = l;
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
pl_set_meta_int64 (playItem_t *it, const char *key, int64_t value) {
    char s[20];
    snprintf (s, sizeof (s), "%lld", (long long)value);
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
    pl_lock ();
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
            pl_meta_free_values(m);
            free (m);
            break;
        }
        prev = m;
        m = m->next;
    }
    pl_unlock ();
}

const char *
pl_find_meta (playItem_t *it, const char *key) {
    pl_ensure_lock ();
    DB_metaInfo_t *m = it->meta;

    if (key && key[0] == ':') {
        // try to find an override
        while (m) {
            if (m->key[0] == '!' && !strcasecmp (key+1, m->key+1)) {
                return m->value;
            }
            m = m->next;
        }
    }

    m = it->meta;
    while (m) {
        if (key && !strcasecmp (key, m->key)) {
            return m->value;
        }
        m = m->next;
    }
    return NULL;
}

const char *
pl_find_meta_with_override (playItem_t *it, const char *key) {
    pl_ensure_lock ();
    DB_metaInfo_t *m = pl_meta_for_key_with_override(it, key);
    if (m) {
        return m->value;
    }
    return NULL;
}

const char *
pl_find_meta_raw (playItem_t *it, const char *key) {
    DB_metaInfo_t *m = pl_meta_for_key (it, key);
    return m ? m->value : NULL;
}

int
pl_find_meta_int (playItem_t *it, const char *key, int def) {
    pl_lock ();
    const char *val = pl_find_meta (it, key);
    int res = val ? atoi (val) : def;
    pl_unlock ();
    return res;
}

int64_t
pl_find_meta_int64 (playItem_t *it, const char *key, int64_t def) {
    pl_lock ();
    const char *val = pl_find_meta (it, key);
    int64_t res = val ? atoll (val) : def;
    pl_unlock ();
    return res;
}

float
pl_find_meta_float (playItem_t *it, const char *key, float def) {
    pl_lock ();
    const char *val = pl_find_meta (it, key);
    float res = val ? (float)atof (val) : def;
    pl_unlock ();
    return res;
}

DB_metaInfo_t *
pl_get_metadata_head (playItem_t *it) {
    return it->meta;
}

void
pl_delete_metadata (playItem_t *it, DB_metaInfo_t *meta) {
    pl_lock ();
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
            pl_meta_free_values(m);
            free (m);
            break;
        }
        prev = m;
        m = m->next;
    }
    pl_unlock ();
}

void
pl_delete_all_meta (playItem_t *it) {
    LOCK;
    DB_metaInfo_t *m = it->meta;
    DB_metaInfo_t *prev = NULL;
    while (m) {
        DB_metaInfo_t *next = m->next;
        if (m->key[0] == ':'  || m->key[0] == '_' || m->key[0] == '!') {
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
            pl_meta_free_values (m);
            free (m);
        }
        m = next;
    }

    // delete replaygain fields
    extern const char *ddb_internal_rg_keys[];
    pl_delete_meta(it, ddb_internal_rg_keys[DDB_REPLAYGAIN_ALBUMGAIN]);
    pl_delete_meta(it, ddb_internal_rg_keys[DDB_REPLAYGAIN_ALBUMPEAK]);
    pl_delete_meta(it, ddb_internal_rg_keys[DDB_REPLAYGAIN_TRACKGAIN]);
    pl_delete_meta(it, ddb_internal_rg_keys[DDB_REPLAYGAIN_TRACKPEAK]);

    uint32_t f = pl_get_item_flags (it);
    f &= ~DDB_TAG_MASK;
    pl_set_item_flags (it, f);
    UNLOCK;
}

int
pl_get_meta (playItem_t *it, const char *key, char *val, int size) {
    *val = 0;
    pl_lock ();
    const char *v = pl_find_meta (it, key);
    if (!v) {
        pl_unlock ();
        return 0;
    }
    strncpy (val, v, size);
    pl_unlock ();
    return 1;
}

int
pl_get_meta_with_override (playItem_t *it, const char *key, char *val, size_t size) {
    *val = 0;
    pl_lock ();
    DB_metaInfo_t *meta = pl_meta_for_key_with_override (it, key);
    if (!meta) {
        pl_unlock ();
        return 0;
    }
    strncpy (val, meta->value, size);
    pl_unlock ();
    return 1;
}

int
pl_get_meta_raw (playItem_t *it, const char *key, char *val, int size) {
    *val = 0;
    pl_lock ();
    const char *v = pl_find_meta_raw (it, key);
    if (!v) {
        pl_unlock ();
        return 0;
    }
    strncpy (val, v, size);
    pl_unlock ();
    return 1;
}

int
pl_meta_exists (playItem_t *it, const char *key) {
    pl_lock ();
    const char *v = pl_find_meta (it, key);
    pl_unlock ();
    return v ? 1 : 0;
}

int
pl_meta_exists_with_override (playItem_t *it, const char *key) {
    pl_lock ();
    const char *v = pl_find_meta_with_override (it, key);
    pl_unlock ();
    return v ? 1 : 0;
}

void
pl_add_meta_copy (playItem_t *it, DB_metaInfo_t *meta) {
    DB_metaInfo_t *m = pl_add_empty_meta_for_key(it, meta->key);
    if (!m) {
        return; // dupe
    }

    m->value = metacache_add_value (meta->value, meta->valuesize);
    m->valuesize = meta->valuesize;
}
