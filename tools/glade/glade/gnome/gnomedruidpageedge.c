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

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-druid-page-start.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *BackgroundColor = "GnomeDruidPageEdge::background_color";
static gchar *LogoBackgroundColor = "GnomeDruidPageEdge::logo_background_color";
static gchar *TextboxColor = "GnomeDruidPageEdge::textbox_color";
static gchar *TextColor = "GnomeDruidPageEdge::text_color";
static gchar *TitleColor = "GnomeDruidPageEdge::title_color";
static gchar *Text = "GnomeDruidPageEdge::text";
static gchar *Title = "GnomeDruidPageEdge::title";
static gchar *LogoImage = "GnomeDruidPageEdge::logo";
static gchar *Watermark = "GnomeDruidPageEdge::watermark";
static gchar *TopWatermark = "GnomeDruidPageEdge::top_watermark";
static gchar *Position = "GnomeDruidPageEdge::position";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeDruidPageEdge, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_gnome_druid_page_edge_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;
  GnomeEdgePosition position = GNOME_EDGE_START;

  if (data->action == GB_LOADING)
    {
      char *position_string = load_string (data->loading_data, Position);
      if (position_string && *position_string)
	{
	  position = glade_enum_from_string (GNOME_TYPE_EDGE_POSITION,
					     position_string);
	}
      else
	{
	  g_warning ("no position property in XML file. Defaulting to GNOME_EDGE_START");
	}
    }
  else
    {
      g_warning ("gb_gnome_druid_page_edge_new() called to create widget. Defaulting to GNOME_EDGE_START");
    }

  new_widget = gnome_druid_page_edge_new (position);

  return new_widget;
}

