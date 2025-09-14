/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include "support.h"
#include "ddbcellrenderertextmultiline.h"


#define _g_free0(var) (var = (g_free (var), NULL))

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _gtk_tree_path_free0(var) ((var == NULL) ? NULL : (var = (gtk_tree_path_free (var), NULL)))

struct _DdbCellRendererTextMultilinePrivate {
    DdbCellEditableTextView* entry;
    gulong focus_out_id;

    gulong populate_popup_id;
    guint entry_menu_popdown_timeout;
    gboolean in_entry_menu;

    guint is_mult_column;
    guint value_column;
};

struct _DdbCellEditableTextViewPrivate {
    gboolean editing_canceled;
};

static gpointer ddb_cell_editable_text_view_parent_class = NULL;
static GtkCellEditableIface* ddb_cell_editable_text_view_gtk_cell_editable_parent_iface = NULL;
static gpointer ddb_cell_renderer_text_multiline_parent_class = NULL;

GType ddb_cell_editable_text_view_get_type (void);
enum  {
    DDB_CELL_EDITABLE_TEXT_VIEW_DUMMY_PROPERTY
};
static gboolean ddb_cell_editable_text_view_real_key_press_event (GtkWidget* base, GdkEventKey* event);
static void ddb_cell_editable_text_view_real_start_editing (GtkCellEditable* base, GdkEvent* event);
DdbCellEditableTextView* ddb_cell_editable_text_view_new (void);
DdbCellEditableTextView* ddb_cell_editable_text_view_construct (GType object_type);
static void ddb_cell_editable_text_view_finalize (GObject* obj);
GType ddb_cell_renderer_text_multiline_get_type (void);
DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_construct (GType object_type);

#define DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE, DdbCellRendererTextMultilinePrivate))
#define DDB_CELL_EDITABLE_TEXT_VIEW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_TYPE_CELL_EDITABLE_TEXT_VIEW, DdbCellEditableTextViewPrivate))
enum  {
    DDB_CELL_RENDERER_TEXT_MULTILINE_DUMMY_PROPERTY
};
static void ddb_cell_renderer_text_multiline_gtk_cell_renderer_text_editing_done (DdbCellEditableTextView* entry, DdbCellRendererTextMultiline* _self_);
static gboolean ddb_cell_renderer_text_multiline_gtk_cell_renderer_focus_out_event (DdbCellEditableTextView* entry, GdkEvent* event, DdbCellRendererTextMultiline* _self_);
static GtkCellEditable* ddb_cell_renderer_text_multiline_real_start_editing (
                                                                             GtkCellRenderer      *cell,
                                                                             GdkEvent             *event,
                                                                             GtkWidget            *widget,
                                                                             const gchar          *path,
                                                                             const GdkRectangle   *background_area,
                                                                             const GdkRectangle   *cell_area,
                                                                             GtkCellRendererState  flags
                                                                             );
DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_new (void);
DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_construct (GType object_type);
static void ddb_cell_renderer_text_multiline_finalize (GObject* obj);


