/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  metadata string cache/database

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "metacache.h"

typedef struct metacache_str_s {
    struct metacache_str_s *next;
    size_t value_length;
    uint32_t refcount;
    char cmpidx; // positive means "equals", negative means "notequals"
    char str[1];
} metacache_str_t;

typedef struct {
    //    uint32_t hash;
    metacache_str_t *chain;
} metacache_hash_t;

#define HASH_SIZE 4096

static metacache_hash_t hash[HASH_SIZE];

static uint32_t
metacache_get_hash_sdbm (const char *str, size_t len) {
    uint32_t h = 0;
    int c;

    const char *end = str + len;

    while (str < end) {
        c = *str++;
        h = c + (h << 6) + (h << 16) - h;
    }

    return h;
}

static metacache_str_t *
metacache_find_in_bucket (uint32_t h, const char *value, size_t len) {
    metacache_hash_t *bucket = &hash[h];
    metacache_str_t *chain = bucket->chain;
    while (chain) {
        if (chain->value_length == len && !memcmp (chain->str, value, len)) {
            return chain;
        }
        chain = chain->next;
    }
    return NULL;
}

static int n_strings = 0;
static int n_inserts = 0;
static int n_buckets = 0;

const char *
metacache_add_value (const char *value, size_t len) {
    //    printf ("n_strings=%d, n_inserts=%d, n_buckets=%d\n", n_strings, n_inserts, n_buckets);
    uint32_t h = metacache_get_hash_sdbm (value, len);
    metacache_str_t *data = metacache_find_in_bucket (h & (HASH_SIZE - 1), value, len);
    n_inserts++;
    if (data) {
        data->refcount++;
        return data->str;
    }
    metacache_hash_t *bucket = &hash[h & (HASH_SIZE - 1)];
    if (!bucket->chain) {
        n_buckets++;
    }
    data = malloc (sizeof (metacache_str_t) + len);
    memset (data, 0, sizeof (metacache_str_t) + len);
    data->refcount = 1;
    memcpy (data->str, value, len);
    data->value_length = len;
    data->next = bucket->chain;
    bucket->chain = data;
    n_strings++;
    return data->str;
}

const char *
metacache_add_string (const char *str) {
    return metacache_add_value (str, (int)strlen (str) + 1);
}

void
metacache_remove_value (const char *value, size_t valuesize) {
    uint32_t h = metacache_get_hash_sdbm (value, valuesize);
    metacache_hash_t *bucket = &hash[h & (HASH_SIZE - 1)];
    metacache_str_t *chain = bucket->chain;
    metacache_str_t *prev = NULL;
    while (chain) {
        if (chain->value_length == valuesize && !memcmp (chain->str, value, valuesize)) {
            chain->refcount--;
            if (chain->refcount == 0) {
                if (prev) {
                    prev->next = chain->next;
                }
                else {
                    bucket->chain = chain->next;
                }
                free (chain);
            }
            break;
        }
        prev = chain;
        chain = chain->next;
    }
}

void
metacache_remove_string (const char *str) {
    return metacache_remove_value (str, strlen (str) + 1);
}

// DEPRECATED_113
void
metacache_ref (const char *str) {
    uint32_t *refc = (uint32_t *)(str - 5);
    (*refc)++;
}

// DEPRECATED_113
void
metacache_unref (const char *str) {
    uint32_t *refc = (uint32_t *)(str - 5);
    (*refc)--;
}

const char *
metacache_get_string (const char *str) {
    return metacache_get_value (str, strlen (str) + 1);
}

const char *
metacache_get_value (const char *value, size_t len) {
    uint32_t h = metacache_get_hash_sdbm (value, len);
    metacache_str_t *data = metacache_find_in_bucket (h & (HASH_SIZE - 1), value, len);
    n_inserts++;
    if (data) {
        data->refcount++;
        return data->str;
    }

    return NULL;
}
