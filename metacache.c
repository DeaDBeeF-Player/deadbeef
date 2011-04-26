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
