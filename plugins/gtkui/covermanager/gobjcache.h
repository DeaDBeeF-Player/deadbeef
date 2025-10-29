/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#ifndef gobjcache_h
#define gobjcache_h

#include <gtk/gtk.h>

struct gobj_cache_impl_s;
typedef struct gobj_cache_impl_s *gobj_cache_t;
guint
gobj_get_refc (gpointer ptr);

void
gobj_ref (gpointer obj);

void
gobj_unref (gpointer obj);

gobj_cache_t
gobj_cache_new (int max_object_count);

void
gobj_cache_free (gobj_cache_t cache);

void
gobj_cache_set (gobj_cache_t cache, const char *key, GObject *obj);

GObject *
gobj_cache_get (gobj_cache_t cache, const char *key);

void
gobj_cache_set_should_wait (gobj_cache_t cache, const char *key, gboolean should_wait);

gboolean
gobj_cache_get_should_wait (gobj_cache_t cache, const char *key);

void
gobj_cache_remove (gobj_cache_t cache, const char *key);

void
gobj_cache_remove_all (gobj_cache_t cache);

#endif /* gobjcache_h */
