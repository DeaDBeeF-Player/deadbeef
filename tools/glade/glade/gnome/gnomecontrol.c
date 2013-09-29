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

/*
 * Glade only shows Bonobo controls which have a 'glade:show' attribute set
 * to TRUE, e.g. with this in their .server file:
 *
 *	<oaf_attribute name="glade:show" type="boolean" value="TRUE"/>
 *
 */

#include <config.h>

#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkspinbutton.h>

#include "../gb.h"
#include "../palette.h"

#include <bonobo.h>

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-control.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/* This is the key we use to store a pointer to the bonobo widget's property
   bag. */
static gchar *GladePropertyBagKey = "GladePropertyBagKey";

/* This is the Moniker property which stores the type of control. It is saved
   in the XML. */
const gchar *Moniker = "BonoboWidget::moniker";


static GbWidget *control_get_gb_widget (const char *obj_id);


/****************************************************************************
 * This is the dummy GbWidget we put in the palette. It only contains a
 * new() function to create Bonobo controls. We create a GbWidget for each
 * type of control when needed, and it is that that contains functions to
 * create/get/set properties.
 ****************************************************************************/

/* Columns in the GtkTreeView for selecting the type of control. */
enum {
  COL_OBJID,
  COL_DESC,
  COL_LAST
};

static GtkWidget *
control_create (GbWidgetNewData * data, char *obj_id, Bonobo_UIContainer uic)
{
  GtkWidget               *widget;
  BonoboControlFrame      *cf;
  Bonobo_PropertyBag       pb;

  g_return_val_if_fail (obj_id != NULL, NULL);

  widget = bonobo_widget_new_control (obj_id, uic);
  
  g_return_val_if_fail (widget != NULL, NULL);
  cf = bonobo_widget_get_control_frame (BONOBO_WIDGET (widget));

  if (!cf) {
    g_warning ("Control has no frame!");
    gtk_widget_destroy (widget);

    return NULL;
  }

  pb = bonobo_control_frame_get_control_property_bag (cf, NULL);

  gtk_object_set_data (GTK_OBJECT (widget), GladePropertyBagKey, pb);
  gtk_object_set_data (GTK_OBJECT (widget), Moniker, g_strdup (obj_id));

  /* Make sure the GbWidget for this control is created & registered,
     and set the real GbWidget for this control. */
  data->widget_data->gbwidget = control_get_gb_widget (obj_id);

  return widget;
}

static Bonobo_PropertyBag
control_get_pb (GtkWidget *widget)
{
  return gtk_object_get_data (GTK_OBJECT (widget),
			      GladePropertyBagKey);
}

static Bonobo_UIContainer
widget_get_uic (GtkWidget *widget)
{
  GtkWidget *top;

  top = gtk_widget_get_toplevel (widget);

  if (!BONOBO_IS_WINDOW (top))
    return CORBA_OBJECT_NIL;

  /* FIXME: Should we unref this somewhere? */
  return (Bonobo_UIContainer) BONOBO_OBJREF (bonobo_window_get_ui_container (BONOBO_WINDOW (top)));
}

static void
on_control_dialog_ok (GtkWidget *widget,
		      GbWidgetNewData *data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkWidget *dialog;
  GtkTreeSelection *selection;
  char *moniker;
  GtkWidget *new_widget;

  dialog = gtk_widget_get_toplevel (widget);

  view = g_object_get_data (G_OBJECT (dialog), "tree_view");
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_get_selected (selection, &model, &iter);
  gtk_tree_model_get (model, &iter, COL_OBJID, &moniker, -1);

  new_widget = control_create (data, moniker, widget_get_uic (data->parent));
  if (new_widget)
    {
      gb_widget_initialize (new_widget, data);
      data->callback (new_widget, data);
    }
  else
    {
      glade_util_show_message_box (_("Couldn't create the Bonobo control"),
				   NULL);
    }

  gtk_widget_destroy (dialog);
}

static void
on_control_dialog_destroy (GtkWidget *widget,
			   GbWidgetNewData *data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}

