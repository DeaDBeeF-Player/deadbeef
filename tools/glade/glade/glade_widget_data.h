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
#ifndef GLADE_WIDGET_DATA_H
#define GLADE_WIDGET_DATA_H

/*
 * Defines the extra data that Glade keeps for each widget, and functions to
 * manipulate it.
 */

#include <time.h>
#include <atk/atkobject.h>
#include <atk/atkrelation.h>
#include "glade.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Flags in GladeWidgetData */
enum
{
  GLADE_VISIBLE		   = 1 << 0, /* If the widget is initially visible */
  GLADE_SENSITIVE	   = 1 << 1, /* If the widget is initially sensitive */
  GLADE_GRAB_DEFAULT	   = 1 << 2, /* If it grabs the default */
  GLADE_GRAB_FOCUS	   = 1 << 3, /* If it grabs the focus */
  GLADE_STYLE_IS_UNNAMED   = 1 << 4, /* If it's using its own unnamed style */
  GLADE_STYLE_PROPAGATE	   = 1 << 5, /* If it propgates style to children */
  GLADE_ACTIVE		   = 1 << 6, /* If it is initially active (toggles) */
  GLADE_WIDTH_SET	   = 1 << 7, /* If the width is set explicitly */
  GLADE_HEIGHT_SET	   = 1 << 8, /* If the height is set explicitly */

  GLADE_SIZE_NOT_ALLOCATED = 1 << 9  /* Internally used so that a widget's size
					and position properties aren't
					displayed until its area has been
					allocated. */
};

typedef struct _GladeWidgetData  GladeWidgetData;
struct _GladeWidgetData
{
  guint16	 flags;
  gint16	 width;
  gint16	 height;
  gint		 events;
  gchar		*tooltip;
  GList		*signals;		/* A list of GladeSignal*. */
  GList		*accelerators;		/* A list of GladeAccelerator*. */
  GList		*relations;		/* A list of GladeRelation*. */
#ifdef GLADE_STYLE_SUPPORT
  GbStyle	*gbstyle;
#endif

  /* C options (currently not used). */
  gchar		*source_file;
  guint		 public_field : 1;

  /* C++ options. */
  guint		 cxx_separate_file : 1;
  guint		 cxx_separate_class : 1;
  guint		 cxx_visibility : 2;	/* 0=private 1=protected 2=public */

  GbWidget      *gbwidget;
};

typedef struct _GladeSignal GladeSignal;
struct _GladeSignal
{
  gchar		*name;
  gchar		*handler;
  gchar		*object;
  gboolean	 after;
  gchar		*data;
  /* This records the last time that handler, object or data were changed,
     since we have to change the prototype output when they change.
     name can't be changed, or it would become a completely different signal */
  time_t	 last_modification_time;
};

typedef struct _GladeAccelerator GladeAccelerator;
struct _GladeAccelerator
{
  GdkModifierType modifiers;
  gchar		*key;
  gchar		*signal;
};

typedef struct _GladeRelation GladeRelation;
struct _GladeRelation
{
  AtkRelationType relationship;
  /* A list of GtkWidget*, with weak pointers to the widgets, so if the
     widget gets destroyed the pointer is set to NULL. */
  GList		*targets;
};


GladeWidgetData* glade_widget_data_new	    (GbWidget	     *gbwidget);
GladeWidgetData* glade_widget_data_copy	    (GladeWidgetData *wdata);

void	    glade_widget_data_free	    (GladeWidgetData *wdata);

void	    glade_widget_data_clear_accels  (GladeWidgetData *wdata);
void	    glade_widget_data_set_accels    (GladeWidgetData *wdata,
					     GList	     *accels);
void	    glade_widget_data_free_accel    (GladeAccelerator *accel);

void	    glade_widget_data_clear_signals (GladeWidgetData *wdata);
void	    glade_widget_data_set_signals   (GladeWidgetData *wdata,
					     GList	     *signals);
void	    glade_widget_data_free_signal   (GladeSignal     *signal);

void	    glade_widget_data_clear_relations(GladeWidgetData	*wdata);
void	    glade_widget_data_set_relations  (GladeWidgetData	*wdata,
					      GList		*relations);
void	    glade_widget_data_set_relation   (GladeWidgetData	*wdata,
					      AtkRelationType    relationship,
					      GList		*targets);
void	    glade_widget_data_add_relation   (GladeWidgetData	*wdata,
					      AtkRelationType    relationship,
					      GtkWidget		*target);
void	    glade_widget_data_free_relation  (GladeRelation	*relation);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_WIDGET_DATA_H */
