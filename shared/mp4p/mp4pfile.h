//
//  mp4file.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/6/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#ifndef mp4file_h
#define mp4file_h

#include <stdio.h>

typedef struct mp4p_file_callbacks_s {
    union {
        void *ptrhandle;
        int handle;
    };
    ssize_t (*read) (struct mp4p_file_callbacks_s *stream, void *ptr, size_t size);
    ssize_t (*write) (struct mp4p_file_callbacks_s *stream, void *ptr, size_t size);
    off_t (*seek) (struct mp4p_file_callbacks_s *stream, off_t offset, int whence);
    off_t (*tell) (struct mp4p_file_callbacks_s *stream); // could be implemented via `lseek(fd, 0, SEEK_CUR)`
    int (*truncate) (struct mp4p_file_callbacks_s *stream, off_t length);
} mp4p_file_callbacks_t;

mp4p_file_callbacks_t *
mp4p_file_open_read (const char *fname);

// Use to read and write the file transactionally -- supports reading, writing and resizing
mp4p_file_callbacks_t *
mp4p_file_open_readwrite (const char *fname);

int
mp4p_file_close (mp4p_file_callbacks_t *callbacks);

#endif /* mp4file_h */
