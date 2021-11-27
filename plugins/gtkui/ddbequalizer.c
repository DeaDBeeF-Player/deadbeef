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

// Original Vala code: Copyright (C) 2010 Viktor Semykin <thesame.ml@gmail.com>
#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <float.h>
#include <math.h>
#include <gdk/gdk.h>
#include <drawing.h>
#include <gtkui.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <pango/pango.h>
#include "support.h"

#define DDB_TYPE_EQUALIZER (ddb_equalizer_get_type ())
#define DDB_EQUALIZER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_EQUALIZER, DdbEqualizer))
#define DDB_EQUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_EQUALIZER, DdbEqualizerClass))
#define DDB_IS_EQUALIZER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_EQUALIZER))
#define DDB_IS_EQUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_EQUALIZER))
#define DDB_EQUALIZER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_EQUALIZER, DdbEqualizerClass))

typedef struct _DdbEqualizer DdbEqualizer;
typedef struct _DdbEqualizerClass DdbEqualizerClass;
typedef struct _DdbEqualizerPrivate DdbEqualizerPrivate;
#define _gdk_cursor_unref0(var) ((var == NULL) ? NULL : (var = (gdk_cursor_unref (var), NULL)))
#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _cairo_destroy0(var) ((var == NULL) ? NULL : (var = (cairo_destroy (var), NULL)))

struct _DdbEqualizer {
    GtkDrawingArea parent_instance;
    DdbEqualizerPrivate * priv;
};

struct _DdbEqualizerClass {
    GtkDrawingAreaClass parent_class;
};

struct _DdbEqualizerPrivate {
    gdouble* values;
    gint values_length1;
    gint _values_size_;
    gdouble preamp;
    gint mouse_y;
    gboolean curve_hook;
    gboolean preamp_hook;
    gint eq_margin_bottom;
    gint eq_margin_left;
    GdkCursor* pointer_cursor;
};


static gpointer ddb_equalizer_parent_class = NULL;

GType ddb_equalizer_get_type (void);
#define DDB_EQUALIZER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_TYPE_EQUALIZER, DdbEqualizerPrivate))
enum  {
    DDB_EQUALIZER_DUMMY_PROPERTY
};
#define DDB_EQUALIZER_BANDS 18
static gboolean ddb_equalizer_real_configure_event (GtkWidget* base, GdkEventConfigure* event);
static void ddb_equalizer_real_realize (GtkWidget* base);
#if !GTK_CHECK_VERSION(3,0,0)
static gboolean ddb_equalizer_real_expose_event (GtkWidget* base, GdkEventExpose* event);
#endif
static inline gdouble ddb_equalizer_scale (DdbEqualizer* self, gdouble val);
static gboolean ddb_equalizer_in_curve_area (DdbEqualizer* self, gdouble x, gdouble y);
static void ddb_equalizer_update_eq_drag (DdbEqualizer* self, gdouble x, gdouble y);
static gboolean ddb_equalizer_real_button_press_event (GtkWidget* base, GdkEventButton* event);
static gboolean ddb_equalizer_real_button_release_event (GtkWidget* base, GdkEventButton* event);
static gboolean ddb_equalizer_real_leave_notify_event (GtkWidget* base, GdkEventCrossing* event);
static gboolean ddb_equalizer_real_motion_notify_event (GtkWidget* base, GdkEventMotion* event);
void ddb_equalizer_set_band (DdbEqualizer* self, gint band, gdouble v);
gdouble ddb_equalizer_get_band (DdbEqualizer* self, gint band);
void ddb_equalizer_set_preamp (DdbEqualizer* self, gdouble v);
gdouble ddb_equalizer_get_preamp (DdbEqualizer* self);
DdbEqualizer* ddb_equalizer_new (void);
DdbEqualizer* ddb_equalizer_construct (GType object_type);
static GObject * ddb_equalizer_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void ddb_equalizer_finalize (GObject* obj);

const gchar* freqs[18] = {"55 Hz", "77 Hz", "110 Hz", "156 Hz", "220 Hz", "311 Hz", "440 Hz", "622 Hz", "880 Hz", "1.2 kHz", "1.8 kHz", "2.5 kHz", "3.5 kHz", "5 kHz", "7 kHz", "10 kHz", "14 kHz", "20 kHz"};

