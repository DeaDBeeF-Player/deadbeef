
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

#include <math.h>
#include <string.h>

#include <gtk/gtkcombo.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkexpander.h>
#include <gtk/gtkfilechooserbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkeventbox.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/label.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Label = "GtkLabel::label";
static gchar *UseUnderline = "GtkLabel::use_underline";
static gchar *UseMarkup = "GtkLabel::use_markup";
static gchar *Justify = "GtkLabel::justify";
static gchar *Wrap = "GtkLabel::wrap";
static gchar *XAlign = "Label|GtkMisc::xalign";
static gchar *YAlign = "Label|GtkMisc::yalign";
static gchar *XPad = "Label|GtkMisc::xpad";
static gchar *YPad = "Label|GtkMisc::ypad";
static gchar *Selectable = "GtkLabel::selectable";

static gchar *Ellipsize = "GtkLabel::ellipsize";
static gchar *WidthChars = "GtkLabel::width_chars";
static gchar *SingleLineMode = "GtkLabel::single_line_mode";
static gchar *Angle = "GtkLabel::angle";

static gchar *FocusTarget = "GtkLabel::mnemonic_widget";


static const gchar *GbJustifyChoices[] =
{
  "Left",
  "Right",
  "Center",
  "Fill",
  NULL
};
static const gint GbJustifyValues[] =
{
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

static const gchar *GbEllipsizeChoices[] =
{
  "None",
  "Start",
  "Middle",
  "End",
  NULL
};
static const gint GbEllipsizeValues[] =
{
  PANGO_ELLIPSIZE_NONE,
  PANGO_ELLIPSIZE_START,
  PANGO_ELLIPSIZE_MIDDLE,
  PANGO_ELLIPSIZE_END
};
static const gchar *GbEllipsizeSymbols[] =
{
  "PANGO_ELLIPSIZE_NONE",
  "PANGO_ELLIPSIZE_START",
  "PANGO_ELLIPSIZE_MIDDLE",
  "PANGO_ELLIPSIZE_END"
};


static void gb_label_get_focus_targets (GtkWidget * widget,
					GList ** focus_targets);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkLabel, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_label_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  new_widget = gtk_label_new (data->name);

  /* If we are creating a new label in a table or in an event box in a table
     set it to left-aligned, since that is what is usually wanted. */
  if (data->action == GB_CREATING && data->parent
      && (GTK_IS_TABLE (data->parent)
	  || (GTK_IS_EVENT_BOX (data->parent) && data->parent->parent
	      && GTK_IS_TABLE (data->parent->parent))))
    {
      gtk_misc_set_alignment (GTK_MISC (new_widget), 0.0, 0.5);
    }

  return new_widget;
}



