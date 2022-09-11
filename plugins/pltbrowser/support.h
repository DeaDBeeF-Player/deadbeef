/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __GTKUI_SUPPORT_H
#define __GTKUI_SUPPORT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(3,0,0)
#include <gdk/gdkkeysyms-compat.h>
#define gdk_cursor_unref(cursor) g_object_unref(cursor)
#else
#include <gdk/gdkkeysyms.h>
#endif

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  define Q_(String) g_strip_context ((String), gettext (String))
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#ifndef _
#  define _(String) (String)
#endif
#  define Q_(String) g_strip_context ((String), (String))
#  define N_(String) (String)
#endif


/*
 * Public Functions.
 */

/*
 * This function returns a widget in a component created by Glade.
 * Call it with the toplevel widget in the component (i.e. a window/dialog),
 * or alternatively any widget in the component, and the name of the widget
 * you want returned.
 */
GtkWidget*  lookup_widget              (GtkWidget       *widget,
                                        const gchar     *widget_name);


/* Use this function to set the directory containing installed pixmaps. */
void        add_pixmap_directory       (const gchar     *directory);


/*
 * Private Functions.
 */

/* This is used to create the pixmaps used in the interface. */
GtkWidget*  create_pixmap              (GtkWidget       *widget,
                                        const gchar     *filename);

/* This is used to create the pixbufs used in the interface. */
GdkPixbuf*  create_pixbuf              (const gchar     *filename);

/* This is used to set ATK action descriptions. */
void        glade_set_atk_action_description (AtkAction       *action,
                                              const gchar     *action_name,
                                              const gchar     *description);

#if GTK_CHECK_VERSION(3,0,0)
GtkWidget *
gtk_combo_box_entry_new_text(void);

void
gtk_dialog_set_has_separator (GtkDialog *dlg, gboolean has);
#endif

#if !GTK_CHECK_VERSION(2,20,0)
#define gtk_widget_set_realized(widget, realized) {if (realized) GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED); else GTK_WIDGET_UNSET_FLAGS(widget, GTK_REALIZED);}
#define gtk_widget_get_realized(widget) (GTK_WIDGET_REALIZED (widget))
#endif

#if !GTK_CHECK_VERSION(2,22,0)
GdkDragAction
gdk_drag_context_get_selected_action (GdkDragContext *context);
GList *
gdk_drag_context_list_targets (GdkDragContext *context);
#endif

#if !GTK_CHECK_VERSION(2,24,0)
#define GTK_COMBO_BOX_TEXT GTK_COMBO_BOX
typedef GtkComboBox GtkComboBoxText;
GtkWidget *gtk_combo_box_text_new ();
GtkWidget *gtk_combo_box_text_new_with_entry   (void);
void gtk_combo_box_text_append_text (GtkComboBoxText *combo_box, const gchar *text);
void gtk_combo_box_text_insert_text (GtkComboBoxText *combo_box, gint position, const gchar *text);
void gtk_combo_box_text_prepend_text (GtkComboBoxText *combo_box, const gchar *text);
gchar *gtk_combo_box_text_get_active_text  (GtkComboBoxText *combo_box);
#endif

#if !GTK_CHECK_VERSION(2,14,0)
#define gtk_widget_get_window(widget) ((widget)->window)
#define gtk_selection_data_get_target(data) (data->target)
#define gtk_dialog_get_content_area(dialog) (dialog->vbox)
#define gtk_dialog_get_action_area(dialog) (dialog->action_area)
#define gtk_selection_data_get_data(data) (data->data)
#define gtk_selection_data_get_length(data) (data->length)
#define gtk_selection_data_get_format(data) (data->format)
#define gtk_adjustment_get_lower(adj) (adj->lower)
#define gtk_adjustment_get_upper(adj) (adj->upper)
#endif

#if !GTK_CHECK_VERSION(2,18,0)
#define gtk_widget_set_has_window(widget, has_window) \
  if (has_window) GTK_WIDGET_UNSET_FLAGS (widget, GTK_NO_WINDOW); \
  else GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);

#define gtk_widget_get_visible(widget) (GTK_WIDGET_VISIBLE(widget))
#define gtk_widget_get_has_window(widget) (!GTK_WIDGET_NO_WINDOW(widget))
void gtk_widget_set_window(GtkWidget *widget, GdkWindow *window);
#endif

#if !GTK_CHECK_VERSION(2,20,0)
#define gtk_widget_get_mapped(widget) (GTK_WIDGET_MAPPED(widget))
#endif

#if !GTK_CHECK_VERSION(2,18,0)
void                gtk_widget_set_allocation           (GtkWidget *widget,
                                                         const GtkAllocation *allocation);

void                gtk_widget_get_allocation           (GtkWidget *widget,
                                                         GtkAllocation *allocation);

#define gtk_widget_set_can_focus(widget, canfocus) {if (canfocus) GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS); else GTK_WIDGET_UNSET_FLAGS(widget, GTK_CAN_FOCUS);}

#define gtk_widget_get_can_focus(widget) (GTK_WIDGET_CAN_FOCUS (widget))

#define gtk_widget_has_focus(widget) (GTK_WIDGET_HAS_FOCUS (widget))

#define gtk_widget_set_can_default(widget, candefault) {if (candefault) GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_DEFAULT); else GTK_WIDGET_UNSET_FLAGS(widget, GTK_CAN_DEFAULT);}

#define gtk_widget_get_can_default(widget) (GTK_WIDGET_CAN_DEFAULT (widget));
#endif

#endif


#if GTK_CHECK_VERSION(3,2,0)
#define gtk_vbox_new(homogeneous,spacing) g_object_new(GTK_TYPE_VBOX,"spacing",spacing,"homogeneous",homogeneous?TRUE:FALSE,NULL)
#define gtk_hbox_new(homogeneous,spacing) g_object_new(GTK_TYPE_HBOX,"spacing",spacing,"homogeneous",homogeneous?TRUE:FALSE,NULL)
#define gtk_hpaned_new() gtk_paned_new(GTK_ORIENTATION_HORIZONTAL)
#define gtk_vpaned_new() gtk_paned_new(GTK_ORIENTATION_VERTICAL)
#define gtk_hbutton_box_new() gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL)
#define gtk_vbutton_box_new() gtk_button_box_new(GTK_ORIENTATION_VERTICAL)
#define gtk_hscale_new(adj) gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,adj)
#define gtk_vscale_new(adj) gtk_scale_new(GTK_ORIENTATION_VERTICAL,adj)
#define gtk_hseparator_new() gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)
#define gtk_vseparator_new() gtk_separator_new(GTK_ORIENTATION_VERTICAL)
#define gtk_hscrollbar_new(adj) gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL,adj)
#define gtk_vscrollbar_new(adj) gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL,adj)
#endif

#ifdef __APPLE__
#define TEST_LEFT_CLICK(ev) (ev->button==1 && !TEST_RIGHT_CLICK(ev))
#define TEST_RIGHT_CLICK(ev) (ev->button==3 || (ev->button==1 && (ev->state&(GDK_CONTROL_MASK|GDK_BUTTON3_MASK))))
#else
#define TEST_LEFT_CLICK(ev) (ev->button==1)
#define TEST_RIGHT_CLICK(ev) (ev->button==3)
#endif