static gboolean ddb_equalizer_real_configure_event (GtkWidget* base, GdkEventConfigure* event) {
    DdbEqualizer * self;
    gboolean result = FALSE;
    self = (DdbEqualizer*) base;
    g_return_val_if_fail (event != NULL, FALSE);
    gtkui_init_theme_colors ();

    GtkStyle* _tmp0_ = NULL;
    const PangoFontDescription* _tmp1_;
    gint _tmp2_ = 0;
    gdouble _tmp3_ = 0.0;
    GdkScreen* _tmp4_ = NULL;
    gdouble _tmp5_ = 0.0;

    _tmp0_ = gtk_widget_get_style ((GtkWidget*) self);
    _tmp1_ = _tmp0_->font_desc;
    _tmp2_ = pango_font_description_get_size (_tmp1_);
    _tmp3_ = pango_units_to_double (_tmp2_);
    _tmp4_ = gdk_screen_get_default ();
    _tmp5_ = gdk_screen_get_resolution (_tmp4_);
    self->priv->eq_margin_bottom = (gint) (_tmp3_ + 4);
    self->priv->eq_margin_left = (gint) (_tmp3_ + 4) * 4;
    result = FALSE;
    return result;
}


static void ddb_equalizer_real_realize (GtkWidget* base) {
    DdbEqualizer * self;
    self = (DdbEqualizer*) base;
    GTK_WIDGET_CLASS (ddb_equalizer_parent_class)->realize ((GtkWidget*) GTK_DRAWING_AREA (self));
    gtk_widget_add_events ((GtkWidget*) self, (gint) ((((GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK) | GDK_BUTTON_RELEASE_MASK) | GDK_LEAVE_NOTIFY_MASK) | GDK_POINTER_MOTION_MASK));
}