static gboolean ddb_cell_editable_text_view_real_key_press_event (GtkWidget* base, GdkEventKey* event) {
    DdbCellEditableTextView * self;
    gboolean result = FALSE;
    gboolean res;
    guint keyval;
    self = (DdbCellEditableTextView*) base;
    g_return_val_if_fail (event != NULL, FALSE);
    res = TRUE;
    keyval = event->keyval;
    if (keyval == ((guint) GDK_Return)) {
        if ((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0) {
            res = GTK_WIDGET_CLASS (ddb_cell_editable_text_view_parent_class)->key_press_event ((GtkWidget*) GTK_TEXT_VIEW (self), event);
        } else {
            gtk_cell_editable_editing_done ((GtkCellEditable*) self);
            gtk_cell_editable_remove_widget ((GtkCellEditable*) self);
            result = TRUE;
            return result;
        }
    } else {
        if (keyval == ((guint) GDK_Escape)) {
            self->priv->editing_canceled = TRUE;
            gtk_cell_editable_editing_done ((GtkCellEditable*) self);
            gtk_cell_editable_remove_widget ((GtkCellEditable*) self);
            result = TRUE;
            return result;
        } else {
            res = GTK_WIDGET_CLASS (ddb_cell_editable_text_view_parent_class)->key_press_event ((GtkWidget*) GTK_TEXT_VIEW (self), event);
        }
    }
    result = res;
    return result;
}

static void ddb_cell_editable_text_view_real_start_editing (GtkCellEditable* base, GdkEvent* event) {
}

DdbCellEditableTextView* ddb_cell_editable_text_view_construct (GType object_type) {
    DdbCellEditableTextView * self;
    self = g_object_newv (object_type, 0, NULL);
    return self;
}


DdbCellEditableTextView* ddb_cell_editable_text_view_new (void) {
    return ddb_cell_editable_text_view_construct (DDB_TYPE_CELL_EDITABLE_TEXT_VIEW);
}

enum
{
  PROP_0,

  PROP_EDITING_CANCELED,

  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
ddb_cell_editable_text_view_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  DdbCellEditableTextView *self = DDB_CELL_EDITABLE_TEXT_VIEW (object);

  switch (property_id)
    {
    case PROP_EDITING_CANCELED:
      self->priv->editing_canceled = g_value_get_boolean (value);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ddb_cell_editable_text_view_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  DdbCellEditableTextView *self = DDB_CELL_EDITABLE_TEXT_VIEW (object);

  switch (property_id)
    {
    case PROP_EDITING_CANCELED:
      g_value_set_boolean (value, self->priv->editing_canceled);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void ddb_cell_editable_text_view_class_init (DdbCellEditableTextViewClass * klass) {
    g_type_class_add_private (klass, sizeof (DdbCellEditableTextViewPrivate));
    ddb_cell_editable_text_view_parent_class = g_type_class_peek_parent (klass);
    GTK_WIDGET_CLASS (klass)->key_press_event = ddb_cell_editable_text_view_real_key_press_event;
    G_OBJECT_CLASS (klass)->finalize = ddb_cell_editable_text_view_finalize;

    G_OBJECT_CLASS (klass)->set_property = ddb_cell_editable_text_view_set_property;
    G_OBJECT_CLASS (klass)->get_property = ddb_cell_editable_text_view_get_property;

    obj_properties[PROP_EDITING_CANCELED] =
        g_param_spec_boolean ("editing-canceled",
                "Editing canceled",
                "Indicates whether editing on the cell has been canceled",
                FALSE,
                G_PARAM_READWRITE);

    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    g_object_class_install_property (gobject_class, PROP_EDITING_CANCELED, obj_properties[PROP_EDITING_CANCELED]);
}

void ddb_cell_editable_text_view_start_editing (DdbCellEditableTextView* self, GdkEvent* event) {
    g_return_if_fail (self != NULL);
    g_return_if_fail (event != NULL);
}

static void ddb_cell_editable_text_view_gtk_cell_editable_interface_init (GtkCellEditableIface * iface) {
    ddb_cell_editable_text_view_gtk_cell_editable_parent_iface = g_type_interface_peek_parent (iface);
    iface->start_editing = ddb_cell_editable_text_view_real_start_editing;
}


static void ddb_cell_editable_text_view_instance_init (DdbCellEditableTextView * self) {
    self->priv = DDB_CELL_EDITABLE_TEXT_VIEW_GET_PRIVATE (self);
    self->priv->editing_canceled = FALSE;
}


static void ddb_cell_editable_text_view_finalize (GObject* obj) {
    DdbCellEditableTextView * self;
    self = DDB_CELL_EDITABLE_TEXT_VIEW (obj);
    _g_free0 (self->tree_path);
    G_OBJECT_CLASS (ddb_cell_editable_text_view_parent_class)->finalize (obj);
}


GType ddb_cell_editable_text_view_get_type (void) {
    static volatile gsize ddb_cell_editable_text_view_type_id__volatile = 0;
    if (g_once_init_enter ((gsize *)(&ddb_cell_editable_text_view_type_id__volatile))) {
        static const GTypeInfo g_define_type_info = { sizeof (DdbCellEditableTextViewClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) ddb_cell_editable_text_view_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (DdbCellEditableTextView), 0, (GInstanceInitFunc) ddb_cell_editable_text_view_instance_init, NULL };
        static const GInterfaceInfo gtk_cell_editable_info = { (GInterfaceInitFunc) ddb_cell_editable_text_view_gtk_cell_editable_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
        GType ddb_cell_editable_text_view_type_id;
        ddb_cell_editable_text_view_type_id = g_type_register_static (GTK_TYPE_TEXT_VIEW, "DdbCellEditableTextView", &g_define_type_info, 0);
        g_type_add_interface_static (ddb_cell_editable_text_view_type_id, GTK_TYPE_CELL_EDITABLE, &gtk_cell_editable_info);
        g_once_init_leave (&ddb_cell_editable_text_view_type_id__volatile, ddb_cell_editable_text_view_type_id);
    }
    return ddb_cell_editable_text_view_type_id__volatile;
}


static gpointer _g_object_ref0 (gpointer self) {
    return self ? g_object_ref (self) : NULL;
}


static void ddb_cell_renderer_text_multiline_gtk_cell_renderer_text_editing_done (DdbCellEditableTextView* entry, DdbCellRendererTextMultiline* _self_) {
    GtkTextBuffer* buf;
    GtkTextIter begin = {0};
    GtkTextIter end = {0};
    gboolean canceled = FALSE;
    gchar* new_text;
    g_return_if_fail (entry != NULL);
    g_return_if_fail (_self_ != NULL);

    _g_object_unref0 (_self_->priv->entry);

    if (_self_->priv->focus_out_id > 0) {
        g_signal_handler_disconnect ((GObject*) entry, _self_->priv->focus_out_id);
        _self_->priv->focus_out_id = 0;
    }

    if (_self_->priv->populate_popup_id > 0)
    {
        g_signal_handler_disconnect (entry, _self_->priv->populate_popup_id);
        _self_->priv->populate_popup_id = 0;
    }

    if (_self_->priv->entry_menu_popdown_timeout)
    {
        g_source_remove (_self_->priv->entry_menu_popdown_timeout);
        _self_->priv->entry_menu_popdown_timeout = 0;
    }


    g_object_get (entry,
            "editing-canceled", &canceled,
            NULL);

    gtk_cell_renderer_stop_editing ((GtkCellRenderer*) _self_, entry->priv->editing_canceled);

    if (canceled) {
        return;
    }

    buf = gtk_text_view_get_buffer ((GtkTextView*) entry);
    gtk_text_buffer_get_iter_at_offset (buf, &begin, 0);
    gtk_text_buffer_get_iter_at_offset (buf, &end, -1);
    new_text = gtk_text_buffer_get_text (buf, &begin, &end, TRUE);
    g_signal_emit_by_name ((GtkCellRendererText*) _self_, "edited", entry->tree_path, new_text);
    _g_free0 (new_text);
}


static gboolean ddb_cell_renderer_text_multiline_gtk_cell_renderer_focus_out_event (DdbCellEditableTextView* entry, GdkEvent* event, DdbCellRendererTextMultiline* _self_) {
    gboolean result = FALSE;
    g_return_val_if_fail (entry != NULL, FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (_self_ != NULL, FALSE);

    DdbCellRendererTextMultilinePrivate *priv;

    priv = DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE (_self_);

    if (priv->in_entry_menu)
        return FALSE;

    gtk_cell_editable_editing_done (GTK_CELL_EDITABLE (entry));
    gtk_cell_editable_remove_widget (GTK_CELL_EDITABLE (entry));
    result = FALSE;
    return result;
}

static gboolean
popdown_timeout (gpointer data)
{
    DdbCellRendererTextMultilinePrivate *priv;

    priv = DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE (data);

    priv->entry_menu_popdown_timeout = 0;

    if (!gtk_widget_has_focus (GTK_WIDGET (priv->entry)))
        ddb_cell_renderer_text_multiline_gtk_cell_renderer_text_editing_done (priv->entry, data);

    return FALSE;
}

static void
ddb_cell_renderer_text_multiline_popup_unmap (GtkMenu *menu,
                                    gpointer data)
{
    DdbCellRendererTextMultilinePrivate *priv;

    priv = DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE (data);

    priv->in_entry_menu = FALSE;

    if (priv->entry_menu_popdown_timeout)
        return;

    priv->entry_menu_popdown_timeout = gdk_threads_add_timeout (500, popdown_timeout,
            data);
}


static void
ddb_cell_renderer_text_multiline_populate_popup (GtkEntry *entry,
                                       GtkMenu  *menu,
                                       gpointer  data)
{
    DdbCellRendererTextMultilinePrivate *priv;

    priv = DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE (data);

    if (priv->entry_menu_popdown_timeout)
    {
        g_source_remove (priv->entry_menu_popdown_timeout);
        priv->entry_menu_popdown_timeout = 0;
    }

    priv->in_entry_menu = TRUE;

    g_signal_connect (menu, "unmap",
            G_CALLBACK (ddb_cell_renderer_text_multiline_popup_unmap), data);
}

static GtkCellEditable* ddb_cell_renderer_text_multiline_real_start_editing (
                                                                             GtkCellRenderer      *cell,
                                                                             GdkEvent             *event,
                                                                             GtkWidget            *widget,
                                                                             const gchar          *path,
                                                                             const GdkRectangle   *background_area,
                                                                             const GdkRectangle   *cell_area,
                                                                             GtkCellRendererState  flags
                                                                             ) {
    DdbCellRendererTextMultiline * self;
    DdbCellEditableTextView* textview;
    GValue v = {0};
    GtkCellEditable* result = NULL;
    GtkListStore* store;
    GtkTextBuffer* buf;
    GtkTreeIter iter = {0};
    GtkTreePath* p;
    GtkTreeView* tv;
    gboolean is_editable = FALSE;
    gchar* renderer_text = NULL;
    gint mult;


    self = (DdbCellRendererTextMultiline*) cell;
    g_return_val_if_fail (widget != NULL, NULL);
    g_return_val_if_fail (path != NULL, NULL);
    g_return_val_if_fail (background_area != NULL, NULL);
    g_return_val_if_fail (cell_area != NULL, NULL);
    g_object_get ((GtkCellRendererText*) self, "editable", &is_editable, NULL);
    if (!is_editable) {
        result = GTK_CELL_EDITABLE (NULL);
        return result;
    }
    p = gtk_tree_path_new_from_string (path);
    tv = GTK_TREE_VIEW (widget);
    _g_object_ref0 (tv);
    store = GTK_LIST_STORE (gtk_tree_view_get_model (tv));
    _g_object_ref0 (store);
    gtk_tree_model_get_iter ((GtkTreeModel*) store, &iter, p);
    G_IS_VALUE (&v) ? ((void)(g_value_unset (&v)), NULL) : NULL;
    gtk_tree_model_get_value ((GtkTreeModel*) store, &iter, self->priv->is_mult_column, &v);
    mult = g_value_get_int (&v);
    _g_object_unref0 (self->priv->entry);
    self->priv->entry = textview = ddb_cell_editable_text_view_new ();
    g_object_ref_sink (textview);
    textview->tree_path = g_strdup (path);
    buf = gtk_text_buffer_new (NULL);
    if (!mult) {
        GValue full_textv = {0};
        gtk_tree_model_get_value ((GtkTreeModel*) store, &iter, self->priv->value_column, &full_textv);
        if (G_IS_VALUE (&full_textv)) {
            const char *full_text = g_value_get_string (&full_textv);
            renderer_text = strdup (full_text);
            g_value_unset (&full_textv);
        }
        if (!renderer_text) {
            g_object_get ((GtkCellRendererText*) self, "text", &renderer_text, NULL);
        }
    }
    else {
        renderer_text = strdup ("");
    }
    if (renderer_text) {
        gtk_text_buffer_set_text (buf, renderer_text, -1);
        _g_free0 (renderer_text);
    }
    gtk_text_view_set_buffer ((GtkTextView*) textview, buf);

    self->priv->in_entry_menu = FALSE;
    if (self->priv->entry_menu_popdown_timeout)
    {
        g_source_remove (self->priv->entry_menu_popdown_timeout);
        self->priv->entry_menu_popdown_timeout = 0;
    }

    g_signal_connect (textview, "editing-done", (GCallback) ddb_cell_renderer_text_multiline_gtk_cell_renderer_text_editing_done, self);
    self->priv->focus_out_id = g_signal_connect_after (textview, "focus-out-event", (GCallback) ddb_cell_renderer_text_multiline_gtk_cell_renderer_focus_out_event, self);

    self->priv->populate_popup_id =
        g_signal_connect (self->priv->entry, "populate-popup",
                G_CALLBACK (ddb_cell_renderer_text_multiline_populate_popup),
                self);

    _g_object_unref0 (buf);
    gtk_widget_set_size_request ((GtkWidget*) textview, cell_area->width, cell_area->height);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_WORD);
    gtk_widget_show ((GtkWidget*) textview);
    result = GTK_CELL_EDITABLE (textview);
    G_IS_VALUE (&v) ? ((void)(g_value_unset (&v)), NULL) : NULL;
    _g_object_unref0 (store);
    _g_object_unref0 (tv);
    _gtk_tree_path_free0 (p);
    // evil hack! need to make an event for that
    extern int trkproperties_block_keyhandler;
    trkproperties_block_keyhandler = 1;
    return result;
}


DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_construct (GType object_type) {
    DdbCellRendererTextMultiline * self;
    self = g_object_newv (object_type, 0, NULL);
    return self;
}


DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_new (void) {
    return ddb_cell_renderer_text_multiline_construct (DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE);
}


static void ddb_cell_renderer_text_multiline_class_init (DdbCellRendererTextMultilineClass * klass) {
    ddb_cell_renderer_text_multiline_parent_class = g_type_class_peek_parent (klass);
    g_type_class_add_private (klass, sizeof (DdbCellRendererTextMultilinePrivate));
#if GTK_CHECK_VERSION(3,0,0)
    GTK_CELL_RENDERER_CLASS (klass)->start_editing = ddb_cell_renderer_text_multiline_real_start_editing;
#else
    // GTK2 uses a slightly different, but compatible declaration without const modifiers
    GTK_CELL_RENDERER_CLASS (klass)->start_editing = (GtkCellEditable *(*) (GtkCellRenderer *, GdkEvent *, GtkWidget *, const gchar *, GdkRectangle *, GdkRectangle *cell_area, GtkCellRendererState))ddb_cell_renderer_text_multiline_real_start_editing;
#endif
    G_OBJECT_CLASS (klass)->finalize = ddb_cell_renderer_text_multiline_finalize;
}


static void ddb_cell_renderer_text_multiline_instance_init (DdbCellRendererTextMultiline * self) {
    self->priv = DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE (self);
    // Default column indexes for Track Properties
    self->priv->is_mult_column = 3;
    self->priv->value_column = 4;
}

void ddb_cell_renderer_text_multiline_set_columns(DdbCellRendererTextMultiline *self, guint is_mult, guint value) {
    self->priv->is_mult_column = is_mult;
    self->priv->value_column = value;
}

static void ddb_cell_renderer_text_multiline_finalize (GObject* obj) {
    DdbCellRendererTextMultiline * self;
    self = DDB_CELL_RENDERER_TEXT_MULTILINE (obj);
    _g_object_unref0 (self->priv->entry);
    G_OBJECT_CLASS (ddb_cell_renderer_text_multiline_parent_class)->finalize (obj);
}


GType ddb_cell_renderer_text_multiline_get_type (void) {
    static volatile gsize ddb_cell_renderer_text_multiline_type_id__volatile = 0;
    if (g_once_init_enter ((gsize *)(&ddb_cell_renderer_text_multiline_type_id__volatile))) {
        static const GTypeInfo g_define_type_info = { sizeof (DdbCellRendererTextMultilineClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) ddb_cell_renderer_text_multiline_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (DdbCellRendererTextMultiline), 0, (GInstanceInitFunc) ddb_cell_renderer_text_multiline_instance_init, NULL };
        GType ddb_cell_renderer_text_multiline_type_id;
        ddb_cell_renderer_text_multiline_type_id = g_type_register_static (GTK_TYPE_CELL_RENDERER_TEXT, "DdbCellRendererTextMultiline", &g_define_type_info, 0);
        g_once_init_leave (&ddb_cell_renderer_text_multiline_type_id__volatile, ddb_cell_renderer_text_multiline_type_id);
    }
    return ddb_cell_renderer_text_multiline_type_id__volatile;
}



