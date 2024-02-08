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

#include <string.h>
#include "filereader.h"

size_t ddb_file_read (void *ptr, size_t size, size_t nmemb, ddb_file_handle_t *handle) {
    return handle->read (ptr, size, nmemb, handle);
}

int ddb_file_seek (ddb_file_handle_t *handle, int64_t offset, int whence) {
    return handle->seek (handle, offset, whence);
}

int64_t ddb_file_tell (ddb_file_handle_t *handle) {
    return handle->tell (handle);
}

void ddb_file_rewind (ddb_file_handle_t *handle) {
    return handle->rewind (handle);
}

int64_t ddb_file_getlength (ddb_file_handle_t *handle) {
    return handle->getlength (handle);
}

#pragma mark - stdio

static size_t _read_stdio (void *ptr, size_t size, size_t nmemb, ddb_file_handle_t *handle) {
    FILE *fp = handle->impl;
    return fread(ptr, size, nmemb, fp);
}

int _seek_stdio (ddb_file_handle_t *handle, int64_t offset, int whence) {
    FILE *fp = handle->impl;
    return fseek(fp, offset, whence);
}

int64_t _tell_stdio (ddb_file_handle_t *handle) {
    FILE *fp = handle->impl;
    return ftell(fp);
}

void _rewind_stdio (ddb_file_handle_t *handle) {
    FILE *fp = handle->impl;
    rewind (fp);
}

int64_t _getlength_stdio (ddb_file_handle_t *handle) {
    FILE *fp = handle->impl;
    size_t pos = ftell (fp);
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, pos, SEEK_SET);
    return (int64_t)length;
}

void ddb_file_init_stdio (ddb_file_handle_t *handle, FILE *fp) {
    handle->impl = fp;
    handle->read = _read_stdio;
    handle->seek = _seek_stdio;
    handle->tell = _tell_stdio;
    handle->rewind = _rewind_stdio;
    handle->getlength = _getlength_stdio;
}

#pragma mark - buffer

typedef struct {
    uint8_t *buffer;
    size_t size;

    size_t pos;
} buffer_t;

static void
_deinit_buffer (ddb_file_handle_t *handle) {
    free (handle->impl);
}

static size_t _read_buffer (void *ptr, size_t size, size_t nmemb, ddb_file_handle_t *handle) {
    buffer_t *impl = handle->impl;
    size_t bytes = size * nmemb;
    if (impl->pos + bytes > impl->size) {
        return 0;
    }
    memcpy (ptr, impl->buffer + impl->pos, size * nmemb);
    impl->pos += size * nmemb;
    return nmemb;
}

int _seek_buffer (ddb_file_handle_t *handle, int64_t offset, int whence) {
    buffer_t *impl = handle->impl;

    long pos = impl->pos;

    switch (whence) {
    case SEEK_CUR:
        pos += offset;
        break;
    case SEEK_SET:
        pos = offset;
        break;
    case SEEK_END:
        pos = impl->size - offset;
        break;
    }

    if (pos > impl->size || pos < 0) {
        return -1;
    }

    impl->pos = pos;

    return 0;
}

int64_t _tell_buffer (ddb_file_handle_t *handle) {
    buffer_t *impl = handle->impl;
    return impl->pos;
}

void _rewind_buffer (ddb_file_handle_t *handle) {
    buffer_t *impl = handle->impl;
    impl->pos = 0;
}

int64_t _getlength_buffer (ddb_file_handle_t *handle) {
    buffer_t *impl = handle->impl;
    return impl->size;
}

void ddb_file_init_buffer (ddb_file_handle_t *handle, uint8_t *buffer, size_t size) {
    buffer_t *data = calloc (1, sizeof (buffer_t));
    handle->impl = data;
    handle->deinit = _deinit_buffer;
    handle->read = _read_buffer;
    handle->seek = _seek_buffer;
    handle->tell = _tell_buffer;
    handle->rewind = _rewind_buffer;
    handle->getlength = _getlength_buffer;
}

void ddb_file_deinit (ddb_file_handle_t *handle) {
    if (handle->deinit != NULL) {
        handle->deinit (handle);
    }
}