static gboolean ddb_equalizer_real_draw (GtkWidget *widget, cairo_t *cr) {
    DdbEqualizer *self = DDB_EQUALIZER (widget);
    GdkColor fore_bright_color;
    gtkui_get_bar_foreground_color (&fore_bright_color);
    GdkColor c1 = fore_bright_color;
    GdkColor c2;
    gtkui_get_bar_background_color (&c2);
    GdkColor fore_dark_color = c2;

    fore_dark_color.red += (c1.red - c2.red) * 0.5;
    fore_dark_color.green += (c1.green - c2.green) * 0.5;
    fore_dark_color.blue += (c1.blue - c2.blue) * 0.5;

    GtkAllocation alloc;
    gtk_widget_get_allocation (widget, &alloc);

    int width = alloc.width;
    int height = alloc.height;

    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width (cr, 1.0);
    gdk_cairo_set_source_color (cr, &c2);
    cairo_rectangle (cr, 0, 0, width, height);
    cairo_fill (cr);

    gdk_cairo_set_source_color (cr, &fore_dark_color);

    //drawing grid:
    double step = (double)(width - self->priv->eq_margin_left) / (double)(DDB_EQUALIZER_BANDS+1);
    int i;
    for (i = 0; i < DDB_EQUALIZER_BANDS; i++)
    {
        cairo_move_to (cr, (int)((i+1)*step)+self->priv->eq_margin_left, 0);
        cairo_line_to (cr, (int)((i+1)*step)+self->priv->eq_margin_left, height - self->priv->eq_margin_bottom);
    }

    double vstep = (double)(height-self->priv->eq_margin_bottom);
    for (double di=0; di < 2; di += 0.25)
    {
        int y = (int)((di-self->priv->preamp)*vstep);
        if (y < alloc.height-self->priv->eq_margin_bottom) {
            cairo_move_to (cr, self->priv->eq_margin_left, y);
            cairo_line_to (cr, width, y);
        }
    }
    cairo_stroke (cr);

    gdk_cairo_set_source_color (cr, &fore_bright_color);

    //drawing freqs:

    PangoLayout *l = pango_cairo_create_layout (cr);
    PangoContext *pctx = pango_layout_get_context (l);

    GtkStyle *st = gtk_widget_get_style (widget);

    PangoFontDescription *fd = pango_font_description_copy (st->font_desc);

    pango_font_description_set_size (fd, (int)(pango_font_description_get_size (st->font_desc) * 0.7));
    pango_context_set_font_description (pctx, fd);
    for (i = 0; i < DDB_EQUALIZER_BANDS; i++)
    {
        cairo_save (cr);
        pango_layout_set_text (l, freqs[i], (int)strlen (freqs[i]));
        PangoRectangle ink, log;
        pango_layout_get_pixel_extents (l, &ink, &log);
        int offs = 2;
        if ((i % 2) != 0) {
            offs += 2;
        }
        cairo_move_to (cr, (int)((i+1)*step)+self->priv->eq_margin_left - ink.width/2, height-self->priv->eq_margin_bottom + offs);
        pango_cairo_show_layout (cr, l);
        cairo_restore (cr);
    }
    pango_font_description_set_size (fd, (int)(pango_font_description_get_size (st->font_desc)));
    pango_context_set_font_description (pctx, fd);

    //drawing db's:
    pango_layout_set_width (l, self->priv->eq_margin_left-1);
    pango_layout_set_alignment (l, PANGO_ALIGN_RIGHT);

    int fontsize = (int)(pango_units_to_double (pango_font_description_get_size (fd)) * gdk_screen_get_resolution (gdk_screen_get_default ()) / 72);

    char tmp[100];

    if ((self->priv->mouse_y >= 0) && (self->priv->mouse_y < height - self->priv->eq_margin_bottom))
    {
        cairo_save (cr);
        double db = ddb_equalizer_scale(self, (double)(self->priv->mouse_y-1) / (double)(height - self->priv->eq_margin_bottom - 2));
        snprintf (tmp, sizeof (tmp), "%s%.1fdB", db > 0 ? "+" : "", db);
        pango_layout_set_text (l, tmp, (int)strlen (tmp));
        cairo_move_to (cr, self->priv->eq_margin_left-1, self->priv->mouse_y-3);
        pango_cairo_show_layout (cr, l);
        cairo_restore (cr);
    }

    cairo_save (cr);
    double val = ddb_equalizer_scale(self, 1);
    snprintf (tmp, sizeof (tmp), "%s%.1fdB", val > 0 ? "+" : "", val);
    pango_layout_set_text (l, tmp, (int)strlen(tmp));
    cairo_move_to (cr, self->priv->eq_margin_left-1, height-self->priv->eq_margin_bottom-fontsize);
    pango_cairo_show_layout (cr, l);
    cairo_restore (cr);

    cairo_save (cr);
    val = ddb_equalizer_scale(self, 0);
    snprintf (tmp, sizeof (tmp), "%s%.1fdB", val > 0 ? "+" : "", val);
    pango_layout_set_text (l, tmp, (int)strlen (tmp));
    cairo_move_to (cr, self->priv->eq_margin_left-1, 1);
    pango_cairo_show_layout (cr, l);
    cairo_restore (cr);

    cairo_save (cr);
    pango_layout_set_text (l, "+0dB", 4);
    cairo_move_to (cr, self->priv->eq_margin_left-1, (int)((1-self->priv->preamp)*(height-self->priv->eq_margin_bottom))-fontsize/2);
    pango_cairo_show_layout (cr, l);
    cairo_restore (cr);

    cairo_save (cr);
    pango_layout_set_text (l, "preamp", 6);
    pango_layout_set_alignment (l, PANGO_ALIGN_LEFT);
    cairo_move_to (cr, 1, height-self->priv->eq_margin_bottom-2);
    pango_cairo_show_layout (cr, l);
    cairo_restore (cr);

    // frame
    cairo_rectangle (cr, self->priv->eq_margin_left, 0, width-self->priv->eq_margin_left, height-self->priv->eq_margin_bottom);
    cairo_stroke (cr);

    //draw preamp
    cairo_rectangle (cr, 0, (int)(self->priv->preamp * (height-self->priv->eq_margin_bottom)), 11, height);
    cairo_clip (cr);

    gdk_cairo_set_source_color (cr, &fore_bright_color);
    int count = (int)((height-self->priv->eq_margin_bottom) / 6)+1;
    for (int j = 0; j < count; j++) {
        cairo_rectangle (cr, 1, height-self->priv->eq_margin_bottom-j*6 - 6, 11, 4);
    }
    cairo_fill (cr);
    cairo_reset_clip (cr);

    //drawing bars:
    int bar_w = 11;
    if (step < bar_w)
        bar_w = (int)step-1;


    for (i = 0; i < DDB_EQUALIZER_BANDS; i++)
    {
        cairo_reset_clip (cr);
        cairo_rectangle (cr, (int)((i+1)*step)+self->priv->eq_margin_left - bar_w/2, (int)(self->priv->values[i] * (height-self->priv->eq_margin_bottom)), 11, height);
        cairo_clip (cr);
        count = (int)((height-self->priv->eq_margin_bottom) * (1-self->priv->values[i]) / 6)+1;
        for (int j = 0; j < count; j++) {
            cairo_rectangle (cr, (int)((i+1)*step)+self->priv->eq_margin_left - bar_w/2, height-self->priv->eq_margin_bottom-j*6 - 6, bar_w, 4);
        }
        cairo_fill (cr);
    }

    //drawing mouse coordinates:
    cairo_reset_clip (cr);
    double dash[] = {4, 4};
    cairo_set_dash (cr, dash, 2, 0);
    cairo_move_to (cr, self->priv->eq_margin_left+1, self->priv->mouse_y);
    cairo_line_to (cr, width, self->priv->mouse_y);
    cairo_stroke (cr);

    return FALSE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static gboolean ddb_equalizer_real_expose_event (GtkWidget* base, GdkEventExpose* event) {
    g_return_val_if_fail (event != NULL, FALSE);
    cairo_t *cr= gdk_cairo_create ((GdkDrawable*) gtk_widget_get_window (base));
    ddb_equalizer_real_draw (base, cr);
    _cairo_destroy0 (cr);
    return FALSE;
}
#endif


static inline gdouble ddb_equalizer_scale (DdbEqualizer* self, gdouble val) {
    gdouble result = 0.0;
    gdouble k;
    gdouble d;
    gdouble _tmp0_;
    gdouble _tmp1_;
    g_return_val_if_fail (self != NULL, 0.0);
    k = (gdouble) (-40);
    d = (gdouble) 20;
    _tmp0_ = val;
    _tmp1_ = self->priv->preamp;
    result = (((_tmp0_ + _tmp1_) - 0.5) * k) + d;
    return result;
}


static gboolean ddb_equalizer_in_curve_area (DdbEqualizer* self, gdouble x, gdouble y) {
    gboolean result = FALSE;
    gboolean _tmp0_ = FALSE;
    gboolean _tmp1_ = FALSE;
    gboolean _tmp2_ = FALSE;
    gdouble _tmp3_;
    gint _tmp4_;
    gboolean _tmp8_;
    gboolean _tmp10_;
    gboolean _tmp15_;
    g_return_val_if_fail (self != NULL, FALSE);
    _tmp3_ = x;
    _tmp4_ = self->priv->eq_margin_left;
    GtkAllocation _tmp6_;
    gtk_widget_get_allocation ((GtkWidget*) self, &_tmp6_);
    if (_tmp3_ > ((gdouble) _tmp4_)) {
        gdouble _tmp5_;
        gint _tmp7_;
        _tmp5_ = x;
        _tmp7_ = _tmp6_.width;
        _tmp2_ = _tmp5_ < ((gdouble) (_tmp7_ - 1));
    } else {
        _tmp2_ = FALSE;
    }
    _tmp8_ = _tmp2_;
    if (_tmp8_) {
        gdouble _tmp9_;
        _tmp9_ = y;
        _tmp1_ = _tmp9_ > ((gdouble) 1);
    } else {
        _tmp1_ = FALSE;
    }
    _tmp10_ = _tmp1_;
    if (_tmp10_) {
        gdouble _tmp11_;
        gint _tmp13_;
        gint _tmp14_;
        _tmp11_ = y;
        _tmp13_ = _tmp6_.height;
        _tmp14_ = self->priv->eq_margin_bottom;
        _tmp0_ = _tmp11_ < ((gdouble) (_tmp13_ - _tmp14_));
    } else {
        _tmp0_ = FALSE;
    }
    _tmp15_ = _tmp0_;
    result = _tmp15_;
    return result;
}


static void ddb_equalizer_update_eq_drag (DdbEqualizer* self, gdouble x, gdouble y) {
    GtkAllocation _tmp0_;
    gint _tmp1_;
    gint _tmp2_;
    gdouble band_width;
    gdouble _tmp3_;
    gint _tmp4_;
    gdouble _tmp5_;
    gdouble _tmp6_ = 0.0;
    gint band;
    gint _tmp7_;
    gint _tmp8_;
    gboolean _tmp10_ = FALSE;
    gint _tmp11_;
    gboolean _tmp13_;
    g_return_if_fail (self != NULL);
    gtk_widget_get_allocation ((GtkWidget*) self, &_tmp0_);
    _tmp1_ = _tmp0_.width;
    _tmp2_ = self->priv->eq_margin_left;
    band_width = ((gdouble) (_tmp1_ - _tmp2_)) / ((gdouble) (DDB_EQUALIZER_BANDS + 1));
    _tmp3_ = x;
    _tmp4_ = self->priv->eq_margin_left;
    _tmp5_ = band_width;
    _tmp6_ = floor (((_tmp3_ - _tmp4_) / _tmp5_) - 0.5);
    band = (gint) _tmp6_;
    _tmp7_ = band;
    if (_tmp7_ < 0) {
        band = 0;
    }
    _tmp8_ = band;
    if (_tmp8_ >= DDB_EQUALIZER_BANDS) {
        gint _tmp9_;
        _tmp9_ = band;
        band = _tmp9_ - 1;
    }
    _tmp11_ = band;
    if (_tmp11_ >= 0) {
        gint _tmp12_;
        _tmp12_ = band;
        _tmp10_ = _tmp12_ < DDB_EQUALIZER_BANDS;
    } else {
        _tmp10_ = FALSE;
    }
    _tmp13_ = _tmp10_;
    if (_tmp13_) {
        gdouble* _tmp14_;
        gint _tmp14__length1;
        gint _tmp15_;
        gdouble _tmp16_;
        GtkAllocation _tmp17_;
        gint _tmp18_;
        gint _tmp19_;
        gdouble _tmp20_;
        gdouble* _tmp21_;
        gint _tmp21__length1;
        gint _tmp22_;
        gdouble _tmp23_;
        _tmp14_ = self->priv->values;
        _tmp14__length1 = self->priv->values_length1;
        _tmp15_ = band;
        _tmp16_ = y;
        gtk_widget_get_allocation ((GtkWidget*) self, &_tmp17_);
        _tmp18_ = _tmp17_.height;
        _tmp19_ = self->priv->eq_margin_bottom;
        _tmp14_[_tmp15_] = _tmp16_ / ((gdouble) (_tmp18_ - _tmp19_));
        _tmp20_ = _tmp14_[_tmp15_];
        _tmp21_ = self->priv->values;
        _tmp21__length1 = self->priv->values_length1;
        _tmp22_ = band;
        _tmp23_ = _tmp21_[_tmp22_];
        if (_tmp23_ > ((gdouble) 1)) {
            gdouble* _tmp24_;
            gint _tmp24__length1;
            gint _tmp25_;
            gdouble _tmp26_;
            _tmp24_ = self->priv->values;
            _tmp24__length1 = self->priv->values_length1;
            _tmp25_ = band;
            _tmp24_[_tmp25_] = (gdouble) 1;
            _tmp26_ = _tmp24_[_tmp25_];
        } else {
            gdouble* _tmp27_;
            gint _tmp27__length1;
            gint _tmp28_;
            gdouble _tmp29_;
            _tmp27_ = self->priv->values;
            _tmp27__length1 = self->priv->values_length1;
            _tmp28_ = band;
            _tmp29_ = _tmp27_[_tmp28_];
            if (_tmp29_ < ((gdouble) 0)) {
                gdouble* _tmp30_;
                gint _tmp30__length1;
                gint _tmp31_;
                gdouble _tmp32_;
                _tmp30_ = self->priv->values;
                _tmp30__length1 = self->priv->values_length1;
                _tmp31_ = band;
                _tmp30_[_tmp31_] = (gdouble) 0;
                _tmp32_ = _tmp30_[_tmp31_];
            }
        }
        g_signal_emit_by_name (self, "on-changed");
    }
}


static gboolean ddb_equalizer_real_button_press_event (GtkWidget* base, GdkEventButton* event) {
    if (!TEST_LEFT_CLICK (event)) {
        return FALSE;
    }
    DdbEqualizer * self;
    gboolean result = FALSE;
    GdkEventButton _tmp0_;
    gdouble _tmp1_;
    GdkEventButton _tmp2_;
    gdouble _tmp3_;
    gboolean _tmp4_ = FALSE;
    gboolean _tmp11_ = FALSE;
    gboolean _tmp12_ = FALSE;
    gboolean _tmp13_ = FALSE;
    GdkEventButton _tmp14_;
    gdouble _tmp15_;
    gboolean _tmp18_;
    gboolean _tmp24_;
    gboolean _tmp27_;
    self = (DdbEqualizer*) base;
    g_return_val_if_fail (event != NULL, FALSE);
    _tmp0_ = *event;
    _tmp1_ = _tmp0_.x;
    _tmp2_ = *event;
    _tmp3_ = _tmp2_.y;
    _tmp4_ = ddb_equalizer_in_curve_area (self, (gdouble) ((gint) _tmp1_), (gdouble) ((gint) _tmp3_));
    if (_tmp4_) {
        GdkEventButton _tmp5_;
        gdouble _tmp6_;
        GdkEventButton _tmp7_;
        gdouble _tmp8_;
        GdkEventButton _tmp9_;
        gdouble _tmp10_;
        self->priv->curve_hook = TRUE;
        _tmp5_ = *event;
        _tmp6_ = _tmp5_.x;
        _tmp7_ = *event;
        _tmp8_ = _tmp7_.y;
        ddb_equalizer_update_eq_drag (self, (gdouble) ((gint) _tmp6_), (gdouble) ((gint) _tmp8_));
        _tmp9_ = *event;
        _tmp10_ = _tmp9_.y;
        self->priv->mouse_y = (gint) _tmp10_;
        gtk_widget_queue_draw ((GtkWidget*) self);
        result = FALSE;
        return result;
    }
    _tmp14_ = *event;
    _tmp15_ = _tmp14_.x;
    if (_tmp15_ <= ((gdouble) 11)) {
        GdkEventButton _tmp16_;
        gdouble _tmp17_;
        _tmp16_ = *event;
        _tmp17_ = _tmp16_.y;
        _tmp13_ = _tmp17_ > ((gdouble) 1);
    } else {
        _tmp13_ = FALSE;
    }
    _tmp18_ = _tmp13_;
    if (_tmp18_) {
        GdkEventButton _tmp19_;
        gdouble _tmp20_;
        GtkAllocation _tmp21_;
        gint _tmp22_;
        gint _tmp23_;
        _tmp19_ = *event;
        _tmp20_ = _tmp19_.y;
        gtk_widget_get_allocation ((GtkWidget*) self, &_tmp21_);
        _tmp22_ = _tmp21_.height;
        _tmp23_ = self->priv->eq_margin_bottom;
        _tmp12_ = _tmp20_ <= ((gdouble) (_tmp22_ - _tmp23_));
    } else {
        _tmp12_ = FALSE;
    }
    _tmp24_ = _tmp12_;
    if (_tmp24_) {
        GdkEventButton _tmp25_;
        guint _tmp26_;
        _tmp25_ = *event;
        _tmp26_ = _tmp25_.button;
        _tmp11_ = _tmp26_ == ((guint) 1);
    } else {
        _tmp11_ = FALSE;
    }
    _tmp27_ = _tmp11_;
    if (_tmp27_) {
        GdkEventButton _tmp28_;
        gdouble _tmp29_;
        GtkAllocation _tmp30_;
        gint _tmp31_;
        gint _tmp32_;
        GdkEventButton _tmp33_;
        gdouble _tmp34_;
        _tmp28_ = *event;
        _tmp29_ = _tmp28_.y;
        gtk_widget_get_allocation ((GtkWidget*) self, &_tmp30_);
        _tmp31_ = _tmp30_.height;
        _tmp32_ = self->priv->eq_margin_bottom;
        self->priv->preamp = _tmp29_ / ((gdouble) (_tmp31_ - _tmp32_));
        g_signal_emit_by_name (self, "on-changed");
        self->priv->preamp_hook = TRUE;
        _tmp33_ = *event;
        _tmp34_ = _tmp33_.y;
        self->priv->mouse_y = (gint) _tmp34_;
        gtk_widget_queue_draw ((GtkWidget*) self);
    }
    result = FALSE;
    return result;
}


static gboolean ddb_equalizer_real_button_release_event (GtkWidget* base, GdkEventButton* event) {
    DdbEqualizer * self;
    gboolean result = FALSE;
    GdkWindow* _tmp0_ = NULL;
    GdkCursor* _tmp1_;
    self = (DdbEqualizer*) base;
    if (!self->priv->curve_hook && !self->priv->preamp_hook) {
        return FALSE;
    }
    g_return_val_if_fail (event != NULL, FALSE);
    self->priv->curve_hook = FALSE;
    self->priv->preamp_hook = FALSE;
    _tmp0_ = gtk_widget_get_window ((GtkWidget*) self);
    _tmp1_ = self->priv->pointer_cursor;
    gdk_window_set_cursor (_tmp0_, _tmp1_);
    result = FALSE;
    return result;
}


static gboolean ddb_equalizer_real_leave_notify_event (GtkWidget* base, GdkEventCrossing* event) {
    DdbEqualizer * self;
    gboolean result = FALSE;
    self = (DdbEqualizer*) base;
    g_return_val_if_fail (event != NULL, FALSE);
    self->priv->mouse_y = -1;
    gtk_widget_queue_draw ((GtkWidget*) self);
    result = FALSE;
    return result;
}


static gboolean ddb_equalizer_real_motion_notify_event (GtkWidget* base, GdkEventMotion* event) {
    DdbEqualizer * self;
    gboolean result = FALSE;
    GdkEventMotion _tmp0_;
    gdouble _tmp1_;
    GtkAllocation _tmp2_;
    gint _tmp3_;
    gint _tmp4_;
    gdouble y;
    gdouble _tmp5_;
    gdouble _tmp6_;
    gboolean _tmp7_;
    GdkEventMotion _tmp9_;
    gdouble _tmp10_;
    GdkEventMotion _tmp11_;
    gdouble _tmp12_;
    gboolean _tmp13_ = FALSE;
    gboolean _tmp16_;
    self = (DdbEqualizer*) base;

    g_return_val_if_fail (event != NULL, FALSE);
    _tmp0_ = *event;
    _tmp1_ = _tmp0_.y;
    gtk_widget_get_allocation ((GtkWidget*) self, &_tmp2_);
    _tmp3_ = _tmp2_.height;
    _tmp4_ = self->priv->eq_margin_bottom;
    y = _tmp1_ / ((gdouble) (_tmp3_ - _tmp4_));
    _tmp5_ = y;
    if (_tmp5_ < ((gdouble) 0)) {
        y = (gdouble) 0;
    }
    _tmp6_ = y;
    if (_tmp6_ > ((gdouble) 1)) {
        y = (gdouble) 1;
    }
    _tmp7_ = self->priv->preamp_hook;
    if (_tmp7_) {
        gdouble _tmp8_;
        _tmp8_ = y;
        self->priv->preamp = _tmp8_;
        g_signal_emit_by_name (self, "on-changed");
        gtk_widget_queue_draw ((GtkWidget*) self);
        result = FALSE;
        return result;
    }
    _tmp9_ = *event;
    _tmp10_ = _tmp9_.x;
    _tmp11_ = *event;
    _tmp12_ = _tmp11_.y;
    _tmp13_ = ddb_equalizer_in_curve_area (self, (gdouble) ((gint) _tmp10_), (gdouble) ((gint) _tmp12_));
    if (!_tmp13_) {
        self->priv->mouse_y = -1;
    } else {
        GdkEventMotion _tmp14_;
        gdouble _tmp15_;
        _tmp14_ = *event;
        _tmp15_ = _tmp14_.y;
        self->priv->mouse_y = (gint) _tmp15_;
    }
    _tmp16_ = self->priv->curve_hook;
    if (_tmp16_) {
        GdkEventMotion _tmp17_;
        gdouble _tmp18_;
        GdkEventMotion _tmp19_;
        gdouble _tmp20_;
        GdkEventMotion _tmp21_;
        gdouble _tmp22_;
        _tmp17_ = *event;
        _tmp18_ = _tmp17_.x;
        _tmp19_ = *event;
        _tmp20_ = _tmp19_.y;
        ddb_equalizer_update_eq_drag (self, (gdouble) ((gint) _tmp18_), (gdouble) ((gint) _tmp20_));
        _tmp21_ = *event;
        _tmp22_ = _tmp21_.y;
        self->priv->mouse_y = (gint) _tmp22_;
    }
    gtk_widget_queue_draw ((GtkWidget*) self);
    result = FALSE;
    return result;
}


void ddb_equalizer_set_band (DdbEqualizer* self, gint band, gdouble v) {
    gdouble* _tmp0_;
    gint _tmp0__length1;
    gint _tmp1_;
    gdouble _tmp2_;
    gdouble _tmp3_;
    g_return_if_fail (self != NULL);
    _tmp0_ = self->priv->values;
    _tmp0__length1 = self->priv->values_length1;
    _tmp1_ = band;
    _tmp2_ = v;
    _tmp0_[_tmp1_] = 1 - ((_tmp2_ + 20.0) / 40.0);
    _tmp3_ = _tmp0_[_tmp1_];
}


gdouble ddb_equalizer_get_band (DdbEqualizer* self, gint band) {
    gdouble result = 0.0;
    gdouble* _tmp0_;
    gint _tmp0__length1;
    gint _tmp1_;
    gdouble _tmp2_;
    g_return_val_if_fail (self != NULL, 0.0);
    _tmp0_ = self->priv->values;
    _tmp0__length1 = self->priv->values_length1;
    _tmp1_ = band;
    _tmp2_ = _tmp0_[_tmp1_];
    result = ((1 - _tmp2_) * 40.0) - 20.0;
    return result;
}


void ddb_equalizer_set_preamp (DdbEqualizer* self, gdouble v) {
    gdouble _tmp0_;
    g_return_if_fail (self != NULL);
    _tmp0_ = v;
    self->priv->preamp = 1 - ((_tmp0_ + 20.0) / 40.0);
}


gdouble ddb_equalizer_get_preamp (DdbEqualizer* self) {
    gdouble result = 0.0;
    gdouble _tmp0_;
    g_return_val_if_fail (self != NULL, 0.0);
    _tmp0_ = self->priv->preamp;
    result = ((1 - _tmp0_) * 40.0) - 20.0;
    return result;
}


DdbEqualizer* ddb_equalizer_construct (GType object_type) {
    DdbEqualizer * self = NULL;
    self = (DdbEqualizer*) g_object_new (object_type, NULL);
    return self;
}


DdbEqualizer* ddb_equalizer_new (void) {
    return ddb_equalizer_construct (DDB_TYPE_EQUALIZER);
}


static GObject * ddb_equalizer_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
    GObject * obj;
    GObjectClass * parent_class;
    DdbEqualizer * self;
    parent_class = G_OBJECT_CLASS (ddb_equalizer_parent_class);
    obj = parent_class->constructor (type, n_construct_properties, construct_properties);
    self = DDB_EQUALIZER (obj);
    return obj;
}


