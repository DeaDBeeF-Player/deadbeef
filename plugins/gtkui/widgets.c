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

#include <gtk/gtk.h>
#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include "gtkui.h"
#include "support.h"
#include "undointegration.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "widgets.h"
#include "widgets/w_dummy.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt, ...)

// The layout as of the last save: this is what the next undo operation restores.
static char *_undo_snapshot;

// Set while the layout is being restored by undo/redo: the opposite operation is then processed by the undo stack.
static int _undo_performing;

typedef struct w_creator_s {
    const char *type;
    const char *title; // set to NULL to avoid exposing this widget type to user
    uint32_t flags;
    int compat; // when this is set to 1 -- it's a backwards compatibility creator, and must be skipped in GUI
    ddb_gtkui_widget_t *(*create_func) (void);
    struct w_creator_s *next;
} w_creator_t;

static w_creator_t *w_creators;

int design_mode;
static ddb_gtkui_widget_t *rootwidget;

static const char associated_widget_data_id[] = "uiwidget";

//// common functions

void
w_init (void) {
    rootwidget = w_create ("box");
}

void
w_free (void) {
    w_creator_t *next = NULL;
    for (w_creator_t *cr = w_creators; cr; cr = next) {
        next = cr->next;
        free (cr);
    }
    w_creators = NULL;

    free (_undo_snapshot);
    _undo_snapshot = NULL;

    if (rootwidget != NULL) {
        w_remove (NULL, rootwidget);
        w_destroy (rootwidget);
        rootwidget = NULL;
    }
}

ddb_gtkui_widget_t *
w_get_rootwidget (void) {
    return rootwidget;
}

void
w_set_design_mode (int active) {
    design_mode = active;
    gtk_widget_queue_draw (mainwin);
}

int
w_get_design_mode (void) {
    return design_mode;
}

static gboolean
w_init_cb (void *data) {
    ddb_gtkui_widget_t *w = data;
    if (w->init) {
        w->init (w);
    }
    return FALSE;
}

void
w_append (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    child->parent = cont;
    if (!cont->children) {
        cont->children = child;
    }
    else {
        for (ddb_gtkui_widget_t *c = cont->children; c; c = c->next) {
            if (!c->next) {
                c->next = child;
                break;
            }
        }
    }

    if (cont->append) {
        cont->append (cont, child);
    }
    if (child->init) {
        child->init (child);
    }
}

void
w_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    // recurse
    while (child->children) {
        ddb_gtkui_widget_t *c = child->children;
        w_remove (child, c);
        w_destroy (c);
    }

    if (cont) {
        if (cont->remove) {
            cont->remove (cont, child);
        }
        ddb_gtkui_widget_t *prev = NULL;
        for (ddb_gtkui_widget_t *c = cont->children; c; c = c->next) {
            if (c == child) {
                if (prev) {
                    prev->next = c->next;
                }
                else {
                    cont->children = c->next;
                }
                break;
            }
            prev = c;
        }
    }
    child->parent = NULL;
    child->widget = NULL;
}

GtkWidget *
w_get_container (ddb_gtkui_widget_t *w) {
    if (w->get_container) {
        return w->get_container (w);
    }
    return w->widget;
}

void
w_replace (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *from, ddb_gtkui_widget_t *to) {
    if (w->replace) {
        w->replace (w, from, to);
        if (to->init) {
            g_idle_add (w_init_cb, to);
        }
    }
    else {
        w_remove (w, from);
        w_destroy (from);
        w_append (w, to);
        // we don't call init here, because w_append does it automatically
    }
}

const char *
w_get_title (ddb_gtkui_widget_t *w) {
    for (w_creator_t *cr = w_creators; cr; cr = cr->next) {
        if (cr->type == w->type && cr->title) {
            return cr->title;
        }
    }
    return NULL;
}

// unknown widget wrapper
typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
    char *data; // json string of the whole node
} w_unknown_t;

void
w_unknown_destroy (ddb_gtkui_widget_t *_w) {
    w_unknown_t *w = (w_unknown_t *)_w;
    free (w->data);
}

static gboolean
unknown_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    ddb_gtkui_widget_t *w = user_data;
    cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);

    cairo_set_font_size (cr, 16);

    cairo_move_to (cr, 20, 30);

    char s[1000];
    snprintf (s, sizeof (s), _ ("Widget \"%s\" is not available"), w->type);

    cairo_show_text (cr, s);
    return TRUE;
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static gboolean
unknown_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = unknown_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}
#endif

