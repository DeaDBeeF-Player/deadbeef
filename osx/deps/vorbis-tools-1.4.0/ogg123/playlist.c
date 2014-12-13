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

 last mod: $Id: playlist.c 16953 2010-03-07 03:06:29Z conrad $

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "playlist.h"
#include "i18n.h"

/* Work with *BSD differences */
#if !defined(NAME_MAX) && defined(MAXNAMLEN) 
#define NAME_MAX MAXNAMLEN
#endif

playlist_element_t *playlist_element_create(char *filename)
{
  playlist_element_t *element = (playlist_element_t *) 
                                   malloc(sizeof(playlist_t));

  if (element == NULL) {
      fprintf(stderr,
	      _("ERROR: Out of memory in create_playlist_member().\n"));
      exit(1);
  }

  if (filename == NULL)
    element->filename = NULL;
  else {
    element->filename = strdup(filename);

    if (element->filename == NULL) {
	fprintf(stderr,
		_("ERROR: Out of memory in create_playlist_member().\n"));
	exit(1);
    }
  }

  element->next = NULL;

  return element;
}


/* Only destroys the current node.  Does not affect linked nodes. */
void playlist_element_destroy(playlist_element_t *element)
{
  free(element->filename);
  free(element);
}



playlist_t *playlist_create()
{
  playlist_t *list = (playlist_t *) malloc(sizeof(playlist_t));

  if (list != NULL) {
    list->head = playlist_element_create(NULL);
    list->last = list->head;
  }

  return list;
}


void playlist_destroy(playlist_t *list) {
  playlist_element_t *next_element;

  while (list->head != NULL) {
    next_element = list->head->next;

    playlist_element_destroy(list->head);

    list->head = next_element;
  }

  free(list);
}


/* All of the playlist_append_* functions return 
   1 if append was successful
   0 if failure (either directory could not be accessed or playlist on disk
   could not be opened)
*/


/* Add this filename to the playlist.  Filename will be strdup()'ed.  Note
   that this function will never fail. */
int playlist_append_file(playlist_t *list, char *filename)
{
  list->last->next = playlist_element_create(filename);
  list->last = list->last->next;

  return 1; /* No way to fail */
}

/* Recursively adds files from the directory and subdirectories */
#if defined(HAVE_ALPHASORT) && defined(HAVE_SCANDIR)
int playlist_append_directory(playlist_t *list, char *dirname)
{
  int dir_len = strlen(dirname);
  int num_entries = 0, i = 0;
  struct dirent **entries;
  struct stat stat_buf;
  char nextfile[NAME_MAX + 1];

  num_entries = scandir(dirname, &entries, 0, alphasort);

  if (num_entries < 0) {
    return 0;
  }

  for (i=0; i<num_entries; i++) {
    int sub_len = strlen(entries[i]->d_name);

    /* Make sure full pathname is within limits and we don't parse the
       relative directory entries. */
    if (dir_len + sub_len + 1 < NAME_MAX 
        && strcmp(entries[i]->d_name, ".") != 0  
        && strcmp(entries[i]->d_name, "..") != 0  ) {

      /* Build the new full pathname */
      strcpy(nextfile, dirname);
      strcat(nextfile, "/");
      strcat(nextfile, entries[i]->d_name);

      if (stat(nextfile, &stat_buf) == 0) {
        
        /* Decide what type of entry this is */
        if (S_ISDIR(stat_buf.st_mode)) {
          
        /* Recursively follow directories */
          if ( playlist_append_directory(list, nextfile) == 0 ) {
            fprintf(stderr,
                  _("Warning: Could not read directory %s.\n"), 
                    nextfile);
          }
        } else {
        /* Assume everything else is a file of some sort */
          playlist_append_file(list, nextfile);
        }
      }
    }

    free(entries[i]);
  }

  free(entries);

  return 1;
}
#else
int playlist_append_directory(playlist_t *list, char *dirname)
{
  DIR *dir;
  int dir_len = strlen(dirname);
  struct dirent *entry;
  struct stat stat_buf;
  char nextfile[NAME_MAX + 1];

  dir = opendir(dirname);

  if (dir == NULL) {
    return 0;
  }

  entry = readdir(dir);
  while (entry != NULL) {
    int sub_len = strlen(entry->d_name);

    /* Make sure full pathname is within limits and we don't parse the
       relative directory entries. */
    if (dir_len + sub_len + 1 < NAME_MAX 
	&& strcmp(entry->d_name, ".") != 0  
	&& strcmp(entry->d_name, "..") != 0  ) {

      /* Build the new full pathname */
      strcpy(nextfile, dirname);
      strcat(nextfile, "/");
      strcat(nextfile, entry->d_name);

      if (stat(nextfile, &stat_buf) == 0) {
	
	/* Decide what type of entry this is */
	if (S_ISDIR(stat_buf.st_mode)) {
	  
	/* Recursively follow directories */
	  if ( playlist_append_directory(list, nextfile) == 0 ) {
	    fprintf(stderr,
		  _("Warning: Could not read directory %s.\n"), 
		    nextfile);
	  }
	} else {
	/* Assume everything else is a file of some sort */
	  playlist_append_file(list, nextfile);
	}
      }
    }

    entry = readdir(dir);
  }

  closedir(dir);

  return 1;
}
#endif


