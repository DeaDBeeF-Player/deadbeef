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
#include <gdk/gdk.h>
#include <drawing.h>
#include <gtkui.h>
#include <math.h>
#include "support.h"
#include "ddbseekbar.h"

#define DDB_TYPE_SEEKBAR (ddb_seekbar_get_type ())
#define DDB_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_SEEKBAR, DdbSeekbar))
#define DDB_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_SEEKBAR, DdbSeekbarClass))
#define DDB_IS_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_SEEKBAR))
#define DDB_IS_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_SEEKBAR))
#define DDB_SEEKBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_SEEKBAR, DdbSeekbarClass))

static gpointer ddb_seekbar_parent_class = NULL;

GType ddb_seekbar_get_type (void);
enum  {
	DDB_SEEKBAR_DUMMY_PROPERTY
};
#if GTK_CHECK_VERSION(3,0,0)
static void ddb_seekbar_get_preferred_width (GtkWidget* base, gint *minimal_width, gint *natural_width);
static void ddb_seekbar_get_preferred_height (GtkWidget* base, gint *minimal_height, gint *natural_height);
#else
static gboolean ddb_seekbar_real_expose_event (GtkWidget* base, GdkEventExpose* event);
#endif
static void ddb_seekbar_real_size_request (GtkWidget* base, GtkRequisition* requisition);
static gboolean ddb_seekbar_real_draw (GtkWidget* base, cairo_t *cr);
static gboolean ddb_seekbar_real_button_press_event (GtkWidget* base, GdkEventButton* event);
static gboolean ddb_seekbar_real_button_release_event (GtkWidget* base, GdkEventButton* event);
static gboolean ddb_seekbar_real_motion_notify_event (GtkWidget* base, GdkEventMotion* event);
static gboolean ddb_seekbar_real_configure_event (GtkWidget* base, GdkEventConfigure* event);
DdbSeekbar* ddb_seekbar_construct (GType object_type);
static GObject * ddb_seekbar_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);


#if GTK_CHECK_VERSION(3,0,0)
static void ddb_seekbar_get_preferred_width (GtkWidget* widget, gint *minimal_width, gint *natural_width) {
    GtkRequisition requisition;

    ddb_seekbar_real_size_request (widget, &requisition);

    *minimal_width = *natural_width = requisition.width;
}

static void ddb_seekbar_get_preferred_height (GtkWidget* widget, gint *minimal_height, gint *natural_height) {
    GtkRequisition requisition;

    ddb_seekbar_real_size_request (widget, &requisition);

    *minimal_height = *natural_height = requisition.height;
}
#endif

static void ddb_seekbar_real_size_request (GtkWidget* base, GtkRequisition* requisition) {
	DdbSeekbar * self;
	GtkRequisition _vala_requisition = {0};
	self = (DdbSeekbar*) base;
	if (requisition) {
		*requisition = _vala_requisition;
	}
}

static gboolean ddb_seekbar_real_draw (GtkWidget* base, cairo_t *cr) {
	seekbar_draw (base, cr);
	return FALSE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static gboolean ddb_seekbar_real_expose_event (GtkWidget* base, GdkEventExpose* event) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (base));
    ddb_seekbar_real_draw (base, cr);
    cairo_destroy (cr);
	return FALSE;
}
#endif

static gboolean ddb_seekbar_real_button_press_event (GtkWidget* base, GdkEventButton* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	GdkEventButton _tmp0_;
	gboolean _tmp1_ = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	_tmp0_ = *event;
	_tmp1_ = on_seekbar_button_press_event ((GtkWidget*) self, &_tmp0_);
	result = _tmp1_;
	return result;
}


static gboolean ddb_seekbar_real_button_release_event (GtkWidget* base, GdkEventButton* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	GdkEventButton _tmp0_;
	gboolean _tmp1_ = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	_tmp0_ = *event;
	_tmp1_ = on_seekbar_button_release_event ((GtkWidget*) self, &_tmp0_);
	result = _tmp1_;
	return result;
}


