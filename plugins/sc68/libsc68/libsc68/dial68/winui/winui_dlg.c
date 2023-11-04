/*
 * @file    winui_dlg.c
 * @brief   sc68 dialogs for Windows systems
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

/* sc68 */
#include "sc68/sc68.h"                  /* for sc68_dial_f */

/* stdc */
#include <assert.h>

/* Used to glue the Windows resource into the library (using partial linking)
 * see <https://sourceware.org/bugzilla/show_bug.cgi?id=17196>
 */
SC68_EXTERN volatile int dial68_rc_force;

/* Needed for UDM32 family defines */
#ifndef WINVER
# define WINVER 0x0505
#endif
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0400
#endif
#ifndef _WIN32_IE
# define _WIN32_IE 0x0500
#endif

/* windows */
#include <windows.h>
#include <commctrl.h>                   /* TBM_* */

/* libc */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

/* resource */
#include "resource.h"

/* Index used to store dialog private pointer */
#ifndef USERCOOKIE
# define USERCOOKIE DWLP_USER
#endif
/* Index used to store dialog result */
#ifndef USERRESULT
# define USERRESULT DWLP_MSGRESULT
#endif
/* The function used for GetWindowLong() */
#ifndef GetWindowLongF
# define GetWindowLongF(H,I) GetWindowLongPtrA(H,I)
#endif
/* The function used for SetWindowLong() */
#ifndef SetWindowLongF
# define SetWindowLongF(H,I,V) SetWindowLongPtrA(H,I,(LONG_PTR)(V))
#endif

#define GET_USERCOOKIE(H) ((dialog_t *)GetWindowLongF(H,USERCOOKIE))
#define SET_USERCOOKIE(H,V) SetWindowLongF(H,USERCOOKIE,V)
#define SET_RESULT(H,V) SetWindowLongF(H, USERRESULT,V)

/* typedef */
typedef struct keydef_s keydef_t;
typedef struct keyval_s keyval_t;
typedef struct dialog_s dialog_t;
typedef struct dlgdef_s dlgdef_t;

struct keydef_s {
  const char * key;                     /* dialog name */
  const int    idc;                     /* dialog resource id */
};

typedef sc68_dial_f dlgmsg_f;

struct dlgdef_s {
  const char * ident;                   /* identifier */
  int          idd;                     /* Windows int identifier */
  const char * locator;                 /* Windows str identifier */
  int        (*init)(dialog_t*);        /* init callback */
};

static const int magic = ('W'<<24)|('D'<<16)|('L'<<8)|'G';

struct dialog_s {
  int          magic;                 /* Magic number               */
  HINSTANCE    hinst;                 /* DLL instance (resource)    */
  HWND         hparent;               /* Parent window              */
  HWND         hdlg;                  /* My window                  */
  int          modal;                 /* created modal or modless   */
  int          retval;                /* dialog return code         */
  int          iid;                   /* dialog integer identifier  */
  const char * ident;                 /* dialog string identifier   */
  const char * locator;               /* dialog resource name.      */
  dlgmsg_f     cntl;                  /* dialog user function       */
  void       * cookie;                /* user provided private data */
};


/* Forward declarations. */

static int  init_config(dialog_t *);
static int  init_fileinfo(dialog_t *);
static int  init_trackselect(dialog_t *);
static void kill_dialog(dialog_t *);
static dialog_t * new_dialog(void *, dlgmsg_f);
static inline int isdial(dialog_t * dial) {
  return dial && dial->magic == magic;
}

static int dial_cntl(dialog_t * dial, const char * key,
                     int fct, sc68_dialval_t * v)
{
  int res;
  assert(isdial(dial));
  assert(key);
  assert(v);
  res = dial->cntl(dial->cookie, key, fct, v);
  assert (res >= -1 && res <= 1);
  return res;
}

static int dial_call(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_CALL, v);
}

static int dial_enum(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_ENUM, v);
}

static int dial_geti(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_GETI, v);
}

static int dial_seti(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_SETI, v);
}

static int dial_gets(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_GETS, v);
}

#if 0
static int dial_sets(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_SETS, v);
}
#endif

static int dial_mini(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_MIN, v);
}

static int dial_maxi(dialog_t * dial, const char * key, sc68_dialval_t * v)
{
  return dial_cntl(dial, key, SC68_DIAL_MAX, v);
}

#ifndef IDD_CFG
# define IDD_CFG IDC_STATIC
#endif

#ifndef IDD_INF
# define IDD_INF IDC_STATIC
#endif

#ifndef IDD_SEL
# define IDD_SEL IDC_STATIC
#endif

