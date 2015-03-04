/**
 * @ingroup file68_devel
 * @file    file68/istream68.h
 * @author  benjamin gerard
 * @date    2003/08/08
 * @brief   generic stream operation.
 *
 * $Id: istream68.h,v 2.3 2003/09/06 16:13:12 benjihan Exp $
 *
 *    isteam68 provides functions for stream operations.
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ISTREAM68_H_
#define _ISTREAM68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** stream type. */
typedef struct _istream_t istream_t;

/** Get stream name.
 *
 * @param  istream  stream
 *
 * @return stream name
 * @retval 0 Failure.
 */
const char * istream_filename(istream_t *istream);

/** Open stream.
 *
 * @param  istream  stream
 *
 * @return error code
 * @retval 0   Success
 * @retval -1  Failure
 */
int istream_open(istream_t *istream);

/** Close stream.
 *
 * @param  istream  stream
 *
 * @return error code
 * @retval 0   Success
 * @retval -1  Failure
 */
int istream_close(istream_t *istream);

/** Read data from stream.
 *
 * @param  istream  stream
 * @param  data     destination buffer
 * @param  len      number of byte to read
 *
 * @return number of byte read
 * @retval -1 Failure.
 */

int istream_read(istream_t *istream, void * data, int len);

/** Write data into stream.
 *
 * @param  istream  stream
 * @param  data     destination buffer
 * @param  len      number of byte to read
 *
 * @return number of byte written
 * @retval -1 Failure.
 */
int istream_write(istream_t *istream, const void * data, int len);

/** Get stream length.
 *
 * @param  istream  stream
 *
 * @return number of bytes.
 * @retval -1 Failure.
 */
int istream_length(istream_t *istream);

/** Get stream current position.
 *
 * @param  istream  stream
 *
 * @return stream position 
 * @retval -1 Failure.
 */
int istream_tell(istream_t *istream);

/** Set stream relative position.
 *
 * @param  istream  stream
 * @param  offset   displacement from current position
 *
 * @return Absolute position after seeking
 * @retval -1 Failure.
 *
 * @see istream_seek_to()
 */
int istream_seek(istream_t *istream, int offset);

/** Set stream absolute position.
 *
 * @param  istream  stream
 * @param  pos      position to reach
 *
 * @return Absolute position after seeking
 * @retval -1 Failure.
 *
 * @see istream_seek()
 */
int istream_seek_to(istream_t *istream, int pos);

/** Close and destroy stream.
 *
 * @param  istream  stream
 *
 */
void istream_destroy(istream_t *istream);

/** Read a '\\0' or '\\n' terminated string.
 *
 * @param  istream  stream
 * @param  buffer   destination buffer
 * @param  max      destination buffer size
 *
 * @return number of char read
 * @retval -1  Failure.
 */
int istream_gets(istream_t *istream, char * buffer, int max);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ISTREAM68_H_ */