static gboolean ddb_seekbar_real_motion_notify_event (GtkWidget* base, GdkEventMotion* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	GdkEventMotion _tmp0_;
	gboolean _tmp1_ = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	_tmp0_ = *event;
	_tmp1_ = on_seekbar_motion_notify_event ((GtkWidget*) self, &_tmp0_);
	result = _tmp1_;
	return result;
}


static gboolean ddb_seekbar_real_configure_event (GtkWidget* base, GdkEventConfigure* event) {
	DdbSeekbar * self;
	gboolean result = FALSE;
	self = (DdbSeekbar*) base;
	g_return_val_if_fail (event != NULL, FALSE);
	gtkui_init_theme_colors ();
	result = FALSE;
	return result;
}


DdbSeekbar* ddb_seekbar_construct (GType object_type) {
	DdbSeekbar * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GtkWidget* ddb_seekbar_new (void) {
	return GTK_WIDGET (ddb_seekbar_construct (DDB_TYPE_SEEKBAR));
}


static GObject * ddb_seekbar_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	DdbSeekbar * self;
	parent_class = G_OBJECT_CLASS (ddb_seekbar_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = DDB_SEEKBAR (obj);
	return obj;
}


static void ddb_seekbar_class_init (DdbSeekbarClass * klass) {
	ddb_seekbar_parent_class = g_type_class_peek_parent (klass);
#if GTK_CHECK_VERSION(3,0,0)
	GTK_WIDGET_CLASS (klass)->get_preferred_width = ddb_seekbar_get_preferred_width;
	GTK_WIDGET_CLASS (klass)->get_preferred_height = ddb_seekbar_get_preferred_height;
	GTK_WIDGET_CLASS (klass)->draw = ddb_seekbar_real_draw;
#else
	GTK_WIDGET_CLASS (klass)->size_request = ddb_seekbar_real_size_request;
	GTK_WIDGET_CLASS (klass)->expose_event = ddb_seekbar_real_expose_event;
#endif
	GTK_WIDGET_CLASS (klass)->button_press_event = ddb_seekbar_real_button_press_event;
	GTK_WIDGET_CLASS (klass)->button_release_event = ddb_seekbar_real_button_release_event;
	GTK_WIDGET_CLASS (klass)->motion_notify_event = ddb_seekbar_real_motion_notify_event;
	GTK_WIDGET_CLASS (klass)->configure_event = ddb_seekbar_real_configure_event;
	G_OBJECT_CLASS (klass)->constructor = ddb_seekbar_constructor;
}


static void ddb_seekbar_instance_init (DdbSeekbar * self) {
	gtk_widget_set_has_window ((GtkWidget*) self, FALSE);
	gtk_widget_set_has_tooltip ((GtkWidget*) self, TRUE);
	self->seekbar_moving = 0;
    self->seekbar_move_x = 0;
}


GType ddb_seekbar_get_type (void) {
	static volatile gsize ddb_seekbar_type_id__volatile = 0;
	if (g_once_init_enter (&ddb_seekbar_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (DdbSeekbarClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) ddb_seekbar_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (DdbSeekbar), 0, (GInstanceInitFunc) ddb_seekbar_instance_init, NULL };
		GType ddb_seekbar_type_id;
		ddb_seekbar_type_id = g_type_register_static (GTK_TYPE_WIDGET, "DdbSeekbar", &g_define_type_info, 0);
		g_once_init_leave (&ddb_seekbar_type_id__volatile, ddb_seekbar_type_id);
	}
	return ddb_seekbar_type_id__volatile;
}

enum
{
	CORNER_NONE        = 0,
	CORNER_TOPLEFT     = 1,
	CORNER_TOPRIGHT    = 2,
	CORNER_BOTTOMLEFT  = 4,
	CORNER_BOTTOMRIGHT = 8,
	CORNER_ALL         = 15
};

static void
clearlooks_rounded_rectangle (cairo_t * cr,
			      double x, double y, double w, double h,
			      double radius, uint8_t corners)
{
    if (radius < 0.01 || (corners == CORNER_NONE)) {
        cairo_rectangle (cr, x, y, w, h);
        return;
    }
	
    if (corners & CORNER_TOPLEFT)
        cairo_move_to (cr, x + radius, y);
    else
        cairo_move_to (cr, x, y);

    if (corners & CORNER_TOPRIGHT)
        cairo_arc (cr, x + w - radius, y + radius, radius, M_PI * 1.5, M_PI * 2);
    else
        cairo_line_to (cr, x + w, y);

    if (corners & CORNER_BOTTOMRIGHT)
        cairo_arc (cr, x + w - radius, y + h - radius, radius, 0, M_PI * 0.5);
    else
        cairo_line_to (cr, x + w, y + h);

    if (corners & CORNER_BOTTOMLEFT)
        cairo_arc (cr, x + radius, y + h - radius, radius, M_PI * 0.5, M_PI);
    else
        cairo_line_to (cr, x, y + h);

    if (corners & CORNER_TOPLEFT)
        cairo_arc (cr, x + radius, y + radius, radius, M_PI, M_PI * 1.5);
    else
        cairo_line_to (cr, x, y);
	
}

void
seekbar_draw (GtkWidget *widget, cairo_t *cr) {
    if (!widget) {
        return;
    }

    DdbSeekbar *self = DDB_SEEKBAR (widget);

#if GTK_CHECK_VERSION(3,0,0)
    GtkAllocation allocation;
    gtk_widget_get_allocation (widget, &allocation);
    cairo_translate (cr, -allocation.x, -allocation.y);
#endif

    GdkColor clr_selection, clr_back;
    gtkui_get_bar_foreground_color (&clr_selection);
    gtkui_get_bar_background_color (&clr_back);

    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    int ax = a.x;
    int ay = a.y;
    int aw = a.width;
    int ah = a.height;

    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (!trk || deadbeef->pl_get_item_duration (trk) < 0) {
        if (trk) {
            deadbeef->pl_item_unref (trk);
        }
        // empty seekbar, just a frame
        clearlooks_rounded_rectangle (cr, 2+ax, a.height/2-4+ay, aw-4, 8, 4, 0xff);
        cairo_set_source_rgb (cr, clr_selection.red/65535.f, clr_selection.green/65535.f, clr_selection.blue/65535.f );
        cairo_set_line_width (cr, 2);
        cairo_stroke (cr);
        return;
    }
    float pos = 0;
    if (self->seekbar_moving) {
        int x = self->seekbar_move_x;
        if (x < 0) {
            x = 0;
        }
        if (x > a.width-1) {
            x = a.width-1;
        }
        pos = x;
    }
    else {
        if (deadbeef->pl_get_item_duration (trk) > 0) {
            pos = deadbeef->streamer_get_playpos () / deadbeef->pl_get_item_duration (trk);
            pos *= a.width;
        }
    }
    // left
    if (pos > 0) {
        cairo_set_source_rgb (cr, clr_selection.red/65535.f, clr_selection.green/65535.f, clr_selection.blue/65535.f );
        cairo_rectangle (cr, ax, ah/2-4+ay, pos, 8);
        cairo_clip (cr);
        clearlooks_rounded_rectangle (cr, 0+ax, ah/2-4 + ay, aw, 8, 4, 0xff);
        cairo_fill (cr);
        cairo_reset_clip (cr);
    }

    // right
    cairo_set_source_rgb (cr, clr_back.red/65535.f, clr_back.green/65535.f, clr_back.blue/65535.f );
    cairo_rectangle (cr, pos+ax, ah/2-4+ay, aw-pos, 8);
    cairo_clip (cr);
    clearlooks_rounded_rectangle (cr, 0+ax, ah/2-4+ay, aw, 8, 4, 0xff);
    cairo_fill (cr);
    cairo_reset_clip (cr);

    if (!gtkui_disable_seekbar_overlay && (self->seekbar_moving || self->seekbar_moved > 0.0) && trk) {
        float time = 0;
        float dur = deadbeef->pl_get_item_duration (trk);

        if (self->seekbar_moved > 0) {
            time = deadbeef->streamer_get_playpos ();
        }
        else {
            time = self->seekbar_move_x * dur / (a.width);
        }

        if (time < 0) {
            time = 0;
        }
        if (time > dur) {
            time = dur;
        }
        char s[1000];
        int hr = time/3600;
        int mn = (time-hr*3600)/60;
        int sc = time-hr*3600-mn*60;
        snprintf (s, sizeof (s), "%02d:%02d:%02d", hr, mn, sc);

        cairo_set_source_rgba (cr, clr_selection.red/65535.f, clr_selection.green/65535.f, clr_selection.blue/65535.f, self->seektime_alpha);
        cairo_save (cr);
        cairo_set_font_size (cr, 20);

        cairo_text_extents_t ex;
        cairo_text_extents (cr, s, &ex);
        if (self->textpos == -1) {
            self->textpos = ax + aw/2 - ex.width/2;
            self->textwidth = ex.width + 20;
        }

        clearlooks_rounded_rectangle (cr, ax + aw/2 - self->textwidth/2, ay+4, self->textwidth, ah-8, 3, 0xff);
        cairo_fill (cr);

        cairo_move_to (cr, self->textpos, ay+ah/2+ex.height/2);
        GdkColor clr;
        gtkui_get_listview_selected_text_color (&clr);
        cairo_set_source_rgba (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f, self->seektime_alpha);
        cairo_show_text (cr, s);
        cairo_restore (cr);

        int fps = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
        if (fps < 1) {
            fps = 1;
        }
        else if (fps > 30) {
            fps = 30;
        }
        if (self->seekbar_moved >= 0.0) {
            self->seekbar_moved -= 1.0/fps;
        }
        else {
            self->seekbar_moved = 0.0;
        }
    }

    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
}

gboolean
on_seekbar_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event)
{
    DdbSeekbar *self = DDB_SEEKBAR (widget);
    if (self->seekbar_moving) {
        GtkAllocation a;
        gtk_widget_get_allocation (widget, &a);
        self->seekbar_move_x = event->x - a.x;
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}

gboolean
on_seekbar_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    DdbSeekbar *self = DDB_SEEKBAR (widget);
    if (deadbeef->get_output ()->state () == OUTPUT_STATE_STOPPED) {
        return FALSE;
    }
    self->seekbar_moving = 1;
    self->seekbar_moved = 0;
    self->textpos = -1;
    self->textwidth = -1;
    self->seektime_alpha = 0.8;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    self->seekbar_move_x = event->x - a.x;
    gtk_widget_queue_draw (widget);
    return FALSE;
}


