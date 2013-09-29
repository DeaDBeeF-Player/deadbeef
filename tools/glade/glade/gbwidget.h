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
#ifndef GLADE_GBWIDGET_H
#define GLADE_GBWIDGET_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtktoolbar.h>

#include "glade.h"
#include "styles.h"
#include "glade_widget_data.h"
#include "glade-parser.h"
#include "gbsource.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GB_WIDGET_DATA_KEY	"GB_WIDGET_DATA"
#define GB_IS_GB_WIDGET(w) \
	(gtk_object_get_data(GTK_OBJECT(w), GB_WIDGET_DATA_KEY) != NULL)

/* This key has a non-null value if a widget is a placeholder. */
#define GB_PLACEHOLDER_KEY	"GB_PLACEHOLDER"
/* Macro to see if a widget is a placeholder. */
#define GB_IS_PLACEHOLDER(w) \
	(gtk_object_get_data(GTK_OBJECT(w), GB_PLACEHOLDER_KEY) != NULL)

/* This key has a non-null value if a widget is a custom widget. */
#define GLADE_CUSTOM_KEY	"GLADE_CUSTOM"
/* Macro to see if a widget is a custom widget. */
#define GLADE_IS_CUSTOM_WIDGET(w) \
	(gtk_object_get_data(GTK_OBJECT(w), GLADE_CUSTOM_KEY) != NULL)

/* This is used to save a pointer to the GladeWidgetInfo inside a widget while
   loading, so we can resolve ATK relations afterwards. */
extern const gchar *GladeWidgetInfoKey;

extern const gchar *GladeMenuItemStockIDKey;
extern const gchar *GladeMenuItemIconKey;
extern const gchar *GladeMenuItemStockIndexKey;

/* This is the key we use to store the stock icon name or full pathname.
   It is used for GtkImage (hence in the menu editor), and also for the
   icons in buttons. */
extern const gchar *GladeIconKey;


/*************************************************************************
 * Data structures passed to the GbWidget functions
 *************************************************************************/

typedef struct _GbWidgetNewData			GbWidgetNewData;
typedef struct _GbWidgetCreateFromData		GbWidgetCreateFromData;
typedef struct _GbWidgetCreateArgData		GbWidgetCreateArgData;
typedef struct _GbWidgetCreateChildArgData	GbWidgetCreateChildArgData;
typedef struct _GbWidgetGetArgData		GbWidgetGetArgData;
typedef struct _GbWidgetSetArgData		GbWidgetSetArgData;
typedef struct _GbWidgetCreateMenuData		GbWidgetCreateMenuData;
typedef struct _GbWidgetDestroyData		GbWidgetDestroyData;

typedef void (*GbWidgetNewCallback)	(GtkWidget *widget,
					 GbWidgetNewData *data);

/* The gb_widget_set_properties() function is used both when loading in
   properties from an XML file and when applying properties within Glade.
   Similarly, gb_widget_get_properties() is used when showing properties and
   when saving, and gb_widget_new() is used when loading and when creating
   a new widget within Glade. The GbWidgetAction field can be used by the
   function to determine which of the actions is actually happening, so
   special-case code can be used if appropriate. For example, when saving
   we don't want to save properties if they are the default value. */
typedef enum
{
  GB_CREATING,
  GB_LOADING,
  GB_APPLYING,
  GB_SHOWING,
  GB_SAVING
} GbWidgetAction;


/* GbWidgetNewData - used when new widgets are created.
   name is the name of the new widget.
   callback is the function to be called when the widget has been created.
     It is used after a dialog has been shown to let the user configure a
     widget (e.g. setting the number of rows in a new vbox).
   parent is the parent widget that the new widget will be added to.
   current_child is the widget which will be replaced by the new one or NULL.
   x, y, width & height specify the position and size of the new widget,
     when adding to a fixed container.
   */
struct _GbWidgetNewData
{
  GladeProject  *project;
  GbWidgetAction action;		/* GB_CREATING or GB_LOADING. */
  gchar		*name;
  GbWidgetNewCallback callback;
  GtkWidget	*parent;
  GtkWidget	*current_child;
  gint		 x, y;
  GladeWidgetData *widget_data;

  /* This is only used when loading. */
  GbWidgetSetArgData *loading_data;
};

struct _GbWidgetCreateFromData
{
  GladeProject  *project;

};

struct _GbWidgetCreateArgData
{
  GladeProject  *project;

};

struct _GbWidgetCreateChildArgData
{
  GladeProject  *project;

};

