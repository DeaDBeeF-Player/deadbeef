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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gladeconfig.h"

#ifdef USE_GNOME
#include <gnome.h>
#endif

#include "glade_clipboard.h"
#include "glade_palette.h"
#include "glade_project.h"
#include "glade_project_window.h"
#include "property.h"
#include "gbwidget.h"
#include "utils.h"
#include "editor.h"
#include "tree.h"
#include "gb.h"

/* The pixmap to use for placeholders */
#include "graphics/placeholder.xpm"

#define MIN_WIDGET_WIDTH		16
#define MIN_WIDGET_HEIGHT		16
#define MAX_INITIAL_WIDGET_WIDTH	300
#define MAX_INITIAL_WIDGET_HEIGHT	200
#define DEFAULT_WIDGET_WIDTH		50
#define DEFAULT_WIDGET_HEIGHT		50

#define PLACEHOLDER_WIDTH  16
#define PLACEHOLDER_HEIGHT 16


/* The grid (for fixed containers) */
#define GB_GRID_DOTS	1
#define GB_GRID_LINES	2
gboolean editor_show_grid = TRUE;
static gint editor_grid_horz_spacing = 8;
static gint editor_grid_vert_spacing = 8;
static gboolean editor_grid_style = GB_GRID_DOTS;


/* Snapping to the grid */
#define GB_SNAP_TOP	1 << 1
#define GB_SNAP_BOTTOM	1 << 2
#define GB_SNAP_LEFT	1 << 3
#define GB_SNAP_RIGHT	1 << 4
/*#define GB_SNAP_CENTER        1 << 5  maybe in future */
gboolean editor_snap_to_grid = TRUE;
static gint editor_snap_to_grid_x = GB_SNAP_LEFT | GB_SNAP_RIGHT;
static gint editor_snap_to_grid_y = GB_SNAP_TOP | GB_SNAP_BOTTOM;


/* Dragging (in a fixed container) - remembers which part of the widget is
   being dragged, the offset of the mouse (used when moving) and the initial
   widget rectangle (used when resizing) */
#define GB_DRAG_NONE    1
#define GB_TOP_LEFT     2
#define GB_TOP_RIGHT    3
#define GB_BOTTOM_LEFT  4
#define GB_BOTTOM_RIGHT 5
#define GB_MIDDLE       6
static gint drag_action;
static gboolean drag_has_pointer_grab = FALSE;
static GtkWidget *dragging_widget = NULL;
static gint drag_offset_x;
static gint drag_offset_y;
static gint drag_widget_x1, drag_widget_y1, drag_widget_x2, drag_widget_y2;

/* The list of selected widgets */
static GList *selected_widgets = NULL;

/* The cursors used when selecting/adding/moving/resizing widgets */
static GdkCursor *cursor_selector;
static GdkCursor *cursor_add_widget;
static GdkCursor *cursor_add_to_fixed;
static GdkCursor *cursor_move;
static GdkCursor *cursor_top_left;
static GdkCursor *cursor_top_right;
static GdkCursor *cursor_bottom_left;
static GdkCursor *cursor_bottom_right;

/* Struct only used for find_child_at callback */
typedef struct _GbFindChildAtData GbFindChildAtData;
struct _GbFindChildAtData
  {
    gint x;
    gint y;
    GtkWidget *found_child;
  };

/* Experimental code to allow typing labels for widgets when the mouse is
   over them. */
GtkWidget *mouse_over_widget = NULL;


/* Static functions */
static void editor_on_widget_realize (GtkWidget *widget,
				      gpointer data);

static gint editor_on_key_press_event (GtkWidget * widget,
				       GdkEventKey * event,
				       gpointer data);
static gint editor_on_key_release_event (GtkWidget * widget,
					 GdkEventKey * event,
					 gpointer data);

static void add_mouse_signals_recursive (GtkWidget *widget,
					 gpointer data);
static gint editor_on_button_press (GtkWidget * signal_widget,
				    GdkEventButton * event,
				    gpointer data);
static gint editor_on_button_release (GtkWidget * widget,
				      GdkEvent * event,
				      gpointer data);

static gboolean editor_check_ignore_event (GtkWidget *widget,
					   GdkEventAny *event);

static gint editor_on_motion_notify (GtkWidget * signal_widget,
				     GdkEventMotion * event,
				     gpointer data);
static gint editor_set_cursor (GtkWidget * signal_widget,
			       GdkEventMotion * event);
static gint editor_do_drag_action (GtkWidget * signal_widget,
				   GdkEventMotion * event);

static gint editor_on_enter_notify (GtkWidget        *widget,
				    GdkEventCrossing *event);
static gint editor_on_leave_notify (GtkWidget        *widget,
				    GdkEventCrossing *event);

static void placeholder_replace (GtkWidget * placeholder);
static void placeholder_finish_replace (GtkWidget * new_widget,
					GbWidgetNewData * data);
static void on_placeholder_destroy (GtkWidget * widget,
				    gpointer data);
static void on_placeholder_size_request (GtkWidget * widget,
					 GtkRequisition *requisition,
					 gpointer data);

static void add_widget_to_fixed (GtkWidget * parent,
				 gint x,
				 gint y);
static void add_widget_to_fixed_finish (GtkWidget * new_widget,
					GbWidgetNewData * data);
#if GLADE_SUPPORTS_GTK_PACKER
static void add_widget_to_container (GtkWidget * parent);
static void add_widget_to_container_finish (GtkWidget * new_widget,
					    GbWidgetNewData * data);
#endif
static gint expose_widget (GtkWidget * widget,
			   GdkEventExpose * event,
			   GladeWidgetData * wdata);
static void draw_grid (GtkWidget * widget);
static void paint_widget (GtkWidget * widget, GdkEventExpose *event);
static void clear_child_windows (GdkWindow * window,
				 gint x,
				 gint y,
				 gint w,
				 gint h);
static gint editor_idle_handler (GdkWindow *expose_win);

static void delete (GtkWidget * widget);
static void delete_placeholder (GtkWidget * widget);

static void on_grid_settings_response (GtkWidget * widget,
				       gint response_id,
				       gpointer data);
static void editor_redraw_component (GtkWidget * widget,
				     gpointer data);

static void on_snap_settings_response (GtkWidget * widget,
				       gint response_id,
				       gpointer data);
static gint snap_top_edge (gint y);
static gint snap_bottom_edge (gint y);
static gint snap_left_edge (gint x);
static gint snap_right_edge (gint x);

static gint get_position_in_widget (GtkWidget * widget,
				    gint x,
				    gint y);
static void raise_fixed_child (GtkWidget * widget);

static void find_child_at (GtkWidget * widget,
			   GbFindChildAtData * data);


#if 0
/* This is probably what we should have used to get all button presses and
   releases on widget etc. But it may be too awkward to change now.
   Note that we'll also get all signals for Glade's own widgets.

   Actually I don't think this would help that much, since emission hooks
   are run after RUN_FIRST class handlers, and you can't stop a signal from
   them.

   So we'd have to substitute a Glade function for all 'button_press' class
   functions, and only call the original class functions when appropriate.
   We'd still need to be able to stop signal handlers. Hmm. */
static gboolean
button_press_event_hook (GSignalInvocationHint *ihint,
			 guint                  n_param_values,
			 const GValue          *param_values,
			 gpointer               data)
{
  GtkWidget *widget;
  GdkEventButton *event;

  widget = g_value_get_object (param_values + 0);
  event = g_value_get_boxed (param_values + 1);

  g_print ("Watch: \"%s\" emitted for %s at %g,%g\n",
	   gtk_signal_name (ihint->signal_id),
	   gtk_type_name (GTK_OBJECT_TYPE (widget)),
	   event->x, event->y);

  /* Test: try to stop the emission. Can't! */
  /*g_signal_stop_emission (widget, ihint->signal_id, 0);*/


  /* If we return FALSE our emission hook is removed. */
  return TRUE;
}


static gboolean
button_release_event_hook (GSignalInvocationHint *ihint,
			   guint                  n_param_values,
			   const GValue          *param_values,
			   gpointer               data)
{
  GtkWidget *widget;
  GdkEvent *event;

  widget = g_value_get_object (param_values + 0);
  event = g_value_get_boxed (param_values + 1);

  g_print ("Watch: \"%s\" emitted for %s type:%i\n",
	   gtk_signal_name (ihint->signal_id),
	   gtk_type_name (GTK_OBJECT_TYPE (widget)),
	   event->type);

  /* If we return FALSE our emission hook is removed. */
  return TRUE;
}
#endif

void
editor_init ()
{
  /* Create all cursors needed */
  cursor_selector = gdk_cursor_new (GDK_TOP_LEFT_ARROW);
  cursor_add_widget = gdk_cursor_new (GDK_PLUS);
  cursor_add_to_fixed = gdk_cursor_new (GDK_TCROSS);
  cursor_move = gdk_cursor_new (GDK_FLEUR);
  cursor_top_left = gdk_cursor_new (GDK_TOP_LEFT_CORNER);
  cursor_top_right = gdk_cursor_new (GDK_TOP_RIGHT_CORNER);
  cursor_bottom_left = gdk_cursor_new (GDK_BOTTOM_LEFT_CORNER);
  cursor_bottom_right = gdk_cursor_new (GDK_BOTTOM_RIGHT_CORNER);

#if 0
 {
   guint button_press_signal_id, button_release_signal_id;

   button_press_signal_id = gtk_signal_lookup ("button_press_event",
					       GTK_TYPE_WIDGET);
   g_signal_add_emission_hook (button_press_signal_id, 0,
			       button_press_event_hook, NULL, NULL);
   
   button_release_signal_id = gtk_signal_lookup ("button_release_event",
						 GTK_TYPE_WIDGET);
   g_signal_add_emission_hook (button_release_signal_id, 0,
			       button_release_event_hook, NULL, NULL);
 }
#endif
}

gint
editor_close_window (GtkWidget * widget,
		     GdkEvent * event,
		     gpointer data)
{
  glade_util_close_window (widget);
  return TRUE;
}


/*
 * Grid settings dialog
 */
gboolean
editor_get_show_grid ()
{
  return editor_show_grid;
}


void
editor_set_show_grid (gboolean show)
{
  if (editor_show_grid == show)
    return;

  editor_show_grid = show;
  if (current_project != NULL)
    glade_project_foreach_component (current_project, editor_redraw_component,
				     NULL);
}


void
editor_show_grid_settings_dialog (GtkWidget *widget)
{
  GtkWidget *dialog, *vbox, *table, *label, *button, *spinbutton;
  GtkObject *adjustment;
  GSList *group;
  GtkWindow *transient_parent;

  transient_parent = (GtkWindow*) glade_util_get_toplevel (widget);
  dialog = gtk_dialog_new_with_buttons (_("Grid Options"), transient_parent, 0,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "grid_options", "Glade");

  vbox = GTK_DIALOG (dialog)->vbox;

  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 5);
  gtk_widget_show (table);

  label = gtk_label_new (_("Horizontal Spacing:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 5, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (editor_grid_horz_spacing, 1, 1000, 1,
				   10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "spinbutton1",
					   GTK_ADJUSTMENT (adjustment), 0, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 5, 5);
  gtk_widget_show (spinbutton);
  gtk_widget_grab_focus (spinbutton);

  label = gtk_label_new (_("Vertical Spacing:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_FILL, 0, 5, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (editor_grid_vert_spacing, 1, 1000, 1,
				   10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "spinbutton2",
					   GTK_ADJUSTMENT (adjustment), 0, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 5, 5);
  gtk_widget_show (spinbutton);

  table = gtk_table_new (1, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  label = gtk_label_new (_("Grid Style:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 5, 5);
  gtk_widget_show (label);

  button = gtk_radio_button_new_with_label (NULL, _("Dots"));
  if (editor_grid_style == GB_GRID_DOTS)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 5, 5);
  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (dialog), "button1", button);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

  button = gtk_radio_button_new_with_label (group, _("Lines"));
  if (editor_grid_style == GB_GRID_LINES)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 5, 1);
  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (dialog), "button2", button);

  gtk_signal_connect (GTK_OBJECT (dialog), "response",
		      GTK_SIGNAL_FUNC (on_grid_settings_response),
		      NULL);

  gtk_widget_show (dialog);
}


static void
on_grid_settings_response (GtkWidget * widget, gint response_id, gpointer data)
{
  GtkWidget *dialog, *spinbutton, *button;

  dialog = gtk_widget_get_toplevel (widget);

  if (response_id != GTK_RESPONSE_OK)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  spinbutton = gtk_object_get_data (GTK_OBJECT (dialog), "spinbutton1");
  editor_grid_horz_spacing = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (spinbutton));
  spinbutton = gtk_object_get_data (GTK_OBJECT (dialog), "spinbutton2");
  editor_grid_vert_spacing = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (spinbutton));

  button = gtk_object_get_data (GTK_OBJECT (dialog), "button1");
  if (GTK_TOGGLE_BUTTON (button)->active)
    editor_grid_style = GB_GRID_DOTS;
  else
    editor_grid_style = GB_GRID_LINES;

  gtk_widget_destroy (dialog);

  /* redraw all windows */
  if (current_project != NULL)
    glade_project_foreach_component (current_project, editor_redraw_component,
				     NULL);
}


static void
editor_redraw_component (GtkWidget * widget, gpointer data)
{
  gtk_widget_queue_draw (widget);
}


/*
 * Snap settings
 */
gboolean
editor_get_snap_to_grid ()
{
  return editor_snap_to_grid;
}


void
editor_set_snap_to_grid (gboolean snap)
{
  editor_snap_to_grid = snap;
}