static dlgdef_t dlgdef[3] = {
  { "config",      IDD_CFG, "sc68-conf", init_config      },
  { "fileinfo",    IDD_INF, "sc68-finf", init_fileinfo    },
  { "trackselect", IDD_SEL, "sc68-tsel", init_trackselect },
};

enum {
  IID_CONFIG = 0, IID_FILEINFO, IID_TRACKSELECT
};

static intptr_t CALLBACK DialogProc(HWND,UINT,WPARAM,LPARAM);

static struct remember_s {
  unsigned int track : 8;
  unsigned int asid  : 2;
  unsigned int set   : 1;
} sel_remember;

/* debug function */
#if defined(DEBUG) || defined(_DEBUG)
#define DBG(F,...) dbg(F, ## __VA_ARGS__)
#define ERR(N,F,...) dbg_error(N,F, ## __VA_ARGS__)

static
void dbg(const char * fmt, ...)
{
  static const char pref[] = "winui  : ";
  char s[256];
  const DWORD max = sizeof(s)-1;
  DWORD i;
  va_list list;
  HANDLE hdl;
  va_start(list,fmt);
  strcpy(s,pref);
  i = sizeof(pref)-1;
  i += vsnprintf(s+i, max-i, fmt, list);
  if (i < max && s[i-1] != '\n')
    s[i++] = '\n';
  if (i > max) i = max;
  s[i] = 0;
  if (hdl = GetStdHandle(STD_ERROR_HANDLE),
      (hdl != NULL && hdl != INVALID_HANDLE_VALUE)) {
    WriteFile(hdl,s,i,&i,NULL);
    FlushFileBuffers(hdl);
  }
  else
    OutputDebugStringA(s);
  va_end(list);
}

static
void dbg_error(int err, const char * fmt, ...)
{
  char str[512];
  int max = sizeof(str), i;

  if (fmt) {
    va_list list;
    va_start(list,fmt);
    i = vsnprintf(str,max,fmt,list);
    va_end(list);
  }
  if (i < max)
    i += snprintf(str+i,max-i," -- ");
  if (i < max) {
    int n =
      FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM, 0, err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        str+i, max-i, 0);
    if (n <= 0)
      n = snprintf(str+i,max-i,"#%d",err);
    while (--n > 0 && isspace((int)str[i+n]))
      str[i+n] = 0;
    i += n;
  }
  str[max-1] = 0;
  dbg("ERROR: %s\n",str);
}

#else
# define DBG(F,...) for(;0;)
# define ERR(N,F,...) for(;0;)
#endif

#if 0
static int SetUnsigned(HWND hdlg, int id, unsigned int new_val)
{
  return -!SetDlgItemInt(hdlg, id, new_val, FALSE);
}

static int GetUnsigned(HWND hdlg, int id, unsigned int * ptr_val)
{
  BOOL ok;
  *ptr_val = GetDlgItemInt(hdlg, id, &ok, FALSE);
  return -!ok;
}
#endif

static int SetInt(HWND hdlg, int id, int new_val)
{
  return -!SetDlgItemInt(hdlg, id, new_val, TRUE);
}

static int GetInt(HWND hdlg, int id, int * ptr_val)
{
  BOOL ok;
  *ptr_val = GetDlgItemInt(hdlg, id, &ok, TRUE);
  return -!ok;
}

static int SetText(HWND hdlg, int id, const char * new_text)
{
  return -!SetDlgItemText(hdlg, id, new_text);

}

#if 0
static int GetText(HWND hdlg, int id, char * str, int len)
{
  return -!GetDlgItemText(hdlg, id, str, len);
}
#endif

static int SetFormat(HWND hdlg, int id, const char * fmt, ...)
{
  char buffer[256];
  va_list list;

  va_start(list,fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, list);
  va_end(list);
  buffer[sizeof(buffer)-1] = 0;
  return SetText(hdlg, id, buffer);
}

static void SetEnable(HWND hdlg, int id, int enable)
{
  EnableWindow(GetDlgItem(hdlg,id),!!enable);
}

static int IsEnabled(HWND hdlg, int id)
{
  return !!IsWindowEnabled(GetDlgItem(hdlg,id));
}

static int GetCheck(HWND hdlg, int id)
{
  switch (SendDlgItemMessage(hdlg, id, BM_GETCHECK, 0, 0)) {
  case BST_CHECKED:   return 1;
  case BST_UNCHECKED: return 0;
  }
  return -1;
}

static void SetCheck(HWND hdlg, int id, int check)
{
  SendDlgItemMessage(hdlg, id, BM_SETCHECK,
                     check ? BST_CHECKED : BST_UNCHECKED, 0);
}

