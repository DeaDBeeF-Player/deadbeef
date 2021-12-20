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

#ifndef ddblistviewheader_h
#define ddblistviewheader_h

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DDB_LISTVIEW_HEADER_TYPE (ddb_listview_header_get_type ())
#define DDB_LISTVIEW_HEADER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DDB_LISTVIEW_HEADER_TYPE, DdbListviewHeader))
#define DDB_LISTVIEW_HEADER_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), DDB_LISTVIEW_HEADER_TYPE, DdbListviewHeaderClass))
#define DDB_IS_LISTVIEW_HEADER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_LISTVIEW_HEADER_TYPE))
#define DDB_IS_LISTVIEW_HEADER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), DDB_LISTVIEW_HEADER_TYPE))

typedef struct _DdbListviewHeader DdbListviewHeader;
typedef struct _DdbListviewHeaderPrivate DdbListviewHeaderPrivate;
typedef struct _DdbListviewHeaderClass DdbListviewHeaderClass;

typedef struct {
    void (*context_menu) (DdbListviewHeader *header, int col);
    struct _DdbListviewColumn *(*get_columns)(DdbListviewHeader *header);
    void (*move_column)(DdbListviewHeader *header, DdbListviewColumn *c, int pos);
    void (*set_column_width)(DdbListviewHeader *header, DdbListviewColumn *c, int width);
    void (*columns_changed)(DdbListviewHeader *header);
    int (*get_list_height)(DdbListviewHeader *header);
    void (*col_sort) (DdbListviewHeader *header, int sort_order, void *user_data);
    void (*update_scroll_ref_point) (DdbListviewHeader *header);
}  ddb_listview_header_delegate_t;

struct _DdbListviewHeader {
    GtkDrawingArea parent;
    ddb_listview_header_delegate_t *delegate;
};

struct _DdbListviewHeaderClass {
    GtkDrawingAreaClass parent_class;
};

G_END_DECLS

GType
ddb_listview_header_get_type(void);

GtkWidget *
ddb_listview_header_new(void);

void
ddb_listview_header_set_hscrollpos(DdbListviewHeader *header, int hscrollpos);

gboolean
ddb_listview_header_is_sizing (DdbListviewHeader *header);

void
ddb_listview_header_update_fonts (DdbListviewHeader *header);

#endif /* ddblistviewheader_h */
