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

#ifndef __DDBTABSTRIP_H
#define __DDBTABSTRIP_H

#include <gtk/gtk.h>
#include "drawing.h"

G_BEGIN_DECLS

#define DDB_TYPE_TABSTRIP (ddb_tabstrip_get_type ())
#define DDB_TABSTRIP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DDB_TYPE_TABSTRIP, DdbTabStrip))
#define DDB_TABSTRIP_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), DDB_TYPE_TABSTRIP, DdbTabStripClass))
#define DDB_IS_TABSTRIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_TABSTRIP))
#define DDB_IS_TABSTRIP_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), DDB_TYPE_TABSTRIP))
#define DDB_TABSTRIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_TABSTRIP, DdbTabStripClass))

typedef struct _DdbTabStrip DdbTabStrip;
typedef struct _DdbTabStripClass DdbTabStripClass;

typedef void * DdbTabStripIter;
typedef void * DdbTabStripColIter;

struct _DdbTabStrip {
    GtkWidget parent;
    int hscrollpos;
    int dragging;
    int prepare;
    int dragpt[2];
    int prev_x;
    int movepos;
    guint scroll_timer;
    int scroll_direction;
    drawctx_t drawctx;
    int calculated_height;
    int row_height;
    int calculated_arrow_width;
    int add_playlistbtn_hover;
};

struct _DdbTabStripClass {
  GtkWidgetClass parent_class;
};

GType ddb_tabstrip_get_type(void) G_GNUC_CONST;
GtkWidget * ddb_tabstrip_new(void);
void ddb_tabstrip_refresh (DdbTabStrip *ts);

G_END_DECLS

#endif // __DDBTABSTRIP_H
