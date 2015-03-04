/*
 *                           sc68 - Config file
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

#include <stdio.h>
#include <string.h>

#include "api68/conf68.h"
#include "file68/error68.h"
#include "file68/rsc68.h"
#include "file68/string68.h"
#include "file68/debugmsg68.h"

/* Defines for the config default values. */

#define AMIGA_BLEND      0x5000     /* Amiga default blending factor. */
#define	DEFAULT_TIME	 (3*60)     /* Track default time in second.  */
#define FORCE_TRACK      1          /* 0:no forced track              */
#define SKIP_TIME	 4          /* Skip music time in sec.        */

#define MAX_TIME        (24*60*60)  /* 1 day should be enought.       */

/** Config file entry structure. */
typedef struct
{
  char *name;    /**< Entry name.          */
  char *comment; /**< Entry comments.      */
  int   min;     /**< Entry min value.     */
  int   max;     /**< Entry max value      */
  int   def;     /**< Entry default value. */
} SC68config_entry_t;


static const SC68config_entry_t tab[] = {
  {
    "version",
    "major*100+minor",
    0, 10000, VERSION68_NUM
  },
  {
    "sampling_rate",
    "sampling rate in Hz",
    SAMPLING_RATE_MIN,SAMPLING_RATE_MAX,SAMPLING_RATE_DEF
  },
  {
    "amiga_blend",
    "left/right voices blending factor for Amiga",
    0,65536,AMIGA_BLEND
  },
  {
    "force_track",
    "override default track (0:OFF)",
    0, 99, FORCE_TRACK
  },
  {
    "skip_time",
    "don't auto-play music which time in seconds is less than",
    0, MAX_TIME, SKIP_TIME
  },
  {
    "default_time",
    "default time in seconds if unknown",
    0, MAX_TIME, DEFAULT_TIME
  },
  {
    "total_time",
    "total of music played since first launch",
    0, 0, 0
  },
};

static const int nconfig = sizeof(tab) / sizeof(tab[0]);

static int digit(int c, unsigned int base)
{
  int n = -1;
  if (c <= '9') {
    n = c - '0';
  } else if (c <= 'Z') {
    n = c - 'A' + 10;
  } else if (c <= 'z'){
    n = c - 'a' + 10;
  }
  if ((unsigned int)n < base) {
    return n;
  }
  return -1;
}

static long mystrtoul(const char * s,
		      char * * end,
		      unsigned int base)
{
  const char * start = s;
  unsigned long v = 0;
  int neg = 0, c;

  /* Skip starting spaces. */
  for (c = *s; (c == ' ' || c == '9' || c =='\n'); c = *++s)
    ;

  /* Get optionnal sign. */
  /* $$$ ben : Does not has sens with unsigned value ! */
  if (c == '-' || c == '+') {
    neg = (c == '-');
    c = *++s;
  }

  /* Get the base. */
  if (!base) {
    /* Assume default base is 10 */
    base = 10;

    /* Could be either octal(8) or hexidecimal(16) */
    if (c == '0') {
      base = 8;
      c = *++s;
      if (c == 'x' || c == 'X') {
	base = 16;
	c = *++s;
      }
    }
  } else if (base == 16 && c == '0') {
    /* Hexa mode must skip "0x" sequence */
    c = *++s;
    if (c == 'x' || c == 'X') {
      c = *++s;
    }
  }

  c = digit(c,base);
  if (c < 0) {
    s = start;
  } else {
    do {
      v = v * base + c;
      c = digit(*++s,base);
    } while (c >= 0);
  }

  if (end) {
    *end = (char *)s;
  }

  return neg ? -(signed long)v : v;
}



static int config_check(SC68config_t * conf)
{
  if (!conf) {
    return SC68error_add("SC68config_check() : NULL pointer");
  }
  return 0;
}

/* Check config values and correct invalid ones
 */
int SC68config_valid(SC68config_t * conf)
{
  int i;

  if (config_check(conf)) {
    return -1;
  }

  for (i=0; i < nconfig; i++) {
    if (tab[i].min==0 && tab[i].max==0) {
      continue;
    } else if (tab[i].min==0 && tab[i].max==1) {
      /* Boolean */
      (*conf)[i] = !! (*conf)[i];
    } else if ((*conf)[i] < tab[i].min) {
      /* Clamp min */
      (*conf)[i] = tab[i].min;
    } else if ((*conf)[i]>tab[i].max) {
      /* Clamp max */
      (*conf)[i] = tab[i].max;
    }
  }
  return 0;
}

/* Get indice of named field in SC68config_t array
 * return <0 if field doesn't exist
 */