void
editor_show_snap_settings_dialog (GtkWidget *widget)
{
  GtkWidget *dialog, *vbox, *table, *label, *button;
  GtkWindow *transient_parent;

  transient_parent = (GtkWindow*) glade_util_get_toplevel (widget);
  dialog = gtk_dialog_new_with_buttons (_("Snap Options"), transient_parent, 0,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "snap_options", "Glade");

  vbox = GTK_DIALOG (dialog)->vbox;

  table = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 5);
  gtk_widget_show (table);

  /* Horizontal snapping */
  label = gtk_label_new (_("Horizontal Snapping:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 5, 5);
  gtk_widget_show (label);

  button = gtk_check_button_new_with_label (_("Left"));
  if (editor_snap_to_grid_x & GB_SNAP_LEFT)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 5, 5);
  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (dialog), "button1", button);
  gtk_widget_grab_focus (button);

  button = gtk_check_button_new_with_label (_("Right"));
  if (editor_snap_to_grid_x & GB_SNAP_RIGHT)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 5, 5);
  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (dialog), "button2", button);

  /* Vertical snapping */
  label = gtk_label_new (_("Vertical Snapping:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_FILL, 0, 5, 5);
  gtk_widget_show (label);

  button = gtk_check_button_new_with_label (_("Top"));
  if (editor_snap_to_grid_y & GB_SNAP_TOP)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_FILL, 0, 5, 5);
  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (dialog), "button3", button);

  button = gtk_check_button_new_with_label (_("Bottom"));
  if (editor_snap_to_grid_y & GB_SNAP_BOTTOM)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 3, 4,
		    GTK_EXPAND | GTK_FILL, 0, 5, 5);
  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (dialog), "button4", button);

  gtk_signal_connect (GTK_OBJECT (dialog), "response",
		      GTK_SIGNAL_FUNC (on_snap_settings_response),
		      NULL);

  gtk_widget_show (dialog);
}


static void
on_snap_settings_response (GtkWidget * widget, gint response_id, gpointer data)
{
  GtkWidget *dialog, *button;

  dialog = gtk_widget_get_toplevel (widget);

  if (response_id != GTK_RESPONSE_OK)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  editor_snap_to_grid_x = 0;
  editor_snap_to_grid_y = 0;
  button = gtk_object_get_data (GTK_OBJECT (dialog), "button1");
  if (GTK_TOGGLE_BUTTON (button)->active)
    editor_snap_to_grid_x |= GB_SNAP_LEFT;
  button = gtk_object_get_data (GTK_OBJECT (dialog), "button2");
  if (GTK_TOGGLE_BUTTON (button)->active)
    editor_snap_to_grid_x |= GB_SNAP_RIGHT;
  button = gtk_object_get_data (GTK_OBJECT (dialog), "button3");
  if (GTK_TOGGLE_BUTTON (button)->active)
    editor_snap_to_grid_y |= GB_SNAP_TOP;
  button = gtk_object_get_data (GTK_OBJECT (dialog), "button4");
  if (GTK_TOGGLE_BUTTON (button)->active)
    editor_snap_to_grid_y |= GB_SNAP_BOTTOM;

  gtk_widget_destroy (dialog);
}


static gint
snap_top_edge (gint y)
{
  if (editor_snap_to_grid && (editor_snap_to_grid_y & GB_SNAP_TOP))
    {
      y += editor_grid_vert_spacing / 2;
      y -= y % editor_grid_vert_spacing;
    }
  return y;
}


static gint
snap_bottom_edge (gint y)
{
  if (editor_snap_to_grid && (editor_snap_to_grid_y & GB_SNAP_BOTTOM))
    {
      y += editor_grid_vert_spacing / 2;
      y -= y % editor_grid_vert_spacing;
    }
  return y;
}


static gint
snap_left_edge (gint x)
{
  if (editor_snap_to_grid && (editor_snap_to_grid_x & GB_SNAP_LEFT))
    {
      x += editor_grid_horz_spacing / 2;
      x -= x % editor_grid_horz_spacing;
    }
  return x;
}


static gint
snap_right_edge (gint x)
{
  if (editor_snap_to_grid && (editor_snap_to_grid_x & GB_SNAP_RIGHT))
    {
      x += editor_grid_horz_spacing / 2;
      x -= x % editor_grid_horz_spacing;
    }
  return x;
}




static void
on_placeholder_size_allocate (GtkWidget * widget,
			      GtkAllocation *allocation,
			      gpointer data)
{
#if 0
  g_print ("In on_placeholder_size_allocate %i,%i %ix%i\n",
	   allocation->x, allocation->y,
	   allocation->width, allocation->height);
#endif
}


GtkWidget *
editor_new_placeholder ()
{
  GtkWidget *placeholder = gtk_drawing_area_new ();
  gtk_widget_set_events (placeholder,
			 gtk_widget_get_events (placeholder)
			 | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK | GDK_BUTTON1_MOTION_MASK);
  gtk_widget_set_usize (placeholder, PLACEHOLDER_WIDTH, PLACEHOLDER_HEIGHT);
  gtk_widget_show (placeholder);
  editor_add_draw_signals (placeholder);
  editor_add_mouse_signals (placeholder);
  gtk_object_set_data (GTK_OBJECT (placeholder), GB_PLACEHOLDER_KEY, "True");

  /* GnomeDock workaround. */
  gtk_signal_connect_after (GTK_OBJECT (placeholder), "size_request",
			    GTK_SIGNAL_FUNC (on_placeholder_size_request),
			    NULL);

  /* FIXME: Just a test. */
  gtk_signal_connect_after (GTK_OBJECT (placeholder), "size_allocate",
			    GTK_SIGNAL_FUNC (on_placeholder_size_allocate),
			    NULL);


  gtk_signal_connect (GTK_OBJECT (placeholder), "destroy",
		      GTK_SIGNAL_FUNC (on_placeholder_destroy), NULL);
  return placeholder;
}


static void
on_placeholder_destroy (GtkWidget * widget,
			gpointer data)
{
  MSG ("IN on_placeholder_destroy");
  /* If the entire project is being destroyed, we don't need to update the
     selection or the widget tree. */

  /* FIXME: GTK+2 */
  if (!(GTK_OBJECT_FLAGS (current_project) & GTK_IN_DESTRUCTION))
    editor_remove_widget_from_selection (widget);
}


/* This is a gnome-libs 1.0.5 bug workaround - GnomeDockItem doesn't use
   the child requisition properly. */
static void
on_placeholder_size_request (GtkWidget * widget,
			     GtkRequisition *requisition,
			     gpointer data)
{
  MSG ("IN on_placeholder_size_request");
#ifdef USE_GNOME
  if (BONOBO_IS_DOCK_ITEM (widget->parent))
    {
      requisition->width = PLACEHOLDER_WIDTH;
      requisition->height = PLACEHOLDER_HEIGHT;
    }
#endif
}


static void
placeholder_replace (GtkWidget * placeholder)
{
  char *class_name;

  class_name = glade_palette_get_widget_class (GLADE_PALETTE (glade_palette));
  g_return_if_fail (class_name != NULL);
  glade_palette_reset_selection (GLADE_PALETTE (glade_palette), TRUE);
  gb_widget_new_full (class_name, TRUE, placeholder->parent, placeholder, 0, 0,
		      placeholder_finish_replace, GB_CREATING, NULL);
}


static void
placeholder_finish_replace (GtkWidget * new_widget, GbWidgetNewData * data)
{
  editor_clear_selection (NULL);

  /* GtkToolItem widgets can only be added into toolbars and cause problems
     inside other containers, so check that here. Note that we have to use
     special tool items for placeholders so data->parent is usually a
     GtkToolItem rather than a GtkToolbar. */
  if (GTK_IS_TOOL_ITEM (new_widget)
      && (!data->parent || (!GTK_IS_TOOLBAR (data->parent)
			    && !GTK_IS_TOOL_ITEM (data->parent))))
    {
      glade_util_show_message_box (_("GtkToolItem widgets can only be added to a GtkToolbar."), data->parent);
      gtk_widget_destroy (new_widget);
      return;
    }

  /* If a scrollable widget is added, we automatically add a parent scrolled
     window, if there isn't one already. */
  if (GTK_WIDGET_CLASS (G_OBJECT_GET_CLASS (new_widget))->set_scroll_adjustments_signal)
    {
      if (data->parent && !GTK_IS_SCROLLED_WINDOW (data->parent))
	{
	  GtkWidget *scrolledwin;

	  scrolledwin = gb_widget_new ("GtkScrolledWindow", data->parent);
	  if (!gb_widget_replace_child (data->parent, data->current_child,
					scrolledwin))
	    {
	      glade_util_show_message_box (_("Couldn't insert a GtkScrolledWindow widget."), data->parent);
	      gtk_widget_destroy (scrolledwin);
	      gtk_widget_destroy (new_widget);
	      return;
	    }

	  if (GTK_BIN (scrolledwin)->child)
	    gtk_container_remove (GTK_CONTAINER (scrolledwin),
				  GTK_BIN (scrolledwin)->child);
	  gtk_container_add (GTK_CONTAINER (scrolledwin), new_widget);

	  /* Set the shadow to In, since that is normal. */
	  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwin),
					       GTK_SHADOW_IN);

	  /* Set both scrollbar policies to automatic for GtkIconView. */
	  if (GTK_IS_ICON_VIEW (new_widget))
	    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
					    GTK_POLICY_AUTOMATIC,
					    GTK_POLICY_AUTOMATIC);

	  /* GtkText doesn't support horizontal scrolling, so we may as well
	     get rid of the scrollbar. */
#if GLADE_SUPPORTS_GTK_TEXT
	  if (GTK_IS_TEXT (new_widget))
	    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
					    GTK_POLICY_NEVER,
					    GTK_POLICY_ALWAYS);
#endif

	  tree_add_widget (scrolledwin);
	  tree_add_widget (new_widget);
	  gb_widget_show_properties (new_widget);
	  glade_project_set_changed (current_project, TRUE);
	  return;
	}
    }
  /* If a non-scrollable widget is added to a GtkScrolledWindow, we add a
     viewport automatically. */
  else if (data->parent && GTK_IS_SCROLLED_WINDOW (data->parent))
    {
      GtkWidget *viewport;

      viewport = gb_widget_new ("GtkViewport", data->parent);
      if (!gb_widget_replace_child (data->parent, data->current_child,
				    viewport))
	{
	  glade_util_show_message_box (_("Couldn't insert a GtkViewport widget."), data->parent);
	  gtk_widget_destroy (viewport);
	  gtk_widget_destroy (new_widget);
	  return;
	}

      if (GTK_BIN (viewport)->child)
	gtk_container_remove (GTK_CONTAINER (viewport),
			      GTK_BIN (viewport)->child);
      gtk_container_add (GTK_CONTAINER (viewport), new_widget);

      tree_add_widget (viewport);
      tree_add_widget (new_widget);
      gb_widget_show_properties (new_widget);
      glade_project_set_changed (current_project, TRUE);
      return;
    }

  /* Replace placeholder in parent container */
  if (gb_widget_replace_child (data->parent, data->current_child, new_widget))
    {
      gb_widget_show_properties (new_widget);
      tree_add_widget (new_widget);
      glade_project_set_changed (current_project, TRUE);
    }
  else
    {
      glade_util_show_message_box (_("Couldn't add new widget."),
				   data->parent);

      /* I think we can safely destroy the new widget. */
      gtk_widget_destroy (new_widget);
    }
}





/* Note: returns last found child if children overlap */
static void
find_child_at (GtkWidget * widget, GbFindChildAtData * data)
{
#if 0
  g_print ("In find_child_at: %s X:%i Y:%i W:%i H:%i\n",
	   gtk_widget_get_name (widget),
	   widget->allocation.x, widget->allocation.y,
	   widget->allocation.width, widget->allocation.height);
#endif
  /* Notebook pages are visible but not mapped if they are not showing. */
  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget)
      && (widget->allocation.x <= data->x)
      && (widget->allocation.y <= data->y)
      && (widget->allocation.x + widget->allocation.width > data->x)
      && (widget->allocation.y + widget->allocation.height > data->y))
    {
#if 0
      g_print ("found child:%s", gtk_widget_get_name (widget));
#endif
      data->found_child = widget;
    }
}


/* Checks if point is in the given notebook's tabs. */
static void
find_notebook_tab (GtkWidget * widget, GbFindChildAtData * data)
{
  gint nchildren, i;
  GtkWidget *child_page, *child_tab;

  /* FIXME: Check this works. I had to change it for GTK+ 2. */
  nchildren = g_list_length (GTK_NOTEBOOK (widget)->children);
  for (i = 0; i < nchildren; i++)
    {
      child_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (widget), i);
      child_tab = gtk_notebook_get_tab_label (GTK_NOTEBOOK (widget),
					      child_page);
      if (child_tab
	  && (child_tab->allocation.x <= data->x)
	  && (child_tab->allocation.y <= data->y)
	  && (child_tab->allocation.x + child_tab->allocation.width > data->x)
	  && (child_tab->allocation.y + child_tab->allocation.height > data->y))
	{
	  /* Make sure this is a GbWidget. */
	  /*if (GB_IS_GB_WIDGET (child_tab))*/
	    data->found_child = child_tab;
	}
    }
}


/* This function is passed a widget which has received a mouse event, and
   the coordinates of that event. It returns the widget which the event is
   really meant for (which could be a descendent of the given widget), and
   the position of the event in the widget's allocated area. */
