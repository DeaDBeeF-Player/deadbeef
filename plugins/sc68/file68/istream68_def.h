/**
 * @ingroup file68_devel
 * @file    file68/istream68_def.h
 * @author  benjamin gerard
 * @date    2003/08/08
 * @brief   generic stream interface.
 *
 * $Id: istream68_def.h,v 2.5 2003/09/30 06:29:57 benjihan Exp $
 *
 *    Generic stream interface definition.
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _ISTREAM68_DEF_H_
#define _ISTREAM68_DEF_H_

#include "file68/istream68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name  Open mode defines.
 *  @{
 */

/** Read open mode bit. */
#define ISTREAM_OPEN_READ_BIT 0

/** Read open mode value. */
#define ISTREAM_OPEN_READ  (1<<ISTREAM_OPEN_READ_BIT)

/** Write open mode bit. */
#define ISTREAM_OPEN_WRITE_BIT 1

/** Write open mode value. */
#define ISTREAM_OPEN_WRITE (1<<ISTREAM_OPEN_WRITE_BIT)

/** Test if any open flags is set (returns 0 or 1). */
#define ISTREAM_IS_OPEN(V) (!!((V)&(ISTREAM_OPEN_READ|ISTREAM_OPEN_WRITE)))

/** Test if READ open flags is set (returns 0 or 1). */
#define ISTREAM_IS_OPEN_READ(V) (((V)>>ISTREAM_OPEN_READ_BIT)&1)

/** Test if WRITE open flags is set (returns 0 or 1). */
#define ISTREAM_IS_OPEN_WRITE(V) (((V)>>ISTREAM_OPEN_WRITE_BIT)&1)

/**@}*/

/** @name input stream function types.
 *  @{
 */
typedef const char * (* istream_name_t) (istream_t *);
typedef int (* istream_open_t) (istream_t *);
typedef int (* istream_close_t) (istream_t *);
typedef int (* istream_length_t) (istream_t *);
typedef int (* istream_tell_t) (istream_t *);
typedef int (* istream_seek_t) (istream_t *, int);
typedef int (* istream_read_t) (istream_t *, void *, int);
typedef int (* istream_write_t) (istream_t *, const void *, int);
typedef void (* istream_destroy_t) (istream_t *);
/**@}*/

/** Input stream structure. */
struct _istream_t {
  /*const*/ istream_name_t name;     /**< Get stream name.        */
  /*const*/ istream_open_t open;     /**< Open stream.            */
  /*const*/ istream_close_t close;   /**< Close stream.           */
  /*const*/ istream_read_t read;     /**< Read data from stream.  */
  /*const*/ istream_write_t write;   /**< Write data to stream/   */
  /*const*/ istream_length_t length; /**< Get stream data length. */
  /*const*/ istream_tell_t tell;     /**< Get stream position.    */
  /*const*/ istream_seek_t seekf;    /**< Seek forward.           */
  /*const*/ istream_seek_t seekb;    /**< Seek backward.          */
  /*const*/ istream_destroy_t destroy; /**< Destructor .          */
};

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ISTREAM68_DEF_H_ */
