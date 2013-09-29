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

#include "gladeconfig.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <dirent.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef USE_GNOME
#include <gnome.h>
#include <bonobo.h>
#endif

#include "gb.h"
#include "gbwidget.h"
#include "glade_project_window.h"
#include "utils.h"


gchar *GladeDayNames[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
gchar *GladeMonthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

typedef void (*GladeDialogCallback) (GtkDialog *dialog, gpointer data);

#define	READ_BUFFER_SIZE	4096

static void on_entry_dialog_ok (GtkWidget * widget,
				gpointer data);
static void on_entry_dialog_destroy (GtkWidget * widget,
				     gpointer data);
static gboolean check_components_match (const gchar *base,
					const gchar *path,
					gint         root_pos,
					gint         len);

static GtkWidget* glade_util_find_table_accelerator_target (GtkWidget *parent,
							    GtkWidget *child);
static GtkWidget* glade_util_find_box_accelerator_target (GtkWidget *parent,
							  GtkWidget *child);
static GtkWidget* glade_util_find_fixed_accelerator_target (GtkWidget *parent,
							    GtkWidget *child);
static GtkWidget* glade_util_find_layout_accelerator_target (GtkWidget *parent,
							     GtkWidget *child);

typedef struct _GladeLayoutCallbackData GladeLayoutCallbackData;
struct _GladeLayoutCallbackData
{
  gint x, y, best_distance;
  GtkWidget *child, *best_target;
};

typedef struct _GladeFindWidgetData GladeFindWidgetData;
struct _GladeFindWidgetData
{
  gchar *name;
  GtkWidget *found_widget;
};

static void glade_util_find_layout_acclererator_target_cb (GtkWidget *widget,
							   GladeLayoutCallbackData *data);

static GtkWidget* glade_util_find_focus_child (GtkWidget *widget);

static void glade_util_find_widget_recursive (GtkWidget *widget,
					      GladeFindWidgetData *data);


/* This shows a simple dialog box with a label and an 'OK' button.
   Example usage:
    glade_util_show_message_box ("Error saving file", NULL);
*/
void
glade_util_show_message_box	(const gchar	*message,
				 GtkWidget	*transient_widget)
{
  GtkWidget *dialog;
  GtkWindow *transient_parent;

  transient_parent = (GtkWindow *)glade_util_get_toplevel (transient_widget);
  dialog = gtk_message_dialog_new (transient_parent,
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_ERROR,
				   GTK_BUTTONS_OK,
				   "%s", message);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}


/* This shows a dialog box with a message and an Entry for entering a value.
 * When the OK button is pressed the handler of the specified widget will
 * be called with the value and the given data.
 * NOTE: The dialog is automatically destroyed when any button is clicked.

   Example usage:
     glade_util_show_entry_dialog ("Name:", "default", widget, on_dialog_ok,
				   "NewName", NULL);

     void
     on_dialog_ok(GtkWidget *widget, gchar *value, gpointer data)
     {
     ...

*/
void
glade_util_show_entry_dialog	(const gchar	*message,
				 const gchar	*initial_value,
				 GtkWidget	*widget,
				 GbEntryDialogFunc signal_handler,
				 gpointer	 data,
				 GtkWidget	*transient_widget)
{
  GtkWidget *dialog, *hbox, *label, *entry, *button, *toplevel;

  dialog = gtk_dialog_new ();
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  /* FIXME: Passing a function as a gpointer isn't compatable with ANSI. */
  gtk_object_set_data (GTK_OBJECT (dialog), "handler", signal_handler);
  gtk_object_set_data (GTK_OBJECT (dialog), "widget", widget);
  gtk_widget_ref (widget);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  entry = glade_util_entry_new (GTK_OBJECT (dialog));
  if (initial_value)
    gtk_entry_set_text (GTK_ENTRY (entry), initial_value);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);
  gtk_widget_grab_focus (entry);
  gtk_object_set_data (GTK_OBJECT (dialog), "entry", entry);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button,
		      FALSE, TRUE, 20);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button,
		      FALSE, TRUE, 20);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_entry_dialog_ok),
		      data);

  gtk_signal_connect (GTK_OBJECT (dialog), "key_press_event",
		      GTK_SIGNAL_FUNC (glade_util_check_key_is_esc),
		      GINT_TO_POINTER (GladeEscDestroys));
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_entry_dialog_destroy), widget);

  if (transient_widget)
    {
      toplevel = glade_util_get_toplevel (transient_widget);
      if (toplevel && GTK_IS_WINDOW (toplevel) && GTK_WIDGET_MAPPED (toplevel))
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
				      GTK_WINDOW (toplevel));
    }

  gtk_widget_show (dialog);
}


static void
on_entry_dialog_destroy (GtkWidget * widget, gpointer data)
{
  gtk_widget_unref (GTK_WIDGET (data));
}


static void
on_entry_dialog_ok (GtkWidget * widget, gpointer data)
{
  GtkWidget *dialog = gtk_widget_get_toplevel (widget);
  GbEntryDialogFunc handler;
  GtkWidget *entry;
  gchar *text;
  gboolean close = TRUE;

  /* FIXME: ANSI forbids casting function pointer to data pointer. */
  handler = (GbEntryDialogFunc) (gtk_object_get_data (GTK_OBJECT (dialog),
						      "handler"));
  widget = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (dialog), "widget"));
  entry = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (dialog), "entry"));
  g_return_if_fail (entry != NULL);
  text = (gchar*) gtk_entry_get_text (GTK_ENTRY (entry));

  if (handler)
    close = (*handler) (widget, text, data);
  if (close)
    gtk_widget_destroy (dialog);
}

