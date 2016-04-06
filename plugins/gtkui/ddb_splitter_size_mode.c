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

#include "ddb_splitter_size_mode.h"

GType
ddb_splitter_size_mode_get_type (void)
{
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
        static const GEnumValue values[] =
        {
            { DDB_SPLITTER_SIZE_MODE_PROP,   "DDB_SPLITTER_SIZE_MODE_PROP",   "Proportional sizing",       },
            { DDB_SPLITTER_SIZE_MODE_LOCK_C1, "DDB_SPLITTER_SIZE_MODE_LOCK_C1", "Size of first child is locked",     },
            { DDB_SPLITTER_SIZE_MODE_LOCK_C2,   "DDB_SPLITTER_SIZE_MODE_LOCK_C2", "Size of second child is locked", },
            { 0, NULL, NULL, },
        };

        type = g_enum_register_static ("DdbSplitterSizeMode", values);
    }

    return type;
}