/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_druid_page_edge_create_properties (GtkWidget * widget,
					     GbWidgetCreateArgData * data)
{
  property_add_string (Title, _("Title:"),
		       _("The title of the page"));
  property_add_text (Text, _("Text:"),
		     _("The main text of the page, "
		       "introducing people to the druid."), 5);
  property_add_color (TitleColor, _("Title Color:"),
		      _("The color of the title text"));
  property_add_color (TextColor, _("Text Color:"),
		      _("The color of the main text"));
  property_add_color (BackgroundColor, _("Back. Color:"),
		      _("The background color of the page"));
  property_add_color (LogoBackgroundColor, _("Logo Back. Color:"),
		      _("The background color around the logo"));
  property_add_color (TextboxColor, _("Text Box Color:"),
		      _("The background color of the main text area"));
  property_add_filename (LogoImage, _("Logo Image:"),
			 _("The logo to display in the top-right of the page"));
  property_add_filename (Watermark, _("Side Watermark:"),
			 _("The main image to display on the side of the page."));
  property_add_filename (TopWatermark, _("Top Watermark:"),
			 _("The watermark to display at the top of the page."));
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_druid_page_edge_get_properties (GtkWidget *widget,
					 GbWidgetGetArgData * data)
{
  GnomeDruidPageEdge *page;
	
  page = GNOME_DRUID_PAGE_EDGE (widget);

  if (data->action == GB_SAVING)
    {
      save_string (data, Position,
		   glade_string_from_enum (GNOME_TYPE_EDGE_POSITION,
					   page->position));
    }

  gb_widget_output_translatable_string (data, Title, gtk_object_get_data (GTK_OBJECT (widget), Title));
  gb_widget_output_translatable_text (data, Text, page->text);

  /* We do this to make sure the colors are set. */
  gtk_widget_ensure_style (widget);

  /* Only save colors if they have been set explicitly. */
  if (data->action == GB_SHOWING
      || gtk_object_get_data (GTK_OBJECT (widget), TitleColor))
    gb_widget_output_color (data, TitleColor, &page->title_color);

  if (data->action == GB_SHOWING
      || gtk_object_get_data (GTK_OBJECT (widget), TextColor))
    gb_widget_output_color (data, TextColor, &page->text_color);

  if (data->action == GB_SHOWING
      || gtk_object_get_data (GTK_OBJECT (widget), BackgroundColor))
    gb_widget_output_color (data, BackgroundColor, &page->background_color);

  if (data->action == GB_SHOWING
      || gtk_object_get_data (GTK_OBJECT (widget), LogoBackgroundColor))
    gb_widget_output_color (data, LogoBackgroundColor,
			    &page->logo_background_color);

  if (data->action == GB_SHOWING
      || gtk_object_get_data (GTK_OBJECT (widget), TextboxColor))
    gb_widget_output_color (data, TextboxColor, &page->textbox_color);

  gb_widget_output_pixmap_filename (data, LogoImage,
				    gtk_object_get_data (GTK_OBJECT (widget),
							 LogoImage));
  gb_widget_output_pixmap_filename (data, Watermark,
				    gtk_object_get_data (GTK_OBJECT (widget),
							 Watermark));

  gb_widget_output_pixmap_filename (data, TopWatermark,
				    gtk_object_get_data (GTK_OBJECT (widget),
							 TopWatermark));
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */

static void
gb_gnome_druid_page_edge_set_properties (GtkWidget * widget,
					  GbWidgetSetArgData * data)
{
  GnomeDruidPageEdge *page;
  gchar *string, *old_filename;
  GdkColor *color;
  GdkPixbuf *pb;
	
  page = GNOME_DRUID_PAGE_EDGE (widget);

  string = gb_widget_input_string (data, Title);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), Title, g_strdup (string));
      gnome_druid_page_edge_set_title (page, string);
    }

  string = gb_widget_input_text (data, Text);
  if (data->apply)
    gnome_druid_page_edge_set_text (page, string);
  if (data->action == GB_APPLYING)
    g_free (string);

  /* For colors, we store a flag to indicate if the color has been set.
     Unfortunately the only way to reset a color to the default at present is
     to remove it from the XML file. */
  color = gb_widget_input_color (data, BackgroundColor);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), BackgroundColor, "Y");
      gnome_druid_page_edge_set_bg_color (page, color);
    }

  color = gb_widget_input_color (data, LogoBackgroundColor);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), LogoBackgroundColor, "Y");
      gnome_druid_page_edge_set_logo_bg_color (page, color);
    }

  color = gb_widget_input_color (data, TextboxColor);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), TextboxColor, "Y");
      gnome_druid_page_edge_set_textbox_color (page, color);
    }

  color = gb_widget_input_color (data, TextColor);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), TextColor, "Y");
      gnome_druid_page_edge_set_text_color (page, color);
    }

  color = gb_widget_input_color (data, TitleColor);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), TitleColor, "Y");
      gnome_druid_page_edge_set_title_color (page, color);
    }

  string = gb_widget_input_pixmap_filename (data, LogoImage);
  if (data->apply)
    {
      if (string && string[0] == '\0')
	string = NULL;
      old_filename = gtk_object_get_data (GTK_OBJECT (widget), LogoImage);
      glade_project_remove_pixmap (data->project, old_filename);
      gtk_object_set_data_full (GTK_OBJECT (widget), LogoImage,
				g_strdup (string), string ? g_free : NULL);
      glade_project_add_pixmap (data->project, string);
      pb = string ? gdk_pixbuf_new_from_file (string, NULL) : NULL;
      gnome_druid_page_edge_set_logo (page, pb);
      if (pb)
	gdk_pixbuf_unref (pb);
    }
  if (data->action == GB_LOADING)
    g_free (string);

  string = gb_widget_input_pixmap_filename (data, Watermark);
  if (data->apply)
    {
      if (string && string[0] == '\0')
	string = NULL;
      old_filename = gtk_object_get_data (GTK_OBJECT (widget), Watermark);
      glade_project_remove_pixmap (data->project, old_filename);
      gtk_object_set_data_full (GTK_OBJECT (widget), Watermark,
				g_strdup (string), string ? g_free : NULL);
      glade_project_add_pixmap (data->project, string);
      pb = string ? gdk_pixbuf_new_from_file (string, NULL) : NULL;
      gnome_druid_page_edge_set_watermark (page, pb);
      if (pb)
	gdk_pixbuf_unref (pb);
    }
  if (data->action == GB_LOADING)
    g_free (string);

  string = gb_widget_input_pixmap_filename (data, TopWatermark);
  if (data->apply)
    {
      if (string && string[0] == '\0')
	string = NULL;
      old_filename = gtk_object_get_data (GTK_OBJECT (widget), TopWatermark);
      glade_project_remove_pixmap (data->project, old_filename);
      gtk_object_set_data_full (GTK_OBJECT (widget), TopWatermark,
				g_strdup (string), string ? g_free : NULL);
      glade_project_add_pixmap (data->project, string);
      pb = string ? gdk_pixbuf_new_from_file (string, NULL) : NULL;
      gnome_druid_page_edge_set_top_watermark (page, pb);
      if (pb)
	gdk_pixbuf_unref (pb);
    }
  if (data->action == GB_LOADING)
    g_free (string);
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeDruidPageEdge, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_druid_page_edge_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_druid_page_edge_write_source (GtkWidget * widget,
					GbWidgetWriteSourceData * data)
{
  GnomeDruidPageEdge *page;
  GdkColor *color;
  gchar *filename, *title;
  gboolean translatable, context;
  gchar *comments;

  g_return_if_fail (GNOME_IS_DRUID (widget->parent));
  page = GNOME_DRUID_PAGE_EDGE (widget);

  /* We do this to make sure the colors are set. */
  gtk_widget_ensure_style (widget);

  if (data->create_widget)
    {
      const char *position = glade_string_from_enum (GNOME_TYPE_EDGE_POSITION,
						     page->position);

      source_add (data, "  %s = gnome_druid_page_edge_new (%s);\n",
		  data->wname, position);
    }

  gb_widget_write_standard_source (widget, data);

  color = &page->background_color;
  if (gtk_object_get_data (GTK_OBJECT (widget), BackgroundColor))
    {
      source_add_decl (data,
		       "  GdkColor %s_bg_color = { 0, %i, %i, %i };\n",
		       data->real_wname,
		       color->red, color->green, color->blue);
      source_add (data,
		  "  gnome_druid_page_edge_set_bg_color (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                      &%s_bg_color);\n",
		  data->wname, data->real_wname);
    }

  color = &page->textbox_color;
  if (gtk_object_get_data (GTK_OBJECT (widget), TextboxColor))
    {
      source_add_decl (data,
		       "  GdkColor %s_textbox_color = { 0, %i, %i, %i };\n",
		       data->real_wname,
		       color->red, color->green, color->blue);
      source_add (data,
		  "  gnome_druid_page_edge_set_textbox_color (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                           &%s_textbox_color);\n",
		  data->wname, data->real_wname);
    }

  color = &page->logo_background_color;
  if (gtk_object_get_data (GTK_OBJECT (widget), LogoBackgroundColor))
    {
      source_add_decl (data,
		       "  GdkColor %s_logo_bg_color = { 0, %i, %i, %i };\n",
		       data->real_wname,
		       color->red, color->green, color->blue);
      source_add (data,
		  "  gnome_druid_page_edge_set_logo_bg_color (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                           &%s_logo_bg_color);\n",
		  data->wname, data->real_wname);
    }

  color = &page->title_color;
  if (gtk_object_get_data (GTK_OBJECT (widget), TitleColor))
    {
      source_add_decl (data,
		       "  GdkColor %s_title_color = { 0, %i, %i, %i };\n",
		       data->real_wname,
		       color->red, color->green, color->blue);
      source_add (data,
		  "  gnome_druid_page_edge_set_title_color (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                         &%s_title_color);\n",
		  data->wname, data->real_wname);
    }

  color = &page->text_color;
  if (gtk_object_get_data (GTK_OBJECT (widget), TextColor))
    {
      source_add_decl (data,
		       "  GdkColor %s_text_color = { 0, %i, %i, %i };\n",
		       data->real_wname,
		       color->red, color->green, color->blue);
      source_add (data,
		  "  gnome_druid_page_edge_set_text_color (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                        &%s_text_color);\n",
		  data->wname, data->real_wname);
    }

  title = gtk_object_get_data (GTK_OBJECT (widget), Title);
  if (title && *title)
    {
      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
       "  gnome_druid_page_edge_set_title (GNOME_DRUID_PAGE_EDGE (%s), %s);\n",
		  data->wname,
		  source_make_string_full (title, data->use_gettext && translatable, context));
    }

  if (page->text && strcmp (page->text, ""))
    {
      glade_util_get_translation_properties (widget, Text, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
	"  gnome_druid_page_edge_set_text (GNOME_DRUID_PAGE_EDGE (%s), %s);\n",
		  data->wname,
		  source_make_string_full (page->text, data->use_gettext && translatable, context));
    }

  filename = gtk_object_get_data (GTK_OBJECT (widget), LogoImage);
  if (filename && filename[0])
    {
      source_ensure_decl (data, "  GdkPixbuf *tmp_pixbuf;\n");

      source_add (data,
		  "  tmp_pixbuf = create_pixbuf (\"%s/%s\");\n"
		  "  if (tmp_pixbuf)\n"
		  "    {\n"
		  "      gnome_druid_page_edge_set_logo (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                      tmp_pixbuf);\n"
		  "      gdk_pixbuf_unref (tmp_pixbuf);\n"
		  "    }\n",
		  data->program_name, g_basename (filename), data->wname);
    }

  filename = gtk_object_get_data (GTK_OBJECT (widget), Watermark);
  if (filename && filename[0])
    {
      source_ensure_decl (data, "  GdkPixbuf *tmp_pixbuf;\n");

      source_add (data,
		  "  tmp_pixbuf = create_pixbuf (\"%s/%s\");\n"
		  "  if (tmp_pixbuf)\n"
		  "    {\n"
		  "      gnome_druid_page_edge_set_watermark (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                           tmp_pixbuf);\n"
		  "      gdk_pixbuf_unref (tmp_pixbuf);\n"
		  "    }\n",
		  data->program_name, g_basename (filename), data->wname);
    }

  filename = gtk_object_get_data (GTK_OBJECT (widget), TopWatermark);
  if (filename && filename[0])
    {
      source_ensure_decl (data, "  GdkPixbuf *tmp_pixbuf;\n");

      source_add (data,
		  "  tmp_pixbuf = create_pixbuf (\"%s/%s\");\n"
		  "  if (tmp_pixbuf)\n"
		  "    {\n"
		  "      gnome_druid_page_edge_set_top_watermark (GNOME_DRUID_PAGE_EDGE (%s),\n"
		  "                                               tmp_pixbuf);\n"
		  "      gdk_pixbuf_unref (tmp_pixbuf);\n"
		  "    }\n",
		  data->program_name, g_basename (filename), data->wname);
    }
}



void
gb_gnome_druid_page_edge_destroy (GtkWidget * widget,
				  GbWidgetDestroyData * data)
{
  gchar *filename;

  filename = gtk_object_get_data (GTK_OBJECT (widget), LogoImage);
  glade_project_remove_pixmap (data->project, filename);

  filename = gtk_object_get_data (GTK_OBJECT (widget), TopWatermark);
  glade_project_remove_pixmap (data->project, filename);

  filename = gtk_object_get_data (GTK_OBJECT (widget), Watermark);
  glade_project_remove_pixmap (data->project, filename);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_druid_page_edge_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_druid_page_edge_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_druid_page_start_xpm;
  gbwidget.tooltip = _("Druid Start or Finish Page");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_druid_page_edge_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_druid_page_edge_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_druid_page_edge_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_druid_page_edge_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_druid_page_edge_write_source;
  gbwidget.gb_widget_destroy		= gb_gnome_druid_page_edge_destroy;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_druid_page_edge_create_popup_menu;
*/

  return &gbwidget;
}