static void
on_dialog_response (GtkDialog *dialog, gint response, gpointer ok_handler_data)
{
  GladeDialogCallback ok_handler;

  if (response == GTK_RESPONSE_OK)
    {
      ok_handler = (GladeDialogCallback) g_object_get_data (G_OBJECT (dialog), "ok_handler");
      ok_handler (dialog, ok_handler_data);
    }
  else
    {
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

/* This creates a dialog with OK & Cancel buttons, usable in plain GTK or
   Gnome, and returns a vbox which the caller can place widgets in.
   If transient_for is non-NULL, the window it is in is used to set the
   transient for relationship, so the dialog will always be above the widget.
   (This only works with GTK 1.1.6+).
   The callback will be called when the OK button is pressed. */
GtkWidget*
glade_util_create_dialog	(const gchar	   *title,
				 GtkWidget	   *transient_for,
				 GtkSignalFunc      ok_handler,
				 gpointer           ok_handler_data,
				 GtkWidget	  **vbox)
{
  GtkWidget *dialog;
  GtkWindow *transient_parent;

  transient_parent = (GtkWindow *)glade_util_get_toplevel (transient_for);
  dialog = gtk_dialog_new_with_buttons (title, transient_parent,
					0,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  g_object_set_data (G_OBJECT (dialog), "ok_handler", ok_handler);

  g_signal_connect (dialog, "response",
		    G_CALLBACK (on_dialog_response),
		    ok_handler_data);

  *vbox = GTK_DIALOG (dialog)->vbox;

  return dialog;
}


gint glade_util_check_key_is_esc (GtkWidget *widget,
				  GdkEventKey *event,
				  gpointer data)
{
  g_return_val_if_fail (GTK_IS_WINDOW (widget), FALSE);
  
  if (event->keyval == GDK_Escape)
    {
      GladeEscAction action = GPOINTER_TO_INT (data);
  
      if (action == GladeEscCloses)
        {
  	glade_util_close_window (widget);
  	return TRUE;
        }
      else if (action == GladeEscDestroys)
        { 
  	gtk_widget_destroy (widget);
  	return TRUE;
        }
      else
        return FALSE;
    }
  else
    return FALSE;
}

/* This returns a new entry ready to insert in a dialog.
   The entry is set up so that <Return> will invoke the default action. The
   returned widget must be added to a container in the dialog. */ 
extern GtkWidget *
glade_util_entry_new (GtkObject *dialog)
{
  GtkWidget *entry;

  g_return_val_if_fail (GTK_IS_WINDOW (dialog), NULL);

  entry = gtk_entry_new ();
  g_return_val_if_fail (entry != NULL, NULL);
  
  /* Make <Return> in entry field invoke dialog default */
  gtk_signal_connect_object (GTK_OBJECT (entry), "activate",
			     GTK_SIGNAL_FUNC (gtk_window_activate_default), 
			     GTK_OBJECT (dialog));

  return entry;
}


/* This returns a new spinbutton ready to insert in a dialog.
   A pointer to the spin button is added as object data to the dialog. The
   spinbutton is set up so that <Return> will invoke the default action. The
   returned widget must be added to a container in the dialog. */ 
GtkWidget *
glade_util_spin_button_new (GtkObject *dialog,
			    const gchar *key,
			    GtkAdjustment *adjustment,
			    gfloat climb_rate,
			    guint digits)
{
  GtkWidget *spinbutton;

  g_return_val_if_fail (GTK_IS_WINDOW (dialog), NULL);
  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), climb_rate,
				    digits);
  g_return_val_if_fail (spinbutton != NULL, NULL);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  
  /* save pointer to entry so we can find it easily in the OK handler */
  gtk_object_set_data (GTK_OBJECT (dialog), key, spinbutton);
  /* Make <Return> in entry field invoke dialog default */
  gtk_signal_connect_object (GTK_OBJECT (spinbutton), "activate",
			     GTK_SIGNAL_FUNC (gtk_window_activate_default), 
			     GTK_OBJECT (dialog));

  return spinbutton;
}
			    
/* This returns the index of the given gint with the given array of gints.
   If the value is not found it outputs a warning and returns -1.
   This function is intended to be used for finding the index when using
   properties which are choices. */
gint
glade_util_int_array_index (const gint array[],
			    gint array_size,
			    gint value)
{
  gint i;

  for (i = 0; i < array_size; i++)
    {
      if (array[i] == value)
	return i;
    }
  return -1;
}


/* This returns the index of the given string with the given array of strings.
   If the value is not found it outputs a warning and returns -1.
   This function is intended to be used for finding the index when using
   properties which are choices. */
gint
glade_util_string_array_index (const gchar	    *array[],
			       gint		     array_size,
			       const gchar	    *value)
{
  gint i;

  for (i = 0; i < array_size; i++)
    {
      if (array[i] && !strcmp (array[i],  value))
	return i;
    }
  return -1;
}


/* This returns TRUE if the two strings are equivalent. Note that NULL is
   considered equivalent to the empty string. */
gboolean
glade_util_strings_equivalent (const gchar *string1,
			       const gchar *string2)
{
  if (string1 && string1[0] == '\0')
    string1 = NULL;
  if (string2 && string2[0] == '\0')
    string2 = NULL;

  if (string1 == NULL)
    {
      if (string2 == NULL)
	return TRUE;
      else
	return FALSE;
    }
  else if (string2 == NULL)
    return FALSE;
  else if (!strcmp (string1, string2))
    return TRUE;
  return FALSE;
}


/* This returns a copy of the given string, or NULL if the string is NULL
   or empty, i.e. "". */
gchar*
glade_util_copy_string	(const gchar	    *string)
{
  if (string == NULL || string[0] == '\0')
    return NULL;
  return g_strdup (string);
}


/* Returns TRUE if string contains substring. Returns FALSE if substring is
   not found or is NULL. */
gboolean
glade_util_strstr (const gchar * string,
		   const gchar * substring)
{
  gchar first_char;
  gint string_len, substring_len, i, j;
  gboolean found;

  if (!string || !substring)
    return FALSE;

  first_char = substring[0];
  string_len = strlen (string);
  substring_len = strlen (substring);

  if (string_len < substring_len)
    return FALSE;
  for (i = 0; i <= string_len - substring_len; i++)
    {
      if (string[i] == first_char)
	{
	  found = TRUE;
	  for (j = 1; j < substring_len; j++)
	    {
	      if (string[i + j] != substring[j])
		{
		  found = FALSE;
		  break;
		}
	    }
	  if (found == TRUE)
	    return TRUE;
	}
    }
  return FALSE;
}


/* This looks for '::' in a property name and returns a pointer to the next
   character. If '::' is not found it returns the argument. */
gchar *
glade_util_find_start_of_tag_name (const gchar * tag_name)
{
  register const gchar *pos = tag_name;

  while (*pos && (*pos != ':' || *(pos + 1) != ':'))
     pos++;
  return (gchar*) (*pos ? pos + 2 : tag_name);
}


/* This creates a string representing the modifiers flags, e.g.
   "GDK_CONTROL_MASK | GDK_SHIFT_MASK". It is used for saving the XML, and
   also for writing the source code. It uses a static buffer which is
   overwritten for each call. */
gchar*
glade_util_create_modifiers_string (guint8 modifier_flags)
{
  static gchar modifiers[128];

  modifiers[0] = '\0';
  if (modifier_flags == 0)
    {
      strcat (modifiers, "0");
      return modifiers;
    }

  if (modifier_flags & GDK_CONTROL_MASK)
    {
      strcat (modifiers, "GDK_CONTROL_MASK");
    }
  if (modifier_flags & GDK_SHIFT_MASK)
    {
      if (modifiers[0] != '\0')
	strcat (modifiers, " | ");
      strcat (modifiers, "GDK_SHIFT_MASK");
    }
  if (modifier_flags & GDK_MOD1_MASK)
    {
      if (modifiers[0] != '\0')
	strcat (modifiers, " | ");
      strcat (modifiers, "GDK_MOD1_MASK");
    }
  return modifiers;
}


/* This creates a GtkPixmap widget, using a colormap and xpm data from an
   '#include'd pixmap file. */
GtkWidget*
glade_util_create_pixmap_using_colormap (GdkColormap  *colormap,
					 gchar **xpm_data)
{
  GtkWidget *pixmap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;

  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
						     NULL, (gchar**) xpm_data);
  pixmap = gtk_pixmap_new (gdkpixmap, mask);

  /* The GtkPixmap assumes the reference count, so we can unref them. */
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);

  return pixmap;
}


/* This returns TRUE if the widget is a toplevel project component,
   i.e. a window, dialog or popup menu. */
gboolean
glade_util_is_component	(GtkWidget	   *widget)
{
  if (widget->parent == NULL)
    return TRUE;

  /* Note that menus may have submenus, so we make sure that this is the
     toplevel menu. */
  if (GTK_IS_MENU (widget)
      && gtk_menu_get_attach_widget (GTK_MENU (widget)) == NULL)
    return TRUE;

  return FALSE;
}


/* This returns the toplevel window/dialog/menu that contains the given
   widget. It even walks up menus, which gtk_widget_get_toplevel() does not. */
GtkWidget*
glade_util_get_toplevel (GtkWidget *widget)
{
  GtkWidget *parent;

  if (widget == NULL)
    return NULL;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (parent == NULL)
        break;
      widget = parent;
    }

  return widget;
}


/* This returns the closes ancestor of the given widget which is a GbWidget. */
GtkWidget*
glade_util_get_parent	(GtkWidget	   *widget)
{
  GtkWidget *parent;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
	parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
	parent = widget->parent;

      /* There is no way to move up from the combo popup list window to the
	 combo, so we added a special pointer. */
      if (!parent)
	parent = gtk_object_get_data (GTK_OBJECT (widget), GladeParentKey);

      if (parent == NULL)
        break;

      if (GB_IS_GB_WIDGET (parent))
	return parent;

      widget = parent;
    }

  return widget;
}


/* This is used when setting up keyboard accelerators resulting from underlined
   keys in labels. It returns the default mnemonic widget to use. */
