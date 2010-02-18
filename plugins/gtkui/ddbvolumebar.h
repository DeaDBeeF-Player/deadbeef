/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __DDBVOLUMEBAR_H
#define __DDBVOLUMEBAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DDB_TYPE_VOLUMEBAR (ddb_volumebar_get_type ())
#define DDB_VOLUMEBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DDB_TYPE_VOLUMEBAR, DdbVolumeBar))
#define DDB_VOLUMEBAR_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), DDB_TYPE_VOLUMEBAR, DdbVolumeBarClass))
#define DDB_IS_VOLUMEBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_VOLUMEBAR))
#define DDB_IS_VOLUMEBAR_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), DDB_TYPE_VOLUMEBAR))
#define DDB_VOLUMEBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_VOLUMEBAR, DdbVolumeBarClass))

typedef struct _DdbVolumeBar DdbVolumeBar;
typedef struct _DdbVolumeBarClass DdbVolumeBarClass;

typedef void * DdbVolumeBarIter;
typedef void * DdbVolumeBarColIter;

struct _DdbVolumeBar {
    GtkWidget parent;
};

struct _DdbVolumeBarClass {
  GtkWidgetClass parent_class;
};

GType ddb_volumebar_get_type(void) G_GNUC_CONST;
GtkWidget * ddb_volumebar_new(void);

G_END_DECLS

#endif // __DDBVOLUMEBAR_H