ddb_gtkui_widget_t *
w_unknown_create (const char *type, const char *node_str) {
    w_unknown_t *w = malloc (sizeof (w_unknown_t));
    memset (w, 0, sizeof (w_unknown_t));
    w->base.type = "unknown";
    w->base.destroy = w_unknown_destroy;
    w->data = strdup (node_str);

    w->base.widget = gtk_event_box_new ();
    w->drawarea = gtk_drawing_area_new ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect_after ((gpointer)w->drawarea, "expose_event", G_CALLBACK (unknown_expose_event), w);
#else
    g_signal_connect_after ((gpointer)w->drawarea, "draw", G_CALLBACK (unknown_draw), w);
#endif
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

int
w_create_from_json (json_t *node, ddb_gtkui_widget_t **parent) {
    json_t *node_type = NULL;
    json_t *node_legacy_params = NULL;
    json_t *node_settings = NULL;
    json_t *node_children = NULL;

    int err = -1;

    node_type = json_object_get (node, "type");
    if (node_type == NULL || !json_is_string (node_type)) {
        goto error;
    }

    node_legacy_params = json_object_get (node, "legacy_params");
    if (node_legacy_params != NULL && !json_is_string (node_legacy_params)) {
        goto error;
    }

    node_settings = json_object_get (node, "settings");
    if (node_settings != NULL && !json_is_object (node_settings)) {
        goto error;
    }

    node_children = json_object_get (node, "children");
    if (node_children != NULL && !json_is_array (node_children)) {
        goto error;
    }

    const char *type = json_string_value (node_type);
    const char *legacy_params = node_legacy_params ? json_string_value (node_legacy_params) : "";

    ddb_gtkui_widget_t *w = w_create (type);
    if (!w) {
        char *node_str = json_dumps (node, JSON_COMPACT);
        w = w_unknown_create (type, node_str);
        free (node_str);
    }
    else {
        // nuke all default children
        while (w->children) {
            ddb_gtkui_widget_t *c = w->children;
            w_remove (w, w->children);
            w_destroy (c);
        }

        uint32_t flags = w_get_type_flags (type);

        if ((flags & DDB_WF_SUPPORTS_EXTENDED_API) && node_settings != NULL) {
            ddb_gtkui_widget_extended_api_t *api = (ddb_gtkui_widget_extended_api_t *)(w + 1);
            if (api->_size >= sizeof (ddb_gtkui_widget_extended_api_t)) {
                size_t count = json_object_size (node_settings);
                if (count != 0) {
                    char const **keyvalues = calloc (count * 2 + 1, sizeof (char *));

                    const char *key;
                    json_t *value;
                    int index = 0;
                    json_object_foreach (node_settings, key, value) {
                        keyvalues[index * 2 + 0] = key;
                        keyvalues[index * 2 + 1] = json_string_value (value);
                        index += 1;
                    }

                    api->deserialize_from_keyvalues (w, keyvalues);

                    free (keyvalues);
                }
            }
            else {
                trace ("widget %s doesn't has unsupported extended api _size=%lld\n", api->_size);
            }
        }
        else if (w->load != NULL && legacy_params != NULL) {
            // load from legacy params
            w->load (w, type, legacy_params);
        }

        size_t children_count = json_array_size (node_children);
        for (int i = 0; i < children_count; i++) {
            json_t *child = json_array_get (node_children, i);

            if (child == NULL || !json_is_object (child)) {
                goto error;
            }

            int res = w_create_from_json (child, &w);
            if (res < 0) {
                goto error;
            }
        }
    }

    if (*parent) {
        w_append (*parent, w);
    }
    else {
        *parent = w;
    }

    err = 0;

error:

    return err;
}

static ddb_gtkui_widget_t *current_widget;
static int hidden = 0;

gboolean
w_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    if (hidden && user_data == current_widget) {
        GtkAllocation allocation;
        gtk_widget_get_allocation (widget, &allocation);
        cairo_set_source_rgb (cr, 0.17, 0, 0.83);

        if (!gtk_widget_get_has_window (widget)) {
#if GTK_CHECK_VERSION(3, 0, 0)
            cairo_translate (cr, -allocation.x, -allocation.y);
#endif
            cairo_reset_clip (cr);
            cairo_rectangle (cr, allocation.x, allocation.y, allocation.width, allocation.height);
        }
        else {
            cairo_reset_clip (cr);
            cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
        }
        cairo_fill (cr);
    }
    return FALSE;
}

