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
#ifndef __TAGLIB_H
#define __TAGLIB_H

typedef void * DB_taglib_file_t;

typedef struct DB_taglib_s {
    DB_misc_t misc;
    DB_taglib_file_t (*open) (const char *fname);
    int (*write) (DB_taglib_file_t file);
    void (*close) (DB_taglib_file_t file);
    void (*set_artist) (DB_taglib_file_t file);
    void (*set_album_artist) (DB_taglib_file_t file);
    void (*set_title) (DB_taglib_file_t file);
    void (*set_track_number) (DB_taglib_file_t file);
    void (*set_album) (DB_taglib_file_t file);
    void (*set_genre) (DB_taglib_file_t file);
    void (*set_date) (DB_taglib_file_t file);
    void (*set_performer) (DB_taglib_file_t file);
    void (*set_composer) (DB_taglib_file_t file);
    void (*set_total_tracks) (DB_taglib_file_t file);
    void (*set_disc_number) (DB_taglib_file_t file);
    void (*set_comment) (DB_taglib_file_t file);
} DB_taglib_t;

#endif