struct _GbWidgetGetArgData
{
  GladeProject  *project;
  GbWidgetAction action;		/* GB_SHOWING or GB_SAVING. */

  GtkWidget	*widget;		/* The widget being processed. Note
					   that this is also passed in as an
					   argument, but having it in the
					   struct as well helps with
					   translatable properties. */
  GladeWidgetData *widget_data;		/* The GladeWidgetData of the widget
					   currently being processed. */
  gchar		*agent;			/* This is set to the agent parameter
					   when outputting properties for C++
					   etc., e.g. "glademm". */

  /* These are only used when saving (to a file or the clipboard). */
  gboolean	 copying_to_clipboard;	/* TRUE if we are copying the widgets
					   to the clipboard. */
  GString	*buffer;		/* A buffer in which the XML is
					   initially saved, before outputting
					   to the file (or copying to the
					   clipboard). */
  gboolean	 save_translatable_strings; /* TRUE if we are saving a file of
					       translatable strings. */
  GString	*translatable_strings;	/* A buffer in which declarations of
					   translatable strings are saved, to
					   be output in a file which is added
					   to an application's POTFILES.in
					   if it is using libglade. */
  gint		 indent;		/* The current indentation level, only
					   used to make the XML look nicer. */
  GladeError	*error;			/* If an error occurs when saving the
					   XML this will be non-NULL. */
};


/* Each widget has two types of properties, standard properties which are all
   the properties shown on the 'Widget' and 'Basic' pages in the property
   editor, and child properties, shown on the 'Place' page, which relate to
   how the widget is placed within its parent. In the code to load the widgets,
   i.e. all the calls to gb_widget_input_bool() etc., the standard properties
   are used by default. If you need to get at the child properties, set the
   loading_type field of GbWidgetSetArgData to GB_CHILD_PROPERTIES, get the
   properties you need, then reset it to GB_STANDARD_PROPERTIES. */
typedef enum
{
  GB_STANDARD_PROPERTIES,
  GB_CHILD_PROPERTIES
} GbPropertyType;


struct _GbWidgetSetArgData {
  GladeProject      *project;
  GbWidgetAction     action;		/* GB_APPLYING or GB_LOADING. */
  GtkWidget	    *widget;		/* The widget being processed. Note
					   that this is also passed in as an
					   argument, but having it in the
					   struct as well helps with
					   translatable properties. */
  GladeWidgetData   *widget_data;
  GtkWidget	    *property_to_apply;
  gboolean	     apply;

  /* These are only used when loading. If we are loading from a file, filename
     is used. If we are pasting from the clipboard, xml_buffer,
     replacing_widget and discard_names are used. */
  gchar		    *filename;

  gchar		    *xml_buffer;
  GtkWidget         *replacing_widget;
  gboolean	     discard_names;

  GladeStatusCode    status;

  GladeInterface    *interface;

  /* This is the GladeChildInfo & GladeWidgetInfo of the widget currently
     being created. */
  GladeChildInfo    *child_info;
  GladeWidgetInfo   *widget_info;

  /* As we load widgets, we add them all to this hash, so that the ATK code
     can find the targets of relations in a second pass. The key is the
     widget name, and the value is the widget. */
  GHashTable	    *all_widgets;

  /* This tells the load_XXX() functions to use the normal or packing
     properties. */
  GbPropertyType     loading_type;

  /* This is set to the agent we want to match when loading properties,
     e.g. "glademm" for the C++ properties. */
  gchar		    *agent;
};


struct _GbWidgetCreateMenuData
{
  GladeProject  *project;
  GtkWidget	*menu;
  GtkWidget	*child;
};


struct _GbWidgetDestroyData
{
  GladeProject  *project;

};


/*************************************************************************
 * The GbWidget struct which contains info on each widget class.
 *************************************************************************/

/* These are used in the properties_page_number of the GbWidget struct to
   specify whether the GbWidget's properties page has been created or if it
   doesn't need one (i.e. it has no properties of its own). */
#define GB_PROPERTIES_NOT_CREATED	-1
#define GB_PROPERTIES_NOT_NEEDED	-2

/* The GbWidget struct, which consists of pointers to the functions
   implemented by each GbWidget. The properties_page_number is the page in
   the widget properties notebook corresponding to the GbWidget.
   The typedef to GbWidget is in glade.h */
struct _GbWidget
{
  gchar		**pixmap_struct;
  gchar          *class_id;
  GdkPixmap	 *gdkpixmap;
  GdkBitmap	 *mask;
  const gchar	 *tooltip;
  gint		  properties_page_number;	/* Used internally */
  gint		  child_properties_page_number;	/* Used internally */

