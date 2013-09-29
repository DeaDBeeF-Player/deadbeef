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
#include "../graphics/gnome-dialog.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Title = "GnomeDialog|GtkWindow::title";
static gchar *Type = "GnomeDialog|GtkWindow::type";
static gchar *Position = "GnomeDialog|GtkWindow::window_position";
static gchar *Modal = "GnomeDialog|GtkWindow::modal";
static gchar *DefaultWidth = "GnomeDialog|GtkWindow::default_width";
static gchar *DefaultHeight = "GnomeDialog|GtkWindow::default_height";
static gchar *Shrink = "GnomeDialogl|GtkWindow::allow_shrink";
static gchar *Grow = "GnomeDialog|GtkWindow::allow_grow";
static gchar *AutoShrink = "GnomeDialog|GtkWindow::auto_shrink";
static gchar *IconName = "GnomeDialog|GtkWindow::icon_name";
static gchar *FocusOnMap = "GnomeDialog|GtkWindow::focus_on_map";

static gchar *Resizable = "GnomeDialog|GtkWindow::resizable";
static gchar *DestroyWithParent = "GnomeDialog|GtkWindow::destroy_with_parent";
static gchar *Icon = "GnomeDialog|GtkWindow::icon";

static gchar *Role = "GnomeDialog|GtkWindow::role";
static gchar *TypeHint = "GnomeDialog|GtkWindow::type_hint";
static gchar *SkipTaskbar = "GnomeDialog|GtkWindow::skip_taskbar_hint";
static gchar *SkipPager = "GnomeDialog|GtkWindow::skip_pager_hint";
static gchar *Decorated = "GnomeDialog|GtkWindow::decorated";
static gchar *Gravity = "GnomeDialog|GtkWindow::gravity";
static gchar *Urgency = "GnomeDialog|GtkWindow::urgency_hint";

static gchar *AutoClose = "GnomeDialog::auto_close";
static gchar *HideOnClose = "GnomeDialog::hide_on_close";



