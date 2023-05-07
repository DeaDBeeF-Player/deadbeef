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

#include <string.h>
#include "ddbvolumebar.h"
#include "drawing.h"
#include "gtkui.h"
#include "interface.h"
#include "support.h"
#include <math.h>

#define min(x,y) ((x)<(y)?(x):(y))

struct _DdbVolumeBarPrivate
{
    DdbVolumeBarScale scale;
};

G_DEFINE_TYPE (DdbVolumeBar, ddb_volumebar, GTK_TYPE_WIDGET);

#define DDB_VOLUMEBAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
            DDB_TYPE_VOLUMEBAR, DdbVolumeBarPrivate))

/* Property identifiers */
enum
{
    PROP_0,
    PROP_SCALE_MODE,
};

static void
ddb_volumebar_send_configure (DdbVolumeBar *darea)
{
  GtkWidget *widget;
  GdkEvent *event = gdk_event_new (GDK_CONFIGURE);

  widget = GTK_WIDGET (darea);

  event->configure.window = g_object_ref (gtk_widget_get_window(widget));
  event->configure.send_event = TRUE;
  GtkAllocation a;
  gtk_widget_get_allocation (widget, &a);
  event->configure.x = a.x;
  event->configure.y = a.y;
  event->configure.width = a.width;
  event->configure.height = a.height;
  
  gtk_widget_event (widget, event);
  gdk_event_free (event);
}

static void
ddb_volumebar_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  g_return_if_fail (DDB_IS_VOLUMEBAR (widget));
  g_return_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    {
      if (gtk_widget_get_has_window (widget))
        gdk_window_move_resize (gtk_widget_get_window(widget),
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);

      ddb_volumebar_send_configure (DDB_VOLUMEBAR (widget));
    }
}

gboolean
on_volumebar_draw (GtkWidget    *widget, cairo_t *cr);

gboolean
on_volumebar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event);

gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event);

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event);

gboolean
on_volumebar_configure_event (GtkWidget *widget, GdkEventConfigure *event);

GType
ddb_volumebar_scale_mode_get_type (void)
{
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
        static const GEnumValue values[] =
        {
            { DDB_VOLUMEBAR_SCALE_DB,   "DDB_VOLUMEBAR_SCALE_DB",   "dB Scale",       },
            { DDB_VOLUMEBAR_SCALE_LINEAR, "DDB_VOLUMEBAR_SCALE_LINEAR", "Linear scale",     },
            { DDB_VOLUMEBAR_SCALE_CUBIC,   "DDB_VOLUMEBAR_SCALE_CUBIC", "Cubic scale", },
            { 0, NULL, NULL, },
        };

        type = g_enum_register_static ("DdbVolumeBarScaleMode", values);
    }

    return type;
}