#if 0
static void SetVisible(HWND hdlg, int id, int visible)
{
  ShowWindow(GetDlgItem(hdlg, id), visible ? SW_SHOW : SW_HIDE);
}
#endif

static inline int SetComboPos(HWND hdlg, int idc, int pos)
{
  pos = SendDlgItemMessageA(hdlg, idc, CB_SETCURSEL, pos, 0);
  return pos == CB_ERR ? -1 : pos;
}

static inline int GetComboPos(HWND hdlg, int idc)
{
  int pos = SendDlgItemMessageA(hdlg, idc, CB_GETCURSEL, 0, 0);
  return pos == CB_ERR ? -1 : pos;
}

static inline int GetSlidePos(HWND hdlg, int idc)
{
  return SendDlgItemMessageA(hdlg, idc, TBM_GETPOS, 0, 0);
}

#if 0
static int init_int(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  if (dial_geti(dial, key, &v)) {
    DBG("%s(%d,%s) <- %d\n", __FUNCTION__, idc, key, v.i);
    return SetInt(dial->hdlg, idc, v.i);
  }
  return -1;
}
#endif

static int return_int(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  if (!GetInt(dial->hdlg, idc, &v.i)) {
    DBG("%s(%d,%s) -> %d\n", __FUNCTION__, idc, key, v.i);
    return dial_seti(dial, key, &v);
  }
  return -1;
}

static int init_text(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  if (!dial_gets(dial, key, &v)) {
    DBG("%s(%d,%s) <- \"%s\"\n", __FUNCTION__, idc, key, v.s);
    return SetText(dial->hdlg, idc, v.s);
  }
  return -1;
}

#if 0
static int return_text(dialog_t * dial, int idc, const char * key)
{
  char s[128];
  if (!GetText(dial->hdlg, idc, s, sizeof(s))) {
    sc68_dialval_t v;
    v.s = s;
    DBG("%s(%d,%s) -> \"%s\"\n", __FUNCTION__, idc, key, s);
    return dial_sets(dial, key, &v);
  }
  return -1;
}
#endif

static int init_time(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  if (!dial_geti(dial, key, &v))
    return SetFormat(dial->hdlg, idc, "%02u:%02u",
                     (unsigned)v.i/60u,  (unsigned)v.i%60u);
  return -1;
}

static int init_check(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  int res = dial_geti(dial, key, &v);
  if (!res) {
    DBG("%s(%d,%s) <- %s\n", __FUNCTION__, idc, key, v.i?"On":"Off");
    SetCheck(dial->hdlg, idc, v.i);
  }
  return res;
}

static int return_check(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  v.i = GetCheck(dial->hdlg, idc);
  if (v.i >= 0) {
    DBG("%s(%d,%s) -> %d\n", __FUNCTION__, idc, key, v.i);
    return dial_seti(dial, key, &v);
  }
  return -1;
}

static int init_combo(dialog_t * dial, int idc, const char * key)
{
  int i;
  sc68_dialval_t v;
  HWND hcb = GetDlgItem(dial->hdlg, idc);
  if (!hcb) return -1;

  DBG("%s(%d,%s)\n", __FUNCTION__, idc, key);
  SendMessageA(hcb, CB_RESETCONTENT,  0, 0);
  i = 0;
  while (i >= 0) {
    v.i = i++;
    switch (dial_enum(dial, key, &v)) {
    case 0:
      if (!v.s) v.s = "<undef>";
      SendMessageA(hcb, CB_ADDSTRING, 0, (LPARAM)v.s);
      break;

    default:
      assert(!"unexepected return");
    case 1:
    case -1:
      i = -1;
      break;
    }
  }

  for (i=0;
       (v.i = i), !dial_enum(dial, key, &v);
       ++i) {
    assert(v.s && v.i != i);
    DBG("%s(%d) %s[%d] = \"%s\"\n", __FUNCTION__,
        idc, key, i, v.s?v.s:"(null)");
  }
  if (!dial_geti(dial, key, &v)) {
    DBG("%s(%d) %s[@] = %d\n", __FUNCTION__, idc, key, v.i);
    SendMessageA(hcb, CB_SETCURSEL, v.i, 0);
  }
  return 0;
}

static int return_combo(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  v.i = SendDlgItemMessageA(dial->hdlg, idc, CB_GETCURSEL,0,0);
  if (v.i >= 0) {
    DBG("%s(%d,%s) -> %d\n", __FUNCTION__, idc, key, v.i);
    return dial_seti(dial, key, &v);
  }
  return -1;
}