static GtkListStore *
do_query (void)
{
  GtkListStore *store;
  Bonobo_ServerInfoList *servers;
#if 0
  /* This is the old query, but too many components are not generally useful
     and cause problems, so we are changing to an 'opt-in' query, where
     we only include components that have a 'glade:show' property set to TRUE.
  */
  const char *query = "repo_ids.has('IDL:Bonobo/Control:1.0') AND NOT "
		      "repo_ids.has('IDL:GNOME/Vertigo/PanelAppletShell:1.0')";
#else
  const char *query = "glade:show";
#endif
  char *sort[] = { "description", NULL };
  CORBA_Environment ev;
  int i;

  Bonobo_ServerInfo *serverinfo;
  const char *desc;
  GtkTreeIter iter;
  
  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  CORBA_exception_init (&ev);

  servers = bonobo_activation_query (query, sort, &ev);
  if (BONOBO_EX (&ev)) {
    g_warning ("query failed: %s\n", ev._id);
    CORBA_exception_free (&ev);
    return store;
  }
  CORBA_exception_free (&ev);
  
#if 0
  g_print ("got %d servers.\n", servers ? servers->_length : 0);
#endif

  for (i = 0; i < servers->_length; i++) {    
    serverinfo = &servers->_buffer[i];
    
    desc = bonobo_server_info_prop_lookup (serverinfo, "description", NULL);
    if (!desc)
      desc = bonobo_server_info_prop_lookup (serverinfo, "name", NULL);
    if (!desc)
      desc = serverinfo->iid;
    
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			COL_OBJID, serverinfo->iid, 
			COL_DESC, desc,
			-1);
  }
  
  CORBA_free (servers);
  
  return store;
}

static void
on_list_selection_changed (GtkTreeSelection *selection, GtkDialog *dialog)
{
  gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK,
				     gtk_tree_selection_get_selected (selection, NULL, NULL));
}

static void
show_control_dialog (GbWidgetNewData *data)
{
  GtkWidget *dialog, *vbox, *label, *list, *scroll;
  GtkListStore *store;
  GtkCellRenderer *ren;

  dialog = glade_util_create_dialog (_("New Bonobo Control"), data->parent,
				     GTK_SIGNAL_FUNC (on_control_dialog_ok),
				     data, &vbox);
  g_object_set (G_OBJECT (dialog),
		"resizable", TRUE,
		"default-width", 400,
		"default-height", 300,
		NULL);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
				     GTK_RESPONSE_OK, FALSE);
  g_signal_connect (dialog, "destroy",
		    G_CALLBACK (on_control_dialog_destroy), data);

  label = gtk_label_new (_("Select a Bonobo Control"));  
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  scroll = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
			 "hadjustment",       NULL,
			 "vadjustment",       NULL,
			 "shadow-type",       GTK_SHADOW_ETCHED_IN,
			 "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
			 "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
			 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

  store = do_query ();

  list = g_object_new (GTK_TYPE_TREE_VIEW,
		       "model",           store,
		       "headers-visible", FALSE,
		       NULL);
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (list)),
		    "changed",
		    G_CALLBACK (on_list_selection_changed), dialog);

  g_object_unref (store);

  ren = gtk_cell_renderer_text_new ();
  
#if 0
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list),
					       -1, _("OAFIID"), ren,
					       "text", COL_OBJID,
					       NULL);
#endif
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list),
					       -1, _("Description"), ren,
					       "text", COL_DESC,
					       NULL);

  gtk_container_add (GTK_CONTAINER (scroll), list);

  g_object_set_data (G_OBJECT (dialog), "tree_view", list);

  gtk_widget_show_all (dialog);
  gtk_grab_add (dialog);
}

static GtkWidget *
gb_bonobo_control_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING) {
    char *moniker;

    moniker = load_string (data->loading_data, Moniker);

    new_widget = control_create (data, moniker, widget_get_uic (data->parent));

    return new_widget;
  } else {
    show_control_dialog (data);
    return NULL;
  }
}

/*
 * Initializes the GbWidget structure.
 */