static void
ddb_volumebar_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    DdbVolumeBar *vb = DDB_VOLUMEBAR (object);

    switch (prop_id) {
        case PROP_SCALE_MODE:
            g_value_set_enum (value, ddb_volumebar_get_scale (vb));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
ddb_volumebar_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    DdbVolumeBar *vb = DDB_VOLUMEBAR (object);

    switch (prop_id) {
        case PROP_SCALE_MODE:
            ddb_volumebar_set_scale (vb, g_value_get_enum (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
ddb_volumebar_class_init(DdbVolumeBarClass *class)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  widget_class->size_allocate = ddb_volumebar_size_allocate;
#if GTK_CHECK_VERSION(3,0,0)
  widget_class->draw = on_volumebar_draw;
#else
  widget_class->expose_event = on_volumebar_expose_event;
#endif
  widget_class->button_press_event = on_volumebar_button_press_event;
  widget_class->button_release_event = on_volumebar_button_release_event;
  widget_class->motion_notify_event = on_volumebar_motion_notify_event;
  widget_class->scroll_event = on_volumebar_scroll_event;
  widget_class->configure_event = on_volumebar_configure_event;

    /* add our private data to the class */
    g_type_class_add_private (class, sizeof (DdbVolumeBarPrivate));
    
    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->get_property = ddb_volumebar_get_property;
    gobject_class->set_property = ddb_volumebar_set_property;


    g_object_class_install_property (gobject_class,
            PROP_SCALE_MODE,
            g_param_spec_enum ("scale-mode",
                "Scale mode",
                "The scale mode of the volumebar widget",
                ddb_volumebar_scale_mode_get_type(), DDB_VOLUMEBAR_SCALE_DB,
                G_PARAM_READWRITE));
}

GtkWidget * ddb_volumebar_new(void) {
    return g_object_new (DDB_TYPE_VOLUMEBAR, NULL);
}

static void
ddb_volumebar_init(DdbVolumeBar *volumebar)
{
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (GTK_WIDGET (volumebar), s);
    gtk_widget_set_has_window (GTK_WIDGET (volumebar), FALSE);
    volumebar->priv = DDB_VOLUMEBAR_GET_PRIVATE (volumebar);
    volumebar->priv->scale = DDB_VOLUMEBAR_SCALE_DB;
}

void
volumebar_draw (GtkWidget *widget, cairo_t *cr) {
    if (!widget) {
        return;
    }

#if GTK_CHECK_VERSION(3,0,0)
    GtkAllocation allocation;
    gtk_widget_get_allocation (widget, &allocation);
    cairo_translate (cr, -allocation.x, -allocation.y);
#endif

    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    float range;
    float vol;
    int n = a.width / 4;

    DdbVolumeBarScale scale = DDB_VOLUMEBAR(widget)->priv->scale;

    switch (scale) {
    case DDB_VOLUMEBAR_SCALE_LINEAR:
        vol = (deadbeef->volume_get_amp () ) * n;
        break;
    case DDB_VOLUMEBAR_SCALE_CUBIC:
        vol = (cbrt(deadbeef->volume_get_amp ()) ) * n;
        break;
    case DDB_VOLUMEBAR_SCALE_DB:
    default:
        range = -deadbeef->volume_get_min_db ();
        vol = (range + deadbeef->volume_get_db ()) / range * n;
        break;
    }

    float h = 17;

    GdkColor clr_fg;
    gtkui_get_bar_foreground_color (&clr_fg);

    for (int i = 0; i < n; i++) {
        float iy = (float)i + 3;
        int _x = i * 4;
        int _h = h * iy / n;
        int _y = a.height/2-h/2;
        _y += (h - _h);
        int _w = 3;
        if (i < vol) {
            cairo_set_source_rgb (cr, clr_fg.red/65535.f, clr_fg.green/65535.f, clr_fg.blue/65535.f);
            cairo_rectangle (cr, _x + a.x, _y + a.y, _w, _h);
            cairo_fill (cr);
        }
        else {
            cairo_set_source_rgba (cr, clr_fg.red/65535.f, clr_fg.green/65535.f, clr_fg.blue/65535.f, 0.3f);
            cairo_rectangle (cr, _x + a.x, _y + a.y, _w, _h);
            cairo_fill (cr);
        }
    }
}

gboolean
on_volumebar_draw (GtkWidget    *widget, cairo_t *cr) {
    volumebar_draw (widget, cr);
    return FALSE;
}

#if !GTK_CHECK_VERSION(3,0,0)
gboolean
on_volumebar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event)
{
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    on_volumebar_draw (widget, cr);
    cairo_destroy (cr);
    return FALSE;
}
#endif

void
ddb_volumebar_update(DdbVolumeBar *volumebar)
{
    gtk_widget_queue_draw (GTK_WIDGET (volumebar));
    char s[100];
    if (volumebar->priv->scale == DDB_VOLUMEBAR_SCALE_DB) {
        int db = deadbeef->volume_get_db ();
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    } else {
        float volume = deadbeef->volume_get_amp ();
        if (volumebar->priv->scale == DDB_VOLUMEBAR_SCALE_CUBIC) {
            volume = cbrt (volume);
        }
        int pct = (int)round (volume*100);
        snprintf (s, sizeof (s), "%d%%", pct);
    }
    gtk_widget_set_tooltip_text (GTK_WIDGET (volumebar), s);
    gtk_widget_trigger_tooltip_query (GTK_WIDGET (volumebar));
}

gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event)
{
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (event->state & GDK_BUTTON1_MASK) {
        DdbVolumeBarScale scale = DDB_VOLUMEBAR(widget)->priv->scale;

        if (scale == DDB_VOLUMEBAR_SCALE_DB) {
            float range = -deadbeef->volume_get_min_db ();
            float volume = (event->x - a.x) / a.width * range - range;
            if (volume > 0) {
                volume = 0;
            }
            if (volume < -range) {
                volume = -range;
            }
            deadbeef->volume_set_db (volume);
        } else {
            float volume = (event->x - a.x) / a.width;
            if (scale == DDB_VOLUMEBAR_SCALE_CUBIC) {
                volume = volume * volume * volume;
            }
            deadbeef->volume_set_amp (volume);
        }
        ddb_volumebar_update (DDB_VOLUMEBAR (widget));
    }
    return FALSE;
}

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (event->button == 1) {
        DdbVolumeBarScale scale = DDB_VOLUMEBAR(widget)->priv->scale;

        if (scale == DDB_VOLUMEBAR_SCALE_DB) {
            float range = -deadbeef->volume_get_min_db ();
            float volume = (event->x - a.x) / a.width * range - range;
            if (volume > 0) {
                volume = 0;
            }
            if (volume < -range) {
                volume = -range;
            }
            deadbeef->volume_set_db (volume);
        } else {
            float volume = (event->x - a.x) / a.width;
            if (scale == DDB_VOLUMEBAR_SCALE_CUBIC) {
                volume = volume * volume * volume;
            }
            deadbeef->volume_set_amp (volume);
        }
        ddb_volumebar_update (DDB_VOLUMEBAR (widget));
    }
    return FALSE;
}


gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    if (event->button == 1) {
//        DDB_VOLUMEBAR (widget)->show_dbs = 0;
        gtk_widget_queue_draw (widget);
    }
  return FALSE;
}

gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event)
{
    DdbVolumeBarScale scale = DDB_VOLUMEBAR(widget)->priv->scale;
    if (scale == DDB_VOLUMEBAR_SCALE_DB) {
        float range = -deadbeef->volume_get_min_db ();
        float vol = deadbeef->volume_get_db ();
        if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
            vol += 1;
        }
        else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
            vol -= 1;
        }
        if (vol > 0) {
            vol = 0;
        }
        else if (vol < -range) {
            vol = -range;
        }
        deadbeef->volume_set_db (vol);
    } else {
        float dbvol = deadbeef->volume_get_amp ();
        if (scale == DDB_VOLUMEBAR_SCALE_CUBIC) {
            dbvol = cbrt(dbvol);
        }

        int vol = (int)round(dbvol*100.0);

        if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
            vol += 5;
        }
        else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
            vol -= 5;
        }
        if (vol > 100) {
            vol = 100;
        }
        else if (vol < 0) {
            vol = 0;
        }
        if (scale == DDB_VOLUMEBAR_SCALE_CUBIC) {
            deadbeef->volume_set_amp (pow(vol/100.0,3));
        } else {
            deadbeef->volume_set_amp (vol/100.0);
        }
    }
    ddb_volumebar_update (DDB_VOLUMEBAR (widget));
    return FALSE;
}

gboolean
on_volumebar_configure_event (GtkWidget *widget, GdkEventConfigure *event) {
    gtkui_init_theme_colors ();
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
ddb_volumebar_init_signals (DdbVolumeBar *vb, GtkWidget *evbox) {
  g_signal_connect ((gpointer) evbox, "button_press_event",
                    G_CALLBACK (on_evbox_button_press_event),
                    vb);
  g_signal_connect ((gpointer) evbox, "button_release_event",
                    G_CALLBACK (on_evbox_button_release_event),
                    vb);
  g_signal_connect ((gpointer) evbox, "scroll_event",
                    G_CALLBACK (on_evbox_scroll_event),
                    vb);
  g_signal_connect ((gpointer) evbox, "motion_notify_event",
                    G_CALLBACK (on_evbox_motion_notify_event),
                    vb);
}

DdbVolumeBarScale
ddb_volumebar_get_scale (const DdbVolumeBar *volumebar)
{
    g_return_val_if_fail (DDB_IS_VOLUMEBAR (volumebar), DDB_VOLUMEBAR_SCALE_DB);
    return volumebar->priv->scale;
}

void
ddb_volumebar_set_scale (DdbVolumeBar *volumebar, DdbVolumeBarScale scale)
{
    g_return_if_fail (DDB_IS_VOLUMEBAR (volumebar));

    if (G_LIKELY (volumebar->priv->scale != scale)) {
        volumebar->priv->scale = scale;
        gtk_widget_queue_resize (GTK_WIDGET (volumebar));
        g_object_notify (G_OBJECT (volumebar), "scale_mode");
    }
}
