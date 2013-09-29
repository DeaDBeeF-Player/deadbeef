/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999-2002  Damon Chaplin
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
#include "../graphics/aboutdialog.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Name = "GtkAboutDialog::name";
static gchar *Copyright = "GtkAboutDialog::copyright";
static gchar *Comments = "GtkAboutDialog::comments";
static gchar *License = "GtkAboutDialog::license";
static gchar *WrapLicense = "GtkAboutDialog::wrap_license";
static gchar *Website = "GtkAboutDialog::website";
static gchar *WebsiteLabel = "GtkAboutDialog::website_label";

static gchar *Authors = "GtkAboutDialog::authors";
static gchar *Documenters = "GtkAboutDialog::documenters";
static gchar *Artists = "GtkAboutDialog::artists";
static gchar *TranslatorCredits = "GtkAboutDialog::translator_credits";

static gchar *Logo = "GtkAboutDialog::logo";


static gchar *DestroyWithParent = "GtkAboutDialog|GtkWindow::destroy_with_parent";

#define GLADE_TRANSLATORS_STRING "translator-credits"

/* This is the old string we used for GnomeAbout, but translators don't like
   it as the '_' character gets confused with an underlined accelerator key. */
#define GLADE_TRANSLATORS_STRING2 "translator_credits"


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkAboutDialog, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_about_dialog_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;
  gchar *project_name;

  new_widget = gtk_about_dialog_new ();

  project_name = glade_project_get_name (data->project);
  if (!project_name)
    project_name = _("Application Name");
  gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (new_widget), project_name);

  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (new_widget), "1.0");

  /* Now we connect our normal delete_event handler. */
  gtk_signal_connect (GTK_OBJECT (new_widget), "delete_event",
		      GTK_SIGNAL_FUNC (editor_close_window), NULL);

  /* Set the default comment string for the translator credits. */
  glade_util_set_translation_properties (new_widget, TranslatorCredits,
					 TRUE, "TRANSLATORS: Replace this string with your names, one name per line.", FALSE);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_about_dialog_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_filename (Logo, _("Logo:"), _("The pixmap to use as the logo"));

  property_add_string (Name, _("Program Name:"), _("The name of the application"));
  property_add_text (Comments, _("Comments:"), _("Additional information, such as a description of the application"), 3);
  property_add_text (Copyright, _("Copyright:"), _("The copyright notice"), 2);

  property_add_string (Website, _("Website URL:"), _("The URL of the application's website"));
  property_add_string (WebsiteLabel, _("Website Label:"), _("The label to display for the link to the website"));

  property_add_text (License, _("License:"), _("The license details of the application"), 3);
  property_add_bool (WrapLicense, _("Wrap License:"), _("If the license text should be wrapped"));

  property_add_text (Authors, _("Authors:"), _("The authors of the package, one on each line"), 2);
  property_add_text (Documenters, _("Documenters:"), _("The documenters of the package, one on each line"), 2);
  property_add_text (Artists, _("Artists:"), _("The people who have created the artwork for the package, one on each line"), 2);
  property_add_text (TranslatorCredits, _("Translators:"), _("The translators of the package. This should normally be left empty so that translators can add their names in the po files"), 2);

  /* We onlt need the common DestroyWithParent window property. The others
     shouldn't really need to be changed on a simple About dialog. */
  gb_window_create_standard_properties (widget, data,
					NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL,
					NULL, NULL,
					NULL, DestroyWithParent, NULL,
					NULL, NULL, NULL,
					NULL, NULL, NULL, NULL);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_about_dialog_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  const gchar *name, *copyright, *comments, *license, *website, *website_label;
  const char *translator_credits, *translator_credits_output;
  gboolean wrap_license;

  gb_window_get_standard_properties (widget, data,
				     NULL, NULL, NULL, NULL,
				     NULL, NULL, NULL, NULL, NULL,
				     NULL, NULL,
				     NULL, DestroyWithParent, NULL,
				     NULL, NULL, NULL,
				     NULL, NULL, NULL, NULL);

  name = gtk_about_dialog_get_name (GTK_ABOUT_DIALOG (widget));
  gb_widget_output_translatable_string (data, Name, name);

  copyright = gtk_about_dialog_get_copyright (GTK_ABOUT_DIALOG (widget));
  gb_widget_output_translatable_text (data, Copyright, copyright);

  comments = gtk_about_dialog_get_comments (GTK_ABOUT_DIALOG (widget));
  gb_widget_output_translatable_text (data, Comments, comments);

  license = gtk_about_dialog_get_license (GTK_ABOUT_DIALOG (widget));
  gb_widget_output_translatable_text (data, License, license);

  wrap_license = gtk_about_dialog_get_wrap_license (GTK_ABOUT_DIALOG (widget));
  gb_widget_output_bool (data, WrapLicense, wrap_license);

  /* We store the Website & WebsiteURL in the object datalist, since the
     widget does odd things when we set them. */
  website = g_object_get_data (G_OBJECT (widget), Website);
  gb_widget_output_string (data, Website, website);

  website_label = g_object_get_data (G_OBJECT (widget), WebsiteLabel);
  gb_widget_output_translatable_string (data, WebsiteLabel, website_label);

  gb_widget_output_text (data, Authors,
			 gtk_object_get_data (GTK_OBJECT (widget), Authors));
  gb_widget_output_text (data, Documenters,
			 gtk_object_get_data (GTK_OBJECT (widget),
					      Documenters));
  gb_widget_output_text (data, Artists,
			 gtk_object_get_data (GTK_OBJECT (widget), Artists));

  /* If we are saving, and translator_credits isn't set but is translatable,
     we save the string "translator-credits". Translators can then
     replace this with their own names in the po files. */
  translator_credits = gtk_about_dialog_get_translator_credits (GTK_ABOUT_DIALOG (widget));
  translator_credits_output = translator_credits;
  if (data->action == GB_SAVING
      && (!translator_credits || *translator_credits == '\0'))
    {
      gchar *comments_text;
      gboolean translatable, context;

      glade_util_get_translation_properties (widget, TranslatorCredits,
					     &translatable,
					     &comments_text, &context);
      if (translatable)
	translator_credits_output = GLADE_TRANSLATORS_STRING;
    }
  gb_widget_output_translatable_text (data, TranslatorCredits,
				      translator_credits_output);

  gb_widget_output_pixmap_filename (data, Logo,
				    gtk_object_get_data (GTK_OBJECT (widget),
							 Logo));
}


