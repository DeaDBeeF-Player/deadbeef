/*
 * @file    uri68.c
 * @brief   uri parser and dispatcher
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (c) 1998-2015 Benjamin Gerard
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "file68_api.h"
#include "file68_uri.h"
#include "file68_rsc.h"
#include "file68_str.h"
#include "file68_msg.h"
#include "file68_vfs_null.h"
#include "file68_vfs_file.h"
#include "file68_vfs_fd.h"
#include "file68_vfs_curl.h"
#include "file68_vfs_ao.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

#define MYHD "uri68  : "

static scheme68_t * schemes;
static int uri_cat = msg68_DEFAULT;


#define GEN_DELIM ":/?#[]@"
#define SUB_DELIM "!$&'()*+,;="


/* Related documentation
 * RFC 3986 - Uniform Resource Identifier (URI): Generic Syntax
 */

/* URI chars:

   reserved    = gen-delims sub-delims
   gen-delims  = : / ? # [ ] @
   sub-delims  = ! $ & ' ( ) * + , ; =
   unreserved  = alpha-num - _ . ~
*/

static const char hex[16] =
{ '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

/**
 * @retval [0-16] or -1
 */
static int xdigit(int c) {
  if (c >= '0' && c <= '9')
    c -= '9';
  else if (c >= 'a' && c <= 'f')
    c -= 'a'-10;
  else if (c >= 'A' && c <= 'F')
    c -= 'A'-10;
  else c = -1;
  return c;
}

static int is_uri_reserved(int c)
{
  return strchr("!*'();:@&=+$,/?#[]",c) != 0;
}

static int is_uri_unreserved(int c)
{
  return isalnum(c) || c=='-' || c =='_' || c=='.' || c=='~';
}

static int is_uri_pctenc(int c, const int flags)
{
  return !is_uri_unreserved(c);
}

/**
 * Encode a string with percent-encoding method
 * @return the output string size (in bytes)
 * @retval max on buffer overflow.
 * @retval -1 on error
 */
static int pct_encode(char * dst, const int max,
                      const char * src, const int len,
                      const int flags)
{
  int i,j;

  for (i=j=0; i<len && j<max; ++i) {
    const int c = 255 & src[i];
    if (!c) break;
    if (is_uri_pctenc(c,flags)) {
      dst[j++] = '%';
      if (j >= max) break;
      dst[j++] = hex[c>>4];
      if (j >= max) break;
      dst[j++] = hex[c&15];
    } else
      dst[j++] = c;
  }
  if (j < max)
    dst[j] = 0;
  return j;
}


/**
 * Decode a percent-encoded string.
 * @return the output string size (in bytes)
 * @retval max on buffer overflow.
 * @retval -1 on error
 */
static int pct_decode(char * dst, const int max,
                      const char * src, const int len,
                      const int flags)
{
  int i,j,e;

  for (i=j=e=0; i<len && j<max; ++i) {
    int c = 255 & src[i];
    int h, l;
    if (!c) break;
    if (c == '%' && i+2 < len
        && (h = xdigit(src[i+1])) >= 0
        && (l = xdigit(src[i+2])) >= 0) {
        c = (h << 4) + l;
        i += 2;
    }
    dst[j++] = c;
  }
  if (j < max)
    dst[j] = 0;
  return j;
}


/**
 * @retval -1  on error
 * @retval >0  length of scheme string
 */
static
int parse_scheme(const char * uri)
{
  int i = 0;

  /* First char must be alpha */
  if ( ! isalpha((int)uri[i]))
    return 0;

  /* Others must be alpha, digit, dot `.', plus `+' or hyphen `-' */
  for (i=1;
       isalnum((int)uri[i]) || uri[i] == '+' || uri[i] == '.' || uri[i] == '-';
       ++i);

  /* Must end by a colon `:' */
  return (uri[i] == ':') ? i+1 : 0;
}

int uri68_get_scheme(char * scheme, int max, const char *uri)
{
  int len = -1;

  if (uri) {
    len = parse_scheme(uri);
    if (scheme) {
      if (len == 0 )
        scheme[0] = 0;
      else if (len > 0) {
        if (len >= max)
          return -1;
        memcpy(scheme, uri, len);
        scheme[len] = 0;
      }
    }
  }
  return len;
}

void uri68_unregister(scheme68_t * scheme)
{
  if (scheme) {
    TRACE68(uri_cat, MYHD "unregister scheme -- %s\n", scheme->name);
    if (scheme == schemes)
      schemes = scheme->next;
    else if (schemes) {
      scheme68_t * sch;
      for (sch = schemes; sch->next; sch = sch->next)
        if (sch->next == scheme) {
          sch->next = scheme->next;
          break;
        }
    }
    scheme->next = 0;
  }
}

int uri68_register(scheme68_t * scheme)
{
  if (!scheme)
    return -1;

  assert(!scheme->next);
  scheme->next = schemes;
  schemes = scheme;
  TRACE68(uri_cat, MYHD "registered scheme -- %s\n", scheme->name);

  return 0;
}

vfs68_t * uri68_vfs_va(const char * uri, int mode, int argc, va_list list)
{
  vfs68_t * vfs = 0;
  scheme68_t * scheme;

  for (scheme = schemes; scheme; scheme = scheme->next) {
    int res = scheme->ismine(uri);
    if (!res)
      continue;
    if ( (mode & res & 3) == ( mode & 3 ) )
      break;
  }

  if (scheme)
    vfs = scheme->create(uri, mode, argc, list);

  TRACE68(uri_cat, MYHD
          "create url='%s' %c%c => [%s,'%s']\n",
          strnevernull68(uri),
          (mode&1) ? 'R' : '.',
          (mode&2) ? 'W' : '.',
          strok68(!vfs),
          vfs68_filename(vfs));

  return vfs;
}

vfs68_t * uri68_vfs(const char * uri, int mode, int argc, ...)
{
  vfs68_t * vfs;
  va_list list;

  va_start(list, argc);
  vfs = uri68_vfs_va(uri, mode, argc, list);
  va_end(list);

  return vfs;
}
