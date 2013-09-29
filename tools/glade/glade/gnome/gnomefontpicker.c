/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999  Damon Chaplin
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

#include <config.h>

#include <gnome.h>
#include "../gb.h"
#include "../glade_gnome.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-fontpicker.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/* Copied from gnome-font-picker.c */
#define DEF_PREVIEW_TEXT "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz"

static gchar *Title = "GnomeFontPicker::title";
static gchar *PreviewText = "GnomeFontPicker::preview_text";
static gchar *Mode = "GnomeFontPicker::mode";
static gchar *ShowSize = "GnomeFontPicker::show_size";
static gchar *UseFont = "GnomeFontPicker::use_font_in_label";
static gchar *UseFontSize = "GnomeFontPicker::label_font_size";
static gchar *FocusOnClick = "GnomeFontPicker|GtkButton::focus_on_click";

static const gchar *GbModeChoices[] =
{
  "Pixmap",
  "Font Information",
  NULL
};
static const gint GbModeValues[] =
{
  GNOME_FONT_PICKER_MODE_PIXMAP,
  GNOME_FONT_PICKER_MODE_FONT_INFO
};
static const gchar *GbModeSymbols[] =
{
  "GNOME_FONT_PICKER_MODE_PIXMAP",
  "GNOME_FONT_PICKER_MODE_FONT_INFO"
};


