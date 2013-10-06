/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  metadata string cache/database

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct metacache_str_s {
    struct metacache_str_s *next;
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

uint32_t
metacache_get_hash_sdbm (const char *str) {
    uint32_t hash = 0;
    int c;

    while (c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}

metacache_str_t *
metacache_find_in_bucket (uint32_t h, const char *str) {
    metacache_hash_t *bucket = &hash[h];
    metacache_str_t *chain = bucket->chain;
    while (chain) {
        if (!strcmp (chain->str, str)) {
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
metacache_add_string (const char *str) {
//    printf ("n_strings=%d, n_inserts=%d, n_buckets=%d\n", n_strings, n_inserts, n_buckets);
    uint32_t h = metacache_get_hash_sdbm (str);
    metacache_str_t *data = metacache_find_in_bucket (h % HASH_SIZE, str);
    n_inserts++;
    if (data) {
        data->refcount++;
        return data->str;
    }
    metacache_hash_t *bucket = &hash[h % HASH_SIZE];
    if (!bucket->chain) {
        n_buckets++;
    }
    size_t len = strlen (str);
    data = malloc (sizeof (metacache_str_t) + len);
    memset (data, 0, sizeof (metacache_str_t) + len);
    data->refcount = 1;
    memcpy (data->str, str, len+1);
    data->next = bucket->chain;
    bucket->chain = data;
    n_strings++;
    return data->str;
}

void
metacache_remove_string (const char *str) {
    uint32_t h = metacache_get_hash_sdbm (str);
    metacache_hash_t *bucket = &hash[h % HASH_SIZE];
    metacache_str_t *chain = bucket->chain;
    metacache_str_t *prev = NULL;
    while (chain) {
        if (!strcmp (chain->str, str)) {
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
metacache_ref (const char *str) {
    uint32_t *refc = (uint32_t *)(str-5);
    *refc++;
}

void
metacache_unref (const char *str) {
    uint32_t *refc = (uint32_t *)(str-5);
    *refc--;
}