GtkWidget*
glade_util_find_default_accelerator_target	(GtkWidget	   *label)
{
  GtkWidget *parent, *child, *target = NULL;

  g_return_val_if_fail (GTK_IS_LABEL (label), NULL);

  /* Now try to find the widget to the right of the label. We can do this
     fairly easily for tables, boxes, and fixed containers. But the packer
     container may be more difficult. */
  parent = label->parent;
  child = label;
  while (parent)
    {
      if (GTK_IS_TABLE (parent))
	target = glade_util_find_table_accelerator_target (parent, child);
      else if (GTK_IS_HBOX (parent))
	target = glade_util_find_box_accelerator_target (parent, child);
      else if (GTK_IS_FIXED (parent))
	target = glade_util_find_fixed_accelerator_target (parent, child);
      else if (GTK_IS_LAYOUT (parent))
	target = glade_util_find_layout_accelerator_target (parent, child);

      if (target)
	{
	  return target;
	}

      child = parent;
      parent = parent->parent;
    }

  return NULL;
}


/* This tries to find a named widget in a component. */
GtkWidget*
glade_util_find_widget (GtkWidget *widget, gchar *name)
{
  GladeFindWidgetData data;

  data.name = name;
  data.found_widget = NULL;

  glade_util_find_widget_recursive (widget, &data);

  return data.found_widget;
}


static void
glade_util_find_widget_recursive (GtkWidget *widget,
				  GladeFindWidgetData *data)
{
  if (!data->found_widget && widget->name
      && !strcmp (widget->name, data->name))
    {
      data->found_widget = widget;
    }

  if (GTK_IS_CONTAINER (widget))
    {
      gtk_container_forall (GTK_CONTAINER (widget),
			    (GtkCallback) glade_util_find_widget_recursive,
			    data);
    }
}


/* This tries to find the widget on the right of the given child. */
static GtkWidget*
glade_util_find_table_accelerator_target (GtkWidget *parent, GtkWidget *child)
{
  GtkTableChild *tchild, *tchild2;
  GList *children;

  tchild = glade_util_find_table_child (GTK_TABLE (parent), child);
  g_return_val_if_fail (tchild != NULL, NULL);

  children = GTK_TABLE (parent)->children;
  while (children)
    {
      tchild2 = (GtkTableChild*) children->data;

      if (tchild2->widget != tchild->widget
	  && tchild2->left_attach == tchild->right_attach
	  && tchild2->top_attach < tchild->bottom_attach
	  && tchild2->bottom_attach > tchild->top_attach)
	{
	  return glade_util_find_focus_child (tchild2->widget);
	}
      children = children->next;
    }
  return NULL;
}


/* This tries to find the widget on the right of the given child. */
static GtkWidget*
glade_util_find_box_accelerator_target (GtkWidget *parent, GtkWidget *child)
{
  GtkBoxChild *bchild, *bchild2;
  GList *children, *elem;
  gint pos;

  pos = glade_util_get_box_pos (GTK_BOX (parent), child);
  g_return_val_if_fail (pos != -1, NULL);

  children = GTK_BOX (parent)->children;
  elem = g_list_nth (children, pos);
  bchild = (GtkBoxChild*) elem->data;

  /* If the child has PACK_START set, try to find the next child also with
     PACK_START set. If that can't be found, try to find the last child with
     PACK_END set, since that will be nearest the middle.
     If the child has PACK_END set, find the previous child with PACK_END. */
  if (bchild->pack == GTK_PACK_START)
    {
      while (elem->next)
	{
	  elem = elem->next;
	  bchild2 = (GtkBoxChild*) elem->data;
	  if (bchild2->pack == GTK_PACK_START)
	    return glade_util_find_focus_child (bchild2->widget);
	}
      /* We couldn't find another child with PACK_START, so we drop through
	 to the second loop with elem containing the last list element. */
    }
  else
    {
      elem = elem->prev;
    }

  /* Step back through the list trying to find a child with PACK_END set. */
  while (elem)
    {
      bchild2 = (GtkBoxChild*) elem->data;
      if (bchild2->pack != GTK_PACK_END)
	return glade_util_find_focus_child (bchild2->widget);
      elem = elem->prev;
    }
  return NULL;
}


/* This tries to find the widget on the right of the given child. */
static GtkWidget*
glade_util_find_fixed_accelerator_target (GtkWidget *parent, GtkWidget *child)
{
  GtkFixedChild *fchild, *fchild2;
  GtkWidget *best_target = NULL;
  gint distance, best_distance = -1;
  GList *children;

  fchild = glade_util_find_fixed_child (GTK_FIXED (parent), child);
  g_return_val_if_fail (fchild != NULL, NULL);

  children = GTK_BOX (parent)->children;
  while (children)
    {
      fchild2 = (GtkFixedChild*) children->data;

      /* We allow a few pixels difference in the y coordinate.
	 The x coordinate must be greater than the given child label. */
      if (fchild2->widget != fchild->widget
	  && abs (fchild2->y - fchild->y) < 5
	  && fchild2->x > fchild->x)
	{
	  distance = fchild2->x - fchild->x;

	  if (best_distance == -1 || distance < best_distance)
	    {
	      best_distance = distance;
	      best_target = fchild2->widget;
	    }
	}

      children = children->next;
    }
    
  return glade_util_find_focus_child (best_target);
}


/* This tries to find the widget on the right of the given child. */
static GtkWidget*
glade_util_find_layout_accelerator_target (GtkWidget *parent, GtkWidget *child)
{
  GladeLayoutCallbackData data;

  data.best_distance = -1;
  data.best_target = NULL;
  data.child = child;
  data.x = child->allocation.x;
  data.y = child->allocation.y;
  gtk_container_forall (GTK_CONTAINER (parent),
			(GtkCallback)glade_util_find_layout_acclererator_target_cb,
			&data);
  return glade_util_find_focus_child (data.best_target);
}

static void
glade_util_find_layout_acclererator_target_cb (GtkWidget *widget,
					       GladeLayoutCallbackData *data)
{
  gint distance;

  /* We allow a few pixels difference in the y coordinate.
     The x coordinate must be greater than the given child label. */
  if (widget != data->child
      && abs (widget->allocation.y - data->y) < 5
      && widget->allocation.x > data->x)
    {
      distance = widget->allocation.x - data->x;

      if (data->best_distance == -1 || distance < data->best_distance)
	{
	  data->best_distance = distance;
	  data->best_target = widget;
	}
    }
}


/* This tries to find a widget which can accept the input focus, within the
   given widget and its descendants. Note that this will only work if the
   child is a GbWidget, since gb_label_write_source() assumes there is a
   variable declared corresponding to the widget. */
static GtkWidget*
glade_util_find_focus_child (GtkWidget *widget)
{
  if (widget == NULL)
    return NULL;

  if (GTK_WIDGET_CAN_FOCUS (widget))
    return widget;

  if (GTK_IS_COMBO (widget))
    return GTK_COMBO (widget)->entry;

#ifdef USE_GNOME
  if (GNOME_IS_FILE_ENTRY (widget))
    return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (widget));
#endif

  return NULL;
}


gint
glade_util_get_box_pos (GtkBox * box,
			 GtkWidget * widget)
{
  GList *children;
  GtkBoxChild *child;
  guint pos = 0;

  children = box->children;
  while (children)
    {
      child = children->data;
      if (widget == child->widget)
	return pos;
      pos++;
      children = children->next;
    }
  g_warning (_("Widget not found in box"));
  return -1;
}


GtkTableChild *
glade_util_find_table_child (GtkTable * table,
			     GtkWidget * widget)
{
  GList *children;
  GtkTableChild *child;

  children = table->children;
  while (children)
    {
      child = children->data;
      if (widget == child->widget)
	return child;
      children = children->next;
    }
  g_warning (_("Widget not found in table"));
  return NULL;
}


