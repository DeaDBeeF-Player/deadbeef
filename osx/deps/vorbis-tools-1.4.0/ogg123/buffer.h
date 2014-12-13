/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************
 
 last mod: $Id: buffer.h 16825 2010-01-27 04:14:08Z xiphmont $
 
********************************************************************/

/* A generic circular buffer interface with the ability to buffer
   actions (conceptually) between bytes in the buffer.*/

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdlib.h>
#include <pthread.h>
#include <ogg/os_types.h>


struct action_t; /* forward declaration */

/* buffer_write_func(void *data, int nbytes, int eos, void *arg) */
typedef int (*buffer_write_func_t) (void *, int, int, void *);

typedef struct buf_t
{
  /* generic buffer interface */
  void *write_arg;
  buffer_write_func_t write_func;

  /* pthread variables */
  pthread_t thread;

  pthread_mutex_t mutex;
  
  pthread_cond_t playback_cond; /* signalled when playback can continue */
  pthread_cond_t write_cond;    /* signalled when more data can be written 
				   to the buffer */
  
  /* buffer info (constant) */
  int  audio_chunk_size;  /* write data to audio device in this chunk size, 
			     if possible */
  long prebuffer_size;    /* number of bytes to prebuffer */
  long size;              /* buffer size, for reference */

  int cancel_flag;        /* When set, the playback thread should exit */

  /* ----- Everything after this point is protected by mutex ----- */

  /* buffering state variables */
  int prebuffering;
  int paused;
  int eos;
  int abort_write;

  /* buffer data */
  long curfill;     /* how much the buffer is currently filled */
  long start;       /* offset in buffer of start of available data */
  ogg_int64_t position; /* How many bytes have we output so far */
  ogg_int64_t position_end; /* Position right after end of data */

  struct action_t *actions; /* Queue actions to perform */
  char buffer[1];   /* The buffer itself. It's more than one byte. */
} buf_t;


/* action_func(buf_t *buf, void *arg) */
typedef void (*action_func_t) (buf_t *, void *);

typedef struct action_t {
  ogg_int64_t position;
  action_func_t action_func;
  void *arg;
  struct action_t *next;
} action_t;


typedef struct buffer_stats_t {
  long size;
  double fill;
  double prebuffer_fill;
  int prebuffering;
  int paused;
  int eos;
} buffer_stats_t;


/* --- Buffer allocation --- */

buf_t *buffer_create (long size, long prebuffer,
		      buffer_write_func_t write_func, void *arg,
		      int audio_chunk_size);

void buffer_reset (buf_t *buf);
void buffer_destroy (buf_t *buf);

/* --- Buffer thread control --- */
int  buffer_thread_start   (buf_t *buf);
void buffer_thread_pause   (buf_t *buf);
void buffer_thread_unpause (buf_t *buf);
void buffer_thread_kill    (buf_t *buf);

/* --- Data buffering functions --- */
int buffer_submit_data (buf_t *buf, char *data, long nbytes);
size_t buffer_get_data (buf_t *buf, char *data, long nbytes);

void buffer_mark_eos (buf_t *buf);
void buffer_abort_write (buf_t *buf);

/* --- Action buffering functions --- */
void buffer_action_now (buf_t *buf, action_func_t action_func, 
			void *action_arg);
void buffer_insert_action_at_end (buf_t *buf, action_func_t action_func, 
				  void *action_arg);
void buffer_append_action_at_end (buf_t *buf, action_func_t action_func, 
				  void *action_arg);
void buffer_insert_action_at (buf_t *buf, action_func_t action_func, 
			      void *action_arg, ogg_int64_t position);
void buffer_append_action_at (buf_t *buf, action_func_t action_func, 
			      void *action_arg, ogg_int64_t position);

/* --- Buffer status functions --- */
void buffer_wait_for_empty (buf_t *buf);
long buffer_full (buf_t *buf);
buffer_stats_t *buffer_statistics (buf_t *buf);

#endif /* __BUFFER_H__ */