static GtkWidget *
editor_get_event_widget (GtkWidget *widget, GdkWindow *window, gint x, gint y,
			 gint * x_return, gint * y_return)
{
  GbFindChildAtData data;
  gint win_x, win_y, saved_x, saved_y;
  GtkWidget *found_gbwidget = NULL, *saved_widget;
  gint found_x = 0, found_y = 0;
  GdkWindow *parent_window = NULL;

#if 0
  g_print ("\n\nOriginal:%s X:%i Y:%i\n", gtk_widget_get_name (widget), x, y);
  if (widget->parent)
    g_print ("  Parent: %s\n", gtk_widget_get_name (widget->parent));
#endif

  /* FIXME: GTK bug workaround? - need to translate coords if mouse button was
     pressed in a child window. */
  /* Remember widgets can have other windows besides their main one, and
     when dragging the event may be sent to the parent's window? */
  /* SPECIAL CODE: GnomeDockItem widgets which are floating use a different
     window. */
  if (widget->parent)
    {
      if (GTK_IS_LAYOUT (widget->parent))
	parent_window = GTK_LAYOUT (widget->parent)->bin_window;

#ifdef USE_GNOME
      if (BONOBO_IS_DOCK_ITEM (widget)
	       && BONOBO_DOCK_ITEM (widget)->is_floating)
	{
	  parent_window = BONOBO_DOCK_ITEM (widget)->float_window;
	}
      else if (BONOBO_IS_DOCK_ITEM (widget->parent)
	       && BONOBO_DOCK_ITEM (widget->parent)->is_floating)
	{
	  parent_window = BONOBO_DOCK_ITEM (widget->parent)->float_window;
	}
#endif
    }

  if (!parent_window)
    parent_window = widget->parent ? widget->parent->window : widget->window;

  while (window && window != parent_window)
    {
      gdk_window_get_position (window, &win_x, &win_y);
#if 0
      g_print ("  adding X:%i Y:%i\n", win_x, win_y);
#endif
      x += win_x;
      y += win_y;
      window = gdk_window_get_parent (window);
    }
  if (window != parent_window)
    {
      MSG ("  editor_get_event_widget - unknown window");
      return NULL;
    }

#if 0
  g_print ("  Translated X:%i Y:%i\n", x, y);
#endif

  /* We now have correct coordinates relative to the parent's window,
     i.e. in the same coordinate space as the widget's allocation.
     Now we find out which widget this event is really for.
     We step down the widget tree, trying to find the widget at the given
     position. We have to translate coordinates for children of widgets with
     windows. We may need to use bin_window for viewport. */
  if (GB_IS_GB_WIDGET (widget) || GB_IS_PLACEHOLDER (widget))
    {
#if 0
      g_print ("Found child:%s\n", gtk_widget_get_name (widget));
#endif
      found_gbwidget = widget;
      found_x = x;
      found_y = y;
    }

  /* Save the widget and the x & y coords. */
  saved_widget = widget;
  saved_x = x;
  saved_y = y;

  /* Now we want to convert the coordinates into the widget's childrens'
     coordinate space, so if the widget has a window, we have to subtract the
     position of it (since the child coordinates are relative to that).
     Viewports need special treatment. */
  if (!GTK_WIDGET_NO_WINDOW (widget) && widget->parent)
    {
      gdk_window_get_position (widget->window, &win_x, &win_y);
      x -= win_x;
      y -= win_y;

      /* SPECIAL CODE: need to translate to bin_window for a viewport. */
      if (GTK_IS_VIEWPORT (widget))
	{
	  gdk_window_get_position (GTK_VIEWPORT (widget)->bin_window,
				   &win_x, &win_y);
	  x -= win_x;
	  y -= win_y;
	}

      /* SPECIAL CODE: need to translate to bin_window for a layout. */
      if (GTK_IS_LAYOUT (widget))
	{
	  gdk_window_get_position (GTK_LAYOUT (widget)->bin_window,
				   &win_x, &win_y);
	  x -= win_x;
	  y -= win_y;
	}
#if 0
      g_print ("  Translated X:%i Y:%i\n", x, y);
#endif
    }

  for (;;)
    {
      if (!GTK_IS_CONTAINER (widget) || GTK_IS_MENU_BAR (widget))
	break;
      data.x = x;
      data.y = y;
      data.found_child = NULL;
#if 0
      g_print ("...Finding child widget\n");
#endif
      gtk_container_forall (GTK_CONTAINER (widget),
			    (GtkCallback) find_child_at, &data);
      /* SPECIAL CODE - Check for notebook tabs. */
      if (GTK_IS_NOTEBOOK (widget))
	find_notebook_tab (widget, &data);

      if (data.found_child)
	{
#if 0
	  g_print ("Found child:%s\n", gtk_widget_get_name (data.found_child));
#endif
	  widget = data.found_child;
	  if (GB_IS_GB_WIDGET (widget) || GB_IS_PLACEHOLDER (widget))
	    {
	      found_gbwidget = widget;
	      found_x = x;
	      found_y = y;
	    }
	}
      else
	break;

      if (!GTK_WIDGET_NO_WINDOW (widget))
	{
	  gdk_window_get_position (widget->window, &win_x, &win_y);
	  x -= win_x;
	  y -= win_y;

	  /* SPECIAL CODE: need to translate to bin_window for a viewport. */
	  if (GTK_IS_VIEWPORT (widget))
	    {
	      gdk_window_get_position (GTK_VIEWPORT (widget)->bin_window,
				       &win_x, &win_y);
	      x -= win_x;
	      y -= win_y;
	    }
#if 0
	  g_print ("  Translated X:%i Y:%i\n", x, y);
#endif
	}
    }

  /* If we haven't found a GbWidget yet, try moving up the hierarchy. */
  widget = saved_widget;
  x = saved_x;
  y = saved_y;
  while (!found_gbwidget && widget->parent)
    {
      widget = widget->parent;
#if 0
      g_print ("  Trying parent: %s X:%i Y:%i\n",
	       gtk_widget_get_name (widget), x, y);
#endif
      if (GB_IS_GB_WIDGET (widget) || GB_IS_PLACEHOLDER (widget))
	{
	  found_gbwidget = widget;
	  found_x = x;
	  found_y = y;
	}
      else if (!GTK_WIDGET_NO_WINDOW (widget))
	{
	  gdk_window_get_position (widget->window, &win_x, &win_y);
	  x += win_x;
	  y += win_y;

	  /* SPECIAL CODE; use bin_window for viewport. */
	  if (GTK_IS_VIEWPORT (widget))
	    {
	      gdk_window_get_position (GTK_VIEWPORT (widget)->bin_window,
				       &win_x, &win_y);
	      x += win_x;
	      y += win_y;
	    }
#if 0
	  g_print ("  Translated X:%i Y:%i\n", x, y);
#endif
	}
    }

  if (!found_gbwidget)
    return NULL;

  *x_return = found_x - found_gbwidget->allocation.x;
  *y_return = found_y - found_gbwidget->allocation.y;

#if 0
  g_print ("  Event widget: %s X:%i Y:%i\n",
	   gtk_widget_get_name (found_gbwidget), *x_return, *y_return);
#endif

  return found_gbwidget;
}


/* We use the "event" signal since it is emitted before the "button_press"
   or "button_release" signal is emitted. This means we can catch it and
   stop the normal signal handlers. */
static gint
editor_on_event (GtkWidget * signal_widget,
		 GdkEvent * event,
		 gpointer user_data)
{
#if 0
  g_print ("In editor_on_event widget: %s\n",
	   gtk_widget_get_name (signal_widget));
#endif

  if (event->type == GDK_BUTTON_PRESS
      || event->type == GDK_2BUTTON_PRESS
      || event->type == GDK_3BUTTON_PRESS)
    return editor_on_button_press (signal_widget, (GdkEventButton*) event,
				   user_data);

  if (event->type == GDK_BUTTON_RELEASE)
    return editor_on_button_release (signal_widget, event, user_data);

  return FALSE;
}


static gint
editor_on_button_press (GtkWidget * signal_widget,
			GdkEventButton * event,
			gpointer user_data)
{
  GtkWidget *widget;
  gint x, y;

#if 0
  g_print ("In editor_on_button_press widget: %s\n",
	   gtk_widget_get_name (signal_widget));
#endif
  widget = editor_get_event_widget (signal_widget, event->window,
				    event->x, event->y, &x, &y);
  if (widget == NULL)
    return FALSE;

  if (editor_check_ignore_event (widget, (GdkEventAny*) event))
    return FALSE;

  MSG ("...Checking which button pressed");
  /* We only want single button press events. */
  if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
      if (glade_palette_is_selector_on (GLADE_PALETTE (glade_palette)))
	{
	  gboolean handled;
	  MSG ("...Selecting widget");
	  handled = editor_select_widget (widget, event, x, y);
#if 0
	  if (handled)
	    gtk_signal_emit_stop_by_name (GTK_OBJECT (signal_widget),
					  "button_press_event");
#endif
	  return handled;
	}
#ifdef USE_GNOME
      else if (GTK_IS_FIXED (widget)
	       || (GTK_IS_LAYOUT (widget) && !GNOME_IS_CANVAS (widget)))
#else
      else if (GTK_IS_FIXED (widget) || GTK_IS_LAYOUT (widget))
#endif
	{
	  add_widget_to_fixed (widget, x, y);
#if 0
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (signal_widget),
					"button_press_event");
#endif
	  return TRUE;
	}
      else if (GB_IS_PLACEHOLDER (widget))
	{
	  placeholder_replace (widget);
#if 0
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (signal_widget),
					"button_press_event");
#endif
	  return TRUE;
	}
#if GLADE_SUPPORTS_GTK_PACKER
      else if (GTK_IS_PACKER (widget))
	{
	  add_widget_to_container (widget);
#if 0
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (signal_widget),
					"button_press_event");
#endif
	  return TRUE;
	}
#endif
      else
        {
         static gboolean already_shown = FALSE;
         if (already_shown) 
           {
             /* Beep if user does mistake of invalid positioning from second
              * time.
              */
             gdk_beep ();	
           }
         else
           {
             glade_util_show_message_box (_("You can't add a widget at the selected position.\n"
					    "\n"
					    "Tip: GTK+ uses containers to lay out widgets.\n"
					    "Try deleting the existing widget and using\n"
					    "a box or table container instead.\n"), widget);
             already_shown = TRUE;
           }
        }
    }
  else if (event->button == 3)
    {
      gb_widget_show_popup_menu (widget, event);
#if 0
      gtk_signal_emit_stop_by_name (GTK_OBJECT (signal_widget),
				    "button_press_event");
#endif
      return TRUE;
    }
  return FALSE;
}


static gint
editor_on_button_release (GtkWidget * widget,
			  GdkEvent * event,
			  gpointer data)
{
  MSG ("In editor_on_button_release");
  if (drag_has_pointer_grab)
    {
#if 0
      g_print ("###### Releasing pointer grab\n");
#endif
      gdk_pointer_ungrab (event->button.time);
      drag_has_pointer_grab = FALSE;
    }

  if (dragging_widget)
    {
      drag_action = GB_DRAG_NONE;
      MSG ("  removing grab");
      gtk_grab_remove (dragging_widget);
      dragging_widget = NULL;
    }
  return FALSE;
}


/* This adds a new widget to a GtkFixed or GtkLayout container. */
static void
add_widget_to_fixed (GtkWidget * parent, gint x, gint y)
{
  char *class_name;

  class_name = glade_palette_get_widget_class (GLADE_PALETTE (glade_palette));
  g_return_if_fail (class_name != NULL);
  glade_palette_reset_selection (GLADE_PALETTE (glade_palette), TRUE);
  gb_widget_new_full (class_name, TRUE, parent, NULL, x, y,
		      add_widget_to_fixed_finish, GB_CREATING, NULL);
}


/* This finishes off adding a new widget to a GtkFixed or GtkLayout container,
   possibly after a dialog has been shown to set some initial properties of
   the new widget (e.g. number of rows for a new GtkCList). */