GtkBoxChild *
glade_util_find_box_child (GtkBox * box,
			   GtkWidget * widget)
{
  GList *children;
  GtkBoxChild *child;

  children = box->children;
  while (children)
    {
      child = children->data;
      if (widget == child->widget)
	return child;
      children = children->next;
    }
  g_warning (_("Widget not found in box"));
  return NULL;
}


GtkFixedChild*
glade_util_find_fixed_child	(GtkFixed	   *fixed,
				 GtkWidget	   *widget)
{
  GList *children;
  GtkFixedChild *child;

  children = fixed->children;
  while (children)
    {
      child = children->data;
      if (widget == child->widget)
	return child;
      children = children->next;
    }
  g_warning (_("Widget not found in fixed container"));
  return NULL;
}


#if GLADE_SUPPORTS_GTK_PACKER
GtkPackerChild *
glade_util_find_packer_child (GtkPacker * packer,
			      GtkWidget * widget)
{
  GList *children;
  GtkPackerChild *child;

  children = packer->children;
  while (children)
    {
      child = children->data;
      if (widget == child->widget)
	return child;
      children = children->next;
    }
  g_warning (_("Widget not found in packer"));
  return NULL;
}
#endif


/* This returns the GtkLabel's text, including the underline characters.
   We don't really need it now for GTK+ 2.0. Free the returned string. */
gchar*
glade_util_get_label_text (GtkWidget *label)
{
  char *result;

  /* Hopefully this will work in GTK+ 2.0. */
  result = (gchar*) gtk_label_get_label (GTK_LABEL (label));

#if 0
  g_print ("Label text: %s\n", result);
#endif

  /* We strdup it just to remain compatable with the old version. */
  return g_strdup (result);

#if 0
  /* This is the old GTK+ 1.2 version, for reference. */
  GdkWChar *label_wc, *label_with_underscores;
  gchar *label_text, *label_pattern, *result;
  gint len, i, j;
  gboolean in_pattern;

  g_return_val_if_fail (GTK_IS_LABEL (label), NULL);

  /* We assume that the multi-byte and wide char versions of the text match. */
  label_text = GTK_LABEL (label)->label;
  label_wc = GTK_LABEL (label)->label_wc;
  label_pattern = GTK_LABEL (label)->pattern;
  /* The maximum space we need is 2 * the string length, i.e. when all letters
     are underlined. */
  len = strlen (label_text);
  label_with_underscores = g_new (GdkWChar, len * 2 + 1);

  /* We are careful in case the pattern is shorter than the label text. */
  in_pattern = label_pattern ? TRUE : FALSE;
  for (i = 0, j = 0; i < len; i++)
    {
      if (in_pattern && label_pattern[i])
	{
	  if (label_pattern[i] == '_')
	    label_with_underscores[j++] = '_';
	}
      else
	in_pattern = FALSE;
      label_with_underscores[j++] = label_wc[i];

      if (label_wc[i] == '_')
	label_with_underscores[j++] = '_';
    }
  label_with_underscores[j] = '\0';

  /* Now convert it to multi-byte. */
  result = gdk_wcstombs (label_with_underscores);
  g_free (label_with_underscores);

  return result;
#endif
}


/* This should be hooked up to the delete_event of windows which you want
   to hide, so that if they are shown again they appear in the same place.
   This stops the window manager asking the user to position the window each
   time it is shown, which is quite annoying. */
gint
glade_util_close_window_on_delete (GtkWidget * widget,
				   GdkEvent * event,
				   gpointer data)
{
  glade_util_close_window (widget);
  return TRUE;
}


gint
glade_util_close_window (GtkWidget * widget)
{
  gint x, y;
  gboolean set_position = FALSE;

  /* remember position of window for when it is used again */
  if (widget->window)
    {
      gdk_window_get_root_origin (widget->window, &x, &y);
      set_position = TRUE;
    }

  gtk_widget_hide (widget);

  if (set_position)
    gtk_widget_set_uposition (widget, x, y);

  return TRUE;
}


/* Returns TRUE if the given file exists. filename must be UTF-8. */
gboolean
glade_util_file_exists (const gchar *filename)
{
  int status;
  struct stat filestat;
  gchar *on_disk_filename;

  on_disk_filename = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
  status = stat (on_disk_filename, &filestat);
  g_free (on_disk_filename);

  if (status == -1 && errno == ENOENT)
    return FALSE;
  return TRUE;
}


/* Returns the last modification time of the given file, or 0 if it doesn't
   exist, or -1 on error. filename must be UTF-8. */
GladeError*
glade_util_file_last_mod_time (const gchar *filename, time_t *last_mod_time)
{
  int status;
  struct stat filestat;
  gchar *on_disk_filename;

  on_disk_filename = g_filename_from_utf8 (filename, -1, NULL, NULL, NULL);
  status = stat (on_disk_filename, &filestat);
  g_free (on_disk_filename);

  if (status == -1)
    {
      return glade_error_new_system (_("Couldn't access file:\n  %s\n"),
				     filename);
    }

  *last_mod_time = filestat.st_mtime;
  return NULL;
}


/* This copies a file from src to dest, and returns a GladeError if an error
   occurs. src & dest must be UTF-8. */
GladeError*
glade_util_copy_file (const gchar *src,
		      const gchar *dest)
{
  FILE *input_fp, *output_fp;
  gchar buffer[READ_BUFFER_SIZE];
  gint bytes_read, bytes_written;
  GladeError *error = NULL;

  input_fp = glade_util_fopen (src, "r");
  if (input_fp == NULL)
    {
      return glade_error_new_system (_("Couldn't open file:\n  %s\n"), src);
    }

  output_fp = glade_util_fopen (dest, "w");
  if (output_fp == NULL)
    {
      error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
				      dest);
      fclose (input_fp);
      return error;
    }

  for (;;)
    {
      bytes_read = fread (buffer, 1, READ_BUFFER_SIZE, input_fp);
      if (bytes_read != READ_BUFFER_SIZE && ferror (input_fp))
	{
	  error = glade_error_new_system (_("Error reading from file:\n  %s\n"),
					  src);
	  break;
	}

      if (bytes_read)
	{
	  bytes_written = fwrite (buffer, 1, bytes_read, output_fp);
	  if (bytes_read != bytes_written)
	    {
	      error = glade_error_new_system (_("Error writing to file:\n  %s\n"),
					      dest);
	      break;
	    }
	}

      if (bytes_read != READ_BUFFER_SIZE && feof (input_fp))
	{
	  break;
	}
    }

  fclose (input_fp);
  fclose (output_fp);

  return error;
}


/* Creates a directory if it doesn't already exist. directory must be an
   absolute path, in UTF-8. */
GladeError*
glade_util_ensure_directory_exists (const gchar *directory)
{
  GladeError *error = NULL;
  struct stat filestat;
  gchar *on_disk_directory;

  g_return_val_if_fail (g_path_is_absolute (directory), NULL);

  on_disk_directory = g_filename_from_utf8 (directory, -1, NULL, NULL, NULL);
  if (stat (on_disk_directory, &filestat) != 0)
    {
      /* If the directory doesn't exist, try to create it. */
      if (errno == ENOENT)
	{
	  gchar *parent_dir;

	  /* First make sure the parent directory exists. */
	  parent_dir = glade_util_parent_directory (directory);
	  if (parent_dir)
	    {
	      error = glade_util_ensure_directory_exists (parent_dir);
	      g_free (parent_dir);
	    }

	  if (!error)
	    {
#ifdef _WIN32
	      if (mkdir (on_disk_directory) != 0)
#else
	      if (mkdir (on_disk_directory, 0777) != 0)
#endif
		{
#ifndef _WIN32
/* This happens under WIN32 when stat is confused by the filename, but this is
   harmless, since we know that the directory exists after all. */
		  error = glade_error_new_system (_("Couldn't create directory:\n  %s\n"), directory);
#endif
		}
	    }
	}
      else
	{
	  error = glade_error_new_system (_("Couldn't access directory:\n  %s\n"), directory);
	}
    }
#ifndef _WIN32
  /* If the directory does exist, check it is a directory. */
  else if (!S_ISDIR (filestat.st_mode))
    {
      error = glade_error_new_general (GLADE_STATUS_INVALID_DIRECTORY,
				       _("Invalid directory:\n  %s\n"),
				       directory);
    }
#endif

  g_free (on_disk_directory);

  return error;
}


