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
#include "../graphics/gnome-messagebox.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Title = "GnomeMessageBox|GtkWindow::title";
static gchar *Position = "GnomeMessageBox|GtkWindow::window_position";
static gchar *Modal = "GnomeMessageBox|GtkWindow::modal";
static gchar *DefaultWidth = "GnomeMessageBox|GtkWindow::default_width";
static gchar *DefaultHeight = "GnomeMessageBox|GtkWindow::default_height";
static gchar *Shrink = "GnomeMessageBox|GtkWindow::allow_shrink";
static gchar *Grow = "GnomeMessageBox|GtkWindow::allow_grow";
static gchar *AutoShrink = "GnomeMessageBox|GtkWindow::auto_shrink";
static gchar *IconName = "GnomeMessageBox|GtkWindow::icon_name";
static gchar *FocusOnMap = "GnomeMessageBox|GtkWindow::focus_on_map";

static gchar *Resizable = "GnomeMessageBox|GtkWindow::resizable";
static gchar *DestroyWithParent = "GnomeMessageBox|GtkWindow::destroy_with_parent";
static gchar *Icon = "GnomeMessageBox|GtkWindow::icon";

static gchar *Role = "GnomeMessageBox|GtkWindow::role";
static gchar *TypeHint = "GnomeMessageBox|GtkWindow::type_hint";
static gchar *SkipTaskbar = "GnomeMessageBox|GtkWindow::skip_taskbar_hint";
static gchar *SkipPager = "GnomeMessageBox|GtkWindow::skip_pager_hint";
static gchar *Decorated = "GnomeMessageBox|GtkWindow::decorated";
static gchar *Gravity = "GnomeMessageBox|GtkWindow::gravity";
static gchar *Urgency = "GnomeMessageBox|GtkWindow::urgency_hint";

static gchar *MessageBoxType = "GnomeMessageBox::message_box_type";
static gchar *Message = "GnomeMessageBox::message";

static gchar *AutoClose = "GnomeMessageBox|GnomeDialog::auto_close";
static gchar *HideOnClose = "GnomeMessageBox|GnomeDialog::hide_on_close";


static const gchar *GbMessageBoxTypeChoices[] =
{
  "Information",
  "Warning",
  "Error",
  "Question",
  "Generic",
  NULL
};
static const gchar *GbMessageBoxTypeSymbols[] =
{
  "GNOME_MESSAGE_BOX_INFO",
  "GNOME_MESSAGE_BOX_WARNING",
  "GNOME_MESSAGE_BOX_ERROR",
  "GNOME_MESSAGE_BOX_QUESTION",
  "GNOME_MESSAGE_BOX_GENERIC"
};
static const gchar* GbMessageBoxTypePixmapStockIDs[] =
{
  GTK_STOCK_DIALOG_INFO,
  GTK_STOCK_DIALOG_WARNING,
  GTK_STOCK_DIALOG_ERROR,
  GTK_STOCK_DIALOG_QUESTION,
  ""
};


static void get_message_box_widgets (GtkWidget *dialog,
				     GtkWidget **pixmap,
				     GtkWidget **label);
static void set_message_box_type (GtkWidget *dialog,
				  GtkWidget *pixmap,
				  gchar *type_name);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the funtion in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeMessageBox, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_gnome_message_box_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;
  GList *elem;
  GtkWidget *aa;

  if (data->action == GB_CREATING)
    {
      /* When creating a new dialog, we add a few standard buttons, which
	 the user can change/delete easily. */
      new_widget = gnome_message_box_new ("", GNOME_MESSAGE_BOX_INFO,
					  GTK_STOCK_OK, NULL);

      /* Now turn the buttons into GbWidgets so the user can edit them. */
      elem = GNOME_DIALOG (new_widget)->buttons;
      gb_widget_create_from (GTK_WIDGET (elem->data), "button");
      gtk_object_set_data (GTK_OBJECT (elem->data), GladeButtonStockIDKey,
			   GTK_STOCK_OK);
    }
  else
    {
      /* FIXME: We create it with an OK button, and then remove the button,
	 to work around a bug in gnome_message_box_new() - it tries to set the
	 keyboard focus to the last button, which may not exist. It also
	 ensures that gnome_dialog_init_action_area() has been called. */
      new_widget = gnome_message_box_new ("",  GNOME_MESSAGE_BOX_INFO,
					  GNOME_STOCK_BUTTON_OK, NULL);
      gtk_container_remove (GTK_CONTAINER (GNOME_DIALOG (new_widget)->action_area),
			    GNOME_DIALOG (new_widget)->buttons->data);
      GNOME_DIALOG (new_widget)->buttons = NULL;
    }

  aa = GNOME_DIALOG (new_widget)->action_area;

  gb_widget_create_from (GNOME_DIALOG (new_widget)->vbox,
			 data->action == GB_CREATING ? "dialog-vbox" : NULL);
  gb_widget_set_child_name (GNOME_DIALOG (new_widget)->vbox, GladeChildDialogVBox);
  gb_widget_create_from (aa,
			 data->action == GB_CREATING ? "dialog-action_area"
			 : NULL);
  gb_widget_set_child_name (aa, GladeChildDialogActionArea);

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
gb_gnome_message_box_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (MessageBoxType, _("Message Type:"),
		       _("The type of the message box"),
		       GbMessageBoxTypeChoices);
  property_add_text (Message, _("Message:"), _("The message to display"), 5);

  /* We don't allow setting of the title, so we pass NULL here. */
  gb_window_create_standard_properties (widget, data,
					Title, NULL, Position, Modal,
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
gb_gnome_message_box_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GtkWidget *pixmap, *label;
  const gchar *label_text;
  gint type_index;

  get_message_box_widgets (widget, &pixmap, &label);
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (label != NULL);

  type_index = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						     MessageBoxType));
  gb_widget_output_choice (data, MessageBoxType, type_index,
			   GbMessageBoxTypeSymbols[type_index]);

  label_text = gtk_label_get_text (GTK_LABEL (label));
  gb_widget_output_translatable_text (data, Message, label_text);

  gb_window_get_standard_properties (widget, data,
				     Title, NULL, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);
  gb_widget_output_bool (data, AutoClose, GNOME_DIALOG (widget)->click_closes);
  gb_widget_output_bool (data, HideOnClose, GNOME_DIALOG (widget)->just_hide);
}


