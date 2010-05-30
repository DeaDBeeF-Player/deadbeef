/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __EMIDI_H
#define __EMIDI_H

#ifdef __cplusplus
extern "C" {
#endif

DB_fileinfo_t *emidi_open (void);
int emidi_init (DB_fileinfo_t *_info, DB_playItem_t *it);
void emidi_free (DB_fileinfo_t *);
int emidi_read (DB_fileinfo_t *, char *bytes, int size);
int emidi_seek (DB_fileinfo_t *, float time);
DB_playItem_t *emidi_insert (DB_playItem_t *after, const char *fname);
int emidi_numvoices (DB_fileinfo_t *);
void emidi_mutevoice (DB_fileinfo_t *, int voice, int mute);
int emidi_start (void);
int emidi_stop (void);

#ifdef __cplusplus
}
#endif

#endif

