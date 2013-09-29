#include <gtk/gtk.h>
#include "gladeconfig.h"

#ifdef USE_GNOME
#include <gnome.h>
#include <bonobo.h>
#endif

#include "gb.h"
#include "gbwidget.h"
#include "source.h"
#include "glade_gnome.h"
#include "glade_atk.h"

/* These are used for outputting signal handler prototypes. */
#define GB_PARAM_INDENT		40
#define GB_PARAM_TYPE_WIDTH	16

static GHashTable *glade_signal_hash = NULL;

static void gb_widget_write_signals_source (GtkWidget * widget,
					    GbWidgetWriteSourceData * data);
static void gb_widget_write_signal_connection_source (GbWidgetWriteSourceData * data,
						      const gchar *signal_name,
						      const gchar *connect_object,
						      gboolean connect_after,
						      const gchar *handler_data,
						      const gchar *handler);
static gchar *get_type_name (GtkType type, gboolean * is_pointer);
static gchar *get_gdk_event (gchar * signal_name);
static gchar **lookup_signal_arg_names (gchar * type, gchar * signal_name,
					gint num_args_expected);
static void gb_widget_write_accelerators_source (GtkWidget * widget,
					    GbWidgetWriteSourceData * data);

/*************************************************************************
 * Functions for writing C source code
 *************************************************************************/

void
gb_widget_write_source (GtkWidget * widget,
			GbWidgetWriteSourceData * data)
{
  GtkWidget *parent;
  GbWidget *gbwidget;
  GladeWidgetData *widget_data;
  gchar *class_id, *child_name, *widget_name = NULL, *real_widget_name = NULL;
  gint source_len;

  /* This is a temporary(?) kludge so that the code for GtkDialogs is OK.
     We stop the source code for the action_area from being written.
     GtkDialog & GnomeDialog need to output the code for their vbox &
     action_area children themselves, since they need to output special code
     to access them, e.g. "GTK_DIALOG (dialog1)->vbox". However, the vbox
     is the parent of the action_area, and so we have to stop the action_area
     being output using the standard code since that won't work.
     The problem is due to the dialogs having 2 special children, where one
     is a descendant of the other. I don't think this occurs anywhere else. */
  child_name = gb_widget_get_child_name (widget);
  if (child_name && data->create_widget)
    {
      if (!strcmp (child_name, GladeChildDialogActionArea))
	return;
    }

  parent = data->parent;

  class_id = gb_widget_get_class_id (widget);
#if 0
  g_print ("class_id: %s\n", class_id ? class_id : "");
#endif
  data->write_children = TRUE;

  widget_data = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  /* If this isn't a gbwidget, skip it. */
  if (widget_data)
    {
      gbwidget = gb_widget_lookup_class (class_id);
      g_return_if_fail (gbwidget != NULL);

      /* For custom widgets, we don't have a default widget to compare with,
	 so all properties should be set. */
      if (GLADE_IS_CUSTOM_WIDGET (widget))
	{
	  data->standard_widget = NULL;
	}
      else
	{
	  /* This stores newly-created widgets, which we use to get default
	     values from. Though we don't use them much at present. */
	  data->standard_widget = (GtkWidget *) g_hash_table_lookup (data->standard_widgets, class_id);
	  if (data->standard_widget == NULL)
	    {
#ifdef USE_GNOME
	      /* FIXME: GnomeLibs 1.0.1 workaround - gtk_object_newv doesn't
		 return a valid GnomeAppBar or GnomeDateEdit, so we create it
		 ourself. */
	      if (!strcmp (class_id, "GnomeAppBar"))
		data->standard_widget = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_NEVER);
	      else if (!strcmp (class_id, "GnomeDateEdit"))
		data->standard_widget = gnome_date_edit_new ((time_t) 0, TRUE,
							     TRUE);
	      /* We don't create standard widgets for Bonobo controls, for
		 now. There may be a better way to get default values for
		 them. */
	      else if (!data->standard_widget && !BONOBO_IS_WIDGET (widget))
		data->standard_widget = GTK_WIDGET (g_object_newv (gtk_type_from_name (class_id), 0, NULL));
#else
	      if (!data->standard_widget)
		data->standard_widget = GTK_WIDGET (g_object_newv (gtk_type_from_name (class_id), 0, NULL));
#endif

	      if (data->standard_widget)
		g_hash_table_insert (data->standard_widgets, class_id,
				     data->standard_widget);
	    }
	}

      real_widget_name = source_create_valid_identifier (gtk_widget_get_name (widget));
      if (data->use_component_struct && widget_data->public_field)
	widget_name = g_strdup_printf ("%s->%s", data->component_name,
				       real_widget_name);
      else
	widget_name = real_widget_name;

      data->widget_data = widget_data;
      data->real_wname = real_widget_name;
      data->wname = widget_name;
      if (gbwidget->gb_widget_write_source)
	(gbwidget->gb_widget_write_source) (widget, data);
      else
	source_add (data, "  /* Skipping %s: unimplemented. */\n", class_id);

      /* Make sure there is a blank line after each widget, for readability. */
      source_len = data->source_buffers[GLADE_SOURCE]->len;
      if (source_len > 2
	  && (data->source_buffers[GLADE_SOURCE]->str[source_len - 1] != '\n'
	      || data->source_buffers[GLADE_SOURCE]->str[source_len - 2] != '\n'))
	source_add (data, "\n");

      data->wname = NULL;
      data->real_wname = NULL;
      data->parent = widget;
    }
  else if (GB_IS_PLACEHOLDER (widget) && parent)
    {
      if (GTK_IS_NOTEBOOK (parent))
	{
	  /* SPECIAL CODE: If notebook pages are empty (i.e. placeholders),
	     we create dummy widgets instead, so it still runs OK. */
	  if (child_name == NULL)
	    {
	      gchar *wname, *parent_name;
	  
	      wname = "empty_notebook_page";
	      /* Make sure the dummy widget is declared. */
	      source_ensure_decl (data, "  GtkWidget *empty_notebook_page;\n");

	      parent_name = (char*) gtk_widget_get_name (parent);
	      parent_name = source_create_valid_identifier (parent_name);
	      source_add (data,
			  "  %s = gtk_vbox_new (FALSE, 0);\n"
			  "  gtk_widget_show (%s);\n"
			  "  gtk_container_add (GTK_CONTAINER (%s), %s);\n"
			  "\n",
			  wname, wname, parent_name, wname);
	      g_free (parent_name);
	    }
	  else
	    {
	      /* For empty notebook tabs, we increment the 'last_child' written
		 value. */
	      gint col = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (parent),
							       "last_child"));
	      gtk_object_set_data (GTK_OBJECT (parent), "last_child",
				   GINT_TO_POINTER (col + 1));
	    }
	}
      else if (GTK_IS_CLIST (parent))
	{
	  /* For empty clist/ctree titles, we increment the 'last_child'
	     written value. */
	  if (child_name && !strcmp (child_name, GladeChildCListTitle))
	    {
	      gint col = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (parent),
							       "last_child"));
	      gtk_object_set_data (GTK_OBJECT (parent), "last_child",
				   GINT_TO_POINTER (col + 1));
	    }
	}
    }

  /* Recursively write source for children.
     We need to reset the parent after the children have been written. */
  data->create_widget = TRUE;
  if (data->write_children)
    gb_widget_children_foreach (widget, (GtkCallback) gb_widget_write_source, data);

  /* We need to reset some of the members of the GbWidgetWriteSourceData struct
     so that they work OK for the next sibling. */
  data->parent = parent;

  /* SPECIAL CODE: For GtkOptionMenu, we have to set the menu after all the
     children are created. */
  if (GTK_IS_OPTION_MENU (widget) && GB_IS_GB_WIDGET (widget)
      && GTK_OPTION_MENU (widget)->menu
      && GB_IS_GB_WIDGET (GTK_OPTION_MENU (widget)->menu))
    {
      gchar *menu_name = source_create_valid_identifier (gtk_widget_get_name (GTK_OPTION_MENU (widget)->menu));
      source_add (data,
		  "  gtk_option_menu_set_menu (GTK_OPTION_MENU (%s), %s);\n\n",
		  widget_name, menu_name);
      g_free (menu_name);
    }

  /* SPECIAL CODE: Finish off the GnomeUIInfo struct if we are building a
     Gnome menu. */
