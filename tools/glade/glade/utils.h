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
#ifndef GLADE_UTILS_H
#define GLADE_UTILS_H

#include <stdio.h>

#include <gtk/gtkbox.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtktable.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkobject.h>

#include "gbwidget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Days of the week, 'Sun' ... 'Sat'. */
extern gchar *GladeDayNames[];
/* Months of the year, 'Jan' ... 'Dec'. */
extern gchar *GladeMonthNames[];


/* This shows a simple dialog box with a label and an 'OK' button.
   If transient_widget is not NULL, then its toplevel window is found and used
   to set the transient_for property (this also works for menu widgets).

   Example usage:
    glade_util_show_message_box ("Error saving file", NULL);
 */
void	    glade_util_show_message_box	(const gchar	*message,
					 GtkWidget	*transient_widget);


/* This creates a dialog box with a message and a number of buttons.
 * Signal handlers can be supplied for any of the buttons.
 * NOTE: The dialog is automatically destroyed when any button is clicked.
 * default_button specifies the default button, numbered from 1..
 * data is passed to the signal handler.

   Example usage:
     gchar *buttons[] = { "Yes", "No", "Cancel" };
     GtkSignalFunc signal_handlers[] = { on_yes, on_no, NULL };
     glade_util_show_dialog ("Do you want to save the current project?",
			     3, buttons, 3, signal_handlers, NULL);
 */
GtkWidget*  glade_util_create_dialog_with_buttons (const gchar	*message,
						   gint		 nbuttons,
						   const gchar	*buttons[],
						   gint		 default_button,
						   GtkSignalFunc signal_handlers[],
						   gpointer	 data,
						   GtkWidget	*transient_widget);


/* This shows a dialog box with a message and an Entry for entering a value.
 * When the OK button is pressed the handler of the specified widget will
 * be called with the value and the given data.
 * NOTE: The dialog is automatically destroyed when any button is clicked.

   Example usage:
     glade_util_show_entry_dialog ("Name:", "default", widget, on_dialog_ok,
				   "NewName");

     void
     on_dialog_ok(GtkWidget *widget, gchar *value, gpointer data)
     {
     ...
 */
typedef gint (*GbEntryDialogFunc)	(GtkWidget	    *widget,
					 gchar		    *value,
					 gpointer	     data);

void	    glade_util_show_entry_dialog(const gchar	    *message,
					 const gchar	    *initial_value,
					 GtkWidget	    *widget,
					 GbEntryDialogFunc   signal_handler,
					 gpointer	     data,
					 GtkWidget	    *transient_widget);


/* This creates a dialog with OK & Cancel buttons, usable in plain GTK or
   Gnome, and returns a vbox which the caller can place widgets in.
   If transient_for is non-NULL, the window it is in is used to set the
   transient for relationship, so the dialog will always be above the widget.
   The callback will be called when the OK button is pressed. */
GtkWidget* glade_util_create_dialog	(const gchar	   *title,
					 GtkWidget	   *transient_for,
					 GtkSignalFunc      ok_handler,
					 gpointer           ok_handler_data,
					 GtkWidget	  **vbox);


typedef enum
{
  GladeEscCloses,
  GladeEscDestroys
} GladeEscAction;


/* This event handler performs the selected GladeEscAction when Esc is
   pressed in a dialog.
   It is not necessary for GnomeDialogs. */
gint glade_util_check_key_is_esc (GtkWidget *widget,
				  GdkEventKey *event,
				  gpointer data);


/* This returns a new entry ready to insert in a dialog.
   The entry is set up so that <Return> will invoke the default action. The
   returned widget must be added to a container in the dialog. */ 
extern GtkWidget *
glade_util_entry_new (GtkObject *dialog);


/* This returns a new spinbutton ready to insert in a dialog.
   A pointer to the spin button is added as object data to the dialog. The
   spinbutton is set up so that <Return> will invoke the default action. The
   returned widget must be added to a container in the dialog. */ 
extern GtkWidget *
glade_util_spin_button_new (GtkObject *dialog,
			    const gchar *key,
			    GtkAdjustment *adjustment,
			    gfloat climb_rate,
			    guint digits);


/* This returns the index of the given gint with the given array of gints.
   If the value is not found it outputs a warning and returns -1.
   This function is intended to be used for finding the index when using
   properties which are choices. */
gint	    glade_util_int_array_index	(const gint	    array[],
					 gint		    array_size,
					 gint		    value);

/* This returns the index of the given string with the given array of strings.
   If the value is not found it outputs a warning and returns -1.
   This function is intended to be used for finding the index when using
   properties which are choices. */
gint	    glade_util_string_array_index (const gchar	    *array[],
					   gint		     array_size,
					   const gchar	    *value);

