#ifndef _UADE_SUPPORT_H_
#define _UADE_SUPPORT_H_

#include <stdio.h>

#define UADE_LINESIZE 1024

#define uadeerror(fmt, args...) do { fprintf(stderr, "uade: " fmt, ## args); exit(1); } while (0)

#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))


char *xbasename(const char *path);

int get_two_ws_separated_fields(char **key, char **value, char *s);

int skipnws(const char *s, int i);

int skip_and_terminate_word(char *s, int i);

int skipws(const char *s, int i);

char **read_and_split_lines(size_t *nitems, size_t *lineno, FILE *f,
			    const char *delim);

/* Same as fgets(), but guarantees that feof() or ferror() have happened
   when xfgets() returns NULL */
char *xfgets(char *s, int size, FILE *stream);

#endif
