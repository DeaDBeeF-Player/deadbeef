/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * Basic stream reading
 *
 * http://crazney.net/programs/itunes/alac.html
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifdef _WIN32
	#include "stdint_win.h"
#else
	#include <stdint.h>
#endif

#include "stream.h"

extern DB_functions_t *deadbeef;

#define _Swap32(v) do { \
                   v = (((v) & 0x000000FF) << 0x18) | \
                       (((v) & 0x0000FF00) << 0x08) | \
                       (((v) & 0x00FF0000) >> 0x08) | \
                       (((v) & 0xFF000000) >> 0x18); } while(0)

#define _Swap16(v) do { \
                   v = (((v) & 0x00FF) << 0x08) | \
                       (((v) & 0xFF00) >> 0x08); } while (0)

extern int host_bigendian;

struct stream_tTAG {
    DB_FILE *f;
    int bigendian;
    int eof;
    int64_t junk_offset;
};

int32_t stream_read(stream_t *stream, size_t size, void *buf)
{
    size_t ret = deadbeef->fread (buf, 1, size, stream->f);
    if (ret == 0 && size != 0) stream->eof = 1;
    return (int32_t)ret;
}

int32_t stream_read_int32(stream_t *stream)
{
    int32_t v;
    stream_read(stream, 4, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap32(v);
    }
    return v;
}

uint32_t stream_read_uint32(stream_t *stream)
{
    uint32_t v;
    stream_read(stream, 4, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap32(v);
    }
    return v;
}

int16_t stream_read_int16(stream_t *stream)
{
    int16_t v;
    stream_read(stream, 2, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap16(v);
    }
    return v;
}

uint16_t stream_read_uint16(stream_t *stream)
{
    uint16_t v;
    stream_read(stream, 2, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap16(v);
    }
    return v;
}

int8_t stream_read_int8(stream_t *stream)
{
    int8_t v;
    stream_read(stream, 1, &v);
    return v;
}

uint8_t stream_read_uint8(stream_t *stream)
{
    uint8_t v;
    stream_read(stream, 1, &v);
    return v;
}


void stream_skip(stream_t *stream, int64_t skip)
{
    if (deadbeef->fseek(stream->f, skip, SEEK_CUR) == 0) return;
    if (errno == ESPIPE)
    {
        char *buffer = malloc(skip);
        stream_read(stream, skip, buffer);
        free(buffer);
    }
}

int stream_eof(stream_t *stream)
{
    return stream->eof;
}

int64_t stream_tell(stream_t *stream)
{
    int64_t res = deadbeef->ftell(stream->f); /* returns -1 on error */
    if (res < 0) {
        return res;
    }
    return res - stream->junk_offset;
}

int64_t stream_setpos(stream_t *stream, int64_t pos)
{
    return deadbeef->fseek(stream->f, pos + stream->junk_offset, SEEK_SET);
}

stream_t *stream_create_file(DB_FILE *file, int bigendian, int64_t junk_offset)
{
    stream_t *new_stream;

    new_stream = (stream_t*)malloc(sizeof(stream_t));
    memset (new_stream, 0, sizeof (new_stream));
    new_stream->f = file;
    new_stream->bigendian = bigendian;
    new_stream->eof = 0;
    new_stream->junk_offset = 0;//junk_offset;

    return new_stream;
}

void stream_destroy(stream_t *stream)
{
    free(stream);
}