#ifdef USE_GNOME
  if (data->project->gnome_support && GTK_IS_MENU_SHELL (widget)
      && GB_IS_GB_WIDGET (widget))
    {
      glade_gnome_finish_menu_source (GTK_MENU_SHELL (widget), data);
    }
#endif

  if (widget_name != real_widget_name)
    g_free (real_widget_name);
  g_free (widget_name);
}


/* This is called by each GbWidget's write_source function to write all the
   common source code, including code to add the widget to its parent. */
void
gb_widget_write_standard_source (GtkWidget * widget,
				 GbWidgetWriteSourceData * data)
{
  GladeWidgetData *wdata = data->widget_data;
  gint i, width, height, can_focus, can_default;

  /* FIXME: unfinished. public fields were to be added to the component struct.
     Private fields would just be declared within the creation function. */
  if (wdata->public_field)
    source_add_decl (data, "  GtkWidget *%s;\n", data->real_wname);
  else
    source_add_decl (data, "  GtkWidget *%s;\n", data->real_wname);

  if (data->set_widget_names)
    source_add (data, "  gtk_widget_set_name (%s, \"%s\");\n", data->wname,
		data->real_wname);

  if (!data->use_component_struct)
    {
      /* For toplevel widgets we don't ref the widget, since if we do it never
	 gets destroyed. This may be a bug in GTK+ 1.2.3. */
      if (data->parent == NULL)
	{
	  source_add_to_buffer (data, GLADE_OBJECT_HOOKUP,
				"  GLADE_HOOKUP_OBJECT_NO_REF (%s, %s, %s);\n",
				data->component_name,
				data->wname,
				source_make_string (data->real_wname, FALSE));
	}
      else
	{
	  source_add_to_buffer (data, GLADE_OBJECT_HOOKUP,
				"  GLADE_HOOKUP_OBJECT (%s, %s, %s);\n",
				data->component_name,
				data->wname,
				source_make_string (data->real_wname, FALSE));
	}
    }

  /* SPECIAL CODE: menus are not shown here. */
  if (widget->parent && wdata->flags & GLADE_VISIBLE && !GTK_IS_MENU (widget))
    {
#ifdef USE_GNOME
      /* FIXME: GnomeDruidPageStandard bug workaround. It needs show_all(). */
      if (GNOME_IS_DRUID_PAGE_STANDARD (widget))
	source_add (data, "  gtk_widget_show_all (%s);\n", data->wname);
      else
	source_add (data, "  gtk_widget_show (%s);\n", data->wname);
#else
      source_add (data, "  gtk_widget_show (%s);\n", data->wname);
#endif
    }

  /* Output code to add widget to parent. */
  gb_widget_write_add_child_source (widget, data);

  if (wdata->flags & (GLADE_WIDTH_SET | GLADE_HEIGHT_SET))
    {
      width = wdata->flags & GLADE_WIDTH_SET ? wdata->width : -1;
      height = wdata->flags & GLADE_HEIGHT_SET ? wdata->height : -1;

      /* GTK BUG WORKAROUND - a combo should manage the size of its entry.
	 I think this isn't needed any more (GTK+ 1.2.3). */
#if 0
      if (GTK_IS_COMBO (widget))
	source_add(data,
		   "  gtk_widget_set_size_request (GTK_COMBO (%s)->entry, %i, %i);\n",
		   data->wname, width - 16 < 0 ? -1 : width - 16, height);
#endif

      source_add (data, "  gtk_widget_set_size_request (%s, %i, %i);\n",
		  data->wname, width, height);
    }


  if (glade_util_uses_border_width (widget))
    {
      if (GTK_CONTAINER (widget)->border_width != 0)
	{
	  source_add (data,
		      "  gtk_container_set_border_width (GTK_CONTAINER (%s), %i);\n",
		      data->wname, GTK_CONTAINER (widget)->border_width);
	}
    }


  /* FIXME: Kludge to set separator menu items insensitive, so that they are
     skipped when using the cursor keys to move up/down the menu. */
  if (!(wdata->flags & GLADE_SENSITIVE)
      || (GTK_IS_MENU_ITEM (widget) && GTK_BIN (widget)->child == NULL))
    source_add (data, "  gtk_widget_set_sensitive (%s, FALSE);\n", data->wname);

  can_focus = GTK_WIDGET_CAN_FOCUS (widget);
  if (!data->standard_widget
      || can_focus != GTK_WIDGET_CAN_FOCUS (data->standard_widget))
    {
      if (can_focus)
	source_add (data, "  gtk_widget_set_can_focus(%s, TRUE);\n",
		    data->wname);
      else
	source_add (data, "  gtk_widget_set_can_focus(%s, FALSE);\n",
		    data->wname);
    }
  can_default = GTK_WIDGET_CAN_DEFAULT (widget);
  if (!data->standard_widget
      || can_default != GTK_WIDGET_CAN_DEFAULT (data->standard_widget))
    {
      if (can_default)
	source_add (data, "  gtk_widget_set_can_default(%s, TRUE);\n",
		    data->wname);
      else
	source_add (data, "  gtk_widget_set_can_default(%s, FALSE);\n",
		    data->wname);
    }

  if (wdata->flags & GLADE_GRAB_FOCUS)
    {
      if (data->focus_widget)
	g_warning ("Multiple widgets with 'Has Focus' set: %s, %s",
		   data->focus_widget, data->real_wname);
      else
	data->focus_widget = g_strdup (data->wname);
    }

  if (wdata->flags & GLADE_GRAB_DEFAULT)
    {
      if (data->default_widget)
	g_warning ("Multiple widgets with 'Has Default' set: %s, %s",
		   data->default_widget, data->real_wname);
      else
	data->default_widget = g_strdup (data->wname);
    }

  if (wdata->tooltip)
    {
      gboolean translatable, context;
      gchar *comments;

      glade_util_get_translation_properties (widget, GbTooltip, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      data->need_tooltips = TRUE;

      /* GtkToolItem subclasses use a special function, since otherwise they
	 wouldn't work as they don't have a window. */
      if (GTK_IS_TOOL_ITEM (widget))
	{
	  source_add (data,
		      "  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (%s), tooltips, %s, NULL);\n",
		      data->wname,
		      source_make_string_full (wdata->tooltip, data->use_gettext && translatable, context));
	}
      else
	{
	  source_add (data,
		      "  gtk_widget_set_tooltip_text (%s, %s);\n",
		      data->wname,
		      source_make_string_full (wdata->tooltip, data->use_gettext && translatable, context));
	}
    }

  if (!GTK_WIDGET_NO_WINDOW (widget))
    {
      GdkExtensionMode ext_mode = gtk_widget_get_extension_events (widget);
      gboolean written_first = FALSE;

      if (wdata->events)
	{
	  source_add (data, "  gtk_widget_set_events (%s, ", data->wname);
	  for (i = 0; i < GB_EVENT_MASKS_COUNT; i++)
	    {
	      if (wdata->events & GbEventMaskValues[i])
		{
		  if (!written_first)
		    source_add (data, "%s", GbEventMaskSymbols[i]);
		  else
		    source_add (data, " | %s", GbEventMaskSymbols[i]);
		  written_first = TRUE;
		}
	    }
	  source_add (data, ");\n");
	}

      if (ext_mode != GDK_EXTENSION_EVENTS_NONE)
	{
	  for (i = 0; GbExtensionModeChoices[i]; i++)
	    {
	      if (GbExtensionModeValues[i] == ext_mode)
		source_add (data, "  gtk_widget_set_extension_events (%s, %s);\n",
			    data->wname, GbExtensionModeSymbols[i]);
	    }
	}
    }

  gb_widget_write_signals_source (widget, data);
  gb_widget_write_accelerators_source (widget, data);
  glade_atk_write_source (widget, data);
}


void
gb_widget_write_add_child_source (GtkWidget * widget,
				  GbWidgetWriteSourceData * data)
{
  GbWidget *gbparent;
  GtkWidget *parent;
  gchar *parent_name;

  /* If the widget is created automatically by its parent, we don't need
     to add source to add it. */
  if (!data->create_widget)
    return;

  /* SPECIAL CODE: to handle menus. */
  if (GTK_IS_MENU (widget))
    parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
  else
    parent = data->parent;

  /* Return if no parent exists */
  if (!parent)
    return;

  parent_name = (char*) gtk_widget_get_name (parent);
  parent_name = source_create_valid_identifier (parent_name);

  MSG2 ("Adding %s to %s", data->wname, parent_name);

  /* If the GbWidget has its own function to output source code to add a child,
     we use that, else we just output the default "gtk_container_add ()". */
  gbparent = gb_widget_lookup (parent);
  if (gbparent && gbparent->gb_widget_write_add_child_source)
    {
      (gbparent->gb_widget_write_add_child_source) (parent, parent_name,
						    widget, data);
    }
  else
    {
      source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
		  parent_name, data->wname);
    }

  g_free (parent_name);
}


