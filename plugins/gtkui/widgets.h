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

#ifndef __WIDGETS_H
#define __WIDGETS_H

#include "gtkui_api.h"

#define DDB_GTKUI_CONF_LAYOUT "gtkui.layout.1.9.0"

struct json_t;

void
w_init (void);

void
w_free (void);

void
w_save (void);

ddb_gtkui_widget_t *
w_get_rootwidget (void);

void
w_set_design_mode (int active);

int
w_get_design_mode (void);

void
w_reg_widget (const char *title, uint32_t flags, ddb_gtkui_widget_t *(*create_func) (void), ...);

void
w_unreg_widget (const char *type);

void
w_override_signals (GtkWidget *widget, gpointer user_data);

int
w_is_registered (const char *type);

ddb_gtkui_widget_t *
w_create (const char *type);

uint32_t
w_get_type_flags(const char *type);

int
w_create_from_json (struct json_t *json, ddb_gtkui_widget_t **parent);

void
w_destroy (ddb_gtkui_widget_t *w);

void
w_append (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

void
w_replace (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *from, ddb_gtkui_widget_t *to);

void
w_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

// returns actual container widget of a composite widget
// e.g. HBox is contained in EventBox, this function will return the HBox
GtkWidget *
w_get_container (ddb_gtkui_widget_t *w);

ddb_gtkui_widget_t *
w_hsplitter_create (void);

ddb_gtkui_widget_t *
w_vsplitter_create (void);

ddb_gtkui_widget_t *
w_box_create (void);

ddb_gtkui_widget_t *
w_dummy_create (void);

ddb_gtkui_widget_t *
w_tabstrip_create (void);

ddb_gtkui_widget_t *
w_tabbed_playlist_create (void);

ddb_gtkui_widget_t *
w_playlist_create (void);

ddb_gtkui_widget_t *
w_placeholder_create (void);

ddb_gtkui_widget_t *
w_tabs_create (void);

ddb_gtkui_widget_t *
w_scope_create (void);

ddb_gtkui_widget_t *
w_spectrum_create (void);

ddb_gtkui_widget_t *
w_hbox_create (void);

ddb_gtkui_widget_t *
w_vbox_create (void);

ddb_gtkui_widget_t *
w_button_create (void);

ddb_gtkui_widget_t *
w_seekbar_create (void);

ddb_gtkui_widget_t *
w_playtb_create (void);

ddb_gtkui_widget_t *
w_volumebar_create (void);

ddb_gtkui_widget_t *
w_ctvoices_create (void);

ddb_gtkui_widget_t *
w_logviewer_create (void);

gboolean
w_logviewer_is_present(void);

ddb_gtkui_widget_t *
w_create_from_conf (const char *key);

int
w_save_to_conf (const char *key, ddb_gtkui_widget_t *val);

#endif
