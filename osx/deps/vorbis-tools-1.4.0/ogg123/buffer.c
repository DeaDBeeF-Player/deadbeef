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

 last mod: $Id: buffer.c 16870 2010-02-04 14:15:42Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "compat.h"
#include "buffer.h"
#include "i18n.h"
#include "ogg123.h"

#define MIN(x,y)       ( (x) < (y) ? (x) : (y) )
#define MIN3(x,y,z)    MIN(x,MIN(y,z))
#define MIN4(w,x,y,z)  MIN( MIN(w,x), MIN(y,z) )

#ifdef DEBUG_BUFFER
FILE *debugfile;
#define DEBUG(x) { fprintf (debugfile, "%d: " x "\n", getpid()); }
#define DEBUG1(x, y) { fprintf (debugfile, "%d: " x "\n", getpid(), y); }
#define DEBUG2(x, y, z) { fprintf (debugfile, "%d: " x "\n", getpid(), y, z); }
#else
#define DEBUG(x)
#define DEBUG1(x, y)
#define DEBUG2(x, y, z)
#endif

/* Macros that support debugging of threading structures */

#define LOCK_MUTEX(mutex) { DEBUG1("Locking mutex %s.", #mutex); pthread_mutex_lock (&(mutex)); }
#define UNLOCK_MUTEX(mutex) { DEBUG1("Unlocking mutex %s", #mutex); pthread_mutex_unlock(&(mutex)); }
#define COND_WAIT(cond, mutex) { DEBUG2("Unlocking %s and waiting on %s", #mutex, #cond); pthread_cond_wait(&(cond), &(mutex)); }
#define COND_SIGNAL(cond) { DEBUG1("Signalling %s", #cond); pthread_cond_signal(&(cond)); }

extern signal_request_t sig_request;  /* Need access to global cancel flag */


/* -------------------- Private Functions ------------------ */

void buffer_init_vars (buf_t *buf)
{
  /* Initialize buffer flags */
  buf->prebuffering = buf->prebuffer_size > 0;
  buf->paused = 0;
  buf->eos = 0;
  buf->abort_write = 0;
  buf->cancel_flag = 0;

  buf->curfill = 0;
  buf->start = 0;
  buf->position = 0;
  buf->position_end = 0;
}

void buffer_thread_init (buf_t *buf)
{
  sigset_t set;

  DEBUG("Enter buffer_thread_init");

  /* Block signals to this thread */
  sigfillset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTSTP);
  sigaddset(&set, SIGCONT);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
    DEBUG("pthread_sigmask failed");
}


void buffer_thread_cleanup (void *arg)
{
  buf_t *buf = (buf_t *)arg;

  DEBUG("Enter buffer_thread_cleanup");
}


void buffer_mutex_unlock (void *arg)
{
  buf_t *buf = (buf_t *)arg;

  UNLOCK_MUTEX(buf->mutex);
}


action_t *malloc_action (action_func_t action_func, void *action_arg)
{
  action_t *action;

  action = malloc(sizeof(action_t));

  if (action == NULL) {
    fprintf(stderr, _("ERROR: Out of memory in malloc_action().\n"));
    exit(1);
  }

  action->position = 0;
  action->action_func = action_func;
  action->arg = action_arg;
  action->next = NULL;

  return action;
}


/* insert = 1:  Make this action the first action associated with this position
   insert = 0:  Make this action the last action associated with this position
*/
#define INSERT 1
#define APPEND 0
void in_order_add_action (action_t **action_list, action_t *action, int insert)
{
  insert = insert > 0 ? 1 : 0;  /* Clamp in case caller messed up */

  while (*action_list != NULL && 
	 (*action_list)->position <= (action->position + insert))
    action_list = &((*action_list)->next);

  action->next = *action_list;
  *action_list = action;
}


void execute_actions (buf_t *buf, action_t **action_list, ogg_int64_t position)
{
  action_t *action;

  while (*action_list != NULL && (*action_list)->position <= position) {
    action = *action_list;
    action->action_func(buf, action->arg);

    *action_list = (*action_list)->next;
    free(action);
  }
}


