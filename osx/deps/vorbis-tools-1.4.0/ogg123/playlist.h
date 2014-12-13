/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2002                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: playlist.h 16825 2010-01-27 04:14:08Z xiphmont $

 ********************************************************************/

#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

typedef struct playlist_element_t {
  char *filename;
  struct playlist_element_t *next;
} playlist_element_t;

/* Actual playlist structure */
typedef struct playlist_t {

  /* Linked list with empty head node */
  playlist_element_t *head;

  /* Keep track of this for speedy appends */
  playlist_element_t *last;
} playlist_t;

playlist_t *playlist_create();
void playlist_destroy(playlist_t *list);

/* All of the playlist_append_* functions return 
   1 if append was successful
   0 if failure (either directory could not be accessed or playlist on disk
   could not be opened)
*/


/* Add this filename to the playlist.  Filename will be strdup()'ed.  Note
   that this function will never fail. */
int playlist_append_file(playlist_t *list, char *filename);

/* Recursively adds files from the directory and subdirectories */
int playlist_append_directory(playlist_t *list, char *dirname);

/* Opens a file containing filenames, one per line, and adds them to the
   playlist */
int playlist_append_from_file(playlist_t *list, char *playlist_filename);

/* Return the number of items in the playlist */
int playlist_length(playlist_t *list);

/* Convert the playlist to an array of strings.  Strings are deep copied. 
   Size will be set to the number of elements in the array. */
char **playlist_to_array(playlist_t *list, int *size);

/* Deallocate array and all contained strings created by playlist_to_array. */
void playlist_array_destroy(char **array, int size);


#endif /* __PLAYLIST_H__ */