static int init_slide(dialog_t * dial, int idc, const char * key,
                      int bud, const int * tic, int ntics)
{
  HWND hcb = GetDlgItem(dial->hdlg, idc);
  sc68_dialval_t min , max, num;

  DBG("%s(%d,%s,%d,%d)\n", __FUNCTION__, idc, key, bud, ntics);

  if (!dial_mini(dial, key, &min) &&
      !dial_maxi(dial, key, &max) &&
      !dial_geti(dial, key, &num )) {

    DBG("slide \"%s\" %d<%d<%d\n", key, min.i, num.i, max.i);

    SendMessageA(hcb, (UINT)TBM_SETRANGE,
                 (WPARAM)FALSE /*redraw*/,
                 MAKELPARAM(min.i,max.i));
    SendMessageA(hcb, (UINT)TBM_CLEARTICS,
                 (WPARAM)FALSE /*redraw*/, 0);
    if (ntics) {
      int i;
      if (tic) {
        for (i = 0; i < ntics; ++i)
          SendMessageA(hcb, TBM_SETTIC, TRUE, tic[i]);
      } else {
        for (i = 0; i < ntics; ++i) {
          const int v = min.i + (i+1)*(max.i-min.i)/(ntics+1);
          SendMessageA(hcb, TBM_SETTIC, TRUE, v);
        }
      }
    }
    if (bud != IDC_STATIC) {
      SetInt(dial->hdlg, bud, num.i);
      SendMessageA(hcb, (UINT)TBM_SETBUDDY,
                   (WPARAM)FALSE /*position*/,
                   (LPARAM)GetDlgItem(dial->hdlg, bud));
    }

    SendMessageA(hcb, (UINT)TBM_SETPOS,
                 (WPARAM)TRUE /*redraw*/,
                 (LPARAM)num.i /* (opt->min+opt->max)>>1 */);

    return 0;
  }
  return -1;
}

static int return_slide(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  v.i = GetSlidePos(dial->hdlg,idc);
  if (v.i >= 0) {
    DBG("%s(%d,%s) -> %d\n", __FUNCTION__, idc, key, v.i);
    return dial_seti(dial, key, &v);
  }
  return -1;
}

static int init_spin(dialog_t * dial, int idc, const char * key, int bud)
{
  sc68_dialval_t min , max, num;

  DBG("%s(%d,%s,%d)\n", __FUNCTION__, idc, key, bud);
  if (!dial_mini(dial, key, &min) &&
      !dial_maxi(dial, key, &max) &&
      !dial_geti(dial, key, &num)) {
    DBG("spin \"%s\" %d<%d<%d\n", key,min.i,num.i,max.i);

    SendDlgItemMessage(dial->hdlg, idc, UDM_SETRANGE32, min.i, max.i);
    SendDlgItemMessage(dial->hdlg, idc, UDM_SETPOS32, 0, num.i);
    SetInt(dial->hdlg, bud, num.i);
    SendDlgItemMessage(dial->hdlg, idc, UDM_SETBUDDY,
                       (WPARAM)GetDlgItem(dial->hdlg, bud), 0);
    return 0;
  }
  return -1;
}

static int return_spin(dialog_t * dial, int idc, const char * key)
{
  sc68_dialval_t v;
  BOOL failed = FALSE;
  v.i = SendDlgItemMessageA(dial->hdlg,idc,UDM_GETPOS32,0,(LPARAM)&failed);
  if (!failed) {
    DBG("%s(%d,%s) -> %d\n", __FUNCTION__, idc, key, v.i);
    return dial_seti(dial, key, &v);
  }
  return -1;
}

static int update_spr(dialog_t * dial)
{
  int v = GetComboPos(dial->hdlg, IDC_CFG_SPR);
  SetEnable(dial->hdlg, IDC_CFG_USRSPR,  !v);
  SetEnable(dial->hdlg, IDC_CFG_SPRSPIN, !v);
  return 0;
}

static int init_config(dialog_t * dial)
{
  int res = 0;

  assert(dial);
  assert(dial->iid == IID_CONFIG);

  res |= init_combo(dial,IDC_CFG_YMENGINE,  "ym-engine");
  res |= init_combo(dial,IDC_CFG_YMFILTER,  "ym-filter");
  res |= init_combo(dial,IDC_CFG_YMVOLUME,  "ym-volmodel");
  res |= init_combo(dial,IDC_CFG_ASID,      "asid");
  res |= init_combo(dial,IDC_CFG_SPR,       "sampling");
  res |= init_spin(dial,IDC_CFG_SPRSPIN,    "sampling-rate", IDC_CFG_USRSPR);
  res |= update_spr(dial);
  res |= init_check(dial,IDC_CFG_AGAFILTER, "amiga-filter");
  res |= init_slide(dial,IDC_CFG_AGABLEND,  "amiga-blend", IDC_STATIC, 0, 3);
  res |= init_spin(dial, IDC_CFG_TIMESPIN,  "default-time", IDC_CFG_DEFTIME);
  res |= init_check(dial,IDC_CFG_UFI,       "ufi");
  res |= init_check(dial,IDC_CFG_HOOK,      "hook");
  return res;
}

