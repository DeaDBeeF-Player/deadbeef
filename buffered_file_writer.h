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

#ifndef buffered_file_writer_h
#define buffered_file_writer_h

#include <stdio.h>

typedef struct _buffered_file_writer_s buffered_file_writer_t;

buffered_file_writer_t *
buffered_file_writer_new (FILE *fp, size_t buffer_size);

void
buffered_file_writer_free (buffered_file_writer_t *writer);

int
buffered_file_writer_write (buffered_file_writer_t *writer, const void *bytes, size_t size);

int
buffered_file_writer_flush (buffered_file_writer_t *writer);

#endif /* buffered_file_writer_h */
