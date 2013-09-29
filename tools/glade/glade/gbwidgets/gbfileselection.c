
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

#include <gtk/gtkfilesel.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/fileseldialog.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Title = "FileSel|GtkWindow::title";
static gchar *Type = "FileSel|GtkWindow::type";
static gchar *Position = "FileSel|GtkWindow::window_position";
static gchar *Modal = "FileSel|GtkWindow::modal";
static gchar *DefaultWidth = "FileSel|GtkWindow::default_width";
static gchar *DefaultHeight = "FileSel|GtkWindow::default_height";
static gchar *FileOps = "GtkFileSelection::show_fileops";
static gchar *Shrink = "FileSel|GtkWindow::allow_shrink";
static gchar *Grow = "FileSel|GtkWindow::allow_grow";
static gchar *AutoShrink = "FileSel|GtkWindow::auto_shrink";
static gchar *IconName = "FileSel|GtkWindow::icon_name";
static gchar *FocusOnMap = "FileSel|GtkWindow::focus_on_map";

static gchar *Resizable = "FileSel|GtkWindow::resizable";
static gchar *DestroyWithParent = "FileSel|GtkWindow::destroy_with_parent";
static gchar *Icon = "FileSel|GtkWindow::icon";

static gchar *Role = "FileSel|GtkWindow::role";
static gchar *TypeHint = "FileSel|GtkWindow::type_hint";
static gchar *SkipTaskbar = "FileSel|GtkWindow::skip_taskbar_hint";
static gchar *SkipPager = "FileSel|GtkWindow::skip_pager_hint";
static gchar *Decorated = "FileSel|GtkWindow::decorated";
static gchar *Gravity = "FileSel|GtkWindow::gravity";
static gchar *Urgency = "FileSel|GtkWindow::urgency_hint";

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkFileSelection, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_file_selection_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_file_selection_new (_("Select File"));

  GtkFileSelection *filesel = GTK_FILE_SELECTION (new_widget);

  /* We want it to be treated as a normal window. */
  gtk_window_set_type_hint (GTK_WINDOW (new_widget),
			    GDK_WINDOW_TYPE_HINT_NORMAL);

  gtk_signal_connect (GTK_OBJECT (new_widget), "delete_event",
		      GTK_SIGNAL_FUNC (editor_close_window), NULL);

  gb_widget_create_from (filesel->ok_button,
			 data->action == GB_CREATING ? "ok_button" : NULL);
  gb_widget_set_child_name (filesel->ok_button, GladeChildOKButton);

  gb_widget_create_from (filesel->cancel_button,
			 data->action == GB_CREATING ? "cancel_button" : NULL);
  gb_widget_set_child_name (filesel->cancel_button, GladeChildCancelButton);

  gtk_object_set_data (GTK_OBJECT (new_widget), TypeHint,
		       GINT_TO_POINTER (GLADE_TYPE_HINT_DIALOG_INDEX));

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_file_selection_create_properties (GtkWidget * widget,
				     GbWidgetCreateArgData * data)
{
  gb_window_create_standard_properties (widget, data,
					Title, Type, Position, Modal,
					DefaultWidth, DefaultHeight,
					Shrink, Grow, AutoShrink,
					IconName, FocusOnMap,
					Resizable, DestroyWithParent, Icon,
					Role, TypeHint, SkipTaskbar,
					SkipPager, Decorated, Gravity, Urgency);
  property_add_bool (FileOps, _("File Ops.:"),
		     _("If the file operation buttons are shown"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_file_selection_get_properties (GtkWidget * widget,
				  GbWidgetGetArgData * data)
{
  gb_window_get_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);
  gb_widget_output_bool (data, FileOps,
			 GTK_FILE_SELECTION (widget)->fileop_c_dir != NULL);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_file_selection_set_properties (GtkWidget * widget,
				  GbWidgetSetArgData * data)
{
  gboolean show_fileops;

  gb_window_set_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);

  show_fileops = gb_widget_input_bool (data, FileOps);
  if (data->apply)
    {
      if (show_fileops)
	gtk_file_selection_show_fileop_buttons (GTK_FILE_SELECTION (widget));
      else
	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (widget));
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkFileSelection, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_file_selection_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_file_selection_write_source (GtkWidget * widget,
				GbWidgetWriteSourceData *data)
{
  gchar *wname, *child_name;
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data, "  %s = gtk_file_selection_new (%s);\n",
		  data->wname,
		  source_make_string_full (GTK_WINDOW (widget)->title,
					   data->use_gettext && translatable,
					   context));
    }

  gb_widget_write_standard_source (widget, data);

  /* The title is already set above, so we pass NULL to skip it. */
  gb_window_write_standard_source (widget, data,
				   NULL, Type, Position, Modal,
				   DefaultWidth, DefaultHeight,
				   Shrink, Grow, AutoShrink,
				   IconName, FocusOnMap,
				   Resizable, DestroyWithParent, Icon,
				   Role, TypeHint, SkipTaskbar,
				   SkipPager, Decorated, Gravity, Urgency);

  if (GTK_FILE_SELECTION (widget)->fileop_c_dir == NULL)
    {
      source_add (data,
		  "  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (%s));\n",
		  data->wname);
    }


  /* We output the source code for the buttons here, but we don't want them
     to be created. We need to remember the dialog's name since data->wname
     will be overwritten. */
  wname = g_strdup (data->wname);

  source_add (data, "\n");
  child_name = (gchar*) gtk_widget_get_name (GTK_FILE_SELECTION (widget)->ok_button);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = GTK_FILE_SELECTION (%s)->ok_button;\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GTK_FILE_SELECTION (widget)->ok_button, data);

  child_name = (gchar*) gtk_widget_get_name (GTK_FILE_SELECTION (widget)->cancel_button);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = GTK_FILE_SELECTION (%s)->cancel_button;\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GTK_FILE_SELECTION (widget)->cancel_button, data);

  g_free (wname);

  data->write_children = FALSE;
}



static GtkWidget *
gb_file_selection_get_child (GtkWidget * widget,
			     const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildOKButton))
    return GTK_FILE_SELECTION (widget)->ok_button;
  else if (!strcmp (child_name, GladeChildCancelButton))
    return GTK_FILE_SELECTION (widget)->cancel_button;
  else
    return NULL;
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_file_selection_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_file_selection_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = fileseldialog_xpm;
  gbwidget.tooltip = _("File Selection Dialog");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_file_selection_new;
  gbwidget.gb_widget_create_properties = gb_file_selection_create_properties;
  gbwidget.gb_widget_get_properties = gb_file_selection_get_properties;
  gbwidget.gb_widget_set_properties = gb_file_selection_set_properties;
  gbwidget.gb_widget_get_child = gb_file_selection_get_child;
  gbwidget.gb_widget_write_source = gb_file_selection_write_source;
  gbwidget.gb_widget_destroy = gb_window_destroy;
/*
   gbwidget.gb_widget_create_popup_menu = gb_file_selection_create_popup_menu;
 */

  return &gbwidget;
}
