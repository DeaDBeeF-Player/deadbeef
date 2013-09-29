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
#ifndef GLADE_GBSOURCE_H
#define GLADE_GBSOURCE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* These identify each of the source code buffers. New buffers can easily be
   added by increasing GLADE_NUM_SOURCE_BUFFERS and adding an id here. */
#define GLADE_NUM_SOURCE_BUFFERS 9
typedef enum
{
  GLADE_UIINFO                = 0,	/* GnomeUIInfo structs, which are
					   output before the function as
					   static structs. */
  GLADE_DECLARATIONS          = 1,	/* The declarations of the widgets
					   and temporary variables in the
					   function to create a component. */
  GLADE_SOURCE                = 2,	/* The main code which creates the
					   widgets in a component. */
  GLADE_SIGNAL_CONNECTIONS    = 3,	/* Code to connect signal handlers. */
  GLADE_ACCELERATORS          = 4,	/* Code to setup accelerator keys, used
					   when the target widget is different
					   from the source (i.e. the label). */

  GLADE_CALLBACK_DECLARATIONS = 5,	/* Declarations of signal handler
					   functions and callbacks. */
  GLADE_CALLBACK_SOURCE       = 6,	/* Source code of signal handlers or
					   callbacks. */
  GLADE_OBJECT_HOOKUP         = 7,	/* Code to hookup pointers to widgets.
					   We put it all together so it can be
					   deleted easily if the user just
					   wants to pinch the source code. */
  GLADE_ATK_SOURCE	      = 8	/* Code to set ATK properties, action
					   descriptions and relations. */
} GladeSourceBuffer;


typedef struct 
{
  GladeProject  *project;
  GladeError	*error;			/* If an error occurs this will be
					   non-NULL, and will contain info
					   about the error. Note that we
					   abort the source generation as
					   soon as an error is encountered,
					   so there can only be one error. */
  gchar		*project_name;
  gchar		*program_name;
  gchar		*interface_c_filename;	/* The full path of the source file
					   which creates the interface. */ 
  gchar		*interface_h_filename;	/* The full path of the main header. */
  gchar		*callback_c_filename;	/* The full path of the source file
					   where the empty callbacks/signal
					   handlers are added. */
  gchar		*callback_h_filename;	/* The full path of callback header. */
  gboolean	 set_widget_names;	/* If gtk_widget_set_name (...) is
					   output to set the name of each
					   widget - useful for rc files. */
  gboolean	 use_component_struct;	/* TRUE if a struct is used to keep
					   pointers to all public widgets in a
					   component. (This is unfinished).
					   FALSE for the old behaviour of
					   using gtk_object_set_data () to
					   save pointers in the toplevel
					   widget's datalist. */
  gboolean	 use_gettext;		/* If strings in the source code are
					   marked with the standard gettext
					   macros _() or N_() so they can be
					   translated easily. */
  gboolean	 creating_callback_files;/*TRUE if we are creating the files
					   containing the signal handlers and
					   callback functions. FALSE if we are
					   just appending new/updated
					   handlers and callbacks. */
  time_t	 last_write_time;	/* The last time that the source was
					   written. We need this to determine
					   exactly which signal handlers have
					   been added or updated. We get it
					   from one of the main source files.
					*/

  gboolean	 need_tooltips;		/* TRUE if any widgets in a component
					   have a tooltip set. If they do then
					   a GtkTooltips object has to be
					   created. */
  gboolean	 need_accel_group;	/* TRUE if any widgets have accelerator
					   keys set. If they do then an
					   accel_group needs to be created. */
  gchar		*wname;			/* The C code to use to refer to the
					   widget currently being written.
					   This is the the name of the
					   variable holding the widget (which
					   is the same as the widget's name),
					   or if we are using structs to hold
					   pointers to all widgets this will
					   be something like "window1->label1".
					   See use_component_struct above. */
  gchar		*real_wname;		/* This is the real name of the widget
					   currently being written, converted
					   so that it is a valid C identifier.
					*/
  GtkWidget	*component;		/* This is the component currently
					   being output (i.e. a window/dialog).
					*/
  gchar		*component_name;	/* This is the name of the component,
					   converted to a valid C identifier.
					*/
  GladeWidgetData *widget_data;		/* The GladeWidgetData of the widget
					   currently being output. */
  GtkWidget	*parent;		/* The parent widget of the widget
					   currently being output. */
  GHashTable	*standard_widgets;	/* A hash of GTK widgets used to
					   determine if values differ from
					   their default values. */
  GtkWidget	*standard_widget;	/* This is the standard widget
					   corresponding to the widget being
					   written. It is used so we only
					   set properties which differ from the
					   standard setting. (It may be NULL,
					   in which case all properties should
					   be set). */
  GHashTable	*handlers_output;	/* A hash of signal handlers we have
					   already output. */
  gboolean	 create_widget;		/* If TRUE then the code to create the
					   current widget needs to be output.
					   It is FALSE for widgets which are
					   created automatically by their
					   parents, e.g. filesel dialog
					   buttons. */
  gboolean	 write_children;	/* This is set to FALSE in a GbWidget
					   write_source() function to
					   indicate that the source code to
					   output the children has already
					   been written using explicit calls,
					   so the default recursive calls are
					   skipped. See gbwidgets/gbdialog.c */

  gchar		*focus_widget;		/* Set to the name of the widget to
					   grab the focus. */
  gchar		*default_widget;	/* Set to the name of the widget to
					   grab the default. We need to grab
					   the default after creating the
					   entire component, to overcome
					   problems with GnomeDialog. */

  /* These buffers hold the code being generated for the current component
     (window/dialog). When the component is finished they are all output
     at once in the correct order, and with other necessary pieces of code
     inserted. See source_write_component() in source.c. */
  GString	*source_buffers[GLADE_NUM_SOURCE_BUFFERS];
} GbWidgetWriteSourceData;


void	    gb_widget_write_source	(GtkWidget	       *widget,
					 GbWidgetWriteSourceData *data);
void	    gb_widget_write_standard_source
					(GtkWidget	       *widget,
					 GbWidgetWriteSourceData *data);
void	    gb_widget_write_add_child_source
					(GtkWidget	       *widget,
					 GbWidgetWriteSourceData *data);
void	    gb_widget_write_signal_handler_source
					(GtkWidget	       *widget,
					 GbWidgetWriteSourceData *data,
					 const gchar	       *signal_name,
					 const gchar	       *handler);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_GBSOURCE_H */
