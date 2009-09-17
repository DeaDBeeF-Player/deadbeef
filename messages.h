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
#ifndef __MESSAGES_H
#define __MESSAGES_H

enum {
    M_SONGFINISHED,
    M_NEXTSONG,
    M_PREVSONG,
    M_PLAYSONG,
    M_PLAYSONGNUM,
    M_STOPSONG,
    M_PAUSESONG,
    M_PLAYRANDOM,
    M_SONGCHANGED, // p1=from, p2=to
    M_ADDDIR, // ctx = pointer to string, which must be freed by f_free
    M_ADDFILES, // ctx = GSList pointer, must be freed with g_slist_free
    M_ADDDIRS, // ctx = GSList pointer, must be freed with g_slist_free
    M_OPENFILES, // ctx = GSList pointer, must be freed with g_slist_free
    M_FMDRAGDROP, // ctx = char* ptr, must be freed with standard free, p1 is length of data, p2 is drop_y
    M_TERMINATE, // must be sent to player thread to terminate
    M_PLAYLISTREFRESH,
    M_REINIT_SOUND,
};

#endif // __MESSAGES_H
