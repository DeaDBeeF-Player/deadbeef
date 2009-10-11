/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __CONF_H
#define __CONF_H

#include "deadbeef.h"

int
conf_load (void);

int
conf_save (void);

void
conf_free (void);

int
conf_ischanged (void);

void
conf_setchanged (int c);

const char *
conf_get_str (const char *key, const char *def);

float
conf_get_float (const char *key, float def);

int
conf_get_int (const char *key, int def);

void
conf_set_str (const char *key, const char *val);

void
conf_set_int (const char *key, int val);

void
conf_set_float (const char *key, float val);

DB_conf_item_t *
conf_find (const char *group, DB_conf_item_t *prev);

#endif // __CONF_H