static void
gb_widget_write_signals_source (GtkWidget * widget,
				GbWidgetWriteSourceData * data)
{
  GList *item;
  GladeSignal *signal;
  gboolean skip_handler_code;
  gchar *handler;

  item = data->widget_data->signals;
  while (item)
    {
      signal = (GladeSignal *) item->data;
      item = item->next;
      skip_handler_code = FALSE;

      /* If we're appending new/changed signals and this is an old one, then
	 skip it. Note that we also skip it if the last mod time is 0, which
	 will happen for XML files created before the last mod tag was added.*/
      MSG2 ("Creating Signals:%s Handler: %s",
	    data->creating_callback_files ? "TRUE" : "FALSE", signal->handler);
      MSG1 ("LastMod:%s", ctime (&signal->last_modification_time));
      MSG1 ("LastWrite:%s", ctime (&data->last_write_time));

      if (!data->creating_callback_files
	  && (signal->last_modification_time <= data->last_write_time))
	{
	  MSG1 ("Skipping signal: %s", signal->handler);
	  skip_handler_code = TRUE;
	}

      if (!signal->name)
	{
	  /* FIXME: store warnings */
	  g_warning ("Signal name not set");
	  continue;
	}

      if (!signal->handler)
	{
	  /* FIXME: store warnings */
	  g_warning ("Signal handler not set");
	  continue;
	}

      /* Make sure handler function name is valid. */
      handler = source_create_valid_identifier (signal->handler);

      /* Output code to connect signal. */
      gb_widget_write_signal_connection_source (data, signal->name,
						signal->object, signal->after,
						signal->data, handler);

      /* We don't need to output code for standard GTK functions. */
      if (!strcmp (handler, "gtk_widget_show")
	  || !strcmp (handler, "gtk_widget_hide")
	  || !strcmp (handler, "gtk_widget_grab_focus")
	  || !strcmp (handler, "gtk_widget_destroy")
	  || !strcmp (handler, "gtk_window_activate_default")
	  || !strcmp (handler, "gtk_true")
	  || !strcmp (handler, "gtk_false")
	  || !strcmp (handler, "gtk_main_quit"))
	skip_handler_code = TRUE;


      /* If we're appending new/changed signals and this is an old one, we
	 don't want to output the source code for it. */
      if (!skip_handler_code)
	{
	  gb_widget_write_signal_handler_source (widget, data, signal->name,
						 handler);
	}
      g_free (handler);
    }
}


