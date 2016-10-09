#ifndef STREAM_H
#define STREAM_H

/* stream.h */

#ifdef _WIN32
	#include "stdint_win.h"
#else
	#include <stdint.h>
#endif

#include "../../deadbeef.h"

typedef struct stream_tTAG stream_t;

int32_t stream_read(stream_t *stream, size_t len, void *buf);

int32_t stream_read_int32(stream_t *stream);
uint32_t stream_read_uint32(stream_t *stream);

int16_t stream_read_int16(stream_t *stream);
uint16_t stream_read_uint16(stream_t *stream);

int8_t stream_read_int8(stream_t *stream);
uint8_t stream_read_uint8(stream_t *stream);

void stream_skip(stream_t *stream, int64_t skip);

int stream_eof(stream_t *stream);

int64_t stream_tell(stream_t *stream);
int64_t stream_setpos(stream_t *stream, int64_t pos);

stream_t *stream_create_file(DB_FILE *file,
                             int bigendian, int64_t junk_offset);
void stream_destroy(stream_t *stream);

#endif /* STREAM_H */

