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

 last mod: $Id: vorbis_comments.c 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "format.h"
#include "utf8.h"
#include "i18n.h"


/* Vorbis comment keys that need special formatting. */
struct {
  char *key;         /* includes the '=' for programming convenience */
  char *formatstr;   /* formatted output */
} vorbis_comment_keys[] = {
  {"TRACKNUMBER=", N_("Track number:")},
  {"REPLAYGAIN_TRACK_GAIN=", N_("ReplayGain (Track):")},
  {"REPLAYGAIN_ALBUM_GAIN=", N_("ReplayGain (Album):")},
  {"REPLAYGAIN_TRACK_PEAK=", N_("ReplayGain Peak (Track):")},
  {"REPLAYGAIN_ALBUM_PEAK=", N_("ReplayGain Peak (Album):")},
  {"COPYRIGHT=", N_("Copyright")},
  {"=", N_("Comment:")},
  {NULL, N_("Comment:")}
};


char *lookup_comment_prettyprint (char *comment, int *offset)
{
  int i, j;
  char *s;

  /* Search for special-case formatting */
  for (i = 0; vorbis_comment_keys[i].key != NULL; i++) {

    if ( !strncasecmp (vorbis_comment_keys[i].key, comment,
		       strlen(vorbis_comment_keys[i].key)) ) {

      *offset = strlen(vorbis_comment_keys[i].key);
      s = malloc(strlen(vorbis_comment_keys[i].formatstr) + 1);
      if (s == NULL) {
	fprintf(stderr, _("ERROR: Out of memory.\n"));
	exit(1);
      };
      strcpy(s, vorbis_comment_keys[i].formatstr);
      return s;
    }

  }

  /* Use default formatting */
  j = strcspn(comment, "=");
  if (j) {
    *offset = j + 1;
    s = malloc(j + 2);
    if (s == NULL) {
      fprintf(stderr, _("ERROR: Out of memory.\n"));
      exit(1);
    };
    strncpy(s, comment, j);
    strcpy(s + j, ":");

    /* Capitalize */
    s[0] = toupper(s[0]);
    for (i = 1; i < j; i++) {
      s[i] = tolower(s[i]);
    };
    return s;
  }

  /* Unrecognized comment, use last format string */
  *offset = 0;
  s = malloc(strlen(vorbis_comment_keys[i].formatstr) + 1);
  if (s == NULL) {
    fprintf(stderr, _("ERROR: Out of memory.\n"));
    exit(1);
  };
  strcpy(s, vorbis_comment_keys[i].formatstr);
  return s;
}

void print_vorbis_comment (char *comment, decoder_callbacks_t *cb, 
			   void *callback_arg)
{
  char *comment_prettyprint;
  char *decoded_value;
  int offset;

  if (cb == NULL || cb->printf_metadata == NULL)
    return;

  comment_prettyprint = lookup_comment_prettyprint(comment, &offset);

  if (utf8_decode(comment + offset, &decoded_value) >= 0) {
    cb->printf_metadata(callback_arg, 1, "%s %s", comment_prettyprint, 
			decoded_value);
    free(decoded_value);
  } else
    cb->printf_metadata(callback_arg, 1, "%s %s", comment_prettyprint, 
			comment + offset);
  free(comment_prettyprint);
}
