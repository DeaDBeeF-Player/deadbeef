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
#include <gdk/gdk.h>
#include <drawing.h>
#include <gtkui.h>
#include <math.h>
#include <string.h>
#include "support.h"
#include "ddbseekbar.h"

#define DDB_TYPE_SEEKBAR (ddb_seekbar_get_type ())
#define DDB_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_SEEKBAR, DdbSeekbar))
#define DDB_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_SEEKBAR, DdbSeekbarClass))
#define DDB_IS_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_SEEKBAR))
#define DDB_IS_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_SEEKBAR))
#define DDB_SEEKBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_SEEKBAR, DdbSeekbarClass))
#define DDB_SEEKBAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_TYPE_SEEKBAR, DdbSeekbarPrivate))

struct _DdbSeekbarPrivate {
    int seekbar_moving;
    float seekbar_moved;
    float seektime_alpha;
    int seekbar_move_x;
    int textpos;
    int textwidth;
    drawctx_t drawctx;
};

static gpointer ddb_seekbar_parent_class = NULL;

GType
ddb_seekbar_get_type (void);
enum { DDB_SEEKBAR_DUMMY_PROPERTY };
#if GTK_CHECK_VERSION(3, 0, 0)
static void
ddb_seekbar_get_preferred_width (GtkWidget *base, gint *minimal_width, gint *natural_width);
static void
ddb_seekbar_get_preferred_height (GtkWidget *base, gint *minimal_height, gint *natural_height);
#else
static gboolean
ddb_seekbar_real_expose_event (GtkWidget *base, GdkEventExpose *event);
#endif
static void
ddb_seekbar_real_size_request (GtkWidget *base, GtkRequisition *requisition);
static gboolean
ddb_seekbar_real_draw (GtkWidget *base, cairo_t *cr);
static gboolean
ddb_seekbar_real_button_press_event (GtkWidget *base, GdkEventButton *event);
static gboolean
ddb_seekbar_real_button_release_event (GtkWidget *base, GdkEventButton *event);
static gboolean
ddb_seekbar_real_motion_notify_event (GtkWidget *base, GdkEventMotion *event);
static gboolean
ddb_seekbar_real_configure_event (GtkWidget *base, GdkEventConfigure *event);
DdbSeekbar *
ddb_seekbar_construct (GType object_type);
static GObject *
ddb_seekbar_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties);

#if GTK_CHECK_VERSION(3, 0, 0)
static void
ddb_seekbar_get_preferred_width (GtkWidget *widget, gint *minimal_width, gint *natural_width) {
    GtkRequisition requisition;

    ddb_seekbar_real_size_request (widget, &requisition);

    *minimal_width = *natural_width = requisition.width;
}

static void
ddb_seekbar_get_preferred_height (GtkWidget *widget, gint *minimal_height, gint *natural_height) {
    GtkRequisition requisition;

    ddb_seekbar_real_size_request (widget, &requisition);

    *minimal_height = *natural_height = requisition.height;
}
#endif

static void
ddb_seekbar_real_size_request (GtkWidget *base, GtkRequisition *requisition) {
    GtkRequisition _vala_requisition = { 0 };
    if (requisition) {
        *requisition = _vala_requisition;
    }
}

static gboolean
ddb_seekbar_real_draw (GtkWidget *base, cairo_t *cr) {
    seekbar_draw (base, cr);
    return FALSE;
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static gboolean
ddb_seekbar_real_expose_event (GtkWidget *base, GdkEventExpose *event) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (base));
    ddb_seekbar_real_draw (base, cr);
    cairo_destroy (cr);
    return FALSE;
}
#endif

static gboolean
ddb_seekbar_real_button_press_event (GtkWidget *base, GdkEventButton *event) {
    DdbSeekbar *self;
    gboolean result = FALSE;
    GdkEventButton _tmp0_;
    gboolean _tmp1_ = FALSE;
    self = (DdbSeekbar *)base;
    g_return_val_if_fail (event != NULL, FALSE);
    _tmp0_ = *event;
    _tmp1_ = on_seekbar_button_press_event ((GtkWidget *)self, &_tmp0_);
    result = _tmp1_;
    return result;
}

