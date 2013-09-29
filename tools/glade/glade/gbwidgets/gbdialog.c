
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
#include <gtk/gtkbbox.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkstock.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/dialog.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Title = "Dialog|GtkWindow::title";
static gchar *Type = "Dialog|GtkWindow::type";
static gchar *Position = "Dialog|GtkWindow::window_position";
static gchar *Modal = "Dialog|GtkWindow::modal";
static gchar *DefaultWidth = "Dialog|GtkWindow::default_width";
static gchar *DefaultHeight = "Dialog|GtkWindow::default_height";
static gchar *Shrink = "Dialog|GtkWindow::allow_shrink";
static gchar *Grow = "Dialog|GtkWindow::allow_grow";
static gchar *AutoShrink = "Dialog|GtkWindow::auto_shrink";
static gchar *IconName = "Dialog|GtkWindow::icon_name";
static gchar *FocusOnMap = "Dialog|GtkWindow::focus_on_map";

static gchar *Resizable = "Dialog|GtkWindow::resizable";
static gchar *DestroyWithParent = "Dialog|GtkWindow::destroy_with_parent";
static gchar *Icon = "Dialog|GtkWindow::icon";

static gchar *Role = "Dialog|GtkWindow::role";
static gchar *TypeHint = "Dialog|GtkWindow::type_hint";
static gchar *SkipTaskbar = "Dialog|GtkWindow::skip_taskbar_hint";
static gchar *SkipPager = "Dialog|GtkWindow::skip_pager_hint";
static gchar *Decorated = "Dialog|GtkWindow::decorated";
static gchar *Gravity = "Dialog|GtkWindow::gravity";

static gchar *HasSeparator = "GtkDialog::has_separator";

static gchar *Urgency = "Dialog|GtkWindow::urgency_hint";

static void show_dialog_creation_dialog (GbWidgetNewData * data);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/


/* Action is GB_LOADING or GB_CREATING. */
static GtkWidget*
create_dialog (const gchar *title, GbWidgetAction action)
{
  GtkWidget *new_widget;

  new_widget = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (new_widget), title);
  gtk_window_set_policy (GTK_WINDOW (new_widget), TRUE, TRUE, FALSE);

  /* We want it to be treated as a normal window. */
  gtk_window_set_type_hint (GTK_WINDOW (new_widget),
			    GDK_WINDOW_TYPE_HINT_NORMAL);

  gtk_signal_connect (GTK_OBJECT (new_widget), "delete_event",
		      GTK_SIGNAL_FUNC (editor_close_window), NULL);

  /* We need to size the placeholders or the dialog is very small. */
  if (action == GB_CREATING)
    {
      GtkWidget *placeholder = editor_new_placeholder ();
      gtk_widget_set_usize (placeholder, 300, 200);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (new_widget)->vbox), placeholder,
			  TRUE, TRUE, 0);
    }

  gb_widget_create_from (GTK_DIALOG (new_widget)->vbox,
			 action == GB_CREATING ? "dialog-vbox" : NULL);
  gb_widget_set_child_name (GTK_DIALOG (new_widget)->vbox, GladeChildDialogVBox);

  gb_widget_create_from (GTK_DIALOG (new_widget)->action_area,
			 action == GB_CREATING ? "dialog-action_area" : NULL);
  gb_widget_set_child_name (GTK_DIALOG (new_widget)->action_area,
			    GladeChildDialogActionArea);

  gtk_object_set_data (GTK_OBJECT (new_widget), TypeHint,
		       GINT_TO_POINTER (GLADE_TYPE_HINT_DIALOG_INDEX));

  return new_widget;
}