static void
add_widget_to_fixed_finish (GtkWidget * new_widget, GbWidgetNewData * data)
{
  GtkRequisition requisition = {0, 0};
  GladeWidgetData *wdata;
  GtkWidget *parent = data->parent;
  gint x = data->x;
  gint y = data->y;
  gint w, h;

  g_return_if_fail (parent != NULL);
  g_return_if_fail (GTK_IS_FIXED (parent) || GTK_IS_LAYOUT (parent));

  /* I think we should do this first, in case the widget needs to be
     realized in order to calculate its requisition. */
  gtk_widget_hide (new_widget);

  if (GTK_IS_FIXED (data->parent))
    {
      gtk_fixed_put (GTK_FIXED (data->parent), new_widget, x, y);
      /*gtk_widget_set_uposition (new_widget, x, y);*/
    }
  else if (GTK_IS_LAYOUT (data->parent))
    {
      GtkLayout *layout = GTK_LAYOUT (data->parent);
      x += layout->hadjustment->value;
      y += layout->vadjustment->value;
      gtk_layout_put (layout, new_widget, x, y);
      /*gtk_widget_set_uposition (new_widget, x, y);*/
    }


  gtk_widget_size_request (new_widget, &requisition);
  w = requisition.width;
  h = requisition.height;
  if (w < MIN_WIDGET_WIDTH || h < MIN_WIDGET_HEIGHT)
    {
      MSG3 ("Default widget size too small: %s W:%i H:%i\n",
	    gtk_widget_get_name (new_widget), w, h);
    }
  if (w > MAX_INITIAL_WIDGET_WIDTH || h > MAX_INITIAL_WIDGET_HEIGHT)
    {
      MSG3 ("Default widget size too big: %s W:%i H:%i\n",
	    gtk_widget_get_name (new_widget), w, h);
    }

  if (w == 0)
    w = DEFAULT_WIDGET_WIDTH;
  if (h == 0)
    h = DEFAULT_WIDGET_HEIGHT;
  w = MAX (w, MIN_WIDGET_WIDTH);
  w = MIN (w, MAX_INITIAL_WIDGET_WIDTH);
  h = MAX (h, MIN_WIDGET_HEIGHT);
  h = MIN (h, MAX_INITIAL_WIDGET_HEIGHT);

  /* FIXME: Make sure a gamma curve is a reasonable size. */
  if (GTK_IS_GAMMA_CURVE (new_widget))
    w = 80;

  /* FIXME: Make sure a clist/ctree or notebook is a reasonable size. */
  if (GTK_IS_CLIST (new_widget) || GTK_IS_NOTEBOOK (new_widget))
    {
      w = 200;
      h = 100;
    }

  /* Children of fixed widgets always have their x, y, width & height set
     explicitly. */
  wdata = gtk_object_get_data (GTK_OBJECT (new_widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);
  wdata->flags |= GLADE_WIDTH_SET | GLADE_HEIGHT_SET;

  /* Calculate real position & size according to grid & snapping */
  /* FIXME: only snaps top & left at present */
  if (editor_snap_to_grid)
    {
      if (editor_snap_to_grid_x & GB_SNAP_LEFT)
	{
	  /*x += editor_grid_horz_spacing / 2; */
	  x -= x % editor_grid_horz_spacing;
	}
      if (editor_snap_to_grid_y & GB_SNAP_TOP)
	{
	  /*y += editor_grid_vert_spacing / 2; */
	  y -= y % editor_grid_vert_spacing;
	}
    }

  gb_widget_set_usize (new_widget, w, h);
  wdata->width = w;
  wdata->height = h;

  if (GTK_IS_FIXED (data->parent))
    {
      gtk_fixed_move (GTK_FIXED (data->parent), new_widget, x, y);
      /*gtk_widget_set_uposition (new_widget, x, y);*/
    }
  else if (GTK_IS_LAYOUT (data->parent))
    {
      gtk_layout_move (GTK_LAYOUT (data->parent), new_widget, x, y);
      /*gtk_widget_set_uposition (new_widget, x, y);*/
    }

  gtk_widget_show (new_widget);
  gb_widget_show_properties (new_widget);
  tree_add_widget (new_widget);
  glade_project_set_changed (data->project, TRUE);
}


#if GLADE_SUPPORTS_GTK_PACKER
static void
add_widget_to_container (GtkWidget * parent)
{
  char *class_name;

  class_name = glade_palette_get_widget_class (GLADE_PALETTE (glade_palette));
  g_return_if_fail (class_name != NULL);
  glade_palette_reset_selection (GLADE_PALETTE (glade_palette), TRUE);
  gb_widget_new_full (class_name, TRUE, parent, NULL, 0, 0,
		      add_widget_to_container_finish, GB_CREATING, NULL);
}


static void
add_widget_to_container_finish (GtkWidget * new_widget, GbWidgetNewData * data)
{
  gtk_container_add (GTK_CONTAINER (data->parent), new_widget);
  gtk_widget_show (new_widget);
  gb_widget_show_properties (new_widget);
  tree_add_widget (new_widget);
  glade_project_set_changed (data->project, TRUE);
}
#endif

/*
 * Clears all currently selected widgets, except the given widget.
 * Returns TRUE if given widget is selected.
 */
gint
editor_clear_selection (GtkWidget * leave_widget)
{
  GList *child = selected_widgets, *next_child;
  GtkWidget *widget;
  gint selected = FALSE;

  while (child)
    {
      next_child = child->next;
      widget = child->data;
      if (widget == leave_widget)
	{
	  selected = TRUE;
	}
      else
	{
	  selected_widgets = g_list_remove (selected_widgets, widget);
	  tree_select_widget (widget, FALSE);
	  editor_refresh_widget_selection (widget);
	}
      child = next_child;
    }
  return selected;
}


void
editor_remove_widget_from_selection (GtkWidget * widget)
{
  selected_widgets = g_list_remove (selected_widgets, widget);

  /* If the widget is not being destroyed, we deselect it in the tree.
     If it is being destroyed, it will get removed from the tree so we don't
     have to bother. */
  if (!(GTK_OBJECT_FLAGS (widget) & GTK_IN_DESTRUCTION))
    tree_select_widget (widget, FALSE);
}


void
editor_deselect_all_placeholders (void)
{
  GList *child, *next_child;
  GtkWidget *widget;

  child = selected_widgets;
  while (child)
    {
      next_child = child->next;
      widget = child->data;

      if (GB_IS_PLACEHOLDER (widget))
	{
	  selected_widgets = g_list_remove (selected_widgets, widget);
	  editor_refresh_widget_selection (widget);
	}
      child = next_child;
    }
}


gboolean
editor_is_selected (GtkWidget *widget)
{
  GList *elem = selected_widgets;

  while (elem)
    {
      if (GTK_WIDGET (elem->data) == widget)
	return TRUE;
      elem = elem->next;
    }
  return FALSE;
}


void
editor_dump_selection (void)
{
  GList *elem = selected_widgets;

  g_print ("Selected widgets:\n");
  while (elem)
    {
      g_print ("  %p: %s\n", elem->data,
	       gtk_widget_get_name (GTK_WIDGET (elem->data)));
      elem = elem->next;
    }
}


static gint
get_position_in_widget (GtkWidget * widget, gint x, gint y)
{
  gint width = widget->allocation.width;
  gint height = widget->allocation.height;
  if (x < GB_CORNER_WIDTH && y < GB_CORNER_HEIGHT)
    return GB_TOP_LEFT;
  if (x > width - GB_CORNER_WIDTH && y < GB_CORNER_HEIGHT)
    return GB_TOP_RIGHT;
  if (x < GB_CORNER_WIDTH && y > height - GB_CORNER_HEIGHT)
    return GB_BOTTOM_LEFT;
  if (x > width - GB_CORNER_WIDTH && y > height - GB_CORNER_HEIGHT)
    return GB_BOTTOM_RIGHT;
  return GB_MIDDLE;
}


static void
raise_fixed_child (GtkWidget * widget)
{
  GtkFixed *fixed;

  g_return_if_fail (GTK_IS_FIXED (widget->parent));
  fixed = GTK_FIXED (widget->parent);
  /* If widget hasn't got a window, move it to the back of the parent fixed's
     children. If it has got a window, raise it. */
  /* Note: this is slightly naughty as it changes the GtkFixed's GList of
     children, but it's better than removing the widget and adding it again. */
  if (GTK_WIDGET_NO_WINDOW (widget))
    {
      GList *child;
      GtkFixedChild *data;
      child = fixed->children;
      while (child)
	{
	  data = child->data;
	  if (data->widget == widget)
	    {
	      fixed->children = g_list_remove (fixed->children, data);
	      fixed->children = g_list_append (fixed->children, data);
	      break;
	    }
	  child = child->next;
	}
    }
  else
    {
      gdk_window_raise (widget->window);
    }
}


gboolean
editor_select_widget_control (GtkWidget * widget)
{
  MSG ("IN editor_select_widget_control");
  /* If widget is currently selected, deslect it, else add it to
     selected. */
  if (g_list_find (selected_widgets, widget))
    {
      selected_widgets = g_list_remove (selected_widgets, widget);
    }
  else
    {
      gb_widget_show_properties (widget);
      selected_widgets = g_list_append (selected_widgets, widget);
      glade_project_view_clear_component_selection (current_project_view,
						    widget);
    }
  editor_refresh_widget_selection (widget);
  return TRUE;
}
 
/* FIXME: handle widget-tree better now it has extended selection */
GList *
editor_get_selection ()
{
  return g_list_first (selected_widgets);
}


/* This sets the list of selected widgets, possibly NULL. It takes control
   of the GList, so don't free it. */
void
editor_set_selection (GList *new_selection)
{
  GList *old_selection, *elem;

  old_selection = selected_widgets;

  selected_widgets = new_selection;

  for (elem = old_selection; elem; elem = elem->next)
    editor_refresh_widget_selection (elem->data);

  for (elem = new_selection; elem; elem = elem->next)
    {
      /* Only refresh it if it wasn't already refreshed above. */
      if (!g_list_find (old_selection, elem->data))
	editor_refresh_widget_selection (elem->data);
    }

  /* If just one widget is selected, show its properties. */
  if (g_list_length (new_selection) == 1)
    gb_widget_show_properties (GTK_WIDGET (new_selection->data));

  g_list_free (old_selection);
}


static gint
get_notebook_page (GtkNotebook * notebook,
		   GtkWidget * widget)
{
  gint nchildren, page;

  nchildren = g_list_length (notebook->children);

  for (page = 0; page < nchildren; page++)
    {
      GtkWidget *child, *tab;

      child = gtk_notebook_get_nth_page (notebook, page);
      tab = gtk_notebook_get_tab_label (notebook, child);
      if (tab == widget)
	return page;
    }

  return -1;
}



/* FIXME: When the editor is rewritten as a GtkObject, we need simple
   functions to select/deselect widgets separate from the complicated
   stuff used for moving/resizing. */
gboolean
editor_select_widget (GtkWidget * widget, GdkEventButton * event,
		      gint x, gint y)
{
  gint already_selected, page;
  GtkWidget *select_widget, *ancestor;
  gboolean handled = FALSE;

  MSG ("IN editor_select_widget");
#if 0
  g_print ("=+=+ widget: %s Alloc X:%i Y:%i W:%i H:%i\n",
	   gtk_widget_get_name (widget),
	   widget->allocation.x, widget->allocation.y,
	   widget->allocation.width, widget->allocation.height);
#endif

  /* reset any move/resize action */
  drag_action = GB_DRAG_NONE;

  /* Shift + selection. Step through parents of this widget, until a selected
     widget is found. Then clear selection & select that widgets parent.
     If no parents were selected, or the top-level parent was selected,
     select this widget. */
  if (event && event->state & GDK_SHIFT_MASK)
    {
      gint found = FALSE;
      ancestor = widget;
      while (ancestor)
	{
	  if (g_list_find (selected_widgets, ancestor))
	    {
	      found = TRUE;
	      break;
	    }
	  ancestor = ancestor->parent;
	}
      if (found && ancestor->parent != NULL)
	{
	  select_widget = ancestor->parent;
	  /* If widget is not a GbWidget, i.e. it has no GladeWidgetData,
	     skip it. */
	  while (select_widget)
	    {
	      if (GB_IS_GB_WIDGET (select_widget))
		break;
	      select_widget = select_widget->parent;
	    }
	}
      else
	{
	  select_widget = widget;
	}
      g_return_val_if_fail (select_widget != NULL, FALSE);
      gb_widget_show_properties (select_widget);
      already_selected = editor_clear_selection (select_widget);
      if (already_selected)
	return FALSE;
      selected_widgets = g_list_append (selected_widgets, select_widget);
      tree_select_widget (select_widget, TRUE);
      glade_project_view_clear_component_selection (current_project_view,
						    widget);
      editor_refresh_widget_selection (select_widget);
      return TRUE;
    }

#if 0
  /* Control + selection. If widget is currently selected, deslect it, else
     add it to the selected widgets. */
  /* I've taken this out as it can cause crashes and it isn't very useful. */
  if (event && event->state & GDK_CONTROL_MASK)
    {
      /* If widget is currently selected, deselect it, else add it to
         selected. */
      editor_select_widget_control (widget);

      if (g_list_find (selected_widgets, widget))
	tree_select_widget (widget, TRUE);
      else
	tree_select_widget (widget, FALSE);

      return TRUE;
    }
#endif

  /* Normal selection. If the widget is currently selected, just get the
     data for a possible resize/drag. If it is not selected, clear all
     currently selected widgets, then select this one.
     Also remember where the button press occurred, in case widget is being
     moved or resized (in a fixed container). */
  gb_widget_show_properties (widget);

  if (!g_list_find (selected_widgets, widget))
    {
      editor_clear_selection (NULL);
      selected_widgets = g_list_append (selected_widgets, widget);
      tree_select_widget (widget, TRUE);
      handled = TRUE;
      glade_project_view_clear_component_selection (current_project_view,
						    widget);
      /* If parent is a fixed container, move widget to front */
      if (widget->parent && GTK_IS_FIXED (widget->parent))
	raise_fixed_child (widget);

      /* SPECIAL CODE: If the widget or an ancestor is a notebook tab,
         show the page */
      ancestor = widget;
      while (ancestor->parent)
	{
	  if (GTK_IS_NOTEBOOK (ancestor->parent))
	    {
	      page = get_notebook_page (GTK_NOTEBOOK (ancestor->parent),
					ancestor);
	      if (page != -1)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (ancestor->parent),
					       page);
	      break;
	    }
	  ancestor = ancestor->parent;
	}

      editor_refresh_widget_selection (widget);
    }


  /* NOTE: this will only work in a GtkFixed or a GtkLayout. */
  if (widget->parent && event
      && (GTK_IS_FIXED (widget->parent) || GTK_IS_LAYOUT (widget->parent)))
    {
      if (gdk_pointer_grab (event->window, FALSE,
			    GDK_POINTER_MOTION_HINT_MASK |
			    GDK_BUTTON1_MOTION_MASK |
			    GDK_BUTTON_RELEASE_MASK,
			    NULL, NULL, event->time))
	return FALSE;

#if 0
      g_print ("###### Grabbed pointer\n");
#endif
      drag_has_pointer_grab = TRUE;
      drag_action = get_position_in_widget (widget, x, y);
      drag_offset_x = x;
      drag_offset_y = y;

      drag_widget_x1 = widget->allocation.x;
      drag_widget_y1 = widget->allocation.y;

      if (GTK_IS_FIXED (widget->parent))
	{
	  drag_widget_x1 -= widget->parent->allocation.x;
	  drag_widget_y1 -= widget->parent->allocation.y;
	}

      drag_widget_x2 = drag_widget_x1 + widget->allocation.width;
      drag_widget_y2 = drag_widget_y1 + widget->allocation.height;

#if 0
      g_print ("drag_action:%i, offset_x:%i offset_y:%i "
	       "x1:%i y1:%i x2:%i y2:%i\n",
	       drag_action, drag_offset_x, drag_offset_y,
	       drag_widget_x1, drag_widget_y1,
	       drag_widget_x2, drag_widget_y2);
#endif

      /* We return TRUE to make sure the signal is stopped so the widget
	 doesn't popup a menu or something, which would make moving/resizing
	 very difficult. */
      return TRUE;
    }
  return handled;
}


/* This returns TRUE if the event should be ignored. Currently we only
   ignore events in clist/ctree resize windows, but more may be added. */