static gboolean
ddb_seekbar_real_button_release_event (GtkWidget *base, GdkEventButton *event) {
    DdbSeekbar *self;
    gboolean result = FALSE;
    GdkEventButton _tmp0_;
    gboolean _tmp1_ = FALSE;
    self = (DdbSeekbar *)base;
    g_return_val_if_fail (event != NULL, FALSE);
    _tmp0_ = *event;
    _tmp1_ = on_seekbar_button_release_event ((GtkWidget *)self, &_tmp0_);
    result = _tmp1_;
    return result;
}

static gboolean
ddb_seekbar_real_motion_notify_event (GtkWidget *base, GdkEventMotion *event) {
    DdbSeekbar *self;
    gboolean result = FALSE;
    GdkEventMotion _tmp0_;
    gboolean _tmp1_ = FALSE;
    self = (DdbSeekbar *)base;
    g_return_val_if_fail (event != NULL, FALSE);
    _tmp0_ = *event;
    _tmp1_ = on_seekbar_motion_notify_event ((GtkWidget *)self, &_tmp0_);
    result = _tmp1_;
    return result;
}

static gboolean
ddb_seekbar_real_configure_event (GtkWidget *base, GdkEventConfigure *event) {
    gboolean result = FALSE;
    g_return_val_if_fail (event != NULL, FALSE);
    gtkui_init_theme_colors ();
    result = FALSE;
    return result;
}

static int
seek_sec (float sec) {
    float pos = deadbeef->streamer_get_playpos ();
    pos += sec;
    deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(pos * 1000), 0);
    return 0;
}

static gboolean
ddb_seekbar_scroll_event (GtkWidget *widget, GdkEventScroll *event) {

    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
        seek_sec (5.0f);
    }
    else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
        seek_sec (-5.0f);
    }

    return FALSE;
}

DdbSeekbar *
ddb_seekbar_construct (GType object_type) {
    DdbSeekbar *self;
    self = g_object_newv (object_type, 0, NULL);
    return self;
}

GtkWidget *
ddb_seekbar_new (void) {
    return GTK_WIDGET (ddb_seekbar_construct (DDB_TYPE_SEEKBAR));
}

static GObject *
ddb_seekbar_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties) {
    GObject *obj;
    GObjectClass *parent_class;
    parent_class = G_OBJECT_CLASS (ddb_seekbar_parent_class);
    obj = parent_class->constructor (type, n_construct_properties, construct_properties);
    return obj;
}

static void
ddb_seekbar_realize (GtkWidget *w) {
    GTK_WIDGET_CLASS (ddb_seekbar_parent_class)->realize (w);
    DdbSeekbar *self = DDB_SEEKBAR (w);
    DdbSeekbarPrivate *priv = DDB_SEEKBAR_GET_PRIVATE (self);
    drawctx_init (&priv->drawctx);
    draw_init_font (&priv->drawctx, DDB_SEEKBAR_FONT, 0);
}

static void
ddb_seekbar_unrealize (GtkWidget *w) {
    GTK_WIDGET_CLASS (ddb_seekbar_parent_class)->unrealize (w);
    DdbSeekbar *self = DDB_SEEKBAR (w);
    DdbSeekbarPrivate *priv = DDB_SEEKBAR_GET_PRIVATE (self);
    draw_free (&priv->drawctx);
}

static void
ddb_seekbar_class_init (DdbSeekbarClass *klass) {
    ddb_seekbar_parent_class = g_type_class_peek_parent (klass);
#if GTK_CHECK_VERSION(3, 0, 0)
    GTK_WIDGET_CLASS (klass)->get_preferred_width = ddb_seekbar_get_preferred_width;
    GTK_WIDGET_CLASS (klass)->get_preferred_height = ddb_seekbar_get_preferred_height;
    GTK_WIDGET_CLASS (klass)->draw = ddb_seekbar_real_draw;
#else
    GTK_WIDGET_CLASS (klass)->size_request = ddb_seekbar_real_size_request;
    GTK_WIDGET_CLASS (klass)->expose_event = ddb_seekbar_real_expose_event;
#endif
    GTK_WIDGET_CLASS (klass)->realize = ddb_seekbar_realize;
    GTK_WIDGET_CLASS (klass)->unrealize = ddb_seekbar_unrealize;
    GTK_WIDGET_CLASS (klass)->button_press_event = ddb_seekbar_real_button_press_event;
    GTK_WIDGET_CLASS (klass)->button_release_event = ddb_seekbar_real_button_release_event;
    GTK_WIDGET_CLASS (klass)->motion_notify_event = ddb_seekbar_real_motion_notify_event;
    GTK_WIDGET_CLASS (klass)->configure_event = ddb_seekbar_real_configure_event;
    GTK_WIDGET_CLASS (klass)->scroll_event = ddb_seekbar_scroll_event;
    G_OBJECT_CLASS (klass)->constructor = ddb_seekbar_constructor;
    g_type_class_add_private (klass, sizeof (DdbSeekbarPrivate));
}