gboolean
w_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = w_draw_event (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

static char paste_buffer[20000];

json_t *
w_save_widget_to_json (ddb_gtkui_widget_t *w) {
    json_t *node = NULL;
    if (!strcmp (w->type, "unknown")) {
        w_unknown_t *unk = (w_unknown_t *)w;
        node = json_loads (unk->data, 0, NULL);
    }
    else {
        node = json_object ();

        json_object_set (node, "type", json_string (w->type));

        uint32_t flags = w_get_type_flags (w->type);

        if (flags & DDB_WF_SUPPORTS_EXTENDED_API) {
            ddb_gtkui_widget_extended_api_t *api = (ddb_gtkui_widget_extended_api_t *)(w + 1);
            if (api->_size >= sizeof (ddb_gtkui_widget_extended_api_t)) {
                char const **keyvalues = api->serialize_to_keyvalues (w);

                if (keyvalues != NULL) {
                    json_t *settings = json_object ();
                    for (int i = 0; keyvalues[i]; i += 2) {
                        json_t *value = json_string (keyvalues[i + 1]);
                        json_object_set (settings, keyvalues[i], value);
                        json_decref (value);
                    }
                    json_object_set (node, "settings", settings);
                    json_decref (settings);
                }
            }
            else {
                trace ("widget %s doesn't has unsupported extended api _size=%lld\n", api->_size);
            }
        }
        else if (w->save) {
            char params[1000] = "";
            w->save (w, params, sizeof (params));
            json_object_set (node, "legacy_params", json_string (params));
        }
    }

    if (w->children != NULL) {
        json_t *children = json_array ();
        for (ddb_gtkui_widget_t *c = w->children; c; c = c->next) {
            json_t *child = w_save_widget_to_json (c);
            json_array_append (children, child);
        }
        json_object_set (node, "children", children);
    }

    return node;
}

ddb_gtkui_widget_t *
w_create_from_conf (const char *key) {
    char *layout_str = NULL;
    {
        deadbeef->conf_lock ();
        const char *layout_str_conf = deadbeef->conf_get_str_fast (key, NULL);
        if (layout_str_conf != NULL) {
            layout_str = strdup (layout_str_conf);
        }
        deadbeef->conf_unlock ();
    }
    if (layout_str == NULL) {
        return NULL;
    }

    ddb_gtkui_widget_t *w = NULL;
    {
        json_t *layout = json_loads (layout_str, 0, NULL);
        if (layout != NULL) {
            w_create_from_json (layout, &w);
            json_delete (layout);
        }
    }
    free (layout_str);

    return w;
}

int
w_save_to_conf (const char *key, ddb_gtkui_widget_t *val) {
    json_t *layout = w_save_widget_to_json (val);
    if (layout == NULL) {
        return -1;
    }

    char *layout_str = json_dumps (layout, JSON_COMPACT);
    json_delete (layout);

    if (layout_str == NULL) {
        return -1;
    }

    deadbeef->conf_set_str (key, layout_str);
    free (layout_str);

    return 0;
}

static char *
_dump_current_layout (void) {
    json_t *layout = w_save_layout_to_json ();
    if (layout == NULL) {
        return NULL;
    }
    char *str = json_dumps (layout, JSON_COMPACT);
    json_delete (layout);
    return str;
}

void
w_reset_undo_snapshot (void) {
    free (_undo_snapshot);
    _undo_snapshot = _dump_current_layout ();
}

static int
_restore_layout_from_string (const char *layout_str, const char *action_name);

static void
_layout_undo_perform (void *ctx) {
    _undo_performing = 1;
    _restore_layout_from_string (ctx, NULL);
    _undo_performing = 0;
}

void
w_save_with_action (const char *action_name) {
    if (rootwidget == NULL) {
        return;
    }
    w_save_to_conf (DDB_GTKUI_CONF_LAYOUT, rootwidget->children);
    deadbeef->conf_save ();

    char *current = _dump_current_layout ();
    if (current == NULL) {
        return;
    }
    if (_undo_snapshot != NULL && strcmp (current, _undo_snapshot) && ddb_undo != NULL) {
        // the undo operation holds the previous layout
        ddb_undo->append_custom_operation (strdup (_undo_snapshot), _layout_undo_perform, free);
        if (!_undo_performing) {
            ddb_undo->set_action_name (action_name != NULL ? action_name : _("Change Layout"));
            deadbeef->undo_process ();
        }
    }
    free (_undo_snapshot);
    _undo_snapshot = current;
}

void
w_save (void) {
    w_save_with_action (NULL);
}

json_t *
w_save_layout_to_json (void) {
    if (rootwidget == NULL || rootwidget->children == NULL) {
        return NULL;
    }
    return w_save_widget_to_json (rootwidget->children);
}

static int
_load_layout (json_t *layout, const char *action_name) {
    if (rootwidget == NULL) {
        return -1;
    }

    ddb_gtkui_widget_t *w = NULL;
    w_create_from_json (layout, &w);
    if (w == NULL) {
        return -1;
    }

    if (rootwidget->children != NULL) {
        w_replace (rootwidget, rootwidget->children, w);
    }
    else {
        w_append (rootwidget, w);
    }

    w_save_with_action (action_name);
    return 0;
}

int
w_load_layout_from_json (json_t *layout) {
    return _load_layout (layout, _("Apply Layout"));
}

static int
_restore_layout_from_string (const char *layout_str, const char *action_name) {
    json_t *layout = json_loads (layout_str, 0, NULL);
    if (layout == NULL) {
        return -1;
    }
    int res = _load_layout (layout, action_name);
    json_delete (layout);
    return res;
}

static void
on_replace_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *ui_widget = g_object_get_data (G_OBJECT (menuitem), associated_widget_data_id);
    for (w_creator_t *cr = w_creators; cr; cr = cr->next) {
        if (cr->type == user_data) {
            // hack for single-instance
            // first replace with a placeholder, so that the original widget is destroyed
            // then do the real replacement
            ddb_gtkui_widget_t *w = w_create ("placeholder");
            w_replace (ui_widget->parent, ui_widget, w);
            ui_widget = w;
            w = w_create (user_data);
            w_replace (ui_widget->parent, ui_widget, w);
            ui_widget = w;
        }
    }
    w_save_with_action (_("Replace Widget"));
}

