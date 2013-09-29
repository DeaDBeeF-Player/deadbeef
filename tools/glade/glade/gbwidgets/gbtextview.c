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

#include <gtk/gtk.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/text.xpm"

static gchar *Editable         = "GtkTextView::editable";
static gchar *Text             = "GtkTextView::text";
static gchar *Justification    = "GtkTextView::justification";
static gchar *WrapMode         = "GtkTextView::wrap_mode";
static gchar *CursorVisible    = "GtkTextView::cursor_visible";
static gchar *PixelsAboveLines = "GtkTextView::pixels_above_lines";
static gchar *PixelsBelowLines = "GtkTextView::pixels_below_lines";
static gchar *PixelsInsideWrap = "GtkTextView::pixels_inside_wrap";
static gchar *LeftMargin       = "GtkTextView::left_margin";
static gchar *RightMargin      = "GtkTextView::right_margin";
static gchar *Indent           = "GtkTextView::indent";

static gchar *Overwrite        = "GtkTextView::overwrite";
static gchar *AcceptsTab       = "GtkTextView::accepts_tab";

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static const char *GbJustifyChoices[] = {
	N_("Left"),
	N_("Right"),
	N_("Center"),
	N_("Fill"),
	NULL
};

static gint GbJustifyValues[] = {
	GTK_JUSTIFY_LEFT,
	GTK_JUSTIFY_RIGHT,
	GTK_JUSTIFY_CENTER,
	GTK_JUSTIFY_FILL
};
static const gchar *GbJustifySymbols[] =
{
  "GTK_JUSTIFY_LEFT",
  "GTK_JUSTIFY_RIGHT",
  "GTK_JUSTIFY_CENTER",
  "GTK_JUSTIFY_FILL"
};

static const char *GbWrapChoices[] = {
	N_("None"),
	N_("Character"),
	N_("Word"),
	NULL
};

static gint GbWrapValues[] = {
	GTK_WRAP_NONE,
	GTK_WRAP_CHAR,
	GTK_WRAP_WORD
};
static const char *GbWrapSymbols[] = {
	"GTK_WRAP_NONE",
	"GTK_WRAP_CHAR",
	"GTK_WRAP_WORD"
};

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkTextView, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
/*
static GtkWidget*
gb_text_view_new (GbWidgetNewData *data)
{
	return gtk_text_view_new ();
}
*/



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_text_view_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
	property_add_bool (Editable, _("Editable:"),
			   _("If the text can be edited"));
	property_add_bool (CursorVisible, _("Cursor Visible:"),
			   _("If the cursor is visible"));
	property_add_bool (Overwrite, _("Overwrite:"),
			   _("If entered text overwrites the existing text"));
	property_add_bool (AcceptsTab, _("Accepts Tab:"),
			   _("If tab characters can be entered"));

	property_add_text (Text, _("Text:"), _("The text to display"), 5);
	
	property_add_choice (Justification, _("Justification:"),
			     _("The justification of the text"),
			     GbJustifyChoices);
	property_add_choice (WrapMode, _("Wrapping:"),
			     _("The wrapping of the text"),
			     GbWrapChoices);

	property_add_int_range (PixelsAboveLines, _("Space Above:"),
				_("Pixels of blank space above paragraphs"),
				0, G_MAXINT, 1, 2, 10);
	property_add_int_range (PixelsBelowLines, _("Space Below:"),
				_("Pixels of blank space below paragraphs"),
				0, G_MAXINT, 1, 2, 10);
	property_add_int_range (PixelsInsideWrap, _("Space Inside:"),
				_("Pixels of blank space between wrapped lines in a paragraph"),
				0, G_MAXINT, 1, 2, 10);

	property_add_int_range (LeftMargin, _("Left Margin:"),
				_("Width of the left margin in pixels"),
				0, G_MAXINT, 1, 2, 10);
	property_add_int_range (RightMargin, _("Right Margin:"),
				_("Width of the right margin in pixels"),
				0, G_MAXINT, 1, 2, 10);
	property_add_int_range (Indent, _("Indent:"),
				_("Amount of pixels to indent paragraphs"),
				0, G_MAXINT, 1, 2, 10);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_text_view_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
	GtkTextBuffer *buffy;
	GtkTextIter start, end;
	gboolean editable, cursor_visible, overwrite, accepts_tab;
	gint wrap_mode, justification;
	gint pixels_above, pixels_below, pixels_inside;
	gint left_margin, right_margin, indent;
	char *text;
	
	g_object_get (widget,
		      "editable",           &editable,
		      "overwrite",	    &overwrite,
		      "accepts_tab",	    &accepts_tab,
		      "justification",      &justification,
		      "wrap-mode",          &wrap_mode,
		      "cursor-visible",     &cursor_visible,
		      "pixels-above-lines", &pixels_above,
		      "pixels-below-lines", &pixels_below,
		      "pixels-inside-wrap", &pixels_inside,
		      "left-margin",        &left_margin,
		      "right-margin",       &right_margin,
		      "indent",             &indent,
		      NULL);


	gb_widget_output_bool (data, Editable, editable);
	gb_widget_output_bool (data, Overwrite, overwrite);
	gb_widget_output_bool (data, AcceptsTab, accepts_tab);
	gb_widget_output_enum (data, GTK_TYPE_JUSTIFICATION,
			       GbJustifyValues,
			       G_N_ELEMENTS (GbJustifyValues),
			       Justification, justification);
	gb_widget_output_enum (data, GTK_TYPE_WRAP_MODE,
			       GbWrapValues,
			       G_N_ELEMENTS (GbWrapValues),
			       WrapMode, wrap_mode);
	gb_widget_output_bool (data, CursorVisible, cursor_visible);
	gb_widget_output_int (data, PixelsAboveLines, pixels_above);
	gb_widget_output_int (data, PixelsBelowLines, pixels_below);
	gb_widget_output_int (data, PixelsInsideWrap, pixels_inside);
	gb_widget_output_int (data, LeftMargin, left_margin);
	gb_widget_output_int (data, RightMargin, right_margin);
	gb_widget_output_int (data, Indent, indent);

	buffy = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	gtk_text_buffer_get_bounds (buffy, &start, &end);
	text = gtk_text_iter_get_text (&start, &end);
	gb_widget_output_translatable_text (data, Text, text);
	g_free (text);
}