/*
 * Creates a new GtkWidget of class GtkDialog, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_dialog_new (GbWidgetNewData * data)
{
  if (data->action == GB_LOADING)
    {
      return create_dialog (data->name, GB_LOADING);
    }
  else
    {
      show_dialog_creation_dialog (data);
      return NULL;
    }
}


static void
add_button (GtkDialog *dialog,
	    const gchar *stock_id,
	    gint response_id,
	    const gchar *name)
{
  GtkWidget *button;

  button = gtk_dialog_add_button (dialog, stock_id, -1);
  gb_widget_create_from (button, name);
  gtk_object_set_data (GTK_OBJECT (button), GladeButtonStockIDKey,
		       (gpointer) stock_id);
  gtk_object_set_data (GTK_OBJECT (button), GladeDialogResponseIDKey,
		       GINT_TO_POINTER (response_id));
  if (response_id == GTK_RESPONSE_HELP)
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (button->parent),
					button, TRUE);
}


static void
on_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *window, *button;
  GtkWidget *cancel_ok, *just_ok,  *cancel_apply_ok, *just_close;
  GtkWidget *standard_layout, *num_buttons, *show_help_button;
  GtkDialog *dialog;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (!gb_widget_can_finish_new (data))
    {
      gtk_widget_destroy (window);
      return;
    }

  new_widget = create_dialog (data->name, GB_CREATING);
  dialog = GTK_DIALOG (new_widget);

  /* Get pointers to all the widgets in the creation dialog we need. */
  standard_layout = gtk_object_get_data (GTK_OBJECT (window), "standard_layout");
  cancel_ok = gtk_object_get_data (GTK_OBJECT (window), "cancel_ok");
  just_ok = gtk_object_get_data (GTK_OBJECT (window), "just_ok");
  cancel_apply_ok = gtk_object_get_data (GTK_OBJECT (window), "cancel_apply_ok");
  just_close = gtk_object_get_data (GTK_OBJECT (window), "just_close");
  num_buttons = gtk_object_get_data (GTK_OBJECT (window), "num_buttons");
  show_help_button = gtk_object_get_data (GTK_OBJECT (window), "show_help_button");

  if (GTK_TOGGLE_BUTTON (show_help_button)->active)
    {
      add_button (dialog, GTK_STOCK_HELP, GTK_RESPONSE_HELP, "helpbutton");
    }

  if (GTK_TOGGLE_BUTTON (standard_layout)->active)
    {
      if (GTK_TOGGLE_BUTTON (cancel_ok)->active)
	{
	  add_button (dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		      "cancelbutton");
	  add_button (dialog, GTK_STOCK_OK, GTK_RESPONSE_OK,
		      "okbutton");
	}
      else if (GTK_TOGGLE_BUTTON (just_ok)->active)
	{
	  add_button (dialog, GTK_STOCK_OK, GTK_RESPONSE_OK,
		      "okbutton");
	}
      else if (GTK_TOGGLE_BUTTON (cancel_apply_ok)->active)
	{
	  add_button (dialog, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
		      "applybutton");
	  add_button (dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		      "cancelbutton");
	  add_button (dialog, GTK_STOCK_OK, GTK_RESPONSE_OK,
		      "okbutton");
	}
      else if (GTK_TOGGLE_BUTTON (just_close)->active)
	{
	  add_button (dialog, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		      "closebutton");
	}
    }
  else
    {
      gint buttons, i;

      buttons = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (num_buttons));
      /* We set the response_ids to 1,2,3 etc. */
      for (i = 1; i <= buttons; i++)
	{
	  button = gb_widget_new ("GtkButton", NULL);
	  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	  gtk_dialog_add_action_widget (GTK_DIALOG (new_widget), button, i);
	}
    }

  gb_widget_initialize (new_widget, data);
  (*data->callback) (new_widget, data);

  gtk_widget_destroy (window);
}


