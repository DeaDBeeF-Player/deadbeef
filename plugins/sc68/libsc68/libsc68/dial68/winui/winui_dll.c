/*
 * @file    winui_dll.c
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
#include <stdint.h>

/* Needed for UDM_32 defines */
#ifndef WINVER
# define WINVER 0x0502
#endif

#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0400
#endif

#ifndef _WIN32_IE
# define _WIN32_IE 0x0500
#endif

/* windows */
#include <windows.h>

HMODULE winui_hmod = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  BOOL ret = TRUE;

  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    if (!winui_hmod)
      winui_hmod = hinstDLL;
    else if (winui_hmod != hinstDLL)
      ret = FALSE;
    break;
  case DLL_PROCESS_DETACH:
    if (winui_hmod == hinstDLL)
      winui_hmod = 0;
    break;
  }

  return ret;
}

intptr_t dialog_modless(void * data, sc68_dial_f cntl);
intptr_t dialog_modal(void * data, sc68_dial_f cntl);

int  __declspec(dllexport) frontend(void * data, sc68_dial_f cntl)
{
  sc68_dialval_t v;
  return !cntl(data,SC68_DIAL_WAIT,SC68_DIAL_CALL,&v) && v.i
    ? dialog_modal(data,cntl)
    : dialog_modless(data,cntl)
    ;
}