/* This returns TRUE if the two strings are equivalent. Note that NULL is
   considered equivalent to the empty string. */
gboolean    glade_util_strings_equivalent (const gchar	    *string1,
					   const gchar	    *string2);
/* This returns a copy of the given string, or NULL if the string is NULL
   or empty, i.e. "". */
gchar*      glade_util_copy_string	(const gchar	    *string);
/* Returns TRUE if string contains substring. Returns FALSE if substring is
   not found or is NULL. */
gboolean    glade_util_strstr		(const gchar	    *string,
					 const gchar	    *substring);

gchar*	    glade_util_find_start_of_tag_name (const gchar * tag_name);

gchar*	    glade_util_create_modifiers_string (guint8 modifier_flags);
	
/* parse a string in the form of "GTK_ANCHOR_NE|GTK_ANCHOR_SW" */
guint       glade_util_flags_from_string (GType type, const char *string);
gchar*      glade_util_string_from_flags (GType type, guint flags);

const char *glade_string_from_enum (GType type, gint value);

gint        glade_enum_from_string (GType type, const char *string);


/* This creates a GtkPixmap widget, using a colormap and xpm data from an
   '#include'd pixmap file. */
GtkWidget*  glade_util_create_pixmap_using_colormap (GdkColormap *colormap,
						     gchar **xpm_data);

/* This returns TRUE if the widget is a toplevel project component,
   i.e. a window, dialog or popup menu. */
gboolean    glade_util_is_component	(GtkWidget	   *widget);

/* This returns the toplevel widget containing the given widget. It is similar
   to gtk_widget_get_toplevel() but is also walks up menus. */
GtkWidget*  glade_util_get_toplevel	(GtkWidget	   *widget);

/* This returns the closest ancestor of the given widget which is a GbWidget.*/
GtkWidget*  glade_util_get_parent	(GtkWidget	   *widget);

/* This tries to find a named widget in a component. It returns the widget or
   NULL if it wasn't found. */
GtkWidget* glade_util_find_widget (GtkWidget *widget, gchar *name);


/* This is used when setting up keyboard accelerators resulting from underlined
   keys in labels. It returns the mnemonic widget to use. */
GtkWidget* glade_util_find_default_accelerator_target (GtkWidget      *label);

/* Returns the index of a widget in a box. Note that the box packing
   property means that widgets may not be displayed in this order. */
gint	    glade_util_get_box_pos	(GtkBox		   *box,
					 GtkWidget	   *widget);

/* Returns the structure corresponding to the given widget in a table. */
GtkTableChild* glade_util_find_table_child	(GtkTable	   *table,
						 GtkWidget	   *widget);
GtkBoxChild* glade_util_find_box_child		(GtkBox		   *box,
						 GtkWidget	   *widget);
GtkFixedChild* glade_util_find_fixed_child	(GtkFixed	   *fixed,
						 GtkWidget	   *widget);
#if GLADE_SUPPORTS_GTK_PACKER
GtkPackerChild* glade_util_find_packer_child	(GtkPacker	   *packer,
						 GtkWidget	   *widget);
#endif

/* This returns the GtkLabel's text, including the underline characters.
   It is needed since GtkLabel doesn't provide the opposite function to
   gtk_label_parse_uline(). The returned string should be freed after use. */
gchar*	    glade_util_get_label_text		(GtkWidget	*label);

/* These close the given window, so that if it is shown again it appears in
   the same place. */
gint	  glade_util_close_window_on_delete	(GtkWidget	*widget,
						 GdkEvent	*event,
						 gpointer	 data);
gint	  glade_util_close_window		(GtkWidget	*widget);



/*
 * File Utility Functions.
 */

/* Returns TRUE if the given file exists. filename must be UTF-8. */
gboolean    glade_util_file_exists		(const gchar *filename);

/* Returns the last modification time of the given file, or 0 if it doesn't
   exist, or -1 on error. filename must be UTF-8. */
GladeError* glade_util_file_last_mod_time	(const gchar *filename,
						 time_t	     *last_mod_time);

/* This copies a file from src to dest, and returns a GladeError if an error
   occurs. src & dest must be UTF-8. */
GladeError* glade_util_copy_file		(const gchar *src,
						 const gchar *dest);

/* Creates a directory if it doesn't already exist. */
GladeError* glade_util_ensure_directory_exists	(const gchar *directory);

/*
 * Filename Utility Functions.
 */

/* Adds a filename onto a directory to make a complete pathname.
   The directory may or may not end in '/'. file must be a simple filename.
   Free the returned string when no longer needed. */
gchar*	  glade_util_make_path		(const gchar	    *dir,
					 const gchar	    *file);

