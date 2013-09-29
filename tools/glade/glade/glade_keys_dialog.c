
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

/*
 * The Accelerator Keys dialog for selecting an accelerator key.
 */

#include <string.h>

#include <gtk/gtk.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkscrolledwindow.h>

#include "gladeconfig.h"

#ifdef USE_GNOME
#include <gnome.h>
#endif

#include "keys.h"
#include "glade_keys_dialog.h"
#include "gbwidget.h"

static void glade_keys_dialog_class_init   (GladeKeysDialogClass  *klass);
static void glade_keys_dialog_init         (GladeKeysDialog       *dialog);

static GtkDialogClass *parent_class = NULL;


GType
glade_keys_dialog_get_type (void)
{
  static GType glade_keys_dialog_type = 0;

  if (!glade_keys_dialog_type)
    {
      GtkTypeInfo glade_keys_dialog_info =
      {
	"GladeKeysDialog",
	sizeof (GladeKeysDialog),
	sizeof (GladeKeysDialogClass),
	(GtkClassInitFunc) glade_keys_dialog_class_init,
	(GtkObjectInitFunc) glade_keys_dialog_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      glade_keys_dialog_type = gtk_type_unique (gtk_dialog_get_type (),
						&glade_keys_dialog_info);
    }

  return glade_keys_dialog_type;
}

static void
glade_keys_dialog_class_init (GladeKeysDialogClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  parent_class = gtk_type_class (gtk_dialog_get_type ());
}


static void
glade_keys_dialog_init (GladeKeysDialog       *dialog)
{
  GtkWidget *scrolled_win;
  int i, row;
  gchar *titles[1];

  gtk_window_set_title (GTK_WINDOW (dialog), _("Select Accelerator Key"));
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "accelerator_key", "Glade");

  titles[0] = _("Keys");
  dialog->clist = gtk_clist_new_with_titles (1, titles);
  gtk_clist_column_titles_passive (GTK_CLIST (dialog->clist));
  gtk_widget_set_usize (dialog->clist, 200, 300);
  gtk_widget_show (dialog->clist);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), dialog->clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scrolled_win,
		      TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  /* Insert events & descriptions */
  gtk_clist_freeze (GTK_CLIST (dialog->clist));

  i = 0;
  while (GbKeys[i].name)
    {
      row = gtk_clist_append (GTK_CLIST (dialog->clist),
			      (gchar**) (&GbKeys[i].name));
      gtk_clist_set_row_data (GTK_CLIST (dialog->clist), row,
			      GINT_TO_POINTER (i));
      i++;
    }

  gtk_clist_thaw (GTK_CLIST (dialog->clist));

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}


GtkWidget*
glade_keys_dialog_new ()
{
  GladeKeysDialog *dialog;

  dialog = gtk_type_new (glade_keys_dialog_get_type ());

  return GTK_WIDGET (dialog);
}


/* This returns TRUE if a key is currently selected, and the given pointer
   is set to the key value. */
gboolean
glade_keys_dialog_get_key (GladeKeysDialog *dialog,
			   guint	   *key)
{
  GList *selection = GTK_CLIST (dialog->clist)->selection;
  gint row, index;

  if (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      index = GPOINTER_TO_INT (gtk_clist_get_row_data (GTK_CLIST (dialog->clist), row));
      *key = GbKeys[index].key;
      return TRUE;
    }
  return FALSE;
}


/* This returns the name of the key currently selected, or NULL if none is
   selected. */
gchar*
glade_keys_dialog_get_key_symbol (GladeKeysDialog *dialog)
{
  GList *selection = GTK_CLIST (dialog->clist)->selection;
  gint row, index;

  if (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      index = GPOINTER_TO_INT (gtk_clist_get_row_data (GTK_CLIST (dialog->clist), row));
      return GbKeys[index].name;
    }
  return NULL;
}


/* This sets the current key, using the given key value. */
void
glade_keys_dialog_set_key (GladeKeysDialog *dialog,
			   guint	    key)
{
  gint row, index;

  for (row = 0; row < GTK_CLIST (dialog->clist)->rows; row++)
    {
      index = GPOINTER_TO_INT (gtk_clist_get_row_data (GTK_CLIST (dialog->clist), row));
      if (GbKeys[index].key == key)
	{
	  gtk_clist_select_row (GTK_CLIST (dialog->clist), row, 0);
	  break;
	}
    }
}


/* This sets the current key, using the given key name. */
void
glade_keys_dialog_set_key_symbol (GladeKeysDialog *dialog,
				  const gchar     *key)
{
  gint row, index;

  for (row = 0; row < GTK_CLIST (dialog->clist)->rows; row++)
    {
      index = GPOINTER_TO_INT (gtk_clist_get_row_data (GTK_CLIST (dialog->clist), row));
      if (!strcmp (GbKeys[index].name, key))
	{
	  gtk_clist_select_row (GTK_CLIST (dialog->clist), row, 0);
	  break;
	}
    }
}


/* This finds the key value corresponding to the given string, which may be
   a symbol in gdkkeysyms.h, e.g. 'GDK_Tab', or a simple character, e.g. 'S'.
   Note that for GTK 1.0 this is a gchar, but for GTK 1.2 it is a guint. */
guint
glade_keys_dialog_find_key (const gchar *symbol)
{
  static GHashTable *keys_hash = NULL;
  guint key;
  gint i;

  /* If this is the first call, create the GHashTable. */
  if (keys_hash == NULL)
    {
      keys_hash = g_hash_table_new (g_str_hash, g_str_equal);
      for (i = 0; GbKeys[i].name; i++)
	{
	  if (GbKeys[i].key == 0)
	    g_warning ("GDK Key value is 0 - will not be found");
	  g_hash_table_insert (keys_hash, GbKeys[i].name,
			       GUINT_TO_POINTER (GbKeys[i].key));
	}
    }

  /* Lookup the key in the GHashTable. */
  key = GPOINTER_TO_UINT (g_hash_table_lookup (keys_hash, symbol));
  if (key)
    return key;

  /* If it's not found, return the first char. */
  return (guint) symbol[0];
}