static void
on_delete_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *ui_widget = user_data;
    ddb_gtkui_widget_t *parent = ui_widget->parent;
    if (!strcmp (ui_widget->type, "placeholder")) {
        return;
    }
    if (parent->replace) {
        parent->replace (parent, ui_widget, w_create ("placeholder"));
    }
    else {
        w_remove (parent, ui_widget);
        w_destroy (ui_widget);
        ui_widget = w_create ("placeholder");
        w_append (parent, ui_widget);
    }
    w_save_with_action (_("Delete Widget"));
}

static void
on_cut_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *ui_widget = user_data;
    ddb_gtkui_widget_t *parent = ui_widget->parent;
    if (!strcmp (ui_widget->type, "placeholder")) {
        return;
    }
    // save hierarchy to string
    paste_buffer[0] = 0;
    json_t *layout = w_save_widget_to_json (ui_widget);
    char *layout_str = json_dumps (layout, JSON_COMPACT);
    if (strlen (layout_str) < sizeof (paste_buffer)) {
        strcpy (paste_buffer, layout_str);
    }
    free (layout_str);
    json_delete (layout);

    if (parent->replace) {
        parent->replace (parent, ui_widget, w_create ("placeholder"));
    }
    else {
        w_remove (parent, ui_widget);
        w_destroy (ui_widget);
        ui_widget = w_create ("placeholder");
        w_append (parent, ui_widget);
    }
    w_save_with_action (_("Cut Widget"));
}

static void
on_copy_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *ui_widget = user_data;
    if (!strcmp (ui_widget->type, "placeholder")) {
        return;
    }
    // save hierarchy to string
    paste_buffer[0] = 0;
    paste_buffer[0] = 0;
    json_t *layout = w_save_widget_to_json (ui_widget);
    char *layout_str = json_dumps (layout, JSON_COMPACT);
    if (strlen (layout_str) < sizeof (paste_buffer)) {
        strcpy (paste_buffer, layout_str);
    }
    free (layout_str);
    json_delete (layout);
}

