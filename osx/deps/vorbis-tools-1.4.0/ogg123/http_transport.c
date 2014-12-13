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
 
 last mod: $Id: http_transport.c 16825 2010-01-27 04:14:08Z xiphmont $
 
********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_CURL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <pthread.h>

#include "ogg123.h"
#include "transport.h"
#include "buffer.h"
#include "status.h"
#include "callbacks.h"
#include "i18n.h"

#define INPUT_BUFFER_SIZE 32768

extern stat_format_t *stat_format;  /* FIXME Bad hack!  Will fix after RC3! */
extern signal_request_t sig_request;  /* Need access to global cancel flag */

typedef struct http_private_t {
  int cancel_flag;

  buf_t *buf;

  pthread_t curl_thread;

  CURL *curl_handle;
  struct curl_slist *header_list;
  char error[CURL_ERROR_SIZE];

  data_source_t *data_source;
  data_source_stats_t stats;
} http_private_t;


transport_t http_transport;  /* Forward declaration */

/* -------------------------- curl callbacks ----------------------- */

size_t write_callback (void *ptr, size_t size, size_t nmemb, void *arg)
{
  http_private_t *myarg = arg;

  if (myarg->cancel_flag || sig_request.cancel)
    return 0;

  if (!buffer_submit_data(myarg->buf, ptr, size*nmemb))
    return 0;

  if (myarg->cancel_flag || sig_request.cancel)
    return 0;

  return size * nmemb;
}

int progress_callback (void *arg, size_t dltotal, size_t dlnow,
		       size_t ultotal, size_t ulnow)
{
  http_private_t *myarg = arg;
  print_statistics_arg_t *pstats_arg;
  data_source_t *source = myarg->data_source;

  if (myarg->cancel_flag || sig_request.cancel)
    return -1;

  pstats_arg = new_print_statistics_arg(stat_format,
					source->transport->statistics(source),
					NULL);

  print_statistics_action(NULL, pstats_arg);

  if (myarg->cancel_flag || sig_request.cancel)
    return -1;

  return 0;
}


/* -------------------------- Private functions --------------------- */

void set_curl_opts (http_private_t *private)
{
  CURL *handle = private->curl_handle;

  curl_easy_setopt(handle, CURLOPT_FILE, private);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(handle, CURLOPT_URL, private->data_source->source_string);
  /*
  if (inputOpts.ProxyPort)
    curl_easy_setopt(handle, CURLOPT_PROXYPORT, inputOpts.ProxyPort);
  if (inputOpts.ProxyHost)
    curl_easy_setopt(handle, CURLOPT_PROXY, inputOpts.ProxyHost);
  if (inputOpts.ProxyTunnel)
    curl_easy_setopt (handle, CURLOPT_HTTPPROXYTUNNEL, inputOpts.ProxyTunnel);
  */
#ifdef CURLOPT_MUTE
  curl_easy_setopt(handle, CURLOPT_MUTE, 1);
#endif
  curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, private->error);
  curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progress_callback);
  curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, private);
  curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
  curl_easy_setopt(handle, CURLOPT_USERAGENT, "ogg123/"VERSION);
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, private->header_list);
  curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
}


void *curl_thread_func (void *arg)
{
  http_private_t *myarg = (http_private_t *) arg;
  CURLcode ret;
  sigset_t set;

  /* Block signals to this thread */
  sigfillset (&set);
  sigaddset (&set, SIGINT);
  sigaddset (&set, SIGTSTP);
  sigaddset (&set, SIGCONT);
  if (pthread_sigmask (SIG_BLOCK, &set, NULL) != 0)
    status_error(_("ERROR: Could not set signal mask."));

  ret = curl_easy_perform((CURL *) myarg->curl_handle);

  if (myarg->cancel_flag || sig_request.cancel) {
    buffer_abort_write(myarg->buf);
    ret = 0;  // "error" was on purpose
  } else
    buffer_mark_eos(myarg->buf);

  if (ret != 0)
    status_error(myarg->error);

  curl_easy_cleanup(myarg->curl_handle);
  myarg->curl_handle = 0;

  curl_slist_free_all(myarg->header_list);
  myarg->header_list = NULL;

  return (void *) ret;
}


/* -------------------------- Public interface -------------------------- */

int http_can_transport (char *source_string)
{
  int tmp;

  tmp = strchr(source_string, ':') - source_string;
  return tmp < 10 && 
    tmp + 2 < strlen(source_string) && 
    !strncmp(source_string + tmp, "://", 3);
}