/* This outputs a signal handler declaration and empty function. handler
   should be a valid identifier. */
void
gb_widget_write_signal_handler_source (GtkWidget *widget,
				       GbWidgetWriteSourceData * data,
				       const gchar *signal_name,
				       const gchar *handler)
{
  guint signal_id;
  GSignalQuery query_info;
  gint param, i;
  gchar buffer[1024];
  gchar *ret_type, *pos, *type_name, *arg_name, *object_name, *object_arg;
  gchar *object_arg_start;
  gboolean is_pointer;
  gint param_num, widget_num, event_num, callback_num;
  gint handler_len, object_name_len, type_name_len, num_spaces;
  gint *arg_num;
  gchar **arg_names;
  gchar *signal_name_copy;

  /* Check if we have already output the handler. */
  if (g_hash_table_lookup (data->handlers_output, handler))
    return;

  /* Remember that we have output the handler. */
  g_hash_table_insert (data->handlers_output, g_strdup (handler), "Output");


  signal_id = gtk_signal_lookup (signal_name, GTK_OBJECT_TYPE (widget));
  /* If a project is converted from GTK+ 1.2.x to 2.x.x it may have some
     signals which aren't valid any more. So we output a warning. */
  if (signal_id == 0)
    {
      g_warning ("Invalid signal: '%s' for widget: '%s'", signal_name,
		 data->wname);
      return;
    }

  g_signal_query (signal_id, &query_info);

  /* Output the return type and function name. */
  ret_type = get_type_name (query_info.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE, &is_pointer);
  pos = buffer;
  sprintf (pos, "%s%s\n%s", ret_type, is_pointer ? "*" : "", handler);
  pos += strlen (pos);

  handler_len = strlen (handler);
  if (handler_len >= GB_PARAM_INDENT - 1)
    {
      *pos++ = '\n';
      for (i = 0; i < GB_PARAM_INDENT; i++)
	*pos++ = ' ';
    }
  else
    {
      num_spaces = GB_PARAM_INDENT - handler_len - 1;
      for (i = 0; i < num_spaces; i++)
	*pos++ = ' ';
    }

  /* Convert signal name to use underscores rather than dashes '-'. */
  signal_name_copy = g_strdup (query_info.signal_name);
  for (i = 0; signal_name_copy[i]; i++)
    {
      if (signal_name_copy[i] == '-')
	signal_name_copy[i] = '_';
    }

  /* Output the signal parameters. */
  object_name = (char*) g_type_name (query_info.itype);
  arg_names = lookup_signal_arg_names (object_name, signal_name_copy,
				       query_info.n_params + 1);


  widget_num = 0;

  /* Output the signal object type and the argument name. We assume the
     type is a pointer - I think that is OK. We remove "Gtk" and convert
     to lower case for the argument name. */
  sprintf (pos, "(%s ", object_name);
  pos += strlen (pos);
  object_name_len = strlen (object_name);
  if (object_name_len + 1 < GB_PARAM_TYPE_WIDTH)
    {
      num_spaces = GB_PARAM_TYPE_WIDTH - object_name_len - 1;
      for (i = 0; i < num_spaces; i++)
	*pos++ = ' ';
    }

  if (arg_names)
    {
      sprintf (pos, "*%s,\n", arg_names[0]);
      pos += strlen (pos);
    }
  else
    {
      object_arg = (!strncmp (object_name, "Gtk", 3)) ? object_name + 3 : object_name;
      object_arg_start = pos + 1;
      sprintf (pos, "*%s", object_arg);
      pos += strlen (pos);
      g_strdown (object_arg_start);
      if (!strncmp (object_arg_start, "widget", 6))
	widget_num = 2;
      sprintf (pos, ",\n");
      pos += strlen (pos);
    }


  param_num = 1;
  event_num = callback_num = 0;
  for (param = 0; param < query_info.n_params; param++)
    {
      for (i = 0; i < GB_PARAM_INDENT; i++)
	*pos++ = ' ';

      /* If the arg name includes the type (with a space between!), just
	 output that. */
      if (arg_names && strchr (arg_names[param + 1], ' '))
	{
	  sprintf (pos, "%s,\n", arg_names[param + 1]);
	  pos += strlen (pos);
	}
      else
	{
	  type_name = get_type_name (query_info.param_types[param] & ~G_SIGNAL_TYPE_STATIC_SCOPE, &is_pointer);
	  /* Most arguments to the callback are called "arg1", "arg2", etc.
	     GtkWidgets are called "widget", "widget2", ...
	     GdkEvents are called "event", "event2", ...
	     GtkCallbacks are called "callback", "callback2", ... */
	  if (!strcmp (type_name, "GtkWidget"))
	    {
	      arg_name = "widget";
	      arg_num = &widget_num;
	    }
	  else if (!strcmp (type_name, "GdkEvent"))
	    {
	      type_name = get_gdk_event (signal_name_copy);
	      arg_name = "event";
	      arg_num = &event_num;
	      is_pointer = TRUE;
	    }
	  else if (!strcmp (type_name, "GtkCallback"))
	    {
	      arg_name = "callback";
	      arg_num = &callback_num;
	    }
	  else
	    {
	      arg_name = "arg";
	      arg_num = &param_num;
	    }
	  sprintf (pos, "%s ", type_name);
	  pos += strlen (pos);
	  type_name_len = strlen (type_name);
	  if (type_name_len + 1 < GB_PARAM_TYPE_WIDTH)
	    {
	      num_spaces = GB_PARAM_TYPE_WIDTH - type_name_len - 1;
	      for (i = 0; i < num_spaces; i++)
		*pos++ = ' ';
	    }

	  if (arg_names)
	    {
	      sprintf (pos, "%s%s,\n", is_pointer ? "*" : " ",
		       arg_names[param + 1]);
	      pos += strlen (pos);
	    }
	  else
	    {
	      if (!arg_num || *arg_num == 0)
		sprintf (pos, "%s%s,\n", is_pointer ? "*" : " ", arg_name);
	      else
		sprintf (pos, "%s%s%i,\n", is_pointer ? "*" : " ", arg_name,
			 *arg_num);
	      pos += strlen (pos);
	  
	      if (arg_num)
		{
		  if (*arg_num == 0)
		    *arg_num = 2;
		  else
		    *arg_num += 1;
		}
	    }
	}
    }

  if (arg_names)
    g_strfreev (arg_names);

  /* Add the final user_data parameter, common to all handlers. */
  for (i = 0; i < GB_PARAM_INDENT; i++)
    *pos++ = ' ';
  sprintf (pos, "gpointer         user_data)");
  pos += strlen (pos);

  /* Output the declaration of the handler, which uses the same buffer. */
  source_add_to_buffer (data, GLADE_CALLBACK_DECLARATIONS,
			"\n%s;\n", buffer);

  /* Output the empty handler function, returning FALSE if the return type is
     bool (i.e. for the GdkEvent handlers). */
  source_add_to_buffer (data, GLADE_CALLBACK_SOURCE,
			"\n%s\n{\n\n%s}\n\n",
			buffer,
			query_info.return_type == GTK_TYPE_BOOL
			? "  return FALSE;\n" : "");

  g_free (signal_name_copy);
}