  /* We need a pixbuf rather than GdkPixmap for the tree. This is only
     created when needed for now. */
  GdkPixbuf	 *pixbuf;

  /* Creates a new widget of the particular class, or possibly shows a dialog
     for the user to set options before creating the widget (but it must not
     do this when loading an XML file, i.e. when the action is GB_LOADING). */
  GtkWidget* (* gb_widget_new)		(GbWidgetNewData             *data);

  /* This is used when we need to turn a normal widget into a GbWidget,
     usually because it was created automatically as part of a dialog.
     It should add any special signal handlers or data that is needed for
     the widget to be used within Glade. For example, it may need to connect
     to certain signals so that it can update the property page or redraw
     the widget. */
  void (* gb_widget_create_from_widget)	(GtkWidget		     *widget,
					 GbWidgetCreateFromData      *data);

  void (* gb_widget_create_properties)	(GtkWidget		     *widget,
					 GbWidgetCreateArgData       *data);
  void (* gb_widget_get_properties)	(GtkWidget                   *widget,
					 GbWidgetGetArgData	     *data);
  void (* gb_widget_set_properties)	(GtkWidget                   *widget,
					 GbWidgetSetArgData	     *data);

  /* Adds a widget as a child, used when loading the XML. Normal containers
     are handled within Glade, but if you need to call special functions to
     add the child, do it here. A similar replace_child function may be added
     in future. */
  void (* gb_widget_add_child)          (GtkWidget                   *widget,
                                         GtkWidget                   *child,
                                         GbWidgetSetArgData          *data);
  /* Returns a special child of the widget, given it's child name,
     e.g. "Dialog:action_area". */
  GtkWidget* (* gb_widget_get_child)	(GtkWidget                   *widget,
					 const gchar		     *child_name);
  /* Creates the properties that apply to the children of the widget's class.*/
  void (* gb_widget_create_child_properties) (GtkWidget		     *widget,
					      GbWidgetCreateChildArgData *data);

  /* Shows or saves the child properties of a child of the widget. */
  void (* gb_widget_get_child_properties) (GtkWidget		     *widget,
					   GtkWidget		     *child,
					   GbWidgetGetArgData        *data);

  /* Applies or loads the child properties of a child of the widget. */
  void (* gb_widget_set_child_properties) (GtkWidget		     *widget,
					   GtkWidget		     *child,
					   GbWidgetSetArgData        *data);

  /* Here the widget can add widget-specific commands to the popup menu. */
  void (* gb_widget_create_popup_menu)	(GtkWidget                   *widget,
					 GbWidgetCreateMenuData      *data);

  /* Outputs the C source code to create the widget and set its properties.
     This function needs to call gb_widget_write_standard_source () to output
     the code to set standard widget properties and add the widget to the
     parent. */
  void (* gb_widget_write_source)	(GtkWidget                   *widget,
					 GbWidgetWriteSourceData     *data);

  /* Outputs the code to add a child to the widget. If the GbWidget doesn't
     supply this function, the default gtk_container_add() function will be
     used. parent_name is the C identifier of the parent, e.g. "vbox1". */
  void (* gb_widget_write_add_child_source) (GtkWidget		     *parent,
					     const gchar	     *parent_name,
					     GtkWidget		     *child,
					     GbWidgetWriteSourceData *data);

  /* Called when the widget is destroyed, to free any allocated memory etc. */
  void (* gb_widget_destroy)		(GtkWidget                   *widget,
					 GbWidgetDestroyData         *data);
};


/*************************************************************************
 * Public Functions
 *************************************************************************/

/* Call this first, to initialize all GtkWidget & GbWidget types */

void	    gb_widgets_init();

/* This registers a GbWidget, so it can be looked up by class name. */
void	    gb_widget_register_gbwidget (const gchar	       *class_id,
					 GbWidget	       *gbwidget);
/* Thes return the GbWidget struct corresponding to the given class name or
   widget. */
GbWidget *  gb_widget_lookup_class	(const gchar	       *class_id);
GbWidget *  gb_widget_lookup		(GtkWidget	       *widget);

/* This returns the class id corresponding to the given widget.
   This should be used instead of gtk_type_name so custom widgets work. */
gchar*	    gb_widget_get_class_id	(GtkWidget	       *widget);

/* This initializes the GbWidget struct, setting all function pointers to NULL.
 */
void	    gb_widget_init_struct	(GbWidget	       *gbwidget);