static void
on_paste_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *ui_widget = user_data;
    ddb_gtkui_widget_t *parent = ui_widget->parent;
    if (!paste_buffer[0]) {
        return;
    }

    ddb_gtkui_widget_t *w = w_create ("placeholder");
    w_replace (parent, ui_widget, w);
    ui_widget = w;

    w = NULL;

    json_t *layout = json_loads (paste_buffer, 0, NULL);
    if (layout != NULL) {
        w_create_from_json (layout, &w);
        w_replace (parent, ui_widget, w);
        w_save_with_action (_("Paste Widget"));
        ui_widget = w;
        json_delete (layout);
    }
}

void
hide_widget (GtkWidget *widget, gpointer data) {
    if (data) {
        GtkAllocation *a = data;
        gtk_widget_get_allocation (widget, a);
    }
    gtk_widget_hide (widget);
}

void
show_widget (GtkWidget *widget, gpointer data) {
    gtk_widget_show (widget);
}

static GtkRequisition prev_req;

void
w_menu_deactivate (GtkMenuShell *menushell, gpointer user_data) {
    hidden = 0;
    ddb_gtkui_widget_t *w = user_data;
    if (GTK_IS_CONTAINER (w->widget)) {
        gtk_container_foreach (GTK_CONTAINER (w->widget), show_widget, NULL);
        gtk_widget_set_size_request (w->widget, prev_req.width, prev_req.height);
    }
    gtk_widget_set_app_paintable (w->widget, FALSE);
    gtk_widget_queue_draw (w->widget);
}

void
w_add_menu_separator (GtkWidget *menu) {
    GtkWidget *separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);
}

static GtkWidget *
create_widget_menu (ddb_gtkui_widget_t *ui_widget) {
    GtkWidget *menu;
    GtkWidget *submenu;
    GtkWidget *item;
    menu = gtk_menu_new ();

    const char *widget_title = w_get_title (ui_widget);
    if (widget_title) {
        // Add title of widget at the top of the menu
        item = gtk_menu_item_new_with_mnemonic (widget_title);
        gtk_widget_show (item);
        gtk_widget_set_sensitive (item, FALSE);
        gtk_container_add (GTK_CONTAINER (menu), item);

        w_add_menu_separator (menu);
    }

    if (strcmp (ui_widget->type, "placeholder")) {
        item = gtk_menu_item_new_with_mnemonic (_ ("Replace with..."));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }
    else {
        item = gtk_menu_item_new_with_mnemonic (_ ("Insert..."));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }

    submenu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

    for (w_creator_t *cr = w_creators; cr; cr = cr->next) {
        if (cr->title) {
            item = gtk_menu_item_new_with_mnemonic (cr->title);
            gtk_widget_show (item);
            gtk_container_add (GTK_CONTAINER (submenu), item);
            g_object_set_data (G_OBJECT (item), associated_widget_data_id, ui_widget);
            g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_replace_activate), (void *)cr->type);
        }
    }

    if (strcmp (ui_widget->type, "placeholder")) {
        item = gtk_menu_item_new_with_mnemonic (_ ("Delete"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_delete_activate), ui_widget);

        item = gtk_menu_item_new_with_mnemonic (_ ("Cut"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_cut_activate), ui_widget);

        item = gtk_menu_item_new_with_mnemonic (_ ("Copy"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_copy_activate), ui_widget);
    }

    item = gtk_menu_item_new_with_mnemonic (_ ("Paste"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_paste_activate), ui_widget);

    if (ui_widget->initmenu) {
        w_add_menu_separator (menu);
        ui_widget->initmenu (ui_widget, menu);
    }
    if (ui_widget->parent && ui_widget->parent->initchildmenu) {
        w_add_menu_separator (menu);
        ui_widget->parent->initchildmenu (ui_widget, menu);
    }

    return menu;
}