void free_action (action_t *action)
{
  free(action);
}



int compute_dequeue_size (buf_t *buf, int request_size)
{
  ogg_int64_t next_action_pos;

  /* 
     For simplicity, the number of bytes played must satisfy the following
     requirements:
     1. Do not extract more bytes than are stored in the buffer.
     2. Do not extract more bytes than the requested number of bytes.
     3. Do not run off the end of the buffer.
     4. Do not go past the next action.
  */

  if (buf->actions != NULL) {

    next_action_pos = buf->actions->position;

    return MIN4((ogg_int64_t)buf->curfill, (ogg_int64_t)request_size,
               (ogg_int64_t)(buf->size - buf->start),
               next_action_pos - buf->position);
  } else
    return MIN3(buf->curfill, (long)request_size, buf->size - buf->start);

}


void *buffer_thread_func (void *arg)
{
  buf_t *buf = (buf_t*) arg;
  size_t write_amount;

  DEBUG("Enter buffer_thread_func");

  buffer_thread_init(buf);

  pthread_cleanup_push(buffer_thread_cleanup, buf);

  DEBUG("Start main play loop");

  /* This test is safe since curfill will never decrease and eos will
     never be unset. */
  while ( !(buf->eos && buf->curfill == 0) && !buf->abort_write) {

    if (buf->cancel_flag || sig_request.cancel)
      break;

    DEBUG("Check for something to play");
    /* Block until we can play something */
    LOCK_MUTEX (buf->mutex);
    if (buf->prebuffering || 
	buf->paused || 
	(buf->curfill < buf->audio_chunk_size && !buf->eos)) {

      DEBUG("Waiting for more data to play.");
      COND_WAIT(buf->playback_cond, buf->mutex);
    }

    DEBUG("Ready to play");

    UNLOCK_MUTEX(buf->mutex);

    if (buf->cancel_flag || sig_request.cancel)
      break;

    /* Don't need to lock buffer while running actions since position
       won't change.  We clear out any actions before we compute the
       dequeue size so we don't consider actions that need to
       run right now.  */
    execute_actions(buf, &buf->actions, buf->position);

    LOCK_MUTEX(buf->mutex);

    /* Need to be locked while we check things. */
    write_amount = compute_dequeue_size(buf, buf->audio_chunk_size);

    UNLOCK_MUTEX(buf->mutex);
 
    if(write_amount){ /* we might have been woken spuriously */
      /* No need to lock mutex here because the other thread will
         NEVER reduce the number of bytes stored in the buffer */
      DEBUG1("Sending %d bytes to the audio device", write_amount);
      write_amount = buf->write_func(buf->buffer + buf->start, write_amount,
                                     /* Only set EOS if this is the last chunk */
                                     write_amount == buf->curfill ? buf->eos : 0,
                                     buf->write_arg);

      if (!write_amount) {
        DEBUG("Error writing to the audio device. Aborting.");
        buffer_abort_write(buf);
      }

      LOCK_MUTEX(buf->mutex);

      buf->curfill -= write_amount;
      buf->position += write_amount;
      buf->start = (buf->start + write_amount) % buf->size;
      DEBUG1("Updated buffer fill, curfill = %ld", buf->curfill);

      /* If we've essentially emptied the buffer and prebuffering is enabled,
         we need to do another prebuffering session */
      if (!buf->eos && (buf->curfill < buf->audio_chunk_size))
        buf->prebuffering = buf->prebuffer_size > 0;
    }else{
      DEBUG("Woken spuriously");
    }

    /* Signal a waiting decoder thread that they can put more audio into the
       buffer */
    DEBUG("Signal decoder thread that buffer space is available");
    COND_SIGNAL(buf->write_cond);

    UNLOCK_MUTEX(buf->mutex);
  }

  pthread_cleanup_pop(1);
  DEBUG("exiting buffer_thread_func");

  return 0;
}


