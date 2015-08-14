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
    gulong entry_menu_popdown_timeout;
    gboolean in_entry_menu;
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
#define DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_TYPE_CELL_RENDERER_TEXT_MULTILINE, DdbCellRendererTextMultilinePrivate))
#define DDB_CELL_EDITABLE_TEXT_VIEW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_TYPE_CELL_EDITABLE_TEXT_VIEW, DdbCellEditableTextViewPrivate))
enum  {
	DDB_CELL_RENDERER_TEXT_MULTILINE_DUMMY_PROPERTY
};
static void ddb_cell_renderer_text_multiline_gtk_cell_renderer_text_editing_done (DdbCellEditableTextView* entry, DdbCellRendererTextMultiline* _self_);
static gboolean ddb_cell_renderer_text_multiline_gtk_cell_renderer_focus_out_event (DdbCellEditableTextView* entry, GdkEvent* event, DdbCellRendererTextMultiline* _self_);
static GtkCellEditable* ddb_cell_renderer_text_multiline_real_start_editing (GtkCellRenderer* base, GdkEvent* event, GtkWidget* widget, const gchar* path, GdkRectangle* background_area, GdkRectangle* cell_area, GtkCellRendererState flags);
DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_new (void);
DdbCellRendererTextMultiline* ddb_cell_renderer_text_multiline_construct (GType object_type);
static void ddb_cell_renderer_text_multiline_finalize (GObject* obj);