gboolean
on_seekbar_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    DdbSeekbar *self = DDB_SEEKBAR (widget);
    self->seekbar_moving = 0;
    self->seekbar_moved = 1.0;
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        if (deadbeef->pl_get_item_duration (trk) >= 0) {
            GtkAllocation a;
            gtk_widget_get_allocation (widget, &a);
            float time = (event->x - a.x) * deadbeef->pl_get_item_duration (trk) / (a.width);
            if (time < 0) {
                time = 0;
            }
            deadbeef->sendmessage (DB_EV_SEEK, 0, time * 1000, 0);
        }
        deadbeef->pl_item_unref (trk);
    }
    gtk_widget_queue_draw (widget);
    return FALSE;
}

static gboolean
on_evbox_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

static gboolean
on_evbox_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

static gboolean
on_evbox_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

static gboolean
on_evbox_scroll_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data) {
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

void
ddb_seekbar_init_signals (DdbSeekbar *sb, GtkWidget *evbox) {
  g_signal_connect ((gpointer) evbox, "button_press_event",
                    G_CALLBACK (on_evbox_button_press_event),
                    sb);
  g_signal_connect ((gpointer) evbox, "button_release_event",
                    G_CALLBACK (on_evbox_button_release_event),
                    sb);
  g_signal_connect ((gpointer) evbox, "scroll_event",
                    G_CALLBACK (on_evbox_scroll_event),
                    sb);
  g_signal_connect ((gpointer) evbox, "motion_notify_event",
                    G_CALLBACK (on_evbox_motion_notify_event),
                    sb);
}