static void
ddb_seekbar_instance_init (DdbSeekbar *self) {
    DdbSeekbarPrivate *priv = DDB_SEEKBAR_GET_PRIVATE (self);
    gtk_widget_set_has_window ((GtkWidget *)self, FALSE);
    gtk_widget_set_has_tooltip ((GtkWidget *)self, TRUE);
    priv->seekbar_moving = 0;
    priv->seekbar_move_x = 0;
}

GType
ddb_seekbar_get_type (void) {
    static volatile gsize ddb_seekbar_type_id__volatile = 0;
    if (g_once_init_enter ((gsize *)(&ddb_seekbar_type_id__volatile))) {
        static const GTypeInfo g_define_type_info = { sizeof (DdbSeekbarClass),
                                                      (GBaseInitFunc)NULL,
                                                      (GBaseFinalizeFunc)NULL,
                                                      (GClassInitFunc)ddb_seekbar_class_init,
                                                      (GClassFinalizeFunc)NULL,
                                                      NULL,
                                                      sizeof (DdbSeekbar),
                                                      0,
                                                      (GInstanceInitFunc)ddb_seekbar_instance_init,
                                                      NULL };
        GType ddb_seekbar_type_id;
        ddb_seekbar_type_id = g_type_register_static (GTK_TYPE_WIDGET, "DdbSeekbar", &g_define_type_info, 0);
        g_once_init_leave (&ddb_seekbar_type_id__volatile, ddb_seekbar_type_id);
    }
    return ddb_seekbar_type_id__volatile;
}

enum {
    CORNER_NONE = 0,
    CORNER_TOPLEFT = 1,
    CORNER_TOPRIGHT = 2,
    CORNER_BOTTOMLEFT = 4,
    CORNER_BOTTOMRIGHT = 8,
    CORNER_ALL = 15
};