gboolean
w_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (!design_mode || !TEST_RIGHT_CLICK (event)) {
        return FALSE;
    }

    current_widget = user_data;
    widget = current_widget->widget;
    hidden = 1;

    if (GTK_IS_CONTAINER (widget)) {
        // hide all children
#if !GTK_CHECK_VERSION(3, 0, 0)
        gtk_widget_size_request (widget, &prev_req);
#else
        gtk_widget_get_preferred_size (widget, NULL, &prev_req);
#endif
        gtk_container_foreach (GTK_CONTAINER (widget), hide_widget, NULL);
        gtk_widget_set_size_request (widget, prev_req.width, prev_req.height);
    }

    gtk_widget_set_app_paintable (widget, TRUE);
    gtk_widget_queue_draw (((ddb_gtkui_widget_t *)user_data)->widget);

    GtkWidget *menu;
    menu = create_widget_menu (current_widget);

    if (current_widget->parent && strcmp (current_widget->parent->type, "box")) {
        GtkWidget *item, *submenu;
        w_add_menu_separator (menu);
        item = gtk_menu_item_new_with_mnemonic (_ ("Parent"));
        gtk_widget_show (item);
        submenu = create_widget_menu (current_widget->parent);
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }

    g_signal_connect ((gpointer)menu, "deactivate", G_CALLBACK (w_menu_deactivate), user_data);
    gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
    return TRUE;
}

void
w_override_signals (GtkWidget *widget, gpointer user_data) {
    g_signal_connect ((gpointer)widget, "button_press_event", G_CALLBACK (w_button_press_event), user_data);
#if !GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect ((gpointer)widget, "expose_event", G_CALLBACK (w_expose_event), user_data);
#else
    g_signal_connect ((gpointer)widget, "draw", G_CALLBACK (w_draw_event), user_data);
#endif
    if (GTK_IS_CONTAINER (widget)) {
        gtk_container_forall (GTK_CONTAINER (widget), w_override_signals, user_data);
    }
}

void
w_reg_widget (const char *title, uint32_t flags, ddb_gtkui_widget_t *(*create_func) (void), ...) {
    int compat = 0;

    va_list vl;
    va_start (vl, create_func);
    for (;;) {
        const char *type = va_arg (vl, const char *);
        if (!type) {
            break;
        }
        w_creator_t *c;
        for (c = w_creators; c; c = c->next) {
            if (!strcmp (c->type, type)) {
                fprintf (stderr, "gtkui w_reg_widget: widget type %s already registered\n", type);
                return;
            }
        }
        c = malloc (sizeof (w_creator_t));
        memset (c, 0, sizeof (w_creator_t));
        c->type = type;
        c->title = title;
        c->flags = flags;
        c->compat = compat;
        c->create_func = create_func;
        c->next = w_creators;
        w_creators = c;
        compat = 1;
    }
    va_end (vl);
}

void
w_unreg_widget (const char *type) {
    w_creator_t *c, *prev = NULL;
    for (c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            if (prev) {
                prev->next = c->next;
            }
            else {
                w_creators = c->next;
            }
            free (c);
            return;
        }
        prev = c;
    }
    trace ("gtkui w_unreg_widget: widget type %s is not registered\n", type);
}

int
w_is_registered (const char *type) {
    for (w_creator_t *c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            return 1;
        }
    }
    return 0;
}

static int
get_num_widgets (ddb_gtkui_widget_t *w, const char *type) {
    int num = 0;
    if (!strcmp (w->type, type)) {
        num++;
    }
    for (w = w->children; w; w = w->next) {
        num += get_num_widgets (w, type);
    }
    return num;
}

ddb_gtkui_widget_t *
w_create (const char *type) {
    for (w_creator_t *c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            if (c->flags & DDB_WF_SINGLE_INSTANCE) {
                int num = get_num_widgets (rootwidget, c->type);
                // HACK: playlist and tabbed playlist are essentially the same
                // widgets with single-instance limit

                if (!strcmp (c->type, "tabbed_playlist")) {
                    num += get_num_widgets (rootwidget, "playlist");
                }
                else if (!strcmp (c->type, "playlist")) {
                    num += get_num_widgets (rootwidget, "tabbed_playlist");
                }
                if (num) {
                    // create dummy
                    w_dummy_t *w = (w_dummy_t *)w_create ("dummy");
                    w->text = strdup (_ ("Multiple widgets of this type are not supported"));
                    return (ddb_gtkui_widget_t *)w;
                }
            }
            ddb_gtkui_widget_t *w = c->create_func ();
            w->type = c->type;

            return w;
        }
    }
    return NULL;
}

uint32_t
w_get_type_flags (const char *type) {
    for (w_creator_t *c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            return c->flags;
        }
    }
    return 0;
}

