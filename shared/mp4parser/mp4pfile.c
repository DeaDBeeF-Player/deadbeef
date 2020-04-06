//
//  mp4file.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/6/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "mp4pfile.h"

#ifndef __linux__
#define O_LARGEFILE 0
#endif

static ssize_t
_file_read (mp4p_file_callbacks_t *stream, void *ptr, size_t size) {
    return read (stream->handle, ptr, size);
}

static ssize_t
_file_write (mp4p_file_callbacks_t *stream, void *ptr, size_t size) {
    return write (stream->handle, ptr, size);
}

static off_t
_file_seek (mp4p_file_callbacks_t *stream, off_t offset, int whence) {
    return lseek (stream->handle, offset, whence);
}

static int64_t
_file_tell (mp4p_file_callbacks_t *stream) {
    return lseek(stream->handle, 0, SEEK_CUR);
}

static int
_file_truncate (mp4p_file_callbacks_t *stream, off_t length) {
    return ftruncate (stream->handle, length);
}

static void
_init_file_callbacks (mp4p_file_callbacks_t *file) {
    file->read = _file_read;
    file->write = _file_write;
    file->seek = _file_seek;
    file->tell = _file_tell;
    file->truncate = _file_truncate;
}

mp4p_file_callbacks_t *
mp4p_file_open_read (const char *fname) {
    int fd = open (fname, O_RDONLY|O_LARGEFILE);
    if (fd < 0) {
        return NULL;
    }

    mp4p_file_callbacks_t *file = calloc (1, sizeof (mp4p_file_callbacks_t));
    file->handle = fd;
    _init_file_callbacks(file);
    return file;
}

mp4p_file_callbacks_t *
mp4p_file_open_readwrite (const char *fname) {
    int fd = open (fname, O_RDWR|O_LARGEFILE);
    if (fd < 0) {
        return NULL;
    }

    mp4p_file_callbacks_t *file = calloc (1, sizeof (mp4p_file_callbacks_t));
    file->handle = fd;
    _init_file_callbacks(file);
    return file;
}

int
mp4p_file_close (mp4p_file_callbacks_t *file) {
    int res = close (file->handle);
    free (file);
    return res;
}