data_source_t* http_open (char *source_string, ogg123_options_t *ogg123_opts)
{
  data_source_t *source;
  http_private_t *private;

  /* Allocate data source structures */
  source = malloc(sizeof(data_source_t));
  private = malloc(sizeof(http_private_t));

  if (source != NULL && private != NULL) {
    source->source_string = strdup(source_string);
    source->transport = &http_transport;
    source->private = private;

    private->buf = buffer_create (ogg123_opts->input_buffer_size,
				  ogg123_opts->input_buffer_size *
				  ogg123_opts->input_prebuffer / 100.0, 
				  NULL, NULL,  /* No write callback, using
						  buffer in pull mode. */
				  0 /* Irrelevant */);
    if (private->buf == NULL) {
      status_error(_("ERROR: Unable to create input buffer.\n"));
      exit(1);
    }

    private->curl_handle = NULL;
    private->header_list = NULL;
    private->data_source = source;
    private->stats.transfer_rate = 0;
    private->stats.bytes_read = 0;
    private->stats.input_buffer_used = 0;
    private->cancel_flag = 0;

  } else {
    fprintf(stderr, _("ERROR: Out of memory.\n"));
    exit(1);
  }

  /* ogg123 only accepts Ogg files, and optionally FLAC as well */
#ifdef HAVE_LIBFLAC
  private->header_list = curl_slist_append(NULL, "Accept: application/ogg, audio/ogg, video/ogg, audio/x-flac;q=0.9");
#else
  private->header_list = curl_slist_append(NULL, "Accept: application/ogg, audio/ogg, video/ogg;q=0.9");
#endif
  if (private->header_list == NULL)
    goto fail;

  /* Open URL */
  private->curl_handle = curl_easy_init();
  if (private->curl_handle == NULL)
    goto fail;

  set_curl_opts(private);

  /* Start thread */
  if (pthread_create(&private->curl_thread, NULL, curl_thread_func, 
		     private) != 0)
    goto fail;


  stat_format[2].enabled = 0;  /* remaining playback time */
  stat_format[3].enabled = 0;  /* total playback time  */
  stat_format[6].enabled = 1;  /* Input buffer fill % */
  stat_format[7].enabled = 1;  /* Input buffer state  */

  return source;


fail:
  if (private->curl_handle != NULL)
    curl_easy_cleanup(private->curl_handle);
  if (private->header_list != NULL)
    curl_slist_free_all(private->header_list);
  free(source->source_string);
  free(private);
  free(source);

  return NULL;
}


int http_peek (data_source_t *source, void *ptr, size_t size, size_t nmemb)
{
  /*
  http_private_t *private = source->private;
  int items;
  long start;

  bytes_read = buffer_get_data(data->buf, ptr, size, nmemb);

  private->stats.bytes_read += bytes_read;
  */

  return 0;
}


int http_read (data_source_t *source, void *ptr, size_t size, size_t nmemb)
{
  http_private_t *private = source->private;
  int bytes_read;

  if (private->cancel_flag || sig_request.cancel)
    return 0;

  bytes_read = buffer_get_data(private->buf, ptr, size * nmemb);

  private->stats.bytes_read += bytes_read;

  return bytes_read;
}


int http_seek (data_source_t *source, long offset, int whence)
{
  return -1;
}


data_source_stats_t *http_statistics (data_source_t *source)
{
  http_private_t *private = source->private;
  data_source_stats_t *data_source_stats;
  buffer_stats_t *buffer_stats;

  data_source_stats = malloc_data_source_stats(&private->stats);
  data_source_stats->input_buffer_used = 1;
  data_source_stats->transfer_rate = 0;

  buffer_stats = buffer_statistics(private->buf);
  data_source_stats->input_buffer = *buffer_stats;
  free(buffer_stats);

  return data_source_stats;
}


long http_tell (data_source_t *source)
{
  return 0;
}


void http_close (data_source_t *source)
{
  http_private_t *private = source->private;

  private->cancel_flag = 1;
  buffer_abort_write(private->buf);
  pthread_join(private->curl_thread, NULL);

  buffer_destroy(private->buf);
  private->buf = NULL;

  free(source->source_string);
  free(source->private);
  free(source);
}


transport_t http_transport = {
  "http",
  &http_can_transport,
  &http_open,
  &http_peek,
  &http_read,
  &http_seek,
  &http_statistics,
  &http_tell,
  &http_close
};

#endif /* HAVE_CURL */
