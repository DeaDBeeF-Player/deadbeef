/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

#ifndef __VFS_H
#define __VFS_H

#include "deadbeef.h"

DB_FILE* vfs_fopen (const char *fname);
void vfs_fclose (DB_FILE *f);
size_t vfs_fread (void *ptr, size_t size, size_t nmemb, DB_FILE *stream);
int vfs_fseek (DB_FILE *stream, int64_t offset, int whence);
int64_t vfs_ftell (DB_FILE *stream);
void vfs_rewind (DB_FILE *stream);
int64_t vfs_fgetlength (DB_FILE *stream);
const char *vfs_get_content_type (DB_FILE *stream);

#endif // __VFS_H
