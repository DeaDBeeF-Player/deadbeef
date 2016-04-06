/*
 * Copyright (c) 2016 Christian Boxdörfer <christian.boxdoerfer@posteo.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifndef __DDB_SPLITTER_H__
#define __DDB_SPLITTER_H__

#include <gtk/gtk.h>
#include "ddb_splitter_size_mode.h"

G_BEGIN_DECLS

typedef struct _DdbSplitterPrivate DdbSplitterPrivate;
typedef struct _DdbSplitterClass   DdbSplitterClass;
typedef struct _DdbSplitter        DdbSplitter;

#define DDB_TYPE_SPLITTER             (ddb_splitter_get_type ())
#define DDB_SPLITTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_SPLITTER, DdbSplitter))
#define DDB_SPLITTER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_SPLITTER, DdbSplitterClass))
#define DDB_IS_SPLITTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_SPLITTER))
#define DDB_IS_SPLITTER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_SPLITTER))
#define DDB_SPLITTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_SPLITTER, DdbSplitterClass))

struct _DdbSplitterClass
{
  /*< private >*/
  GtkContainerClass __parent__;

  /* padding for further expansion */
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
};

/**
 * DdbSplitter:
 *
 *  The #DdbSplitter struct contains only private fields
* and should not be directly accessed.
 **/
struct _DdbSplitter
{
  /*< private >*/
  GtkContainer         __parent__;
  DdbSplitterPrivate *priv;
};

GType
ddb_splitter_get_type (void) G_GNUC_CONST;

GtkWidget
*ddb_splitter_new (GtkOrientation orientation);

DdbSplitterSizeMode
ddb_splitter_get_size_mode (const DdbSplitter *splitter);
GtkOrientation
ddb_splitter_get_orientation (const DdbSplitter *splitter);
gfloat
ddb_splitter_get_proportion (const DdbSplitter *splitter);
void
ddb_splitter_set_size_mode (DdbSplitter *splitter, DdbSplitterSizeMode size_mode);
gboolean
ddb_splitter_add_child_at_pos (DdbSplitter *splitter, GtkWidget *child, guint pos);
void
ddb_splitter_remove_c1 (DdbSplitter *splitter);
void
ddb_splitter_remove_c2 (DdbSplitter *splitter);
void
ddb_splitter_set_proportion (DdbSplitter *splitter, gfloat proportion);
guint
ddb_splitter_get_child1_size (DdbSplitter *splitter);
guint
ddb_splitter_get_child2_size (DdbSplitter *splitter);
void
ddb_splitter_set_child1_size (DdbSplitter *splitter, guint size);
void
ddb_splitter_set_child2_size (DdbSplitter *splitter, guint size);

G_END_DECLS

#endif /* !__DDB_SPLITTER_H__ */