int submit_data_chunk (buf_t *buf, char *data, size_t size)
{
  long   buf_write_pos; /* offset of first available write location */
  size_t write_size;

  DEBUG1("Enter submit_data_chunk, size %d", size);

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  /* Put the data into the buffer as space is made available */
  while (size > 0 && !buf->abort_write) {

    /* Section 1: Write a chunk of data */
    DEBUG("Obtaining lock on buffer");
    LOCK_MUTEX(buf->mutex);
    if (buf->size - buf->curfill > 0) {

      /* Figure how much we can write into the buffer.  Requirements:
	 1. Don't write more data than we have.
	 2. Don't write more data than we have room for.
	 3. Don't write past the end of the buffer. */
      buf_write_pos = (buf->start + buf->curfill) % buf->size;
      write_size = MIN3(size, buf->size - buf->curfill,
			buf->size - buf_write_pos);

      memcpy(buf->buffer + buf_write_pos, data, write_size);
      buf->curfill += write_size;
      data += write_size;
      size -= write_size;
      buf->position_end += write_size;
      DEBUG1("writing chunk into buffer, curfill = %ld", buf->curfill);
    }
    else {

      if (buf->cancel_flag || sig_request.cancel) {
        UNLOCK_MUTEX(buf->mutex);
        break;
      }
      /* No room for more data, wait until there is */
      DEBUG("No room for data in buffer.  Waiting.");
      COND_WAIT(buf->write_cond, buf->mutex);
    }
    /* Section 2: signal if we are not prebuffering, done
       prebuffering, or paused */
    if (buf->prebuffering && (buf->prebuffer_size <= buf->curfill)) {

      DEBUG("prebuffering done")
	buf->prebuffering = 0; /* done prebuffering */
    }

    if (!buf->prebuffering && !buf->paused) {

      DEBUG("Signalling playback thread that more data is available.");
      COND_SIGNAL(buf->playback_cond);
    } else
      DEBUG("Not signalling playback thread since prebuffering or paused.");

    UNLOCK_MUTEX(buf->mutex);
  }

  pthread_cleanup_pop(0);

  DEBUG("Exit submit_data_chunk");
  return !buf->abort_write;
}


buffer_stats_t *malloc_buffer_stats ()
{
  buffer_stats_t *new_stats;

  new_stats = malloc(sizeof(buffer_stats_t));

  if (new_stats == NULL) {
    fprintf(stderr, _("ERROR: Could not allocate memory in malloc_buffer_stats()\n"));
    exit(1);
  }

  return new_stats;
}


/* ------------------ Begin public interface ------------------ */

/* --- Buffer allocation --- */

buf_t *buffer_create (long size, long prebuffer,
		      buffer_write_func_t write_func, void *arg,
		      int audio_chunk_size)
{
  buf_t *buf = malloc (sizeof(buf_t) + sizeof (char) * (size - 1));

  if (buf == NULL) {
      perror ("malloc");
      exit(1);
  }

#ifdef DEBUG_BUFFER
  debugfile = fopen ("/tmp/bufferdebug", "w");
  setvbuf (debugfile, NULL, _IONBF, 0);
#endif

  /* Initialize the buffer structure. */
  DEBUG1("buffer_create, size = %ld", size);

  memset (buf, 0, sizeof(*buf));

  buf->write_func = write_func;
  buf->write_arg = arg;

  /* Setup pthread variables */
  pthread_mutex_init(&buf->mutex, NULL);
  pthread_cond_init(&buf->write_cond, NULL);
  pthread_cond_init(&buf->playback_cond, NULL);

  /* Correct for impossible prebuffer and chunk sizes */
  if (audio_chunk_size > size || audio_chunk_size == 0)
    audio_chunk_size = size / 2;

  if (prebuffer > size)
    prebuffer = prebuffer / 2;

  buf->audio_chunk_size = audio_chunk_size;

  buf->prebuffer_size = prebuffer;
  buf->size = size;

  buf->actions = 0;

  /* Initialize flags */
  buffer_init_vars(buf);

  return buf;
}