static void
gb_widget_write_signal_connection_source (GbWidgetWriteSourceData * data,
					  const gchar *signal_name,
					  const gchar *connect_object,
					  gboolean connect_after,
					  const gchar *handler_data,
					  const gchar *handler)
{
  if (connect_object && connect_object[0])
    {
      if (connect_after)
	{
	  source_add_to_buffer (data, GLADE_SIGNAL_CONNECTIONS,
				"  g_signal_connect_data ((gpointer) %s, \"%s\",\n"
				"                         G_CALLBACK (%s),\n"
				"                         GTK_OBJECT (%s),\n"
				"                         NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);\n",
				data->wname, signal_name, handler,
				connect_object);
	}
      else
	{
	  source_add_to_buffer (data, GLADE_SIGNAL_CONNECTIONS,
				"  g_signal_connect_swapped ((gpointer) %s, \"%s\",\n"
				"                            G_CALLBACK (%s),\n"
				"                            GTK_OBJECT (%s));\n",
				data->wname, signal_name, handler,
				connect_object);
	}
    }
  else
    {
      if (connect_after)
	{
	  source_add_to_buffer (data, GLADE_SIGNAL_CONNECTIONS,
				"  g_signal_connect_after ((gpointer) %s, \"%s\",\n"
				"                          G_CALLBACK (%s),\n"
				"                          %s);\n",
				data->wname, signal_name, handler,
				handler_data ? handler_data : "NULL");
	}
      else
	{
	  source_add_to_buffer (data, GLADE_SIGNAL_CONNECTIONS,
				"  g_signal_connect ((gpointer) %s, \"%s\",\n"
				"                    G_CALLBACK (%s),\n"
				"                    %s);\n",
				data->wname, signal_name, handler,
				handler_data ? handler_data : "NULL");
	}
    }
}


/* Returns the type name to use for a signal argument or return value, given
   the GtkType from the signal info. It also sets is_pointer to TRUE if the
   argument needs a '*' since it is a pointer. */
