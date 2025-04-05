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
#ifndef __METACACHE_H
#define __METACACHE_H

#ifdef __cplusplus
extern "C" {
#endif

void
metacache_init (void);

void
metacache_deinit (void);

// Adds a new NULL-terminated string, or finds an existing one
const char *
metacache_add_string (const char *str);

// Returns an existing NULL-terminated string, or NULL if it doesn't exist
const char *
metacache_get_string (const char *str);

// Removes an existing string, ignoring refcount
void
metacache_remove_string (const char *str);

// Adds a new value of specified size, or finds an existing one
const char *
metacache_add_value (const char *value, size_t valuesize);

// Returns an existing value of specified size, or NULL if it doesn't exist
const char *
metacache_get_value (const char *value, size_t valuesize);

// Removes an existing value of specified size, ignoring refcount
void
metacache_remove_value (const char *value, size_t valuesize);

// Increases reference count of the specified value
void
metacache_ref (const char *str);

// Decreases reference count of the specified value
void
metacache_unref (const char *str);

#ifdef __cplusplus
}
#endif

#endif