void buffer_reset (buf_t *buf)
{
  action_t *action;

  /* Cleanup pthread variables */
  pthread_mutex_destroy(&buf->mutex);
  pthread_cond_destroy(&buf->write_cond);
  pthread_cond_destroy(&buf->playback_cond);

  /* Reinit pthread variables */
  pthread_mutex_init(&buf->mutex, NULL);
  pthread_cond_init(&buf->write_cond, NULL);
  pthread_cond_init(&buf->playback_cond, NULL);

  /* Clear old actions */
  while (buf->actions != NULL) {
    action = buf->actions;
    buf->actions = buf->actions->next;
    free(action);
  }

  buffer_init_vars(buf);
}


void buffer_destroy (buf_t *buf)
{
  DEBUG("buffer_destroy");

  /* Cleanup pthread variables */
  pthread_mutex_destroy(&buf->mutex);
  COND_SIGNAL(buf->write_cond);
  pthread_cond_destroy(&buf->write_cond);
  COND_SIGNAL(buf->playback_cond);
  pthread_cond_destroy(&buf->playback_cond);

  free(buf);
}


/* --- Buffer thread control --- */

int buffer_thread_start (buf_t *buf)
{
  DEBUG("Starting new thread.");

  return pthread_create(&buf->thread, NULL, buffer_thread_func, buf);
}


/* WARNING: DO NOT call buffer_submit_data after you pause the
   playback thread, or you run the risk of deadlocking.  Call
   buffer_thread_unpause first. */
void buffer_thread_pause (buf_t *buf)
{
  DEBUG("Pausing playback thread");

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);
  buf->paused = 1;
  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);
}


void buffer_thread_unpause (buf_t *buf)
{
  DEBUG("Unpausing playback thread");

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);
  buf->paused = 0;
  COND_SIGNAL(buf->playback_cond);
  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);
}


void buffer_thread_kill (buf_t *buf)
{
  DEBUG("Attempting to kill playback thread.");

  /* Flag the cancellation */
  buf->cancel_flag = 1;

  /* Signal the playback condition to wake stuff up */
  COND_SIGNAL(buf->playback_cond);

  pthread_join(buf->thread, NULL);

  buffer_thread_cleanup(buf);

  DEBUG("Playback thread killed.");
}


/* --- Data buffering functions --- */

int buffer_submit_data (buf_t *buf, char *data, long nbytes) {
  return submit_data_chunk (buf, data, nbytes);
}

size_t buffer_get_data (buf_t *buf, char *data, long nbytes)
{
  int write_amount;
  int orig_size;

  orig_size = nbytes;

  DEBUG("Enter buffer_get_data");

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);

  /* Put the data into the buffer as space is made available */
  while (nbytes > 0) {

    if (buf->abort_write)
      break;

    DEBUG("Obtaining lock on buffer");
    /* Block until we can read something */
    if (buf->curfill == 0 && buf->eos)
      break; /* No more data to read */

    if (buf->curfill == 0 || (buf->prebuffering && !buf->eos)) {
      DEBUG("Waiting for more data to copy.");
      COND_WAIT(buf->playback_cond, buf->mutex);
    }

    if (buf->abort_write)
      break;

    /* Note: Even if curfill is still 0, nothing bad will happen here */

    /* For simplicity, the number of bytes played must satisfy
       the following three requirements:

       1. Do not copy more bytes than are stored in the buffer.
       2. Do not copy more bytes than the reqested data size.
       3. Do not run off the end of the buffer. */
    write_amount = compute_dequeue_size(buf, nbytes);

    UNLOCK_MUTEX(buf->mutex);
    execute_actions(buf, &buf->actions, buf->position);

    /* No need to lock mutex here because the other thread will
       NEVER reduce the number of bytes stored in the buffer */
    DEBUG1("Copying %d bytes from the buffer", write_amount);
    memcpy(data, buf->buffer + buf->start, write_amount);
    LOCK_MUTEX(buf->mutex);

    buf->curfill -= write_amount;
    data += write_amount;
    nbytes -= write_amount;
    buf->start = (buf->start + write_amount) % buf->size;
    DEBUG1("Updated buffer fill, curfill = %ld", buf->curfill);

    /* Signal a waiting decoder thread that they can put more
       audio into the buffer */
    DEBUG("Signal decoder thread that buffer space is available");
    COND_SIGNAL(buf->write_cond);
  }

  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);

  pthread_testcancel();

  DEBUG("Exit buffer_get_data");

  return orig_size - nbytes;
}

