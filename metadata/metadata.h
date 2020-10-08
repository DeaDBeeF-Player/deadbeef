#ifndef metadata_h
#define metadata_h

#include "../deadbeef.h"

typedef struct ddb_keyValue_s {
    struct ddb_keyValue_s *next;
    const char *key;
    const char *value;
    int valuesize;
    int isBinaryValue;
} ddb_keyValue_t;

/// Thread-safe unordered set of key-values
typedef struct {
    ddb_keyValue_t *head;
    uintptr_t mutex;
} ddb_keyValueList_t;

ddb_keyValueList_t *
md_alloc (void);

void
md_init (ddb_keyValueList_t *md);

void
md_free (ddb_keyValueList_t *md);

void
md_lock (ddb_keyValueList_t *md);

void
md_unlock (ddb_keyValueList_t *md);

void
md_add_meta_full (ddb_keyValueList_t *md, const char *key, const char *value, size_t valuesize);

// if it already exists, append new value(s)
// otherwise, call pl_add_meta
void
md_append_meta (ddb_keyValueList_t *md, const char *key, const char *value);

void
md_append_meta_full (ddb_keyValueList_t *md, const char *key, const char *value, size_t valuesize);

// must be used in explicit pl_lock/unlock block
// that makes it possible to avoid copying metadata on every access
// pl_find_meta may return overriden value (where the key is prefixed with '!')
const char *
md_find_meta (ddb_keyValueList_t *md, const char *key);

const char *
md_find_meta_raw (ddb_keyValueList_t *md, const char *key);

int
md_find_meta_int (ddb_keyValueList_t *md, const char *key, int def);

int64_t
md_find_meta_int64 (ddb_keyValueList_t *md, const char *key, int64_t def);

float
md_find_meta_float (ddb_keyValueList_t *md, const char *key, float def);

void
md_replace_meta (ddb_keyValueList_t *md, const char *key, const char *value);

void
md_set_meta_int (ddb_keyValueList_t *md, const char *key, int value);

void
md_set_meta_int64 (ddb_keyValueList_t *md, const char *key, int64_t value);

void
md_set_meta_float (ddb_keyValueList_t *md, const char *key, float value);

void
md_delete_meta (ddb_keyValueList_t *md, const char *key);

void
md_delete_all_meta (ddb_keyValueList_t *md);

int
md_get_meta (ddb_keyValueList_t *md, const char *key, char *val, size_t size);

int
md_get_meta_raw (ddb_keyValueList_t *md, const char *key, char *val, size_t size);

int
md_meta_exists (ddb_keyValueList_t *md, const char *key);

ddb_keyValue_t *
md_meta_for_key (ddb_keyValueList_t *md, const char *key);

void
md_add_meta_copy (ddb_keyValueList_t *md, DB_metaInfo_t *meta);

#endif /* metadata_h */