static gboolean
editor_check_ignore_event (GtkWidget *widget,
			   GdkEventAny *event)
{
  GtkWidget *window_widget;

  g_return_val_if_fail (event != NULL, FALSE);

  gdk_window_get_user_data (event->window, (gpointer) &window_widget);

  if (GTK_IS_CLIST (window_widget))
    {
      gint i;
      for (i = 0; i < GTK_CLIST (window_widget)->columns; i++)
	{
	  GtkCListColumn *col = &GTK_CLIST (window_widget)->column[i];
	  if (event->window == col->window)
	    {
	      MSG ("Ignored event (clist resize window)");
	      return TRUE;
	    }
	}
    }
  return FALSE;
}


static gint
editor_on_motion_notify (GtkWidget * signal_widget,
			 GdkEventMotion * event,
			 gpointer data)
{
#if 0
  g_print ("In editor_on_motion_notify: %s\n",
	   gtk_widget_get_name (signal_widget));
#endif

  if (event->state & GDK_BUTTON1_MASK)
    return editor_do_drag_action (signal_widget, event);
  else
    return editor_set_cursor (signal_widget, event);
}


static gint
editor_set_cursor (GtkWidget * signal_widget,
		   GdkEventMotion * event)
{
  /* We remember the last cursor set and the last window, so we don't set the
     same cursor on the same window repeatedly. */
  static GdkWindow *last_window = NULL;
  static GdkCursor *last_cursor = NULL;

  GtkWidget *widget, *event_widget;
  GdkCursor *cursor = NULL;
  gint x, y, event_x, event_y, pos;

#if 0
  g_print ("In editor_set_cursor %s hint:%i\n",
	   gtk_widget_get_name (signal_widget), event->is_hint);
#endif

  /* First of all, we find out which widget the event originated from.
     We step up the parents until we find the current widget, checking if
     any GbWidgets are children. If they are, we have already seen this event
     so we just return. */
  event_widget = gtk_get_event_widget ((GdkEvent*) event);
  if (event_widget)
    {
      while (event_widget != signal_widget)
	{
	  if (event_widget == NULL)
	    {
	      g_warning ("motion_notify - didn't find signal widget");
	      break;
	    }

	  if (GB_IS_GB_WIDGET (event_widget))
	    return FALSE;

	  event_widget = event_widget->parent;
	}

    }

  if (event->is_hint)
    gdk_window_get_pointer (event->window, &event_x, &event_y, NULL);
  else
    {
      event_x = event->x;
      event_y = event->y;
    }

  widget = editor_get_event_widget (signal_widget, event->window,
				    event_x, event_y, &x, &y);
  if (widget == NULL)
    return FALSE;

  if (editor_check_ignore_event (widget, (GdkEventAny*) event))
    return FALSE;

  /* Remember the widget, so we can redirect key presses to it. But only
     keep pointers to GbWidgets, as we can reset the pointer to NULL when the
     widget is destroyed. */
  if (GB_IS_GB_WIDGET (widget))
    {
#if 0
      g_print ("   mouse_over_widget: %s\n", gtk_widget_get_name (widget));
#endif
      mouse_over_widget = widget;
    }

#if 0
  g_print ("editor_set_cursor widget: %s (%p) X:%i Y:%i\n",
	   gtk_widget_get_name (widget), widget, x, y);
#endif
  if (glade_palette_is_selector_on (GLADE_PALETTE (glade_palette)))
    {
#ifdef USE_GNOME
      if (widget->parent
	  && (GTK_IS_FIXED (widget)
	      || (GTK_IS_LAYOUT (widget) && !GNOME_IS_CANVAS (widget))))
#else
      if (widget->parent
	  && (GTK_IS_FIXED (widget->parent)
	      || GTK_IS_LAYOUT (widget->parent)))
#endif
	{
	  pos = get_position_in_widget (widget, x, y);
	  switch (pos)
	    {
	    case GB_TOP_LEFT:
#if 0
	      g_print ("TOP_LEFT\n");
#endif
	      cursor = cursor_top_left;
	      break;
	    case GB_TOP_RIGHT:
#if 0
	      g_print ("TOP_RIGHT\n");
#endif
	      cursor = cursor_top_right;
	      break;
	    case GB_BOTTOM_LEFT:
#if 0
	      g_print ("BOTTOM_LEFT\n");
#endif
	      cursor = cursor_bottom_left;
	      break;
	    case GB_BOTTOM_RIGHT:
#if 0
	      g_print ("BOTTOM_RIGHT\n");
#endif
	      cursor = cursor_bottom_right;
	      break;
	    case GB_MIDDLE:
#if 0
	      g_print ("MIDDLE\n");
#endif
	      cursor = cursor_move;
	      break;
	    }
	}
      else
	{
	  cursor = cursor_selector;
	}
    }
  else
    {
      if (GTK_IS_FIXED (widget) || GTK_IS_LAYOUT (widget)
	  || (widget->parent && (GTK_IS_FIXED (widget->parent)
				 || GTK_IS_LAYOUT (widget->parent))))
	cursor = cursor_add_to_fixed;
      else
	cursor = cursor_add_widget;
    }

  if (cursor)
    {
      if (last_window != event->window || last_cursor != cursor)
	{
	  gdk_window_set_cursor (event->window, cursor);
	  last_window = event->window;
	  last_cursor = cursor;

	}
    }

  return FALSE;
}


static gint
editor_do_drag_action (GtkWidget * signal_widget, GdkEventMotion * event)
{
  GtkWidget *widget;
  GladeWidgetData *wdata;
  gint x, y, event_x, event_y;
  gint mouse_x, mouse_y, new_x = 0, new_y = 0, new_width = 0, new_height = 0;
  gint old_x, old_y, old_width, old_height;

#if 0
  g_print ("In editor_do_drag_action %s hint:%i\n",
	   gtk_widget_get_name (signal_widget), event->is_hint);
#endif

  /* If no move/resize action was started in the button_press event, return. */
  if (drag_action == GB_DRAG_NONE)
    return FALSE;

  if (event->is_hint)
    gdk_window_get_pointer (event->window, &event_x, &event_y, NULL);
  else
    {
      event_x = event->x;
      event_y = event->y;
    }

#if 0
  g_print ("In editor_do_drag_action %s hint:%i %i,%i\n",
	   gtk_widget_get_name (signal_widget), event->is_hint,
	   event_x, event_y);
#endif

  /* Use our function to figure out which widget the mouse is in, and where
     in the widget. */
  widget = editor_get_event_widget (signal_widget, event->window,
				    event_x, event_y, &x, &y);
  if (widget == NULL)
    return FALSE;

  if (editor_check_ignore_event (widget, (GdkEventAny*) event))
    return FALSE;


  if (!widget->parent
      || (!GTK_IS_FIXED (widget->parent) && !GTK_IS_LAYOUT (widget->parent)))
    return FALSE;

  if (dragging_widget == NULL)
    {
      dragging_widget = widget;
      gtk_grab_add (widget);
    }
  else
    {
      if (dragging_widget != widget)
	return FALSE;
    }

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_val_if_fail (wdata != NULL, FALSE);

  old_x = widget->allocation.x;
  old_y = widget->allocation.y;
  old_width = widget->allocation.width;
  old_height = widget->allocation.height;

  /* GtkFixed doesn't normally have a window now, so we need to subtract its
     position so our coordinates are relative to it. */
  if (GTK_IS_FIXED (widget->parent))
    {
      old_x -= widget->parent->allocation.x;
      old_y -= widget->parent->allocation.y;
    }

  gdk_window_get_pointer (widget->parent->window, &mouse_x, &mouse_y, NULL);
  if (GTK_IS_FIXED (widget->parent))
    {
      mouse_x -= widget->parent->allocation.x;
      mouse_y -= widget->parent->allocation.y;
    }
  if (GTK_IS_LAYOUT (widget->parent))
    {
      mouse_x += GTK_LAYOUT (widget->parent)->hadjustment->value;
      mouse_y += GTK_LAYOUT (widget->parent)->vadjustment->value;
      old_x += GTK_LAYOUT (widget->parent)->hadjustment->value;
      old_y += GTK_LAYOUT (widget->parent)->vadjustment->value;
    }

  switch (drag_action)
    {
    case GB_TOP_LEFT:
      new_x = snap_left_edge (mouse_x);
      new_y = snap_top_edge (mouse_y);
      new_width = drag_widget_x2 - new_x;
      new_height = drag_widget_y2 - new_y;
      if (new_width < MIN_WIDGET_WIDTH)
	{
	  new_width = MIN_WIDGET_WIDTH;
	  new_x = drag_widget_x2 - new_width;
	}
      if (new_height < MIN_WIDGET_HEIGHT)
	{
	  new_height = MIN_WIDGET_HEIGHT;
	  new_y = drag_widget_y2 - new_height;
	}
      break;

    case GB_TOP_RIGHT:
      new_x = drag_widget_x1;
      new_y = snap_top_edge (mouse_y);
      new_width = snap_right_edge (mouse_x) - new_x;
      new_height = drag_widget_y2 - new_y;
      if (new_width < MIN_WIDGET_WIDTH)
	{
	  new_width = MIN_WIDGET_WIDTH;
	}
      if (new_height < MIN_WIDGET_HEIGHT)
	{
	  new_height = MIN_WIDGET_HEIGHT;
	  new_y = drag_widget_y2 - new_height;
	}
      break;

    case GB_BOTTOM_LEFT:
      new_x = snap_left_edge (mouse_x);
      new_y = drag_widget_y1;
      new_width = drag_widget_x2 - new_x;
      new_height = snap_bottom_edge (mouse_y) - new_y;
      if (new_width < MIN_WIDGET_WIDTH)
	{
	  new_width = MIN_WIDGET_WIDTH;
	  new_x = drag_widget_x2 - new_width;
	}
      if (new_height < MIN_WIDGET_HEIGHT)
	new_height = MIN_WIDGET_HEIGHT;
      break;

    case GB_BOTTOM_RIGHT:
      new_x = drag_widget_x1;
      new_y = drag_widget_y1;
      new_width = snap_right_edge (mouse_x) - new_x;
      new_height = snap_bottom_edge (mouse_y) - new_y;
      if (new_width < MIN_WIDGET_WIDTH)
	new_width = MIN_WIDGET_WIDTH;
      if (new_height < MIN_WIDGET_HEIGHT)
	new_height = MIN_WIDGET_HEIGHT;
      break;

    case GB_MIDDLE:
      new_x = snap_left_edge (mouse_x - drag_offset_x);
      new_y = snap_top_edge (mouse_y - drag_offset_y);
      new_width = widget->allocation.width;
      new_height = widget->allocation.height;
      if (new_x < 0)
	new_x = 0;
      if (new_y < 0)
	new_y = 0;
      break;
    }

#if 0
      g_print ("old_x: %i old_y: %i new_x: %i new_y: %i w: %i h: %i\n",
	       widget->allocation.x, widget->allocation.y,
	       new_x, new_y, new_width, new_height);
#endif

  /* Only move/resize widget if values have changed */
  if (new_width != widget->allocation.width
      || new_height != widget->allocation.height)
    {
      wdata->width = new_width;
      wdata->height = new_height;
      gb_widget_set_usize (widget, new_width, new_height);
    }

  if (new_x != old_x || new_y != old_y)
    {
#if 0
      g_print ("  moving widget\n");
#endif
      if (GTK_IS_FIXED (widget->parent))
	{
	  /* FIXME: GTK+ bug workaround. The widget doesn't move if we only
	     call gtk_fixed_move(). This is very slow as well. */
	  gtk_fixed_move (GTK_FIXED (widget->parent), widget, new_x, new_y);
	  /*gtk_widget_set_uposition (widget, new_x, new_y);*/
	}
      else if (GTK_IS_LAYOUT (widget->parent))
	{
	  gtk_layout_move (GTK_LAYOUT (widget->parent), widget, new_x, new_y);
	}

      gtk_widget_queue_draw (widget->parent);
    }

  return TRUE;
}


/*
 * Adding signals to widgets to allow manipulation, e.g. selecting/drawing
 */

static void
editor_on_widget_realize (GtkWidget *widget, gpointer data)
{
#if 0
  static GdkPixmap *placeholder_pixmap = NULL;

  g_print ("In editor_on_widget_realize widget:%s (%p)\n",
	   gtk_widget_get_name (widget), widget);

  if (GB_IS_PLACEHOLDER (widget))
    {
      /* Create placeholder pixmap if it hasn't already been created.
	 There may be a problem with multi-depth displays. */
      if (placeholder_pixmap == NULL)
	{
	  placeholder_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
							     NULL, NULL,
							     placeholder_xpm);
	  if (!placeholder_pixmap)
	    {
	      g_warning ("Couldn't create placeholder pixmap\n");
	      /* FIXME: Use a color instead? */
	    }
	}

      if (placeholder_pixmap != NULL)
	gdk_window_set_back_pixmap (widget->window, placeholder_pixmap, FALSE);
    }
#endif
}


/* This adds the button signals to an existing widget (currently only the
   Clist title buttons). */
void
editor_add_mouse_signals_to_existing (GtkWidget * widget)
{
  gtk_signal_connect (GTK_OBJECT (widget), "event",
		      GTK_SIGNAL_FUNC (editor_on_event), NULL);
#if 0
  gtk_signal_connect (GTK_OBJECT (widget), "button_press_event",
		      GTK_SIGNAL_FUNC (editor_on_button_press), NULL);
  gtk_signal_connect (GTK_OBJECT (widget), "button_release_event",
		      GTK_SIGNAL_FUNC (editor_on_button_release), NULL);
#endif
}


static gint
editor_on_enter_notify (GtkWidget        *widget,
			GdkEventCrossing *event)
{
  /* We try to stop enter/leave notify events when moving/resizing widget in
     a GtkFixed/GtkLayout as it causes flicker. */
  return dragging_widget ? TRUE : FALSE;
}


