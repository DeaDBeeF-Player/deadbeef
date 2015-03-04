/*
 * @file    config.c
 * @brief   sc68 configuration dialog
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

/* generated config header */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* libc */
#include <string.h>
#include <stdlib.h>

/* sc68 */
#include "sc68/sc68.h"
#include "sc68/file68_opt.h"
#include "sc68/file68_msg.h"
#include "dial68.h"

static const int magic = ('C'<<24)|('N'<<16)|('F'<<8)|'G';

struct dial_s {
  dial68_t    dial;                     /* base dialog */
};
typedef struct dial_s dial_t;

static inline int ismagic(dial_t * dial)
{
 return
   dial &&
   dial->dial.dial == magic &&
   dial->dial.size == sizeof(*dial) &&
   dial->dial.cntl
   ;
}

static inline void del_dial(dial_t * dial)
{
  free(dial);
}

static int getopt(const char * key, int op, sc68_dialval_t * val)
{
  option68_t * opt = option68_get(key, opt68_ALWAYS);
  const int org = opt68_CFG;

  if (opt) {
    const int isset = opt->org != opt68_UDF;

    TRACE68(dial_cat,
            P "Got an option \"%s\" %sset opcode:%d\n",
            key, isset?"":"not ", op);

    switch (op) {

    case SC68_DIAL_GETI:
      if (isset && (opt->type == opt68_BOL ||
                    opt->type == opt68_INT ||
                    opt->type == opt68_ENU)) {
        val->i = opt->val.num;
        return 0;
      }
      break;

    case SC68_DIAL_GETS:
      if (isset && opt->type == opt68_STR) {
        val->s = opt->val.str;
        return 0;
      }
      break;

    case SC68_DIAL_ENUM:
      if (isset && opt->type == opt68_ENU &&
          val->i >= 0 && val->i < (int)opt->sets) {
        val->s = val->i[(const char **)opt->set];
        return 0;
      }
      break;

    case SC68_DIAL_SETI:
      return option68_iset(opt, val->i, opt68_ALWAYS, org);

    case SC68_DIAL_SETS:
      return option68_set(opt, val->s, opt68_ALWAYS, org);

    case SC68_DIAL_MIN:
      val->i = opt->min;
      return 0;

    case SC68_DIAL_MAX:
      val->i = opt->max;
      return 0;

    case SC68_DIAL_CNT:
      val->i = opt->sets;
      return 0;

    case SC68_DIAL_CAT:
      val->s = opt->cat;
      return 0;

    case SC68_DIAL_DESC:
      val->s = opt->desc;
      return 0;


    }
  }

  return -1;
}

#define keyis(N) !strcmp(key,N)

static int conf(void * data, const char * key, int op, sc68_dialval_t *val)
{
  dial_t * dial = (dial_t *) data;
  int res = -1;

  static const char * l_spr[] = {
    "< custom >",
    "11 khz","22 khz", "44.1 khz","48 khz","96 khz"
  };
  static const int i_spr[] = {
    0,
    11025, 22050, 44100, 48000, 96000
  };
  const int sprmax = sizeof (l_spr) / sizeof (*l_spr);

  /* Sanity check */
  if (!key || !ismagic(dial))
    goto exit;

  /* Run user control function */
  res = dial->dial.cntl(dial->dial.data, key, op, val);

  /* Kill special case. */
  if (op == SC68_DIAL_CALL && !strcmp(key,SC68_DIAL_KILL)) {
    del_dial(dial);
    goto exit;
  }

  /* User dealt with that message. */
  if (res <= 0)
    goto exit;

  /* Assume no error */
  res = 0;

  if (keyis("sampling")) {
    /* This key is used for the predefined sampling rate */

    switch (op) {

    case SC68_DIAL_CNT:
      val->i = sprmax;
      break;

    case SC68_DIAL_GETI: {
      const option68_t * opt
        = option68_get("sampling-rate", opt68_ISSET);
      if (!opt)
        val->i = 5;                     /* default to 48khz */
      else {
        for (val->i=1; val->i<sprmax; ++val->i)
          if (i_spr[val->i] == opt->val.num)
            break;
        if (val->i >= sprmax)
          val->i = 0;
      }
    } break;

    case SC68_DIAL_ENUM:
      if (val->i >= 0 && val->i < sprmax) {
        val->s = l_spr[val->i];
        break;
      }

    default:
      res = -1;
    }

  }
  else if (op == SC68_DIAL_CALL) {
    /* Other special calls like real-tiem access */
    if (keyis(SC68_DIAL_NEW))
      val->i = 0;
    else if (keyis("save"))
      val->i = sc68_cntl(0, SC68_CONFIG_SAVE);
    else if (keyis("amiga-filter"))
      /* $$$ TODO: realtime amiga-filter */
      val->i = !!val->i;
    else if (keyis("amiga-blend"))
      /* $$$ TODO: realtime amiga-blend */
      val->i = val->i;
  }
  else {
    /* Finally try it as an option key.
     *
     * "sampling-rate" is converted if the value is in the index range
     *                 instead of the frequency value range.
     */
    if (op == SC68_DIAL_SETI && keyis("sampling-rate")
        && val->i > 0 && val->i < sprmax)
        val->i = i_spr[val->i];
    res = getopt(key, op, val);
  }

exit:
  TRACE68(dial_cat, P
          "%s() #%02d \"%s\" -> %d\n", __FUNCTION__, op, key, res);
  return res;
}

SC68_EXTERN
int dial68_new_conf(void ** pdata, sc68_dial_f * pcntl)
{
  int res = -1;
  const int size = sizeof(dial_t);
  dial_t * dial = (dial_t *) malloc(size);

  if (dial) {
    dial->dial.dial = magic;
    dial->dial.size = size;
    dial->dial.data = *pdata;
    dial->dial.cntl = *pcntl;
    *pcntl = conf;
    *pdata = dial;
    res = 0;
  }
  TRACE68(dial_cat, P "%s -> %p %d\n", __FUNCTION__, (void*)dial, res);
  return res;
}