static gchar *
get_type_name (GtkType type, gboolean * is_pointer)
{
  gchar *type_name;

  *is_pointer = FALSE;
  type_name = (char*) g_type_name (type);

  switch (type) {
  case G_TYPE_NONE:
  case G_TYPE_CHAR:
  case G_TYPE_UCHAR:
  case G_TYPE_BOOLEAN:
  case G_TYPE_INT:
  case G_TYPE_UINT:
  case G_TYPE_LONG:
  case G_TYPE_ULONG:
  case G_TYPE_FLOAT:
  case G_TYPE_DOUBLE:
  case G_TYPE_POINTER:
    /* These all have normal C type names so they are OK. */
    return type_name;

  case G_TYPE_STRING:
    /* A GtkString is really a gchar*. */
    *is_pointer = TRUE;
    return "gchar";

  case G_TYPE_ENUM:
  case G_TYPE_FLAGS:
    /* We use a gint for both of these. Hopefully a subtype with a decent
       name will be registered and used instead, as GTK+ does itself. */
    return "gint";

  case G_TYPE_BOXED:
    /* The boxed type shouldn't be used itself, only subtypes. Though we
       return 'gpointer' just in case. */
    return "gpointer";

  case G_TYPE_PARAM:
    /* A GParam is really a GParamSpec*. */
    *is_pointer = TRUE;
    return "GParamSpec";

  default:
    break;
  }

  if (!strcmp (type_name, "GtkTypeTextIter"))
    {
      *is_pointer = TRUE;
      return "GtkTextIter";
    }

  if (!strcmp (type_name, "GtkTypeTreeIter"))
    {
      *is_pointer = TRUE;
      return "GtkTreeIter";
    }

  if (!strcmp (type_name, "GtkTypeTreePath"))
    {
      *is_pointer = TRUE;
      return "GtkTreePath";
    }

  /* For all GtkObject subtypes we can use the class name with a "*",
     e.g. 'GtkWidget *'. */
  if (g_type_is_a (type, G_TYPE_OBJECT))
    *is_pointer = TRUE;

  /* All boxed subtypes will be pointers as well. */
  if (g_type_is_a (type, G_TYPE_BOXED))
    *is_pointer = TRUE;

  /* All pointer subtypes will be pointers as well. */
  if (g_type_is_a (type, G_TYPE_POINTER))
    *is_pointer = TRUE;

  return type_name;
}


/* Returns the type name to use for the GdkEvent arg of a signal handler,
   based on the signal name. This assumes that there will only be one GdkEvent
   arg to the signal handler, but that is OK since for the signals we support
   here we know that is true. If any new signals come along with more than one
   GdkEvent arg, it will just use "GdkEvent" for all of them, which is still
   OK. */
static gchar *
get_gdk_event (gchar * signal_name)
{
  static gchar *GbGDKEvents[] =
  {
    "button_press_event",		"GdkEventButton",
    "button_release_event",		"GdkEventButton",
    "motion_notify_event",		"GdkEventMotion",
    "delete_event",			"GdkEvent",
    "destroy_event",			"GdkEvent",
    "expose_event",			"GdkEventExpose",
    "key_press_event",			"GdkEventKey",
    "key_release_event",		"GdkEventKey",
    "enter_notify_event",		"GdkEventCrossing",
    "leave_notify_event",		"GdkEventCrossing",
    "configure_event",			"GdkEventConfigure",
    "focus_in_event",			"GdkEventFocus",
    "focus_out_event",			"GdkEventFocus",
    "map_event",			"GdkEvent",
    "unmap_event",			"GdkEvent",
    "property_notify_event",		"GdkEventProperty",
    "selection_clear_event",		"GdkEventSelection",
    "selection_request_event",		"GdkEventSelection",
    "selection_notify_event",		"GdkEventSelection",
    "proximity_in_event",		"GdkEventProximity",
    "proximity_out_event",		"GdkEventProximity",
    "drag_begin_event",			"GdkEventDragBegin",
    "drag_request_event",		"GdkEventDragRequest",
    "drag_end_event",			"GdkEventDragRequest",
    "drop_enter_event",			"GdkEventDropEnter",
    "drop_leave_event",			"GdkEventDropLeave",
    "drop_data_available_event",	"GdkEventDropDataAvailable",
    "other_event",			"GdkEventOther",
    "client_event",			"GdkEventClient",
    "no_expose_event",			"GdkEventNoExpose",
    NULL
  };

  gint i;

  for (i = 0; GbGDKEvents[i]; i += 2)
    {
      if (!strcmp (signal_name, GbGDKEvents[i]))
	return GbGDKEvents[i + 1];
    }
  return "GdkEvent";
}


