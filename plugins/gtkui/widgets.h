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

// Version of the layout format, major.minor.point. Used for the config key, and in the .ddblayout files.
#define DDB_GTKUI_LAYOUT_VERSION "1.9.0"
#define DDB_GTKUI_CONF_LAYOUT "gtkui.layout." DDB_GTKUI_LAYOUT_VERSION

struct json_t;

void
w_init (void);

void
w_free (void);

// Saves the layout to the config. If the layout has changed since the previous save,
// registers an undo operation named "Change Layout".
void
w_save (void);

// Same as w_save, with the name of the undo operation.
void
w_save_with_action (const char *action_name);

// Remembers the current layout as the starting point for undo. To be called after the layout is loaded.
void
w_reset_undo_snapshot (void);


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

// utility code for parsing keyvalues
#define get_keyvalue(s, key, val)            \
    {                                        \
        s = gettoken_ext (s, key, "={}();"); \
        if (!s) {                            \
            return NULL;                     \
        }                                    \
        if (s && !strcmp (key, "{")) {       \
            break;                           \
        }                                    \
        s = gettoken_ext (s, val, "={}();"); \
        if (!s || strcmp (val, "=")) {       \
            return NULL;                     \
        }                                    \
        s = gettoken_ext (s, val, "={}();"); \
        if (!s) {                            \
            return NULL;                     \
        }                                    \
    }

// Helpers for the widget implementations, see widgets/

void
w_container_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

void
w_container_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

gboolean
w_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data);

gboolean
w_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data);

// Returns the widget, and its children, as a new json object
struct json_t *
w_save_widget_to_json (ddb_gtkui_widget_t *w);

// Returns the user visible title of the widget type, or NULL if the type is not exposed to the user
const char *
w_get_title (ddb_gtkui_widget_t *w);

void
w_add_menu_separator (GtkWidget *menu);

// A row of the visualization preferences dialog: "Use custom <color>" checkbox and a color button.
// The dialog writes directly into `use_custom` and `color`, so the widget updates live.
typedef struct {
    const char *label;
    gboolean *use_custom;
    gboolean *color_set; // whether `color` holds a color chosen by the user
    GdkColor *color;
    GdkColor default_color; // shown while there is no custom color
} w_vis_color_row_t;

// Shows the modal preferences dialog. Returns TRUE if anything was changed.
gboolean
w_vis_color_preferences_dialog (const char *title, w_vis_color_row_t *rows, int row_count);

// Colors are stored as "r g b a" with components in 0..1, same as in the macOS UI.
// The returned string must be freed by the caller.
char *
w_vis_color_to_string (const GdkColor *color);

gboolean
w_vis_color_from_string (const char *str, GdkColor *color);

ddb_gtkui_widget_t *
w_create_from_conf (const char *key);

int
w_save_to_conf (const char *key, ddb_gtkui_widget_t *val);

// Returns the current layout as a new json object, in the same format as saved to DDB_GTKUI_CONF_LAYOUT.
struct json_t *
w_save_layout_to_json (void);

// Replaces the current layout, and saves it to the config (as undoable "Apply Layout"). Returns 0 on success.
int
w_load_layout_from_json (struct json_t *layout);

#endif
