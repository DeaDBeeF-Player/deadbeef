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
#ifndef GLADE_KEYS_H
#define GLADE_KEYS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _GbKey  GbKey;
struct _GbKey
{
  guint		 key;
  gchar		*name;
};

/* Array of GDK key values & symbols which can be used as GTK accelerator keys.
   These are used for the GladeKeysDialog.
   Note that in GTK 1.0 accelerator keys are a guchar which means that only
   keys with values 0-255 can be used. In GTK 1.1 a guint is used. */
extern const GbKey GbKeys[];


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_KEYS_H */