GbWidget *
gb_bonobo_control_init ()
{
	/* Initialise the GTK type */
	volatile GtkType type;
	type = BONOBO_TYPE_WIDGET;
	
	gb_widget_init_struct (&gbwidget);

	gbwidget.pixmap_struct = gnome_control_xpm;
	gbwidget.tooltip       = _("Bonobo Control");

	gbwidget.gb_widget_new               = gb_bonobo_control_new;

	return &gbwidget;
}


/****************************************************************************
 * This is the GbWidget that will be created for each type of control.
 ****************************************************************************/

/* We use the moniker to ensure the property names are unique in the property
   editor. */
static char *
create_prop_name (const char *moniker, const char *txt)
{
  return g_strconcat (moniker, "::", txt, NULL);
}

/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_bonobo_control_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  Bonobo_PropertyBag pb = control_get_pb (widget);
  char *moniker = gtk_object_get_data (GTK_OBJECT (widget), Moniker);
  GList *key_list, *names;

  g_assert (moniker);

  if (!pb)
	  return;

  key_list = bonobo_pbclient_get_keys (pb, NULL);
  for (names = key_list; names; names = names->next) {
    CORBA_TypeCode tc;
    char          *title, *doc;
    char          *prop = create_prop_name (moniker, names->data);

    tc     = bonobo_pbclient_get_type      (pb, names->data, NULL);
    title  = bonobo_pbclient_get_doc_title (pb, names->data, NULL);
    doc    = bonobo_pbclient_get_doc       (pb, names->data, NULL);

    switch (tc->kind) {
    case CORBA_tk_boolean:
      property_add_bool (prop, title, doc);
      break;
    case CORBA_tk_string:
      property_add_string (prop, title, doc);
      break;
    case CORBA_tk_short:
    case CORBA_tk_ushort:
      /* FIXME: _int_range() ? */
      property_add_int (prop, title, doc);
      break;
    case CORBA_tk_double:
    case CORBA_tk_float:
      property_add_float (prop, title, doc);
      break;
    case CORBA_tk_ulong:
    case CORBA_tk_long:
      property_add_int (prop, title, doc);
      break;
    default:
      g_warning ("Unhandled type %d", tc->kind);
      break;
    }
    g_free (prop);
    CORBA_free (title);
    CORBA_free (doc);
  }
  bonobo_pbclient_free_keys (key_list);
}