/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_text_view_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
	int i;
	char *s;

	i = gb_widget_input_bool (data, Editable);
	if (data->apply)
		g_object_set (widget, "editable", i, NULL);

	i = gb_widget_input_bool (data, Overwrite);
	if (data->apply)
		g_object_set (widget, "overwrite", i, NULL);

	i = gb_widget_input_bool (data, AcceptsTab);
	if (data->apply)
		g_object_set (widget, "accepts_tab", i, NULL);

	s = gb_widget_input_text (data, Text);
	if (data->apply) {
		GtkTextBuffer *buffy;
		buffy = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
		gtk_text_buffer_set_text (buffy, s, strlen (s));
	}
	if (data->action == GB_APPLYING)
		g_free (s);


	i = gb_widget_input_enum (data, GTK_TYPE_JUSTIFICATION,
				  GbJustifyChoices, GbJustifyValues,
				  Justification);
	if (data->apply)
		g_object_set (widget, "justification", i, NULL);

	i = gb_widget_input_enum (data, GTK_TYPE_WRAP_MODE,
				  GbWrapChoices, GbWrapValues,
				  WrapMode);
	if (data->apply)
		g_object_set (widget, "wrap-mode", i, NULL);

	i = gb_widget_input_bool (data, CursorVisible);
	if (data->apply)
		g_object_set (widget, "cursor-visible", i, NULL);

	i = gb_widget_input_int (data, PixelsAboveLines);
	if (data->apply)
		g_object_set (widget, "pixels-above-lines", i, NULL);

	i = gb_widget_input_int (data, PixelsBelowLines);
	if (data->apply)
		g_object_set (widget, "pixels-below-lines", i, NULL);

	i = gb_widget_input_int (data, PixelsInsideWrap);
	if (data->apply)
		g_object_set (widget, "pixels-inside-wrap", i, NULL);

	i = gb_widget_input_int (data, LeftMargin);
	if (data->apply)
		g_object_set (widget, "left-margin", i, NULL);

	i = gb_widget_input_int (data, RightMargin);
	if (data->apply)
		g_object_set (widget, "right-margin", i, NULL);

	i = gb_widget_input_int (data, Indent);
	if (data->apply)
		g_object_set (widget, "indent", i, NULL);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkTextView, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_text_view_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_text_view_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkTextBuffer *buffy;
  GtkTextIter start, end;
  gboolean editable, cursor_visible, overwrite, accepts_tab;
  gint wrap_mode, justification;
  gint pixels_above, pixels_below, pixels_inside;
  gint left_margin, right_margin, indent, i;
  char *text;
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_text_view_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  g_object_get (widget,
		"editable",           &editable,
		"overwrite",	      &overwrite,
		"accepts_tab",	      &accepts_tab,
		"justification",      &justification,
		"wrap-mode",          &wrap_mode,
		"cursor-visible",     &cursor_visible,
		"pixels-above-lines", &pixels_above,
		"pixels-below-lines", &pixels_below,
		"pixels-inside-wrap", &pixels_inside,
		"left-margin",        &left_margin,
		"right-margin",       &right_margin,
		"indent",             &indent,
		NULL);

  if (!editable)
    source_add (data,
		"  gtk_text_view_set_editable (GTK_TEXT_VIEW (%s), FALSE);\n",
		data->wname);

  if (overwrite)
    source_add (data,
		"  gtk_text_view_set_overwrite (GTK_TEXT_VIEW (%s), TRUE);\n",
		data->wname);

  if (!accepts_tab)
    source_add (data,
		"  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (%s), FALSE);\n",
		data->wname);

  if (justification != GTK_JUSTIFY_LEFT)
    {
      for (i = 0; i < sizeof (GbJustifyValues) / sizeof (GbJustifyValues[0]);
	   i++)
	{
	  if (GbJustifyValues[i] == justification)
	    source_add (data,
	       "  gtk_text_view_set_justification (GTK_TEXT_VIEW (%s), %s);\n",
			data->wname, GbJustifySymbols[i]);
	}
    }

  if (wrap_mode != GTK_WRAP_NONE)
    {
      for (i = 0; i < sizeof (GbWrapValues) / sizeof (GbWrapValues[0]); i++)
	{
	  if (GbWrapValues[i] == wrap_mode)
	    source_add (data,
	       "  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (%s), %s);\n",
			data->wname, GbWrapSymbols[i]);
	}
    }

  if (!cursor_visible)
    source_add (data,
	"  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (%s), FALSE);\n",
		data->wname);

  if (pixels_above != 0)
    source_add (data,
	"  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (%s), %i);\n",
		data->wname, pixels_above);

  if (pixels_below != 0)
    source_add (data,
	"  gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (%s), %i);\n",
		data->wname, pixels_below);

  if (pixels_inside != 0)
    source_add (data,
	"  gtk_text_view_set_pixels_inside_wrap (GTK_TEXT_VIEW (%s), %i);\n",
		data->wname, pixels_inside);

  if (left_margin != 0)
    source_add (data,
		"  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (%s), %i);\n",
		data->wname, left_margin);

  if (right_margin != 0)
    source_add (data,
		"  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (%s), %i);\n",
		data->wname, right_margin);

  if (indent != 0)
    source_add (data,
		"  gtk_text_view_set_indent (GTK_TEXT_VIEW (%s), %i);\n",
		data->wname, indent);


  buffy = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_get_bounds (buffy, &start, &end);
  text = gtk_text_iter_get_text (&start, &end);
  if (text && *text)
    {
      glade_util_get_translation_properties (widget, Text, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);
      source_add (data,
		  "  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (%s)), %s, -1);\n",
		  data->wname,
		  source_make_string_full (text,
					   data->use_gettext && translatable,
					   context));
    }
  g_free (text);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_text_view_init ()
{
  GtkWidgetClass *klass;

  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_text_view_get_type();

  /* Add a signal emission hook so we can connect signal handlers to the
     scrollbar adjustments to redraw the clist when necessary. This will also
     work for subclasses of GtkCList. */
  klass = gtk_type_class (gtk_text_view_get_type ());
  g_signal_add_emission_hook (klass->set_scroll_adjustments_signal, 0,
			      gb_set_scroll_adjustments_hook, NULL, NULL);

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = text_xpm;
  gbwidget.tooltip = _("Text View");

  /* Fill in any functions that this GbWidget has */
/*
  gbwidget.gb_widget_new		= gb_text_view_new;
*/
  gbwidget.gb_widget_create_properties	= gb_text_view_create_properties;
  gbwidget.gb_widget_get_properties	= gb_text_view_get_properties;
  gbwidget.gb_widget_set_properties	= gb_text_view_set_properties;
  gbwidget.gb_widget_write_source	= gb_text_view_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_text_view_create_popup_menu;
*/

  return &gbwidget;
}