static int return_config(dialog_t * dial)
{
  int res = 0;

  assert(dial);
  assert(dial->iid == IID_CONFIG);

  res |= return_combo(dial,IDC_CFG_YMENGINE,  "ym-engine");
  res |= return_combo(dial,IDC_CFG_YMFILTER,  "ym-filter");
  res |= return_combo(dial,IDC_CFG_YMVOLUME,  "ym-volmodel");
  res |= return_combo(dial,IDC_CFG_ASID,      "asid");
  res |= IsEnabled(dial->hdlg,IDC_CFG_USRSPR)
    ? return_int(dial,IDC_CFG_USRSPR, "sampling-rate")
    : return_combo(dial,IDC_CFG_SPR,  "sampling-rate")
    ;
  res |= return_check(dial,IDC_CFG_AGAFILTER, "amiga-filter");
  res |= return_slide(dial,IDC_CFG_AGABLEND,  "amiga-blend");
  res |= return_spin(dial, IDC_CFG_TIMESPIN,  "default-time");
  res |= return_check(dial,IDC_CFG_UFI,       "ufi");
  res |= return_check(dial,IDC_CFG_HOOK,      "hook");

  if (res >= 0) {
    sc68_dialval_t v;
    res = dial_call(dial,"save",&v);
    if (!res)
      res = -!!v.i;
    else if (res > 0)
      res = 0;
  }
  return dial->retval = res<0 ? -3 : 0;
}

static int update_fileinfo(dialog_t * dial, int what)
{
  int res = 0;

  assert(dial);
  assert(dial->iid == IID_FILEINFO);

  if (what & 1) {
    init_time(dial, IDC_INF_TIME,      "time");
    init_text(dial, IDC_INF_FORMAT,    "format");
    init_text(dial, IDC_INF_GENRE,     "genre");
    init_check(dial, IDC_INF_YM,       "hw_ym");
    init_check(dial, IDC_INF_STE,      "hw_ste");
    init_check(dial, IDC_INF_ASID,     "hw_asid");
    init_text(dial, IDC_INF_TITLE,     "title");
    init_text(dial, IDC_INF_ARTIST,    "artist");
    init_text(dial, IDC_INF_ALBUM,     "album");
    init_text(dial, IDC_INF_RIPPER,    "ripper");
    init_text(dial, IDC_INF_CONVERTER, "converter");
    init_text(dial, IDC_INF_YEAR,      "year");
  }
  if (what & 2) {
    sc68_dialval_t v;
    v.i = GetComboPos(dial->hdlg, IDC_INF_TAGNAME);
    if (v.i >= 0 &&
        !dial_enum(dial,"tag-val",&v)
      ) {
      SetText(dial->hdlg, IDC_INF_TAGVALUE, v.s);
    }
  }
  InvalidateRect(dial->hdlg,0,TRUE); /* or no erase ... Wiiindooows */
  return res;
}

static int init_fileinfo(dialog_t * dial)
{
  int res = 0;

  assert(dial);
  assert(dial->iid == IID_FILEINFO);

  init_text(dial, IDC_INF_URI,     "uri");
  init_combo(dial, IDC_INF_TRACK,  "track");
  init_combo(dial,IDC_INF_TAGNAME, "tag-key");
  res = update_fileinfo(dial,3);
  SetEnable(dial->hdlg, IDC_INF_YM, 0);
  SetEnable(dial->hdlg, IDC_INF_STE, 0);
  SetEnable(dial->hdlg, IDC_INF_ASID, 0);
  return res;
}

static int init_trackselect(dialog_t * dial)
{
  int res = 0;
  HWND hdlg = dial->hdlg;
  void * user = dial->cookie;
  sc68_dialval_t v;

  assert(dial);
  assert(dial->iid == IID_TRACKSELECT);

  DBG("remember:%s track:%d asid:%d\n",
      sel_remember.set?"On":"Off", sel_remember.track, sel_remember.asid);

  res |= init_text(dial, IDC_SEL_ALBUM, "album");
  res |= init_combo(dial,IDC_SEL_TRACK, "track");
  res |= init_combo(dial,IDC_SEL_ASID,  "asid");

  if (sel_remember.set) {
    v.i = sel_remember.track;
    dial_seti(user, "track", &v);
    SetComboPos(dial->hdlg, IDC_SEL_TRACK, v.i);
    v.i = sel_remember.asid;
    dial_seti(user, "asid", &v);
    SetComboPos(dial->hdlg, IDC_SEL_ASID, v.i);
  }
  SetCheck(hdlg, IDC_SEL_REMEMBER, sel_remember.set);
  if (!dial_geti(dial,"hw_asid", &v))
    SetEnable(hdlg,IDC_SEL_ASID,!!v.i);
  DBG("%s() -> %d\n", __FUNCTION__, res);

  return res;
}