/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_bonobo_control_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  Bonobo_PropertyBag pb = control_get_pb (widget);
  char *moniker = gtk_object_get_data (GTK_OBJECT (widget), Moniker);
  GList *key_list, *names;

  g_assert (moniker);

  if (!pb)
    return;

  /* We save the moniker in the XML, though we don't show it in the property
     editor. */
  if (data->action == GB_SAVING) 
    {
      save_string (data, Moniker, moniker);
    }

  key_list = bonobo_pbclient_get_keys (pb, NULL);
  for (names = key_list; names; names = names->next) {
    CORBA_TypeCode tc;
    char          *prop = create_prop_name (moniker, names->data);

    tc  = bonobo_pbclient_get_type (pb, names->data, NULL);
    switch (tc->kind) {
    case CORBA_tk_boolean:
      gb_widget_output_bool (data, prop,
			     bonobo_pbclient_get_boolean (pb, names->data, NULL));
      break;
    case CORBA_tk_string:
    {
      char *str = bonobo_pbclient_get_string (pb, names->data, NULL);

      gb_widget_output_translatable_string (data, prop, str);

      g_free (str);
      break;
    }
    case CORBA_tk_ulong:
      gb_widget_output_int (data, prop,
			    bonobo_pbclient_get_ulong (pb, names->data, NULL));
      break;
    case CORBA_tk_long:
      gb_widget_output_int (data, prop,
			    bonobo_pbclient_get_long (pb, names->data, NULL));
      break;
    case CORBA_tk_short:
      gb_widget_output_int (data, prop,
			    bonobo_pbclient_get_short (pb, names->data, NULL));
      break;
    case CORBA_tk_ushort:
      gb_widget_output_int (data, prop,
			    bonobo_pbclient_get_ushort (pb, names->data, NULL));
      break;
    case CORBA_tk_float:
      gb_widget_output_float (data, prop,
			      bonobo_pbclient_get_float (pb, names->data, NULL));
      break;
    case CORBA_tk_double:
      gb_widget_output_float (data, prop,
			      bonobo_pbclient_get_double (pb, names->data, NULL));
      break;
    default:
      g_warning ("Unhandled type %d", tc->kind);
      break;
    }
    g_free (prop);
  }
  bonobo_pbclient_free_keys (key_list);
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_bonobo_control_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  Bonobo_PropertyBag pb = control_get_pb (widget);
  char *moniker = gtk_object_get_data (GTK_OBJECT (widget), Moniker);
  GList *key_list, *names;

  g_assert (moniker);

  if (!pb)
    return;

  key_list = bonobo_pbclient_get_keys (pb, NULL);
  for (names = key_list; names; names = names->next) {
    CORBA_TypeCode tc;
    char          *prop = create_prop_name (moniker, names->data);

#if 0
    g_print ("Checking property: %s\n", prop);
#endif

    tc  = bonobo_pbclient_get_type (pb, names->data, NULL);
    switch (tc->kind) {
    case CORBA_tk_boolean:
    {
      gboolean val;

      val = gb_widget_input_bool (data, prop);
      if (data->apply)
        bonobo_pbclient_set_boolean (pb, names->data, val, NULL);
      break;
    }
    case CORBA_tk_string:
    {
      const char *str;

      str = gb_widget_input_string (data, prop);
      if (data->apply)
        bonobo_pbclient_set_string (pb, names->data, str, NULL);

      break;
    }
    case CORBA_tk_float:
    {
      gfloat val;

      val = gb_widget_input_float (data, prop);
      if (data->apply)
        bonobo_pbclient_set_float (pb, names->data, val, NULL);
      break;
    }
    case CORBA_tk_double:
    {
      gdouble val;

      val = gb_widget_input_float (data, prop);
      if (data->apply)
        bonobo_pbclient_set_double (pb, names->data, val, NULL);
      break;
    }
    case CORBA_tk_long:
    {
      glong val;

      val = gb_widget_input_int (data, prop);
      if (data->apply)
        bonobo_pbclient_set_long (pb, names->data, val, NULL);
      break;
    }
    case CORBA_tk_ulong:
    {
      glong val;

      val = gb_widget_input_int (data, prop);
      if (data->apply)
        bonobo_pbclient_set_ulong (pb, names->data, val, NULL);
      break;
    }
    case CORBA_tk_short:
    {
      glong val;

      val = gb_widget_input_int (data, prop);
      if (data->apply)
        bonobo_pbclient_set_short (pb, names->data, val, NULL);
      break;
    }
    case CORBA_tk_ushort:
    {
      glong val;

      val = gb_widget_input_int (data, prop);
      if (data->apply)
        bonobo_pbclient_set_ushort (pb, names->data, val, NULL);
      break;
    }
    default:
      g_warning ("Unhandled type %d", tc->kind);
      break;
    }
    g_free (prop);
  }
  bonobo_pbclient_free_keys (key_list);
}

