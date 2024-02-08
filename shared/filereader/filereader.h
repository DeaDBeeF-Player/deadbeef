/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2024 Oleksiy Yakovenko and other contributors

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
*/

#ifndef filereader_h
#define filereader_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

struct ddb_file_handle_s;
typedef struct ddb_file_handle_s ddb_file_handle_t;

struct ddb_file_handle_s {
    void *impl;
    void (*deinit) (ddb_file_handle_t *handle);
    size_t (*read) (void *ptr, size_t size, size_t nmemb, ddb_file_handle_t *handle);
    int (*seek) (ddb_file_handle_t *handle, int64_t offset, int whence);
    int64_t (*tell) (ddb_file_handle_t *handle);
    void (*rewind) (ddb_file_handle_t *handle);
    int64_t (*getlength) (ddb_file_handle_t *handle);
};

void ddb_file_init_stdio (ddb_file_handle_t *handle, FILE *fp);
void ddb_file_init_buffer (ddb_file_handle_t *handle, const uint8_t *buffer, size_t size);
void ddb_file_deinit (ddb_file_handle_t *handle);

size_t ddb_file_read (void *ptr, size_t size, size_t nmemb, ddb_file_handle_t *handle);
int ddb_file_seek (ddb_file_handle_t *handle, int64_t offset, int whence);
int64_t ddb_file_tell (ddb_file_handle_t *handle);
void ddb_file_rewind (ddb_file_handle_t *handle);
int64_t ddb_file_getlength (ddb_file_handle_t *handle);

#endif /* filereader_h */
