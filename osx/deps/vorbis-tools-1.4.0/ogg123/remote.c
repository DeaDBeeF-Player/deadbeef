/* remote.c by Richard van Paasen <rvpaasen@dds.nl> */

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Kenneth C. Arnold <ogg@arnoldnet.net> AND OTHER CONTRIBUTORS  *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: remote.c 17013 2010-03-24 08:11:06Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>

#if HAVE_SELECT
#include <sys/select.h>
#endif

#include "ogg123.h"
#include "format.h"

/* Maximum size of the input buffer */
#define MAXBUF 1024
/* Undefine logfile if you don't want it */
//#define LOGFILE "/tmp/ogg123.log"

/* The play function in ogg123.c */
extern void play (char *source_string);
extern ogg123_options_t options;
extern void set_seek_opt(ogg123_options_t *ogg123_opts, char *buf);

/* Status */
typedef enum { PLAY, STOP, PAUSE, NEXT, QUIT} Status;
static Status status = STOP;

/* Threading is introduced to reduce the
   amount of processor time that ogg123
   will take if it is in idle state */

/* Thread control locks */
static pthread_mutex_t main_lock;
static sem_t sem_command;
static sem_t sem_processed;
static pthread_mutex_t output_lock;

#ifdef LOGFILE
void send_log(const char* fmt, ...) {

  FILE* fp;
  va_list ap;
  pthread_mutex_lock (&output_lock);
  fp=fopen(LOGFILE,"a");
  va_start(ap, fmt);
  vfprintf(fp, fmt, ap);
  va_end(ap);
  fprintf(fp, "\n");
  fclose(fp);
  pthread_mutex_unlock (&output_lock);
  return;
}
#else
  #define send_log(...)
#endif

static void send_msg(const char* fmt, ...) {

  va_list ap;
  pthread_mutex_lock (&output_lock);
  fprintf(stdout, "@");
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  fprintf(stdout, "\n");
  pthread_mutex_unlock (&output_lock);
  return;
}

static void send_err(const char* fmt, ...) {
  va_list ap;
  pthread_mutex_lock (&output_lock);
  fprintf(stderr, "@");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  pthread_mutex_unlock (&output_lock);
  return;
}

static Status getstatus() {

  return status;
}

static void setstatus(Status s) {

  status = s;
}

static void invertpause() {

  if (status==PLAY) {
    status = PAUSE;
  }
  else if (status==PAUSE) {
    status = PLAY;
  }
  return;
}

static void * remotethread(void * arg) {

  int done = 0;
  int error = 0;
  int ignore = 0;
  char buf[MAXBUF+1];
  char *b;

#if HAVE_SELECT
  fd_set fd;
#endif
  
  buf[MAXBUF]=0;

  while(!done) {
    /* Read a line */
    buf[0] = 0;
    send_log("Waiting for input: ...");

#if HAVE_SELECT
    FD_ZERO(&fd);
    FD_SET(0,&fd);
    select (1, &fd, NULL, NULL, NULL);
#endif

    fgets(buf, MAXBUF, stdin);
    buf[strlen(buf)-1] = 0;

    /* Lock on */
    pthread_mutex_lock (&main_lock);

    send_log("Input: %s", buf);
    error = 0;

    if (!strncasecmp(buf,"l",1)) {
	/* prepare to load */
      if ((b=strchr(buf,' ')) != NULL) {
        /* Prepare to load a new song */
        strcpy((char*)arg, b+1);
        setstatus(NEXT);
      } 
      else {
        /* Invalid load command */
        error = 1;
      }
    }
    else
    if (!strncasecmp(buf,"p",1)) {
      /* Prepare to (un)pause */
      invertpause();
    }
	else
    if (!strncasecmp(buf,"j",1)) {
      /* Prepare to seek */
      if ((b=strchr(buf,' ')) != NULL) {
        set_seek_opt(&options, b+1);
	  }
      ignore = 1;
    }
    else
    if (!strncasecmp(buf,"s",1)) {
      /* Prepare to stop */
      setstatus(STOP);
    }
	else
    if (!strncasecmp(buf,"r",1)) {
      /* Prepare to reload */
      setstatus(NEXT);
    }
    else
    if (!strncasecmp(buf,"h",1)) {
      /* Send help */
	  send_msg("H +----------------------------------------------------+");
	  send_msg("H | Ogg123 remote interface                            |");
	  send_msg("H |----------------------------------------------------|");
	  send_msg("H | Load <file>     -  load a file and starts playing  |");
	  send_msg("H | Pause           -  pause or unpause playing        |");
	  send_msg("H | Jump [+|-]<f>   -  jump <f> seconds forth or back  |");
	  send_msg("H | Stop            -  stop playing                    |");
	  send_msg("H | Reload          -  reload last song                |");
	  send_msg("H | Quit            -  quit ogg123                     |");
	  send_msg("H |----------------------------------------------------|");
	  send_msg("H | refer to README.remote for documentation           |");
	  send_msg("H +----------------------------------------------------+");
	  ignore = 1;
    }
    else
    if (!strncasecmp(buf,"q",1)) {
      /* Prepare to quit */
      setstatus(QUIT);
      done = 1;
    }
    else {
      /* Unknown input received */
      error = 1;
    }

    if (ignore) {
      /* Unlock */
      pthread_mutex_unlock (&main_lock);
      ignore = 0;
    } else {
      if (error) {
    	/* Send the error and unlock */
    	send_err("E Unknown command '%s'", buf);
    	send_log("Unknown command '%s'", buf);
		/* Unlock */
    	pthread_mutex_unlock (&main_lock);
      } 
      else {
    	/* Signal the main thread */
    	sem_post(&sem_command);
    	/* Unlock */
    	pthread_mutex_unlock (&main_lock);
    	/* Wait until the change has been noticed */
    	sem_wait(&sem_processed);
      }
    }
  }

  return NULL;
}

