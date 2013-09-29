
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
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/entry.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Editable = "Entry|GtkEditable::editable";
static gchar *Visible = "GtkEntry::visibility";
static gchar *MaxLength = "GtkEntry::max_length";
static gchar *Text = "GtkEntry::text";

static gchar *HasFrame = "GtkEntry::has_frame";
static gchar *InvisibleChar = "GtkEntry::invisible_char";
static gchar *ActivatesDefault = "GtkEntry::activates_default";
static gchar *WidthChars = "GtkEntry::width_chars";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkEntry, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
/*
   GtkWidget*
   gb_entry_new(GbWidgetNewData *data)
   {

   }
 */



static void
on_toggle_width_chars (GtkWidget * widget, gpointer value)
{
  GtkWidget *property_widget;
  gboolean value_set;
  gint width = -1;

  property_widget = property_get_widget ();
  if (property_widget == NULL)
    return;

  value_set = GTK_TOGGLE_BUTTON (widget)->active ? TRUE : FALSE;
  gtk_widget_set_sensitive (GTK_WIDGET (value), value_set);
  if (value_set)
    {
      width = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (property_widget), WidthChars));
    }

  gtk_entry_set_width_chars (GTK_ENTRY (property_widget), width);
}

/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_entry_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (Editable, _("Editable:"), _("If the text can be edited"));
  property_add_bool (Visible, _("Text Visible:"),
		     _("If the text entered by the user will be shown. When turned off, the text typed in is displayed as asterix characters, which is useful for entering passwords"));
  property_add_int_range (MaxLength, _("Max Length:"),
			  _("The maximum length of the text"),
			  0, 10000, 1, 10, 1);
  property_add_string (Text, _("Text:"), _("The text to display"));

  property_add_bool (HasFrame, _("Has Frame:"), _("If the entry has a frame around it"));
  property_add_string (InvisibleChar, _("Invisible Char:"), _("The character to use if the text should not visible, e.g. when entering passwords"));
  property_add_bool (ActivatesDefault, _("Activates Default:"), _("If the default widget in the window is activated when Enter is pressed"));
  property_add_optional_int_range (WidthChars, _("Width In Chars:"), _("The number of characters to leave space for in the entry"),
				   0, 10000, 1, 10, 1,
				   on_toggle_width_chars);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_entry_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gchar buf[8];
  gint len, width;
  const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (widget));
  gb_widget_output_bool (data, Editable, GTK_ENTRY (widget)->editable);
  gb_widget_output_bool (data, Visible, GTK_ENTRY (widget)->visible);
  gb_widget_output_int (data, MaxLength, GTK_ENTRY (widget)->text_max_length);
  gb_widget_output_translatable_string (data, Text, entry_text);

  gb_widget_output_bool (data, HasFrame,
			 gtk_entry_get_has_frame (GTK_ENTRY (widget)));
  len = g_unichar_to_utf8 (gtk_entry_get_invisible_char (GTK_ENTRY (widget)),
			   buf);
  buf[len] = '\0';
  gb_widget_output_string (data, InvisibleChar, buf);
  gb_widget_output_bool (data, ActivatesDefault,
			 gtk_entry_get_activates_default (GTK_ENTRY (widget)));

  width = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						WidthChars));
  gb_widget_output_optional_int (data, WidthChars, width,
				 gtk_entry_get_width_chars (GTK_ENTRY (widget)) != -1);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_entry_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean editable, visible, has_frame, activates_default, is_set;
  gint max_length, width_chars;
  gchar *text, *invisible_char;

  editable = gb_widget_input_bool (data, Editable);
  if (data->apply)
    gtk_editable_set_editable (GTK_EDITABLE (widget), editable);

  visible = gb_widget_input_bool (data, Visible);
  if (data->apply)
    gtk_entry_set_visibility (GTK_ENTRY (widget), visible);

  max_length = gb_widget_input_int (data, MaxLength);
  if (data->apply)
    gtk_entry_set_max_length (GTK_ENTRY (widget), max_length);

  text = gb_widget_input_string (data, Text);
  if (data->apply)
    gtk_entry_set_text (GTK_ENTRY (widget), text);

  has_frame = gb_widget_input_bool (data, HasFrame);
  if (data->apply)
    gtk_entry_set_has_frame (GTK_ENTRY (widget), has_frame);

  invisible_char = gb_widget_input_string (data, InvisibleChar);
  if (data->apply)
    {
      gunichar c = g_utf8_get_char_validated (invisible_char, -1);
      if (c > 0)
	gtk_entry_set_invisible_char (GTK_ENTRY (widget), c);
    }

  activates_default = gb_widget_input_bool (data, ActivatesDefault);
  if (data->apply)
    gtk_entry_set_activates_default (GTK_ENTRY (widget), activates_default);

  width_chars = gb_widget_input_optional_int (data, WidthChars, &is_set);
  if (data->apply)
    {
      if (is_set)
	{
	  gtk_object_set_data (GTK_OBJECT (widget), WidthChars,
			       GINT_TO_POINTER (width_chars));
	  gtk_entry_set_width_chars (GTK_ENTRY (widget), width_chars);
	}
      else
	{
	  gtk_entry_set_width_chars (GTK_ENTRY (widget), -1);
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkEntry, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_entry_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_entry_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  guint16 max_len = GTK_ENTRY (widget)->text_max_length;
  const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (widget));
  gunichar c;
  gint width_chars;
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_entry_new ();\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  if (max_len)
    {
      source_add (data, "  gtk_entry_set_max_length (GTK_ENTRY (%s), %d);\n",
		  data->wname, max_len);
    }
  if (!GTK_ENTRY (widget)->editable)
    {
      source_add (data, "  gtk_editable_set_editable (GTK_EDITABLE (%s), FALSE);\n",
		  data->wname);
    }
  if (!GTK_ENTRY (widget)->visible)
    {
      source_add (data, "  gtk_entry_set_visibility (GTK_ENTRY (%s), FALSE);\n",
		  data->wname);
    }
  if (entry_text && strlen (entry_text) > 0)
    {
      glade_util_get_translation_properties (widget, Text, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data, "  gtk_entry_set_text (GTK_ENTRY (%s), %s);\n",
		  data->wname,
		  source_make_string_full (entry_text, data->use_gettext && translatable, context));
    }

  if (!gtk_entry_get_has_frame (GTK_ENTRY (widget)))
    {
      source_add (data, "  gtk_entry_set_has_frame (GTK_ENTRY (%s), FALSE);\n",
		  data->wname);
    }

  c = gtk_entry_get_invisible_char (GTK_ENTRY (widget));
  if (c != '*')
    {
      /* We just output the integer Unicode character code. I think that is
	 OK. */
      source_add (data,
		 "  gtk_entry_set_invisible_char (GTK_ENTRY (%s), %i);\n",
		  data->wname, c);
    }

  if (gtk_entry_get_activates_default (GTK_ENTRY (widget)))
    {
      source_add (data,
		 "  gtk_entry_set_activates_default (GTK_ENTRY (%s), TRUE);\n",
		  data->wname);
    }

  width_chars = gtk_entry_get_width_chars (GTK_ENTRY (widget));
  if (width_chars != -1)
    {
      source_add (data,
		 "  gtk_entry_set_width_chars (GTK_ENTRY (%s), %i);\n",
		  data->wname, width_chars);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_entry_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_entry_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = entry_xpm;
  gbwidget.tooltip = _("Text Entry");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties = gb_entry_create_properties;
  gbwidget.gb_widget_get_properties = gb_entry_get_properties;
  gbwidget.gb_widget_set_properties = gb_entry_set_properties;
/*
   gbwidget.gb_widget_new               = gb_entry_new;
   gbwidget.gb_widget_create_popup_menu = gb_entry_create_popup_menu;
 */
  gbwidget.gb_widget_write_source = gb_entry_write_source;

  return &gbwidget;
}