/* Adds a filename onto a directory to make a complete pathname.
   The directory may or may not end in '/'. file must be a simple filename.
   Free the returned string when no longer needed. */
gchar*
glade_util_make_path		(const gchar	    *dir,
				 const gchar	    *file)
{
  gint dir_len;

  g_return_val_if_fail (dir != NULL, NULL);
  g_return_val_if_fail (file != NULL, NULL);

  dir_len = strlen (dir);
  g_return_val_if_fail (dir_len > 0, NULL);

  if (dir[dir_len - 1] == G_DIR_SEPARATOR)
    return g_strdup_printf ("%s%s", dir, file);
  else
    return g_strdup_printf ("%s%c%s", dir, G_DIR_SEPARATOR, file);
}


/* This turns a relative pathname into an absolute one based on the given
   base directory (which MUST be absolute).
   e.g. "/home/damon" + "../dave/test" -> "/home/dave/test"
   The returned path should be freed when no longer needed. */
gchar*
glade_util_make_absolute_path (const gchar *dir, const gchar *file)
{
  gint dir_pos, file_pos, len, root_pos = 0;
  gchar *path;

  g_return_val_if_fail (dir != NULL, NULL);

  if (file == NULL || file[0] == '\0')
    return g_strdup (dir);

  if (g_path_is_absolute (file))
    return g_strdup (file);

  /* For windows if dir has a drive set, e.g. "C:\", we never delete that. */
#ifdef NATIVE_WIN32
  if (isalpha (dir[0]) && dir[1] == ':' && dir[2] == G_DIR_SEPARATOR)
    root_pos = 2;
#endif

  /* Start at last character in dir. */
  dir_pos = strlen (dir) - 1;

  /* First we make sure we skip any '/' at the end of dir. */
  if (dir_pos > root_pos && dir[dir_pos] == G_DIR_SEPARATOR)
    dir_pos--;

  /* Now for each '..' in file, we step back one component in dir, and
     forward one component in file. */
  file_pos = 0;
  for (;;)
    {
      /* Skip './' */
      if (file[file_pos] == '.' && file[file_pos + 1] == G_DIR_SEPARATOR)
	file_pos += 2;

      else if (file[file_pos] == '.' && file[file_pos + 1] == '.'
	       && (file[file_pos + 2] == G_DIR_SEPARATOR
		   || file[file_pos + 2] == '\0'))
	{
	  while (dir_pos > root_pos && dir[dir_pos] != G_DIR_SEPARATOR)
	    dir_pos--;
	  if (dir_pos > root_pos)
	    dir_pos--;

	  if (file[file_pos + 2] == G_DIR_SEPARATOR)
	    file_pos += 3;
	  else
	    file_pos += 2;
	}

      else
	break;
    }

  /* Now concatenate the parts of dir and file together. */
  if (dir_pos > root_pos)
    dir_pos++;
  len = dir_pos + 1 + (strlen (file) - file_pos) + 1;
  path = g_malloc (len); 
  strncpy (path, dir, dir_pos);
  path[dir_pos] = G_DIR_SEPARATOR;
  strcpy (path + dir_pos + 1, file + file_pos);
  return path;
}


/* This turns an absolute pathname into an relative one based on the given
   base directory. Both arguments must be absolute paths, and should be in
   canonical form, i.e. not containing '..', '.' or multiple '/'s together.
   The returned value may or may not end with a '/', depending on the
   arguments.
   e.g. "/home/damon" + "/home/damon/pixmaps" -> "pixmaps"
        "/home/damon" + "/home/damon/pixmaps/" -> "pixmaps/"
        "/home/damon/project" + "/home/damon/test/pic.xpm" -> "../test/pic.xpm"
        "/home/damon/project" + "/home/damon/project" -> ""
        "/home/damon/project" + "/home/damon" -> "../"
   The returned path should be freed when no longer needed. */
gchar*
glade_util_make_relative_path (const gchar *base, const gchar *path)
{
  gchar *relative_path;
  gint pos, num_parents = 0, len, path_len, i, root_pos = 0;
  gboolean match = FALSE;

  if (path == NULL)
    return g_strdup ("");

  /* For windows if base has a drive set, e.g. "C:\", then path must start with
     the same drive or we just return path. */
#ifdef NATIVE_WIN32
  if (isalpha (base[0]) && base[1] == ':' && base[2] == G_DIR_SEPARATOR)
    {
      root_pos = 2;
      if (!isalpha (path[0]) || path[1] != ':' || path[2] != G_DIR_SEPARATOR
	  || base[0] != path[0])
	return g_strdup (path);
    }
#endif

  /* We step up each component of base_dir until we find a match with the
     start of file. */
  pos = strlen (base) - 1;
  for (;;)
    {
      /* Skip trailing '/'s. */
      if (pos > root_pos && base[pos] == G_DIR_SEPARATOR)
	pos--;

      match = check_components_match (base, path, root_pos, pos + 1);
      if (match)
	break;

      /* They didn't match, so we move up one component and try again. */
      num_parents++;
      while (pos && base[pos] != G_DIR_SEPARATOR)
	pos--;
    }

  /* If we match some components, build the relative path, else just return
     path. */
  if (match)
    {
      path_len = strlen (path);
      /* Skip over the '/', but special case for root directory. */
      if (pos == root_pos)
	pos++;
      else
	pos += 2;
      len = (num_parents * 3) + 1;
      if (path_len > pos)
	len += path_len - pos;
      relative_path = g_malloc (len);
      /* Add a '../' for each parent directory needed. */
      for (i = 0; i < num_parents; i++)
	{
	  relative_path[i * 3]     = '.';
	  relative_path[i * 3 + 1] = '.';
	  relative_path[i * 3 + 2] = G_DIR_SEPARATOR;
	}
      /* Add on the end of path, skipping the '/' after the component. */
      if (path_len > pos)
	strcpy (relative_path + num_parents * 3, path + pos);
      relative_path[len - 1] = '\0';

      return relative_path;
    }
  else
    return g_strdup (path);
}


/* This checks that the leading directory components of base match those in
   path up to the given length. */
static gboolean
check_components_match (const gchar *base,
			const gchar *path,
			gint         root_pos,
			gint         len)
{
  if (strncmp (base, path, len))
    return FALSE;

  /* We also need to check that it is a complete component in path, i.e.
     "/home/damon" should NOT match "/home/damon2". But root dir is a
     special case. "/" matches "/home". */
  if (len == root_pos + 1 || path[len] == '\0' || path[len] == G_DIR_SEPARATOR)
    return TRUE;
  return FALSE;
}


/* Returns TRUE if file is in dir. Both paths must be absolute. file can
   be a directory, and both can end in '/' or not. Note that we assume
   that both are in proper form, i.e. there are no instances of '//', '.',
   or '..' in either. */
gboolean
glade_util_directory_contains_file (const gchar *dir,
				    const gchar *file)
{
  gint dir_len, file_len;
  gchar *next_dir_separator;

  g_return_val_if_fail (g_path_is_absolute (dir), FALSE);
  g_return_val_if_fail (g_path_is_absolute (file), FALSE);

  dir_len = strlen (dir);
  file_len = strlen (file);

  /* First check that file matches dir up until dir finishes. */
  if (strncmp (dir, file, dir_len))
    return FALSE;

  /* If dir doesn't end in a '/',  then we also need to check that the next
     character in file is a '/'. */
  if (dir[dir_len - 1] != G_DIR_SEPARATOR)
    {
      if (file[dir_len] != G_DIR_SEPARATOR)
	return FALSE;
      dir_len++;
    }

  /* If file is equivalent to dir we return FALSE. */
  if (dir_len == file_len)
    return FALSE;

  /* Now we need to check that file is not in a subdirectory, i.e. there are
     no more '/'s in file except possibly for the last character. */
  next_dir_separator = strchr (file + dir_len, G_DIR_SEPARATOR);
  if (next_dir_separator && next_dir_separator < file + file_len - 1)
    return FALSE;

  return TRUE;
}