/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeDialog, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_gnome_dialog_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget, *placeholder;
  GList *elem;

  if (data->action == GB_CREATING)
    {
      /* When creating a new dialog, we add a few standard buttons, which
	 the user can change/delete easily. */
      new_widget = gnome_dialog_new (NULL, 
				     GTK_STOCK_CANCEL, 
				     GTK_STOCK_OK,
				     NULL);

      /* Now turn the buttons into GbWidgets so the user can edit them. */
      elem = GNOME_DIALOG (new_widget)->buttons;
      gb_widget_create_from (GTK_WIDGET (elem->data), "button");
      gtk_object_set_data (GTK_OBJECT (elem->data), GladeButtonStockIDKey,
			   GTK_STOCK_CANCEL);

      elem = elem->next;
      gb_widget_create_from (GTK_WIDGET (elem->data), "button");
      gtk_object_set_data (GTK_OBJECT (elem->data), GladeButtonStockIDKey,
			   GTK_STOCK_OK);

      /* We need to size the placeholders or the dialog is very small. */
      placeholder = editor_new_placeholder ();
      gtk_widget_set_usize (placeholder, 300, 200);
      gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (new_widget)->vbox),
			  placeholder, TRUE, TRUE, 0);
    }
  else
    {
      /* When loading we create the bare dialog with no buttons. */
      new_widget = gnome_dialog_new (NULL, NULL);
    }

  gb_widget_create_from (GNOME_DIALOG (new_widget)->vbox,
			 data->action == GB_CREATING ? "dialog-vbox" : NULL);
  gb_widget_set_child_name (GNOME_DIALOG (new_widget)->vbox, GladeChildDialogVBox);

  gb_widget_create_from (GNOME_DIALOG (new_widget)->action_area,
			 data->action == GB_CREATING ? "dialog-action_area"
						     : NULL);
  gb_widget_set_child_name (GNOME_DIALOG (new_widget)->action_area,
			    GladeChildDialogActionArea);

  /* We connect a close signal handler which always returns TRUE so that
     the built-in close functionality is skipped. */
  gtk_signal_connect (GTK_OBJECT (new_widget), "close",
		      GTK_SIGNAL_FUNC (gtk_true), NULL);

  /* Now we connect our normal delete_event handler. */
  gtk_signal_connect (GTK_OBJECT (new_widget), "delete_event",
		      GTK_SIGNAL_FUNC (editor_close_window), NULL);

  gtk_object_set_data (GTK_OBJECT (new_widget), TypeHint,
		       GINT_TO_POINTER (GLADE_TYPE_HINT_DIALOG_INDEX));

  return new_widget;
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_dialog_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  gb_window_create_standard_properties (widget, data,
					Title, Type, Position, Modal,
					DefaultWidth, DefaultHeight,
					Shrink, Grow, AutoShrink,
					IconName, FocusOnMap,
					Resizable, DestroyWithParent, Icon,
					Role, TypeHint, SkipTaskbar,
					SkipPager, Decorated, Gravity, Urgency);
  property_add_bool (AutoClose, _("Auto Close:"),
		     _("If the dialog closes when any button is clicked"));
  property_add_bool (HideOnClose, _("Hide on Close:"),
		     _("If the dialog is hidden when it is closed, instead of being destroyed"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_dialog_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_window_get_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);

  gb_widget_output_bool (data, AutoClose, GNOME_DIALOG (widget)->click_closes);
  gb_widget_output_bool (data, HideOnClose, GNOME_DIALOG (widget)->just_hide);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_dialog_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean auto_close, hide_on_close;

  gb_window_set_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);

  auto_close = gb_widget_input_bool (data, AutoClose);
  if (data->apply)
    gnome_dialog_set_close (GNOME_DIALOG (widget), auto_close);

  hide_on_close = gb_widget_input_bool (data, HideOnClose);
  if (data->apply)
    gnome_dialog_close_hides (GNOME_DIALOG (widget), hide_on_close);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeDialog, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_dialog_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_dialog_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *wname, *child_name;
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      gchar *title;

      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      title = GTK_WINDOW (widget)->title;
      source_add (data, "  %s = gnome_dialog_new (%s, NULL);\n",
		  data->wname,
		  title ? source_make_string_full (title, data->use_gettext && translatable, context)
		        : "NULL");
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

  if (GNOME_DIALOG (widget)->click_closes)
    {
      source_add (data,
		  "  gnome_dialog_set_close (GNOME_DIALOG (%s), TRUE);\n",
		  data->wname);
    }

  if (GNOME_DIALOG (widget)->just_hide)
    {
      source_add (data,
		  "  gnome_dialog_close_hides (GNOME_DIALOG (%s), TRUE);\n",
		  data->wname);
    }

  /* We output the source code for the children here, since the code should
     not include calls to create the widgets. We need to specify that the
     names used are like: "GTK_DIALOG (<dialog-name>)->vbox".
     We need to remember the dialog's name since data->wname
     will be overwritten. */
  wname = g_strdup (data->wname);

  source_add (data, "\n");
  child_name = (char*) gtk_widget_get_name (GNOME_DIALOG (widget)->vbox);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = GNOME_DIALOG (%s)->vbox;\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GNOME_DIALOG (widget)->vbox, data);

  /* action_area is a child of vbox so I had to add a kludge to stop it
     being written as a normal child - we need to do it here so that we
     don't output code to create it. */
  child_name = (char*) gtk_widget_get_name (GNOME_DIALOG (widget)->action_area);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = GNOME_DIALOG (%s)->action_area;\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GNOME_DIALOG (widget)->action_area, data);

  g_free (wname);
  data->write_children = FALSE;
}


static GtkWidget *
gb_gnome_dialog_get_child (GtkWidget * widget,
			   const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildDialogVBox))
    return GNOME_DIALOG (widget)->vbox;
  else if (!strcmp (child_name, GladeChildDialogActionArea))
    return GNOME_DIALOG (widget)->action_area;
  else
    return NULL;
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_dialog_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_dialog_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_dialog_xpm;
  gbwidget.tooltip = _("Gnome Dialog Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_dialog_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_dialog_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_dialog_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_dialog_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_dialog_write_source;
  gbwidget.gb_widget_get_child		= gb_gnome_dialog_get_child;
  gbwidget.gb_widget_destroy = gb_window_destroy;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_dialog_create_popup_menu;
*/

  return &gbwidget;
}

