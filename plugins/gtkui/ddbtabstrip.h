/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __DDBTABSTRIP_H
#define __DDBTABSTRIP_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DDB_TYPE_TABSTRIP (ddb_tabstrip_get_type ())
#define DDB_TABSTRIP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DDB_TYPE_TABSTRIP, DdbTabStrip))
#define DDB_TABSTRIP_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), DDB_TYPE_TABSTRIP, DdbTabStripClass))
#define DDB_IS_TABSTRIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_TABSTRIP))
#define DDB_IS_TABSTRIP_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), DDB_TYPE_TABSTRIP))
#define DDB_TABSTRIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_TABSTRIP, DdbTabStripClass))

typedef struct _DdbTabStrip DdbTabStrip;
typedef struct _DdbTabStripClass DdbTabStripClass;

typedef void * DdbTabStripIter;
typedef void * DdbTabStripColIter;

struct _DdbTabStrip {
    GtkWidget parent;
    int hscrollpos;
    int dragging;
    int prepare;
    int dragpt[2];
    int prev_x;
    int movepos;
    guint scroll_timer;
    int scroll_direction;
};

struct _DdbTabStripClass {
  GtkWidgetClass parent_class;
};

GType ddb_tabstrip_get_type(void) G_GNUC_CONST;
GtkWidget * ddb_tabstrip_new(void);
void ddb_tabstrip_refresh (DdbTabStrip *ts);

G_END_DECLS

#endif // __DDBTABSTRIP_H