void buffer_mark_eos (buf_t *buf)
{
  DEBUG("buffer_mark_eos");

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);
  buf->eos = 1;
  buf->prebuffering = 0;
  COND_SIGNAL(buf->playback_cond);
  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);
}

void buffer_abort_write (buf_t *buf)
{
  DEBUG("buffer_abort_write");

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);
  buf->abort_write = 1;
  COND_SIGNAL(buf->write_cond);
  COND_SIGNAL(buf->playback_cond);
  UNLOCK_MUTEX(buf->mutex);  

  pthread_cleanup_pop(0);
}


/* --- Action buffering functions --- */

void buffer_action_now (buf_t *buf, action_func_t action_func, 
			void *action_arg)
{
  action_t *action;

  action = malloc_action(action_func, action_arg);

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);

  action->position = buf->position;

  /* Insert this action right at the front */
  action->next = buf->actions;
  buf->actions = action;

  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);
}


void buffer_insert_action_at_end (buf_t *buf, action_func_t action_func, 
				  void *action_arg)
{
  action_t *action;

  action = malloc_action(action_func, action_arg);

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);

  /* Stick after the last item in the buffer */
  action->position = buf->position_end;

  in_order_add_action(&buf->actions, action, INSERT);

  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);
}


void buffer_append_action_at_end (buf_t *buf, action_func_t action_func, 
				  void *action_arg)
{
  action_t *action;

  action = malloc_action(action_func, action_arg);

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);

  /* Stick after the last item in the buffer */
  action->position = buf->position_end;

  in_order_add_action(&buf->actions, action, APPEND);

  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);
}


void buffer_insert_action_at (buf_t *buf, action_func_t action_func, 
			      void *action_arg, ogg_int64_t position)
{
  action_t *action;

  action = malloc_action(action_func, action_arg);

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);
  
  action->position = position;

  in_order_add_action(&buf->actions, action, INSERT);

  UNLOCK_MUTEX(buf->mutex);  

  pthread_cleanup_pop(0);
}


void buffer_append_action_at (buf_t *buf, action_func_t action_func, 
			      void *action_arg, ogg_int64_t position)
{
  action_t *action;

  action = malloc_action(action_func, action_arg);

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);

  action->position = position;

  in_order_add_action(&buf->actions, action, APPEND);

  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);
}


/* --- Buffer status functions --- */

void buffer_wait_for_empty (buf_t *buf)
{
  int empty = 0;

  DEBUG("Enter buffer_wait_for_empty");

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);
  while (!empty && !buf->abort_write) {

    if (buf->curfill > 0) {
      DEBUG1("Buffer curfill = %ld, going back to sleep.", buf->curfill);
      COND_WAIT(buf->write_cond, buf->mutex);
    } else 
      empty = 1;
  }
  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);

  DEBUG("Exit buffer_wait_for_empty");
}


long buffer_full (buf_t *buf)
{
  return buf->curfill;
}


buffer_stats_t *buffer_statistics (buf_t *buf)
{
  buffer_stats_t *stats;

  pthread_cleanup_push(buffer_mutex_unlock, buf);

  LOCK_MUTEX(buf->mutex);

  stats = malloc_buffer_stats();

  stats->size = buf->size;
  stats->fill = (double) buf->curfill / (double) buf->size * 100.0;
  stats->prebuffer_fill = (double) buf->prebuffer_size / (double) buf->size;
  stats->prebuffering = buf->prebuffering;
  stats->paused = buf->paused;
  stats->eos = buf->eos;

  UNLOCK_MUTEX(buf->mutex);

  pthread_cleanup_pop(0);

  return stats;
}