GtkWidget*  gb_widget_new		(const gchar	       *class_id,
					 GtkWidget	       *parent);
GtkWidget*  gb_widget_new_full		(const gchar	       *class_id,
					 gboolean	        create_default_name,
					 GtkWidget	       *parent,
					 GtkWidget	       *current_child,
					 gint			x,
					 gint			y,
					 GbWidgetNewCallback	callback,
					 GbWidgetAction		action,
					 GbWidgetSetArgData    *loading_data);
void	    gb_widget_create_from	(GtkWidget	       *widget,
					 const gchar	       *name);
void	    gb_widget_create_from_full	(GtkWidget		*widget,
					 const gchar		*name,
					 GladeWidgetData	*wdata);
gboolean    gb_widget_can_finish_new	(GbWidgetNewData       *data);
void        gb_widget_initialize	(GtkWidget	       *widget,
					 GbWidgetNewData       *data);
void	    gb_widget_free_new_data	(GbWidgetNewData       *data);

void	    gb_widget_set_usize		(GtkWidget	       *widget,
					 gint			w,
					 gint			h);
gint	    gb_widget_create_properties	(GtkWidget	       *widget);

gint	    gb_widget_create_child_properties (GtkWidget       *widget);

void	    gb_widget_show_properties	(GtkWidget	       *widget);
void	    gb_widget_show_position_properties
					(GtkWidget	       *widget);
#ifdef GLADE_STYLE_SUPPORT
void	    gb_widget_show_style	(GtkWidget	       *widget);
#endif

void	    gb_widget_apply_properties	(GtkWidget	       *widget,
					 GtkWidget	       *property);

void	    gb_widget_show_popup_menu	(GtkWidget	       *widget,
					 GdkEventButton	       *event);
GtkWidget*  gb_widget_load		(GtkWidget	       *widget,
					 GbWidgetSetArgData    *data,
					 GtkWidget	       *parent);
void	    gb_widget_load_style	(GbWidgetSetArgData    *data);
void	    gb_widget_load_project	(GbWidgetSetArgData    *data);
gboolean    gb_widget_add_child		(GtkWidget	       *parent,
					 GbWidgetSetArgData    *data,
					 GtkWidget	       *child);

void	    gb_widget_save		(GtkWidget	       *widget,
					 GbWidgetGetArgData    *data);
#ifdef GLADE_STYLE_SUPPORT
void	    gb_widget_save_style	(GbStyle	       *gbstyle,
					 GbWidgetGetArgData    *data,
					 gboolean	        save_all);
#endif

gboolean    gb_widget_replace_child	(GtkWidget	       *widget,
					 GtkWidget	       *current_child,
					 GtkWidget	       *new_child);

/*
 * Getting and setting the child name of a widget.
 */

/* Returns the child name of the widget. */
gchar*	    gb_widget_get_child_name	(GtkWidget	       *widget);

/* Sets the child name of the widget. The child_name string is duplicated. */
void	    gb_widget_set_child_name	(GtkWidget	       *widget,
					 const gchar	       *child_name);


/* Inputting Properties - Loading or Applying.
   Note: The only time you have to free the returned value is when calling
   gb_widget_input_text() and data->action == GB_APPLYING (because we get the
   value from the text widget and it has to be freed), and when calling
   gb_widget_input_pixmap_filename() with data->action == GB_LOADING. */
gchar*	    gb_widget_input_string	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gchar*	    gb_widget_input_text	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gint	    gb_widget_input_int		(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gint	    gb_widget_input_optional_int(GbWidgetSetArgData    *data,
					 const gchar	       *property,
					 gboolean	       *is_set);
gfloat	    gb_widget_input_float	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gboolean    gb_widget_input_bool	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gchar*	    gb_widget_input_choice	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gchar*	    gb_widget_input_combo	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
GdkColor*   gb_widget_input_color	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
GdkPixmap*  gb_widget_input_bgpixmap	(GbWidgetSetArgData    *data,
					 const gchar	       *property,
					 gchar		      **filename);
gpointer    gb_widget_input_dialog	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gchar*	    gb_widget_input_filename	(GbWidgetSetArgData    *data,
					 const gchar	       *property);
gchar*	    gb_widget_input_pixmap_filename (GbWidgetSetArgData *data,
					     const gchar	*property);
GdkFont*    gb_widget_input_font	(GbWidgetSetArgData    *data,
					 const gchar	       *property,
					 gchar		      **xlfd_fontname);
gchar*	    gb_widget_input_stock_item	(GbWidgetSetArgData	*data,
					 const gchar		*property);
gchar*	    gb_widget_input_icon	(GbWidgetSetArgData	*data,
					 const gchar		*property);
gchar*	    gb_widget_input_named_icon	(GbWidgetSetArgData	*data,
					 const gchar		*property);

/* load only */
gint        gb_widget_input_enum        (GbWidgetSetArgData     *data,
					 GType                   enum_type,
					 const char            **labels,
					 int                    *values,
					 const gchar            *property);

/* Outputting Properties - Saving or Showing.
   When saving, the translatable string and text properties may also be output
   in a separate file, as a number of translatable strings understood by
   xgettext, i.e. using "N_()". This may be useful when using libglade, as
   you could include this file in your POTFILES.in so the interface can easily
   be translated into other languages. */
void	    gb_widget_output_string	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 const gchar	       *value);
void	    gb_widget_output_translatable_string (GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 const gchar	       *value);
void	    gb_widget_output_text	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 const gchar	       *value);
void	    gb_widget_output_translatable_text (GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 const gchar	       *value);
void	    gb_widget_output_translatable_text_in_lines (GbWidgetGetArgData *data,
					 const gchar	       *property,
					 const gchar	       *value);