void remote_mainloop(void) {

  int r;
  pthread_t th;
  Status s;
  char fname[MAXBUF+1];

  /* Need to output line by line! */
  setlinebuf(stdout);

  /* Send a greeting */
  send_msg("R ogg123 from " PACKAGE " " VERSION);

  /* Initialize the thread controlling variables */
  pthread_mutex_init(&main_lock, NULL);
  sem_init(&sem_command, 0, 0);
  sem_init(&sem_processed, 0, 0);

  /* Start the thread */
  r = pthread_create(&th, NULL, remotethread, (void*)fname);
  if (r != 0) {
    send_err("E Could not create a thread (code %d)", r);
    return;
  }

  send_log("Start");

  /* The thread may already have processed some input,
     get the current status
   */
  pthread_mutex_lock(&main_lock);
  s = getstatus();
  pthread_mutex_unlock(&main_lock);

  while (s != QUIT) {

    /* wait for a new command */
    if (s != NEXT) {

      /* Wait until a new status is available,
         This puts the main tread asleep and
         saves resources
       */
       
      sem_wait(&sem_command);

      pthread_mutex_lock(&main_lock);
      s = getstatus();
      pthread_mutex_unlock(&main_lock);
    }

    send_log("Status: %d", s);

    if (s == NEXT) {

      /* The status is to play a new song. Set
         the status to PLAY and signal the thread
         that the status has been processed.
       */
       
      send_msg("I %s", fname);
      send_msg("S 0.0 0 00000 xxxxxx 0 0 0 0 0 0 0 0");
      send_msg("P 2");
      pthread_mutex_lock(&main_lock);
      setstatus(PLAY);
      s = getstatus();
      send_log("mainloop s=%d", s);
      sem_post(&sem_processed);
      s = getstatus();
      send_log("mainloop s=%d", s);
      pthread_mutex_unlock(&main_lock);

      /* Start the player. The player calls the playloop
         frequently to check for a new status (e.g. NEXT,
         STOP or PAUSE.
       */

      pthread_mutex_lock(&main_lock);
      s = getstatus();
      pthread_mutex_unlock(&main_lock);
      send_log("mainloop s=%d", s);
      play(fname);
      
      /* Retrieve the new status */
      pthread_mutex_lock(&main_lock);
      s = getstatus();
      pthread_mutex_unlock(&main_lock);

/* don't know why this was here, sending "play stoped" on NEXT wasn't good idea... */
//      if (s == NEXT) {
	  
	    /* Send "play stopped" */
//        send_msg("P 0");
//        send_log("P 0");
//      } else {
	  
	    /* Send "play stopped at eof" */
//        send_msg("P 0 EOF");
//        send_log("P 0 EOF");
//      }
      
    }
    else {

      /* Irrelevent status, notice the thread that
         it has been processed.
       */
       
      sem_post(&sem_processed);
    }    
  }

  /* Send "Quit" */
  send_msg("Q");
  send_log("Quit");

  /* Cleanup the semaphores */
  sem_destroy(&sem_command);
  sem_destroy(&sem_processed);

  return;
}

int remote_playloop(void) {

  Status s;

  /* Check the status. If the player should pause,
     then signal that the command has been processed
     and wait for a new status. A new status will end
     the player and return control to the main loop.
     The main loop will signal that the new command
     has been processed.
   */
  
  pthread_mutex_lock (&main_lock);
  s = getstatus();
  pthread_mutex_unlock (&main_lock);

  send_log("playloop entry s=%d", s);

  if (s == PAUSE) {

    /* Send "pause on" */
    send_msg("P 1");

    while (s == PAUSE) {

      sem_post(&sem_processed);
      sem_wait(&sem_command);
      pthread_mutex_lock (&main_lock);
      s = getstatus();
      pthread_mutex_unlock (&main_lock);
    }

    /* Send "pause off" */
    send_msg("P 2");
  }

    /* Send stop msg to the frontend */
    /* this probably should be done after the audio buffer is flushed and no audio is actually playing, but don't know how */
  if ((s == STOP) || (s == QUIT)) send_msg("P 0");

  send_log("playloop exit s=%d", s);

  return ((s == NEXT) || (s == STOP) || (s == QUIT));
}

void remote_time(double current, double total) {

  /* Send the frame (not implemented yet) and the time */
  send_msg("F 0 0 %.2f %.2f", current, (total-current));

  return;
}