static int return_tracksel(dialog_t * dial)
{
  int track, asid;

  assert(dial);
  assert(dial->iid == IID_TRACKSELECT);

  track = GetComboPos(dial->hdlg, IDC_SEL_TRACK);
  if (track < 0) track = 0;
  asid = GetComboPos(dial->hdlg, IDC_SEL_ASID);
  if (asid < 0) asid = 0;
  dial->retval = track & 255;
  if (IsEnabled(dial->hdlg,IDC_SEL_ASID))
    dial->retval |= asid << 8;
  sel_remember.set = GetCheck(dial->hdlg, IDC_SEL_REMEMBER) == 1;
  sel_remember.track = track;
  sel_remember.asid  = asid;
  DBG("remember:%s track:%d asid:%d\n",
      sel_remember.set?"On":"Off", sel_remember.track, sel_remember.asid);
  return dial->retval;
}

static intptr_t CALLBACK DialogProc(
  HWND hdlg,      // handle to dialog box
  UINT umsg,      // message
  WPARAM wparam,  // first message parameter
  LPARAM lparam)  // second message parameter
{
  dialog_t * dial = GET_USERCOOKIE(hdlg);
  sc68_dialval_t v;

  /* To set a result : */
  /* SetWindowLong(hdlg, DWL_MSGRESULT, lResult); return TRUE; */

  /* DBG("DialogProc: 0x%03x %p\n", umsg, (void *) dial); */

  switch (umsg) {

  case WM_INITDIALOG:
    DBG("%s() -- WM_INITDIALOG:\n", __FUNCTION__);
    dial = (dialog_t *) lparam;
    if (!isdial(dial))  {
      DBG("%s() WARNING user cookie corrupted\n", __FUNCTION__);
      dial = 0;
    } else {
      const int max_iid = sizeof(dlgdef)/sizeof(*dlgdef);
      dial->hdlg = hdlg;
      SET_USERCOOKIE(hdlg, dial);
      if (dial->iid >= 0 && dial->iid < max_iid)
        dlgdef[dial->iid].init(dial);
      else
        DBG("%s() -- \"%s\" iid out of range -- %d\n",
            dial->ident, dial->iid);
    }
    return TRUE; /* set the focus to the control specified by wParam */

  case WM_NCDESTROY:
    DBG("WM_NCDESTROY %p\n", (void *) dial);
    if (isdial(dial)) {
      if (!dial->modal) {
        DBG("WM_NCDESTROY %p -- Kill modless dialog\n", (void *) dial);
        dial->hdlg = 0;
        kill_dialog(dial);
      }
      SET_USERCOOKIE(hdlg, 0);
    }
    return FALSE;
  }

  if (!isdial(dial))
    return FALSE;

  switch(umsg)
  {
  case WM_COMMAND: {
    /* int wNotifyCode = HIWORD(wparam); // notification code */
    int wID = LOWORD(wparam); // item, control, or accelerator identifier
    HWND hwndCtl = (HWND) lparam;       // handle of control

    switch(wID) {

      /* ----------------------------------------------------------------
       * Generic
       * ---------------------------------------------------------------- */

    case IDOK:
      switch (dial->iid) {

        /* OK on trackselect */
      case IID_TRACKSELECT:
        return_tracksel(dial); break;

        /* OK on config */
      case IID_CONFIG:
        return_config(dial); break;

        /* OK on others */
      default:
        dial->retval = 0; break;
      }
      PostMessage(hdlg, WM_CLOSE, 0,0);
      return TRUE;

    case IDCANCEL:
      PostMessage(hdlg, WM_CLOSE, 0,0);
      return TRUE;

    case IDC_SEL_ASID:
    case IDC_CFG_ASID:
      v.i = SendMessageA(hwndCtl, CB_GETCURSEL, 0, 0);
      if (v.i >= 0 && v.i < 3) {
        dial_seti(dial, "asid", &v);
        if (wID == IDC_CFG_ASID)
          dial_call(dial, "asid", &v);
      }
      return TRUE;

    case IDC_SEL_REMEMBER:
      return TRUE;

    case IDC_SEL_TRACK:
    case IDC_INF_TRACK:
      if (HIWORD(wparam) == CBN_SELCHANGE) {
        v.i = SendMessageA(hwndCtl,CB_GETCURSEL,0,0);
        if (v.i != CB_ERR) {
          dial_seti(dial, "track", &v);
          if (wID == IDC_INF_TRACK) {
            update_fileinfo(dial,1);
          } else /* if (wID == IDC_SEL_TRACK) */ {
            if (!dial_geti(dial,"hw_asid", &v))
              SetEnable(hdlg,IDC_SEL_ASID,!!v.i);
          }
        }
      }
      return TRUE;

    case IDC_INF_TAGNAME:
      if (HIWORD(wparam) == CBN_SELCHANGE)
        update_fileinfo(dial,2);
      return TRUE;

    case IDC_CFG_SPR:
      update_spr(dial);
      return TRUE;

    case IDC_CFG_AGAFILTER:
      /* Real Time apply */
      v.i = GetCheck(hdlg, IDC_CFG_AGAFILTER) == 1;
      if (!dial_call(dial, "amiga-filter", &v))
        DBG("Real-Time amiga-filter => %d\n", v.i);
      return TRUE;

    case IDC_CFG_AGABLEND:
      /* Real Time apply */
      v.i = GetSlidePos(hdlg, IDC_CFG_AGABLEND);
      if (v.i >= 0 && !dial_call(dial,"amiga-blend", &v))
        DBG("Real-Time amiga-blend => %d\n", v.i);
      return TRUE;

    default:
      DBG("#%05d W=%04x/%04x L=%08x\n",
          wID, HIWORD(wparam), LOWORD(wparam), (unsigned)lparam);
      break;
    }
  } return TRUE;

  case WM_CLOSE:
    DBG("%s(%08x) CLOSE(%s)\n", __FUNCTION__,
        (unsigned)hdlg, dial->modal?"modal":"modless");
    if (dial->modal) {
      EndDialog(hdlg,1);
    } else {
      SET_RESULT(hdlg,1);
      DestroyWindow(hdlg);
    }
    return TRUE;
  }

  /* DBG("%s(dlg:x%x,msg:x%x,wpar:x%x,lpar:x%x) *unhandled message*\n", */
  /*     __FUNCTION__, */
  /*     (unsigned) hdlg, */
  /*     (unsigned) umsg, */
  /*     (unsigned) wparam, */
  /*     (unsigned) lparam); */
  return FALSE;
}

