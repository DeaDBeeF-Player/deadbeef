/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2020 Oleksiy Yakovenko and other contributors

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

#ifndef __DDBSEEKBAR_H__
#define __DDBSEEKBAR_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DDB_TYPE_SEEKBAR (ddb_seekbar_get_type ())
#define DDB_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_SEEKBAR, DdbSeekbar))
#define DDB_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_SEEKBAR, DdbSeekbarClass))
#define DDB_IS_SEEKBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_SEEKBAR))
#define DDB_IS_SEEKBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_SEEKBAR))
#define DDB_SEEKBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_SEEKBAR, DdbSeekbarClass))

typedef struct _DdbSeekbar DdbSeekbar;
typedef struct _DdbSeekbarClass DdbSeekbarClass;
typedef struct _DdbSeekbarPrivate DdbSeekbarPrivate;

struct _DdbSeekbar {
    GtkWidget parent_instance;
    DdbSeekbarPrivate *priv;
};

struct _DdbSeekbarClass {
    GtkWidgetClass parent_class;
};

GType
ddb_seekbar_get_type (void);

GtkWidget *
ddb_seekbar_new (void);

void
ddb_seekbar_init_signals (DdbSeekbar *sb, GtkWidget *evbox);

G_END_DECLS

#endif