static void ddb_equalizer_class_init (DdbEqualizerClass * klass) {
    ddb_equalizer_parent_class = g_type_class_peek_parent (klass);
    g_type_class_add_private (klass, sizeof (DdbEqualizerPrivate));
    GTK_WIDGET_CLASS (klass)->configure_event = ddb_equalizer_real_configure_event;
    GTK_WIDGET_CLASS (klass)->realize = ddb_equalizer_real_realize;
#if GTK_CHECK_VERSION(3,0,0)
    GTK_WIDGET_CLASS (klass)->draw = ddb_equalizer_real_draw;
#else
    GTK_WIDGET_CLASS (klass)->expose_event = ddb_equalizer_real_expose_event;
#endif
    GTK_WIDGET_CLASS (klass)->button_press_event = ddb_equalizer_real_button_press_event;
    GTK_WIDGET_CLASS (klass)->button_release_event = ddb_equalizer_real_button_release_event;
    GTK_WIDGET_CLASS (klass)->leave_notify_event = ddb_equalizer_real_leave_notify_event;
    GTK_WIDGET_CLASS (klass)->motion_notify_event = ddb_equalizer_real_motion_notify_event;
    G_OBJECT_CLASS (klass)->constructor = ddb_equalizer_constructor;
    G_OBJECT_CLASS (klass)->finalize = ddb_equalizer_finalize;
    g_signal_new ("on_changed", DDB_TYPE_EQUALIZER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void ddb_equalizer_instance_init (DdbEqualizer * self) {
    gdouble* _tmp0_ = NULL;
    GdkCursor* _tmp1_;
    self->priv = DDB_EQUALIZER_GET_PRIVATE (self);
    _tmp0_ = g_new0 (gdouble, DDB_EQUALIZER_BANDS);
    self->priv->values = _tmp0_;
    self->priv->values_length1 = DDB_EQUALIZER_BANDS;
    self->priv->_values_size_ = self->priv->values_length1;
    self->priv->preamp = 0.5;
    self->priv->mouse_y = -1;
    self->priv->curve_hook = FALSE;
    self->priv->preamp_hook = FALSE;
    self->priv->eq_margin_bottom = -1;
    self->priv->eq_margin_left = -1;
    _tmp1_ = gdk_cursor_new (GDK_LEFT_PTR);
    self->priv->pointer_cursor = _tmp1_;
}


static void ddb_equalizer_finalize (GObject* obj) {
    DdbEqualizer * self;
    self = DDB_EQUALIZER (obj);
    self->priv->values = ((void)(g_free (self->priv->values)), NULL);
    _gdk_cursor_unref0 (self->priv->pointer_cursor);
    G_OBJECT_CLASS (ddb_equalizer_parent_class)->finalize (obj);
}


GType ddb_equalizer_get_type (void) {
    static volatile gsize ddb_equalizer_type_id__volatile = 0;
    if (g_once_init_enter ((gsize *)(&ddb_equalizer_type_id__volatile))) {
        static const GTypeInfo g_define_type_info = { sizeof (DdbEqualizerClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) ddb_equalizer_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (DdbEqualizer), 0, (GInstanceInitFunc) ddb_equalizer_instance_init, NULL };
        GType ddb_equalizer_type_id;
        ddb_equalizer_type_id = g_type_register_static (GTK_TYPE_DRAWING_AREA, "DdbEqualizer", &g_define_type_info, 0);
        g_once_init_leave (&ddb_equalizer_type_id__volatile, ddb_equalizer_type_id);
    }
    return ddb_equalizer_type_id__volatile;
}



