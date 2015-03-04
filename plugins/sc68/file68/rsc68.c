/*
 *                         sc68 - Resource access
 *         Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
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

#include <config68.h>

#include <string.h>
#include <stdlib.h>

#include "file68/error68.h"
#include "file68/rsc68.h"
#include "file68/string68.h"
#include "file68/alloc68.h"
#include "file68/istream68_file.h"
#include "file68/debugmsg68.h"

static const char * default_share_path = SC68_SHARED_DATA_PATH;
static const char * share_path = 0;
static const char * user_path = 0;

static istream_t * default_open(SC68rsc_t type, const char *name, int mode);
static SC68rsc_handler_t rsc68 = default_open;

#ifndef HAVE_GETENV
static char * getenv(const char *name)
{
  return 0;
}
#endif

static void set_default_share(void)
{
  const char * path;

  /* Set SHARE resource path */
  if (path = getenv("SC68_DATA"), !path) {
    path = default_share_path;
  }
  SC68rsc_set_share(path);
  SC68os_pdebug("set_default_share() := [%s] \n", share_path);
}

static void set_default_user(void)
{
  char * path;

  /* Set SHARE resource path */
  if (path = getenv("SC68_HOME"), path) {
    SC68rsc_set_user(path);
  } else {
    path = getenv("HOME");
    if (path) {
      char * npath = SC68strcatdup(path,"/.sc68");
      if (npath) {
	SC68free((char *)user_path);
	user_path = npath;
      }
    }
  }
  SC68os_pdebug("set_default_user() := [%s] \n", user_path);
}

static const char * rsc_set_any(const char ** any, const char * path)
{
  SC68free((char *)*any);
  return *any = SC68strdup(path) ;
}

const char * SC68rsc_set_share(const char *path)
{
  return rsc_set_any(&share_path, path);
}

const char * SC68rsc_set_user(const char *path)
{
  return rsc_set_any(&user_path, path);
}

void SC68rsc_get_path(const char **share, const char **user)
{
  if (!share_path) {
    set_default_share();
  }
  if (!user_path) {
    set_default_user();
  }
  if (share) *share = share_path;
  if (user) *share = user_path;
}


static void * mystrtolower(void *dest, const void *src, size_t n)
{
  char *d = dest;
  const char *s = src;
  size_t i;

  for (i=0; i<n; ++i) {
    int c = s[i];
    if (c>='A' && c<='Z') c = c - 'A' + 'a';
    d[i] = c;
  }
  return d;
}

static istream_t * default_open(SC68rsc_t type, const char *name, int mode)
{
  istream_t * is = 0;
  int err = -1;
  const char *cpath, *subdir = 0, *ext = 0;
  char tmp[1024], * apath = 0;
  int alen = 0;
  void * (*mycpy)(void *, const void *, size_t) = memcpy;

  SC68os_pdebug("default_open(%d,%s,%d) : enter\n", type,name, mode);

  /* Get subdir and optionnal extension. */
  switch (type) {
  case SC68rsc_replay:
    subdir = "/Replay/";
    mycpy = mystrtolower; /* $$$ transform replay name to lower case. */
    ext = ".bin";
    break;
    
  case SC68rsc_config:
    subdir = "/";
    ext = ".txt";
    break;

  case SC68rsc_sample:
    subdir = "/Sample/";
    break;

  case SC68rsc_dll:
    subdir = "/Dll/";
    break;
  }
  SC68os_pdebug("default_open() : subdir = %s\n", subdir);

  if (!subdir) {
    SC68error_add("SC68rsc_open(%d,%s,%d) : invalid resource type",
		  type, name, mode);
    return 0;
  }

  cpath = user_path;
  for (;;) {

    SC68os_pdebug("default_open() : trying '%s'\n", cpath);

    if (cpath) {
      char * path;
      int clen = strlen(cpath);
      int slen = strlen(subdir);
      int nlen = strlen(name);
      int elen = ext ? strlen(ext) : 0;
      int len = clen + slen + nlen + elen + 1;

      if (len <= alen) {
	path = apath;
      } else if (len  <= sizeof(tmp)) {
	path = tmp;
      } else {
	SC68free(apath);
	apath = SC68alloc(len);
	alen = apath ? len : 0;
	path = apath;
      }

      if (path) {
	char * p = path;
	/* Build path. */
	memcpy(p, cpath, clen); p += clen;
	memcpy(p, subdir, slen); p += slen;
	mycpy(p, name, nlen); p += nlen;
	memcpy(p, ext, elen); p += elen;
	*p = 0;
	SC68os_pdebug("default_open() : path '%s'\n", path);

	is = istream_file_create(path, mode);
	if (!is) {
	  SC68os_pdebug("default_open() : istream create failed\n");
	}

	err = istream_open(is);
	if (!err) {
	  SC68os_pdebug("default_open() : ok for '%s'\n", path);
	  break;
	} else {
	  SC68os_pdebug("default_open() : failed '%s'\n", path);
	  istream_destroy(is);
	  is = 0;
	}
      } else {
	SC68error_add("SC68rsc_open(%d,%s,%d) : path allocation failed",
		      type, name, mode);
	break;
      }
    }

    if (mode == 2 || cpath == share_path) {
      /* Write mode : no further snoop. */
      SC68error_add("SC68rsc_open(%d,%s,%d) : not found", type, name, mode);
      break;
    }
    cpath = share_path;
  }

  if (apath != tmp) {
    SC68free(apath);
  }
  if (err) {
    istream_destroy(is);
    is = 0;
  }
  return is;
}


SC68rsc_handler_t SC68rsc_set_handler(SC68rsc_handler_t fct)
{
  SC68rsc_handler_t old;

  old = rsc68;
  if (fct) {
    rsc68 = fct;
  }
  return old;
}


istream_t * SC68rsc_open(SC68rsc_t type, const char *name, int mode)
{
  if (!name) {
    SC68error_add("share_path() : NULL resource name");
    return 0;
  }

  if (mode < 1 || mode > 2) {
    SC68error_add("share_path(%s) : invalid open mode (%d)",name, mode);
    return 0;
  }

  if (!rsc68) {
    SC68error_add("share_path(%s) : NULL resource handler");
    return 0;
  }

  if (!share_path) {
    set_default_share();
  }

  if (!user_path) {
    set_default_user();
  }

  return rsc68(type, name, mode);
}

#ifdef __cplusplus
}
#endif