/* Opens a file containing filenames, one per line, and adds them to the
   playlist */
int playlist_append_from_file(playlist_t *list, char *playlist_filename)
{
  FILE *fp;
  char filename[NAME_MAX+1];
  struct stat stat_buf;
  int length;
  int i;

  if (strcmp(playlist_filename, "-") == 0)
    fp = stdin;
  else
    fp = fopen(playlist_filename, "r");

  if (fp == NULL)
    return 0;

  while (!feof(fp)) {

    if ( fgets(filename, NAME_MAX+1 /* no, really! */, fp) == NULL )
      continue;

    filename[NAME_MAX] = '\0'; /* Just to make sure */
    length = strlen(filename);

    /* Skip blank lines */
    for (i = 0; i < length && isspace(filename[i]); i++);
    if (i == length)
      continue;

    /* Crop off trailing newlines if present. Handle DOS (\r\n), Unix (\n)
     * and MacOS<9 (\r) line endings. */
    if (filename[length - 2] == '\r' && filename[length - 1] == '\n')
      filename[length - 2] = '\0';
    else if (filename[length - 1] == '\n' || filename[length - 1] == '\r')
      filename[length - 1] = '\0';

    if (stat(filename, &stat_buf) == 0) {

      if (S_ISDIR(stat_buf.st_mode)) {
	if (playlist_append_directory(list, filename) == 0)
	  fprintf(stderr, 
		  _("Warning from playlist %s: "
		    "Could not read directory %s.\n"), playlist_filename,
		  filename);
      } else {
	playlist_append_file(list, filename);
      }
    } else /* If we can't stat it, it might be a non-disk source */
      playlist_append_file(list, filename);

  }

  return 1;
}


/* Return the number of items in the playlist */
int playlist_length(playlist_t *list)
{
  int length;
  playlist_element_t *element;

  element = list->head;
  length = 0; /* don't count head node */
  while (element->next != NULL) {
    length++;
    element = element->next;
  }

  return length;
}


/* Convert the playlist to an array of strings.  Strings are deep copied. 
   Size will be set to the number of elements in the array. */
char **playlist_to_array(playlist_t *list, int *size)
{
  char **array;
  int i;
  playlist_element_t *element;

  *size = playlist_length(list);
  array = calloc(*size, sizeof(char *));

  if (array == NULL) {
    fprintf(stderr,
	    _("ERROR: Out of memory in playlist_to_array().\n"));
    exit(1);
  }

  for (i = 0, element = list->head->next; 
       i < *size; 
       i++, element = element->next) {

    array[i] = strdup(element->filename);

    if (array[i] == NULL) {
      fprintf(stderr,
	      _("ERROR: Out of memory in playlist_to_array().\n"));
      exit(1);
    }
  }


  return array;
}


/* Deallocate array and all contained strings created by playlist_to_array. */
void playlist_array_destroy(char **array, int size)
{
  int i;

  for (i = 0; i < size; i++)
    free(array[i]);

  free(array);
}