static void gb_gnome_font_picker_set_property_states (GtkWidget *widget);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeFontPicker, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_gnome_font_picker_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gnome_font_picker_new ();

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_font_picker_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_string (Title, _("Title:"),
		       _("The title of the font selection dialog"));
  property_add_string (PreviewText, _("Preview Text:"),
		       _("The preview text to show in the font selection dialog"));
  property_add_choice (Mode, _("Mode:"),
		       _("What to display in the font picker button"),
		       GbModeChoices);
  property_add_bool (ShowSize, _("Show Size:"),
		     _("If the font size is shown as part of the font information"));
  property_add_bool (UseFont, _("Use Font:"),
		     _("If the selected font is used when displaying the font information"));
  property_add_int_range (UseFontSize, _("Use Size:"),
			  _("The size of the font to use in the font picker button"),
			  2, 1000, 1, 10, 1);
  property_add_bool (FocusOnClick, _("Focus On Click:"), _("If the button grabs focus when it is clicked"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_font_picker_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gint i, show_size, use_font_in_label, use_font_in_label_size;
  const gchar *title, *preview_text;

  /* Only save if the title is different to the default. */
  title = gnome_font_picker_get_title (GNOME_FONT_PICKER (widget));
  if (data->action == GB_SHOWING
      || (title && strcmp (title, dgettext (GLADE_LIBGNOMEUI_GETTEXT_PACKAGE,
					    "Pick a Font"))))
    gb_widget_output_translatable_string (data, Title, title);

  /* Only save if the preview text is different to the default. */
  preview_text = gnome_font_picker_get_preview_text (GNOME_FONT_PICKER (widget));
  if (data->action == GB_SHOWING
      || (preview_text && strcmp (dgettext (GLADE_LIBGNOMEUI_GETTEXT_PACKAGE,
					    DEF_PREVIEW_TEXT), preview_text)))
      gb_widget_output_translatable_string (data, PreviewText, preview_text);

  for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
    {
      if (GbModeValues[i] == gnome_font_picker_get_mode (GNOME_FONT_PICKER (widget)))
	gb_widget_output_choice (data, Mode, i, GbModeSymbols[i]);
    }

  g_object_get (G_OBJECT (widget),
		"show-size", &show_size,
		"use-font-in-label", &use_font_in_label,
		"label-font-size", &use_font_in_label_size,
		NULL);

  gb_widget_output_bool (data, ShowSize, show_size);
  gb_widget_output_bool (data, UseFont, use_font_in_label);
  gb_widget_output_int (data, UseFontSize, use_font_in_label_size);

  if (data->action == GB_SHOWING)
    gb_gnome_font_picker_set_property_states (widget);

  gb_widget_output_bool (data, FocusOnClick,
			 gtk_button_get_focus_on_click (GTK_BUTTON (widget)));
}


static void
gb_gnome_font_picker_set_property_states (GtkWidget *widget)
{
  gboolean use_font_sens = FALSE, use_font_size_sens = FALSE;

  if (gnome_font_picker_get_mode (GNOME_FONT_PICKER (widget)) == GNOME_FONT_PICKER_MODE_FONT_INFO)
    {
      use_font_sens = TRUE;
      g_object_get (G_OBJECT (widget),
		    "use-font-in-label", &use_font_size_sens,
		    NULL);
    }
  property_set_sensitive (ShowSize, use_font_sens);
  property_set_sensitive (UseFont, use_font_sens);
  property_set_sensitive (UseFontSize, use_font_size_sens);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_font_picker_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *title, *preview_text, *mode;
  gint i, use_font_size, new_use_font_size;
  gboolean show_size, use_font, focus_on_click;
  gboolean new_use_font, set_use_font = FALSE;
  gboolean set_property_states = FALSE;

  title = gb_widget_input_string (data, Title);
  if (data->apply)
    gnome_font_picker_set_title (GNOME_FONT_PICKER (widget),
				 title && title[0]
				 ? title : dgettext (GLADE_LIBGNOMEUI_GETTEXT_PACKAGE,
						     "Pick a Font"));

  preview_text = gb_widget_input_string (data, PreviewText);
  if (data->apply)
    gnome_font_picker_set_preview_text (GNOME_FONT_PICKER (widget),
					preview_text && preview_text[0]
					? preview_text : DEF_PREVIEW_TEXT);

  mode = gb_widget_input_choice (data, Mode);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (!strcmp (mode, GbModeChoices[i])
	      || !strcmp (mode, GbModeSymbols[i]))
	    {
	      gnome_font_picker_set_mode (GNOME_FONT_PICKER (widget),
					  GbModeValues[i]);
	      set_property_states = TRUE;
	      break;
	    }
	}
    }

  show_size = gb_widget_input_bool (data, ShowSize);
  if (data->apply)
    gnome_font_picker_fi_set_show_size (GNOME_FONT_PICKER (widget),
					show_size);

  g_object_get (G_OBJECT (widget),
		"use-font-in-label", &new_use_font,
		"label-font-size", &new_use_font_size,
		NULL);

  use_font = gb_widget_input_bool (data, UseFont);
  if (data->apply)
    {
      new_use_font = use_font;
      set_use_font = TRUE;
    }

  use_font_size = gb_widget_input_int (data, UseFontSize);
  if (data->apply)
    {
      new_use_font_size = use_font_size;
      set_use_font = TRUE;
    }

  if (set_use_font)
    {
      gnome_font_picker_fi_set_use_font_in_label (GNOME_FONT_PICKER (widget),
						  new_use_font,
						  new_use_font_size);
      set_property_states = TRUE;
    }

  if ((data->action == GB_APPLYING) && set_property_states)
    gb_gnome_font_picker_set_property_states (widget);

  focus_on_click = gb_widget_input_bool (data, FocusOnClick);
  if (data->apply)
    gtk_button_set_focus_on_click (GTK_BUTTON (widget), focus_on_click);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeFontPicker, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_font_picker_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_font_picker_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  const gchar *title, *preview_text;
  gint i;
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    source_add (data, "  %s = gnome_font_picker_new ();\n", data->wname);

  gb_widget_write_standard_source (widget, data);

  title = gnome_font_picker_get_title (GNOME_FONT_PICKER (widget));
  if (title && title[0]
      && strcmp (title, dgettext (GLADE_LIBGNOMEUI_GETTEXT_PACKAGE,
				  "Pick a Font")))
    {
      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  gnome_font_picker_set_title (GNOME_FONT_PICKER (%s), %s);\n",
		  data->wname, source_make_string_full (title, data->use_gettext && translatable, context));
    }

  preview_text = gnome_font_picker_get_preview_text (GNOME_FONT_PICKER (widget));
  if (preview_text && preview_text[0]
      && strcmp (dgettext (GLADE_LIBGNOMEUI_GETTEXT_PACKAGE, DEF_PREVIEW_TEXT),
		 preview_text))
    {
      glade_util_get_translation_properties (widget, PreviewText,
					     &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  gnome_font_picker_set_preview_text (GNOME_FONT_PICKER (%s), %s);\n",
		  data->wname, source_make_string_full (preview_text,
							data->use_gettext && translatable, context));
    }

  if (gnome_font_picker_get_mode (GNOME_FONT_PICKER (widget)) != GNOME_FONT_PICKER_MODE_PIXMAP)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (GbModeValues[i] == gnome_font_picker_get_mode (GNOME_FONT_PICKER (widget)))
	    source_add (data,
			"  gnome_font_picker_set_mode (GNOME_FONT_PICKER (%s),\n"
			"                              %s);\n",
			data->wname, GbModeSymbols[i]);
	}
    }

  if (gnome_font_picker_get_mode (GNOME_FONT_PICKER (widget)) == GNOME_FONT_PICKER_MODE_FONT_INFO)
    {
      int show_size, use_font_in_label, label_font_size;
      g_object_get (G_OBJECT (widget),
		    "show-size", &show_size,
		    "use-font-in-label", &use_font_in_label,
		    "label-font-size", &label_font_size,
		    NULL);
      if (!show_size)
	{
	  source_add (data,
		      "  gnome_font_picker_fi_set_show_size (GNOME_FONT_PICKER (%s), FALSE);\n",
		      data->wname);
	}

      if (use_font_in_label)
	{
	  source_add (data,
		      "  gnome_font_picker_fi_set_use_font_in_label (GNOME_FONT_PICKER (%s),\n"
		      "                                              TRUE, %i);\n",
		      data->wname,
		      label_font_size);
	}
    }

  if (!gtk_button_get_focus_on_click (GTK_BUTTON (widget)))
    {
      source_add (data,
		  "  gtk_button_set_focus_on_click (GTK_BUTTON (%s), FALSE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_font_picker_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_font_picker_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_fontpicker_xpm;
  gbwidget.tooltip = _("Gnome Font Picker");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_font_picker_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_font_picker_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_font_picker_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_font_picker_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_font_picker_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_font_picker_create_popup_menu;
*/

  return &gbwidget;
}