static gchar**
strv_from_property (GbWidgetSetArgData *data, GtkWidget *widget, const char *key)
{
  gchar **retval = NULL;
  char *str;

  str = gb_widget_input_text (data, key);
  if (data->apply)
    {
      gtk_object_set_data_full (GTK_OBJECT (widget), key, g_strdup (str),
				str ? g_free : NULL);
      retval = g_strsplit (str, "\n", 0);
    }
  if (data->action == GB_APPLYING)
    g_free (str);

  return retval;
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_about_dialog_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar **authors, **documenters, **artists;
  gchar *name, *copyright, *comments, *license, *website, *website_label;
  gchar *translators, *filename, *old_filename;
  GdkPixbuf *logo = NULL;
  GObject *object;
  gboolean wrap_license;

  object = G_OBJECT (widget);

  gb_window_set_standard_properties (widget, data,
				     NULL, NULL, NULL, NULL,
				     NULL, NULL, NULL, NULL, NULL,
				     NULL, NULL,
				     NULL, DestroyWithParent, NULL,
				     NULL, NULL, NULL,
				     NULL, NULL, NULL, NULL);

  name = gb_widget_input_string (data, Name);
  if (data->apply)
    gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (widget), name);

  copyright = gb_widget_input_text (data, Copyright);
  if (data->apply)
    gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (widget), copyright);
  if (data->action == GB_APPLYING)
    g_free (copyright);

  comments = gb_widget_input_text (data, Comments);  
  if (data->apply)
    gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (widget), comments);
  if (data->action == GB_APPLYING)
    g_free (comments);

  license = gb_widget_input_text (data, License);  
  if (data->apply)
    gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (widget), license);
  if (data->action == GB_APPLYING)
    g_free (license);

  wrap_license = gb_widget_input_bool (data, WrapLicense);
  if (data->apply)
    gtk_about_dialog_set_wrap_license (GTK_ABOUT_DIALOG (widget), wrap_license);


  website = gb_widget_input_string (data, Website);  
  if (data->apply)
    g_object_set_data_full (G_OBJECT (widget), Website, g_strdup (website),
			    website ? g_free : NULL);

  website_label = gb_widget_input_string (data, WebsiteLabel);  
  if (data->apply)
    g_object_set_data_full (G_OBJECT (widget), WebsiteLabel,
			    g_strdup (website_label),
			    website_label ? g_free : NULL);

  authors = strv_from_property (data, widget, Authors);
  if (data->apply)
    gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (widget),
				  (const gchar**) authors);
  g_strfreev (authors);

  documenters = strv_from_property (data, widget, Documenters);
  if (data->apply)
    gtk_about_dialog_set_documenters (GTK_ABOUT_DIALOG (widget),
				      (const gchar**) documenters);
  g_strfreev (documenters);

  artists = strv_from_property (data, widget, Artists);
  if (data->apply)
    gtk_about_dialog_set_artists (GTK_ABOUT_DIALOG (widget),
				  (const gchar**) artists);
  g_strfreev (artists);

  translators = gb_widget_input_text (data, TranslatorCredits);
  if (data->apply)
    {
      if (!strcmp (translators, GLADE_TRANSLATORS_STRING)
	  || !strcmp (translators, GLADE_TRANSLATORS_STRING2))
	g_object_set (object, "translator-credits", "", NULL);
      else
	g_object_set (object, "translator-credits", translators, NULL);
    }
  if (data->action == GB_APPLYING)
    g_free (translators);

  filename = gb_widget_input_pixmap_filename (data, Logo);
  if (data->apply)
    {
      if (filename && filename[0] == '\0')
	filename = NULL;

      old_filename = gtk_object_get_data (GTK_OBJECT (widget), Logo);
      glade_project_remove_pixmap (data->project, old_filename);
      gtk_object_set_data_full (GTK_OBJECT (widget), Logo,
				g_strdup (filename), filename ? g_free : NULL);
      glade_project_add_pixmap (data->project, filename);
      logo = filename ? gdk_pixbuf_new_from_file (filename, NULL) : NULL;
      g_object_set (object, "logo", logo, NULL);
      if (logo)
	g_object_unref (logo);
    }
  if (data->action == GB_LOADING)
    g_free (filename);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkAboutDialog, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_about_dialog_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/


