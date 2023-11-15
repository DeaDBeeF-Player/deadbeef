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
#include "mlcellrendererpixbuf.h"

extern DB_functions_t *deadbeef;

struct _MlCellRendererPixbuf {
    GtkCellRenderer parent_instance;

    // instance variables for subclass go here
    ddb_playItem_t *track;
};

G_DEFINE_TYPE (MlCellRendererPixbuf, ml_cell_renderer_pixbuf, GTK_TYPE_CELL_RENDERER)

static void
ml_cell_renderer_pixbuf_init (MlCellRendererPixbuf *self) {
    // initialisation goes here
}

enum {
    PROP_0,
    PROP_TRACK,
};

static void
_get_property (
    GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *psec) {
    MlCellRendererPixbuf *self = ML_CELL_RENDERER_PIXBUF (object);

    switch (param_id) {
    case PROP_TRACK:
        g_value_set_pointer (value, self->track);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
        break;
    }
}

static void
_set_property (
    GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec) {
    MlCellRendererPixbuf *cellpixbuf = ML_CELL_RENDERER_PIXBUF (object);

    switch (param_id) {
    case PROP_TRACK:
        cellpixbuf->track = (ddb_playItem_t *)g_value_get_pointer (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
_render (
    GtkCellRenderer *cell,
    cairo_t *cr,
    GtkWidget *widget,
    const GdkRectangle *background_area,
    const GdkRectangle *cell_area,
    GtkCellRendererState flags) {
    MlCellRendererPixbuf *cellpixbuf = ML_CELL_RENDERER_PIXBUF (cell);

    // get associated track
    // ask cover manager for album art
    // render, or wait for callback
    ddb_playItem_t *track = cellpixbuf->track;

    const char *title = "<NULL>";
    if (track != NULL) {
        title = deadbeef->pl_find_meta (track, "title") ?: "<?>";
    }
    //    printf ("Render cell: %p %s\n", track, title);
}

static void
ml_cell_renderer_pixbuf_class_init (MlCellRendererPixbufClass *class) {
    // virtual function overrides go here
    // property and signal definitions go here
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    class->parent_class.render = _render;
    object_class->get_property = _get_property;
    object_class->set_property = _set_property;

    g_object_class_install_property (object_class, PROP_TRACK, g_param_spec_pointer ("track", "Track Object", "The track", G_PARAM_READWRITE));
}

MlCellRendererPixbuf *
ml_cell_renderer_pixbuf_new (void) {
    return g_object_new (ML_TYPE_CELL_RENDERER_PIXBUF, NULL);
}
