/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  playlist metadata management

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
#ifndef __PLMETA_H
#define __PLMETA_H

void
plt_add_meta (playlist_t *it, const char *key, const char *value);

void
plt_append_meta (playlist_t *it, const char *key, const char *value);

void
plt_replace_meta (playlist_t *it, const char *key, const char *value);

void
plt_set_meta_int (playlist_t *it, const char *key, int value);

void
plt_set_meta_float (playlist_t *it, const char *key, float value);

void
plt_delete_meta (playlist_t *it, const char *key);

const char *
plt_find_meta (playlist_t *it, const char *key);

int
plt_find_meta_int (playlist_t *it, const char *key, int def);

float
plt_find_meta_float (playlist_t *it, const char *key, float def);

DB_metaInfo_t *
plt_get_metadata_head (playlist_t *it);

void
plt_delete_metadata (playlist_t *it, DB_metaInfo_t *meta);

void
plt_delete_all_meta (playlist_t *it);

#endif
