#ifndef plmeta_h
#define plmeta_h

#include "playlist.h"

#ifdef __cplusplus
extern "C" {
#endif

void
pl_add_meta_full (playItem_t *it, const char *key, const char *value, int valuesize);

// if it already exists, append new value(s)
// otherwise, call pl_add_meta
void
pl_append_meta (playItem_t *it, const char *key, const char *value);

void
pl_append_meta_full (playItem_t *it, const char *key, const char *value, int valuesize);

// must be used in explicit pl_lock/unlock block
// that makes it possible to avoid copying metadata on every access
// pl_find_meta may return overridden value (where the key is prefixed with '!')
const char *
pl_find_meta (playItem_t *it, const char *key);

const char *
pl_find_meta_raw (playItem_t *it, const char *key);

int
pl_find_meta_int (playItem_t *it, const char *key, int def);

int64_t
pl_find_meta_int64 (playItem_t *it, const char *key, int64_t def);

float
pl_find_meta_float (playItem_t *it, const char *key, float def);

void
pl_replace_meta (playItem_t *it, const char *key, const char *value);

void
pl_set_meta_int (playItem_t *it, const char *key, int value);

void
pl_set_meta_int64 (playItem_t *it, const char *key, int64_t value);

void
pl_set_meta_float (playItem_t *it, const char *key, float value);

void
pl_delete_meta (playItem_t *it, const char *key);

void
pl_delete_all_meta (playItem_t *it);

int
pl_get_meta (playItem_t *it, const char *key, char *val, int size);

int
pl_get_meta_raw (playItem_t *it, const char *key, char *val, int size);

int
pl_meta_exists (playItem_t *it, const char *key);

int
pl_meta_exists_with_override (playItem_t *it, const char *key);

DB_metaInfo_t *
pl_meta_for_key (playItem_t *it, const char *key);

DB_metaInfo_t *
pl_meta_for_key_with_override (playItem_t *it, const char *key);

int
pl_get_meta_with_override (playItem_t *it, const char *key, char *val, size_t size);

const char *
pl_find_meta_with_override (playItem_t *it, const char *key);

void
pl_meta_free_values (DB_metaInfo_t *meta);

void
pl_add_meta_copy (playItem_t *it, DB_metaInfo_t *meta);

#ifdef __cplusplus
}
#endif

#endif /* plmeta_h */