static gint
editor_on_leave_notify (GtkWidget        *widget,
			GdkEventCrossing *event)
{
  /* We try to stop enter/leave notify events when moving/resizing widget in
     a GtkFixed/GtkLayout as it causes flicker. */
  return dragging_widget ? TRUE : FALSE;
}


static void
add_mouse_signals_recursive (GtkWidget *widget, gpointer data)
{
#if 0
  g_print ("Adding mouse signals to:%s (%s, %p)\n",
	   gtk_widget_get_name (widget),
	   gtk_type_name (GTK_OBJECT_TYPE (widget)),
	   widget);
#endif

  /* FIXME: We don't add signals to menu items, since it currently makes it
     impossible to popup the menus in a menubar. */
  if (GTK_IS_MENU_ITEM (widget))
    return;

  /* Ensure that the event mask is set so we get button press & release
     events. */
  if (!GTK_WIDGET_NO_WINDOW (widget))
    {
      if (!GTK_WIDGET_REALIZED (widget))
	{
	  gtk_widget_set_events (widget, gtk_widget_get_events (widget)
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK);
	}
      else
	{
	  GdkEventMask event_mask;
	  
	  /* FIXME: Here we set the event mask for the main window of a widget,
	     but widgets can have more than one window. How do we get all the
	     windows of a widget? */
	  event_mask = gdk_window_get_events (widget->window);
	  gdk_window_set_events (widget->window, event_mask
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK);
	}
    }

  gtk_signal_connect (GTK_OBJECT (widget), "event",
		      GTK_SIGNAL_FUNC (editor_on_event), NULL);
#if 0
  gtk_signal_connect (GTK_OBJECT (widget), "button_press_event",
		      GTK_SIGNAL_FUNC (editor_on_button_press), NULL);
  gtk_signal_connect (GTK_OBJECT (widget), "button_release_event",
		      GTK_SIGNAL_FUNC (editor_on_button_release), NULL);
#endif

  /* We connect to these so we can stop widgets getting them while we are
     dragging/resizing. It stops widgets changing state, i.e. normal/active
     and so cuts down on flickering a bit. */
  gtk_signal_connect (GTK_OBJECT (widget), "enter_notify_event",
		      GTK_SIGNAL_FUNC (editor_on_enter_notify), NULL);
  gtk_signal_connect (GTK_OBJECT (widget), "leave_notify_event",
		      GTK_SIGNAL_FUNC (editor_on_leave_notify), NULL);

  gb_widget_children_foreach (widget,
			      (GtkCallback) add_mouse_signals_recursive, NULL);
}

/* We need to be careful about passing events on to widgets, especially with
   regard to mouse grabs - in a GtkEntry the mouse is grabbed while selecting
   text, and this can cause all sorts of problems for Glade. */
void
editor_add_mouse_signals (GtkWidget * widget)
{
  /* Widgets without windows will not get events directly from X Windows,
     but they may have child widgets which pass events up to them, e.g.
     a GtkCombo has a GtkEntry which will get X events.
     This doesn't matter too much since we have to call a function to figure
     out which widget the event is for anyway. */
  add_mouse_signals_recursive (widget, NULL);

  gtk_signal_connect_after (GTK_OBJECT (widget), "realize",
			    GTK_SIGNAL_FUNC (editor_on_widget_realize), NULL);
}

void
editor_add_key_signals (GtkWidget * widget)
{
  /* We only add key signal handlers to windows. */
  if (!GTK_IS_WINDOW (widget))
    return;

  gtk_signal_connect (GTK_OBJECT (widget), "key_press_event",
		      GTK_SIGNAL_FUNC (editor_on_key_press_event), NULL);
  gtk_signal_connect (GTK_OBJECT (widget), "key_release_event",
		      GTK_SIGNAL_FUNC (editor_on_key_release_event), NULL);
}


void
on_size_allocate (GtkWidget * widget,
		  GtkAllocation *allocation,
		  GladeWidgetData * wdata)
{
  /* Reset the flag, since the size is allocated now. Note that wdata may be
     NULL as widget may be a placeholder. */
  if (wdata)
    {
      wdata->flags &= ~GLADE_SIZE_NOT_ALLOCATED;
    }

#if 0
  g_print ("In on_size_allocate: %s (%p) x:%i y:%i w:%i h:%i\n",
	   gtk_widget_get_name (widget), widget, allocation->x, allocation->y,
	   allocation->width, allocation->height);
#endif

  if (property_get_widget () == widget)
    {
      gb_widget_show_position_properties (widget);

      if (widget->parent && GTK_IS_FIXED (widget->parent))
	{
	  property_set_auto_apply (FALSE);
	  property_set_int (GladeFixedChildX,
			    allocation->x - widget->parent->allocation.x);
	  property_set_int (GladeFixedChildY,
			    allocation->y - widget->parent->allocation.y);
	  property_set_auto_apply (TRUE);
	}
      else if (widget->parent && GTK_IS_LAYOUT (widget->parent))
	{
	  property_set_auto_apply (FALSE);
	  property_set_int (GladeLayoutChildX, allocation->x);
	  property_set_int (GladeLayoutChildY, allocation->y);
	  property_set_auto_apply (TRUE);
	}
    }
}


void
editor_add_draw_signals (GtkWidget * widget)
{
  GladeWidgetData *widget_data;

  /* FIXME: Note that we set GDK_POINTER_MOTION_HINT_MASK here. This may not
     be wise since widgets may be designed to work with normal motion events
     only. Also this won't work if the widget is already realized. */
  if (!GTK_WIDGET_NO_WINDOW (widget) && !GTK_WIDGET_REALIZED (widget))
    gtk_widget_set_events (widget, gtk_widget_get_events (widget)
			   | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK
			   | GDK_BUTTON1_MOTION_MASK
			   | GDK_POINTER_MOTION_HINT_MASK);

  widget_data = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);

  gtk_signal_connect (GTK_OBJECT (widget), "expose_event",
		      GTK_SIGNAL_FUNC (expose_widget), widget_data);
  gtk_signal_connect_after (GTK_OBJECT (widget), "size_allocate",
			    GTK_SIGNAL_FUNC (on_size_allocate), widget_data);

  /* FIXME: mouse signal - This also needs to be added to all children. */
  gtk_signal_connect (GTK_OBJECT (widget), "motion_notify_event",
		      GTK_SIGNAL_FUNC (editor_on_motion_notify), NULL);

  /* Needed for scrolled window, clist? & possibly other widgets */
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget),
			  (GtkCallback) editor_add_draw_signals, NULL);
}



static gint
expose_widget (GtkWidget * widget, GdkEventExpose * event,
	       GladeWidgetData * widget_data)
{
  GtkWidgetClass *class;

#if 0
  g_print ("In expose_event widget:%s (%p)\n", gtk_widget_get_name (widget),
	   widget);
  g_print ("Area x:%i y:%i w:%i h:%i\n", event->area.x, event->area.y,
	   event->area.width, event->area.height);
#endif

  /* Run the class handler here, then we return TRUE to stop the signal. */
  class = GTK_WIDGET_GET_CLASS (widget);
  if (class->expose_event)
    class->expose_event (widget, event);

  /* Ignore spurious exposes before widget is positioned. */
  if (widget->allocation.x == -1 || widget->allocation.y == -1)
    return TRUE;

  paint_widget (widget, event);

  return TRUE;
}


void
paint_widget (GtkWidget * widget, GdkEventExpose *event)
{
  static GdkPixmap *placeholder_pixmap = NULL;
  GType type;

  MSG3 ("Painting widget: %s W:%i H:%i", gtk_widget_get_name (widget),
	widget->allocation.width, widget->allocation.height);

  /* Check widget is drawable in case it has been deleted. */
  if (!GTK_WIDGET_DRAWABLE (widget))
    return;

  /* Don't try to draw anything if the width or height of the widget is 0. */
  if (widget->allocation.width == 0 || widget->allocation.height == 0)
    return;

  /* If widget is a placeholder, draw the placeholder pixmap in it and a
     3D border around it. */
  if (GB_IS_PLACEHOLDER (widget))
    {
      GdkGC *light_gc;
      GdkGC *dark_gc;
      gint w, h;

      light_gc = widget->style->light_gc[GTK_STATE_NORMAL];
      dark_gc = widget->style->dark_gc[GTK_STATE_NORMAL];
      gdk_window_get_size (widget->window, &w, &h);

      /* Draw the background pixmap. */
      if (placeholder_pixmap == NULL)
	{
	  /* FIXME: Use a hash of placeholder pixmaps? So we always use the
	     correct depth? */
	  placeholder_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
							     NULL, NULL,
							     placeholder_xpm);
	  if (!placeholder_pixmap)
	    {
	      g_warning ("Couldn't create placeholder pixmap\n");
	      /* FIXME: Use a color instead? */
	    }
	}

      if (placeholder_pixmap)
	{
	  gdk_gc_set_fill (light_gc, GDK_TILED);
	  gdk_gc_set_tile (light_gc, placeholder_pixmap);
	  gdk_draw_rectangle (widget->window, light_gc, TRUE, 0, 0, w, h);
	  gdk_gc_set_fill (light_gc, GDK_SOLID);
	}

      gdk_draw_line (widget->window, light_gc, 0, 0, w - 1, 0);
      gdk_draw_line (widget->window, light_gc, 0, 0, 0, h - 1);
      gdk_draw_line (widget->window, dark_gc, 0, h - 1, w - 1, h - 1);
      gdk_draw_line (widget->window, dark_gc, w - 1, 0, w - 1, h - 1);
    }

  if (event->window)
    {
      gpointer expose_widget;

      gdk_window_get_user_data (event->window, &expose_widget);

      if (expose_widget)
	{
	  gtk_idle_add_priority (GTK_PRIORITY_DEFAULT + 10,
				 (GtkFunction)editor_idle_handler,
				 event->window);

	  /* We ref the window, to make sure it isn't freed before the idle
	     handler. We unref it there. */
	  gdk_window_ref (event->window);
	}
    }

  /* Draw grid for fixed containers */
  type = G_OBJECT_TYPE (widget);
  if (GB_IS_GB_WIDGET (widget)
      && (type == GTK_TYPE_FIXED || type == GTK_TYPE_LAYOUT))
    draw_grid (widget);
}


/* This returns the window that the given widget's position is relative to.
   Usually this is the widget's parent's window. But if the widget is a
   toplevel, we use its own window, as it doesn't have a parent.
   Some widgets also lay out widgets in different ways. */
static GdkWindow*
glade_util_get_window_positioned_in (GtkWidget *widget)
{
	GtkWidget *parent;

	parent = widget->parent;

#ifdef USE_GNOME
	/* BonoboDockItem widgets use a different window when floating. */
	if (BONOBO_IS_DOCK_ITEM (widget)
	    && BONOBO_DOCK_ITEM (widget)->is_floating) {
		return BONOBO_DOCK_ITEM (widget)->float_window;
	}

	if (parent && BONOBO_IS_DOCK_ITEM (parent)
	    && BONOBO_DOCK_ITEM (parent)->is_floating) {
		return BONOBO_DOCK_ITEM (parent)->float_window;
	}
#endif

	if (parent)
		return parent->window;

	return widget->window;
}

