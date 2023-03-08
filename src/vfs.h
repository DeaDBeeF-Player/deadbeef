/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  high-level vfs access interface

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

#ifndef __VFS_H
#define __VFS_H

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

DB_FILE* vfs_fopen (const char *fname);
void vfs_set_track (DB_FILE *stream, DB_playItem_t *it);
void vfs_fclose (DB_FILE *f);
size_t vfs_fread (void *ptr, size_t size, size_t nmemb, DB_FILE *stream);
int vfs_fseek (DB_FILE *stream, int64_t offset, int whence);
int64_t vfs_ftell (DB_FILE *stream);
void vfs_rewind (DB_FILE *stream);
int64_t vfs_fgetlength (DB_FILE *stream);
const char *vfs_get_content_type (DB_FILE *stream);
void vfs_fabort (DB_FILE *stream);
uint64_t vfs_get_identifier (DB_FILE *stream);
void vfs_abort_with_identifier (DB_vfs_t *vfs, uint64_t identifier);

#ifdef __cplusplus
}
#endif

#endif // __VFS_H