/* Returns TRUE if the 2 directories are equivalent. Both must be absolute
   paths, and may or may not end in '/'. */
gboolean
glade_util_directories_equivalent  (const gchar	    *dir1,
				    const gchar	    *dir2)
{
  gint dir1_len, dir2_len;

  g_return_val_if_fail (g_path_is_absolute (dir1), FALSE);
  g_return_val_if_fail (g_path_is_absolute (dir2), FALSE);

  /* Find the length of both directories and decrement it if they end in a
     '/'. */
  dir1_len = strlen (dir1);
  dir2_len = strlen (dir2);

  if (dir1[dir1_len - 1] == G_DIR_SEPARATOR)
    dir1_len--;
  if (dir2[dir2_len - 1] == G_DIR_SEPARATOR)
    dir2_len--;

  /* Now both lengths must be equal and the directories must match up to
     that point. */
  if (dir1_len != dir2_len)
    return FALSE;

  if (strncmp (dir1, dir2, dir1_len))
    return FALSE;

  return TRUE;
}


/* This is similar to GLib's dirname, but it makes sure the dirname ends with
   a G_DIR_SEPARATOR. The returned string should be freed later. */
gchar*
glade_util_dirname (const gchar	   *file_name)
{
  register gchar *base;
  register guint len;
  
  g_return_val_if_fail (file_name != NULL, NULL);
  
  base = strrchr (file_name, G_DIR_SEPARATOR);
  if (!base)
    return g_strdup ("." G_DIR_SEPARATOR_S);
  while (base > file_name && *base == G_DIR_SEPARATOR)
    base--;
  len = (guint) 1 + base - file_name;
  
  base = g_new (gchar, len + 2);
  g_memmove (base, file_name, len);
  if (len > 1)
    base[len++] = G_DIR_SEPARATOR;
  base[len] = 0;
  
  return base;
}


/* This returns the parent directory of the given directory, which may or may
   not end in a G_DIR_SEPARATOR. If there is no parent directory, NULL is
   returned. The returned string should be freed. dir MUST be absolute. */
gchar*
glade_util_parent_directory	(const gchar	    *dir)
{
  gchar *skipped_root, *parent_dir;
  gint pos;

  g_return_val_if_fail (g_path_is_absolute (dir), NULL);

  /* We handle the root dir specially here, and return NULL. */
  skipped_root = (gchar*) g_path_skip_root ((gchar*) dir);
  if (*skipped_root == '\0')
    return NULL;

  /* Ignore any G_DIR_SEPARATOR at the end of dir (just by skipping the last
     char), and step back to the previous G_DIR_SEPARATOR. */
  pos = strlen (dir) - 2;
  while (pos >= 0 && dir[pos] != G_DIR_SEPARATOR)
    pos--;

  /* This shouldn't really happen, since we dealt with the root dir above,
     but just in case. */
  if (pos < 0)
    return NULL;

  /* Check if the parent directory is the root directory. If it is, we just
     want to return the root directory, i.e. "/" or "C:\". */
  if (pos <= skipped_root - dir)
    pos = skipped_root - dir;

  parent_dir = g_malloc (pos + 1);
  strncpy (parent_dir, dir, pos);
  parent_dir[pos] = '\0';

  return parent_dir;
}


/* This searches the $HOME/Projects directory to find the default directory to
   use for the next project, e.g. $HOME/Projects/project1. The returned
   directory should be freed when no longer needed. */
GladeError*
glade_util_get_next_free_project_directory (gchar **project_directory_return,
					    gint   *project_num_return)
{
  GladeError *error = NULL;
  gchar *projects_dir, *project_string, *subdir;
  const gchar *home_dir;
  DIR *directory;
  struct dirent *entry;
  gint project_num, max_project_num, project_string_len;
  gint num_matched, chars_matched;

  home_dir = g_get_home_dir ();
  if (home_dir)
    {
      projects_dir = glade_util_make_absolute_path (home_dir, _("Projects"));
    }
  else
    {
#ifdef _WIN32
      projects_dir = g_strdup ("C:\\Projects");
#else
      /* Just use the root directory. Though it probably won't be writable. */
      projects_dir = g_strdup (G_DIR_SEPARATOR_S);
#endif
    }

  /* Step through the 'Projects' directory, if it exists, to find
     subdirectories named 'projectXX', and get the highest number used so
     far. */
  max_project_num = 0;
  directory = opendir (projects_dir);
  project_string = _("project");
  project_string_len = strlen (project_string);
  if (directory == NULL)
    {
      if (errno != ENOENT)
	{
	  error = glade_error_new_system (_("Couldn't open directory:\n  %s\n"),
					  projects_dir);
	  g_free (projects_dir);
	  return error;
	}
    }
  else
    {
      for (;;)
	{
	  entry = readdir (directory);

	  if (entry == NULL)
	    break;

	  if (!strncmp (entry->d_name, project_string, project_string_len))
	    {
	      /* Now see if it has a number on the end. */
	      num_matched = sscanf (entry->d_name + project_string_len,
				    "%i%n", &project_num, &chars_matched);
	      if (num_matched >= 1
		  && chars_matched == strlen (entry->d_name) - project_string_len)
		{
		  max_project_num = MAX (max_project_num, project_num);
		}
	    }
	}

      closedir (directory);
    }

  max_project_num++;
  *project_num_return = max_project_num;
  subdir = g_strdup_printf ("%s%i", project_string, max_project_num);
  *project_directory_return = glade_util_make_absolute_path (projects_dir,
							     subdir);
  g_free (subdir);
  g_free (projects_dir);

  return NULL;
}


/* This will hold the last "TZ=XXX" string we used with putenv(). After we
   call putenv() again to set a new TZ string, we can free the previous one.
   As far as I know, no libc implementations actually free the memory used in
   the environment variables (how could they know if it is a static string or
   a malloc'ed string?), so we have to free it ourselves. */
static char* saved_tz = NULL;


/* Sets the TZ environment variable to the given value, e.g. "UTC", returning
   the old setting.
   NOTE: You must call glade_util_reset_timezone() some time later to restore
   the original TZ. Pass glade_util_reset_timezone() the string that
   glade_util_set_timezone() returns. */
gchar*
glade_util_set_timezone		(const gchar	*tz)
{
  char *old_tz, *old_tz_copy = NULL, *new_tz;

  /* Get the old TZ setting and save a copy of it to return. */
  old_tz = getenv ("TZ");
  if (old_tz)
    {
      old_tz_copy = g_malloc (strlen (old_tz) + 4);
      strcpy (old_tz_copy, "TZ=");
      strcpy (old_tz_copy + 3, old_tz);
    }

  /* Create the new TZ string. */
  new_tz = g_malloc (strlen (tz) + 4);
  strcpy (new_tz, "TZ=");
  strcpy (new_tz + 3, tz);

  /* Add the new TZ to the environment. */
  putenv (new_tz); 
  tzset ();

  /* Free any previous TZ environment string we have used. */
  if (saved_tz)
    g_free (saved_tz);

  /* Save a pointer to the TZ string we just set, so we can free it later. */
  saved_tz = new_tz;

  return old_tz_copy; /* This will be zero if the TZ env var was not set */
}


