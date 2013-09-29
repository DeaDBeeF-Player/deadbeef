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
#include "../graphics/filechooserdialog.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Title = "FileChooserDialog|GtkWindow::title";
static gchar *Type = "FileChooserDialog|GtkWindow::type";
static gchar *Position = "FileChooserDialog|GtkWindow::window_position";
static gchar *Modal = "FileChooserDialog|GtkWindow::modal";
static gchar *DefaultWidth = "FileChooserDialog|GtkWindow::default_width";
static gchar *DefaultHeight = "FileChooserDialog|GtkWindow::default_height";
static gchar *Shrink = "FileChooserDialog|GtkWindow::allow_shrink";
static gchar *Grow = "FileChooserDialog|GtkWindow::allow_grow";
static gchar *AutoShrink = "FileChooserDialog|GtkWindow::auto_shrink";
static gchar *IconName = "FileChooserDialog|GtkWindow::icon_name";
static gchar *FocusOnMap = "FileChooserDialog|GtkWindow::focus_on_map";

static gchar *Resizable = "FileChooserDialog|GtkWindow::resizable";
static gchar *DestroyWithParent = "FileChooserDialog|GtkWindow::destroy_with_parent";
static gchar *Icon = "FileChooserDialog|GtkWindow::icon";

static gchar *Role = "FileChooserDialog|GtkWindow::role";
static gchar *TypeHint = "FileChooserDialog|GtkWindow::type_hint";
static gchar *SkipTaskbar = "FileChooserDialog|GtkWindow::skip_taskbar_hint";
static gchar *SkipPager = "FileChooserDialog|GtkWindow::skip_pager_hint";
static gchar *Decorated = "FileChooserDialog|GtkWindow::decorated";
static gchar *Gravity = "FileChooserDialog|GtkWindow::gravity";

static gchar *Action = "GtkFileChooserDialog::action";
static gchar *LocalOnly = "GtkFileChooserDialog::local_only";
static gchar *SelectMultiple = "GtkFileChooserDialog::select_multiple";
static gchar *ShowHidden = "GtkFileChooserDialog::show_hidden";
static gchar *Confirm = "GtkFileChooserDialog::do_overwrite_confirmation";
static gchar *Urgency = "FileChooserDialog|GtkWindow::urgency_hint";