void
gb_label_create_standard_properties (GtkWidget * widget,
				     GbWidgetCreateArgData * data,
				     const char *label_p,
				     const char *use_underline_p,
				     const char *use_markup_p,
				     const char *justify_p,
				     const char *wrap_p,
				     const char *selectable_p,
				     const char *xalign_p,
				     const char *yalign_p,
				     const char *xpad_p,
				     const char *ypad_p,
				     const char *focus_target_p,
				     const char *ellipsize_p,
				     const char *width_chars_p,
				     const char *single_line_mode_p,
				     const char *angle_p)
{
  GtkWidget *combo;

  property_add_text (label_p, _("Label:"), _("The text to display"), 5);
  property_add_bool (use_underline_p, _("Use Underline:"),
		     _("If the text includes an underlined access key"));
  property_add_bool (use_markup_p, _("Use Markup:"),
		     _("If the text includes pango markup"));
  property_add_choice (justify_p, _("Justify:"),
		       _("The justification of the lines of the label"),
		       GbJustifyChoices);
  property_add_bool (wrap_p, _("Wrap Text:"),
		     _("If the text is wrapped to fit within the width of the label"));
  property_add_bool (selectable_p, _("Selectable:"),
		     _("If the label text can be selected with the mouse"));
  property_add_float_range (xalign_p, _("X Align:"),
			    _("The horizontal alignment of the entire label"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (yalign_p, _("Y Align:"),
			    _("The vertical alignment of the entire label"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_int_range (xpad_p, _("X Pad:"), _("The horizontal padding"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (ypad_p, _("Y Pad:"), _("The vertical padding"),
			  0, 1000, 1, 10, 1);
  property_add_combo (focus_target_p, _("Focus Target:"),
		      _("The widget to set the keyboard focus to when the underlined access key is used"),
		      NULL);
  combo = property_get_value_widget (focus_target_p);
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (combo)->entry), FALSE);
  /*gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, TRUE);*/

  property_add_choice (ellipsize_p, _("Ellipsize:"),
		       _("How to ellipsize the string"),
		       GbEllipsizeChoices);
  property_add_int_range (width_chars_p, _("Width in Chars:"),
			  _("The width of the label in characters"),
			  -1, 1000, 1, 10, 1);
  property_add_bool (single_line_mode_p, _("Single Line Mode:"),
		     _("If the label is only given enough height for a single line"));
  property_add_float_range (angle_p, _("Angle:"),
			    _("The angle of the label text"),
			    0, 360, 1, 10, 1, 2);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_label_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  gb_label_create_standard_properties (widget, data,
				       Label, UseUnderline, UseMarkup,
				       Justify, Wrap, Selectable,
				       XAlign, YAlign, XPad, YPad,
				       FocusTarget, Ellipsize, WidthChars,
				       SingleLineMode, Angle);
}



/* This tries to find a parent widget of a label that would be used for
   mnemonic activation, e.g. a button or a menuitem. If it finds one, it
   returns TRUE. */
static gboolean
gb_label_find_mnemonic_widget (GtkWidget *widget)
{
  GtkWidget *parent;

  parent = widget->parent;

  while (parent)
    {
      if (GTK_IS_EXPANDER (parent))
	{
	  if (gtk_expander_get_label_widget (GTK_EXPANDER (parent)) == widget)
	    return TRUE;
	  else
	    return FALSE;
	}
      if (GTK_WIDGET_GET_CLASS (parent)->activate_signal)
	{
	  return TRUE;
	}
      if (GTK_IS_MENU_ITEM (parent))
	{
	  return TRUE;
	}

      parent = parent->parent;
    }

  return FALSE;
}


void
gb_label_get_standard_properties (GtkWidget * widget,
				  GbWidgetGetArgData * data,
				  const char *label_p,
				  const char *use_underline_p,
				  const char *use_markup_p,
				  const char *justify_p,
				  const char *wrap_p,
				  const char *selectable_p,
				  const char *xalign_p,
				  const char *yalign_p,
				  const char *xpad_p,
				  const char *ypad_p,
				  const char *focus_target_p,
				  const char *ellipsize_p,
				  const char *width_chars_p,
				  const char *single_line_mode_p,
				  const char *angle_p)
{
  const gchar *label_text;
  PangoEllipsizeMode ellipsize_mode;
  gint i, width_chars;
  gboolean single_line_mode;
  gfloat angle;

  label_text = gtk_label_get_label (GTK_LABEL (widget));
  gb_widget_output_translatable_text (data, label_p, label_text);

  gb_widget_output_bool (data, use_underline_p,
			 gtk_label_get_use_underline (GTK_LABEL (widget)));
  gb_widget_output_bool (data, use_markup_p,
			 GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), use_markup_p)));

  for (i = 0; i < sizeof (GbJustifyValues) / sizeof (GbJustifyValues[0]); i++)
    {
      if (GbJustifyValues[i] == GTK_LABEL (widget)->jtype)
	gb_widget_output_choice (data, justify_p, i, GbJustifySymbols[i]);
    }
  gb_widget_output_bool (data, wrap_p, GTK_LABEL (widget)->wrap);
  gb_widget_output_bool (data, selectable_p,
			 gtk_label_get_selectable (GTK_LABEL (widget)));
  gb_widget_output_float (data, xalign_p, GTK_MISC (widget)->xalign);
  gb_widget_output_float (data, yalign_p, GTK_MISC (widget)->yalign);
  gb_widget_output_int (data, xpad_p, GTK_MISC (widget)->xpad);
  gb_widget_output_int (data, ypad_p, GTK_MISC (widget)->ypad);

  /* Labels not in buttons may have a focus target widget. */
  if (!gb_label_find_mnemonic_widget (widget))
    {
      gchar *accel_target;

      accel_target = gtk_object_get_data (GTK_OBJECT (widget), focus_target_p);

      /* If we're showing we need to display the list of possible focus target
	 widgets. We walk the tree of widgets in this component, and if a
	 widget has CAN_FOCUS set, we add it to the list. */
      if (data->action == GB_SHOWING)
	{
	  GList *focus_targets = NULL, *standard_items = NULL;
	  GtkWidget *item, *combo;

	  property_set_visible (focus_target_p, TRUE);

	  gb_label_get_focus_targets (gtk_widget_get_toplevel (widget),
				      &focus_targets);
	  property_set_combo_strings (focus_target_p, focus_targets);
	  g_list_free (focus_targets);

	  combo = property_get_value_widget (focus_target_p);

	  item = gtk_list_item_new_with_label (_("Auto"));
	  gtk_widget_show (item);
	  standard_items = g_list_append (standard_items, item);

	  item = gtk_list_item_new ();
	  gtk_widget_set_sensitive (item, FALSE);
	  gtk_widget_show (item);
	  standard_items = g_list_append (standard_items, item);
	  gtk_combo_set_item_string (GTK_COMBO (combo), GTK_ITEM (item), "");

	  gtk_list_prepend_items (GTK_LIST (GTK_COMBO (combo)->list),
				  standard_items);

	  if (!accel_target)
	    {
	      accel_target = _("Auto");
	    }
	  gb_widget_output_combo (data, focus_target_p, accel_target);
	}
      else
	{
	  /* When saving, we only save the property if it has been set. */
	  if (accel_target)
	    {
	      /* First check that the widget is still there, and if it isn't
		 just skip it. */
	      if (glade_util_find_widget (gtk_widget_get_toplevel (widget),
					  accel_target))
		{
		  gb_widget_output_combo (data, focus_target_p, accel_target);
		}
	    }
	  else
	    {
	      /* If no target has been set, and the label has an underlined
		 key, we try to find a default target and save that. */
	      if (gtk_label_get_use_underline (GTK_LABEL (widget)))
		{
		  GtkWidget *accel_target;

		  accel_target = glade_util_find_default_accelerator_target (widget);
		  if (accel_target)
		    {
		      gb_widget_output_string (data, focus_target_p, gtk_widget_get_name (accel_target));
		    }
		}
	    }
	}
    }
  else
    {
      if (data->action == GB_SHOWING)
	{
	  property_set_visible (focus_target_p, FALSE);
	}
    }

  ellipsize_mode = gtk_label_get_ellipsize (GTK_LABEL (widget));
  for (i = 0; i < sizeof (GbEllipsizeValues) / sizeof (GbEllipsizeValues[0]); i++)
    {
      if (GbEllipsizeValues[i] == ellipsize_mode)
	gb_widget_output_choice (data, ellipsize_p, i, GbEllipsizeSymbols[i]);
    }

  width_chars = gtk_label_get_width_chars (GTK_LABEL (widget));
  gb_widget_output_int (data, width_chars_p, width_chars);

  single_line_mode = gtk_label_get_single_line_mode (GTK_LABEL (widget));
  gb_widget_output_bool (data, single_line_mode_p, single_line_mode);

  angle = gtk_label_get_angle (GTK_LABEL (widget));
  gb_widget_output_float (data, angle_p, angle);
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_label_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gb_label_get_standard_properties (widget, data,
				    Label, UseUnderline, UseMarkup,
				    Justify, Wrap, Selectable,
				    XAlign, YAlign, XPad, YPad,
				    FocusTarget, Ellipsize, WidthChars,
				    SingleLineMode, Angle);
}


static void
gb_label_get_focus_targets (GtkWidget * widget,
			    GList ** focus_targets)
{
  /* GtkWidget now has a "mnemonic_activate" signal, which some widgets use
     to set focus to themselves or a child. So it is difficult to know if
     a widget can be a focus target. We can't just use CAN_FOCUS as before.
     For now I've just added some specific widgets. */
  if (GB_IS_GB_WIDGET (widget)
      && (GTK_WIDGET_CAN_FOCUS (widget)
	  || GTK_IS_COMBO_BOX (widget) || GTK_IS_FILE_CHOOSER_BUTTON (widget)
	  || GLADE_IS_CUSTOM_WIDGET (widget)))
    {
      *focus_targets = g_list_insert_sorted (*focus_targets,
					     (gchar*) gtk_widget_get_name (widget),
					     (GCompareFunc) g_utf8_collate);
    }

  if (GTK_IS_CONTAINER (widget))
    {
      gtk_container_forall (GTK_CONTAINER (widget),
			    (GtkCallback) gb_label_get_focus_targets,
			    focus_targets);
    }
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
void
gb_label_set_standard_properties (GtkWidget * widget,
				  GbWidgetSetArgData * data,
				  const char *label_p,
				  const char *use_underline_p,
				  const char *use_markup_p,
				  const char *justify_p,
				  const char *wrap_p,
				  const char *selectable_p,
				  const char *xalign_p,
				  const char *yalign_p,
				  const char *xpad_p,
				  const char *ypad_p,
				  const char *focus_target_p,
				  const char *ellipsize_p,
				  const char *width_chars_p,
				  const char *single_line_mode_p,
				  const char *angle_p)
{
  gchar *label, *justify, *accel_target, *ellipsize_mode;
  const gchar *label_text;
  gfloat xalign, yalign, angle;
  gint xpad, ypad, i, width_chars;
  gboolean wrap, selectable, set_alignment = FALSE, set_padding = FALSE;
  gboolean use_underline, use_markup, single_line_mode;
  gboolean set_label = FALSE;

  use_underline = gb_widget_input_bool (data, use_underline_p);
  if (data->apply)
    gtk_label_set_use_underline (GTK_LABEL (widget), use_underline);

  use_markup = gb_widget_input_bool (data, use_markup_p);
  if (data->apply)
    {
      set_label = TRUE;
      gtk_object_set_data (GTK_OBJECT (widget), use_markup_p,
			   GINT_TO_POINTER (use_markup));
    }

  label = gb_widget_input_text (data, label_p);
  if (data->apply)
    {
      set_label = TRUE;
      label_text = label;
    }
  else
    {
      label_text = gtk_label_get_label (GTK_LABEL (widget));
    }

  if (set_label)
    {
      gboolean prev_use_markup;

      use_markup = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
							 use_markup_p));

      /* We try to parse the markup here, and if it isn't valid, we will
	 turn use_markup off and show it as plain text. */
      if (use_markup)
	{
	  GError *error = NULL;
	  gunichar accel_marker = 0;
	  PangoAttrList *attrs = NULL;
	  gunichar accel_char = 0;
	  gchar *text = NULL;

	  if (gtk_label_get_use_underline (GTK_LABEL (widget)))
	    accel_marker = '_';

	  /* We check it is valid markup. If it isn't we will set "use_markup"
	     to FALSE. Note that we don't need attrs, text, or accel_char but
	     it seems to crash otherwise. */
	  if (!pango_parse_markup (label_text, -1, accel_marker, &attrs, &text,
				   &accel_char, &error))
	    {
	      use_markup = FALSE;
	      g_error_free (error);
	    }
	  else
	    {
	      if (attrs)
		pango_attr_list_unref (attrs);
	      g_free (text);
	    }
	}

      /* If we are turning use_markup off, we want to do that before setting
	 the text. If we are turning it on, we want to do it after. */
      prev_use_markup = gtk_label_get_use_markup (GTK_LABEL (widget));
      if (!use_markup && prev_use_markup)
	gtk_label_set_use_markup (GTK_LABEL (widget), use_markup);

      gtk_label_set_label (GTK_LABEL (widget), label_text);

      if (use_markup && !prev_use_markup)
	gtk_label_set_use_markup (GTK_LABEL (widget), use_markup);
    }

  if (data->action == GB_APPLYING)
    g_free (label);

  justify = gb_widget_input_choice (data, justify_p);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbJustifyValues) / sizeof (GbJustifyValues[0]);
	   i++)
	{
	  if (!strcmp (justify, GbJustifyChoices[i])
	      || !strcmp (justify, GbJustifySymbols[i]))
	    {
	      gtk_label_set_justify (GTK_LABEL (widget), GbJustifyValues[i]);
	      break;
	    }
	}
    }

  wrap = gb_widget_input_bool (data, wrap_p);
  if (data->apply)
    gtk_label_set_line_wrap (GTK_LABEL (widget), wrap);

  selectable = gb_widget_input_bool (data, selectable_p);
  if (data->apply)
    gtk_label_set_selectable (GTK_LABEL (widget), selectable);

  xalign = gb_widget_input_float (data, xalign_p);
  if (data->apply)
    set_alignment = TRUE;
  else
    xalign = GTK_MISC (widget)->xalign;

  yalign = gb_widget_input_float (data, yalign_p);
  if (data->apply)
    set_alignment = TRUE;
  else
    yalign = GTK_MISC (widget)->yalign;

  if (set_alignment)
    gtk_misc_set_alignment (GTK_MISC (widget), xalign, yalign);

  xpad = gb_widget_input_int (data, xpad_p);
  if (data->apply)
    set_padding = TRUE;
  else
    xpad = GTK_MISC (widget)->xpad;

  ypad = gb_widget_input_int (data, ypad_p);
  if (data->apply)
    set_padding = TRUE;
  else
    ypad = GTK_MISC (widget)->ypad;

  if (set_padding)
    gtk_misc_set_padding (GTK_MISC (widget), xpad, ypad);

  /* Labels not in buttons may have a focus target widget. */
  accel_target = gb_widget_input_combo (data, focus_target_p);
  if (data->apply)
    {
      if (!gb_label_find_mnemonic_widget (widget))
	{
	  if (!strcmp (accel_target, _("Auto")))
	    accel_target = NULL;

	  gtk_object_set_data_full (GTK_OBJECT (widget), focus_target_p,
				    g_strdup (accel_target),
				    accel_target ? g_free : NULL);
	}
    }

  ellipsize_mode = gb_widget_input_choice (data, ellipsize_p);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbEllipsizeValues) / sizeof (GbEllipsizeValues[0]);
	   i++)
	{
	  if (!strcmp (ellipsize_mode, GbEllipsizeChoices[i])
	      || !strcmp (ellipsize_mode, GbEllipsizeSymbols[i]))
	    {
	      gtk_label_set_ellipsize (GTK_LABEL (widget),
				       GbEllipsizeValues[i]);
	      break;
	    }
	}
    }

  width_chars = gb_widget_input_int (data, width_chars_p);
  if (data->apply)
    gtk_label_set_width_chars (GTK_LABEL (widget), width_chars);

  single_line_mode = gb_widget_input_bool (data, single_line_mode_p);
  if (data->apply)
    gtk_label_set_single_line_mode (GTK_LABEL (widget), single_line_mode);

  angle = gb_widget_input_float (data, angle_p);
  if (data->apply)
    gtk_label_set_angle (GTK_LABEL (widget), angle);
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_label_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gb_label_set_standard_properties (widget, data,
				    Label, UseUnderline, UseMarkup,
				    Justify, Wrap, Selectable,
				    XAlign, YAlign, XPad, YPad,
				    FocusTarget, Ellipsize, WidthChars,
				    SingleLineMode, Angle);
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkLabel, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_label_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */


void
gb_label_write_standard_source (GtkWidget * widget,
				GbWidgetWriteSourceData * data,
				const char *label_p,
				const char *use_underline_p,
				const char *use_markup_p,
				const char *justify_p,
				const char *wrap_p,
				const char *selectable_p,
				const char *xalign_p,
				const char *yalign_p,
				const char *xpad_p,
				const char *ypad_p,
				const char *focus_target_p,
				const char *ellipsize_p,
				const char *width_chars_p,
				const char *single_line_mode_p,
				const char *angle_p)
{
  GtkWidget *accel_target = NULL;
  PangoEllipsizeMode ellipsize_mode;
  gint i, width_chars;
  gboolean single_line_mode;
  gfloat angle;

  if (gtk_object_get_data (GTK_OBJECT (widget), use_markup_p))
    source_add (data, "  gtk_label_set_use_markup (GTK_LABEL (%s), TRUE);\n",
		data->wname);

  /* If there is an underlined accelerator, set up the accel signal.
     If the label is in a button or something similar, this is now setup
     automatically by GTK+. If the accelerator is being used to set focus
     to something like a GtkEntry, we need to set the mnemonic_widget. */
  if (gtk_label_get_use_underline (GTK_LABEL (widget))
      && !gb_label_find_mnemonic_widget (widget))
    {
      gchar *target_name = gtk_object_get_data (GTK_OBJECT (widget),
						focus_target_p);
      if (target_name)
	accel_target = glade_util_find_widget (gtk_widget_get_toplevel (widget), target_name);

      if (!accel_target)
	accel_target = glade_util_find_default_accelerator_target (widget);

      if (accel_target)
	{
	  target_name = (gchar*) gtk_widget_get_name (accel_target);
	  target_name = source_create_valid_identifier (target_name);
	  source_add_to_buffer (data, GLADE_ACCELERATORS,
		"  gtk_label_set_mnemonic_widget (GTK_LABEL (%s), %s);\n",
				data->wname, target_name);
	  g_free (target_name);
	}
    }

  if (GTK_LABEL (widget)->jtype != GTK_JUSTIFY_LEFT)
    {
      for (i = 0; i < sizeof (GbJustifyValues) / sizeof (GbJustifyValues[0]);
	   i++)
	{
	  if (GbJustifyValues[i] == GTK_LABEL (widget)->jtype)
	    source_add (data,
			"  gtk_label_set_justify (GTK_LABEL (%s), %s);\n",
			data->wname, GbJustifySymbols[i]);
	}
    }

  if (GTK_LABEL (widget)->wrap)
    source_add (data, "  gtk_label_set_line_wrap (GTK_LABEL (%s), TRUE);\n",
		data->wname);

  if (gtk_label_get_selectable (GTK_LABEL (widget)))
    source_add (data, "  gtk_label_set_selectable (GTK_LABEL (%s), TRUE);\n",
		data->wname);


  if (fabs (GTK_MISC (widget)->xalign - 0.5) > 0.0001
      || fabs (GTK_MISC (widget)->yalign - 0.5) > 0.0001)
    source_add (data, "  gtk_misc_set_alignment (GTK_MISC (%s), %g, %g);\n",
	 data->wname, GTK_MISC (widget)->xalign, GTK_MISC (widget)->yalign);

  if (GTK_MISC (widget)->xpad != 0 || GTK_MISC (widget)->ypad != 0)
    source_add (data, "  gtk_misc_set_padding (GTK_MISC (%s), %i, %i);\n",
	     data->wname, GTK_MISC (widget)->xpad, GTK_MISC (widget)->ypad);

  ellipsize_mode = gtk_label_get_ellipsize (GTK_LABEL (widget));
  if (ellipsize_mode != PANGO_ELLIPSIZE_NONE)
    {
      for (i = 0;
	   i < sizeof (GbEllipsizeValues) / sizeof (GbEllipsizeValues[0]);
	   i++)
	{
	  if (GbEllipsizeValues[i] == ellipsize_mode)
	    source_add (data,
			"  gtk_label_set_ellipsize (GTK_LABEL (%s), %s);\n",
			data->wname, GbEllipsizeSymbols[i]);
	}
    }

  width_chars = gtk_label_get_width_chars (GTK_LABEL (widget));
  if (width_chars != -1)
    {
      source_add (data,
		  "  gtk_label_set_width_chars (GTK_LABEL (%s), %i);\n",
		  data->wname, width_chars);
    }

  single_line_mode = gtk_label_get_single_line_mode (GTK_LABEL (widget));
  if (single_line_mode)
    {
      source_add (data,
		  "  gtk_label_set_single_line_mode (GTK_LABEL (%s), TRUE);\n",
		  data->wname);
    }

  angle = gtk_label_get_angle (GTK_LABEL (widget));
  if (angle > GLADE_EPSILON)
    {
      source_add (data,
		  "  gtk_label_set_angle (GTK_LABEL (%s), %g);\n",
		  data->wname, angle);
    }
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_label_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      const gchar *label_text = gtk_label_get_label (GTK_LABEL (widget));

      glade_util_get_translation_properties (widget, Label, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      if (gtk_label_get_use_underline (GTK_LABEL (widget)))
	source_add (data, "  %s = gtk_label_new_with_mnemonic (%s);\n",
		    data->wname,
		    source_make_string_full (label_text,
					     data->use_gettext && translatable,
					     context));
      else
	source_add (data, "  %s = gtk_label_new (%s);\n",
		    data->wname,
		    source_make_string_full (label_text,
					     data->use_gettext && translatable,
					     context));
    }

  gb_widget_write_standard_source (widget, data);

  gb_label_write_standard_source (widget, data,
				  Label, UseUnderline, UseMarkup,
				  Justify, Wrap, Selectable,
				  XAlign, YAlign, XPad, YPad,
				  FocusTarget, Ellipsize, WidthChars,
				  SingleLineMode, Angle);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_label_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_label_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = label_xpm;
  gbwidget.tooltip = _("Label");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_label_new;
  gbwidget.gb_widget_create_properties = gb_label_create_properties;
  gbwidget.gb_widget_get_properties = gb_label_get_properties;
  gbwidget.gb_widget_set_properties = gb_label_set_properties;
  gbwidget.gb_widget_write_source = gb_label_write_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_label_create_popup_menu;
 */

  return &gbwidget;
}