void
glade_util_reset_timezone	(gchar		*tz)
{
  /* restore the original TZ setting. */
  if (tz)
    putenv (tz);
  else
    putenv ("TZ"); /* Delete from environment */

  tzset ();

  /* Free any previous TZ environment string we have used. */
  if (saved_tz)
    g_free (saved_tz);

  /* Save a pointer to the TZ string we just set, so we can free it later.
     (This can possibly be NULL if there was no TZ to restore.) */
  saved_tz = tz;
}


gboolean
glade_util_check_is_stock_id	(const gchar *stock_id)
{
  static GHashTable *stock_hash = NULL;

  gboolean retval = FALSE;

  /* Create a hash of stock ids, so future calls will be fast. */
  if (!stock_hash)
    {
      GSList *stock_items, *elem;

      stock_hash = g_hash_table_new (g_str_hash, g_str_equal);

      stock_items = gtk_stock_list_ids ();
      for (elem = stock_items; elem; elem = elem->next)
	{
	  /* We just use the stock id for the key and the data, as we don't
	     need to store anything. We should free the keys at some point,
	     but we need them until Glade exits. */
	  g_hash_table_insert (stock_hash, elem->data, elem->data);
	}

      g_slist_free (stock_items);
    }

  if (stock_id && g_hash_table_lookup (stock_hash, stock_id))
    retval = TRUE;

#if 0
  g_print ("glade_util_check_is_stock_id: %s -> %i\n",
	   stock_id ? stock_id : "", retval);
#endif

  return retval;
}


static gint
compare_uline_labels (const gchar *labela, const gchar *labelb)
{
  for (;;) {
    gunichar c1, c2;

    if (*labela == '\0')
      return (*labelb == '\0') ? 0 : -1;
    if (*labelb == '\0')
      return 1;

    c1 = g_utf8_get_char (labela);
    if (c1 == '_')
      {
	labela = g_utf8_next_char (labela);
	c1 = g_utf8_get_char (labela);
      }

    c2 = g_utf8_get_char (labelb);
    if (c2 == '_')
      {
	labelb = g_utf8_next_char (labelb);
	c2 = g_utf8_get_char (labelb);
      }

    if (c1 < c2)
      return -1;
    if (c1 > c2)
      return 1;

    labela = g_utf8_next_char (labela);
    labelb = g_utf8_next_char (labelb);
  }

  /* Shouldn't be reached. */
  return 0;
}


/* This is a GCompareFunc for comparing the labels of 2 stock items, ignoring
   any '_' characters. It isn't particularly efficient. */
gint
glade_util_compare_stock_labels (gconstpointer a, gconstpointer b)
{
  const gchar *stock_ida = a, *stock_idb = b;
  GtkStockItem itema, itemb;
  gboolean founda, foundb;
  gint retval;

  founda = gtk_stock_lookup (stock_ida, &itema);
  foundb = gtk_stock_lookup (stock_idb, &itemb);

  if (founda)
    {
      if (!foundb)
	retval = -1;
      else
	/* FIXME: Not ideal for UTF-8. */
	retval = compare_uline_labels (itema.label, itemb.label);
    }
  else
    {
      if (!foundb)
	retval = 0;
      else
	retval = 1;
    }

  return retval;
}


/* These are pinched from gtkcombo.c */
gpointer /* GtkListItem *  */
glade_util_gtk_combo_find (GtkCombo * combo)
{
  gchar *text;
  gchar *ltext;
  GList *clist;
  int (*string_compare) (const char *, const char *);

  if (combo->case_sensitive)
    string_compare = strcmp;
  else
    string_compare = g_strcasecmp;

  text = (gchar*) gtk_entry_get_text (GTK_ENTRY (combo->entry));
  clist = GTK_LIST (combo->list)->children;

  while (clist && clist->data)
    {
      ltext = glade_util_gtk_combo_func (GTK_LIST_ITEM (clist->data));
      if (!ltext)
	continue;
      if (!(*string_compare) (ltext, text))
	return (GtkListItem *) clist->data;
      clist = clist->next;
    }

  return NULL;
}

gchar *
glade_util_gtk_combo_func (gpointer data)
{
  GtkListItem * listitem = data;

  /* I needed to pinch this as well - Damon. */
  static const gchar *gtk_combo_string_key = "gtk-combo-string-value";

  GtkWidget *label;
  gchar *ltext = NULL;

  ltext = (gchar *) gtk_object_get_data (GTK_OBJECT (listitem),
					 gtk_combo_string_key);
  if (!ltext)
    {
      label = GTK_BIN (listitem)->child;
      if (!label || !GTK_IS_LABEL (label))
	return NULL;
      ltext = (gchar*) gtk_label_get_text (GTK_LABEL (label));
    }
  return ltext;
}

/**
 * glade_enum_from_string
 * @type: the GType for this enum type.
 * @string: the string representation of the enum value.
 *
 * This helper routine is designed to be used by widget build routines to
 * convert the string representations of enumeration values found in the
 * XML descriptions to the integer values that can be used to configure
 * the widget.
 *
 * Returns: the integer value for this enumeration, or 0 if it couldn't be
 * found.
 */
gint
glade_enum_from_string (GType type, const char *string)
{
    GEnumClass *eclass;
    GEnumValue *ev;
    gchar *endptr;
    gint ret = 0;

    ret = strtoul(string, &endptr, 0);
    if (endptr != string) /* parsed a number */
	return ret;

    eclass = g_type_class_ref(type);
    ev = g_enum_get_value_by_name(eclass, string);
    if (!ev) ev = g_enum_get_value_by_nick(eclass, string);
    if (ev)  ret = ev->value;

    g_type_class_unref(eclass);

    return ret;
}

/**
 * glade_string_from_enum
 * @type: the GType for this enum type.
 * @value: the value of the enum
 *
 * This helper routine is designed to be used by widget build routines to
 * convert the string representations of enumeration values found in the
 * XML descriptions to the integer values that can be used to configure
 * the widget.
 *
 * Returns: the string value for this enumeration
 */
const char *
glade_string_from_enum (GType type, gint value)
{
    GEnumClass *eclass;
    GEnumValue *ev;

    eclass = g_type_class_ref(type);

    ev = g_enum_get_value (eclass, value);

    g_type_class_unref(eclass);

    return ev ? ev->value_name : "0";
}


/**
 * glade_util_flags_from_string
 * @type: the GType for this flags type.
 * @string: the string representation of the flags value.
 *
 * This helper routine is designed to be used by widget build routines
 * to convert the string representations of flags values found in the
 * XML descriptions to the integer values that can be used to
 * configure the widget.  The string is composed of string names or
 * nicknames for various flags separated by '|'.
 *
 * Returns: the integer value for this flags string
 */
guint
glade_util_flags_from_string (GType type, const char *string)
{
    GFlagsClass *fclass;
    gchar *endptr, *prevptr;
    guint i, j, ret = 0;
    char *flagstr;

    ret = strtoul(string, &endptr, 0);
    if (endptr != string) /* parsed a number */
	return ret;

    fclass = g_type_class_ref(type);


    flagstr = g_strdup (string);
    for (ret = i = j = 0; ; i++) {
	gboolean eos;

	eos = flagstr [i] == '\0';
	
	if (eos || flagstr [i] == '|') {
	    GFlagsValue *fv;
	    const char  *flag;
	    gunichar ch;

	    flag = &flagstr [j];
            endptr = &flagstr [i];

	    if (!eos) {
		flagstr [i++] = '\0';
		j = i;
	    }

            /* trim spaces */
	    for (;;)
	      {
		ch = g_utf8_get_char (flag);
		if (!g_unichar_isspace (ch))
		  break;
		flag = g_utf8_next_char (flag);
	      }

	    while (endptr > flag)
	      {
		prevptr = g_utf8_prev_char (endptr);
		ch = g_utf8_get_char (prevptr);
		if (!g_unichar_isspace (ch))
		  break;
		endptr = prevptr;
	      }

	    if (endptr > flag)
	      {
		*endptr = '\0';
		fv = g_flags_get_value_by_name (fclass, flag);

		if (!fv)
		  fv = g_flags_get_value_by_nick (fclass, flag);

		if (fv)
		  ret |= fv->value;
		else
		  g_warning ("Unknown flag: '%s'", flag);
	      }

	    if (eos)
		break;
	}
    }
    
    g_free (flagstr);

    g_type_class_unref(fclass);

    return ret;
}