/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_bonobo_control_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  char *moniker = gtk_object_get_data (GTK_OBJECT (widget), Moniker);
  BonoboControlFrame *cf;
  Bonobo_PropertyBag pb;
  GList *key_list, *names;

  if (data->create_widget)
    {
      /* Currently we don't support BonoboWindow, so it will always be NIL,
	 but if we ever do, then it just needs to set bonobo_uic when it is
	 created. */
      source_ensure_decl (data, "  Bonobo_UIContainer bonobo_uic = CORBA_OBJECT_NIL;\n");
      source_add (data,
		  "  %s = bonobo_widget_new_control (%s, bonobo_uic);\n",
		  data->wname,
		  source_make_string (moniker, FALSE));
    }

  gb_widget_write_standard_source (widget, data);

  cf = bonobo_widget_get_control_frame (BONOBO_WIDGET (widget));
  if (!cf)
    return;

  pb = bonobo_control_frame_get_control_property_bag (cf, NULL);

  source_ensure_decl (data, "  BonoboControlFrame *bonobo_cf;\n");
  source_ensure_decl (data, "  Bonobo_PropertyBag bonobo_pb;\n");

  source_add (data,
	      "  bonobo_cf = bonobo_widget_get_control_frame (BONOBO_WIDGET (%s));\n",
	      data->wname);
  source_add (data,
	      "  bonobo_pb = bonobo_control_frame_get_control_property_bag (bonobo_cf, NULL);\n");


  key_list = bonobo_pbclient_get_keys (pb, NULL);
  for (names = key_list; names; names = names->next) {
    CORBA_TypeCode tc;

    tc  = bonobo_pbclient_get_type (pb, names->data, NULL);
    switch (tc->kind) {
    case CORBA_tk_boolean: {
      gboolean val = bonobo_pbclient_get_boolean (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_boolean (bonobo_pb, %s, %s, NULL);\n",
		  source_make_string (names->data, FALSE),
		  val ? "TRUE" : "FALSE");
      break;
    }
    case CORBA_tk_string: {
      char *val = bonobo_pbclient_get_string (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_string (bonobo_pb, %s,",
		  source_make_string (names->data, FALSE));
      source_add (data, " %s, NULL);\n",
		  source_make_string (val, data->use_gettext));
      g_free (val);
      break;
    }
    case CORBA_tk_ulong: {
      gulong val = bonobo_pbclient_get_ulong (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_ulong (bonobo_pb, %s, %lu, NULL);\n",
		  source_make_string (names->data, FALSE),
		  val);
      break;
    }
    case CORBA_tk_long: {
      glong val = bonobo_pbclient_get_long (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_long (bonobo_pb, %s, %li, NULL);\n",
		  source_make_string (names->data, FALSE),
		  val);
      break;
    }
    case CORBA_tk_short: {
      gshort val = bonobo_pbclient_get_short (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_short (bonobo_pb, %s, %i, NULL);\n",
		  source_make_string (names->data, FALSE),
		  val);
      break;
    }
    case CORBA_tk_ushort: {
      gushort val = bonobo_pbclient_get_ushort (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_ushort (bonobo_pb, %s, %u, NULL);\n",
		  source_make_string (names->data, FALSE),
		  val);
      break;
    }
    case CORBA_tk_float: {
      gfloat val = bonobo_pbclient_get_float (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_float (bonobo_pb, %s, %g, NULL);\n",
		  source_make_string (names->data, FALSE),
		  val);
      break;
    }
    case CORBA_tk_double: {
      gdouble val = bonobo_pbclient_get_double (pb, names->data, NULL);
      source_add (data,
		  "  bonobo_pbclient_set_double (bonobo_pb, %s, %g, NULL);\n",
		  source_make_string (names->data, FALSE),
		  val);
      break;
    }
    default:
      g_warning ("Unhandled type %d", tc->kind);
      break;
    }


  }
  bonobo_pbclient_free_keys (key_list);
}

static GbWidget *
control_get_gb_widget (const char *obj_id)
{
  GbWidget *gbwidget;

  /* Check if this GbWidget is already registered. */
  gbwidget = gb_widget_lookup_class (obj_id);

  /* If it isn't registered, create it and register it. */
  if (!gbwidget)
    {
      gbwidget = g_new (GbWidget, 1);

      gb_widget_init_struct (gbwidget);

      /* Fill in the pixmap struct & tooltip */
      gbwidget->pixmap_struct = gnome_control_xpm;
      gbwidget->tooltip       = NULL;
	
      /* Fill in any functions that this Gbwidget has */
      gbwidget->gb_widget_new = NULL;
      gbwidget->gb_widget_create_properties = gb_bonobo_control_create_properties;
      gbwidget->gb_widget_get_properties = gb_bonobo_control_get_properties;
      gbwidget->gb_widget_set_properties = gb_bonobo_control_set_properties;
      gbwidget->gb_widget_write_source	 = gb_bonobo_control_write_source;

#if 0
      g_print ("Registering Bonobo control GbWidget: %s\n", obj_id);
#endif
      gb_widget_register_gbwidget (obj_id, gbwidget);
    }
	
  return gbwidget;
}

