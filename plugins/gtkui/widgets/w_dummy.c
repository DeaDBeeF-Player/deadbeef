/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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



#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "../../../shared/parser.h"
#include "../gtkui.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_dummy.h"

// dummy widget
static const char *
w_dummy_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    w_dummy_t *b = (w_dummy_t *)w;
    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s, key, val);
        if (!strcmp (key, "text")) {
            b->text = val[0] ? strdup (val) : NULL;
        }
    }

    return s;
}

static void
w_dummy_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    char save[1000] = "";
    char *pp = save;
    int ss = sizeof (save);
    int n;

    w_dummy_t *b = (w_dummy_t *)w;
    if (b->text) {
        n = snprintf (pp, ss, " text=\"%s\"", b->text);
        ss -= n;
        pp += n;
    }

    strncat (s, save, sz);
}

void
w_dummy_init (ddb_gtkui_widget_t *wbase) {
    w_dummy_t *w = (w_dummy_t *)wbase;
    if (w->label) {
        gtk_widget_destroy (w->label);
        w->label = NULL;
    }
    if (w->text) {
        w->label = gtk_label_new_with_mnemonic (w->text);
        gtk_widget_show (w->label);
        gtk_container_add (GTK_CONTAINER (w->base.widget), w->label);
    }
}

static void
w_dummy_destroy (ddb_gtkui_widget_t *wbase) {
    w_dummy_t *w = (w_dummy_t *)wbase;
    if (w->text) {
        free (w->text);
        w->text = NULL;
    }
}

ddb_gtkui_widget_t *
w_dummy_create (void) {
    w_dummy_t *w = (w_dummy_t *)malloc (sizeof (w_dummy_t));
    memset (w, 0, sizeof (w_dummy_t));
    w->base.widget = gtk_event_box_new ();
    w->base.init = w_dummy_init;
    w->base.destroy = w_dummy_destroy;
    w->base.load = w_dummy_load;
    w->base.save = w_dummy_save;
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