static gboolean
output_creators_array (GtkWidget *widget, GbWidgetWriteSourceData *data,
		       gchar *array_name, gchar *property_name)
{
  gchar *string, **strv;
  gint i;

  string = gtk_object_get_data (GTK_OBJECT (widget), property_name);
  if (!string || *string == '\0')
    return FALSE;

  source_add_decl (data, "  const gchar *%s[] = {\n", array_name);

  strv = g_strsplit (string, "\n", 0);
  for (i = 0; strv[i]; i++)
    {
      if (strv[i][0])
	source_add_decl (data, "    %s,\n",
			 source_make_string (strv[i], FALSE));
    }

  source_add_decl (data, "    NULL\n  };\n");
  g_strfreev (strv);

  return TRUE;
}

/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_about_dialog_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gboolean translatable, context, wrap_license;
  gchar *comments;
  const gchar *name, *copyright, *about_comments, *license;
  const gchar *website, *website_label, *translators;
  gchar *logo_filename;
  char *pixbuf_name;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_about_dialog_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  gb_window_write_standard_source (widget, data,
				   NULL, NULL, NULL, NULL,
				   NULL, NULL, NULL, NULL, NULL,
				   NULL, NULL,
				   NULL, DestroyWithParent, NULL,
				   NULL, NULL, NULL,
				   NULL, NULL, NULL, NULL);

  /* We set the version using the standard "VERSION" macro. */
  source_add (data,
	      "  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (%s), VERSION);\n", data->wname);

  name = gtk_about_dialog_get_name (GTK_ABOUT_DIALOG (widget));
  if (name && *name)
    {
      glade_util_get_translation_properties (widget, Name, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);
      source_add (data,
		  "  gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (%s), %s);\n",
		  data->wname,
		  source_make_string_full (name,
					   data->use_gettext && translatable,
					   context));
    }

  copyright = gtk_about_dialog_get_copyright (GTK_ABOUT_DIALOG (widget));
  if (copyright && *copyright)
    {
      glade_util_get_translation_properties (widget, Copyright, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);
      source_add (data,
		  "  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (%s), %s);\n",
		  data->wname,
		  source_make_string_full (copyright,
					   data->use_gettext && translatable,
					   context));
    }

  about_comments = gtk_about_dialog_get_comments (GTK_ABOUT_DIALOG (widget));
  if (about_comments && *about_comments)
    {
      glade_util_get_translation_properties (widget, Comments, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);
      source_add (data,
		  "  gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (%s), %s);\n",
		  data->wname,
		  source_make_string_full (about_comments,
					   data->use_gettext && translatable,
					   context));
    }

  license = gtk_about_dialog_get_license (GTK_ABOUT_DIALOG (widget));
  if (license && *license)
    {
      glade_util_get_translation_properties (widget, License, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);
      source_add (data,
		  "  gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (%s), %s);\n",
		  data->wname,
		  source_make_string_full (license,
					   data->use_gettext && translatable,
					   context));
    }

  wrap_license = gtk_about_dialog_get_wrap_license (GTK_ABOUT_DIALOG (widget));
  if (wrap_license)
    {
      source_add (data,
		  "  gtk_about_dialog_set_wrap_license (GTK_ABOUT_DIALOG (%s), TRUE);\n",
		  data->wname);
    }


  website = g_object_get_data (G_OBJECT (widget), Website);
  if (website && *website)
    {
      source_add (data,
		  "  gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (%s), %s);\n",
		  data->wname,
		  source_make_string (website, FALSE));
    }

  website_label = g_object_get_data (G_OBJECT (widget), WebsiteLabel);
  if (website_label && *website_label)
    {
      glade_util_get_translation_properties (widget, WebsiteLabel,
					     &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);
      source_add (data,
		  "  gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG (%s), %s);\n",
		  data->wname,
		  source_make_string_full (website_label,
					   data->use_gettext && translatable,
					   context));
    }


  if (output_creators_array (widget, data, "authors", Authors))
    source_add (data,
		"  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (%s), authors);\n", data->wname);

  if (output_creators_array (widget, data, "documenters", Documenters))
    source_add (data,
		"  gtk_about_dialog_set_documenters (GTK_ABOUT_DIALOG (%s), documenters);\n", data->wname);

  if (output_creators_array (widget, data, "artists", Artists))
    source_add (data,
		"  gtk_about_dialog_set_artists (GTK_ABOUT_DIALOG (%s), artists);\n", data->wname);
  

  /* If translators is set, then we use that specific string.
     If it is not set, then if gettext support is enabled we output code
     to call gettext to translate the special "translator_credits" string,
     and use that if it is translated. */
  translators = gtk_about_dialog_get_translator_credits (GTK_ABOUT_DIALOG (widget));
  glade_util_get_translation_properties (widget, TranslatorCredits,
					 &translatable, &comments, &context);
  if (translators && translators[0])
    {
      source_add_translator_comments_to_buffer (data, GLADE_DECLARATIONS, translatable, comments);
      source_add_decl (data, "  gchar *translators = %s;\n",
		       source_make_string (translators, FALSE));

      source_add (data,
		  "  gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG (%s), translators);\n", data->wname);
    }
  else if (data->use_gettext && translatable)
    {
      if (!comments || comments[0] == '\0')
	comments = "TRANSLATORS: Replace this string with your names, one name per line.";

      source_add_translator_comments_to_buffer (data, GLADE_DECLARATIONS,
						translatable, comments);
      source_add_decl (data,
		       "  gchar *translators = _(\"%s\");\n",
		       GLADE_TRANSLATORS_STRING);

      source_add (data,
		  "  gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG (%s), translators);\n", data->wname);
    }


  logo_filename = gtk_object_get_data (GTK_OBJECT (widget), Logo);
  if (logo_filename && logo_filename[0])
    {
      pixbuf_name = g_strdup_printf ("%s_logo_pixbuf", data->wname);

      source_add_decl (data, "  GdkPixbuf *%s;\n", pixbuf_name);
      source_create_pixbuf (data, pixbuf_name, logo_filename);

      source_add (data,
		  "  gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (%s), %s);\n",
		  data->wname, pixbuf_name);

      g_free (pixbuf_name);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_about_dialog_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_about_dialog_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = aboutdialog_xpm;
  gbwidget.tooltip = _("About Dialog");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_about_dialog_new;
  gbwidget.gb_widget_create_properties	= gb_about_dialog_create_properties;
  gbwidget.gb_widget_get_properties	= gb_about_dialog_get_properties;
  gbwidget.gb_widget_set_properties	= gb_about_dialog_set_properties;
  gbwidget.gb_widget_write_source	= gb_about_dialog_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_about_dialog_create_popup_menu;
*/

  return &gbwidget;
}