/* This tries to find the pixmap and label in the message box. */
static void
get_message_box_widgets (GtkWidget *dialog,
			 GtkWidget **pixmap,
			 GtkWidget **label)
{
  GtkWidget *vbox, *hbox;
  GtkBoxChild *child;
  GList *elem;

  *pixmap = NULL;
  *label = NULL;

  vbox = GNOME_DIALOG (dialog)->vbox;
  if (!vbox || !GTK_IS_VBOX (vbox))
    return;

  for (elem = GTK_BOX (vbox)->children; elem; elem = elem->next)
    {
      child = (GtkBoxChild*)elem->data;
      if (GTK_IS_HBOX (child->widget))
	break;
    }

  if (!elem)
    return;

  hbox = child->widget;

  elem = GTK_BOX (hbox)->children;
  while (elem)
    {
      child = (GtkBoxChild*)elem->data;
      if (GTK_IS_LABEL (child->widget))
	*label = child->widget;
      if (GTK_IS_IMAGE (child->widget))
	*pixmap = child->widget;

      elem = elem->next;
    }
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_message_box_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GtkWidget *pixmap, *label;
  gchar *message, *type_name;
  gboolean auto_close, hide_on_close;

  get_message_box_widgets (widget, &pixmap, &label);
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (label != NULL);

  type_name = gb_widget_input_choice (data, MessageBoxType);
  if (data->apply)
    {
      set_message_box_type (widget, pixmap, type_name);
    }

  message = gb_widget_input_text (data, Message);
  if (data->apply)
    {
      gtk_label_set_text (GTK_LABEL (label), message);
    }
  if (data->action == GB_APPLYING)
    g_free (message);

  gb_window_set_standard_properties (widget, data,
				     Title, NULL, Position, Modal,
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


/* Here we set the type of the message box, i.e. we change the pixmap. */
static void
set_message_box_type (GtkWidget *dialog,
		      GtkWidget *pixmap,
		      gchar *type_name)
{
  gint i, type_index = 0; /* Set to 0 in case we can't find it. */

  /* Find out the index of the type. */
  for (i = 0; GbMessageBoxTypeChoices[i]; i++)
    {
      if (!strcmp (type_name, GbMessageBoxTypeChoices[i])
	  || !strcmp (type_name, GbMessageBoxTypeSymbols[i]))
	{
	  type_index = i;
	  break;
	}
    }

  gtk_object_set_data (GTK_OBJECT (dialog), MessageBoxType,
		       GINT_TO_POINTER (type_index));

  g_object_set (G_OBJECT (pixmap), "stock",
		GbMessageBoxTypePixmapStockIDs[type_index], NULL);
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeMessageBox, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_message_box_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_message_box_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkWidget *pixmap, *label;
  const gchar *label_text;
  gint type_index;
  gchar *wname, *child_name;
  gboolean translatable, context;
  gchar *comments;

  get_message_box_widgets (widget, &pixmap, &label);
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (label != NULL);

  type_index = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						     MessageBoxType));
  label_text = gtk_label_get_text (GTK_LABEL (label));

  if (data->create_widget)
    {
      glade_util_get_translation_properties (widget, Message, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  %s = gnome_message_box_new (%s,\n"
		  "                              %s, NULL);\n",
		  data->wname,
		  source_make_string_full (label_text, data->use_gettext && translatable, context),
		  GbMessageBoxTypeSymbols[type_index]);

#if 0
      source_add (data,
		  "  gtk_container_remove (GTK_CONTAINER (GNOME_DIALOG (%s)->action_area), GNOME_DIALOG (%s)->buttons->data);\n"
		  "  GNOME_DIALOG (%s)->buttons = NULL;\n",
		  data->wname, data->wname, data->wname);
#endif
    }

  gb_widget_write_standard_source (widget, data);

  gb_window_write_standard_source (widget, data,
				   Title, NULL, Position, Modal,
				   DefaultWidth, DefaultHeight,
				   Shrink, Grow, AutoShrink,
				   IconName, FocusOnMap,
				   Resizable, DestroyWithParent, Icon,
				   Role, TypeHint, SkipTaskbar,
				   SkipPager, Decorated, Gravity, Urgency);

  if (!GNOME_DIALOG (widget)->click_closes)
    {
      source_add (data,
		  "  gnome_dialog_set_close (GNOME_DIALOG (%s), FALSE);\n",
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
gb_gnome_message_box_get_child (GtkWidget * widget,
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
gb_gnome_message_box_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_message_box_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_messagebox_xpm;
  gbwidget.tooltip = _("Gnome Message Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_message_box_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_message_box_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_message_box_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_message_box_set_properties;
  gbwidget.gb_widget_get_child		= gb_gnome_message_box_get_child;
  gbwidget.gb_widget_write_source	= gb_gnome_message_box_write_source;
  gbwidget.gb_widget_destroy = gb_window_destroy;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_message_box_create_popup_menu;
*/

  return &gbwidget;
}

