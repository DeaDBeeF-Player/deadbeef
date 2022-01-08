/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#ifndef __DDBCELLRENDERERTEXTMULTILINE_H__
#define __DDBCELLRENDERERTEXTMULTILINE_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS


#define DDB_TYPE_CELL_EDITABLE_TEXT_VIEW (ddb_cell_editable_text_view_get_type ())
#define DDB_CELL_EDITABLE_TEXT_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_CELL_EDITABLE_TEXT_VIEW, DdbCellEditableTextView))
#define DDB_CELL_EDITABLE_TEXT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_CELL_EDITABLE_TEXT_VIEW, DdbCellEditableTextViewClass))
#define DDB_IS_CELL_EDITABLE_TEXT_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_CELL_EDITABLE_TEXT_VIEW))
#define DDB_IS_CELL_EDITABLE_TEXT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_CELL_EDITABLE_TEXT_VIEW))
#define DDB_CELL_EDITABLE_TEXT_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_CELL_EDITABLE_TEXT_VIEW, DdbCellEditableTextViewClass))

typedef struct _DdbCellEditableTextView DdbCellEditableTextView;
typedef struct _DdbCellEditableTextViewClass DdbCellEditableTextViewClass;
typedef struct _DdbCellEditableTextViewPrivate DdbCellEditableTextViewPrivate;

#define DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE (ddb_cell_renderer_text_multiline_get_type ())
#define DDB_CELL_RENDERER_TEXT_MULTILINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE, DdbCellRendererTextMultiline))
#define DDB_CELL_RENDERER_TEXT_MULTILINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE, DdbCellRendererTextMultilineClass))
#define DDB_IS_CELL_RENDERER_TEXT_MULTILINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE))
#define DDB_IS_CELL_RENDERER_TEXT_MULTILINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE))
#define DDB_CELL_RENDERER_TEXT_MULTILINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE, DdbCellRendererTextMultilineClass))

typedef struct _DdbCellRendererTextMultiline DdbCellRendererTextMultiline;
typedef struct _DdbCellRendererTextMultilineClass DdbCellRendererTextMultilineClass;
typedef struct _DdbCellRendererTextMultilinePrivate DdbCellRendererTextMultilinePrivate;

struct _DdbCellEditableTextView {
    GtkTextView parent_instance;
    DdbCellEditableTextViewPrivate * priv;
    gchar* tree_path;
};

struct _DdbCellEditableTextViewClass {
    GtkTextViewClass parent_class;
};

struct _DdbCellRendererTextMultiline {
    GtkCellRendererText parent_instance;
    DdbCellRendererTextMultilinePrivate * priv;
};

struct _DdbCellRendererTextMultilineClass {
    GtkCellRendererTextClass parent_class;
};


GType ddb_cell_editable_text_view_get_type (void);
void ddb_cell_editable_text_view_start_editing (DdbCellEditableTextView* self, GdkEvent* event);
DdbCellEditableTextView* ddb_cell_editable_text_view_new (void);
DdbCellEditableTextView* ddb_cell_editable_text_view_construct (GType object_type);
GType ddb_cell_renderer_text_multiline_get_type (void);
DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_new (void);
void ddb_cell_renderer_text_multiline_set_columns(DdbCellRendererTextMultiline *self, guint is_mult, guint value);


G_END_DECLS

#endif