/* ----------------------------------------------------------------------
   Export
   ---------------------------------------------------------------------- */

static void kill_dialog(dialog_t * dial)
{
  assert(dial);
  assert(dial->cntl);
  assert(dial->cookie);

  if (dial) {
    DBG("kill dialog \"%s\"\n",dial->ident?dial->ident:"(nil)");
    if (dial->cntl) {
      sc68_dialval_t v;
      v.i = dial->retval;
      dial_call(dial, SC68_DIAL_KILL, &v);
    }
    free(dial);
  }
}

static HMODULE get_hmodule(void)
{
#if 1
  return GetModuleHandleA(0);
#else
  HMODULE hModule = NULL;
  static int avar;
  GetModuleHandleExA(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
    (LPCTSTR) &avar,
    &hModule);
  return hModule;
#endif
}

static dialog_t * new_dialog(void * cookie, dlgmsg_f cntl)
{
  sc68_dialval_t v;
  dialog_t tmp, *dial = 0;
  int i;
  const int maxdef = sizeof(dlgdef)/sizeof(*dlgdef);

  if (!cntl)
    return 0;

  dial68_rc_force = ~dial68_rc_force;

  ZeroMemory(&tmp,sizeof(tmp));
  tmp.magic   = magic;
  tmp.iid     = -1;
  tmp.cookie  = cookie;
  tmp.cntl    = cntl;
  tmp.retval  = -2;

  /* Introducing myself  */
  v.s =  "win32/native";
  if (cntl(cookie,SC68_DIAL_HELLO,SC68_DIAL_CALL,&v))
    goto failed;


  DBG("connected to \"%s\"\n", v.s);

  /* Do I know you ? */
  for (i=0; i<maxdef; ++i) {
    if (!strcmp(dlgdef[i].ident,v.s)) {
      tmp.iid   = i;
      tmp.ident = v.s;
      tmp.locator = dlgdef[i].idd == IDC_STATIC
        ? dlgdef[i].locator
        : MAKEINTRESOURCE(dlgdef[i].idd);
      break;
    }
  }
  if (i == maxdef) {
    DBG("don't know a dialog named \"%s\"\n", v.s);
    goto failed;
  }

  /* Instance / Hmodule */
  if (cntl(cookie, "instance", SC68_DIAL_CALL, &v))
    v.s = 0;
  if (!v.s) {
    v.s = (void*) get_hmodule();
    DBG("User did not provide a instance handle -> default is %p\n",v.s);
    if (!v.s)
      goto failed;
  }
  tmp.hinst = (HINSTANCE) v.s;
  DBG("win32 instance -> %p\n", (void*) tmp.hinst);

  if (cntl(cookie, "parent", SC68_DIAL_CALL, &v))
    v.s = 0;
  if (!v.s) {
    /* User did not provide a parent window; let's try some
     * reasonnable guess. */
    v.s = (void*) GetActiveWindow();     /* thread msg Q window */
    if (!v.s)
      v.s = (void*) GetDesktopWindow(); /* desktop windows */
    DBG("User did not provide a parent window -> default is %p\n", v.s);
  }
  tmp.hparent = (HWND) v.s;
  DBG("win32 parent window -> %p\n", (void*) tmp.hparent);

  dial = malloc(sizeof(dialog_t));
  if (dial) {
    *dial = tmp;
    return dial;
  }


failed:
  DBG("%s -- failed to create dialog\n", __FUNCTION__);
  if (cntl) {
    v.i = -1;
    cntl(cookie,SC68_DIAL_KILL, SC68_DIAL_CALL, &v);
  }
  return 0;
}

