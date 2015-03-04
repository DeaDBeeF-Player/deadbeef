/*
 *                 sc68 - String utility functions
 *         Copyright (C) 2001 Ben(jamin) Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "file68/string68.h"
#include "file68/alloc68.h"
 
#include <stdio.h>
#include <string.h>

/* Convert letter to uppercase. */
int SC68toupper(int c)
{
  return (c < 'a' || c > 'z') ? c : (c-'a'+'A');
}

/* Convert letter to uppercase. */
int SC68tolower(int c)
{
  return (c < 'A' || c > 'Z') ? c : (c-'A'+'a');
}


/*  Compare two string without case.
 */
int SC68strcmp(const char *a, const char *b)
{
  int av, bv;
  /* This is not very smart ... but what to do ? */
  if (a == b)
    return 0;
  if (!a)
    return -1;
  if (!b)
    return 1;
  for (av = SC68toupper(*a), bv = SC68toupper(*b); av && av == bv; a++, b++) {
    av = SC68toupper(*a);
    bv = SC68toupper(*b);
  }
  return av - bv;
}

/*  "a+b" with sizeof("a+b") <= l
 */
char * SC68strcat(char * a, const char * b, int l)
{
  int n;
  if (!a || l < 0)
    return 0;
  if (!b)
    return a;
  for (n = strlen(a); n < l && *b; n++, b++)
    a[n] = *b;
  if (n < l)
    a[n] = 0;
  return a;
}

/* Duplicate s */
char * SC68strdup(const char * s)
{
  char * d = 0;

  if (s) {
    int len = strlen(s) + 1;

    d = SC68alloc(len);
    if (d) {
      int i;
      for (i=0; i<len; ++i) {
	d[i] = s[i];
      }
    }
  }
  return d;
}

/* Duplicate a+b */
char * SC68strcatdup(const char * a, const char * b)
{
  if (!a) {
    return SC68strdup(b);
  } else if (!b) {
    return SC68strdup(a);
  } else {
    int lena = strlen(a), lenb = strlen(b);
    char * d = SC68alloc(lena+lenb+1);
    if (d) {
      int i = 0;
      while (lena--) {
	d[i++] = *a++;
      }
      while (lenb--) {
	d[i++] = *b++;
      }
      d[i] = 0;
    }
    return d;
  }
}

/* Construct a track-time info string
 * Format "TK MN:SS"
 * track_num : 01 to 99 or <=0 to deactivate
 * seconds   : 00 to 5999 [99'59"] or  <0 to deactivate
 */
char * SC68time_str(char * buffer, int track_num, int seconds)
{
  static char tmp[12] = "-- --:--";

  if (!buffer) {
    buffer = tmp;
  }

  if (track_num < 0) {
    track_num = 0;
  } else if (track_num > 99) {
    track_num = 99;
  }

  if (!track_num) {
    buffer[0] = buffer[1] = '-';
  } else {
    buffer[0] = '0' + track_num / 10;
    buffer[1] = '0' + track_num % 10;
  }
  buffer[2] = ' ';

  if (seconds < 0) {
    seconds = -1;
  } else if (seconds > 99 * 60 + 59) {
    seconds = 99 * 60 + 59;
  }

  if (seconds < 0) {
    strcpy(buffer + 3, "--:--");
  } else {
    sprintf(buffer + 3, "%02u:%02u", seconds / 60, seconds % 60);
  }

  buffer[8] = 0;
  return buffer;
}

/* Construct a playing-time info string from a tine in seconds
 * Format [D days, ][H h, ] MN' SS"
 *        0 => none
 */
char * SC68long_time_str(char * buffer, int time)
{
  int sec, min, hr, day;
  static char tmp[32], *s;

  s = buffer ? buffer : tmp;

  if (time <= 0) {
    return strcpy(s, "none");
  }

  sec = time % 60;
  time /= 60;
  min = time % 60;
  time /= 60;
  hr = time % 24;
  time /= 24;
  day = time;

  if (day) {
    sprintf(s, "%d day%s, %2dh, %02d' %02d\"", day, day > 1 ? "s" : "", hr,
	    min, sec);
  } else if (hr) {
    sprintf(s, "%2dh, %02d' %02d\"", hr, min, sec);
  } else {
    sprintf(s, "%02d' %02d\"", min, sec);
  }
  return s;
}