static void
glade_util_draw_nodes (GdkWindow *window, GdkGC *gc,
		       gint x, gint y,
		       gint width, gint height)
{
#if 0
  g_print ("draw_nodes window: %p %i,%i %ix%i\n",
	   window, x, y, width, height);
#endif
	if (width > GB_CORNER_WIDTH && height > GB_CORNER_HEIGHT) {
		gdk_draw_rectangle (window, gc, TRUE,
				    x, y,
				    GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
		gdk_draw_rectangle (window, gc, TRUE,
				    x, y + height - GB_CORNER_HEIGHT,
				    GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
		gdk_draw_rectangle (window, gc, TRUE,
				    x + width - GB_CORNER_WIDTH, y,
				    GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
		gdk_draw_rectangle (window, gc, TRUE,
				    x + width - GB_CORNER_WIDTH,
				    y + height - GB_CORNER_HEIGHT,
				    GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
	}

	gdk_draw_rectangle (window, gc, FALSE, x, y, width - 1, height - 1);
}

/* This calculates the offset of the given window within its toplevel.
   It also returns the toplevel. */
static void
glade_util_calculate_window_offset (GdkWindow *window,
				    gint *x, gint *y,
				    GdkWindow **toplevel)
{
	gint tmp_x, tmp_y;

	/* Calculate the offset of the window within its toplevel. */
	*x = 0;
	*y = 0;

	for (;;) {
		if (gdk_window_get_window_type (window) != GDK_WINDOW_CHILD)
			break;
		gdk_window_get_position (window, &tmp_x, &tmp_y);
		*x += tmp_x;
		*y += tmp_y;
		window = gdk_window_get_parent (window);
	}

	*toplevel = window;
}

/* This returns TRUE if it is OK to draw the selection nodes for the given
   selected widget inside the given window that has received an expose event.
   For most widgets it returns TRUE, but if a selected widget is inside a
   widget like a viewport, that uses its own coordinate system, then it only
   returns TRUE if the expose window is inside the viewport as well. */
static gboolean
glade_util_can_draw_nodes (GtkWidget *sel_widget, GdkWindow *sel_win,
			   GdkWindow *expose_win)
{
	GtkWidget *widget, *viewport = NULL;
	GdkWindow *viewport_win = NULL;

	/* Check if the selected widget is inside a viewport. */
	for (widget = sel_widget->parent; widget; widget = widget->parent) {
		if (GTK_IS_VIEWPORT (widget)) {
			viewport = widget;
			viewport_win = GTK_VIEWPORT (widget)->bin_window;
			break;
		}
	}

	/* If there is no viewport-type widget above the selected widget,
	   it is OK to draw the selection anywhere. */
	if (!viewport)
		return TRUE;

	/* If we have a viewport-type widget, check if the expose_win is
	   beneath the viewport. If it is, we can draw in it. If not, we
	   can't.*/
	for (;;) {
		if (expose_win == sel_win)
			return TRUE;
		if (expose_win == viewport_win)
			return FALSE;
		if (gdk_window_get_window_type (expose_win) != GDK_WINDOW_CHILD)
			break;
		expose_win = gdk_window_get_parent (expose_win);
	}

	return FALSE;
}


/* This is coped from glade3/src/glade-utils.c glade_util_draw_nodes_idle(). */
static gint
editor_idle_handler (GdkWindow *expose_win)
{
	GtkWidget *expose_widget;
	gint expose_win_x, expose_win_y;
	gint expose_win_w, expose_win_h;
	GdkWindow *expose_toplevel;
	GdkGC *gc;
	GList *elem;
	gpointer expose_widget_ptr;

	/* Find the corresponding GtkWidget. */
	gdk_window_get_user_data (expose_win, &expose_widget_ptr);
	expose_widget = GTK_WIDGET (expose_widget_ptr);

	/* Check that the window is still alive. */
	if (!expose_widget || !GTK_WIDGET_DRAWABLE (expose_widget)
	    || !gdk_window_is_viewable (expose_win))
		goto out;

	gc = expose_widget->style->black_gc;

	/* Calculate the offset of the expose window within its toplevel. */
	glade_util_calculate_window_offset (expose_win,
					    &expose_win_x,
					    &expose_win_y,
					    &expose_toplevel);

#if 0
	g_print ("expose_win: %p x: %i y: %i toplevel: %p\n",
		 expose_win, expose_win_x, expose_win_y, expose_toplevel);
#endif
	gdk_drawable_get_size (expose_win,
			       &expose_win_w, &expose_win_h);
#if 0
	g_print ("drawable size %ix%i\n", expose_win_w, expose_win_h);
#endif
	/* Step through all the selected widgets in the project. */
	for (elem = selected_widgets; elem; elem = elem->next) {
		GtkWidget *sel_widget;
		GdkWindow *sel_win, *sel_toplevel;
		gint sel_x, sel_y, x, y, w, h, sel_alloc_x, sel_alloc_y;

		sel_widget = elem->data;

		/* Skip the selected widget if it isn't realized. */
		if (!GTK_WIDGET_REALIZED (sel_widget))
		  continue;

		sel_win = glade_util_get_window_positioned_in (sel_widget);

		/* Calculate the offset of the selected widget's window
		   within its toplevel. */
		glade_util_calculate_window_offset (sel_win, &sel_x, &sel_y,
						    &sel_toplevel);

#if 0
		g_print ("sel_win: %p x: %i y: %i toplevel: %p allocation %i,%i\n",
			 sel_win, sel_x, sel_y, sel_toplevel,
			 sel_widget->allocation.x, sel_widget->allocation.y);
#endif
		/* Toplevel windows/dialogs may have their allocation set
		   relative to the root window, so we need to ignore that. */
		if (sel_widget->parent)
		  {
		    sel_alloc_x = sel_widget->allocation.x;
		    sel_alloc_y = sel_widget->allocation.y;
		  }
		else
		  {
		    sel_alloc_x = 0;
		    sel_alloc_y = 0;
		  }		    

		/* We only draw the nodes if the window that got the expose
		   event is in the same toplevel as the selected widget. */
		if (expose_toplevel == sel_toplevel
		    && glade_util_can_draw_nodes (sel_widget, sel_win,
						  expose_win)) {
			x = sel_x + sel_alloc_x - expose_win_x;
			y = sel_y + sel_alloc_y - expose_win_y;
			w = sel_widget->allocation.width;
			h = sel_widget->allocation.height;

#if 0
			g_print ("checking coords %i,%i %ix%i\n",
				 x, y, w, h);
#endif
			/* Draw the selection nodes if they intersect the
			   expose window bounds. */
			if (x < expose_win_w && x + w >= 0
			    && y < expose_win_h && y + h >= 0) {
				glade_util_draw_nodes (expose_win, gc,
						       x, y, w, h);
			}
		}
	}

 out:
	/* Remove the reference added in glade_util_queue_draw_nodes(). */
	gdk_window_unref (expose_win);

	/* Return FALSE so the idle handler isn't called again. */
	return FALSE;
}


static void
draw_grid (GtkWidget * widget)
{
  GdkGC *gc = widget->style->dark_gc[GTK_STATE_NORMAL];
  gint min_x = 0, max_x = widget->allocation.width - 1;
  gint min_y = 0, max_y = widget->allocation.height - 1;
  gint gridx, gridy, origin_x, origin_y;
  GdkWindow *window;

  if (!editor_show_grid)
    return;

  if (GTK_IS_LAYOUT (widget))
    {
      gint offset;

      /* The window size is the entire size of the layout. The allocation is
	 the part that is showing. */
      window = GTK_LAYOUT (widget)->bin_window;

      origin_x = (int) GTK_LAYOUT (widget)->hadjustment->value;
      min_x += origin_x;
      max_x += origin_x;
      offset = origin_x % editor_grid_horz_spacing;
      if (offset != 0)
	origin_x += editor_grid_horz_spacing - offset;

      origin_y = (int) GTK_LAYOUT (widget)->vadjustment->value;
      min_y += origin_y;
      max_y += origin_y;
      offset = origin_y % editor_grid_vert_spacing;
      if (offset != 0)
	origin_y += editor_grid_vert_spacing - offset;
    }
  else
    {
      /* The GtkFixed container doesn't have a window in GTK+ 2.0 (by default),
	 so we have to use the allocation. */
      window = widget->window;
      origin_x = widget->allocation.x;
      origin_y = widget->allocation.y;
      max_x += origin_x;
      max_y += origin_y;
    }

  /* Note: should we take the border_width into account? - i.e. start the
     grid inside the border. It makes it awkward if you change the border
     size. */
  if (editor_grid_style == GB_GRID_DOTS)
    {
      for (gridx = origin_x; gridx <= max_x; gridx += editor_grid_horz_spacing)
	{
	  for (gridy = origin_y; gridy <= max_y;
	       gridy += editor_grid_vert_spacing)
	    gdk_draw_point (window, gc, gridx, gridy);
	}
    }
  else
    {
      for (gridx = origin_x; gridx <= max_x; gridx += editor_grid_horz_spacing)
	gdk_draw_line (window, gc, gridx, min_y, gridx, max_y);
      for (gridy = origin_y; gridy <= max_y; gridy += editor_grid_vert_spacing)
	gdk_draw_line (window, gc, min_x, gridy, max_x, gridy);
    }
}

/*
 * Redraw the given widget completely, including all space allocated by its
 * parent (since this may be used for drawing the widget's selection).
 * If widget has no parent (i.e. its a toplevel window) just clear
 * it all and redraw.
 */

void
editor_refresh_widget (GtkWidget * widget)
{
#if 0
  g_print ("In editor_refresh_widget widget: %s (%p)\n",
	   gtk_widget_get_name (widget), widget);
#endif

  editor_refresh_widget_area (widget,
			      widget->allocation.x,
			      widget->allocation.y,
			      widget->allocation.width,
			      widget->allocation.height);
  gtk_widget_draw (widget, NULL);
}


void
editor_refresh_widget_selection (GtkWidget * widget)
{
  gint x, y, w, h;

#if 0
  g_print ("In editor_refresh_widget_selection widget: %s (%p)\n",
	   gtk_widget_get_name (widget), widget);
#endif

  x = widget->allocation.x;
  y = widget->allocation.y;
  w = widget->allocation.width;
  h = widget->allocation.height;

  /* Don't try to refresh an area if the width or height is 0. */
  if (w == 0 || h == 0)
    return;

  /* Clear the four corners. */
  editor_refresh_widget_area (widget,
			      x, y,
			      GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
  editor_refresh_widget_area (widget,
			      x, y + h - GB_CORNER_HEIGHT,
			      GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
  editor_refresh_widget_area (widget,
			      x + w - GB_CORNER_WIDTH, y,
			      GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
  editor_refresh_widget_area (widget,
			      x + w - GB_CORNER_WIDTH,
			      y + h - GB_CORNER_HEIGHT,
			      GB_CORNER_WIDTH, GB_CORNER_HEIGHT);
  /* Clear the four lines along the edges. */
  editor_refresh_widget_area (widget,
			      x + GB_CORNER_WIDTH, y,
			      w - 2 * GB_CORNER_WIDTH, 1);
  editor_refresh_widget_area (widget,
			      x + GB_CORNER_WIDTH, y + h - 1,
			      w - 2 * GB_CORNER_WIDTH, 1);
  editor_refresh_widget_area (widget,
			      x, y + GB_CORNER_HEIGHT,
			      1, h - 2 * GB_CORNER_HEIGHT);
  editor_refresh_widget_area (widget,
			      x + w - 1,
			      y + GB_CORNER_HEIGHT,
			      1, h - 2 * GB_CORNER_HEIGHT);

  gtk_widget_draw (widget, NULL);
}


void
editor_refresh_widget_area (GtkWidget * widget, gint x, gint y, gint w, gint h)
{
  GdkWindow *window;

  if (!GTK_WIDGET_DRAWABLE (widget))
    return;

  /* Don't try to refresh an area if the width or height is 0. */
  if (w <= 0 || h <= 0)
    return;

  window = glade_util_get_window_positioned_in (widget);
  gdk_window_clear_area (window, x, y, w, h);
  clear_child_windows (window, x, y, w, h);
}


/* This clears all child windows which fall within the given rectangle.
   If the rectangle width is -1, then all children are cleared. */
static void
clear_child_windows (GdkWindow * window, gint x, gint y, gint w, gint h)
{
  GList *children, *orig_children;
  GdkWindow *child_window;
  gint win_x, win_y, win_w, win_h;
  GdkRectangle area, child, intersection;

  area.x = x;
  area.y = y;
  area.width = w;
  area.height = h;

  orig_children = children = gdk_window_get_children (window);
  while (children)
    {
      child_window = children->data;
      gdk_window_get_position (child_window, &win_x, &win_y);
      gdk_window_get_size (child_window, &win_w, &win_h);

      child.x = win_x;
      child.y = win_y;
      child.width = win_w;
      child.height = win_h;

      if (gdk_rectangle_intersect (&area, &child, &intersection))
	{
	  /* We need to make sure this is not an InputOnly window, or we get
	     a BadMatch. CList uses InputOnly windows - for resizing columns.
	  */
	  if (! GDK_WINDOW_OBJECT(child_window)->input_only)
	    {
	      /* Convert to the child's coordinate space. */
	      intersection.x -= child.x;
	      intersection.y -= child.y;
	      gdk_window_clear_area (child_window,
				     intersection.x, intersection.y,
				     intersection.width, intersection.height);
	      clear_child_windows (child_window,
				   intersection.x, intersection.y,
				   intersection.width, intersection.height);
	    }
	}
      children = children->next;
    }
  g_list_free (orig_children);
}



/*
 * Key events
 */
static gint
editor_on_key_press_event (GtkWidget * widget, GdkEventKey * event,
			   gpointer data)
{
  gboolean handled = FALSE;
  guint key = event->keyval;

  MSG ("In on_key_press_event");
  switch (key)
    {
    case GDK_Delete:
      /* If we are typing over the widget, the delete key is used for that
	 rather than deleting the widget. */
      if (!property_is_typing_over_widget ())
	{
	  if (selected_widgets)
	    delete (selected_widgets->data);
	  handled = TRUE;
	}
      break;
    case GDK_Escape:
      editor_clear_selection (NULL);
      handled = TRUE;
      break;
    case GDK_l:
      /* Ctrl-L refreshes the entire window/dialog. */
      if (event->state & GDK_CONTROL_MASK)
	{
	  editor_refresh_widget (glade_util_get_toplevel (widget));
	  handled = TRUE;
	}
      break;
    case GDK_r:
      /* Ctrl-R hides the window and shows it again in the same position.
	 Hopefully it will appear at the 'real' size. */
      if (event->state & GDK_CONTROL_MASK)
	{
	  GtkWidget *toplevel;

	  /* See also gb_widget_redisplay_window() in gbwidget.c. */
	  toplevel = glade_util_get_toplevel (widget);
	  glade_util_close_window (toplevel);
	  gtk_window_reshow_with_initial_size (GTK_WINDOW (toplevel));
	  handled = TRUE;
	}
      break;
    }

#if 0
  /* We don't want modifier keys to be redirected, since trying to use an
     accelerator, e.g. Ctrl-X to cut the widget, would clear the label. */
  if (!handled
      && !(event->state & GDK_CONTROL_MASK)
      && key != GDK_Caps_Lock
      && key != GDK_Tab && key != GDK_KP_Tab
      && key != GDK_Left && key != GDK_KP_Left
      && key != GDK_Right && key != GDK_KP_Right
      && key != GDK_Up && key != GDK_KP_Up
      && key != GDK_Down && key != GDK_KP_Down
      && key != GDK_Page_Up && key != GDK_KP_Page_Up
      && key != GDK_Page_Down && key != GDK_KP_Page_Down
      && key != GDK_Home && key != GDK_KP_Home
      && key != GDK_End && key != GDK_KP_End
      && key != GDK_Control_L && key != GDK_Control_R
      && key != GDK_Shift_L && key != GDK_Shift_R
      && key != GDK_Meta_L && key != GDK_Meta_R
      && key != GDK_Alt_L && key != GDK_Alt_R
      && key != GDK_Super_L && key != GDK_Super_R
      && key != GDK_Hyper_L && key != GDK_Hyper_R)
    {
      /* Experimental code. */
#if 0
      g_print ("Set label? widget=%s\n",
	       mouse_over_widget ? gtk_widget_get_name (mouse_over_widget) : "NULL");
#endif
      if (mouse_over_widget
	  && (GTK_IS_ACCEL_LABEL (mouse_over_widget)
	      || GTK_IS_LABEL (mouse_over_widget)
	      || GTK_IS_BUTTON (mouse_over_widget)))
	{
	  gb_widget_show_properties (mouse_over_widget);
	  property_redirect_key_press (event);
	  handled = TRUE;
	}
    }
#endif

  if (handled)
    {
#if 0
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
#endif
      return TRUE;
    }
  return FALSE;
}


static gint
editor_on_key_release_event (GtkWidget * widget, GdkEventKey * event,
			     gpointer data)
{
  MSG ("In on_key_release_event");

  return FALSE;
}



/* This is when the 'Select' menuitem on the popup menu is selected */
void
editor_on_select_activate (GtkWidget * menuitem, GtkWidget * widget)
{
  editor_select_widget (widget, NULL, 0, 0);
}



void
editor_on_delete ()
{
  if (selected_widgets)
    delete (selected_widgets->data);
}


/* This is when the 'Cut' menuitem on the popup menu is selected */
void
editor_on_cut_activate (GtkWidget * menuitem, GtkWidget * widget)
{
  glade_clipboard_cut (GLADE_CLIPBOARD (glade_clipboard), current_project,
		       widget);
}


/* This is when the 'Copy' menuitem on the popup menu is selected */
void
editor_on_copy_activate (GtkWidget * menuitem, GtkWidget * widget)
{
  glade_clipboard_copy (GLADE_CLIPBOARD (glade_clipboard), current_project,
			widget);
}


/* This is when the 'Cut' menuitem on the popup menu is selected */
void
editor_on_paste_activate (GtkWidget * menuitem, GtkWidget * widget)
{
  glade_clipboard_paste (GLADE_CLIPBOARD (glade_clipboard), current_project,
			 widget);
}


/* This is when the 'Delete' menuitem on the popup menu is selected */
void
editor_on_delete_activate (GtkWidget * menuitem, GtkWidget * widget)
{
  delete (widget);
}


static void
delete (GtkWidget * widget)
{
  if (GB_IS_PLACEHOLDER (widget))
    delete_placeholder (widget);
  else
    editor_delete_widget (widget);
}


static void
delete_placeholder (GtkWidget * placeholder)
{
  GtkWidget *parent = placeholder->parent;
  gchar *child_name;

  /* SPECIAL CODE: Don't allow placeholders in clist titles to be deleted. */
  child_name = gb_widget_get_child_name (placeholder);
  if (child_name)
    {
      if (!strcmp (child_name, GladeChildCListTitle))
	{
	  MSG1 ("Not deleting special widget: %s\n", child_name);
	  return;
	}
    }

  /* SPECIAL CODE: Don't allow placeholder in BonoboDock to be deleted. */
#ifdef USE_GNOME	  
  if (BONOBO_IS_DOCK (parent))
    return;
#endif

  /* Remove widget from the selection */
  editor_clear_selection (NULL);

  /* Can't delete children of a paned or a viewport */
  if (GTK_IS_PANED (parent) || GTK_IS_VIEWPORT (parent))
    return;

  /* For a Clist, we can delete everything except column title widgets */
  if (GTK_IS_CLIST (parent))
    {
      g_warning ("Deleting a widget in a clist - not implemented yet");
      return;
    }

  /* If the parent is a toolitem, delete that. */
  if (GTK_IS_TOOL_ITEM (parent))
    {
      gtk_container_remove (GTK_CONTAINER (parent->parent), parent);
      return;
    }

  /* Widgets with these parents can all be deleted OK */
  if (GTK_IS_TOOLBAR (parent)
#if GLADE_SUPPORTS_GTK_TREE
      || GTK_IS_TREE (parent)
#endif
      || GTK_IS_LIST (parent))
    {
      gtk_widget_destroy (placeholder);
      return;
    }

  /* For a frame we can delete a placeholder in the label widget on its own.
     Otherwise we delete the parent, just like GtkBin. */
  if (GTK_IS_FRAME (parent))
    {
      if (gtk_frame_get_label_widget (GTK_FRAME (parent)) == placeholder)
	gtk_widget_destroy (placeholder);
      else
	editor_delete_widget (parent);
      return;
    }

  /* For these widgets replace the parent with a placeholder, or delete the
     component if parent is a toplevel widget */
  if (GTK_IS_BIN (parent) || GTK_IS_BUTTON (parent))
    {
      editor_delete_widget (parent);
      return;
    }

  /* For a box, if the placeholder is the only child then replace the box with
     a placeholder, else just delete the placeholder */
  if (GTK_IS_BOX (parent))
    {
      if (g_list_length (GTK_BOX (parent)->children) == 1)
	{
	  editor_delete_widget (parent);
	}
      else
	{
	  gtk_container_remove (GTK_CONTAINER (parent), placeholder);
	  /* Shouldn't really need to do this */
	  gtk_widget_queue_resize (parent);
	}
      return;
    }

  /* For a notebook, if placeholder is the only page, replace the notebook with
     a placeholder, else delete the current notebook page (i.e. placeholder) */
  if (GTK_IS_NOTEBOOK (parent))
    {
      if (g_list_length (GTK_NOTEBOOK (parent)->children) == 1)
	{
	  editor_delete_widget (parent);
	}
      else
	{
	  gtk_notebook_remove_page (GTK_NOTEBOOK (parent), gtk_notebook_get_current_page (GTK_NOTEBOOK (parent)));
	}
      return;
    }

  /* For a table, can't delete placeholder, unless there is only 1 row or
     column. In this case delete the placeholder, and move all the other
     children up/left. If the table is 1 x 1 then delete the table. */
  if (GTK_IS_TABLE (parent))
    {
      gint nrows, ncols, position = 0, distance_to_move = 0;
      GList *item;
      GtkTableChild *table_child;

      nrows = GTK_TABLE (parent)->nrows;
      ncols = GTK_TABLE (parent)->ncols;
      if (nrows > 1 && ncols > 1)
	return;
      if (nrows == 1 && ncols == 1)
	{
	  editor_delete_widget (parent);
	  return;
	}

      /* Find out where placeholder is */
      item = GTK_TABLE (parent)->children;
      while (item)
	{
	  table_child = (GtkTableChild *) item->data;

	  if (table_child->widget == placeholder)
	    {
	      /* Calculate how far up/left we will have to move the rest of the
	         children */
	      if (nrows == 1)
		{
		  position = table_child->left_attach;
		  distance_to_move = table_child->right_attach
		    - table_child->left_attach;
		}
	      else
		{
		  position = table_child->top_attach;
		  distance_to_move = table_child->bottom_attach
		    - table_child->top_attach;
		}
	      break;
	    }
	  item = item->next;
	}
      /* Shouldn't reach the end of the list */
      g_return_if_fail (item != NULL);
      gtk_widget_destroy (placeholder);

      /* Now step through the table again, moving children up or left */
      item = GTK_TABLE (parent)->children;
      while (item)
	{
	  table_child = (GtkTableChild *) item->data;
	  if (nrows == 1)
	    {
	      if (table_child->left_attach > position)
		{
		  table_child->left_attach -= distance_to_move;
		  table_child->right_attach -= distance_to_move;
		}
	    }
	  else
	    {
	      if (table_child->top_attach > position)
		{
		  table_child->top_attach -= distance_to_move;
		  table_child->bottom_attach -= distance_to_move;
		}
	    }
	  item = item->next;
	}

      /* Now update the tables nrows & ncols */
      if (nrows == 1)
	GTK_TABLE (parent)->ncols -= distance_to_move;
      else
	GTK_TABLE (parent)->nrows -= distance_to_move;

      return;
    }

  g_warning ("Don't know how to delete widget");
}


void
editor_delete_widget (GtkWidget * widget)
{
  GtkWidget *parent, *placeholder;
  gchar *error;

  MSG1 ("In editor_delete_widget: %s\n", gtk_widget_get_name (widget));

  error = editor_can_delete_widget (widget);
  if (error)
    {
      glade_util_show_message_box (error, widget);
      return;
    }

  /* If we are deleting a GtkTextView set the text to "". This avoids an odd
     crash. See bug #111604. */
  if (GTK_IS_TEXT_VIEW (widget))
    gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)), "", 0);

  /* Set properties widget to NULL, in case the widget or parent is deleted */
  property_set_widget (NULL);

  /* Remove widget from the selection */
  editor_clear_selection (NULL);

  if (GTK_IS_MENU (widget))
    parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
  else
    parent = widget->parent;

  /* If widget is a toplevel widget (i.e. a project component) delete the
     component. */
  if (parent == NULL)
    {
      glade_project_remove_component (current_project, widget);
      return;
    }

  /* If the widget's parent is a fixed container or a packer remove the widget
     completely. */
  if (GTK_IS_FIXED (widget->parent)
#if GLADE_SUPPORTS_GTK_PACKER
      || GTK_IS_PACKER (widget->parent)
#endif
      || GTK_IS_LAYOUT (widget->parent))
    {
      gtk_widget_destroy (widget);
      return;
    }

  /* If the widget's parent is a button box remove widget completely. */
  if (GTK_IS_BUTTON_BOX (widget->parent))
    {
      gtk_widget_destroy (widget);
      return;
    }

  /* If the widget is a menu item remove widget completely. */
  if (GTK_IS_MENU_ITEM (widget))
    {
      gtk_widget_destroy (widget);
      return;
    }

#ifdef USE_GNOME
  /* GnomeDockItem widgets are also removed completely rather than being
     replaced by a placeholder. */
  if (BONOBO_IS_DOCK_ITEM (widget))
    {
      gtk_widget_destroy (widget);
      return;
    }

  /* If this is a page in a GnomeDruid, then if it is the only page delete
     the entire GnomeDruid, else delete the page. We need to make sure the
     current page is set to something else before deleting the page. */
  if (GNOME_IS_DRUID_PAGE (widget))
    {
      gint num_pages;
      GList *children, *elem;
      GnomeDruidPage *new_current_page;

      g_return_if_fail (GNOME_IS_DRUID (parent));

      children = gtk_container_get_children (GTK_CONTAINER (parent));
      num_pages = g_list_length (children);
      if (num_pages == 1)
	{
	  editor_delete_widget (parent);
	}
      else
	{
	  elem = g_list_find (children, widget);
	  g_return_if_fail (elem != NULL);

	  if (elem->next)
	    new_current_page = elem->next->data;
	  else
	    new_current_page = elem->prev->data;

	  gnome_druid_set_page (GNOME_DRUID (parent), new_current_page);

	  gtk_widget_destroy (widget);
	}

      g_list_free (children);

      return;
    }
#endif

  /* Replace normal widget's with a placeholder  & select it so it can also
     be deleted easily using the Delete key. But we must be careful since
     there is a slight chance that the placeholder will be automatically
     destroyed, e.g. if it is placed in a table which already has another
     widget in the same position. */
  placeholder = editor_new_placeholder ();
  gtk_widget_ref (placeholder);
  if (gb_widget_replace_child (widget->parent, widget, placeholder))
    {
      if (placeholder->parent)
	editor_select_widget (placeholder, NULL, 0, 0);
      gtk_widget_unref (placeholder);
    }
  else
    {
      glade_util_show_message_box (_("Couldn't delete widget."), widget);
      gtk_object_sink (GTK_OBJECT (placeholder));
      gtk_widget_unref (placeholder);
    }

  MSG ("Out editor_delete_widget");
}


/* This sees if a widget can be deleted. It returns an appropriate error
   message if it can't. */
gchar*
editor_can_delete_widget (GtkWidget * widget)
{
  gchar *child_name;

  /* Button & item children are special - they can be deleted. */
  if (widget->parent && GB_IS_GB_WIDGET (widget->parent)
      && (GTK_IS_BUTTON (widget->parent) || GTK_IS_ITEM (widget->parent)))
    return NULL;

  /* Don't allow widgets which aren't GbWidgets to be deleted, since we know
     nothing about them. */
  if (!GB_IS_PLACEHOLDER (widget) && !GB_IS_GB_WIDGET (widget))
    return _("The widget can't be deleted");

  /* Non-toplevel menus are created automatically so we can't delete them. */
  if (GTK_IS_MENU (widget) && gtk_menu_get_attach_widget (GTK_MENU (widget)))
    return _("The widget can't be deleted");


  /* SPECIAL CODE: Don't allow dialog buttons & widgets to be deleted. */
  child_name = gb_widget_get_child_name (widget);
  if (child_name)
    {
      if (!strcmp (child_name, GladeChildOKButton)
	  || !strcmp (child_name, GladeChildCancelButton)
	  || !strcmp (child_name, GladeChildHelpButton)
	  || !strcmp (child_name, GladeChildApplyButton)
	  || !strcmp (child_name, GladeChildSaveButton)
	  || !strcmp (child_name, GladeChildCloseButton)
	  || !strcmp (child_name, GladeChildDialogVBox)
	  || !strcmp (child_name, GladeChildDialogActionArea)
	  || !strcmp (child_name, GladeChildComboEntry)
	  || !strcmp (child_name, GladeChildComboList)
	  || !strcmp (child_name, GladeChildFontSelection)
	  || !strcmp (child_name, GladeChildColorSelection)
#ifdef USE_GNOME
	  || !strcmp (child_name, GladeChildGnomeAppDock)
	  || !strcmp (child_name, GladeChildGnomeAppBar)
	  || !strcmp (child_name, GladeChildGnomeDruidVBox)
	  || !strcmp (child_name, GladeChildGnomeEntry)
#endif
	  )
	{
	  return _("The widget is created automatically as part of the parent widget, and it can't be deleted.");
	}
    }

  return NULL;
}


/* Called when a GbWidget is destroyed so the editor can remove any references
   to it. */
void
editor_on_widget_destroyed		(GtkWidget	    *widget)
{
#if 0
  const char *name = gtk_widget_get_name (widget);
  g_print ("In editor_on_widget_destroyed: %s\n", name ? name : "NULL");
#endif

  if (mouse_over_widget == widget)
    {
#if 0
      g_print ("  resetting mouse_over_widget to NULL\n");
#endif
      mouse_over_widget = NULL;
    }
}

