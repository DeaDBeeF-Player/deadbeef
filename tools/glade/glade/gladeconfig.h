/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998  Damon Chaplin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* This header should be included by all Glade sources. */

#ifndef GLADE_CONFIG_H
#define GLADE_CONFIG_H

#include <config.h>

#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Internationalization. */
#ifdef USE_GNOME
#include <libgnome/gnome-i18n.h>
#else

#ifdef ENABLE_NLS
#include <libintl.h>
#undef _
#define _(String) dgettext (GETTEXT_PACKAGE, String)
#ifdef gettext_noop
#define N_(String) gettext_noop (String)
#else
#define N_(String) (String)
#endif /* gettext_noop */
#else
/* Stubs that do something close enough.  */
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,Message) (Message)
#define dcgettext(Domain,Message,Type) (Message)
#define bindtextdomain(Domain,Directory) (Domain)
#define _(String) (String)
#define N_(String) (String)
#endif /* ENABLE_NLS */

#endif /* USE_GNOME */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_CONFIG_H */
