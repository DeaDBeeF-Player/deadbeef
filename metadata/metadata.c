#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "metacache.h"
#include "metadata.h"
#include "threading.h"

#pragma mark - Private functions

static char *
_strip_empty (const char *value, size_t size, size_t *outsize) {
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
_meta_set_value (ddb_keyValue_t *m, const char *value, size_t size) {
    size_t len = strlen (value) + 1;
    if (len != size) {
        // multivalue -- need to strip empty parts
        size_t valuesize;
        char *data = _strip_empty (value, size, &valuesize);
        m->valuesize = (int)valuesize;

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
        m->valuesize = (int)size;
    }
}

// NOTE: this has to be different from plmeta, since we don't need overrides here.
static ddb_keyValue_t *
_add_empty_meta_for_key (ddb_keyValueList_t *md, const char *key) {
    // check if it's already set
    ddb_keyValue_t *normaltail = NULL;
    ddb_keyValue_t *propstart = NULL;
    ddb_keyValue_t *tail = NULL;
    ddb_keyValue_t *m = md->head;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            // duplicate key
            return NULL;
        }
        // find end of properties
        tail = m;
        m = m->next;
    }
    // add
    m = calloc (1, sizeof (DB_metaInfo_t));
    m->key = metacache_add_string (key);

    m->next = propstart;
    if (normaltail) {
        normaltail->next = m;
    }
    else {
        md->head = m;
    }

    return m;
}

static char *
_combine_into_unique_multivalue (const char *value1, size_t size1, const char *value2, size_t size2, size_t *outsize) {
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

    *outsize = buflen;
    return buf;
}

static void
_add_meta (ddb_keyValueList_t *md, const char *key, const char *value) {
    if (!value || !*value) {
        return;
    }

    md_add_meta_full (md, key, value, strlen (value) + 1);
}

void
_meta_free_values (ddb_keyValue_t *meta) {
    metacache_remove_value (meta->value, meta->valuesize);
    meta->value = NULL;
    meta->valuesize = 0;
}

#pragma mark - Public functions

#pragma mark Managing

ddb_keyValueList_t *
md_alloc (void) {
    return calloc(1, sizeof(ddb_keyValueList_t));
}

void
md_init (ddb_keyValueList_t *md) {
    md->mutex = mutex_create ();
}

void
md_free (ddb_keyValueList_t *md) {
    md_delete_all_meta(md);
    if (md->mutex) {
        mutex_free (md->mutex);
        md->mutex = 0;
    }
    free (md);
}

#pragma mark Locking

void
md_lock (ddb_keyValueList_t *md) {
    mutex_lock(md->mutex);
}

void
md_unlock (ddb_keyValueList_t *md) {
    mutex_unlock(md->mutex);
}

#pragma mark Metadata queries

void
md_add_meta_full (ddb_keyValueList_t *md, const char *key, const char *value, size_t valuesize) {
    if (!value || !*value) {
        return;
    }

    ddb_keyValue_t *meta = _add_empty_meta_for_key (md, key);
    if (!meta) {
        return;
    }

    _meta_set_value (meta, value, valuesize);
}

// if it already exists, append new value(s)
// otherwise, call md_add_meta
void
md_append_meta (ddb_keyValueList_t *md, const char *key, const char *value) {
    md_append_meta_full(md, key, value, strlen (value)+1);
}

void
md_append_meta_full (ddb_keyValueList_t *md, const char *key, const char *value, size_t valuesize) {
    if (!value || valuesize == 0 || *value == 0) {
        return;
    }
    md_lock (md);
    ddb_keyValue_t *m = md_meta_for_key (md, key);
    if (!m) {
        m = _add_empty_meta_for_key(md, key);
    }

    if (!m->value) {
        _meta_set_value (m, value, valuesize);
        md_unlock (md);
        return;
    }

    size_t buflen;
    char *buf = _combine_into_unique_multivalue(m->value, m->valuesize, value, valuesize, &buflen);

    if (!buf) {
        md_unlock (md);
        return;
    }

    metacache_remove_value (m->value, m->valuesize);
    m->value = metacache_add_value (buf, buflen);
    m->valuesize = (int)buflen;
    free (buf);
    md_unlock (md);
}

// must be used in explicit md_lock/unlock block
// that makes it possible to avoid copying metadata on every access
// md_find_meta may return overriden value (where the key is prefixed with '!')
const char *
md_find_meta (ddb_keyValueList_t *md, const char *key) {
    ddb_keyValue_t *m = md->head;

    while (m) {
        if (key && !strcasecmp (key, m->key)) {
            return m->value;
        }
        m = m->next;
    }
    return NULL;
}

