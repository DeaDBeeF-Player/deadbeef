/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  config file manager

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
#ifndef __CONF_H
#define __CONF_H

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

int
conf_load (void);

int
conf_save (void);

void
conf_init (void);

void
conf_free (void);

void
conf_lock (void);

void
conf_unlock (void);

void
conf_get_str (const char *key, const char *def, char *buffer, int buffer_size);

const char *
conf_get_str_fast (const char *key, const char *def);

float
conf_get_float (const char *key, float def);

int
conf_get_int (const char *key, int def);

int64_t
conf_get_int64 (const char *key, int64_t def);

void
conf_set_str (const char *key, const char *val);

void
conf_set_int (const char *key, int val);

void
conf_set_int64 (const char *key, int64_t val);

void
conf_set_float (const char *key, float val);

DB_conf_item_t *
conf_find (const char *group, DB_conf_item_t *prev);

// remove all items starting with key
void
conf_remove_items (const char *key);

void
conf_item_free (DB_conf_item_t *it);

void
conf_enable_saving (int enable);

#ifdef __cplusplus
}
#endif

#endif // __CONF_H