static void
clearlooks_rounded_rectangle (cairo_t *cr, double x, double y, double w, double h, double radius, uint8_t corners) {
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
    DdbSeekbarPrivate *priv = DDB_SEEKBAR_GET_PRIVATE (self);

#if GTK_CHECK_VERSION(3, 0, 0)
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

    DB_playItem_t *trk = deadbeef->streamer_get_playing_track_safe ();
    // filler, only while playing a finite stream
    if (trk && deadbeef->pl_get_item_duration (trk) > 0) {
        float pos = 0;
        if (priv->seekbar_moving) {
            int x = priv->seekbar_move_x;
            if (x < 0) {
                x = 0;
            }
            if (x > a.width - 1) {
                x = a.width - 1;
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
            cairo_set_source_rgb (
                cr,
                clr_selection.red / 65535.f,
                clr_selection.green / 65535.f,
                clr_selection.blue / 65535.f);
            cairo_rectangle (cr, ax, ah / 2 - 4 + ay, pos, 8);
            cairo_clip (cr);
            clearlooks_rounded_rectangle (cr, 2 + ax, ah / 2 - 4 + ay, aw - 4, 8, 4, 0xff);
            cairo_fill (cr);
            cairo_reset_clip (cr);
        }
    }

    // empty seekbar, just a frame, always visible
    clearlooks_rounded_rectangle (cr, 2 + ax, a.height / 2 - 4 + ay, aw - 4, 8, 4, 0xff);
    cairo_set_source_rgb (cr, clr_selection.red / 65535.f, clr_selection.green / 65535.f, clr_selection.blue / 65535.f);
    cairo_set_line_width (cr, 2);
    cairo_stroke (cr);

    // overlay, only while playing a finite stream, and only during seeking
    if (trk && deadbeef->pl_get_item_duration (trk) > 0) {
        if (!gtkui_disable_seekbar_overlay && (priv->seekbar_moving || priv->seekbar_moved > 0.0) && trk) {
            float time = 0;
            float dur = deadbeef->pl_get_item_duration (trk);

            if (priv->seekbar_moved > 0) {
                time = deadbeef->streamer_get_playpos ();
            }
            else {
                time = priv->seekbar_move_x * dur / (a.width);
            }

            if (time < 0) {
                time = 0;
            }
            if (time > dur) {
                time = dur;
            }
            char s[1000];
            int hr = time / 3600;
            int mn = (time - hr * 3600) / 60;
            int sc = time - hr * 3600 - mn * 60;
            snprintf (s, sizeof (s), "%02d:%02d:%02d", hr, mn, sc);

            draw_begin (&priv->drawctx, cr);

            // overlay extent

            int ew, eh;
            draw_get_text_extents (&priv->drawctx, s, (int)strlen (s), &ew, &eh);

            if (priv->textpos == -1) {
                priv->textpos = ax + aw / 2 - ew / 2;
                priv->textwidth = ew + 20;
            }

            // overlay background

            cairo_set_source_rgba (
                cr,
                clr_selection.red / 65535.f,
                clr_selection.green / 65535.f,
                clr_selection.blue / 65535.f,
                priv->seektime_alpha);
            cairo_save (cr);

            clearlooks_rounded_rectangle (
                cr,
                ax + aw / 2 - priv->textwidth / 2,
                ay + 4,
                priv->textwidth,
                ah - 8,
                3,
                0xff);
            cairo_fill (cr);
            cairo_restore (cr);

            // overlay foreground

            GdkColor clr;
            gtkui_get_listview_selected_text_color (&clr);

            float text_color[3] = { (float)clr.red / 0xffff, (float)clr.green / 0xffff, (float)clr.blue / 0xffff };

            draw_set_fg_color (&priv->drawctx, text_color);
            draw_text_custom (&priv->drawctx, priv->textpos, ay + ah / 2 - eh / 2, ew, 0, 0, 0, 0, s);

            draw_end (&priv->drawctx);

            int fps = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
            if (fps < 1) {
                fps = 1;
            }
            else if (fps > 30) {
                fps = 30;
            }
            if (priv->seekbar_moved >= 0.0) {
                priv->seekbar_moved -= 1.0 / fps;
            }
            else {
                priv->seekbar_moved = 0.0;
            }
        }
    }

    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
}

gboolean
on_seekbar_motion_notify_event (GtkWidget *widget, GdkEventMotion *event) {
    DdbSeekbar *self = DDB_SEEKBAR (widget);
    DdbSeekbarPrivate *priv = DDB_SEEKBAR_GET_PRIVATE (self);
    if (priv->seekbar_moving) {
        GtkAllocation a;
        gtk_widget_get_allocation (widget, &a);
        priv->seekbar_move_x = event->x - a.x;
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}

gboolean
on_seekbar_button_press_event (GtkWidget *widget, GdkEventButton *event) {
    DdbSeekbar *self = DDB_SEEKBAR (widget);
    DdbSeekbarPrivate *priv = DDB_SEEKBAR_GET_PRIVATE (self);
    if (deadbeef->get_output ()->state () == DDB_PLAYBACK_STATE_STOPPED) {
        return FALSE;
    }
    priv->seekbar_moving = 1;
    priv->seekbar_moved = 0;
    priv->textpos = -1;
    priv->textwidth = -1;
    priv->seektime_alpha = 0.8;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    priv->seekbar_move_x = event->x - a.x;
    gtk_widget_queue_draw (widget);
    return FALSE;
}

gboolean
on_seekbar_button_release_event (GtkWidget *widget, GdkEventButton *event) {
    DdbSeekbar *self = DDB_SEEKBAR (widget);
    DdbSeekbarPrivate *priv = DDB_SEEKBAR_GET_PRIVATE (self);
    priv->seekbar_moving = 0;
    priv->seekbar_moved = 1.0;
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track_safe ();
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
on_evbox_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

static gboolean
on_evbox_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

static gboolean
on_evbox_motion_notify_event (GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

static gboolean
on_evbox_scroll_event (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    return gtk_widget_event (GTK_WIDGET (user_data), (GdkEvent *)event);
}

void
ddb_seekbar_init_signals (DdbSeekbar *sb, GtkWidget *evbox) {
    g_signal_connect ((gpointer)evbox, "button_press_event", G_CALLBACK (on_evbox_button_press_event), sb);
    g_signal_connect ((gpointer)evbox, "button_release_event", G_CALLBACK (on_evbox_button_release_event), sb);
    g_signal_connect ((gpointer)evbox, "scroll_event", G_CALLBACK (on_evbox_scroll_event), sb);
    g_signal_connect ((gpointer)evbox, "motion_notify_event", G_CALLBACK (on_evbox_motion_notify_event), sb);
}
