#include "ossupport.h"

#include "unixsupport.c"
/* This module was written by Heikki Orsila <heikki.orsila@iki.fi> 2000-2005.
 * No copyrights claimed, so this module is in Public Domain (only this
 * code module). See OpenBSD man pages for strlcat and strlcpy
 */

#include <string.h>

size_t strlcpy(char *dst, const char *src, size_t size)
{
  size_t slen = strlen(src);
  if(slen < size)
    strcpy(dst, src);
  else if (size > 0) {
    strncpy(dst, src, size-1);
    dst[size-1] = 0;
  }
  return slen;
}


size_t strlcat(char *dst, const char *src, size_t size)
{
  size_t slen = strlen(src);
  size_t dlen = 0;
  while(dlen < size) {
    if(dst[dlen] == 0)
      break;
    dlen++;
  }

  if(dlen == size) {
    return slen + dlen;
  }

  if((dlen + slen) < size)
    strcat(dst, src);
  else {
    int left = size - dlen - 1;
    if(left > 0) {
      strncat(dst, src, left);
    }
    dst[size-1] = 0;
  }
  return slen + dlen;
}