/* This turns a relative pathname into an absolute one based on the given
   base directory (which MUST be absolute).
   e.g. "/home/damon" + "../dave/test" -> "/home/dave/test"
   The returned path should be freed when no longer needed. */
gchar*	  glade_util_make_absolute_path	(const gchar	    *dir,
					 const gchar	    *file);

/* This turns an absolute pathname into an relative one based on the given
   base directory. Both arguments must be absolute paths, and should be in
   canonical form, i.e. not containing '..', '.' or multiple '/'s together.
   The returned value may or may not end with a '/', depending on the
   arguments. The returned path should be freed when no longer needed. */
gchar*	  glade_util_make_relative_path	(const gchar	    *base_dir,
					 const gchar	    *file);

/* Returns TRUE if file is in dir. Both paths must be absolute. file can
   be a directory, and both can end in '/' or not. Note that we assume
   that both are in proper form, i.e. there are no instances of '//', '.',
   or '..' in either. */
gboolean  glade_util_directory_contains_file (const gchar	    *dir,
					      const gchar	    *file);

/* Returns TRUE if the 2 directories are equivalent. Both must be absolute
   paths, and may or may not end in '/'. */
gboolean  glade_util_directories_equivalent  (const gchar	    *dir1,
					      const gchar	    *dir2);


/* This is similar to GLib's dirname, but it makes sure the dirname ends with
   a G_DIR_SEPARATOR. */
gchar*    glade_util_dirname		(const gchar	    *file_name);

/* This returns the parent directory of the given directory, which may or may
   not end in a G_DIR_SEPARATOR. The returned string should be freed. */
gchar*    glade_util_parent_directory	(const gchar	    *dir);


/* This searches the $HOME/Projects directory to find the default directory to
   use for the next project, e.g. $HOME/Projects/project1. The returned
   directory should be freed when no longer needed. */
GladeError* glade_util_get_next_free_project_directory (gchar **project_directory_return,
							gint   *project_num_return);


/* Sets the TZ environment variable to the given value, e.g. "UTC", returning
   the old setting.
   NOTE: You must call glade_util_reset_timezone() some time later to restore
   the original TZ. Pass glade_util_reset_timezone() the string that
   glade_util_set_timezone() returns. */
gchar*	  glade_util_set_timezone		(const gchar	*tz);
void	  glade_util_reset_timezone		(gchar		*tz);


gboolean  glade_util_check_is_stock_id		(const gchar	*stock_id);

/* This is a GCompareFunc for comparing the labels of 2 stock items, ignoring
   any '_' characters. It isn't particularly efficient. */
gint	  glade_util_compare_stock_labels	(gconstpointer	 a,
						 gconstpointer	 b);


/* FIXME: These are pinched from gtkcombo.c. Get rid of them. */
/* GtkListItem *glade_util_gtk_combo_find	(GtkCombo	*combo);*/
/*gchar	 *glade_util_gtk_combo_func		(GtkListItem	*listitem);*/
gpointer  glade_util_gtk_combo_find		(GtkCombo	*combo);
gchar	 *glade_util_gtk_combo_func		(gpointer	 listitem);

/* Returns TRUE if we need to support a 'Border Width' property for a widget.*/
gboolean  glade_util_uses_border_width		(GtkWidget	*widget);

/* Converts a filename from UTF-8 to on-disk encoding, and sets it in a
   GtkFileSelection. */
void	  glade_util_set_file_selection_filename(GtkWidget	*filesel,
						 const gchar	*filename_utf8);
/* Gets the selected file in a GtkFileSelection, converts it to UTF-8 and
   returns it. Note that the returned string must be freed. */
gchar*	  glade_util_get_file_selection_filename(GtkWidget *filesel);

/* Like fopen but takes a UTF-8 filename and converts to on-disk encoding. */
FILE*	  glade_util_fopen			(const gchar	*filename_utf8,
						 const gchar	*mode);

/* Gets a list of file names from a text/uri-list */
GList	 *glade_util_uri_list_parse		(const gchar	*uri_list);

/* Gets the extra translation properties stored inside the widget data. */
void	  glade_util_get_translation_properties (GtkWidget	*widget,
						 const gchar	*property_name,
						 gboolean	*translatable,
						 gchar	       **comments,
						 gboolean	*context);

/* Sets the extra translation properties stored inside the widget data. */
void	  glade_util_set_translation_properties (GtkWidget	*widget,
						 const gchar	*property_name,
						 gboolean	 translatable,
						 const gchar	*comments,
						 gboolean	 context);

/* Copies the extra translation properties stored inside the widget data. */
void	  glade_util_copy_translation_properties(GtkWidget	*from_widget,
						 const gchar	*from_property_name,
						 GtkWidget	*to_widget,
						 const gchar	*to_property_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_UTILS_H */
