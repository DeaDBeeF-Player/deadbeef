/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  track metadata management

  Copyright (C) 2009-2013 Alexey Yakovenko

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

  Alexey Yakovenko waker@users.sourceforge.net
*/

#include <string.h>
#include <stdlib.h>
#include "playlist.h"
#include "deadbeef.h"
#include "metacache.h"

#define LOCK {pl_lock();}
#define UNLOCK {pl_unlock();}


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

static ddb_metaValue_t *
_meta_value_alloc (void) {
    return calloc (1, sizeof (ddb_metaValue_t));
}

static void
_meta_free_values (DB_metaInfo_t *meta) {
    ddb_metaValue_t *data = meta->values;
    while (data) {
        metacache_remove_string (data->value);
        ddb_metaValue_t *next = data->next;
        free (data);
        data = next;
    }
    meta->value = NULL;
    meta->values = NULL;
}

static ddb_metaValue_t *
_meta_append_value (DB_metaInfo_t *meta, const char *value, ddb_metaValue_t *tail) {
    ddb_metaValue_t *data = _meta_value_alloc ();
    data->value = metacache_add_string (value);

    if (!tail && meta->values) {
        tail = meta->values;
        while (tail && tail->next) {
            tail = tail->next;
        }
    }

    if (tail) {
        tail->next = data;
    }
    else {
        meta->values = data;
        meta->value = data->value;
    }
    return data;
}

void
pl_add_meta (playItem_t *it, const char *key, const char *value) {
    if (!value || !*value) {
        return;
    }
    LOCK;
    // check if it's already set
    DB_metaInfo_t *normaltail = NULL;
    DB_metaInfo_t *propstart = NULL;
    DB_metaInfo_t *tail = NULL;
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            // duplicate key
            UNLOCK;
            return;
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
    _meta_append_value(m, value, NULL);

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
    UNLOCK;
}

void
pl_append_meta (playItem_t *it, const char *key, const char *value) {
    pl_lock ();
    DB_metaInfo_t *meta = pl_meta_for_key (it, key);

    if (meta && (!strcasecmp (key, "cuesheet") || !strcasecmp (key, "log"))) {
        pl_unlock ();
        return;
    }

    if (!meta) {
        pl_add_meta (it, key, value);
    }
    else {
        ddb_metaValue_t *data = meta->values;
        ddb_metaValue_t *tail = NULL;

        // dupe check, and find tail
        while (data) {
            if (!strcmp (data->value, value)) {
                pl_unlock ();
                return;
            }
            tail = data;
            data = data->next;
        }

        _meta_append_value (meta, value, tail);
    }
    pl_unlock ();
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
        _meta_free_values (m);
        _meta_append_value(m, value, NULL);
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
            _meta_free_values(m);
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
        if (!strcasecmp (key, m->key)) {
            return m->value;
        }
        m = m->next;
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

float
pl_find_meta_float (playItem_t *it, const char *key, float def) {
    pl_lock ();
    const char *val = pl_find_meta (it, key);
    float res = val ? atof (val) : def;
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
            _meta_free_values(m);
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
            _meta_free_values (m);
            free (m);
        }
        m = next;
    }
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
