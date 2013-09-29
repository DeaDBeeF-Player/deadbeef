/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-2000  Damon Chaplin
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
 * Defines the extra data that Glade keeps for each widget, and functions to
 * manipulate it.
 */

#include "gladeconfig.h"

#include "glade_widget_data.h"


static GList* glade_widget_data_copy_signals	  (GList	*signals);
static GladeSignal* glade_widget_data_copy_signal (GladeSignal	*signal);
static GList* glade_widget_data_copy_accels	  (GList	*accels);
static GladeAccelerator* glade_widget_data_copy_accel(GladeAccelerator *accel);
static GList* glade_widget_data_copy_relations	(GList		*relations);
static GladeRelation* glade_widget_data_copy_relation	(GladeRelation	*relation);



GladeWidgetData *
glade_widget_data_new		(GbWidget *gbwidget)
{
  GladeWidgetData *wdata;

  g_return_val_if_fail (gbwidget != NULL, NULL);

  wdata = g_new (GladeWidgetData, 1);

  wdata->flags = GLADE_VISIBLE | GLADE_SENSITIVE | GLADE_STYLE_IS_UNNAMED
    | GLADE_STYLE_PROPAGATE | GLADE_SIZE_NOT_ALLOCATED;
  wdata->width = 0;
  wdata->height = 0;
  wdata->events = 0;
  wdata->tooltip = NULL;
  wdata->signals = NULL;
  wdata->accelerators = NULL;
  wdata->relations = NULL;
#ifdef GLADE_STYLE_SUPPORT
  wdata->gbstyle = gb_widget_default_gb_style;
  gb_widget_ref_gb_style (gb_widget_default_gb_style);
#endif

  /* C options. */
  wdata->source_file = NULL;
  wdata->public_field = 1;

  /* C++ options. */
  wdata->cxx_separate_file = 0;
  wdata->cxx_separate_class = 0;
  wdata->cxx_visibility = 0;

  wdata->gbwidget = gbwidget;

  return wdata;
}


GladeWidgetData *
glade_widget_data_copy		(GladeWidgetData *wdata)
{
  GladeWidgetData *new_wdata = g_new (GladeWidgetData, 1);

  new_wdata->flags = wdata->flags;
  new_wdata->width = wdata->width;
  new_wdata->height = wdata->height;
  new_wdata->events = wdata->events;
  new_wdata->tooltip = g_strdup (wdata->tooltip);

  new_wdata->signals = glade_widget_data_copy_signals (wdata->signals);
  new_wdata->accelerators = glade_widget_data_copy_accels (wdata->accelerators);
  new_wdata->relations = glade_widget_data_copy_relations (wdata->relations);

  /* C options. */
  new_wdata->source_file = g_strdup (wdata->source_file);
  new_wdata->public_field = wdata->public_field;

  /* C++ options. */
  new_wdata->cxx_separate_file = wdata->cxx_separate_file;
  new_wdata->cxx_separate_class = wdata->cxx_separate_class;
  new_wdata->cxx_visibility = wdata->cxx_visibility;

  new_wdata->gbwidget = wdata->gbwidget;

  return new_wdata;
}


static GList*
glade_widget_data_copy_signals	(GList		*signals)
{
  GList *signals_copy = NULL, *elem;
  GladeSignal *signal;

  for (elem = signals; elem; elem = elem->next)
    {
      signal = (GladeSignal*) elem->data;
      signals_copy = g_list_prepend (signals_copy,
				     glade_widget_data_copy_signal (signal));
    }

  return g_list_reverse (signals_copy);
}


static GladeSignal*
glade_widget_data_copy_signal	(GladeSignal	*signal)
{
  GladeSignal *signal_copy;

  signal_copy = g_new (GladeSignal, 1);

  signal_copy->name    = g_strdup (signal->name);
  signal_copy->handler = g_strdup (signal->handler);
  signal_copy->object  = g_strdup (signal->object);
  signal_copy->after   = signal->after;
  signal_copy->data    = g_strdup (signal->data);
  signal_copy->last_modification_time = signal->last_modification_time;

  return signal_copy;
}


static GList*
glade_widget_data_copy_accels	(GList		*accels)
{
  GList *accels_copy = NULL, *elem;
  GladeAccelerator *accel;

  for (elem = accels; elem; elem = elem->next)
    {
      accel = (GladeAccelerator*) elem->data;
      accels_copy = g_list_prepend (accels_copy,
				    glade_widget_data_copy_accel (accel));
    }

  return g_list_reverse (accels_copy);
}


static GladeAccelerator*
glade_widget_data_copy_accel	(GladeAccelerator *accel)
{
  GladeAccelerator *accel_copy;

  accel_copy = g_new (GladeAccelerator, 1);

  accel_copy->modifiers = accel->modifiers;
  accel_copy->key       = g_strdup (accel->key);
  accel_copy->signal    = g_strdup (accel->signal);

  return accel_copy;
}


static GList*
glade_widget_data_copy_relations	(GList		*relations)
{
  GList *relations_copy = NULL, *elem;
  GladeRelation *relation;

  for (elem = relations; elem; elem = elem->next)
    {
      relation = (GladeRelation*) elem->data;
      relations_copy = g_list_prepend (relations_copy,
				       glade_widget_data_copy_relation (relation));
    }

  return g_list_reverse (relations_copy);
}


