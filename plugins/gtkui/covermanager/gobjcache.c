/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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
*/

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "gobjcache.h"

typedef struct {
    char *key;
    time_t atime;
    GObject *obj;
} gobj_cache_item_t;

typedef struct {
    gobj_cache_item_t *items;
    int count;
} gobj_cache_impl_t;

static void
gobj_cache_item_deinit (gobj_cache_item_t * restrict item) {
    free (item->key);
    item->key = NULL;
    g_object_unref(item->obj);
    item->obj = NULL;
}

gobj_cache_t
gobj_cache_new (int max_object_count) {
    assert (max_object_count);
    gobj_cache_impl_t *impl = calloc (1, sizeof (gobj_cache_impl_t));
    impl->items = calloc (max_object_count, sizeof (gobj_cache_item_t));
    impl->count = max_object_count;
    return impl;
}

void
gobj_cache_free (gobj_cache_t restrict cache) {
    gobj_cache_impl_t *impl = cache;

    for (int i = 0; i < impl->count; i++) {
        if (impl->items[i].key != NULL) {
            gobj_cache_item_deinit (&impl->items[i]);
        }
    }
    free (impl->items);
    impl->items = NULL;

    free (impl);
}

void
gobj_cache_set (gobj_cache_t restrict cache, const char * restrict key, GObject *obj) {
    if (key == NULL || obj == NULL) {
        return;
    }
    gobj_cache_impl_t *impl = cache;
    g_object_ref(obj);

    // find item with that key or empty
    gobj_cache_item_t *reuse = NULL;
    gobj_cache_item_t *evict = NULL;
    for (int i = 0; i < impl->count; i++) {
        gobj_cache_item_t *item = &impl->items[i];
        if (item->key == NULL) {
            if (reuse == NULL) {
                reuse = item;
            }
        }
        else if (!strcmp (item->key, key)) {
            item->atime = time(NULL);
            g_object_unref(item->obj);
            item->obj = obj;
            return;
        }

        if (!evict) {
            evict = item;
        }
        else {
            if (item->atime < evict->atime) {
                evict = item;
            }
        }
    }

    if (!reuse) {
        // evict
        gobj_cache_item_deinit(evict);
        reuse = evict;
        evict = NULL;
    }

    reuse->atime = time(NULL);
    reuse->key = strdup(key);
    reuse->obj = obj;
}

GObject *
gobj_cache_get (gobj_cache_t cache, const char *key) {
    if (key == NULL) {
        return NULL;
    }

    gobj_cache_impl_t *impl = cache;

    for (int i = 0; i < impl->count; i++) {
        gobj_cache_item_t *item = &impl->items[i];
        if (item->key != NULL && !strcmp (item->key, key)) {
            item->atime = time(NULL);
            g_object_ref(item->obj);
            return item->obj;
        }
    }
    return NULL;
}

void
gobj_cache_remove (gobj_cache_t cache, const char *key) {
    if (key == NULL) {
        return;
    }

    gobj_cache_impl_t *impl = cache;

    for (int i = 0; i < impl->count; i++) {
        gobj_cache_item_t *item = &impl->items[i];
        if (item->key != NULL && !strcmp (item->key, key)) {
            gobj_cache_item_deinit(item);
            break;
        }
    }
}

void
gobj_cache_remove_all (gobj_cache_t cache) {
    gobj_cache_impl_t *impl = cache;

    for (int i = 0; i < impl->count; i++) {
        gobj_cache_item_t *item = &impl->items[i];
        gobj_cache_item_deinit(item);
    }
}