static void
on_dialog_destroy (GtkWidget * widget,
		   GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


static void on_standard_layout_toggled (GtkWidget *radiobutton,
					GtkWidget *dialog)
{
  GtkWidget *cancel_ok, *just_ok,  *cancel_apply_ok, *just_close;
  GtkWidget *num_buttons;
  gboolean sens;

  cancel_ok = gtk_object_get_data (GTK_OBJECT (dialog), "cancel_ok");
  just_ok = gtk_object_get_data (GTK_OBJECT (dialog), "just_ok");
  cancel_apply_ok = gtk_object_get_data (GTK_OBJECT (dialog), "cancel_apply_ok");
  just_close = gtk_object_get_data (GTK_OBJECT (dialog), "just_close");

  num_buttons = gtk_object_get_data (GTK_OBJECT (dialog), "num_buttons");

  sens = (GTK_TOGGLE_BUTTON (radiobutton)->active) ? TRUE : FALSE;

  gtk_widget_set_sensitive (cancel_ok, sens);
  gtk_widget_set_sensitive (just_ok, sens);
  gtk_widget_set_sensitive (cancel_apply_ok, sens);
  gtk_widget_set_sensitive (just_close, sens);

  gtk_widget_set_sensitive (num_buttons, !sens);
}


static void
show_dialog_creation_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox1;
  GtkWidget *table1;
  GtkWidget *cancel_ok;
  GSList *layout_group_group = NULL;
  GtkWidget *just_ok;
  GtkWidget *cancel_apply_ok;
  GtkWidget *just_close;
  GtkWidget *standard_layout;
  GSList *main_group_group = NULL;
  GtkWidget *show_help_button;
  GtkWidget *number_of_buttons;
  GtkObject *num_buttons_adj;
  GtkWidget *num_buttons;

  dialog = glade_util_create_dialog (_("New dialog"), data->parent,
				     GTK_SIGNAL_FUNC (on_dialog_ok),
				     data, &vbox1);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_dialog_destroy), data);

  table1 = gtk_table_new (7, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (vbox1), table1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 8);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 8);

  cancel_ok = gtk_radio_button_new_with_mnemonic (NULL, _("Cancel, OK"));
  gtk_object_set_data (GTK_OBJECT (dialog), "cancel_ok", cancel_ok);
  gtk_widget_show (cancel_ok);
  gtk_table_attach (GTK_TABLE (table1), cancel_ok, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (cancel_ok), layout_group_group);
  layout_group_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (cancel_ok));

  just_ok = gtk_radio_button_new_with_mnemonic (NULL, _("OK"));
  gtk_object_set_data (GTK_OBJECT (dialog), "just_ok", just_ok);
  gtk_widget_show (just_ok);
  gtk_table_attach (GTK_TABLE (table1), just_ok, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (just_ok), layout_group_group);
  layout_group_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (just_ok));

  cancel_apply_ok = gtk_radio_button_new_with_mnemonic (NULL, _("Cancel, Apply, OK"));
  gtk_object_set_data (GTK_OBJECT (dialog), "cancel_apply_ok", cancel_apply_ok);
  gtk_widget_show (cancel_apply_ok);
  gtk_table_attach (GTK_TABLE (table1), cancel_apply_ok, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (cancel_apply_ok), layout_group_group);
  layout_group_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (cancel_apply_ok));

  just_close = gtk_radio_button_new_with_mnemonic (NULL, _("Close"));
  gtk_object_set_data (GTK_OBJECT (dialog), "just_close", just_close);
  gtk_widget_show (just_close);
  gtk_table_attach (GTK_TABLE (table1), just_close, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (just_close), layout_group_group);
  layout_group_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (just_close));

  standard_layout = gtk_radio_button_new_with_mnemonic (NULL, _("_Standard Button Layout:"));
  gtk_object_set_data (GTK_OBJECT (dialog), "standard_layout", standard_layout);
  gtk_widget_show (standard_layout);
  gtk_table_attach (GTK_TABLE (table1), standard_layout, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (standard_layout), main_group_group);
  main_group_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (standard_layout));

  number_of_buttons = gtk_radio_button_new_with_mnemonic (NULL, _("_Number of Buttons:"));
  gtk_widget_show (number_of_buttons);
  gtk_table_attach (GTK_TABLE (table1), number_of_buttons, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (number_of_buttons), main_group_group);
  main_group_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (number_of_buttons));

  num_buttons_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
  num_buttons = gtk_spin_button_new (GTK_ADJUSTMENT (num_buttons_adj), 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (num_buttons), TRUE);
  gtk_object_set_data (GTK_OBJECT (dialog), "num_buttons", num_buttons);
  gtk_widget_show (num_buttons);
  gtk_table_attach (GTK_TABLE (table1), num_buttons, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 4);

  show_help_button = gtk_check_button_new_with_mnemonic (_("Show Help Button"));
  gtk_object_set_data (GTK_OBJECT (dialog), "show_help_button", show_help_button);
  gtk_widget_show (show_help_button);
  gtk_table_attach (GTK_TABLE (table1), show_help_button, 0, 2, 6, 7,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 6);

  gtk_widget_set_sensitive (num_buttons, FALSE);

  gtk_signal_connect (GTK_OBJECT (standard_layout), "toggled",
		      GTK_SIGNAL_FUNC (on_standard_layout_toggled), dialog);

  gtk_widget_show (dialog);
  gtk_grab_add (dialog);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_dialog_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  gb_window_create_standard_properties (widget, data,
					Title, Type, Position, Modal,
					DefaultWidth, DefaultHeight,
					Shrink, Grow, AutoShrink,
					IconName, FocusOnMap,
					Resizable, DestroyWithParent, Icon,
					Role, TypeHint, SkipTaskbar,
					SkipPager, Decorated, Gravity, Urgency);
  property_add_bool (HasSeparator, _("Has Separator:"),
		     _("If the dialog has a horizontal separator above the buttons"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_dialog_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gb_window_get_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);
  gb_widget_output_bool (data, HasSeparator,
			 gtk_dialog_get_has_separator (GTK_DIALOG (widget)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_dialog_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean has_separator;

  gb_window_set_standard_properties (widget, data,
				     Title, Type, Position, Modal,
				     DefaultWidth, DefaultHeight,
				     Shrink, Grow, AutoShrink,
				     IconName, FocusOnMap,
				     Resizable, DestroyWithParent, Icon,
				     Role, TypeHint, SkipTaskbar,
				     SkipPager, Decorated, Gravity, Urgency);
  has_separator = gb_widget_input_bool (data, HasSeparator);
  if (data->apply)
    {
      gtk_dialog_set_has_separator (GTK_DIALOG (widget), has_separator);
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkDialog, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_dialog_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_dialog_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *wname, *child_name;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_dialog_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  gb_window_write_standard_source (widget, data,
				   Title, Type, Position, Modal,
				   DefaultWidth, DefaultHeight,
				   Shrink, Grow, AutoShrink,
				   IconName, FocusOnMap,
				   Resizable, DestroyWithParent, Icon,
				   Role, TypeHint, SkipTaskbar,
				   SkipPager, Decorated, Gravity, Urgency);

  if (!gtk_dialog_get_has_separator (GTK_DIALOG (widget)))
    {
      source_add (data, "  gtk_dialog_set_has_separator (GTK_DIALOG (%s), FALSE);\n",
		  data->wname);
    }


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
gb_dialog_get_child (GtkWidget * widget,
		     const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildDialogVBox))
    return GTK_DIALOG (widget)->vbox;
  else if (!strcmp (child_name, GladeChildDialogActionArea))
    return GTK_DIALOG (widget)->action_area;
  else
    return NULL;
}


/* Converts a response id to a string, either a standard GTK+ response string
   such as "GTK_RESPONSE_OK" or an integer e.g. "1". Note that for integers
   it uses a static buffer. */
char*
gb_dialog_response_id_to_string (gint response_id)
{
  gint i;

  if (response_id >= 0)
    {
      static char buffer[16];

      sprintf (buffer, "%i", response_id);
      return buffer;
    }

  for (i = 0; i < GladeStockResponsesSize; i++)
    {
      if (GladeStockResponses[i].response_id == response_id)
	return GladeStockResponses[i].name;
    }

  return "0";
}


gint
gb_dialog_response_id_from_string (const gchar *response_id)
{
  gint i;

  if (!response_id || !*response_id)
    return 0;

  for (i = 0; i < GladeStockResponsesSize; i++)
    {
      if (!strcmp (GladeStockResponses[i].name, response_id))
	return GladeStockResponses[i].response_id;
    }

  return atoi (response_id);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_dialog_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_dialog_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = dialog_xpm;
  gbwidget.tooltip = _("Dialog");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_dialog_new;
  gbwidget.gb_widget_create_properties = gb_dialog_create_properties;
  gbwidget.gb_widget_get_properties = gb_dialog_get_properties;
  gbwidget.gb_widget_set_properties = gb_dialog_set_properties;
  gbwidget.gb_widget_get_child = gb_dialog_get_child;
  gbwidget.gb_widget_write_source = gb_dialog_write_source;
  gbwidget.gb_widget_destroy = gb_window_destroy;
/*
   gbwidget.gb_widget_create_popup_menu = gb_dialog_create_popup_menu;
 */

  return &gbwidget;
}
