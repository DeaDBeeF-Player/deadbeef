/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel and Shawn T. Amundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_CALENDAR_H__
#define __GTK_CALENDAR_H__

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

/* Not needed, retained for compatibility -Yosh */
#include <gtk/gtksignal.h>


G_BEGIN_DECLS

#define GTK_TYPE_CALENDAR                  (gtk_calendar_get_type ())
#define GTK_CALENDAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CALENDAR, GtkCalendar))
#define GTK_CALENDAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CALENDAR, GtkCalendarClass))
#define GTK_IS_CALENDAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CALENDAR))
#define GTK_IS_CALENDAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CALENDAR))
#define GTK_CALENDAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CALENDAR, GtkCalendarClass))


typedef struct _GtkCalendar	       GtkCalendar;
typedef struct _GtkCalendarClass       GtkCalendarClass;

typedef struct _GtkCalendarPrivate     GtkCalendarPrivate;

typedef enum
{
  GTK_CALENDAR_SHOW_HEADING		= 1 << 0,
  GTK_CALENDAR_SHOW_DAY_NAMES		= 1 << 1,
  GTK_CALENDAR_NO_MONTH_CHANGE		= 1 << 2,
  GTK_CALENDAR_SHOW_WEEK_NUMBERS	= 1 << 3,
  GTK_CALENDAR_WEEK_START_MONDAY	= 1 << 4
} GtkCalendarDisplayOptions;

struct _GtkCalendar
{
  GtkWidget widget;
  
  GtkStyle  *header_style;
  GtkStyle  *label_style;
  
  gint month;
  gint year;
  gint selected_day;
  
  gint day_month[6][7];
  gint day[6][7];
  
  gint num_marked_dates;
  gint marked_date[31];
  GtkCalendarDisplayOptions  display_flags;
  GdkColor marked_date_color[31];
  
  GdkGC *gc;			/* unused */
  GdkGC *xor_gc;		/* unused */

  gint focus_row;
  gint focus_col;

  gint highlight_row;
  gint highlight_col;
  
  GtkCalendarPrivate *priv;
  gchar grow_space [32];

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

struct _GtkCalendarClass
{
  GtkWidgetClass parent_class;
  
  /* Signal handlers */
  void (* month_changed)		(GtkCalendar *calendar);
  void (* day_selected)			(GtkCalendar *calendar);
  void (* day_selected_double_click)	(GtkCalendar *calendar);
  void (* prev_month)			(GtkCalendar *calendar);
  void (* next_month)			(GtkCalendar *calendar);
  void (* prev_year)			(GtkCalendar *calendar);
  void (* next_year)			(GtkCalendar *calendar);
  
};


GType	   gtk_calendar_get_type	(void) G_GNUC_CONST;
GtkWidget* gtk_calendar_new		(void);

gboolean   gtk_calendar_select_month	(GtkCalendar *calendar, 
					 guint	      month,
					 guint	      year);
void	   gtk_calendar_select_day	(GtkCalendar *calendar,
					 guint	      day);

gboolean   gtk_calendar_mark_day	(GtkCalendar *calendar,
					 guint	      day);
gboolean   gtk_calendar_unmark_day	(GtkCalendar *calendar,
					 guint	      day);
void	   gtk_calendar_clear_marks	(GtkCalendar *calendar);


void	   gtk_calendar_set_display_options (GtkCalendar    	      *calendar,
					     GtkCalendarDisplayOptions flags);
GtkCalendarDisplayOptions
           gtk_calendar_get_display_options (GtkCalendar   	      *calendar);
#ifndef GTK_DISABLE_DEPRECATED
void	   gtk_calendar_display_options (GtkCalendar		  *calendar,
					 GtkCalendarDisplayOptions flags);
#endif

void	   gtk_calendar_get_date	(GtkCalendar *calendar, 
					 guint	     *year,
					 guint	     *month,
					 guint	     *day);
#ifndef GTK_DISABLE_DEPRECATED
void	   gtk_calendar_freeze		(GtkCalendar *calendar);
void	   gtk_calendar_thaw		(GtkCalendar *calendar);
#endif

G_END_DECLS

#endif /* __GTK_CALENDAR_H__ */
