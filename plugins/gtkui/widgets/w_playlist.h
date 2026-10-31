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

#ifndef w_playlist_h
#define w_playlist_h

#include <gtk/gtk.h>
#include "../gtkui_api.h"
#include "../playlist/ddblistview.h"
#include "../playlist/playlistcontroller.h"

typedef struct {
    ddb_gtkui_widget_t base;
    playlist_controller_t *controller;
    DdbListview *listview;
    int hideheaders;
    int width;
} w_playlist_t;

ddb_gtkui_widget_t *
w_playlist_create (void);

// The functions below are shared with the tabbed playlist widget
int
w_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);

const char *
w_playlist_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s);

void
w_playlist_save (struct ddb_gtkui_widget_s *w, char *s, int sz);

void
w_playlist_init (ddb_gtkui_widget_t *base);

void
w_playlist_destroy (ddb_gtkui_widget_t *base);

void
w_playlist_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu);

#endif