static gboolean ddb_cell_editable_text_view_real_key_press_event (GtkWidget* base, GdkEventKey* event) {
	DdbCellEditableTextView * self;
	gboolean result = FALSE;
	gboolean res;
	GdkEventKey _tmp0_;
	guint _tmp1_;
	self = (DdbCellEditableTextView*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	res = TRUE;
	_tmp0_ = *event;
	_tmp1_ = _tmp0_.keyval;
	if (_tmp1_ == ((guint) GDK_Return)) {
		GdkEventKey _tmp2_;
		GdkModifierType _tmp3_;
		_tmp2_ = *event;
		_tmp3_ = _tmp2_.state;
		if ((_tmp3_ & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0) {
			GdkEventKey _tmp4_;
			gboolean _tmp5_ = FALSE;
			_tmp4_ = *event;
			_tmp5_ = GTK_WIDGET_CLASS (ddb_cell_editable_text_view_parent_class)->key_press_event ((GtkWidget*) GTK_TEXT_VIEW (self), &_tmp4_);
			res = _tmp5_;
		} else {
			gtk_cell_editable_editing_done ((GtkCellEditable*) self);
			gtk_cell_editable_remove_widget ((GtkCellEditable*) self);
			result = TRUE;
			return result;
		}
	} else {
		GdkEventKey _tmp6_;
		guint _tmp7_;
		_tmp6_ = *event;
		_tmp7_ = _tmp6_.keyval;
		if (_tmp7_ == ((guint) GDK_Escape)) {
			self->priv->editing_canceled = TRUE;
			gtk_cell_editable_editing_done ((GtkCellEditable*) self);
			gtk_cell_editable_remove_widget ((GtkCellEditable*) self);
			result = TRUE;
			return result;
		} else {
			GdkEventKey _tmp8_;
			gboolean _tmp9_ = FALSE;
			_tmp8_ = *event;
			_tmp9_ = GTK_WIDGET_CLASS (ddb_cell_editable_text_view_parent_class)->key_press_event ((GtkWidget*) GTK_TEXT_VIEW (self), &_tmp8_);
			res = _tmp9_;
		}
	}
	result = res;
	return result;
}

static void ddb_cell_editable_text_view_real_start_editing (GtkCellEditable* base, GdkEvent* event) {
	DdbCellEditableTextView * self;
	self = (DdbCellEditableTextView*) base;
}


#if GTK_CHECK_VERSION(2,20,0)
static void ddb_cell_editable_text_view_real_editing_canceled (GtkCellRenderer* base) {
}
#endif

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
	if (g_once_init_enter (&ddb_cell_editable_text_view_type_id__volatile)) {
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
	DdbCellEditableTextView* _tmp0_;
	DdbCellRendererTextMultiline* _tmp1_;
	gulong _tmp2_;
	DdbCellRendererTextMultiline* _tmp3_;
	DdbCellEditableTextView* _tmp4_;
	gboolean _tmp5_;
	DdbCellEditableTextView* _tmp6_;
	GtkTextBuffer* _tmp7_ = NULL;
	GtkTextBuffer* _tmp8_;
	GtkTextBuffer* buf;
	GtkTextIter begin = {0};
	GtkTextIter end = {0};
	GtkTextIter _tmp9_ = {0};
	GtkTextIter _tmp10_ = {0};
	GtkTextIter _tmp11_;
	GtkTextIter _tmp12_;
	gchar* _tmp13_ = NULL;
	gchar* new_text;
	DdbCellRendererTextMultiline* _tmp14_;
	DdbCellEditableTextView* _tmp15_;
	const gchar* _tmp16_;
	g_return_if_fail (entry != NULL);
	g_return_if_fail (_self_ != NULL);
	_tmp0_ = entry;
	_tmp1_ = _self_;
	_tmp2_ = _tmp1_->priv->focus_out_id;
	g_signal_handler_disconnect ((GObject*) _tmp0_, _tmp2_);

    if (_tmp1_->priv->populate_popup_id > 0)
    {
        g_signal_handler_disconnect (entry, _tmp1_->priv->populate_popup_id);
        _tmp1_->priv->populate_popup_id = 0;
    }

    if (_tmp1_->priv->entry_menu_popdown_timeout)
    {
        g_source_remove (_tmp1_->priv->entry_menu_popdown_timeout);
        _tmp1_->priv->entry_menu_popdown_timeout = 0;
    }



	_tmp3_ = _self_;
	_tmp4_ = entry;
	_tmp5_ = _tmp4_->priv->editing_canceled;
	gtk_cell_renderer_stop_editing ((GtkCellRenderer*) _tmp3_, _tmp5_);
	_tmp6_ = entry;
	_tmp7_ = gtk_text_view_get_buffer ((GtkTextView*) _tmp6_);
	_tmp8_ = _g_object_ref0 (_tmp7_);
	buf = _tmp8_;
	gtk_text_buffer_get_iter_at_offset (buf, &_tmp9_, 0);
	begin = _tmp9_;
	gtk_text_buffer_get_iter_at_offset (buf, &_tmp10_, -1);
	end = _tmp10_;
	_tmp11_ = begin;
	_tmp12_ = end;
	_tmp13_ = gtk_text_buffer_get_text (buf, &_tmp11_, &_tmp12_, TRUE);
	new_text = _tmp13_;
	_tmp14_ = _self_;
	_tmp15_ = entry;
	_tmp16_ = _tmp15_->tree_path;
	g_signal_emit_by_name ((GtkCellRendererText*) _tmp14_, "edited", _tmp16_, new_text);
	_g_free0 (new_text);
	_g_object_unref0 (buf);
	_g_free0 (new_text);
}


static gboolean ddb_cell_renderer_text_multiline_gtk_cell_renderer_focus_out_event (DdbCellEditableTextView* entry, GdkEvent* event, DdbCellRendererTextMultiline* _self_) {
	gboolean result = FALSE;
	g_return_val_if_fail (entry != NULL, FALSE);
	g_return_val_if_fail (event != NULL, FALSE);
	g_return_val_if_fail (_self_ != NULL, FALSE);

    DdbCellRendererTextMultilinePrivate *priv;

    priv = DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE (_self_);

	entry->priv->editing_canceled = TRUE;
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

static GtkCellEditable* ddb_cell_renderer_text_multiline_real_start_editing (GtkCellRenderer* base, GdkEvent* event, GtkWidget* widget, const gchar* path, GdkRectangle* background_area, GdkRectangle* cell_area, GtkCellRendererState flags) {
	DdbCellRendererTextMultiline * self;
	GtkCellEditable* result = NULL;
	gboolean _tmp0_ = FALSE;
	gboolean _tmp1_;
	const gchar* _tmp2_;
	GtkTreePath* _tmp3_;
	GtkTreePath* p;
	GtkWidget* _tmp4_;
	GtkTreeView* _tmp5_;
	GtkTreeView* tv;
	GtkTreeView* _tmp6_;
	GtkTreeModel* _tmp7_ = NULL;
	GtkListStore* _tmp8_;
	GtkListStore* store;
	GtkTreeIter iter = {0};
	GtkListStore* _tmp9_;
	GtkTreePath* _tmp10_;
	GtkTreeIter _tmp11_ = {0};
	GValue v = {0};
	GtkListStore* _tmp12_;
	GtkTreeIter _tmp13_;
	GValue _tmp14_ = {0};
	gint _tmp15_ = 0;
	gint mult;
	DdbCellEditableTextView* _tmp16_;
	DdbCellEditableTextView* _tmp17_;
	gint _tmp18_;
	DdbCellEditableTextView* _tmp19_;
	const gchar* _tmp20_;
	gchar* _tmp21_;
	GtkTextBuffer* _tmp22_;
	GtkTextBuffer* buf;
	gchar* _tmp23_ = NULL;
	gchar* _tmp24_;
	gchar* _tmp25_;
	gboolean _tmp26_;
	DdbCellEditableTextView* _tmp31_;
	GtkTextBuffer* _tmp32_;
	DdbCellEditableTextView* _tmp33_;
	DdbCellEditableTextView* _tmp34_;
	gulong _tmp35_ = 0UL;
	DdbCellEditableTextView* _tmp36_;
	GdkRectangle _tmp37_;
	gint _tmp38_;
	GdkRectangle _tmp39_;
	gint _tmp40_;
	DdbCellEditableTextView* _tmp41_;
	DdbCellEditableTextView* _tmp42_;
	self = (DdbCellRendererTextMultiline*) base;
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (background_area != NULL, NULL);
	g_return_val_if_fail (cell_area != NULL, NULL);
	g_object_get ((GtkCellRendererText*) self, "editable", &_tmp0_, NULL);
	_tmp1_ = _tmp0_;
	if (!_tmp1_) {
		result = GTK_CELL_EDITABLE (NULL);
		return result;
	}
	_tmp2_ = path;
	_tmp3_ = gtk_tree_path_new_from_string (_tmp2_);
	p = _tmp3_;
	_tmp4_ = widget;
	_tmp5_ = _g_object_ref0 (GTK_TREE_VIEW (_tmp4_));
	tv = _tmp5_;
	_tmp6_ = tv;
	_tmp7_ = gtk_tree_view_get_model (_tmp6_);
	_tmp8_ = _g_object_ref0 (GTK_LIST_STORE (_tmp7_));
	store = _tmp8_;
	_tmp9_ = store;
	_tmp10_ = p;
	gtk_tree_model_get_iter ((GtkTreeModel*) _tmp9_, &_tmp11_, _tmp10_);
	iter = _tmp11_;
	_tmp12_ = store;
	_tmp13_ = iter;
	gtk_tree_model_get_value ((GtkTreeModel*) _tmp12_, &_tmp13_, 3, &_tmp14_);
	G_IS_VALUE (&v) ? (g_value_unset (&v), NULL) : NULL;
	v = _tmp14_;
	_tmp15_ = g_value_get_int (&v);
	mult = _tmp15_;
	_tmp16_ = ddb_cell_editable_text_view_new ();
	_tmp17_ = g_object_ref_sink (_tmp16_);
	_g_object_unref0 (self->priv->entry);
	self->priv->entry = _tmp17_;
	_tmp18_ = mult;
	if (_tmp18_ != 0) {
		g_object_set ((GtkCellRendererText*) self, "text", "", NULL);
	}
	_tmp19_ = self->priv->entry;
	_tmp20_ = path;
	_tmp21_ = g_strdup (_tmp20_);
	_g_free0 (_tmp19_->tree_path);
	_tmp19_->tree_path = _tmp21_;
	_tmp22_ = gtk_text_buffer_new (NULL);
	buf = _tmp22_;
	g_object_get ((GtkCellRendererText*) self, "text", &_tmp23_, NULL);
	_tmp24_ = _tmp23_;
	_tmp25_ = _tmp24_;
	_tmp26_ = _tmp25_ != NULL;
	_g_free0 (_tmp25_);
	if (_tmp26_) {
		GtkTextBuffer* _tmp27_;
		gchar* _tmp28_ = NULL;
		gchar* _tmp29_;
		gchar* _tmp30_;
		_tmp27_ = buf;
		g_object_get ((GtkCellRendererText*) self, "text", &_tmp28_, NULL);
		_tmp29_ = _tmp28_;
		_tmp30_ = _tmp29_;
		gtk_text_buffer_set_text (_tmp27_, _tmp30_, -1);
		_g_free0 (_tmp30_);
	}
	_tmp31_ = self->priv->entry;
	_tmp32_ = buf;
	gtk_text_view_set_buffer ((GtkTextView*) _tmp31_, _tmp32_);
	_tmp33_ = self->priv->entry;

    self->priv->in_entry_menu = FALSE;
    if (self->priv->entry_menu_popdown_timeout)
    {
        g_source_remove (self->priv->entry_menu_popdown_timeout);
        self->priv->entry_menu_popdown_timeout = 0;
    }

	g_signal_connect (_tmp33_, "editing-done", (GCallback) ddb_cell_renderer_text_multiline_gtk_cell_renderer_text_editing_done, self);
	_tmp34_ = self->priv->entry;
	_tmp35_ = g_signal_connect_after (_tmp34_, "focus-out-event", (GCallback) ddb_cell_renderer_text_multiline_gtk_cell_renderer_focus_out_event, self);

    self->priv->populate_popup_id =
        g_signal_connect (self->priv->entry, "populate-popup",
                G_CALLBACK (ddb_cell_renderer_text_multiline_populate_popup),
                self);

	self->priv->focus_out_id = _tmp35_;
	_tmp36_ = self->priv->entry;
	_tmp37_ = *cell_area;
	_tmp38_ = _tmp37_.width;
	_tmp39_ = *cell_area;
	_tmp40_ = _tmp39_.height;
	gtk_widget_set_size_request ((GtkWidget*) _tmp36_, _tmp38_, _tmp40_);
	_tmp41_ = self->priv->entry;
	gtk_widget_show ((GtkWidget*) _tmp41_);
	_tmp42_ = self->priv->entry;
	result = GTK_CELL_EDITABLE (_tmp42_);
	_g_object_unref0 (buf);
	G_IS_VALUE (&v) ? (g_value_unset (&v), NULL) : NULL;
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
	GTK_CELL_RENDERER_CLASS (klass)->start_editing = ddb_cell_renderer_text_multiline_real_start_editing;
	G_OBJECT_CLASS (klass)->finalize = ddb_cell_renderer_text_multiline_finalize;
}


static void ddb_cell_renderer_text_multiline_instance_init (DdbCellRendererTextMultiline * self) {
	self->priv = DDB_CELL_RENDERER_TEXT_MULTILINE_GET_PRIVATE (self);
}


static void ddb_cell_renderer_text_multiline_finalize (GObject* obj) {
	DdbCellRendererTextMultiline * self;
	self = DDB_CELL_RENDERER_TEXT_MULTILINE (obj);
	_g_object_unref0 (self->priv->entry);
	G_OBJECT_CLASS (ddb_cell_renderer_text_multiline_parent_class)->finalize (obj);
}


GType ddb_cell_renderer_text_multiline_get_type (void) {
	static volatile gsize ddb_cell_renderer_text_multiline_type_id__volatile = 0;
	if (g_once_init_enter (&ddb_cell_renderer_text_multiline_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (DdbCellRendererTextMultilineClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) ddb_cell_renderer_text_multiline_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (DdbCellRendererTextMultiline), 0, (GInstanceInitFunc) ddb_cell_renderer_text_multiline_instance_init, NULL };
		GType ddb_cell_renderer_text_multiline_type_id;
		ddb_cell_renderer_text_multiline_type_id = g_type_register_static (GTK_TYPE_CELL_RENDERER_TEXT, "DdbCellRendererTextMultiline", &g_define_type_info, 0);
		g_once_init_leave (&ddb_cell_renderer_text_multiline_type_id__volatile, ddb_cell_renderer_text_multiline_type_id);
	}
	return ddb_cell_renderer_text_multiline_type_id__volatile;
}



