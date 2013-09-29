/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-1999  Damon Chaplin
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
#ifndef GLADE_H
#define GLADE_H

#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* This is the main header file for the Glade library. */

/* Error codes returned by Glade functions. */
typedef enum
{
  GLADE_STATUS_OK			= 0,

  /* General error, when nothing else is suitable. */
  GLADE_STATUS_ERROR			= 1,

  /* System error. In a GladeError, system_errno will hold the error code. */
  GLADE_STATUS_SYSTEM_ERROR		= 2,

  /* File related errors. */
  GLADE_STATUS_FILE_OPEN_ERROR		= 11,
  GLADE_STATUS_FILE_READ_ERROR		= 12,
  GLADE_STATUS_FILE_WRITE_ERROR		= 13,

  GLADE_STATUS_INVALID_DIRECTORY	= 15,

  /* XML Parsing errors. */
  GLADE_STATUS_INVALID_VALUE		= 20,
  GLADE_STATUS_INVALID_ENTITY		= 21,
  GLADE_STATUS_START_TAG_EXPECTED	= 22,
  GLADE_STATUS_END_TAG_EXPECTED		= 23,
  GLADE_STATUS_DATA_EXPECTED		= 24,
  GLADE_STATUS_CLASS_ID_MISSING		= 25,
  GLADE_STATUS_CLASS_UNKNOWN		= 26,
  GLADE_STATUS_INVALID_COMPONENT	= 27,
  GLADE_STATUS_EOF			= 28
} GladeStatusCode;


/* This is the global clipboard. */
extern GtkWidget *glade_clipboard;

typedef struct _GladeWidgetInitData	GladeWidgetInitData;
typedef struct _GladePaletteSectionData	GladePaletteSectionData;

typedef struct _GladeProject	GladeProject;
typedef struct _GbWidget	GbWidget;

struct _GladeWidgetInitData
{
  gchar *name;
  GbWidget* (*init_func)();
};

struct _GladePaletteSectionData
{
  gchar *section;
  GladeWidgetInitData *widgets;
};


void	    glade_app_init				(void);

void	    glade_load_settings		(gpointer	 project_window,
					 GtkWidget	*palette,
					 gboolean	*show_palette,
					 GtkWidget	*property_editor,
					 gboolean	*show_property_editor,
					 GtkWidget	*widget_tree,
					 gboolean	*show_widget_tree,
					 GtkWidget	*clipboard,
					 gboolean	*show_clipboard);
void	    glade_save_settings		(gpointer	 project_window,
					 GtkWidget	*palette,
					 GtkWidget	*property_editor,
					 GtkWidget	*widget_tree,
					 GtkWidget	*clipboard);
/*
GladeProject* glade_project_new (void);
GladeProject* glade_project_open (gchar *filename);
GladeStatusCode glade_project_save (GladeProject *project);
GladeStatusCode glade_project_set_interface_filename (GladeProject *project);
GladeStatusCode glade_project_set_graphics_directory (GladeProject *project);
*/

void	    glade_show_project_window		(void);
void	    glade_hide_project_window		(void);

void	    glade_show_palette			(void);
void	    glade_hide_palette			(void);

void	    glade_show_property_editor		(void);
void	    glade_hide_property_editor		(void);

void	    glade_show_widget_tree		(void);
void	    glade_hide_widget_tree		(void);

void	    glade_show_clipboard		(void);
void	    glade_hide_clipboard		(void);

void	    glade_show_widget_tooltips		(gboolean	show);

void	    glade_show_grid			(gboolean	show);
void	    glade_snap_to_grid			(gboolean	snap);
#if 0
/* Changed editor_show_grid_settings_dialog and
   editor_show_grid_settings_dialog to take a widget parameter, to use for
   selecting a transient parent.
   These functions don't know about any widgets. Since they're unused,
   they're commented out. I guess it would be even better to remove them
   outright. */
void	    glade_show_grid_settings		(void);
void	    glade_show_snap_settings		(void);
#endif

gchar*	    glade_get_error_message		(GladeStatusCode  status);

GtkAccelGroup* glade_get_global_accel_group	(void);


/*************************************************************************
 * GladeError - an ADT to represent an error which occurred in Glade.
 *              Currently it is only used for writing source, but it may be
 *		extended for all errors, since GladeStatus doesn't really
 *		provide enough detail to show the user useful messages.
 *************************************************************************/

typedef struct _GladeError GladeError;

struct _GladeError
{
  GladeStatusCode status;
  gint system_errno;
  gchar *message;
};

/* Creates a new GladeError with no error set. */
GladeError* glade_error_new			(void);

/* Creates a GladeError with the given Glade status code and the printf-like
   message and arguments. */
GladeError* glade_error_new_general	(GladeStatusCode  status,
					 gchar		 *message,
					 ...);

/* Creates a GladeError using the system errno and the printf-like
   message and arguments. This must be called immediately after the error
   is detected, so that errno is still valid and can be copied into the
   GladeError. */
GladeError* glade_error_new_system	(gchar		 *message,
					 ...);

/* Frees the GladeError and its contents. */
void	    glade_error_free			(GladeError      *error);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_H */
