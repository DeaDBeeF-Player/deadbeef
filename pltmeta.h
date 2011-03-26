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

#endif