void	    gb_widget_output_int	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 gint			value);
void	    gb_widget_output_optional_int (GbWidgetGetArgData  *data,
					 const gchar	       *property,
					 gint			value,
					 gboolean		is_set);
void	    gb_widget_output_float	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 gfloat			value);
void	    gb_widget_output_bool	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 gint			value);
void	    gb_widget_output_choice	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 gint			value,
					 const gchar	       *symbol);
void	    gb_widget_output_combo	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 const gchar	       *value);
void	    gb_widget_output_color	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 GdkColor	       *value);
void	    gb_widget_output_bgpixmap	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 GdkPixmap	       *value,
					 const gchar	       *filename);
void	    gb_widget_output_dialog	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 const gchar	       *string,
					 gconstpointer	        value);
void	    gb_widget_output_filename	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 const gchar	       *value);
void	    gb_widget_output_pixmap_filename (GbWidgetGetArgData *data,
					      const gchar      *property,
					      const gchar      *value);
void	    gb_widget_output_font	(GbWidgetGetArgData    *data,
					 const gchar	       *property,
					 GdkFont	       *value,
					 const gchar	       *xlfd_fontname);
void	    gb_widget_output_stock_item (GbWidgetGetArgData	*data,
					 const gchar		*property,
					 const gchar		*value);
void	    gb_widget_output_icon	(GbWidgetGetArgData	*data,
					 const gchar		*property,
					 const gchar		*value);
void	    gb_widget_output_named_icon	(GbWidgetGetArgData	*data,
					 const gchar		*property,
					 const gchar		*value);

void        gb_widget_output_enum       (GbWidgetGetArgData     *data,
					 GType                   enum_type,
					 int                    *values,
					 int                     n_values,
					 const gchar            *property,
					 gint                    value);



/* Tooltips functions. */
gboolean    gb_widget_get_show_tooltips	(void);
void	    gb_widget_set_show_tooltips (gboolean		show);
void	    gb_widget_reset_tooltips	(void);

/* Convenience functions for handling all 6 properties of adjustments. */
void	    gb_widget_output_adjustment	(GbWidgetGetArgData *data,
					 const gchar	    *Values[],
					 GtkAdjustment	    *adjustment,
					 gchar		    *saved_property_name);
gboolean    gb_widget_input_adjustment	(GbWidgetSetArgData *data, 
					 const gchar	    *Values[],
					 GtkAdjustment	    *adjustment,
					 gchar		    *saved_property_name);


/* Common functions for gbwidgets that have a child label - button/menuitem
   etc. */
void	    gb_widget_remove_label	(GtkWidget	       *menuitem,
					 GtkWidget	       *widget);

void	    gb_widget_output_child_label(GtkWidget	       *widget,
					 GbWidgetGetArgData    *data,
					 const gchar	       *Label);
void	    gb_widget_input_child_label	(GtkWidget	       *widget,
					 GbWidgetSetArgData    *data,
					 const gchar	       *Label);
void	    gb_widget_create_child_label_popup_menu
					(GtkWidget	       *widget,
					 GbWidgetCreateMenuData *data);

/* This calls the given function for all children of the widget that Glade is
   interested in. It is used for saving the entire tree etc. */
void	    gb_widget_children_foreach	(GtkWidget	       *widget,
					 GtkCallback		callback,
					 gpointer		data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_GBWIDGET_H */
