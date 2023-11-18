/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2023 Oleksiy Yakovenko and other contributors

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

#include <deadbeef/deadbeef.h>
#include <stdlib.h>
#include <string.h>
#include "mlcellrendererpixbuf.h"

extern DB_functions_t *deadbeef;

struct _MlCellRendererPixbuf {
    GtkCellRenderer parent_instance;

    // instance variables for subclass go here
    MlCellRendererPixbufDelegate *delegate;
    char *path;
    GdkPixbuf *pixbuf;
};

G_DEFINE_TYPE (MlCellRendererPixbuf, ml_cell_renderer_pixbuf, GTK_TYPE_CELL_RENDERER)

static void
ml_cell_renderer_pixbuf_init (MlCellRendererPixbuf *self) {
    // initialisation goes here
}

enum {
    PROP_0,
    PROP_PATH,
    PROP_PIXBUF,
};

static void
_finalize (GObject *object) {
    MlCellRendererPixbuf *self = ML_CELL_RENDERER_PIXBUF (object);
    if (self->path) {
        free (self->path);
        self->path = NULL;
    }
    if (self->pixbuf) {
        g_object_unref (self->pixbuf);
        self->pixbuf = NULL;
    }
}

static void
_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *psec) {
    MlCellRendererPixbuf *self = ML_CELL_RENDERER_PIXBUF (object);

    switch (param_id) {
    case PROP_PATH:
        g_value_set_string (value, self->path);
        break;
    case PROP_PIXBUF:
        g_value_set_object (value, self->pixbuf);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
        break;
    }
}

static void
_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec) {
    MlCellRendererPixbuf *cellpixbuf = ML_CELL_RENDERER_PIXBUF (object);

    switch (param_id) {
    case PROP_PATH: {
        const char *str = g_value_get_string (value);
        if (cellpixbuf->path) {
            free (cellpixbuf->path);
            cellpixbuf->path = NULL;
        }
        if (str != NULL) {
            cellpixbuf->path = strdup (str);
        }
        break;
    }
    case PROP_PIXBUF:
        if (cellpixbuf->pixbuf != NULL) {
            g_object_unref (cellpixbuf->pixbuf);
            cellpixbuf->pixbuf = NULL;
        }
        cellpixbuf->pixbuf = g_value_get_object (value);
        if (cellpixbuf->pixbuf != NULL) {
            g_object_ref (cellpixbuf->pixbuf);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static void
_get_size (
    GtkCellRenderer *cell,
    GtkWidget *widget,
    GdkRectangle *cell_area,
    gint *x_offset,
    gint *y_offset,
    gint *width,
    gint *height) {
#else
static void
_get_size (
    GtkCellRenderer *cell,
    GtkWidget *widget,
    const GdkRectangle *cell_area,
    gint *x_offset,
    gint *y_offset,
    gint *width,
    gint *height) {
#endif
    if (width != NULL) {
        *width = ML_CELL_RENDERER_PIXBUF_SIZE;
    }
    if (height != NULL) {
        *height = ML_CELL_RENDERER_PIXBUF_SIZE;
    }
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static void
_render (
    GtkCellRenderer *cell,
    GdkDrawable *window,
    GtkWidget *widget,
    GdkRectangle *background_area,
    GdkRectangle *cell_area,
    GdkRectangle *expose_area,
    GtkCellRendererState flags) {
#else
static void
_render (
    GtkCellRenderer *cell,
    cairo_t *cr,
    GtkWidget *widget,
    const GdkRectangle *background_area,
    const GdkRectangle *cell_area,
    GtkCellRendererState flags) {
#endif
    MlCellRendererPixbuf *self = ML_CELL_RENDERER_PIXBUF (cell);

    GdkRectangle pix_rect;
    pix_rect.x = cell_area->x;
    pix_rect.y = cell_area->y;
    pix_rect.width = ML_CELL_RENDERER_PIXBUF_SIZE;
    pix_rect.height = ML_CELL_RENDERER_PIXBUF_SIZE;

    GdkRectangle draw_rect;
    if (!gdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect)) {
        return;
    }

    GdkPixbuf *pixbuf = self->pixbuf;
    if (pixbuf == NULL) {
        pixbuf = self->delegate->cell_did_became_visible (self->delegate->ctx, self->path);
    }

    if (pixbuf != NULL) {
#if GTK_CHECK_VERSION(3, 0, 0)
        GtkStyleContext *context = gtk_widget_get_style_context (widget);
        gtk_style_context_save (context);

        gtk_style_context_add_class (context, GTK_STYLE_CLASS_IMAGE);
#else
        cairo_t *cr = gdk_cairo_create (window);
#endif

        gdk_cairo_set_source_pixbuf (cr, pixbuf, pix_rect.x, pix_rect.y);
        gdk_cairo_rectangle (cr, &draw_rect);
        cairo_fill (cr);
#if GTK_CHECK_VERSION(3, 0, 0)
        gtk_style_context_restore (context);
#else
        cairo_destroy (cr);
#endif
    }
}

static void
ml_cell_renderer_pixbuf_class_init (MlCellRendererPixbufClass *class) {
    // virtual function overrides go here
    // property and signal definitions go here
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->finalize = _finalize;
    object_class->get_property = _get_property;
    object_class->set_property = _set_property;
    class->parent_class.get_size = _get_size;
    class->parent_class.render = _render;

    g_object_class_install_property (
        object_class,
        PROP_PATH,
        g_param_spec_string ("path", "Track Object", "The path", "", G_PARAM_READWRITE));
    g_object_class_install_property (
        object_class,
        PROP_PIXBUF,
        g_param_spec_object ("pixbuf", "Pixbuf Object", "The pixbuf", GDK_TYPE_PIXBUF, G_PARAM_READWRITE));
}

MlCellRendererPixbuf *
ml_cell_renderer_pixbuf_new (MlCellRendererPixbufDelegate *delegate) {
    MlCellRendererPixbuf *self = g_object_new (ML_TYPE_CELL_RENDERER_PIXBUF, NULL);
    self->delegate = delegate;
    return self;
}
