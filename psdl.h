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
#ifndef __PSDL_H
#define __PSDL_H

int
psdl_init (void);

void
psdl_free (void);

int
psdl_play (void);

int
psdl_stop (void);

int
psdl_ispaused (void);

int
psdl_pause (void);

int
psdl_unpause (void);

void
psdl_set_volume (float vol);

int
psdl_get_rate (void);

#endif // __PSDL_H