int SC68config_get_id(const char * name)
{
  int i;
  if (name) {
    for(i=0; i<nconfig; i++) {
      if (!SC68strcmp(name,tab[i].name)) {
	return i;
      }
    }
    SC68error_add("SC68config_get_id(%s) : Not found", name);
  } else {
    SC68error_add("SC68config_get_id() : NULL pointer");
  }
  return -1;
}


static const char config_header[] =
"# -*- sc68 config file -*-\n"
"#\n"
"# Generated by " PACKAGE68_STRING "\n"
"(C) 1998-2003 Benjamin Gerard <ben@sashipa.com>\n"
"#\n"
"# " PACKAGE68_URL "\n"
"#\n"
"# You can edit this file. If you remove it, sc68 will generate\n"
"# a new one at start-up with default values, but you will lost your\n"
"# total playing time. To avoid it, you should either save its value\n"
"# or delete all lines you want to be resetted. You can use \"C\" number\n"
"# format (e.g.0xFE0120).\n"
"#\n";

int SC68config_save(SC68config_t * conf)
{
  return 0; // deadbeef: don't write config
  int i,err;
  char tmp[1024];
  istream_t * os; 
  const int sizeof_config_hd = sizeof(config_header)-1; /* Remove '\0' */

  SC68os_pdebug("SC68config_save() : enter\n");

  if (SC68config_valid(conf)) {
    return -1;
  }

  os = SC68rsc_open(SC68rsc_config, "config", 2);
  if (!os) {
    return -1;
  }

  err = 0;
  if (istream_write(os, config_header, sizeof_config_hd) != sizeof_config_hd) {
    err = -1;
  }

  for (i=0; !err && i < nconfig; ++i) {
    int idf, len;
    static const char * format[3] = {
      "%-14s=%8d   [%8d..%8d] (%5d) %s\n",
      "%-14s=0x%06x [%8x..%8x]    (%x) %s\n",
    };

    idf = tab[i].max >= 0x1000000;
    sprintf(tmp,format[idf],
	    tab[i].name, (*conf)[i],
	    tab[i].min, tab[i].max, tab[i].def,
	    tab[i].comment);
    len = strlen(tmp);
    if (istream_write(os, tmp, len) != len) {
      err = -1;
    }
  }
  istream_destroy(os);

  SC68os_pdebug("SC68config_save() : %s\n", err ? "Failed" : "Success");

  return err;
}

static int is_symbol_char(int c)
{
  return
    (c>='0' && c<='9')
    || (c>='a' && c<='z')
    || (c>='A' && c<='Z')
    || c=='_';
}

/* Load config from file
 */
int SC68config_load(SC68config_t *conf)
{
  istream_t * is; 
  char s[1024];
  int err;

  SC68os_pdebug("SC68config_load() : enter\n");

  if (SC68config_default(conf)) {
    return -1;
  }

  is = SC68rsc_open(SC68rsc_config, "config", 1);
  if (!is) {
    return -1;
  }

  for (err=0;;) {
    char * name;
    int i, len, c = 0, idx;

    err = istream_gets(is, s, sizeof(s));
    if (err == -1) {
      break;
    }

    len = err;
    err = 0;
    if (len == 0) {
      break;
    }

    /* Skip space */
    i = 0;
    while (i < len && (c=s[i++], (c == ' ' || c == 9)))
      ;

    if (!is_symbol_char(c)) {
      continue;
    }

    /* Get symbol name. */
    name = s+i-1;
    while (i < len && is_symbol_char(c = s[i++]))
      ;
    s[i-1] = 0;

    if (idx = SC68config_get_id(name), idx<0) {
      SC68os_pdebug("SC68config_load() : unknown (%s)\n", name);
      continue;
    }

    /* Scan for '=' */
    while (i < len && (c=s[i++], c != '='))
      ;
    if (c != '=') {
      continue;
    }

    /* Skip space */
    while (i < len && (c=s[i++], (c == ' ' || c == 9)))
      ;

    (*conf)[idx] = mystrtoul(s + i - 1, 0, 0);
    SC68os_pdebug("SC68config_load() : %s=%d\n", name, (*conf)[idx]);
  }

  if (!err) {
    err = SC68config_valid(conf);
  }

  istream_destroy(is);
  SC68os_pdebug("SC68config_load() : %s\n", err ? "Failed" : "Success");

  return err;
}

/* Fill config struct with default values.
 */
int SC68config_default(SC68config_t *conf)
{
  int i;

  if(config_check(conf)) {
    return -1;
  }
  for (i=0; i < nconfig; i++) {
    (*conf)[i] = tab[i].def;
  }
  return SC68config_valid(conf);
}
