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

#ifndef __DDB_SPLITTER_SIZE_MODE_H__
#define __DDB_SPLITTER_SIZE_MODE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define DDB_SPLITTER_TYPE_SIZE_MODE (ddb_splitter_size_mode_get_type ())

typedef enum
{
    DDB_SPLITTER_SIZE_MODE_PROP,
    DDB_SPLITTER_SIZE_MODE_LOCK_C1,
    DDB_SPLITTER_SIZE_MODE_LOCK_C2,
} DdbSplitterSizeMode;

GType ddb_splitter_size_mode_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* !__DDB_SPLITTER_SIZE_MODE_H__ */