/**
 * glade_util_string_from_flags
 * @type: the GType for this flags type.
 * @flags: the value of the flags
 *
 * This helper routine is designed to be used by widget build routines
 * to convert the flags values into the string representations in the
 * XML descriptions.
 * The string is composed of string names or
 * nicknames for various flags separated by '|'.
 *
 * Returns: the string representation of these flags. Must be freed.
 */
gchar *
glade_util_string_from_flags (GType type, guint flags)
{
    GFlagsClass *flags_class;
    GString *string;
    char *ret;

    flags_class = g_type_class_ref (type);

    string = g_string_new ("");

    if (flags_class->n_values)
      {
	GFlagsValue *fval;
      
	for (fval = flags_class->values; fval->value_name; fval++)
	  {
	    /* We have to be careful as some flags include 0 values, e.g.
	       BonoboDockItemBehavior uses 0 for BONOBO_DOCK_ITEM_BEH_NORMAL.
	       If a 0 value is available, we only output it if the entire
	       flags value is 0, otherwise we check if the bit-flag is set. */
	    if ((fval->value == 0 && flags == 0)
		|| (fval->value && (fval->value & flags) == fval->value))
	      {
		if (string->len)
		  g_string_append_c (string, '|');
		g_string_append (string, fval->value_name);
	      }
	  }
      }

    ret = string->str;
    g_string_free (string, FALSE);

    g_type_class_unref (flags_class);

    return ret;
}


/* Returns TRUE if the widget should have a border width property.
   Containers normally do, but Bonobo controls don't.
   Also, GtkDialog widgets and their action area hbuttonboxes don't, as they
   use style properties. */
gboolean
glade_util_uses_border_width (GtkWidget *widget)
{
  char *child_name = gb_widget_get_child_name (widget);

#ifdef USE_GNOME
  if (BONOBO_IS_WIDGET (widget))
    return FALSE;
#endif

  /* The GtkDialog vbox has its border width set from a style property, so
     we shouldn't show the property in Glade. */
  if (GTK_IS_VBOX (widget) && widget->parent && GTK_IS_DIALOG (widget->parent))
    return FALSE;

  if (child_name && !strcmp (child_name, GladeChildDialogActionArea))
    return FALSE;

  if (GTK_IS_CONTAINER (widget))
    return TRUE;

  return FALSE;
}


/* Converts a filename from UTF-8 to on-disk encoding, and sets it in a
   GtkFileSelection. */
void
glade_util_set_file_selection_filename (GtkWidget *filesel,
					const gchar *filename_utf8)
{
  gchar *on_disk_filename;
  
  on_disk_filename = g_filename_from_utf8 (filename_utf8, -1, NULL, NULL,
					   NULL);

  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filesel),
				 on_disk_filename);
  g_free (on_disk_filename);
}


/* Gets the selected file in a GtkFileSelection, converts it to UTF-8 and
   returns it. Note that the returned string must be freed. */
gchar*
glade_util_get_file_selection_filename (GtkWidget *filesel)
{
  const gchar *on_disk_filename;
  gchar *filename;
  
  on_disk_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filesel));
  filename = g_filename_to_utf8 (on_disk_filename, -1, NULL, NULL, NULL);
  return filename;
}


/* Like fopen but takes a UTF-8 filename and converts to on-disk encoding. */
FILE*
glade_util_fopen (const gchar *filename_utf8, const gchar *mode)
{
  gchar *on_disk_filename;
  FILE *fp;

  on_disk_filename = g_filename_from_utf8 (filename_utf8, -1, NULL, NULL,
					   NULL);
  fp = fopen (on_disk_filename, mode);
  g_free (on_disk_filename);
  return fp;
}


/**
 * glade_util_uri_list_parse:
 * @uri_list: the text/urilist, must be NULL terminated.
 *
 * Extracts a list of file names from a standard text/uri-list,
 * such as one you would get on a drop operation.
 * This is mostly stolen from gnome-vfs-uri.c.
 *
 * Returns a GList of gchars.
 **/
GList *
glade_util_uri_list_parse (const gchar *uri_list)
{
  const gchar *p, *q;
  GList *result = NULL;

  g_return_val_if_fail (uri_list != NULL, NULL);

  p = uri_list;

  /* We don't actually try to validate the URI according to RFC
   * 2396, or even check for allowed characters - we just ignore
   * comments and trim whitespace off the ends.  We also
   * allow LF delimination as well as the specified CRLF.
   */
  while (p)
    {
      if (*p != '#')
        {
          gchar *retval;
          gchar *path;

          while (g_ascii_isspace (*p))
            p++;

          q = p;
          while ((*q != '\0') && (*q != '\n') && (*q != '\r'))
            q++;

          if (q > p)
            {
              q--;
              while (q > p && g_ascii_isspace (*q))
                q--;

              retval = g_new (gchar, q - p + 2);
              memcpy (retval, p, q - p + 1);
              retval[q - p + 1] = '\0';

              path = g_filename_from_uri (retval, NULL, NULL);
              if (!path)
                {
                  g_free (retval);
                  continue;
                }

              result = g_list_prepend (result, path);

              g_free (retval);
            }
        }
      p = strchr (p, '\n');
      if (p)
        p++;
    }

  return g_list_reverse (result);
}


void
glade_util_get_translation_properties (GtkWidget *widget,
				       const gchar *property_name,
				       gboolean *translatable,
				       gchar **comments,
				       gboolean *context)
{
  gchar buffer[1024];

  g_return_if_fail (GTK_IS_WIDGET (widget));

  sprintf (buffer, "%s:::comments", property_name);
  *comments = g_object_get_data (G_OBJECT (widget), buffer);

  /* This defaults to TRUE if no object data is set. */
  sprintf (buffer, "%s:::not_translatable", property_name);
  *translatable = g_object_get_data (G_OBJECT (widget), buffer) ? FALSE : TRUE;

  /* This defaults to FALSE if no object data is set. */
  sprintf (buffer, "%s:::context", property_name);
  *context = g_object_get_data (G_OBJECT (widget), buffer) ? TRUE : FALSE;
}


void
glade_util_set_translation_properties (GtkWidget *widget,
				       const gchar *property_name,
				       gboolean translatable,
				       const gchar *comments,
				       gboolean context)
{
  gchar buffer[1024];

  g_return_if_fail (GTK_IS_WIDGET (widget));

  sprintf (buffer, "%s:::comments", property_name);
  g_object_set_data_full (G_OBJECT (widget), buffer,
			  g_strdup (comments), comments ? g_free : NULL);

  sprintf (buffer, "%s:::not_translatable", property_name);
  g_object_set_data (G_OBJECT (widget), buffer, translatable ? NULL : "N");

  sprintf (buffer, "%s:::context", property_name);
  g_object_set_data (G_OBJECT (widget), buffer, context ? "Y" : NULL);
}


/* Copies the extra translation properties stored inside the widget data. */
void
glade_util_copy_translation_properties (GtkWidget	*from_widget,
					const gchar	*from_property_name,
					GtkWidget	*to_widget,
					const gchar	*to_property_name)
{
  gchar *comments;
  gboolean translatable, context;

  g_return_if_fail (GTK_IS_WIDGET (from_widget));
  g_return_if_fail (GTK_IS_WIDGET (to_widget));

  /* Copy the translator comments string, and translatable & context flags
     from one widget to the other. */
  glade_util_get_translation_properties (from_widget, from_property_name,
					 &translatable, &comments, &context);
  glade_util_set_translation_properties (to_widget, to_property_name,
					 translatable, comments, context);
}

