/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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

#include <stdlib.h>
#include <string.h>
#include "buffered_file_writer.h"

struct _buffered_file_writer_s {
    FILE *fp;
    char *buffer;
    size_t size;
    size_t written;
};

buffered_file_writer_t *
buffered_file_writer_new (FILE *fp, size_t buffer_size) {
    buffered_file_writer_t *writer = calloc (1, sizeof (buffered_file_writer_t));
    writer->fp = fp;
    writer->size = buffer_size;
    writer->buffer = malloc (buffer_size);
    return writer;
}

void
buffered_file_writer_free (buffered_file_writer_t *writer) {
    free (writer->buffer);
    free (writer);
}

int
buffered_file_writer_write (buffered_file_writer_t *writer, const void *bytes, size_t size) {
    if (writer->fp != NULL) {
        if (size > writer->size - writer->written) {
            int res = buffered_file_writer_flush (writer);
            if (res < 0) {
                return -1;
            }
        }
        if (size >= writer->size) {
            size_t res = fwrite (bytes, 1, size, writer->fp);
            if (res != size) {
                return -1;
            }
            return 0;
        }
    }
    else {
        if (size > writer->size - writer->written) {
            writer->size *= 2;
            writer->buffer = realloc (writer->buffer, writer->size);
        }
    }

    memcpy (writer->buffer + writer->written, bytes, size);
    writer->written += size;

    return 0;
}

int
buffered_file_writer_flush (buffered_file_writer_t *writer) {
    if (writer->written == 0 || writer->fp == NULL) {
        return 0;
    }
    size_t res = fwrite (writer->buffer, 1, writer->written, writer->fp);
    if (res != writer->written) {
        return -1;
    }
    writer->written = 0;
    return 0;
}

void *
buffered_file_writer_get_buffer (buffered_file_writer_t *writer) {
    return writer->buffer;
}

size_t
buffered_file_writer_get_size (buffered_file_writer_t *writer) {
    return writer->written;
}