static GladeRelation*
glade_widget_data_copy_relation	(GladeRelation	*relation)
{
  GladeRelation *relation_copy;
  GList *elem;

  relation_copy = g_new (GladeRelation, 1);

  relation_copy->relationship = relation->relationship;

  relation_copy->targets = NULL;
  for (elem = relation->targets; elem; elem = elem->next)
    {
      GtkWidget *target = elem->data;
      relation_copy->targets = g_list_prepend (relation_copy->targets, target);
      /* We add a weak pointer to the widget, so the element data will be
	 set to NULL if the widget is destroyed. */
      g_object_add_weak_pointer (G_OBJECT (target),
				 &relation_copy->targets->data);
    }
  relation_copy->targets = g_list_reverse (relation_copy->targets);

  return relation_copy;
}


void
glade_widget_data_free		(GladeWidgetData *wdata)
{
  g_free (wdata->tooltip);
  g_free (wdata->source_file);

  glade_widget_data_clear_accels (wdata);
  glade_widget_data_clear_signals (wdata);
  glade_widget_data_clear_relations (wdata);

  g_free (wdata);
}


void
glade_widget_data_clear_accels  (GladeWidgetData *wdata)
{
  GList *item;
  GladeAccelerator *accel;

  item = wdata->accelerators;
  while (item)
    {
      accel = (GladeAccelerator *) item->data;
      glade_widget_data_free_accel (accel);
      item = item->next;
    }
  g_list_free (wdata->accelerators);
  wdata->accelerators = NULL;
}


void
glade_widget_data_free_accel  (GladeAccelerator *accel)
{
      g_free (accel->key);
      g_free (accel->signal);
}


void
glade_widget_data_set_accels    (GladeWidgetData *wdata,
				 GList	         *accels)
{
  glade_widget_data_clear_accels (wdata);
  wdata->accelerators = accels;
}


void
glade_widget_data_clear_signals (GladeWidgetData *wdata)
{
  GList *item;
  GladeSignal *signal;

  item = wdata->signals;
  while (item)
    {
      signal = (GladeSignal *) item->data;
      glade_widget_data_free_signal (signal);
      item = item->next;
    }
  g_list_free (wdata->signals);
  wdata->signals = NULL;
}


void
glade_widget_data_free_signal (GladeSignal *signal)
{
      g_free (signal->name);
      g_free (signal->handler);
      g_free (signal->object);
      g_free (signal->data);
}


void
glade_widget_data_set_signals   (GladeWidgetData *wdata,
				 GList	         *signals)
{
  glade_widget_data_clear_signals (wdata);
  wdata->signals = signals;
}


void
glade_widget_data_clear_relations  (GladeWidgetData *wdata)
{
  GList *item;
  GladeRelation *relation;

  item = wdata->relations;
  while (item)
    {
      relation = (GladeRelation *) item->data;
      glade_widget_data_free_relation (relation);
      item = item->next;
    }
  g_list_free (wdata->relations);
  wdata->relations = NULL;
}


void
glade_widget_data_free_relation  (GladeRelation *relation)
{
  GList *elem;

  /* We need to remove the weak pointers. */
  for (elem = relation->targets; elem; elem = elem->next)
    g_object_remove_weak_pointer (G_OBJECT (elem->data), &elem->data);

  g_list_free (relation->targets);
}


/* The relations should already have weak pointers added to the widgets. */
void
glade_widget_data_set_relations    (GladeWidgetData *wdata,
				    GList	    *relations)
{
  glade_widget_data_clear_relations (wdata);
  wdata->relations = relations;
}


/* This sets one relation type, used when the relation dialog is applied.
   The targets should be weak pointers to the widgets. */
void
glade_widget_data_set_relation   (GladeWidgetData	*wdata,
				  AtkRelationType        relationship,
				  GList			*targets)
{
  GList *elem;
  GladeRelation *relation;

  /* If we already have a relation of this type then replace the targets,
     else add a new relation. */
  for (elem = wdata->relations; elem; elem = elem->next)
    {
      relation = (GladeRelation *) elem->data;

      if (relation->relationship == relationship)
	{
	  glade_widget_data_free_relation (relation);
	  relation->targets = targets;
	  return;
	}
    }

  relation = g_new (GladeRelation, 1);
  relation->relationship = relationship;
  relation->targets = targets;

  wdata->relations = g_list_prepend (wdata->relations, relation);
}


/* This adds one target widget to a relation, or creates a new relation if
   it doesn't exist yet. It will use a weak pointer to the widget. */
void
glade_widget_data_add_relation   (GladeWidgetData	*wdata,
				  AtkRelationType        relationship,
				  GtkWidget		*target)
{
  GList *elem;
  GladeRelation *relation;

  /* If we already have a relation of this type then add the new target,
     else add a new relation. */
  for (elem = wdata->relations; elem; elem = elem->next)
    {
      relation = (GladeRelation *) elem->data;

      if (relation->relationship == relationship)
	{
	  relation->targets = g_list_prepend (relation->targets, target);
	  g_object_add_weak_pointer (G_OBJECT (target),
				     &relation->targets->data);
	  return;
	}
    }

  relation = g_new (GladeRelation, 1);
  relation->relationship = relationship;
  relation->targets = g_list_append (NULL, target);
  g_object_add_weak_pointer (G_OBJECT (target),
			     &relation->targets->data);

  wdata->relations = g_list_prepend (wdata->relations, relation);
}

