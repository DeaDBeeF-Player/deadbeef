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

#ifndef __DDBVOLUMEBAR_H
#define __DDBVOLUMEBAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DDB_TYPE_VOLUMEBAR (ddb_volumebar_get_type ())
#define DDB_VOLUMEBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DDB_TYPE_VOLUMEBAR, DdbVolumeBar))
#define DDB_VOLUMEBAR_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), DDB_TYPE_VOLUMEBAR, DdbVolumeBarClass))
#define DDB_IS_VOLUMEBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_VOLUMEBAR))
#define DDB_IS_VOLUMEBAR_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), DDB_TYPE_VOLUMEBAR))
#define DDB_VOLUMEBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_VOLUMEBAR, DdbVolumeBarClass))

typedef struct _DdbVolumeBarPrivate DdbVolumeBarPrivate;
typedef struct _DdbVolumeBar DdbVolumeBar;
typedef struct _DdbVolumeBarClass DdbVolumeBarClass;

typedef void * DdbVolumeBarIter;
typedef void * DdbVolumeBarColIter;

typedef enum {
    DDB_VOLUMEBAR_SCALE_DB,
    DDB_VOLUMEBAR_SCALE_LINEAR,
    DDB_VOLUMEBAR_SCALE_CUBIC,
    DDB_VOLUMEBAR_SCALE_COUNT
} DdbVolumeBarScale;

struct _DdbVolumeBar {
    GtkWidget parent;
    int show_dbs;
    DdbVolumeBarPrivate *priv;
};

struct _DdbVolumeBarClass {
  GtkWidgetClass parent_class;
};

GType ddb_volumebar_get_type(void) G_GNUC_CONST;
GtkWidget * ddb_volumebar_new(void);

void
ddb_volumebar_init_signals (DdbVolumeBar *vb, GtkWidget *evbox);

DdbVolumeBarScale
ddb_volumebar_get_scale (const DdbVolumeBar *volumebar);

void
ddb_volumebar_set_scale (DdbVolumeBar *volumebar, DdbVolumeBarScale scale);

void
ddb_volumebar_update(DdbVolumeBar *volumebar);

G_END_DECLS

#endif // __DDBVOLUMEBAR_H
