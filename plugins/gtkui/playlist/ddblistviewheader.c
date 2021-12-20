/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include "ddblistviewheader.h"

struct _DdbListviewHeaderPrivate {
};

#define DDB_LISTVIEW_HEADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_LISTVIEW_HEADER_TYPE, DdbListviewHeaderPrivate))
G_DEFINE_TYPE (DdbListviewHeader, ddb_listview_header, GTK_TYPE_DRAWING_AREA);

static void ddb_listview_header_class_init(DdbListviewHeaderClass *klass);
static void ddb_listview_header_init(DdbListviewHeader *listview);
static void ddb_listview_header_destroy(GObject *object);

static void
ddb_listview_header_class_init(DdbListviewHeaderClass *class) {
    GObjectClass *object_class = (GObjectClass *) class;
    object_class->finalize = ddb_listview_header_destroy;
    g_type_class_add_private(class, sizeof(DdbListviewHeaderPrivate));
}

static void
ddb_listview_header_init(DdbListviewHeader *listview) {

    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(listview);
    memset (priv, 0, sizeof (DdbListviewHeaderPrivate));
}
GtkWidget *
ddb_listview_header_new(void) {
    return GTK_WIDGET(g_object_new(ddb_listview_header_get_type(), NULL));
}

static void
ddb_listview_header_destroy(GObject *object) {
}