#ifdef DEBUG
#define DOENUM 1

static
BOOL CALLBACK myenum(
  HMODULE  hModule,
  LPCTSTR  lpszType,
  LPTSTR   lpszName,
  LONG_PTR lParam
  ) {
  dialog_t * dial = (dialog_t *) lParam;
  if (IS_INTRESOURCE(lpszName))
    DBG("%s: int %d\n", dial->ident, (int)lpszName);
  else
    DBG("%s: str \"%s\"\n", dial->ident, lpszName);
  return TRUE;                          /* continue */
}

static void dialog_enum(dialog_t * dial)
{
  BOOL res = EnumResourceNamesA(
    dial->hinst,
    RT_DIALOG,
    myenum,
    (LONG_PTR)dial);
  if (!res) {
    int err = GetLastError();
    ERR(err, "enum/%s (failed)", dial->ident);
  }
}

#endif

static intptr_t dialog_modal(void * data, dlgmsg_f cntl)
{
  intptr_t ret;
  dialog_t * dial = new_dialog(data, cntl);

  if (!dial)
    ret = -1;
  else {
    DWORD err;
    const char * tpl = dial->locator;

#ifdef DOENUM
    dialog_enum(dial);
#endif

    dial->modal = 1;
    SetLastError(ERROR_SUCCESS);
    /* returns the result of EndDialog() */
    ret = DialogBoxParamA(dial->hinst, tpl, dial->hparent,
                          (DLGPROC) DialogProc, (LPARAM)dial);
    switch (ret) {
    case 0: /* For Windows compatibily reason */
      err = GetLastError();
      ERR(err, "%s/%s (Invalid parent window)", "modal", dial->ident);
      ret = -1;
      break;
    case -1:
      err = GetLastError();
      ERR(err, "%s/%s", "modal", dial->ident);
      break;
    default:
      ret = dial->retval;
    }
    kill_dialog(dial);
  }
  DBG("%s -> %d\n", __FUNCTION__, (int) ret);
  return ret;
}

static intptr_t dialog_modless(void * data, dlgmsg_f cntl)
{
  intptr_t ret;
  dialog_t * dial = new_dialog(data, cntl);

  if (!dial)
    ret = -1;
  else {
    DWORD err;
    const char * tpl = dial->locator;

#ifdef DOENUM
    // $$$ TEMP
    dialog_enum(dial);
#endif

    dial->modal = 0;
    SetLastError(ERROR_SUCCESS);
    dial->hdlg =
      CreateDialogParamA(dial->hinst, tpl, dial->hparent,
                         (DLGPROC) DialogProc, (LPARAM)dial);
    err = GetLastError();
    if (!dial->hdlg) {
      ERR(err, "%s/%s", "modless", dial->ident);
      kill_dialog(dial);
      ret = -1;
    } else {
      if (err != ERROR_SUCCESS)
        ERR(err,"%s/%s (non fatal)", "modless", dial->ident);
      ret = 0;
    }
  }
  DBG("%s -> %d\n", __FUNCTION__, (int) ret);
  return ret;
}

int dial68_frontend(void * data, sc68_dial_f cntl)
{
  int ret;
  sc68_dialval_t v;
  assert(cntl && data);
  if (!data || !cntl)
    ret = -1;
  else if (!cntl(data,SC68_DIAL_WAIT,SC68_DIAL_CALL,&v) && v.i)
    ret = dialog_modal(data,cntl);
  else
    ret = dialog_modless(data,cntl);
  return ret;
}