static void
init_signal_hash (void)
{
  static const gchar *signal_data[][2] = {
    /*
     * GTK+ Signals.
     */
    { "GtkAccelGroup::accel_activate", "accelgroup,object,key,modifier" },
    { "GtkAccelGroup::accel_changed", "accelgroup,key,modifier,closure" },
    { "GtkCellRenderer::editing_started", "cellrenderer,editable,path" },
    { "GtkCellRendererText::edited", "cellrenderertext,path,new_text" },
    { "GtkCellRendererToggle::toggled", "cellrenderertoggle,path" },
    { "GtkCList::click_column", "clist,column" },
    { "GtkCList::extend_selection", "clist,scroll_type,position,auto_start_selection" },
    { "GtkCList::resize_column", "clist,column,width" },
    { "GtkCList::row_move", "clist,source_row,dest_row" },
    { "GtkCList::scroll_horizontal", "clist,scroll_type,position" },
    { "GtkCList::scroll_vertical", "clist,scroll_type,position" },
    { "GtkCList::select_row", "clist,row,column,event" },
    { "GtkCList::set_scroll_adjustments", "clist,hadjustment,vadjustment" },
    { "GtkCList::unselect_row", "clist,row,column,event" },
    { "GtkCTree::change_focus_row_expansion", "ctree,expansion" },
    { "GtkCTree::tree_collapse", "ctree,node" },
    { "GtkCTree::tree_expand", "ctree,node" },
    { "GtkCTree::tree_move", "ctree,node,new_parent,new_sibling" },
    { "GtkCTree::tree_select_row", "ctree,node,column" },
    { "GtkCTree::tree_unselect_row", "ctree,node,column" },
    { "GtkDialog::response", "dialog,response_id" },
    { "GtkEditable::delete_text", "editable,start_pos,end_pos" },
    { "GtkEditable::insert_text", "editable,new_text,new_text_length,position" },
    { "GtkEntry::delete_from_cursor", "entry,type,count" },
    { "GtkEntry::insert_at_cursor", "entry,string" },
    { "GtkEntry::move_cursor", "entry,step,count,extend_selection" },
    { "GtkEntry::populate_popup", "entry,menu" },
    { "GtkIconView::item_activated", "iconview,path" },
    { "GtkIconView::move_cursor", "iconview,step,count" },
    { "GtkIconView::set_scroll_adjustments", "iconview,hadjustment,vadjustment" },
    { "GtkInputDialog::disable_device", "inputdialog,device" },
    { "GtkInputDialog::enable_device", "inputdialog,device" },
    { "GtkIMContext::commit", "imcontext,string" },
    { "GtkIMContext::delete_surrounding", "imcontext,offset,n_chars" },
    { "GtkLabel::move_cursor", "label,step,count,extend_selection" },
    { "GtkLabel::populate_popup", "label,menu" },
    { "GtkLayout::set_scroll_adjustments", "layout,hadjustment,vadjustment" },
    { "GtkListItem::extend_selection", "listitem,scroll_type,position,auto_start_selection" },
    { "GtkListItem::scroll_horizontal", "listitem,scroll_type,position" },
    { "GtkListItem::scroll_vertical", "listitem,scroll_type,position" },
    { "GtkMenuItem::toggle_size_allocate", "menuitem,allocation" },
    { "GtkMenuItem::toggle_size_request", "menuitem,gint *requisition" },
    { "GtkMenuShell::activate_current", "menushell,force_hide" },
    { "GtkMenuShell::cycle_focus", "menushell,direction" },
    { "GtkMenuShell::move_current", "menushell,direction" },
    { "GtkNotebook::change_current_page", "notebook,offset" },
    { "GtkNotebook::focus_tab", "notebook,type" },
    { "GtkNotebook::move_focus_out", "notebook,direction" },
    { "GtkNotebook::select_page", "notebook,move_focus" },
    { "GtkNotebook::switch_page", "notebook,GtkNotebookPage *page,page_num" },
    { "GtkOldEditable::kill_char", "oldeditable,direction" },
    { "GtkOldEditable::kill_line", "oldeditable,direction" },
    { "GtkOldEditable::kill_word", "oldeditable,direction" },
    { "GtkOldEditable::move_cursor", "oldeditable,x,y" },
    { "GtkOldEditable::move_page", "oldeditable,x,y" },
    { "GtkOldEditable::move_to_column", "oldeditable,column" },
    { "GtkOldEditable::move_to_row", "oldeditable,row" },
    { "GtkOldEditable::move_word", "oldeditable,offset" },
    { "GtkOldEditable::set_editable", "oldeditable,is_editable" },
    { "GtkPaned::cycle_child_focus", "paned,reverse" },
    { "GtkPaned::cycle_handle_focus", "paned,reverse" },
    { "GtkPaned::move_handle", "paned,scroll" },
    { "GtkRange::adjust_bounds", "range,value" },
    { "GtkRange::change_value", "range,scroll,value" },
    { "GtkRange::move_slider", "range,scroll" },
    { "GtkScale::format_value", "scale,value" },
    { "GtkScrolledWindow::move_focus_out", "scrolledwindow,direction" },
    { "GtkScrolledWindow::scroll_child", "scrolledwindow,scroll,horizontal" },
    { "GtkSpinButton::change_value", "spinbutton,scroll" },
    { "GtkSpinButton::input", "spinbutton,gdouble *new_value" },
    { "GtkStatusbar::text_popped", "statusbar,context_id,text" },
    { "GtkStatusbar::text_pushed", "statusbar,context_id,text" },
    { "GtkTextBuffer::apply_tag", "textbuffer,tag,start,end" },
    { "GtkTextBuffer::delete_range", "textbuffer,start,end" },
    { "GtkTextBuffer::insert_child_anchor", "textbuffer,iter,anchor" },
    { "GtkTextBuffer::insert_pixbuf", "textbuffer,iter,pixbuf" },
    { "GtkTextBuffer::insert_text", "textbuffer,iter,text,length" },
    { "GtkTextBuffer::mark_deleted", "textbuffer,mark" },
    { "GtkTextBuffer::mark_set", "textbuffer,iter,mark" },
    { "GtkTextBuffer::remove_tag", "textbuffer,tag,start,end" },
    { "GtkText::set_scroll_adjustments", "text,hadjustment,vadjustment" },
    { "GtkTextTag::event", "texttag,object,event,iter" },
    { "GtkTextTagTable::tag_added", "texttagtable,tag" },
    { "GtkTextTagTable::tag_changed", "texttagtable,tag,size_changed" },
    { "GtkTextTagTable::tag_removed", "texttagtable,tag" },
    { "GtkTextView::delete_from_cursor", "textview,type,count" },
    { "GtkTextView::insert_at_cursor", "textview,string" },
    { "GtkTextView::move_cursor", "textview,step,count,extend_selection" },
    { "GtkTextView::move_focus", "textview,direction" },
    { "GtkTextView::page_horizontally", "textview,count,extend_selection" },
    { "GtkTextView::populate_popup", "textview,menu" },
    { "GtkTextView::set_scroll_adjustments", "textview,hadjustment,vadjustment" },
    { "GtkTipsQuery::widget_entered", "tipsquery,widget,tip_text,tip_private" },
    { "GtkTipsQuery::widget_selected", "tipsquery,widget,tip_text,tip_private,event" },
    { "GtkToolbar::orientation_changed", "toolbar,orientation" },
    { "GtkToolbar::style_changed", "toolbar,style" },
    { "GtkTreeModel::row_changed", "treemodel,path,iter" },
    { "GtkTreeModel::row_deleted", "treemodel,path" },
    { "GtkTreeModel::row_has_child_toggled", "treemodel,path,iter" },
    { "GtkTreeModel::row_inserted", "treemodel,path,iter" },
    { "GtkTreeModel::rows_reordered", "treemodel,path,iter,gint *new_order" },
    { "GtkTreeView::expand_collapse_cursor_row", "treeview,logical,expand,open_all" },
    { "GtkTreeView::move_cursor", "treeview,step,count" },
    { "GtkTreeView::row_activated", "treeview,path,column" },
    { "GtkTreeView::row_collapsed", "treeview,iter,path" },
    { "GtkTreeView::row_expanded", "treeview,iter,path" },
    { "GtkTreeView::select_cursor_row", "treeview,start_editing" },
    { "GtkTreeView::set_scroll_adjustments", "treeview,hadjustment,vadjustment" },
    { "GtkTreeView::test_collapse_row", "treeview,iter,path" },
    { "GtkTreeView::test_expand_row", "treeview,iter,path" },
    { "GtkViewport::set_scroll_adjustments", "viewport,hadjustment,vadjustment" },
    { "GtkWidget::child_notify", "widget,pspec" },
    { "GtkWidget::direction_changed", "widget,old_direction" },
    { "GtkWidget::drag_begin", "widget,drag_context" },
    { "GtkWidget::drag_data_delete", "widget,drag_context" },
    { "GtkWidget::drag_data_get", "widget,drag_context,data,info,time" },
    { "GtkWidget::drag_data_received", "widget,drag_context,x,y,data,info,time" },
    { "GtkWidget::drag_drop", "widget,drag_context,x,y,time" },
    { "GtkWidget::drag_end", "widget,drag_context" },
    { "GtkWidget::drag_leave", "widget,drag_context,time" },
    { "GtkWidget::drag_motion", "widget,drag_context,x,y,time" },
    { "GtkWidget::focus", "widget,direction" },
    { "GtkWidget::grab_notify", "widget,was_grabbed" },
    { "GtkWidget::hierarchy_changed", "widget,previous_toplevel" },
    { "GtkWidget::mnemonic_activate", "widget,group_cycling" },
    { "GtkWidget::parent_set", "widget,old_parent" },
    { "GtkWidget::selection_get", "widget,data,info,time" },
    { "GtkWidget::selection_received", "widget,data,time" },
    { "GtkWidget::show_help", "widget,help_type" },
    { "GtkWidget::size_allocate", "widget,allocation" },
    { "GtkWidget::size_request", "widget,requisition" },
    { "GtkWidget::state_changed", "widget,state" },
    { "GtkWidget::style_set", "widget,previous_style" },
    { "GtkWindow::move_focus", "window,direction" },

    /*
     * libgnomeui Signals.
     */
    { "GnomeClient::connect", "client,restarted" },
    { "GnomeClient::save_yourself", "client,phase,save_style,shutdown,interact_style,fast" },
    { "GnomeColorPicker::color_set", "colorpicker,red,green,blue,alpha" },
    { "GnomeDialog::clicked", "dialog,button_number" },
    { "GnomeFontPicker::font_set", "fontpicker,font_name" },
    { "GnomeIconList::focus_icon", "iconlist,num" },
    { "GnomeIconList::move_cursor", "iconlist,direction,clear_selection" },
    { "GnomeIconList::select_icon", "iconlist,num,event" },
    { "GnomeIconList::text_changed", "iconlist,num,new_text" },
    { "GnomeIconList::unselect_icon", "iconlist,num,event" },
    { "GnomeMDI::add_child", "mdi,child" },
    { "GnomeMDI::app_created", "mdi,app" },
    { "GnomeMDI::child_changed", "mdi,child" },
    { "GnomeMDI::remove_child", "mdi,child" },
    { "GnomePropertyBox::apply", "propertybox,page_num" },
    { "GnomePropertyBox::help", "propertybox,page_num" },

    /*
     * libgnomedb Signals.
     */
    { "GnomeDbBrowser::progress_message", "browser,message" },
    { "GnomeDbDsnConfigDruid::finished", "druid,error" },
    { "GnomeDbForm::model_changed", "form" },
    { "GnomeDbGrid::row_selected", "grid,row" },
    { "GnomeDbGrid::selection_cleared", "grid" },
    { "GnomeDbList::row_selected", "list,row" },
    { "GnomeDbList::selection_cleared", "list" },

    { NULL, NULL }
  };

  gint i;

  glade_signal_hash = g_hash_table_new (g_str_hash, g_str_equal);

  for (i = 0; signal_data[i][0]; i++)
    {
      g_hash_table_insert (glade_signal_hash, (char*) signal_data[i][0],
			   (char*) signal_data[i][1]);
    }
}