static const gchar *GbActionChoices[] =
{"Open", "Save", "Select Folder", "Create Folder", NULL};
static const gint GbActionValues[] =
{
  GTK_FILE_CHOOSER_ACTION_OPEN,
  GTK_FILE_CHOOSER_ACTION_SAVE,
  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
  GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
};
static const gchar *GbActionSymbols[] =
{
  "GTK_FILE_CHOOSER_ACTION_OPEN",
  "GTK_FILE_CHOOSER_ACTION_SAVE",
  "GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER",
  "GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkFileChooserDialog, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_file_chooser_dialog_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget, *button;

  new_widget = gtk_file_chooser_dialog_new (NULL, NULL,
					    GTK_FILE_CHOOSER_ACTION_OPEN,
					    NULL);

  /* We want it to be treated as a normal window. */
  gtk_window_set_type_hint (GTK_WINDOW (new_widget),
			    GDK_WINDOW_TYPE_HINT_NORMAL);

  gtk_signal_connect (GTK_OBJECT (new_widget), "delete_event",
		      GTK_SIGNAL_FUNC (editor_close_window), NULL);


  gb_widget_create_from (GTK_DIALOG (new_widget)->vbox,
			 data->action == GB_CREATING ? "dialog-content_area" : NULL);
  gb_widget_set_child_name (GTK_DIALOG (new_widget)->vbox, GladeChildDialogVBox);

  gb_widget_create_from (GTK_DIALOG (new_widget)->action_area,
			 data->action == GB_CREATING ? "dialog-action_area" : NULL);
  gb_widget_set_child_name (GTK_DIALOG (new_widget)->action_area,
			    GladeChildDialogActionArea);


  if (data->action == GB_CREATING)
    {
      GladeWidgetData *wdata;

      button = gtk_dialog_add_button (GTK_DIALOG (new_widget),
				      GTK_STOCK_CANCEL, -1);
      gb_widget_create_from (button, "button");
      gtk_object_set_data (GTK_OBJECT (button), GladeButtonStockIDKey,
			   (gpointer) GTK_STOCK_CANCEL);
      gtk_object_set_data (GTK_OBJECT (button), GladeDialogResponseIDKey,
			   GINT_TO_POINTER (GTK_RESPONSE_CANCEL));

      button = gtk_dialog_add_button (GTK_DIALOG (new_widget),
				      GTK_STOCK_OPEN, -1);
      gb_widget_create_from (button, "button");
      gtk_object_set_data (GTK_OBJECT (button), GladeButtonStockIDKey,
			   (gpointer) GTK_STOCK_OPEN);
      gtk_object_set_data (GTK_OBJECT (button), GladeDialogResponseIDKey,
			   GINT_TO_POINTER (GTK_RESPONSE_OK));
      /* Set this button as the default. */
      wdata = g_object_get_data (G_OBJECT (button), GB_WIDGET_DATA_KEY);
      wdata->flags |= GLADE_GRAB_DEFAULT;
    }

  gtk_object_set_data (GTK_OBJECT (new_widget), TypeHint,
		       GINT_TO_POINTER (GLADE_TYPE_HINT_DIALOG_INDEX));

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_file_chooser_dialog_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Action, _("Action:"),
		       _("The type of file operation being performed"),
		       GbActionChoices);
  property_add_bool (LocalOnly, _("Local Only:"),
		     _("Whether the selected files should be limited to local files"));
  property_add_bool (SelectMultiple, _("Select Multiple:"),
		     _("Whether to allow multiple files to be selected"));
  property_add_bool (ShowHidden, _("Show Hidden:"),
		     _("Whether the hidden files and folders should be displayed"));
  property_add_bool (Confirm, _("Confirm:"),
		     _("Whether a confirmation dialog will be displayed if a file will be overwritten"));

  gb_window_create_standard_properties (widget, data,
					Title, Type, Position, Modal,
					DefaultWidth, DefaultHeight,
					Shrink, Grow, AutoShrink,
					IconName, FocusOnMap,
					Resizable, DestroyWithParent, Icon,
					Role, TypeHint, SkipTaskbar,
					SkipPager, Decorated, Gravity, Urgency);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_file_chooser_dialog_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GtkFileChooserAction action;
  gboolean local_only, select_multiple, show_hidden, confirm;
  gint i;

  g_object_get (widget,
		"action", &action,
		"local_only", &local_only,
		"select_multiple", &select_multiple,
		"show_hidden", &show_hidden,
		"do_overwrite_confirmation", &confirm,
		NULL);

  for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]); i++)
    {
      if (GbActionValues[i] == action)
	gb_widget_output_choice (data, Action, i, GbActionSymbols[i]);
    }

  gb_widget_output_bool (data, LocalOnly, local_only);
  gb_widget_output_bool (data, SelectMultiple, select_multiple);
  gb_widget_output_bool (data, ShowHidden, show_hidden);
  gb_widget_output_bool (data, Confirm, confirm);

  gb_window_get_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);

}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_file_chooser_dialog_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *action;
  gboolean local_only, select_multiple, show_hidden, confirm;
  gint i;

  action = gb_widget_input_choice (data, Action);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]);
	   i++)
	{
	  if (!strcmp (action, GbActionChoices[i])
	      || !strcmp (action, GbActionSymbols[i]))
	    {
	      g_object_set (widget, "action", GbActionValues[i], NULL);
	      break;
	    }
	}
    }

  local_only = gb_widget_input_bool (data, LocalOnly);
  if (data->apply)
    g_object_set (widget, "local_only", local_only, NULL);

  select_multiple = gb_widget_input_bool (data, SelectMultiple);
  if (data->apply)
    g_object_set (widget, "select_multiple", select_multiple, NULL);

  show_hidden = gb_widget_input_bool (data, ShowHidden);
  if (data->apply)
    g_object_set (widget, "show_hidden", show_hidden, NULL);

  confirm = gb_widget_input_bool (data, Confirm);
  if (data->apply)
    g_object_set (widget, "do_overwrite_confirmation", confirm, NULL);

  gb_window_set_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);

}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkFileChooserDialog, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_file_chooser_dialog_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_file_chooser_dialog_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkFileChooserAction action;
  gboolean local_only, select_multiple, show_hidden, confirm;
  gchar *wname, *child_name;
  gboolean translatable, context;
  gchar *comments;

  g_object_get (widget,
		"action", &action,
		"local_only", &local_only,
		"select_multiple", &select_multiple,
		"show_hidden", &show_hidden,
		"do_overwrite_confirmation", &confirm,
		NULL);

  if (data->create_widget)
    {
      const gchar *action_symbol = GbActionSymbols[0];
      gint i;

      for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]);
	   i++)
	{
	  if (GbActionValues[i] == action)
	    action_symbol = GbActionSymbols[i];
	}

      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  %s = gtk_file_chooser_dialog_new (%s, NULL, %s, NULL);\n",
		  data->wname,
		  source_make_string_full (GTK_WINDOW (widget)->title,
					   data->use_gettext && translatable,
					   context),
		  action_symbol);
    }

  gb_widget_write_standard_source (widget, data);

  if (!local_only || select_multiple || show_hidden || confirm)
    {
      source_add (data,   "  g_object_set (%s,\n", data->wname);

      if (!local_only)
	source_add (data, "                \"local-only\", FALSE,\n");

      if (select_multiple)
	source_add (data, "                \"select-multiple\", TRUE,\n");

      if (show_hidden)
	source_add (data, "                \"show-hidden\", TRUE,\n");

      if (confirm)
	source_add (data, "                \"confirm\", TRUE,\n");

      source_add (data,   "                NULL);\n");
    }

  gb_window_write_standard_source (widget, data,
				   NULL, Type, Position, Modal,
				   DefaultWidth, DefaultHeight,
				   Shrink, Grow, AutoShrink,
				   IconName, FocusOnMap,
				   Resizable, DestroyWithParent, Icon,
				   Role, TypeHint, SkipTaskbar,
				   SkipPager, Decorated, Gravity, Urgency);

  /* We output the source code for the children here, since the code should
     not include calls to create the widgets. We need to specify that the
     names used are like: "GTK_DIALOG (<dialog-name>)->vbox".
     We need to remember the dialog's name since data->wname
     will be overwritten. */
  wname = g_strdup (data->wname);

  source_add (data, "\n");
  child_name = (gchar*) gtk_widget_get_name (GTK_DIALOG (widget)->vbox);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = gtk_dialog_get_content_area (GTK_DIALOG (%s));\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GTK_DIALOG (widget)->vbox, data);

  /* action_area is a child of vbox so I had to add a kludge to stop it
     being written as a normal child - we need to do it here so that we
     don't output code to create it. */
  child_name = (gchar*) gtk_widget_get_name (GTK_DIALOG (widget)->action_area);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = gtk_dialog_get_action_area (GTK_DIALOG (%s));\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GTK_DIALOG (widget)->action_area, data);

  g_free (wname);
  data->write_children = FALSE;
}


static GtkWidget *
gb_file_chooser_dialog_get_child (GtkWidget * widget,
				  const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildDialogVBox))
    return GTK_DIALOG (widget)->vbox;
  else if (!strcmp (child_name, GladeChildDialogActionArea))
    return GTK_DIALOG (widget)->action_area;
  else
    return NULL;
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_file_chooser_dialog_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_file_chooser_dialog_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = filechooserdialog_xpm;
  gbwidget.tooltip = _("File Chooser Dialog");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_file_chooser_dialog_new;
  gbwidget.gb_widget_create_properties	= gb_file_chooser_dialog_create_properties;
  gbwidget.gb_widget_get_properties	= gb_file_chooser_dialog_get_properties;
  gbwidget.gb_widget_set_properties	= gb_file_chooser_dialog_set_properties;
  gbwidget.gb_widget_get_child		= gb_file_chooser_dialog_get_child;
  gbwidget.gb_widget_write_source	= gb_file_chooser_dialog_write_source;
  gbwidget.gb_widget_destroy = gb_window_destroy;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_file_chooser_dialog_create_popup_menu;
*/

  return &gbwidget;
}