void
w_destroy (ddb_gtkui_widget_t *w) {
    if (w->destroy) {
        w->destroy (w);
    }
    if (w->widget) {
        gtk_widget_destroy (w->widget);
    }
    free (w);
}

///// gtk_container convenience functions
void
w_container_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *container = w_get_container (cont);
    gtk_container_add (GTK_CONTAINER (container), child->widget);
    gtk_widget_show (child->widget);
}

void
w_container_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *container = NULL;
    container = cont->widget;
    gtk_container_remove (GTK_CONTAINER (container), child->widget);
}

typedef struct {
    GtkWidget *button;
    w_vis_color_row_t *row;
    gboolean *changed;
} vis_color_row_ctx_t;

static void
_vis_color_check_toggled (GtkToggleButton *check, gpointer user_data) {
    vis_color_row_ctx_t *ctx = user_data;
    gboolean active = gtk_toggle_button_get_active (check);
    gtk_widget_set_sensitive (ctx->button, active);
    if (active && !*ctx->row->color_set) {
        // start from the color currently shown on the button
        gtk_color_button_get_color (GTK_COLOR_BUTTON (ctx->button), ctx->row->color);
        *ctx->row->color_set = TRUE;
    }
    *ctx->row->use_custom = active;
    *ctx->changed = TRUE;
}

static void
_vis_color_button_set (GtkColorButton *button, gpointer user_data) {
    vis_color_row_ctx_t *ctx = user_data;
    gtk_color_button_get_color (button, ctx->row->color);
    *ctx->row->color_set = TRUE;
    *ctx->changed = TRUE;
}

// Shows the modal preferences dialog. Returns TRUE if anything was changed.
gboolean
w_vis_color_preferences_dialog (const char *title, w_vis_color_row_t *rows, int row_count) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons (
        title,
        GTK_WINDOW (mainwin),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        _("Done"),
        GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);

    gboolean changed = FALSE;
    vis_color_row_ctx_t *contexts = calloc (row_count, sizeof (vis_color_row_ctx_t));

    GtkWidget *vbox = gtk_dialog_get_content_area (GTK_DIALOG (dlg));
    gtk_box_set_spacing (GTK_BOX (vbox), 6);
    gtk_container_set_border_width (GTK_CONTAINER (dlg), 8);

    for (int i = 0; i < row_count; i++) {
        w_vis_color_row_t *row = &rows[i];
        vis_color_row_ctx_t *ctx = &contexts[i];

        GtkWidget *hbox = gtk_hbox_new (FALSE, 12);
        GtkWidget *check = gtk_check_button_new_with_label (row->label);
        GtkWidget *button = gtk_color_button_new ();

        gtk_color_button_set_color (GTK_COLOR_BUTTON (button), *row->color_set ? row->color : &row->default_color);
        gtk_widget_set_sensitive (button, *row->use_custom);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), *row->use_custom);

        ctx->button = button;
        ctx->row = row;
        ctx->changed = &changed;

        // connect after the initial state is set, so that opening the dialog doesn't count as a change
        g_signal_connect (check, "toggled", G_CALLBACK (_vis_color_check_toggled), ctx);
        g_signal_connect (button, "color-set", G_CALLBACK (_vis_color_button_set), ctx);

        gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
        gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    }

    gtk_widget_show_all (dlg);
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    free (contexts);

    return changed;
}

// Colors are stored as "r g b a" with components in 0..1, same as in the macOS UI.
char *
w_vis_color_to_string (const GdkColor *color) {
    char temp[64];
    snprintf (temp, sizeof (temp), "%0.3f %0.3f %0.3f 1.000", color->red / 65535.f, color->green / 65535.f, color->blue / 65535.f);
    return strdup (temp);
}

gboolean
w_vis_color_from_string (const char *str, GdkColor *color) {
    float c[4];
    if (sscanf (str, "%f %f %f %f", &c[0], &c[1], &c[2], &c[3]) != 4) {
        return FALSE;
    }
    for (int i = 0; i < 3; i++) {
        c[i] = c[i] < 0 ? 0 : (c[i] > 1 ? 1 : c[i]);
    }
    color->red = (guint16)(c[0] * 65535.f + 0.5f);
    color->green = (guint16)(c[1] * 65535.f + 0.5f);
    color->blue = (guint16)(c[2] * 65535.f + 0.5f);
    return TRUE;
}