/* This returns argument names to use for some known signals.
   The returned array must be freed with g_str_freev(). */
static gchar **
lookup_signal_arg_names (gchar * type, gchar * signal_name,
			 gint num_args_expected)
{
  char *signal_id, *arg_names;
  char **arg_array = NULL;

  if (glade_signal_hash == NULL)
    init_signal_hash ();

  signal_id = g_strdup_printf ("%s::%s", type, signal_name);
  arg_names = g_hash_table_lookup (glade_signal_hash, signal_id);
#if 0
  g_print ("Found signal: %s args: %s\n", signal_id, arg_names);
#endif

  if (arg_names)
    {
      gint num_args = 0;

      arg_array = g_strsplit (arg_names, ",", -1);

      while (arg_array[num_args])
	num_args++;

      /* Check we got the expected number of parameters. If not, output a
	 warning and return NULL. */
      if (num_args != num_args_expected)
	{
	  g_warning ("Internal error: argument names invalid for signal: %s."
		     "Expected %i arguments. Found %i",
		     signal_id, num_args_expected, num_args);
	  g_strfreev (arg_array);
	  arg_array = NULL;
	}
    }

  g_free (signal_id);

  return arg_array;
}


static void
gb_widget_write_accelerators_source (GtkWidget * widget,
				     GbWidgetWriteSourceData * data)
{
  GList *item = data->widget_data->accelerators;
  GladeAccelerator *accel;

  while (item)
    {
      accel = (GladeAccelerator *) item->data;
      item = item->next;

      /* The code to create the accel_group is output in source.c */
      data->need_accel_group = TRUE;
      source_add (data,
		  "  gtk_widget_add_accelerator (%s, \"%s\", accel_group,\n"
		  "                              GDK_%s, (GdkModifierType) %s,\n"
		  "                              GTK_ACCEL_VISIBLE);\n",
		  data->wname, accel->signal, accel->key,
		  glade_util_create_modifiers_string (accel->modifiers));
    }
}

