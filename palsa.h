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
#ifndef __PALSA_H
#define __PALSA_H

int
palsa_init (void);

void
palsa_free (void);

int
palsa_change_rate (int rate);

int
palsa_play (void);

int
palsa_stop (void);

int
palsa_isstopped (void);

int
palsa_ispaused (void);

int
palsa_pause (void);

int
palsa_unpause (void);

int
palsa_get_rate (void);

#endif // __PALSA_H