const char *
md_find_meta_raw (ddb_keyValueList_t *md, const char *key) {
    ddb_keyValue_t *m = md_meta_for_key (md, key);
    return m ? m->value : NULL;
}

int
md_find_meta_int (ddb_keyValueList_t *md, const char *key, int def) {
    md_lock (md);
    const char *val = md_find_meta (md, key);
    int res = val ? atoi (val) : def;
    md_unlock (md);
    return res;
}

int64_t
md_find_meta_int64 (ddb_keyValueList_t *md, const char *key, int64_t def) {
    md_lock (md);
    const char *val = md_find_meta (md, key);
    int64_t res = val ? atoll (val) : def;
    md_unlock (md);
    return res;
}

float
md_find_meta_float (ddb_keyValueList_t *md, const char *key, float def) {
    md_lock (md);
    const char *val = md_find_meta (md, key);
    float res = val ? (float)atof (val) : def;
    md_unlock (md);
    return res;
}

void
md_replace_meta (ddb_keyValueList_t *md, const char *key, const char *value) {
    md_lock (md);
    // check if it's already set
    ddb_keyValue_t *m = md_meta_for_key (md, key);

    if (m) {
        _meta_free_values (m);
        int l = (int)strlen (value) + 1;
        m->value = metacache_add_value(value, l);
        m->valuesize = l;
        md_unlock (md);
        return;
    }
    else {
        _add_meta (md, key, value);
    }
    md_unlock (md);
}

void
md_set_meta_int (ddb_keyValueList_t *md, const char *key, int value) {
    char s[20];
    snprintf (s, sizeof (s), "%d", value);
    md_replace_meta (md, key, s);
}

void
md_set_meta_int64 (ddb_keyValueList_t *md, const char *key, int64_t value) {
    char s[20];
    snprintf (s, sizeof (s), "%lld", value);
    md_replace_meta (md, key, s);
}

void
md_set_meta_float (ddb_keyValueList_t *md, const char *key, float value) {
    char s[20];
    snprintf (s, sizeof (s), "%f", value);
    md_replace_meta (md, key, s);
}

void
md_delete_meta (ddb_keyValueList_t *md, const char *key) {
    md_lock (md);
    ddb_keyValue_t *prev = NULL;
    ddb_keyValue_t *m = md->head;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            if (prev) {
                prev->next = m->next;
            }
            else {
                md->head = m->next;
            }
            metacache_remove_string (m->key);
            _meta_free_values(m);
            free (m);
            break;
        }
        prev = m;
        m = m->next;
    }
    md_unlock (md);
}

void
md_delete_all_meta (ddb_keyValueList_t *md) {
    md_lock (md);
    ddb_keyValue_t *m = md->head;
    ddb_keyValue_t *prev = NULL;
    while (m) {
        ddb_keyValue_t *next = m->next;
        if (prev) {
            prev->next = next;
        }
        else {
            md->head = next;
        }
        metacache_remove_string (m->key);
        _meta_free_values (m);
        free (m);
        m = next;
    }

    md_unlock(md);
}

int
md_get_meta (ddb_keyValueList_t *md, const char *key, char *val, size_t size) {
    *val = 0;
    md_lock (md);
    const char *v = md_find_meta (md, key);
    if (!v) {
        md_unlock (md);
        return 0;
    }
    strncpy (val, v, size);
    md_unlock (md);
    return 1;
}

int
md_get_meta_raw (ddb_keyValueList_t *md, const char *key, char *val, size_t size) {
    *val = 0;
    md_lock (md);
    const char *v = md_find_meta_raw (md, key);
    if (!v) {
        md_unlock (md);
        return 0;
    }
    strncpy (val, v, size);
    md_unlock (md);
    return 1;
}

int
md_meta_exists (ddb_keyValueList_t *md, const char *key) {
    md_lock (md);
    const char *v = md_find_meta (md, key);
    md_unlock (md);
    return v ? 1 : 0;
}

ddb_keyValue_t *
md_meta_for_key (ddb_keyValueList_t *md, const char *key) {
    ddb_keyValue_t *m = md->head;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}

void
md_add_meta_copy (ddb_keyValueList_t *md, DB_metaInfo_t *meta) {
    ddb_keyValue_t *m = _add_empty_meta_for_key (md, meta->key);
    if (!m) {
        return; // dupe
    }

    m->value = metacache_add_value (meta->value, meta->valuesize);
    m->valuesize = meta->valuesize;
}
