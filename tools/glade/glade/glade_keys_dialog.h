
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

#ifndef GLADE_KEYS_DIALOG_H
#define GLADE_KEYS_DIALOG_H


#include <gdk/gdk.h>
#include <gtk/gtkdialog.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GLADE_KEYS_DIALOG(obj)          GTK_CHECK_CAST (obj, glade_keys_dialog_get_type (), GladeKeysDialog)
#define GLADE_KEYS_DIALOG_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, glade_keys_dialog_get_type (), GladeKeysDialogClass)
#define GLADE_IS_KEYS_DIALOG(obj)       GTK_CHECK_TYPE (obj, glade_keys_dialog_get_type ())


typedef struct _GladeKeysDialog       GladeKeysDialog;
typedef struct _GladeKeysDialogClass  GladeKeysDialogClass;

struct _GladeKeysDialog
{
  GtkDialog dialog;

  GtkWidget *clist;
};

struct _GladeKeysDialogClass
{
  GtkDialogClass parent_class;
};


GType      glade_keys_dialog_get_type      (void);
GtkWidget* glade_keys_dialog_new           (void);

/* This returns TRUE if a key is currently selected, and the given pointer
   is set to the key value. */
gboolean glade_keys_dialog_get_key	  (GladeKeysDialog	*dialog,
					   guint		*key);

/* This returns the name of the key currently selected, or NULL if none is
   selected. */
gchar* glade_keys_dialog_get_key_symbol	  (GladeKeysDialog	*dialog);

/* This sets the current key, using the given key value. */
void glade_keys_dialog_set_key		  (GladeKeysDialog	*dialog,
					   guint		 key);

/* This sets the current key, using the given key name. */
void glade_keys_dialog_set_key_symbol	  (GladeKeysDialog	*dialog,
					   const gchar		*key);


/* This finds the key value corresponding to the given string, which may be
   a symbol in gdkkeysyms.h, e.g. 'GDK_Tab', or a simple character, e.g. 'S'.
   Note that for GTK 1.0 this is a gchar, but for GTK 1.1 it is a guint. */
guint glade_keys_dialog_find_key	  (const gchar		*symbol);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* GLADE_KEYS_DIALOG_H */
