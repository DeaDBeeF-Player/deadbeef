
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

#ifndef GLADE_MENU_EDITOR_H
#define GLADE_MENU_EDITOR_H


#include <gdk/gdk.h>
#include <gtk/gtkmenushell.h>
#include <gtk/gtkwindow.h>

#include "glade_project.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GLADE_MENU_EDITOR(obj)          GTK_CHECK_CAST (obj, glade_menu_editor_get_type (), GladeMenuEditor)
#define GLADE_MENU_EDITOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, glade_menu_editor_get_type (), GladeMenuEditorClass)
#define GLADE_IS_MENU_EDITOR(obj)       GTK_CHECK_TYPE (obj, glade_menu_editor_get_type ())


typedef struct _GladeMenuEditor       GladeMenuEditor;
typedef struct _GladeMenuEditorClass  GladeMenuEditorClass;

struct _GladeMenuEditor
{
  GtkWindow window;

  GtkWidget *clist;

  GtkWidget *stock_label;
  GtkWidget *stock_combo;
  GtkWidget *icon_label;
  GtkWidget *icon_widget;
  GtkWidget *icon_button;

  GtkWidget *label_label;
  GtkWidget *label_entry;
  GtkWidget *name_label;
  GtkWidget *name_entry;
  GtkWidget *handler_label;
  GtkWidget *handler_entry;
  GtkWidget *tooltip_label;
  GtkWidget *tooltip_entry;
  GtkWidget *type_frame;
  GtkWidget *normal_radiobutton;
  GtkWidget *check_radiobutton;
  GtkWidget *radio_radiobutton;
  GtkWidget *right_justify_label;
  GtkWidget *right_justify_togglebutton;
  GtkWidget *state_label;
  GtkWidget *state_togglebutton;
  GtkWidget *group_label;
  GtkWidget *group_combo;
  GtkWidget *accel_frame;
  GtkWidget *accel_key_entry;
  GtkWidget *accel_ctrl_checkbutton;
  GtkWidget *accel_shift_checkbutton;
  GtkWidget *accel_alt_checkbutton;
  GtkWidget *up_button;
  GtkWidget *down_button;
  GtkWidget *left_button;
  GtkWidget *right_button;
  GtkWidget *add_button;
  GtkWidget *add_child_button;
  GtkWidget *add_separator_button;
  GtkWidget *delete_button;

  GtkWidget *ok_button;
  GtkWidget *apply_button;
  GtkWidget *cancel_button;

  /* This is our key selection dialog, used for selecting accelerator keys. */
  GtkWidget *keys_dialog;

  /* This is our file selection dialog, used for selecting icons. */
  GtkWidget *filesel;

  /* This is the menu widget we are editing and the project it is in. */
  GladeProject *project;
  GtkMenuShell *menu;

  /* This is set if Glade is compiled with GNOME support and the project has
     GNOME support turned on. We use the GnomeUIInfo stuff for menus in GNOME,
     rather than the GTK+ stock system. */
  gboolean gnome_support;

  /* This is the id of the handler connected to the menu widget's destroy
     signal, so we can destroy the menu editor if the menu is destroyed. */
  guint menu_destroy_handler_id;

  /* This is TRUE if we are updating the entry fields and so should ignore
     changed signals. */
  gboolean updating_widgets;

  /* The list of GTK+ stock items. */
  GSList *stock_items;
};

struct _GladeMenuEditorClass
{
  GtkWindowClass parent_class;
};


GType      glade_menu_editor_get_type    (void);

/* This creates a menu editor to edit the given menubar or popup menu in
   the given project. When the user selects the 'OK' or 'Apply' buttons,
   the menu widget will be updated. If the menu is destroyed, the menu editor
   is automatically destroyed as well. */
GtkWidget* glade_menu_editor_new         (GladeProject    *project,
					  GtkMenuShell    *menu);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* GLADE_MENU_EDITOR_H */
