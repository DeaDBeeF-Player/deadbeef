
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

#include <string.h>
#include <libgnomedb/gnome-db-editor.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-db-editor.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Editable = "GnomeDbEditor::editable";
static gchar *Highlight = "GnomeDbEditor::highlight";
static gchar *Text = "GnomeDbEditor::text";

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeDbEditor, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */

GtkWidget*
gb_gnome_db_editor_new(GbWidgetNewData *data)
{
  return (GtkWidget *) gnome_db_editor_new();
}

/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_db_editor_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (Editable, _("Editable:"), _("If the text can be edited"));
  property_add_bool (Highlight, _("Highlight text:"), _("If selected, text will be highlighted inside the widget"));
  property_add_string (Text, _("Text:"), _("The text to display"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_db_editor_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gchar *wtext;
  wtext = (gchar *) gnome_db_editor_get_all_text(GNOME_DB_EDITOR(widget));
  gb_widget_output_translatable_string (data, Text, wtext);
  gb_widget_output_bool(data, Editable, gnome_db_editor_get_editable(GNOME_DB_EDITOR(widget)));
#ifdef HAVE_GTKSOURCEVIEW
  gb_widget_output_bool(data, Highlight, gnome_db_editor_get_highlight(GNOME_DB_EDITOR(widget)));
#else
#endif
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_db_editor_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean editable, highlight;
  gchar *text;

  editable = gb_widget_input_bool (data, Editable);
  if (data->apply)
    gnome_db_editor_set_editable (GNOME_DB_EDITOR (widget), editable);

#ifdef HAVE_GTKSOURCEVIEW
  highlight = gb_widget_input_bool (data, Highlight);
  if (data->apply)
    gnome_db_editor_set_highlight (GNOME_DB_EDITOR (widget), highlight);
#else
#endif

  text = gb_widget_input_string (data, Text);
  if (data->apply)
    gnome_db_editor_set_text (GNOME_DB_EDITOR (widget), text, -1);
}

/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_db_editor_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  const gchar *entry_text = (gchar *)gnome_db_editor_get_all_text (GNOME_DB_EDITOR (widget));
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      source_add (data, "  %s = gnome_db_editor_new ();\n", data->wname);
    }

  if (entry_text != NULL)
    {
      glade_util_get_translation_properties (widget, Text, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  gnome_db_editor_set_text (GNOME_DB_EDITOR (%s), %s, -1);\n",
		  data->wname,
		  source_make_string_full (entry_text,
					   data->use_gettext && translatable,
					   context));
    }

  if (!gnome_db_editor_get_editable (GNOME_DB_EDITOR(widget)))
    {
      source_add (data, "  gnome_db_editor_set_editable (GNOME_DB_EDITOR (%s), FALSE);\n",
		  data->wname);
    }
  if (!gnome_db_editor_get_highlight (GNOME_DB_EDITOR(widget)))
    {
      source_add (data, "  gnome_db_editor_set_highlight (GNOME_DB_EDITOR (%s), FALSE);\n",
		  data->wname);
    }
  gb_widget_write_standard_source (widget, data);

}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_gnome_db_editor_init ()
{
  /* Initialise the GTK type */
  volatile GType type;
  type = gnome_db_editor_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_db_editor_xpm;
  gbwidget.tooltip = _("GnomeDbEditor");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties = gb_gnome_db_editor_create_properties;
  gbwidget.gb_widget_get_properties = gb_gnome_db_editor_get_properties;
  gbwidget.gb_widget_set_properties = gb_gnome_db_editor_set_properties;
  gbwidget.gb_widget_new               = gb_gnome_db_editor_new;
  gbwidget.gb_widget_write_source = gb_gnome_db_editor_write_source;

  return &gbwidget;
}
