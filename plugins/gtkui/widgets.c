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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "gtkui.h"
#include "interface.h"
#include "support.h"
#include "widgets.h"
#include "ddbtabstrip.h"
#include "ddblistview.h"
#include "mainplaylist.h"
#include "../libparser/parser.h"
#include "trkproperties.h"
#include "coverart.h"
#include "namedicons.h"
#include "hotkeys.h" // for building action treeview
#include "../../strdupa.h"
#include "../../fastftoi.h"
#include "actions.h"
#include "ddbseekbar.h"
#include "ddbvolumebar.h"
#include "callbacks.h"
#include "drawing.h"
#include "ddb_splitter.h"
#include "../../analyzer/analyzer.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))


// utility code for parsing keyvalues
#define get_keyvalue(s,key,val) {\
    s = gettoken_ext (s, key, "={}();");\
    if (!s) {\
        return NULL;\
    }\
    if (s && !strcmp (key, "{")) {\
        break;\
    }\
    s = gettoken_ext (s, val, "={}();");\
    if (!s || strcmp (val, "=")) {\
        return NULL;\
    }\
    s = gettoken_ext (s, val, "={}();");\
    if (!s) {\
        return NULL;\
    }\
}

typedef struct w_creator_s {
    const char *type;
    const char *title; // set to NULL to avoid exposing this widget type to user
    uint32_t flags;
    int compat; // when this is set to 1 -- it's a backwards compatibility creator, and must be skipped in GUI
    ddb_gtkui_widget_t *(*create_func) (void);
    struct w_creator_s *next;
} w_creator_t;

static w_creator_t *w_creators;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *label;
    char *text;
} w_dummy_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *box;
    // size of first child
    int size1;
    // size of second child
    int size2;
    float ratio;
    int locked;
} w_splitter_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_box_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *tabstrip;
} w_tabstrip_t;

typedef struct {
    ddb_gtkui_widget_t base;
    DdbListview *list;
    int hideheaders;
    int width;
} w_playlist_t;

typedef struct {
    w_playlist_t plt;
    DdbTabStrip *tabstrip;
} w_tabbed_playlist_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
} w_placeholder_t;

typedef struct {
    ddb_gtkui_widget_t base;
    int clicked_page;
    int active;
    int num_tabs;
    char **titles;
} w_tabs_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *tree;
    guint refresh_timeout;
} w_selproperties_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
    int widget_height;
    int widget_width;
    guint load_timeout_id;
} w_coverart_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
    guint drawtimer;
    float *samples;
    int nsamples;
    int resized;
    intptr_t mutex;
    cairo_surface_t *surf;
} w_scope_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
    guint drawtimer;

    intptr_t mutex;

    ddb_analyzer_t analyzer;
    ddb_analyzer_draw_data_t draw_data;
    ddb_waveformat_t fmt;
    ddb_audio_data_t input_data;

    cairo_surface_t *surf;
} w_spectrum_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *box;
    uint64_t expand;
    uint64_t fill;
    unsigned homogeneous : 1;
} w_hvbox_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *button;
    GtkWidget *alignment;
    GdkColor color;
    GdkColor textcolor;
    char *icon;
    char *label;
    char *action;
    int action_ctx;
    unsigned use_color : 1;
    unsigned use_textcolor : 1;
} w_button_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *seekbar;
    gint timer;
    float last_songpos;
} w_seekbar_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_playtb_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *volumebar;
} w_volumebar_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *voices[8];
} w_ctvoices_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *textview;
    int scroll_bottomed;
} w_logviewer_t;

static int w_logviewer_instancecount;

typedef struct {
    w_logviewer_t *w;
    char *text_to_add;
} logviewer_addtexts_t;

static int design_mode;
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

static const char *
w_get_title (ddb_gtkui_widget_t *w)
{
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
    char *expected_type;
    char *parms;
    char *children;
} w_unknown_t;

const char *
w_unknown_load (struct ddb_gtkui_widget_s *base, const char *type, const char *s) {
    w_unknown_t *w = (w_unknown_t *)base;
    char buffer_parms[4000];
    char buffer_children[4000];

    const char *p = s;
    while (*p && *p != '{') {
        p++;
    }

    if (!(*p)) {
        fprintf (stderr, "reached EOL before expected { while trying to load unknown widget %s\n", w->expected_type);
        return NULL;
    }

    if (p - s + 1 > sizeof (buffer_parms)) {
        fprintf (stderr, "buffer to small to load unknown widget %s\n", w->expected_type);
        return NULL;
    }

    memcpy (buffer_parms, s, p-s);
    buffer_parms[p-s] = 0;

    p++;
    s = p;

    int nb = 1;
    while (*p) {
        if (*p == '{') {
            nb++;
        }
        if (*p == '}') {
            nb--;
            if (nb == 0) {
                break;
            }
        }
        p++;
    }
    if (!(*p)) {
        fprintf (stderr, "reached EOL before expected } while trying to load unknown widget %s\n", w->expected_type);
        return NULL;
    }
    if (p - s + 1 > sizeof (buffer_parms)) {
        fprintf (stderr, "buffer to small to load unknown widget %s\n", w->expected_type);
        return NULL;
    }
    memcpy (buffer_children, s, p-s);
    buffer_children[p-s] = 0;

    w->parms = strdup (buffer_parms);
    w->children = strdup (buffer_children);

    // caller expects 's' to point to '}'
    return p;
}

static void
w_unknown_save (struct ddb_gtkui_widget_s *base, char *s, int sz) {
    w_unknown_t *w = (w_unknown_t *)base;
    size_t l = strlen (s);
    s += l;
    sz -= l;
    snprintf (s, sz, "%s%s {%s}", w->expected_type, w->parms, w->children);
}

void w_unknown_destroy (ddb_gtkui_widget_t *_w) {
    w_unknown_t *w = (w_unknown_t *)_w;
    if (w->expected_type) {
        free (w->expected_type);
        w->expected_type = NULL;
    }
    if (w->parms) {
        free (w->parms);
        w->parms = NULL;
    }
    if (w->children) {
        free (w->children);
        w->children = NULL;
    }
}

static gboolean
unknown_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    ddb_gtkui_widget_t *w = user_data;
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);

    cairo_set_font_size(cr, 16);

    cairo_move_to(cr, 20, 30);

    char s[1000];
    snprintf (s, sizeof (s), _("Widget \"%s\" is not available"), ((w_unknown_t *)w)->expected_type);

    cairo_show_text(cr, s);
    return TRUE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static gboolean
unknown_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = unknown_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}
#endif

ddb_gtkui_widget_t *
w_unknown_create (const char *type) {
    w_unknown_t *w = malloc (sizeof (w_unknown_t));
    memset (w, 0, sizeof (w_unknown_t));
    w->base.type = "unknown";
    w->base.load = w_unknown_load;
    w->base.save = w_unknown_save;
    w->base.destroy = w_unknown_destroy;
    w->expected_type = strdup (type);

    w->base.widget = gtk_event_box_new ();
    w->drawarea = gtk_drawing_area_new ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (unknown_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (unknown_draw), w);
#endif
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

const char *
w_create_from_string (const char *s, ddb_gtkui_widget_t **parent) {
    char t[MAX_TOKEN];
    s = gettoken (s, t);
    if (!s) {
        return NULL;
    }
    char *type = strdupa (t);
    ddb_gtkui_widget_t *w = w_create (type);
    if (!w) {
        w = w_unknown_create (type);
    }
    // nuke all default children
    while (w->children) {
        ddb_gtkui_widget_t *c = w->children;
        w_remove (w, w->children);
        w_destroy (c);
    }

    // load widget params
    if (w->load) {
        s = w->load (w, type, s);
        if (!s) {
            w_destroy (w);
            return NULL;
        }
    }
    else {
        // skip all params (if any)
        for (;;) {
            s = gettoken_ext (s, t, "={}();");
            if (!s) {
                w_destroy (w);
                return NULL;
            }
            if (!strcmp (t, "{")) {
                break;
            }
            // match '='
            char eq[MAX_TOKEN];
            s = gettoken_ext (s, eq, "={}();");
            if (!s || strcmp (eq, "=")) {
                w_destroy (w);
                return NULL;
            }
            s = gettoken_ext (s, eq, "={}();");
            if (!s) {
                w_destroy (w);
                return NULL;
            }
        }
    }

    // we don't need to match '{' here, it's already done above
    const char *back = s;
    s = gettoken (s, t);
    if (!s) {
        w_destroy (w);
        return NULL;
    }
    for (;;) {
        if (!strcmp (t, "}")) {
            break;
        }

        s = w_create_from_string (back, &w);
        if (!s) {
            w_destroy (w);
            return NULL;
        }

        back = s;
        s = gettoken (s, t);
        if (!s) {
            w_destroy (w);
            return NULL;
        }
    }

    if (*parent) {
        w_append (*parent, w);
    }
    else {
        *parent = w;
    }
    return s;
}

static ddb_gtkui_widget_t *current_widget;
static int hidden = 0;

static gboolean
w_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    if (hidden && user_data == current_widget) {
        GtkAllocation allocation;
        gtk_widget_get_allocation (widget, &allocation);
        cairo_set_source_rgb (cr, 0.17, 0, 0.83);

        if (!gtk_widget_get_has_window (widget)) {
#if GTK_CHECK_VERSION(3,0,0)
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

void
save_widget_to_string (char *str, int sz, ddb_gtkui_widget_t *w) {
    // uknown is special case
    if (!strcmp (w->type, "unknown")) {
        w->save (w, str, sz);
        return;
    }

    strcat (str, w->type);
    if (w->save) {
        w->save (w, str, sz);
    }

    strcat (str, " {");
    for (ddb_gtkui_widget_t *c = w->children; c; c = c->next) {
        save_widget_to_string (str, sz, c);
    }
    strcat (str, "} ");
}

void
w_save (void) {
    if (rootwidget == NULL) return;
    
    char buf[20000] = "";
    save_widget_to_string (buf, sizeof (buf), rootwidget->children);
    deadbeef->conf_set_str (DDB_GTKUI_CONF_LAYOUT, buf);
    deadbeef->conf_save ();
}

static void
on_replace_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *ui_widget = g_object_get_data(G_OBJECT(menuitem), associated_widget_data_id);
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
    w_save ();
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
    w_save ();
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
    save_widget_to_string (paste_buffer, sizeof (paste_buffer), ui_widget);

    if (parent->replace) {
        parent->replace (parent, ui_widget, w_create ("placeholder"));
    }
    else {
        w_remove (parent, ui_widget);
        w_destroy (ui_widget);
        ui_widget = w_create ("placeholder");
        w_append (parent, ui_widget);
    }
    w_save ();
}

static void
on_copy_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *ui_widget = user_data;
    if (!strcmp (ui_widget->type, "placeholder")) {
        return;
    }
    // save hierarchy to string
    paste_buffer[0] = 0;
    save_widget_to_string (paste_buffer, sizeof (paste_buffer), ui_widget);
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
    w_create_from_string (paste_buffer, &w);
    w_replace (parent, ui_widget, w);
    w_save ();
    ui_widget = w;
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

static void
add_menu_separator (GtkWidget *menu)
{
    GtkWidget *separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);
}

static GtkWidget *
create_widget_menu(ddb_gtkui_widget_t *ui_widget) {
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

        add_menu_separator (menu);
    }

    if (strcmp (ui_widget->type, "placeholder")) {
        item = gtk_menu_item_new_with_mnemonic (_("Replace with..."));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }
    else {
        item = gtk_menu_item_new_with_mnemonic (_("Insert..."));
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
            g_object_set_data(G_OBJECT(item), associated_widget_data_id, ui_widget);
            g_signal_connect ((gpointer) item, "activate",
                    G_CALLBACK (on_replace_activate),
                    (void *)cr->type);
        }
    }

    if (strcmp (ui_widget->type, "placeholder")) {
        item = gtk_menu_item_new_with_mnemonic (_("Delete"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_delete_activate),
                ui_widget);

        item = gtk_menu_item_new_with_mnemonic (_("Cut"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_cut_activate),
                ui_widget);

        item = gtk_menu_item_new_with_mnemonic (_("Copy"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_copy_activate),
                ui_widget);
    }

    item = gtk_menu_item_new_with_mnemonic (_("Paste"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_paste_activate),
            ui_widget);

    if (ui_widget->initmenu) {
        add_menu_separator (menu);
        ui_widget->initmenu (ui_widget, menu);
    }
    if (ui_widget->parent && ui_widget->parent->initchildmenu) {
        add_menu_separator (menu);
        ui_widget->parent->initchildmenu (ui_widget, menu);
    }

    return menu;
}

gboolean
w_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (!design_mode || !TEST_RIGHT_CLICK(event)) {
        return FALSE;
    }

    current_widget = user_data;
    widget = current_widget->widget;
    hidden = 1;

    if (GTK_IS_CONTAINER (widget)) {
        // hide all children
#if !GTK_CHECK_VERSION(3,0,0)
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
    menu = create_widget_menu(current_widget);

    if (current_widget->parent && strcmp(current_widget->parent->type, "box")) {
        GtkWidget *item, *submenu;
        add_menu_separator (menu);
        item = gtk_menu_item_new_with_mnemonic (_("Parent"));
        gtk_widget_show (item);
        submenu = create_widget_menu (current_widget->parent);
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(item), submenu);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }

    g_signal_connect ((gpointer) menu, "deactivate", G_CALLBACK (w_menu_deactivate), user_data);
    gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
    return TRUE;
}

void
w_override_signals (GtkWidget *widget, gpointer user_data) {
    g_signal_connect ((gpointer) widget, "button_press_event", G_CALLBACK (w_button_press_event), user_data);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) widget, "expose_event", G_CALLBACK (w_expose_event), user_data);
#else
    g_signal_connect ((gpointer) widget, "draw", G_CALLBACK (w_draw_event), user_data);
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
        const char *type = va_arg(vl, const char *);
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
    va_end(vl);
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
                    w->text = strdup (_("Multiple widgets of this type are not supported"));
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

////// placeholder widget
gboolean
w_placeholder_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    if (!design_mode) {
        return FALSE;
    }
    cairo_set_source_rgb (cr, 255, 0, 0);
    cairo_surface_t *checker;
    cairo_t *cr2;

    checker = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 12, 12);
    cr2 = cairo_create (checker);

    cairo_set_source_rgb (cr2, 0.5, 0.5 ,0.5);
    cairo_paint (cr2);
    cairo_set_source_rgb (cr2, 0, 0, 0);
    cairo_move_to (cr2, 0, 0);
    cairo_line_to (cr2, 12, 12);
    cairo_move_to (cr2, 1, 12);
    cairo_line_to (cr2, 12, 1);
    cairo_set_line_width (cr2, 1);
    cairo_set_antialias (cr2, CAIRO_ANTIALIAS_NONE);
    cairo_stroke (cr2);
    cairo_fill (cr2);
    cairo_destroy (cr2);

    cairo_set_source_surface (cr, checker, 0, 0);
    cairo_pattern_t *pt = cairo_get_source(cr);
    cairo_pattern_set_extend (pt, CAIRO_EXTEND_REPEAT);
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    cairo_rectangle (cr, 0, 0, a.width, a.height);
    cairo_paint (cr);
    cairo_surface_destroy (checker);
    return FALSE;
}

gboolean
w_placeholder_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = w_placeholder_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

ddb_gtkui_widget_t *
w_placeholder_create (void) {
    w_placeholder_t *w = malloc (sizeof (w_placeholder_t));
    memset (w, 0, sizeof (w_placeholder_t));

    w->base.widget = gtk_event_box_new ();
    w->drawarea  = gtk_drawing_area_new ();
    gtk_widget_set_size_request (w->base.widget, 20, 20);
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);

#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (w_placeholder_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (w_placeholder_draw), w);
#endif
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

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

// common splitter funcs
const char *
w_splitter_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "vsplitter") && strcmp (type, "hsplitter")) {
        return NULL;
    }

    w_splitter_t *sp = (w_splitter_t *)w;

    int got_ratio = 0;

    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s,key,val);

        if (!strcmp (key, "locked")) {
            sp->locked = atoi (val);
        }
        else if (!strcmp (key, "ratio")) {
            sp->ratio = atof (val);
            if (sp->ratio < 0) {
                sp->ratio = 0;
            }
            if (sp->ratio > 1) {
                sp->ratio = 1;
            }
            got_ratio = 1;
        }
        else if (!strcmp (key, "pos")) {
            sp->size1 = atoi (val);
        }
        else if (!strcmp (key, "size2")) {
            sp->size2 = atoi (val);
        }
    }

    if (!got_ratio) {
        sp->ratio = 0.5;
    }

    return s;
}

void
w_splitter_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    w_splitter_t *sp = (w_splitter_t *)w;
    int locked = ddb_splitter_get_size_mode (DDB_SPLITTER (sp->box));

    float ratio = ddb_splitter_get_proportion (DDB_SPLITTER (sp->box));
    char spos[100];
    // NOTE: we use pos instead of size1 to ensure compatiblity with older deadbeef versions
    snprintf (spos, sizeof (spos), " locked=%d ratio=%f pos=%d size2=%d", locked, ratio, sp->size1, sp->size2);
    strncat (s, spos, sz);
}

void
w_splitter_add (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *child) {
    w_container_add (w, child);
}

GtkWidget *
w_splitter_get_container (struct ddb_gtkui_widget_s *b) {
    return ((w_splitter_t *)b)->box;
}

void
on_splitter_lock_c1_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_splitter_t *sp = (w_splitter_t *)user_data;
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem))) {
        sp->locked = DDB_SPLITTER_SIZE_MODE_LOCK_C1;
        sp->size1 = ddb_splitter_get_child1_size (DDB_SPLITTER (sp->box));
        ddb_splitter_set_size_mode (DDB_SPLITTER (sp->box), DDB_SPLITTER_SIZE_MODE_LOCK_C1);
    }
}

void
on_splitter_lock_c2_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_splitter_t *sp = (w_splitter_t *)user_data;
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem))) {
        sp->locked = DDB_SPLITTER_SIZE_MODE_LOCK_C2;
        sp->size2 = ddb_splitter_get_child2_size (DDB_SPLITTER (sp->box));
        ddb_splitter_set_size_mode (DDB_SPLITTER (sp->box), DDB_SPLITTER_SIZE_MODE_LOCK_C2);
    }
}

void
on_splitter_lock_prop_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_splitter_t *sp = (w_splitter_t *)user_data;
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem))) {
        sp->locked = DDB_SPLITTER_SIZE_MODE_PROP;
        ddb_splitter_set_size_mode (DDB_SPLITTER (sp->box), DDB_SPLITTER_SIZE_MODE_PROP);
    }
}

void
w_splitter_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    w_splitter_t *sp = (w_splitter_t *)w;
    GtkOrientation orientation = ddb_splitter_get_orientation (DDB_SPLITTER (sp->box));

    GSList *group = NULL;

    GtkWidget *item = gtk_radio_menu_item_new_with_mnemonic (group, _("Proportional Sizing"));
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    if (sp->locked == DDB_SPLITTER_SIZE_MODE_PROP) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_splitter_lock_prop_toggled),
            w);

    if (orientation == GTK_ORIENTATION_VERTICAL) {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _("Lock Top Pane Height"));
    }
    else {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _("Lock Left Pane Width"));
    }
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    if (sp->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C1) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_splitter_lock_c1_toggled),
            w);

    if (orientation == GTK_ORIENTATION_VERTICAL) {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _("Lock Bottom Pane Height"));
    }
    else {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _("Lock Right Pane Width"));
    }
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    if (sp->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C2) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_splitter_lock_c2_toggled),
            w);
}

void
w_splitter_replace (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child, ddb_gtkui_widget_t *newchild) {
    int ntab = 0;
    ddb_gtkui_widget_t *prev = NULL;
    for (ddb_gtkui_widget_t *c = cont->children; c; c = c->next, ntab++) {
        if (c == child) {
            newchild->next = c->next;
            if (prev) {
                prev->next = newchild;
            }
            else {
                cont->children = newchild;
            }
            newchild->parent = cont;
            w_remove (cont, child);
            w_destroy (child);
            GtkWidget *container = ((w_splitter_t *)cont)->box;
            gtk_widget_show (newchild->widget);
            if (ntab == 0) {
                ddb_splitter_add_child_at_pos (DDB_SPLITTER (container), newchild->widget, 0);
            }
            else {
                ddb_splitter_add_child_at_pos (DDB_SPLITTER (container), newchild->widget, 1);
            }
            break;
        }
        prev = c;
    }
}

void
w_splitter_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *container = w_get_container (cont);
    gtk_container_remove (GTK_CONTAINER (container), child->widget);
}

void
w_splitter_init (ddb_gtkui_widget_t *base) {
    w_splitter_t *w = (w_splitter_t *)base;

    ddb_splitter_set_proportion (DDB_SPLITTER (w->box), w->ratio);
    ddb_splitter_set_size_mode (DDB_SPLITTER (w->box), w->locked);
    if (w->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C1) {
        ddb_splitter_set_child1_size (DDB_SPLITTER (w->box), w->size1);
    }
    else if (w->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C2) {
        ddb_splitter_set_child2_size (DDB_SPLITTER (w->box), w->size2);
    }
}

////// vsplitter widget
ddb_gtkui_widget_t *
w_vsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->ratio = 0.5f;
    w->locked = DDB_SPLITTER_SIZE_MODE_PROP;
    w->base.append = w_splitter_add;
    w->base.remove = w_splitter_remove;
    w->base.replace = w_splitter_replace;
    w->base.get_container = w_splitter_get_container;
    w->base.load = w_splitter_load;
    w->base.save = w_splitter_save;
    w->base.init = w_splitter_init;
    w->base.initmenu = w_splitter_initmenu;

    w->base.widget = gtk_event_box_new ();
    w->box = ddb_splitter_new (GTK_ORIENTATION_VERTICAL);
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);
    w_override_signals (w->base.widget, w);

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);

    return (ddb_gtkui_widget_t*)w;
}

////// hsplitter widget
ddb_gtkui_widget_t *
w_hsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->ratio = 0.5f;
    w->locked = DDB_SPLITTER_SIZE_MODE_PROP;
    w->base.append = w_splitter_add;
    w->base.remove = w_splitter_remove;
    w->base.replace = w_splitter_replace;
    w->base.get_container = w_splitter_get_container;
    w->base.load = w_splitter_load;
    w->base.save = w_splitter_save;
    w->base.init = w_splitter_init;
    w->base.initmenu = w_splitter_initmenu;

    w->base.widget = gtk_event_box_new ();
    w->box = ddb_splitter_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);
    w_override_signals (w->base.widget, w);

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);

    return (ddb_gtkui_widget_t*)w;
}

///// tabs widget

static void
w_tabs_destroy (ddb_gtkui_widget_t *w) {
    w_tabs_t *s = (w_tabs_t *)w;
    if (s->titles) {
        for (int i = 0; i < s->num_tabs; i++) {
            if (s->titles[i]) {
                free (s->titles[i]);
            }
        }
        free (s->titles);
    }
}

const char *
w_tabs_load (struct ddb_gtkui_widget_s *widget, const char *type, const char *s) {
    w_tabs_t *w = (w_tabs_t *)widget;
    if (strcmp (type, "tabs")) {
        return NULL;
    }

    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s,key,val);

        if (!strcmp (key, "active")) {
            w->active = atoi (val);
            continue;
        }
        if (!strcmp (key, "num_tabs")) {
            w->num_tabs = atoi (val);
            w->titles = malloc (w->num_tabs * sizeof (char *));
            continue;
        }
        for (int i = 0; i < w->num_tabs; i++) {
            char tab_name[100];
            snprintf (tab_name, sizeof (tab_name), "tab%03d", i);
            if (!strcmp (key, tab_name)) {
                w->titles[i] = strdup (val);
                continue;
            }
        }
    }

    return s;
}

void
w_tabs_save (struct ddb_gtkui_widget_s *widget, char *s, int sz) {
    w_tabs_t *w = (w_tabs_t *)widget;
    int active = gtk_notebook_get_current_page (GTK_NOTEBOOK (w->base.widget));
    int num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (w->base.widget));
    char spos[1000];
    char *pp = spos;
    int ss = sizeof (spos);
    int n;

    n = snprintf (spos, sizeof (spos), " active=%d num_tabs=%d", active, num_pages);
    ss -= n;
    pp += n;
    for (int i = 0; i < num_pages; i++) {
        GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), i);
        const char *text = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (w->base.widget), child);
        char *esctext = parser_escape_string (text);
        n = snprintf (pp, ss, " tab%03d=\"%s\"", i, esctext);
        free (esctext);
        ss -= n;
        pp += n;
    }
    strncat (s, spos, sz);
}

static void
tabs_add_tab (gpointer user_data)
{
    w_tabs_t *w = user_data;

    ddb_gtkui_widget_t *ph;
    ph = w_create ("placeholder");
    w_append ((ddb_gtkui_widget_t*)w, ph);

    int i = 0;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++);
    w->clicked_page = i-1;
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->clicked_page);

}

static void
tabs_remove_tab (gpointer user_data, int tab)
{
    w_tabs_t *w = user_data;
    int i = 0;
    int num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (w->base.widget));
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == tab) {
            w_remove ((ddb_gtkui_widget_t *)w, c);
            w_destroy (c);
            if (num_pages == 1) {
                // if last tab was deleted add a new placeholder tab
                tabs_add_tab (w);
            }
            return;
        }
    }
}

static void
on_rename_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;

    GtkWidget *dlg = create_entrydialog ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Rename Tab"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL(e), _("Title:"));
    e = lookup_widget (dlg, "title");
    int active = gtk_notebook_get_current_page (GTK_NOTEBOOK (w->base.widget));
    GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), active);
    gtk_entry_set_text (GTK_ENTRY (e), gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (w->base.widget), child));
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (w->base.widget), child, gtk_entry_get_text (GTK_ENTRY (e)));
    }
    gtk_widget_destroy (dlg);
}

static void
on_remove_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;
    tabs_remove_tab (w, w->clicked_page);
}

static void
on_add_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    tabs_add_tab (user_data);
}

static void
on_move_tab_left_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;
    if (w->clicked_page <= 0) {
        return;
    }
    // remove and save widget
    int i = 0;
    ddb_gtkui_widget_t *newchild = NULL;
    ddb_gtkui_widget_t *prev = NULL;
    char *title = NULL;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == w->clicked_page) {
            char buf[20000] = "";
            save_widget_to_string (buf, sizeof (buf), c);
            GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), i);
            title = strdup (gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (w->base.widget), child));
            w_remove ((ddb_gtkui_widget_t *)w, c);
            w_destroy (c);
            w_create_from_string (buf, &newchild);
            break;
        }
    }
    if (!newchild) {
        goto out;
    }

    // add new child at new position
    i = 0;
    prev = NULL;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == w->clicked_page-1) {
            if (prev) {
                newchild->next = prev->next;
                prev->next = newchild;
            }
            else {
                newchild->next = w->base.children;
                w->base.children = newchild;
            }
            GtkWidget *label = gtk_label_new (title);
            gtk_widget_show (label);
            gtk_widget_show (newchild->widget);

            gtk_notebook_insert_page (GTK_NOTEBOOK (w->base.widget), newchild->widget, label, w->clicked_page-1);
#if GTK_CHECK_VERSION(3,0,0)
            gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
            gtk_misc_set_padding (GTK_MISC (label), 0, 0);
            gtk_container_child_set (GTK_CONTAINER (w->base.widget),
                    newchild->widget,
                    "tab-expand", TRUE,
                    "tab-fill", TRUE,
                    NULL);
#endif
            gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->clicked_page-1);
            w->clicked_page--;
            break;
        }
        prev = c;
    }

out:
    if (title) {
        free (title);
    }
}

static void
on_move_tab_right_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;

    int num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (w->base.widget));
    if (w->clicked_page == num_pages - 1) {
        return;
    }

    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), ++w->clicked_page);
    on_move_tab_left_activate (menuitem, user_data);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), ++w->clicked_page);
}

static void
on_tab_popup_menu (GtkWidget *widget, gpointer user_data)
{
    w_tabs_t *w = user_data;
    GtkWidget *menu;
    GtkWidget *item;
    menu = gtk_menu_new ();

    item = gtk_menu_item_new_with_mnemonic (_("Rename Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_rename_tab_activate),
            w);

    item = gtk_menu_item_new_with_mnemonic (_("Remove Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_remove_tab_activate),
            w);

    item = gtk_menu_item_new_with_mnemonic (_("Add New Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_add_tab_activate),
            w);

    add_menu_separator (menu);

    item = gtk_menu_item_new_with_mnemonic (_("Move Tab Left"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_move_tab_left_activate),
            w);

    item = gtk_menu_item_new_with_mnemonic (_("Move Tab Right"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_move_tab_right_activate),
            w);

    gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

static void
w_tabs_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    const char *title = w_get_title (child);
    GtkWidget *label = gtk_label_new (title ? title : child->type);
    gtk_widget_show (label);
    gtk_widget_show (child->widget);
    gtk_notebook_append_page (GTK_NOTEBOOK (cont->widget), child->widget, label);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_misc_set_padding (GTK_MISC (label), 0, 0);
    gtk_container_child_set (GTK_CONTAINER (cont->widget),
                           child->widget,
                           "tab-expand", TRUE,
                           "tab-fill", TRUE,
                           NULL);
#endif
}

static void
w_tabs_replace (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child, ddb_gtkui_widget_t *newchild) {
    int ntab = 0;
    ddb_gtkui_widget_t *prev = NULL;
    for (ddb_gtkui_widget_t *c = cont->children; c; prev = c, c = c->next, ntab++) {
        if (c == child) {
            newchild->next = c->next;
            if (prev) {
                prev->next = newchild;
            }
            else {
                cont->children = newchild;
            }
            newchild->parent = cont;
            gtk_notebook_remove_page (GTK_NOTEBOOK(cont->widget), ntab);
            c->widget = NULL;
            w_destroy (c);
            const char *title = w_get_title (newchild);
            GtkWidget *label = gtk_label_new (title ? title : newchild->type);
            gtk_widget_show (label);
            gtk_widget_show (newchild->widget);
            int pos = gtk_notebook_insert_page (GTK_NOTEBOOK (cont->widget), newchild->widget, label, ntab);
#if GTK_CHECK_VERSION(3,0,0)
            gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
            gtk_misc_set_padding (GTK_MISC (label), 0, 0);
            gtk_container_child_set (GTK_CONTAINER (cont->widget),
                           newchild->widget,
                           "tab-expand", TRUE,
                           "tab-fill", TRUE,
                           NULL);
#endif
            gtk_notebook_set_current_page (GTK_NOTEBOOK (cont->widget), pos);
            break;
        }
    }
}

static gboolean
get_event_coordinates_in_widget (GtkWidget *widget,
			GdkEventButton  *event,
			gint      *x,
			gint      *y)
{
    GdkWindow *window = event->window;
    gdouble tx, ty;
    tx = event->x;
    ty = event->y;

    while (window && window != gtk_widget_get_window (widget)) {
        gint window_x, window_y;
        gdk_window_get_position (window, &window_x, &window_y);
        tx += window_x;
        ty += window_y;
        window = gdk_window_get_parent (window);
    }

    if (window) {
        *x = tx;
        *y = ty;
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static gboolean
on_tabs_button_press_event (GtkWidget      *notebook,
                            GdkEventButton *event,
                            gpointer   user_data)
{
    w_tabs_t *w = user_data;

    int           page_num = 0;
    GtkWidget     *page;
    GtkWidget     *label_box;
    GtkAllocation  alloc;

    int event_x, event_y;
    if (!get_event_coordinates_in_widget (notebook, event, &event_x, &event_y)) {
        // clicked outside the tabstrip (e.g. in one of its children)
        return FALSE;
    }

    /* lookup the clicked tab */
    while ((page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num)) != NULL)
    {
        label_box = gtk_notebook_get_tab_label (GTK_NOTEBOOK (notebook), page);
        gtk_widget_get_allocation (label_box, &alloc);

        if (event_x >= alloc.x && event_x < alloc.x + alloc.width
                && event_y >= alloc.y && event_y < alloc.y + alloc.height)
            break;

        page_num++;
    }
    w->clicked_page = page_num;

    if (event->type == GDK_BUTTON_PRESS) {
        /* leave if no tab could be found */
        if (page == NULL) {
            return FALSE;
        }

        if (event->button == 2) {
            /* check if we should close the tab */
            if (design_mode) {
                tabs_remove_tab (w, page_num);
            }
        }
        else if (event->button == 3) {
            if (!design_mode) {
                /* update the current tab before we show the menu */
                gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);

                /* show the tab menu */
                on_tab_popup_menu (notebook, user_data);
                return TRUE;
            }
        }
    }
    else if (event->type == GDK_2BUTTON_PRESS) {
        if (event->button == 1 && page == NULL && design_mode) {
            // open new tab
            tabs_add_tab (w);
        }
        return TRUE;
    }
    return FALSE;
}

static void
w_tabs_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_("Add New Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_add_tab_activate),
            w);
}

static void
w_tabs_init (ddb_gtkui_widget_t *base) {
    w_tabs_t *w = (w_tabs_t *)base;
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->active);
    if (w->titles) {
        int page = 0;
        while (page < w->num_tabs) {
            GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), page);
            if (w->titles[page]) {
                gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (w->base.widget), child, w->titles[page]);
#if GTK_CHECK_VERSION(3,0,0)
                GtkLabel *label = GTK_LABEL(gtk_notebook_get_tab_label (GTK_NOTEBOOK (w->base.widget), child));
                gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
                gtk_misc_set_padding (GTK_MISC (label), 0, 0);
#endif
            }
            page++;
        }
    }
}

ddb_gtkui_widget_t *
w_tabs_create (void) {
    w_tabs_t *w = malloc (sizeof (w_tabs_t));
    memset (w, 0, sizeof (w_tabs_t));
    w->base.widget = gtk_notebook_new ();
    w->base.append = w_tabs_add;
    w->base.remove = w_container_remove;
    w->base.replace = w_tabs_replace;
    w->base.initmenu = w_tabs_initmenu;
    w->base.save = w_tabs_save;
    w->base.load = w_tabs_load;
    w->base.init = w_tabs_init;
    w->base.destroy = w_tabs_destroy;

    ddb_gtkui_widget_t *ph1, *ph2, *ph3;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");
    ph3 = w_create ("placeholder");

    gtk_notebook_set_scrollable (GTK_NOTEBOOK (w->base.widget), TRUE);

#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
#else
    g_signal_connect ((gpointer) w->base.widget, "draw", G_CALLBACK (w_draw_event), w);
#endif
    g_signal_connect ((gpointer) w->base.widget, "button_press_event", G_CALLBACK (on_tabs_button_press_event), w);

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);
    w_append ((ddb_gtkui_widget_t*)w, ph3);

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

//// box widget
//// this widget should not be exposed to user, it is used as a top level
//// container (rootwidget)

ddb_gtkui_widget_t *
w_box_create (void) {
    w_box_t *w = malloc (sizeof (w_box_t));
    memset (w, 0, sizeof (w_box_t));
    w->base.widget = gtk_vbox_new (FALSE, 0);
    w->base.append = w_container_add;
    w->base.remove = w_container_remove;

    return (ddb_gtkui_widget_t*)w;
}

//// tabstrip widget
static gboolean
tabstrip_refresh_cb (void *ctx) {
    w_tabstrip_t *w = ctx;
    ddb_tabstrip_refresh (DDB_TABSTRIP (w->tabstrip));
    return FALSE;
}
static int
w_tabstrip_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_TITLE
            || p1 == DDB_PLAYLIST_CHANGE_POSITION
            || p1 == DDB_PLAYLIST_CHANGE_DELETED
            || p1 == DDB_PLAYLIST_CHANGE_CREATED) {
            g_idle_add (tabstrip_refresh_cb, w);
        }
        break;
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_tabstrip_override_conf(conf_str) || gtkui_tabstrip_colors_conf(conf_str) || gtkui_tabstrip_font_conf(conf_str)) {
                g_idle_add (tabstrip_refresh_cb, w);
            }
        }
    case DB_EV_PLAYLISTSWITCHED:
    case DB_EV_TRACKINFOCHANGED:
        g_idle_add (tabstrip_refresh_cb, w);
        break;
    }
    return 0;
}

ddb_gtkui_widget_t *
w_tabstrip_create (void) {
    w_tabstrip_t *w = malloc (sizeof (w_tabstrip_t));
    memset (w, 0, sizeof (w_tabstrip_t));
    w->base.flags = DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE;
    w->base.widget = gtk_event_box_new ();
    w->base.message = w_tabstrip_message;
    GtkWidget *ts = ddb_tabstrip_new ();
    gtk_widget_show (ts);
    gtk_container_add (GTK_CONTAINER (w->base.widget), ts);
    w->tabstrip = ts;
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

//// tabbed playlist widget

typedef struct {
    DdbListview *listview;
    DB_playItem_t *trk;
} w_trackdata_t;

static w_trackdata_t *
playlist_trackdata (DdbListview *listview, DB_playItem_t *track) {
    w_trackdata_t *td = malloc (sizeof (w_trackdata_t));
    td->listview = listview;
    td->trk = track;
    deadbeef->pl_item_ref(track);
    return td;
}

static gboolean
playlist_tabstriprefresh_cb (gpointer p) {
    w_tabbed_playlist_t *tp = p;
    ddb_tabstrip_refresh (tp->tabstrip);
    return FALSE;
}

static gboolean
trackinfochanged_cb (gpointer data) {
    w_trackdata_t *d = data;
    int idx = deadbeef->pl_get_idx_of (d->trk);
    if (idx != -1) {
        ddb_listview_draw_row (d->listview, idx, d->trk);
    }
    deadbeef->pl_item_unref (d->trk);
    free (d);
    return FALSE;
}

static gboolean
paused_cb (gpointer data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        int idx = deadbeef->pl_get_idx_of (it);
        if (idx != -1) {
            ddb_listview_draw_row (DDB_LISTVIEW(data), idx, it);
        }
        deadbeef->pl_item_unref (it);
    }
    return FALSE;
}

static gboolean
playlist_config_changed_cb (gpointer data) {
    ddb_listview_refresh (DDB_LISTVIEW(data), DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_CONFIG);
    return FALSE;
}

static gboolean
playlist_sort_reset_cb (gpointer data) {
    ddb_listview_col_sort_update (DDB_LISTVIEW(data));
    return FALSE;
}

static gboolean
playlist_list_refresh_cb (gpointer data) {
    ddb_listview_refresh (DDB_LISTVIEW(data), DDB_REFRESH_LIST);
    return FALSE;
}

static gboolean
playlist_header_refresh_cb (gpointer data) {
    ddb_listview_refresh (DDB_LISTVIEW(data), DDB_REFRESH_COLUMNS);
    return FALSE;
}

static gboolean
playlist_setup_cb (gpointer data) {
    DdbListview *listview = DDB_LISTVIEW(data);
    ddb_listview_clear_sort (listview);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        int scroll = deadbeef->plt_get_scroll (plt);
        if (!ddb_listview_list_setup(listview, scroll)) {
            deadbeef->plt_unref (plt);
            return TRUE;
        }

        int cursor = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cursor != -1) {
            DB_playItem_t *it = deadbeef->pl_get_for_idx (cursor);
            if (it) {
                deadbeef->pl_set_selected (it, 1);
                deadbeef->pl_item_unref (it);
            }
        }
        deadbeef->plt_unref (plt);

        if (scroll < 0) {
            ddb_listview_scroll_to (listview, scroll * -1);
        }

        ddb_listview_refresh(listview, DDB_REFRESH_LIST);
    }
    return FALSE;
}

static gboolean
songfinished_cb (gpointer data) {
    w_trackdata_t *d = data;
    int idx = deadbeef->pl_get_idx_of (d->trk);
    if (idx != -1) {
        ddb_listview_draw_row (d->listview, idx, d->trk);
    }
    deadbeef->pl_item_unref (d->trk);
    free (data);
    return FALSE;
}

// This only actually does anything if the track is in the current playlist, otherwise gtkui.c will handle it
static gboolean
songstarted_cb (gpointer data) {
    w_trackdata_t *d = data;
    int idx = deadbeef->pl_get_idx_of (d->trk);
    if (idx != -1) {
        if (!gtkui_listview_busy) {
            if (deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)) {
                ddb_listview_select_single (d->listview, idx);
                deadbeef->pl_set_cursor (PL_MAIN, idx);
            }
            if (deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)) {
                ddb_listview_scroll_to (d->listview, idx);
            }
        }
        ddb_listview_draw_row (d->listview, idx, d->trk);
    }
    deadbeef->pl_item_unref (d->trk);
    free (data);
    return FALSE;
}

static void
playlist_set_cursor (DdbListview *listview, DB_playItem_t *it) {
    int new_cursor = deadbeef->pl_get_idx_of_iter (it, PL_MAIN);
    if (new_cursor != -1) {
        int cursor = deadbeef->pl_get_cursor (PL_MAIN);
        if (new_cursor != cursor) {
            deadbeef->pl_set_cursor (PL_MAIN, new_cursor);
            ddb_listview_draw_row (listview, new_cursor, NULL);
            if (cursor != -1) {
                ddb_listview_draw_row (listview, cursor, NULL);
            }
        }
        ddb_listview_scroll_to (listview, new_cursor);
    }
}

static gboolean
cursor_moved_cb (gpointer data) {
    w_trackdata_t *d = data;
    playlist_set_cursor (d->listview, d->trk);
    deadbeef->pl_item_unref (d->trk);
    free (data);
    return FALSE;
}

static gboolean
focus_selection_cb (gpointer data) {
    DdbListview *listview = data;
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it && !deadbeef->pl_is_selected (it)) {
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    if (it) {
        playlist_set_cursor (listview, it);
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
    return FALSE;
}

// This only actually does anything if the track is in the current playlist
// Otherwise gtkui.c will handle it and send a PLAYLISTSWITCHED message
static gboolean
trackfocus_cb (gpointer data) {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        int cursor = deadbeef->pl_get_idx_of (it);
        if (cursor != -1) {
            ddb_listview_select_single (data, cursor);
            deadbeef->pl_set_cursor (PL_MAIN, cursor);
            ddb_listview_scroll_to (data, cursor);
        }
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
    return FALSE;
}

static int
w_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_playlist_t *p = (w_playlist_t *)w;
    switch (id) {
    case DB_EV_PAUSED:
        g_idle_add (paused_cb, p->list);
        break;
    case DB_EV_SONGFINISHED:
    {
        ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
        if (ev->track) {
            g_idle_add (songfinished_cb, playlist_trackdata(p->list, ev->track));
        }
        break;
    }
    case DB_EV_SONGSTARTED:
    {
        ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
        if (ev->track) {
            g_idle_add (songstarted_cb, playlist_trackdata(p->list, ev->track));
        }
        break;
    }
    case DB_EV_TRACKINFOCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
            g_idle_add (playlist_sort_reset_cb, p->list);
        }
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || (p1 == DDB_PLAYLIST_CHANGE_SELECTION && p2 != PL_MAIN) || p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                g_idle_add (trackinfochanged_cb, playlist_trackdata(p->list, ev->track));
            }
        }
        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
            g_idle_add (playlist_sort_reset_cb, p->list);
        }
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT ||
            (p1 == DDB_PLAYLIST_CHANGE_SELECTION && (p2 != PL_MAIN || (DdbListview *)ctx != p->list)) ||
            p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
            g_idle_add (playlist_list_refresh_cb, p->list);
        }
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (playlist_setup_cb, p->list);
        break;
    case DB_EV_FOCUS_SELECTION:
        g_idle_add (focus_selection_cb, p->list);
        break;
    case DB_EV_TRACKFOCUSCURRENT:
        g_idle_add (trackfocus_cb, p->list);
        break;
    case DB_EV_CURSOR_MOVED:
        if (p1 != PL_MAIN) {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                g_idle_add (cursor_moved_cb, playlist_trackdata(p->list, ev->track));
            }
        }
        break;
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_listview_override_conf(conf_str) || gtkui_listview_font_conf(conf_str)) {
                g_idle_add (playlist_config_changed_cb, p->list);
            }
            else if (gtkui_listview_colors_conf(conf_str)) {
                g_idle_add (playlist_list_refresh_cb, p->list);
                g_idle_add (playlist_header_refresh_cb, p->list);
            }
            else if (gtkui_listview_font_style_conf(conf_str) || !strcmp (conf_str, "playlist.pin.groups") ||
                    !strcmp (conf_str, "playlist.groups.spacing") ) {
                g_idle_add (playlist_list_refresh_cb, p->list);
            }
            else if (gtkui_tabstrip_override_conf(conf_str) || gtkui_tabstrip_colors_conf(conf_str)) {
                g_idle_add (playlist_header_refresh_cb, p->list);
            }
        }
        break;
    }
    return 0;
}

static int
w_tabbed_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *str = (char *)ctx;
            if (gtkui_tabstrip_override_conf(str) || gtkui_tabstrip_colors_conf(str) || gtkui_tabstrip_font_conf(str) || gtkui_tabstrip_font_style_conf(str)) {
                g_idle_add (playlist_tabstriprefresh_cb, w);
            }
        }
        break;
    case DB_EV_TRACKINFOCHANGED:
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (playlist_tabstriprefresh_cb, w);
        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_TITLE
            || p1 == DDB_PLAYLIST_CHANGE_POSITION
            || p1 == DDB_PLAYLIST_CHANGE_DELETED
            || p1 == DDB_PLAYLIST_CHANGE_CREATED) {
            g_idle_add (playlist_tabstriprefresh_cb, w);
        }
        break;
    }
    w_playlist_message (w, id, ctx, p1, p2);
    return 0;
}

static const char *
w_playlist_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "playlist") && strcmp (type, "tabbed_playlist")) {
        return NULL;
    }
    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s, key, val);
        if (!strcmp (key, "hideheaders")) {
            ((w_playlist_t *)w)->hideheaders = atoi (val);
        }
        if (!strcmp (key, "width")) {
            ((w_playlist_t *)w)->width = atoi (val);
        }
    }

    return s;
}

static void
w_playlist_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    w_playlist_t *ww = (w_playlist_t *)w;

    GtkAllocation a;
    gtk_widget_get_allocation(ww->base.widget, &a);
    int width = a.width;

    char save[100];
    snprintf (save, sizeof (save), " hideheaders=%d width=%d", ww->hideheaders, width);
    strncat (s, save, sz);
}

static void
w_playlist_init (ddb_gtkui_widget_t *base) {
    w_playlist_t *w = (w_playlist_t *)base;

    ddb_listview_show_header (w->list, !w->hideheaders);
    ddb_listview_init_autoresize (w->list, w->width);
    g_idle_add (playlist_setup_cb, w->list);
}

static void
on_playlist_showheaders_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_playlist_t *w = user_data;
    w->hideheaders = !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    ddb_listview_show_header (DDB_LISTVIEW (w->list), !w->hideheaders);
}

static void
w_playlist_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_check_menu_item_new_with_mnemonic (_("Show Column Headers"));
    gtk_widget_show (item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), !((w_playlist_t *)w)->hideheaders);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_playlist_showheaders_toggled),
            w);
}

ddb_gtkui_widget_t *
w_tabbed_playlist_create (void) {
    w_tabbed_playlist_t *w = malloc (sizeof (w_tabbed_playlist_t));
    memset (w, 0, sizeof (w_tabbed_playlist_t));

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    w->plt.base.widget = vbox;
    w->plt.base.save = w_playlist_save;
    w->plt.base.load = w_playlist_load;
    w->plt.base.init = w_playlist_init;
    w->plt.base.initmenu = w_playlist_initmenu;
    gtk_widget_show (vbox);

    GtkWidget *tabstrip = ddb_tabstrip_new ();
    w->tabstrip = DDB_TABSTRIP (tabstrip);
    gtk_widget_show (tabstrip);
    GtkWidget *list = ddb_listview_new ();
    gtk_widget_set_size_request (vbox, 250, 100);
    w->plt.list = (DdbListview *)list;
    gtk_widget_show (list);

    GtkWidget *sepbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (sepbox);
    gtk_container_set_border_width (GTK_CONTAINER (sepbox), 1);

    GtkWidget *hsep  = gtk_hseparator_new ();
    gtk_widget_show (hsep);
    gtk_box_pack_start (GTK_BOX (sepbox), hsep, FALSE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), tabstrip, FALSE, TRUE, 0);
    gtk_widget_set_can_focus (tabstrip, FALSE);
    gtk_widget_set_can_default (tabstrip, FALSE);

    gtk_box_pack_start (GTK_BOX (vbox), sepbox, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), list, TRUE, TRUE, 0);

    main_playlist_init (list);

    w_override_signals (w->plt.base.widget, w);

    w->plt.base.message = w_tabbed_playlist_message;
    return (ddb_gtkui_widget_t*)w;
}

///// playlist widget

ddb_gtkui_widget_t *
w_playlist_create (void) {
    w_playlist_t *w = malloc (sizeof (w_playlist_t));
    memset (w, 0, sizeof (w_playlist_t));

    w->base.widget = gtk_event_box_new ();
    w->list = DDB_LISTVIEW (ddb_listview_new ());
    gtk_widget_set_size_request (GTK_WIDGET (w->base.widget), 100, 100);
    w->base.save = w_playlist_save;
    w->base.load = w_playlist_load;
    w->base.init = w_playlist_init;
    w->base.initmenu = w_playlist_initmenu;
    gtk_widget_show (GTK_WIDGET (w->list));
    main_playlist_init (GTK_WIDGET (w->list));

    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        ddb_listview_show_header (DDB_LISTVIEW (w->list), 1);
    }
    else {
        ddb_listview_show_header (DDB_LISTVIEW (w->list), 0);
    }

    gtk_container_add (GTK_CONTAINER (w->base.widget), GTK_WIDGET (w->list));
    w_override_signals (w->base.widget, w);
    w->base.message = w_playlist_message;
    return (ddb_gtkui_widget_t*)w;
}

////// selection properties widget

gboolean
fill_selproperties_cb (gpointer data) {
    w_selproperties_t *w = data;
    DB_playItem_t **tracks = NULL;
    if (w->refresh_timeout) {
        g_source_remove (w->refresh_timeout);
        w->refresh_timeout = 0;
    }
    int numtracks = 0;
    deadbeef->pl_lock ();
    int nsel = deadbeef->pl_getselcount ();
    if (0 < nsel) {
        tracks = malloc (sizeof (DB_playItem_t *) * nsel);
        if (tracks) {
            int n = 0;
            DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    assert (n < nsel);
                    deadbeef->pl_item_ref (it);
                    tracks[n++] = it;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
            numtracks = nsel;
        }
        else {
            deadbeef->pl_unlock ();
            return FALSE;
        }
    }
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (w->tree)));
    trkproperties_fill_meta (store, tracks, numtracks);
    if (tracks) {
        for (int i = 0; i < numtracks; i++) {
            deadbeef->pl_item_unref (tracks[i]);
        }
        free (tracks);
        tracks = NULL;
        numtracks = 0;
    }
    deadbeef->pl_unlock ();
    return FALSE;
}

static void
selproperties_selection_changed (gpointer user_data)
{
    w_selproperties_t *selprop_w = user_data;
    if (selprop_w->refresh_timeout) {
        g_source_remove (selprop_w->refresh_timeout);
        selprop_w->refresh_timeout = 0;
    }
    selprop_w->refresh_timeout = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 10, fill_selproperties_cb, user_data, NULL);
}

static int
selproperties_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_TRACKINFOCHANGED:
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || p1 == DDB_PLAYLIST_CHANGE_SELECTION) {
            selproperties_selection_changed (w);
        }
        break;
    case DB_EV_PLAYLISTSWITCHED:
        selproperties_selection_changed (w);
        break;
    }
    return 0;
}

static void
w_selproperties_init (struct ddb_gtkui_widget_s *widget) {
    w_selproperties_t *w = (w_selproperties_t *)widget;
    w->refresh_timeout = 0;
    fill_selproperties_cb (widget);
}

static void
on_selproperties_showheaders_toggled (GtkCheckMenuItem *checkmenuitem, gpointer          user_data) {
    w_selproperties_t *w = user_data;
    int showheaders = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    deadbeef->conf_set_int ("gtkui.selection_properties.show_headers", showheaders);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), showheaders);
}

static void
w_selproperties_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_check_menu_item_new_with_mnemonic (_("Show Column Headers"));
    gtk_widget_show (item);
    int showheaders = deadbeef->conf_get_int ("gtkui.selection_properties.show_headers", 1);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), showheaders);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_selproperties_showheaders_toggled),
            w);
}

ddb_gtkui_widget_t *
w_selproperties_create (void) {
    w_selproperties_t *w = malloc (sizeof (w_selproperties_t));
    memset (w, 0, sizeof (w_selproperties_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_selproperties_init;
    w->base.message = selproperties_message;
    w->base.initmenu = w_selproperties_initmenu;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->tree = gtk_tree_view_new ();
    gtk_widget_show (w->tree);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_container_add (GTK_CONTAINER (scroll), w->tree);

    // NOTE: this list store must be compatible with trkproperties_fill_meta
    GtkListStore *store = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);

    GtkCellRenderer *rend1 = gtk_cell_renderer_text_new ();
    GtkCellRenderer *rend2 = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend1, "text", 0, NULL);
    gtk_tree_view_column_set_sizing (col1, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend2, "text", 1, NULL);
    gtk_tree_view_column_set_sizing (col2, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (w->tree), col1);
    gtk_tree_view_append_column (GTK_TREE_VIEW (w->tree), col2);
    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), TRUE);

    int showheaders = deadbeef->conf_get_int ("gtkui.selection_properties.show_headers", 1);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), showheaders);

    w_override_signals (w->base.widget, w);

    return (ddb_gtkui_widget_t *)w;
}

///// cover art display
static GdkPixbuf *
get_cover_art(const int width, const int height, void (*callback)(void *), void *user_data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track();
    if (!it) {
        return NULL;
    }

    deadbeef->pl_lock();
    const char *uri = deadbeef->pl_find_meta(it, ":URI");
    const char *album = deadbeef->pl_find_meta(it, "album");
    const char *artist = deadbeef->pl_find_meta(it, "artist");
    if (!album || !*album) {
        album = deadbeef->pl_find_meta(it, "title");
    }
    GdkPixbuf *pixbuf = get_cover_art_primary_by_size(uri, artist, album, width, height, callback, user_data);
    deadbeef->pl_unlock();
    deadbeef->pl_item_unref(it);
    return pixbuf;
}

static gboolean
coverart_invalidate_cb (void *user_data) {
    gtk_widget_queue_draw(user_data);
    return FALSE;
}

static void
coverart_invalidate (void *user_data) {
    g_idle_add(coverart_invalidate_cb, user_data);
}

static gboolean
coverart_unref_cb (void *user_data) {
    g_object_unref(user_data);
    return FALSE;
}

static void
coverart_unref (void *user_data) {
    g_idle_add(coverart_unref_cb, user_data);
}

static gboolean
coverart_load (void *user_data) {
    w_coverart_t *w = user_data;
    w->load_timeout_id = 0;
    GdkPixbuf *pixbuf = get_cover_art(w->widget_width, w->widget_height, coverart_invalidate, w->drawarea);
    if (pixbuf) {
        coverart_invalidate(w->drawarea);
        g_object_unref(pixbuf);
    }
    return FALSE;
}

static void
coverart_draw_cairo (GdkPixbuf *pixbuf, GtkAllocation *a, cairo_t *cr, const int filter) {
    const int pw = gdk_pixbuf_get_width(pixbuf);
    const int ph = gdk_pixbuf_get_height(pixbuf);
    cairo_rectangle(cr, 0, 0, a->width, a->height);
    if (pw > a->width || ph > a->height || (pw < a->width && ph < a->height)) {
        const double scale = min(a->width/(double)pw, a->height/(double)ph);
        cairo_translate(cr, (a->width - a->width*scale)/2., (a->height - a->height*scale)/2.);
        cairo_scale(cr, scale, scale);
        cairo_pattern_set_filter(cairo_get_source(cr), filter);
    }
    gdk_cairo_set_source_pixbuf(cr, pixbuf, (a->width - pw)/2., (a->height - ph)/2.);
    cairo_fill(cr);
}

static void
coverart_draw_anything (GtkAllocation *a, cairo_t *cr) {
    GdkPixbuf *pixbuf = get_cover_art(-1, -1, NULL, NULL);
    if (pixbuf) {
        coverart_draw_cairo(pixbuf, a, cr, CAIRO_FILTER_FAST);
        g_object_unref(pixbuf);
    }
}

static void
coverart_draw_exact (GtkAllocation *a, cairo_t *cr, void *user_data) {
    w_coverart_t *w = user_data;
    GdkPixbuf *pixbuf = get_cover_art(a->width, a->height, coverart_invalidate, w->drawarea);
    if (pixbuf) {
        coverart_draw_cairo(pixbuf, a, cr, CAIRO_FILTER_BEST);
        g_object_unref(pixbuf);
    }
    else {
        coverart_draw_anything(a, cr);
    }
}

static gboolean
coverart_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);
    if (a.width < 8 || a.height < 8) {
        return TRUE;
    }

    w_coverart_t *w = user_data;
    if (w->widget_height == a.height && w->widget_width == a.width) {
        coverart_draw_exact(&a, cr, user_data);
    }
    else {
        coverart_draw_anything(&a, cr);
        if (w->load_timeout_id) {
            g_source_remove(w->load_timeout_id);
        }
        w->load_timeout_id = g_timeout_add(w->widget_height == -1 ? 100 : 1000, coverart_load, user_data);
        w->widget_height = a.height;
        w->widget_width = a.width;
    }

    return TRUE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static gboolean
coverart_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = coverart_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}
#endif

static int
coverart_message (ddb_gtkui_widget_t *base, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_coverart_t *w = (w_coverart_t *)base;
    switch (id) {
    case DB_EV_PLAYLIST_REFRESH:
    case DB_EV_SONGCHANGED:
        coverart_invalidate(w->drawarea);
        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
            coverart_invalidate(w->drawarea);
        }
        break;
    case DB_EV_TRACKINFOCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it == ev->track) {
                coverart_invalidate(w->drawarea);
            }
            if (it) {
                deadbeef->pl_item_unref (it);
            }
        }
        break;
    }
    return 0;
}

static gboolean
coverart_destroy_drawarea (GtkWidget *drawarea, gpointer user_data) {
    w_coverart_t *w = (w_coverart_t *)user_data;
    if (w->load_timeout_id) {
        g_source_remove(w->load_timeout_id);
    }
    g_object_ref(drawarea);
    queue_cover_callback(coverart_unref, drawarea);
    return FALSE;
}

ddb_gtkui_widget_t *
w_coverart_create (void) {
    w_coverart_t *w = malloc (sizeof (w_coverart_t));
    memset (w, 0, sizeof (w_coverart_t));

    w->base.widget = gtk_event_box_new ();
    w->base.message = coverart_message;
    w->drawarea = gtk_drawing_area_new ();
    w->widget_height = -1;
    w->widget_width = -1;
    w->load_timeout_id = 0;
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (coverart_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (coverart_draw), w);
#endif
    g_signal_connect ((gpointer) w->drawarea, "destroy", G_CALLBACK (coverart_destroy_drawarea), w);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

///// scope vis
void
w_scope_destroy (ddb_gtkui_widget_t *w) {
    w_scope_t *s = (w_scope_t *)w;
    deadbeef->vis_waveform_unlisten (w);
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (s->surf) {
        cairo_surface_destroy (s->surf);
        s->surf = NULL;
    }
    if (s->samples) {
        free (s->samples);
        s->samples = NULL;
    }
    if (s->mutex) {
        deadbeef->mutex_free (s->mutex);
        s->mutex = 0;
    }
}

gboolean
w_scope_draw_cb (void *data) {
    w_scope_t *s = data;
    gtk_widget_queue_draw (s->drawarea);
    return TRUE;
}

static void
scope_wavedata_listener (void *ctx, const ddb_audio_data_t *data) {
    w_scope_t *w = ctx;
    if (w->nsamples != w->resized) {
        deadbeef->mutex_lock (w->mutex);
        float *oldsamples = w->samples;
        int oldnsamples = w->nsamples;
        w->samples = NULL;
        w->nsamples = w->resized;
        if (w->nsamples > 0) {
            w->samples = malloc (sizeof (float) * w->nsamples);
            memset (w->samples, 0, sizeof (float) * w->nsamples);
            if (oldsamples) {
                int n = min (oldnsamples, w->nsamples);
                memcpy (w->samples + w->nsamples - n, oldsamples + oldnsamples - n, n * sizeof (float));
            }
        }
        if (oldnsamples) {
            free (oldsamples);
        }
        deadbeef->mutex_unlock (w->mutex);
    }

    if (w->samples && w->nsamples) {
        // append
        int nsamples = data->nframes / data->fmt->channels;
        float ratio = data->fmt->samplerate / 44100.f;
        int size = nsamples / ratio;

        int sz = min (w->nsamples, size);
        int n = w->nsamples-sz;

        memmove (w->samples, w->samples + sz, n * sizeof (float));
        float pos = 0;
        for (int i = 0; i < sz && pos < nsamples; i++, pos += ratio) {
            w->samples[n + i] = data->data[ftoi(pos * data->fmt->channels) * data->fmt->channels + 0];
            for (int j = 1; j < data->fmt->channels; j++) {
                w->samples[n + i] += data->data[ftoi(pos * data->fmt->channels) * data->fmt->channels + j];
            }
            w->samples[n+i] /= data->fmt->channels;
        }
    }
}

static inline void
_draw_vline (uint8_t *data, int stride, int x0, int y0, int y1) {
    if (y0 > y1) {
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
        y1--;
    }
    else if (y0 < y1) {
        y0++;
    }
    while (y0 <= y1) {
        uint32_t *ptr = (uint32_t*)&data[y0*stride+x0*4];
        *ptr = 0xffffffff;
        y0++;
    }
}

gboolean
scope_draw_cairo (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    w_scope_t *w = user_data;

    if (!w->surf || cairo_image_surface_get_width (w->surf) != a.width || cairo_image_surface_get_height (w->surf) != a.height) {
        if (w->surf) {
            cairo_surface_destroy (w->surf);
            w->surf = NULL;
        }
        w->surf = cairo_image_surface_create (CAIRO_FORMAT_RGB24, a.width, a.height);
    }

    int nsamples = a.width;
    if (w->nsamples != nsamples) {
        w->resized = nsamples;
    }
    cairo_surface_flush (w->surf);
    unsigned char *data = cairo_image_surface_get_data (w->surf);
    if (!data) {
        return FALSE;
    }
    int stride = cairo_image_surface_get_stride (w->surf);
    memset (data, 0, a.height * stride);
    if (w->samples && a.height > 2) {
        deadbeef->mutex_lock (w->mutex);
        float h = a.height;
        if (h > 50) {
            h -= 20;
        }
        if (h > 100) {
            h -= 40;
        }
        h /= 2;
        float hh = a.height/2.f;

        int prev_y = w->samples[0] * h + hh;

        int n = min (w->nsamples, a.width);
        for (int i = 1; i < n; i++) {
            int y = ftoi (w->samples[i] * h + hh);
            if (y < 0) {
                y = 0;
            }
            if (y >= a.height) {
                y = a.height-1;
            }
            _draw_vline (data, stride, i, prev_y, y);
            prev_y = y;
        }
        if (n < a.width) {
            memset (data + a.height / 2 * stride + n * 4, 0xff, (a.width-n)*4);
        }
        deadbeef->mutex_unlock (w->mutex);
    }
    else if (a.height > 0) {
        memset (data + a.height / 2 *stride, 0xff, stride);
    }
    cairo_surface_mark_dirty (w->surf);
    cairo_save (cr);
    cairo_set_source_surface (cr, w->surf, 0, 0);
    cairo_rectangle (cr, 0, 0, a.width, a.height);
    cairo_fill (cr);
    cairo_restore (cr);

    return FALSE;
}

gboolean
scope_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    return scope_draw_cairo (widget, cr, user_data);
}

gboolean
scope_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = scope_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

void
w_scope_init (ddb_gtkui_widget_t *w) {
    w_scope_t *s = (w_scope_t *)w;
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    s->drawtimer = g_timeout_add (33, w_scope_draw_cb, w);
}

ddb_gtkui_widget_t *
w_scope_create (void) {
    w_scope_t *w = malloc (sizeof (w_scope_t));
    memset (w, 0, sizeof (w_scope_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_scope_init;
    w->base.destroy  = w_scope_destroy;
    w->drawarea = gtk_drawing_area_new ();

    w->mutex = deadbeef->mutex_create ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (scope_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (scope_draw), w);
#endif
    w_override_signals (w->base.widget, w);
    deadbeef->vis_waveform_listen(w, scope_wavedata_listener);
    return (ddb_gtkui_widget_t *)w;
}

///// spectrum vis
void
w_spectrum_destroy (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    deadbeef->vis_spectrum_unlisten (w);
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (s->surf) {
        cairo_surface_destroy (s->surf);
        s->surf = NULL;
    }

    ddb_analyzer_dealloc(&s->analyzer);
    ddb_analyzer_draw_data_dealloc(&s->draw_data);
    free (s->input_data.data);
    s->input_data.data = NULL;

    if (s->mutex) {
        deadbeef->mutex_free (s->mutex);
        s->mutex = 0;
    }
}

gboolean
w_spectrum_draw_cb (void *data) {
    w_spectrum_t *s = data;
    gtk_widget_queue_draw (s->drawarea);
    return TRUE;
}


static void
spectrum_audio_listener (void *ctx, const ddb_audio_data_t *data) {
    w_spectrum_t *w = ctx;

    deadbeef->mutex_lock (w->mutex);
    // copy the input data for later consumption
    if (w->input_data.nframes != data->nframes) {
        free (w->input_data.data);
        w->input_data.data = malloc (data->nframes * data->fmt->channels * sizeof (float));
        w->input_data.nframes = data->nframes;
    }
    memcpy (w->input_data.fmt, data->fmt, sizeof (ddb_waveformat_t));
    memcpy (w->input_data.data, data->data, data->nframes * data->fmt->channels * sizeof (float));
    deadbeef->mutex_unlock (w->mutex);
}

static void
_spectrum_run (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    if (s->drawtimer > 0) {
        return;
    }
    s->drawtimer = g_timeout_add (33, w_spectrum_draw_cb, w);
}

static void
_spectrum_stop (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    if (s->drawtimer > 0) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
}

static gboolean
spectrum_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    w_spectrum_t *w = user_data;

    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_paint (cr);

    if (w->input_data.nframes == 0) {
        return FALSE;
    }

    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    deadbeef->mutex_lock (w->mutex);
        ddb_analyzer_process(&w->analyzer, w->input_data.fmt->samplerate, w->input_data.fmt->channels, w->input_data.data, w->input_data.nframes);
        ddb_analyzer_tick(&w->analyzer);
        ddb_analyzer_get_draw_data(&w->analyzer, a.width, a.height, &w->draw_data);
    deadbeef->mutex_unlock(w->mutex);

    // TODO: draw grid and labels

    // bars
    ddb_analyzer_draw_bar_t *bar = w->draw_data.bars;
    cairo_set_source_rgb(cr, 0, 0.5, 1);
    for (int i = 0; i < w->draw_data.bar_count; i++, bar++) {
        if (w->analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
            cairo_move_to(cr, bar->xpos, a.height-bar->bar_height);
            cairo_line_to(cr, bar->xpos, a.height-1);
        }
        else {
            cairo_rectangle(cr, bar->xpos, a.height-bar->bar_height, w->draw_data.bar_width, bar->bar_height);
        }
    }

    if (w->analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
        cairo_stroke(cr);
    }
    else {
        cairo_fill(cr);
    }

    // peaks
    bar = w->draw_data.bars;
    cairo_set_source_rgb(cr, 0.65, 0.78, 1);
    for (int i = 0; i < w->draw_data.bar_count; i++, bar++) {
        cairo_rectangle(cr, bar->xpos, a.height-bar->peak_ypos-1, w->draw_data.bar_width, 1);
    }
    cairo_fill(cr);

    return FALSE;
}

gboolean
spectrum_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = spectrum_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

void
w_spectrum_init (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    ddb_playback_state_t playback_state = deadbeef->get_output ()->state ();

    if (playback_state == DDB_PLAYBACK_STATE_PLAYING) { 
        _spectrum_run (w);
    }
}

static int
w_spectrum_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SONGCHANGED: {
            ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
            if (!ev->to) {
                _spectrum_stop (w);
            }
        }
        break;
    case DB_EV_SONGSTARTED:
        _spectrum_run (w);
        break;
    case DB_EV_PAUSED:
        if (p1) {
            _spectrum_stop (w);
        }
        else {
            _spectrum_run (w);
        }
        break;
    }
    return 0;
}

ddb_gtkui_widget_t *
w_spectrum_create (void) {
    w_spectrum_t *w = malloc (sizeof (w_spectrum_t));
    memset (w, 0, sizeof (w_spectrum_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_spectrum_init;
    w->base.destroy  = w_spectrum_destroy;
    w->base.message = w_spectrum_message;
    w->drawarea = gtk_drawing_area_new ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (spectrum_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (spectrum_draw), w);
#endif
    w_override_signals (w->base.widget, w);

    w->input_data.fmt = &w->fmt;

    // using the analyzer framework
    ddb_analyzer_init(&w->analyzer);
    w->analyzer.db_lower_bound = -80;
    w->analyzer.peak_hold = 10;
    w->analyzer.view_width = 1000;
    w->analyzer.fractional_bars = 0;
    w->analyzer.octave_bars_step = 2;
    w->analyzer.max_of_stereo_data = 1;
    w->analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;

    w->mutex = deadbeef->mutex_create ();

    deadbeef->vis_spectrum_listen2 (w, spectrum_audio_listener);
    return (ddb_gtkui_widget_t *)w;
}

// hbox and vbox
static const char *
w_hvbox_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "hbox") && strcmp (type, "vbox")) {
        return NULL;
    }
    w_hvbox_t *hvbox = (w_hvbox_t *)w;

    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s,key,val);

        if (!strcmp (key, "expand")) {
            const char *s = val;
            int n = 0;
            hvbox->expand = 0;
            char t[MAX_TOKEN];
            while (n < 64) {
                s = gettoken (s, t);
                if (!s) {
                    break;
                }
                if (atoi (t)) {
                    hvbox->expand |= (1ULL << n);
                }
                n++;
            }
        }
        else if (!strcmp (key, "fill")) {
            const char *s = val;
            int n = 0;
            hvbox->fill = 0;
            char t[MAX_TOKEN];
            while (n < 64) {
                s = gettoken (s, t);
                if (!s) {
                    break;
                }
                if (atoi (t)) {
                    hvbox->fill |= (1ULL << n);
                }
                n++;
            }
        }
        else if (!strcmp (key, "homogeneous")) {
            hvbox->homogeneous = atoi (val) ? 1 : 0;
        }
    }

    return s;
}

typedef struct {
    GtkWidget *hvbox;
    char expand[150];
    char fill[150];
} w_hvbox_save_info_t;

static void
save_hvbox_packing (GtkWidget *child, gpointer user_data) {
    w_hvbox_save_info_t *info = user_data;
    gboolean expand;
    gboolean fill;
    guint padding;
    GtkPackType pack_type;
    gtk_box_query_child_packing (GTK_BOX (info->hvbox), child, &expand, &fill, &padding, &pack_type);
    char s[10];
    snprintf (s, sizeof (s), info->expand[0] ? " %d" : "%d", expand);
    strncat (info->expand, s, sizeof (info->expand) - strlen (info->expand));

    snprintf (s, sizeof (s), info->fill[0] ? " %d" : "%d", fill);
    strncat (info->fill, s, sizeof (info->fill) - strlen (info->fill));
}

static void
w_hvbox_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    char save[1000];

    w_hvbox_save_info_t info;
    memset (&info, 0, sizeof (info));
    info.hvbox = ((w_hvbox_t *)w)->box;
    gtk_container_foreach (GTK_CONTAINER (((w_hvbox_t *)w)->box), save_hvbox_packing, &info);
    gboolean homogeneous = gtk_box_get_homogeneous (GTK_BOX (((w_hvbox_t *)w)->box));

    snprintf (save, sizeof (save), " expand=\"%s\" fill=\"%s\" homogeneous=%d", info.expand, info.fill, homogeneous);
    strncat (s, save, sz);
}

typedef struct {
    w_hvbox_t *w;
    int n;
} hwbox_init_info_t;

static void
hvbox_init_child (GtkWidget *child, void *user_data) {
    hwbox_init_info_t *info = user_data;
    gboolean expand;
    gboolean fill;
    guint padding;
    GtkPackType pack_type;
    gtk_box_query_child_packing (GTK_BOX (info->w->box), child, &expand, &fill, &padding, &pack_type);
    expand = (info->w->expand & (1<<info->n)) ? TRUE : FALSE;
    fill = (info->w->fill & (1<<info->n)) ? TRUE : FALSE;
    gtk_box_set_child_packing (GTK_BOX (info->w->box), child, expand, fill, padding, pack_type);
    info->n++;
}

static void
w_hvbox_init (struct ddb_gtkui_widget_s *w) {
    w_hvbox_t *hvbox = (w_hvbox_t *)w;
    hwbox_init_info_t info;
    info.w = hvbox;
    info.n = 0;
    gtk_container_foreach (GTK_CONTAINER (hvbox->box), hvbox_init_child, &info);
    gtk_box_set_homogeneous (GTK_BOX (hvbox->box), hvbox->homogeneous ? TRUE : FALSE);
}

static void
w_hvbox_append (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child) {
    w_hvbox_t *b = (w_hvbox_t *)container;
    gtk_box_pack_start (GTK_BOX (b->box), child->widget, (child->flags & DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE) ? FALSE : TRUE, TRUE, 0);
    gtk_widget_show (child->widget);
}

static void
w_hvbox_remove (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child) {
    w_hvbox_t *b = (w_hvbox_t *)container;
    gtk_container_remove (GTK_CONTAINER (b->box), child->widget);
}

static void
w_hvbox_replace (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child, struct ddb_gtkui_widget_s *newchild) {
    w_hvbox_t *b = (w_hvbox_t *)container;
    ddb_gtkui_widget_t *c;
    ddb_gtkui_widget_t *prev = NULL;
    int n = 0;
    for (c = container->children; c; prev = c, c = c->next, n++) {
        if (c == child) {
            break;
        }
    }

    if (!c) {
        return;
    }

    if (prev) {
        prev->next = newchild;
    }
    else {
        container->children = newchild;
    }
    newchild->next = c->next;
    newchild->parent = container;
    w_remove (container, c);
    w_destroy (c);

    gtk_box_pack_start (GTK_BOX (b->box), newchild->widget, TRUE, TRUE, 0);
    gtk_widget_show (newchild->widget);
    gtk_box_reorder_child (GTK_BOX (b->box), newchild->widget, n);

}

static void
on_hvbox_expand (GtkMenuItem *menuitem, gpointer user_data) {
    w_append ((ddb_gtkui_widget_t*)user_data, w_create ("placeholder"));
}

static void
on_hvbox_shrink (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *w = (ddb_gtkui_widget_t *)user_data;
    ddb_gtkui_widget_t *c;
    for (c = w->children; c && c->next; c = c->next);
    if (c) {
        w_remove (w, c);
        w_destroy (c);
    }
    if (!w->children) {
        w_append (w, w_create ("placeholder"));
    }
}

static void
on_hvbox_toggle_homogeneous (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_hvbox_t *w = user_data;
    gboolean hmg = gtk_box_get_homogeneous (GTK_BOX (((w_hvbox_t *)w)->box));
    gtk_box_set_homogeneous (GTK_BOX (((w_hvbox_t *)w->box)), hmg ? FALSE : TRUE);
}


static void
w_hvbox_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_("Expand Box by 1 Item"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate", G_CALLBACK (on_hvbox_expand), w);

    item = gtk_menu_item_new_with_mnemonic (_("Shrink Box by 1 Item"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate", G_CALLBACK (on_hvbox_shrink), w);

    item = gtk_check_menu_item_new_with_mnemonic (_("Homogeneous"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), gtk_box_get_homogeneous (GTK_BOX (((w_hvbox_t *)w)->box)));
    g_signal_connect ((gpointer) item, "toggled", G_CALLBACK (on_hvbox_toggle_homogeneous), w);
}

static void
on_hvbox_child_toggle_expand (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    ddb_gtkui_widget_t *w = user_data;
    w_hvbox_t *box = (w_hvbox_t *)w->parent;
    gboolean expand, fill;
    guint padding;
    GtkPackType packtype;
    gtk_box_query_child_packing (GTK_BOX (box->box), w->widget, &expand, &fill, &padding, &packtype);
    gtk_box_set_child_packing (GTK_BOX (box->box), w->widget, !expand, fill, padding, packtype);
}

static void
on_hvbox_child_toggle_fill (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    ddb_gtkui_widget_t *w = user_data;
    w_hvbox_t *box = (w_hvbox_t *)w->parent;
    gboolean expand, fill;
    guint padding;
    GtkPackType packtype;
    gtk_box_query_child_packing (GTK_BOX (box->box), w->widget, &expand, &fill, &padding, &packtype);
    gtk_box_set_child_packing (GTK_BOX (box->box), w->widget, expand, !fill, padding, packtype);
}

static void
w_hvbox_initchildmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    w_hvbox_t *box = (w_hvbox_t *)w->parent;

    gboolean expand, fill;
    guint padding;
    GtkPackType packtype;
    gtk_box_query_child_packing (GTK_BOX (box->box), w->widget, &expand, &fill, &padding, &packtype);

    GtkWidget *item;

    item = gtk_check_menu_item_new_with_mnemonic (_("Expand"));
    gtk_widget_show (item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), expand);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_hvbox_child_toggle_expand),
            w);

    item = gtk_check_menu_item_new_with_mnemonic (_("Fill"));
    gtk_widget_show (item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), fill);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_hvbox_child_toggle_fill),
            w);
}

GtkWidget *
w_hvbox_get_container (struct ddb_gtkui_widget_s *b) {
    return ((w_hvbox_t *)b)->box;
}

ddb_gtkui_widget_t *
w_hbox_create (void) {
    w_hvbox_t *w = malloc (sizeof (w_hvbox_t));
    memset (w, 0, sizeof (w_hvbox_t));
    w->base.widget = gtk_event_box_new ();
    w->base.append = w_hvbox_append;
    w->base.remove = w_hvbox_remove;
    w->base.replace = w_hvbox_replace;
    w->base.initmenu = w_hvbox_initmenu;
    w->base.initchildmenu = w_hvbox_initchildmenu;
    w->base.load = w_hvbox_load;
    w->base.save = w_hvbox_save;
    w->base.init = w_hvbox_init;
    w->base.get_container = w_hvbox_get_container;
    w->box = gtk_hbox_new (TRUE, 3);
    w->homogeneous = 1;
    w->expand = -1;
    w->fill = -1;
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);

    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

ddb_gtkui_widget_t *
w_vbox_create (void) {
    w_hvbox_t *w = malloc (sizeof (w_hvbox_t));
    memset (w, 0, sizeof (w_hvbox_t));
    w->base.widget = gtk_event_box_new ();
    w->base.append = w_hvbox_append;
    w->base.remove = w_hvbox_remove;
    w->base.replace = w_hvbox_replace;
    w->base.get_container = w_hvbox_get_container;
    w->base.initmenu = w_hvbox_initmenu;
    w->base.initchildmenu = w_hvbox_initchildmenu;
    w->base.load = w_hvbox_load;
    w->base.save = w_hvbox_save;
    w->base.init = w_hvbox_init;
    w->box = gtk_vbox_new (TRUE, 3);
    w->homogeneous = 1;
    w->expand = -1;
    w->fill = -1;
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);

    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

// button widget
static const char *
w_button_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "button")) {
        return NULL;
    }
    w_button_t *b = (w_button_t *)w;
    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s, key, val);
        if (!strcmp (key, "color")) {
            int red, green, blue;
            if (3 == sscanf (val, "#%02x%02x%02x", &red, &green, &blue)) {
                b->color.red = red << 8;
                b->color.green = green << 8;
                b->color.blue = blue << 8;
            }
        }
        else if (!strcmp (key, "textcolor")) {
            int red, green, blue;
            if (3 == sscanf (val, "#%02x%02x%02x", &red, &green, &blue)) {
                b->textcolor.red = red << 8;
                b->textcolor.green = green << 8;
                b->textcolor.blue = blue << 8;
            }
        }
        else if (!strcmp (key, "icon")) {
            b->icon = val[0] ? strdup (val) : NULL;
        }
        else if (!strcmp (key, "label")) {
            b->label = strdup (val);
        }
        else if (!strcmp (key, "action")) {
            b->action = val[0] ? strdup (val) : NULL;
        }
        else if (!strcmp (key, "action_ctx")) {
            b->action_ctx = atoi (val);
        }
        else if (!strcmp (key, "use_color")) {
            b->use_color = atoi (val);
        }
        else if (!strcmp (key, "use_textcolor")) {
            b->use_textcolor = atoi (val);
        }
    }

    return s;
}

static void
w_button_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    char save[1000] = "";
    char *pp = save;
    int ss = sizeof (save);
    int n;

    w_button_t *b = (w_button_t *)w;
    n = snprintf (pp, ss, " color=\"#%02x%02x%02x\"", b->color.red>>8, b->color.green>>8, b->color.blue>>8);
    ss -= n;
    pp += n;
    n = snprintf (pp, ss, " textcolor=\"#%02x%02x%02x\"", b->textcolor.red>>8, b->textcolor.green>>8, b->textcolor.blue>>8);
    ss -= n;
    pp += n;
    if (b->icon) {
        n = snprintf (pp, ss, " icon=\"%s\"", b->icon);
        ss -= n;
        pp += n;
    }
    if (b->label) {
        n = snprintf (pp, ss, " label=\"%s\"", b->label);
        ss -= n;
        pp += n;
    }
    if (b->action) {
        n = snprintf (pp, ss, " action=\"%s\"", b->action);
        ss -= n;
        pp += n;
    }
    if (b->action_ctx) {
        n = snprintf (pp, ss, " action_ctx=%d", (int)b->action_ctx);
        ss -= n;
        pp += n;
    }

    n = snprintf (pp, ss, " use_color=%d", (int)b->use_color);
    ss -= n;
    pp += n;

    n = snprintf (pp, ss, " use_textcolor=%d", (int)b->use_textcolor);
    ss -= n;
    pp += n;

    strncat (s, save, sz);
}

static void
on_button_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    w_button_t *w = user_data;
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;
    for (i = 0; plugins[i]; i++) {
        if (!plugins[i]->get_actions) {
            continue;
        }
        DB_plugin_action_t *acts = plugins[i]->get_actions (NULL);
        while (acts) {
            if (!strcmp (acts->name, w->action)) {
                if (acts->callback) {
                    gtkui_exec_action_14 (acts, -1);
                }
                else if (acts->callback2) {
                    acts->callback2 (acts, w->action_ctx);
                }
                return;
            }
            acts = acts->next;
        }
    }
}

static void
w_button_init (ddb_gtkui_widget_t *ww) {
    w_button_t *w = (w_button_t *)ww;

    // clean before re-creating
    if (w->button) {
        gtk_widget_destroy (w->button);
        w->button = NULL;
    }

    w->button = gtk_button_new ();
    gtk_widget_show (w->button);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->button);

    GtkWidget *alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_widget_show (alignment);
    gtk_container_add (GTK_CONTAINER (w->button), alignment);

    GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (alignment), hbox);

    if (w->icon) {
        GtkWidget *image = gtk_image_new_from_stock (w->icon, GTK_ICON_SIZE_BUTTON);
        gtk_widget_show (image);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    }

    GtkWidget *label = gtk_label_new_with_mnemonic (w->label ? w->label : _("Button"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    if (w->use_color) {
        gtk_widget_modify_bg (w->button, GTK_STATE_NORMAL, &w->color);
    }

    if (w->use_textcolor) {
        gtk_widget_modify_fg (label, GTK_STATE_NORMAL, &w->textcolor);
    }

    if (w->action) {
        g_signal_connect ((gpointer) w->button, "clicked",
                G_CALLBACK (on_button_clicked),
                w);
    }

    w_override_signals (w->button, w);
}

static void
w_button_destroy (ddb_gtkui_widget_t *w) {
    w_button_t *b = (w_button_t *)w;
    if (b->icon) {
        free (b->icon);
    }
    if (b->label) {
        free (b->label);
    }
    if (b->action) {
        free (b->action);
    }
}

static void
on_button_set_action_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
    w_button_t *b = user_data;
    GtkWidget *dlg = create_select_action ();
    GtkWidget *treeview = lookup_widget (dlg, "actions");
    init_action_tree (treeview, b->action, b->action_ctx);
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        if (b->action) {
            free (b->action);
            b->action = NULL;
        }
        b->action_ctx = -1;
        GtkTreePath *path;
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (treeview), &path, NULL);
        GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        GtkTreeIter iter;
        if (path && gtk_tree_model_get_iter (model, &iter, path)) {
            GValue val = {0,};
            gtk_tree_model_get_value (model, &iter, 1, &val);
            const gchar *name = g_value_get_string (&val);
            GValue val_ctx = {0,};
            gtk_tree_model_get_value (model, &iter, 2, &val_ctx);
            int ctx = g_value_get_int (&val_ctx);
            if (name && ctx >= 0) {
                b->action = strdup (name);
                b->action_ctx = ctx;
            }
        }
        set_button_action_label (b->action, b->action_ctx, GTK_WIDGET (button));
    }
    gtk_widget_destroy (dlg);
}

static void
on_button_config (GtkMenuItem *menuitem, gpointer user_data) {
    w_button_t *b = user_data;
    GtkWidget *dlg = create_button_properties ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    GtkWidget *color = lookup_widget (dlg, "color");
    GtkWidget *use_color = lookup_widget (dlg, "use_color");
    GtkWidget *textcolor = lookup_widget (dlg, "textcolor");
    GtkWidget *use_textcolor = lookup_widget (dlg, "use_textcolor");
    GtkWidget *label = lookup_widget (dlg, "label");
    GtkWidget *action = lookup_widget (dlg, "action");
    GtkWidget *icon = lookup_widget (dlg, "icon");
    gtk_color_button_set_color (GTK_COLOR_BUTTON (color), &b->color);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_color), b->use_color);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (textcolor), &b->textcolor);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_textcolor), b->use_textcolor);
    gtk_entry_set_text (GTK_ENTRY (label), b->label ? b->label : _("Button"));
    set_button_action_label (b->action, b->action_ctx, action);
    g_signal_connect ((gpointer) action, "clicked",
            G_CALLBACK (on_button_set_action_clicked),
            user_data);

    GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, NULL, 1, _("None"), -1);
    int sel = 0;
    for (int n = 0; GtkNamedIcons[n]; n++) {
        gtk_list_store_append (store, &iter);

        GtkStockItem it;
        if (gtk_stock_lookup (GtkNamedIcons[n], &it)) {
            char *s = strdupa (it.label);
            for (char *c = s; *c; c++) {
                if (*c == '_') {
                    memmove (c, c+1, strlen (c));
                    c--;
                }
            }
            gtk_list_store_set (store, &iter, 0, GtkNamedIcons[n], 1, s, -1);
        }
        else {
            gtk_list_store_set (store, &iter, 0, GtkNamedIcons[n], 1, GtkNamedIcons[n], -1);
        }

        if (b->icon && !strcmp (GtkNamedIcons[n], b->icon)) {
            sel = n+1;
        }
    }

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (icon));
    GtkCellRenderer *renderer;
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon), renderer, FALSE );
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon), renderer, "stock-id", 0, NULL );

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon), renderer, FALSE );
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon), renderer, "text", 1, NULL );


    gtk_combo_box_set_model (GTK_COMBO_BOX (icon), GTK_TREE_MODEL (store));

    gtk_combo_box_set_active (GTK_COMBO_BOX (icon), sel);

    for (;;) {
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY) {
            gtk_color_button_get_color (GTK_COLOR_BUTTON (color), &b->color);
            b->use_color = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_color));
            gtk_color_button_get_color (GTK_COLOR_BUTTON (textcolor), &b->textcolor);
            b->use_textcolor = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_textcolor));
            const char *l = gtk_entry_get_text (GTK_ENTRY (label));
            if (b->label) {
                free (b->label);
                b->label = NULL;
            }
            b->label = strdup (l);

            const char *ic = NULL;
            int sel = gtk_combo_box_get_active (GTK_COMBO_BOX (icon));
            if (sel >= 1) {
                ic = GtkNamedIcons[sel-1];
            }
            if (b->icon) {
                free (b->icon);
                b->icon = NULL;
            }
            if (ic) {
                b->icon = strdup (ic);
            }

            w_button_init (user_data);
        }
        if (response == GTK_RESPONSE_APPLY) {
            continue;
        }
        break;
    }
    gtk_widget_destroy (dlg);
}

static void
w_button_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_("Configure Button"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate", G_CALLBACK (on_button_config), w);
}


ddb_gtkui_widget_t *
w_button_create (void) {
    w_button_t *w = malloc (sizeof (w_button_t));
    memset (w, 0, sizeof (w_button_t));
    w->base.widget = gtk_event_box_new ();
    w->base.load = w_button_load;
    w->base.save = w_button_save;
    w->base.init = w_button_init;
    w->base.destroy = w_button_destroy;
    w->base.initmenu = w_button_initmenu;
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

// seekbar
static gboolean
redraw_seekbar_cb (gpointer data) {
    w_seekbar_t *w = data;
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    gtk_widget_queue_draw (w->seekbar);
    return FALSE;
}

static gboolean
seekbar_frameupdate (gpointer data) {
    w_seekbar_t *w = data;
    DB_output_t *output = deadbeef->get_output ();
    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    float songpos = w->last_songpos;
    float duration = track ? deadbeef->pl_get_item_duration (track) : -1;
    if (!output || (output->state () == DDB_PLAYBACK_STATE_STOPPED || !track)) {
        songpos = 0;
    }
    else {
        songpos = deadbeef->streamer_get_playpos ();
    }
    // translate pos to seekbar pixels
    songpos /= duration;
    GtkAllocation a;
    gtk_widget_get_allocation (w->seekbar, &a);
    songpos *= a.width;
    if (fabs (songpos - w->last_songpos) > 0.01) {
        gtk_widget_queue_draw (w->seekbar);
        w->last_songpos = songpos;
    }
    if (track) {
        deadbeef->pl_item_unref (track);
    }
    return TRUE;
}

static void
w_seekbar_init (ddb_gtkui_widget_t *base) {
    w_seekbar_t *w = (w_seekbar_t *)base;
    if (w->timer) {
        g_source_remove (w->timer);
        w->timer = 0;
    }

    w->timer = g_timeout_add (1000/gtkui_get_gui_refresh_rate (), seekbar_frameupdate, w);
}

static int
w_seekbar_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        w_seekbar_init (w);
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_bar_override_conf(conf_str) || gtkui_bar_colors_conf(conf_str)) {
                g_idle_add (redraw_seekbar_cb, w);
            }
        }
        break;
    case DB_EV_SONGCHANGED:
        g_idle_add (redraw_seekbar_cb, w);
        break;
    }
    return 0;
}

static void
w_seekbar_destroy (ddb_gtkui_widget_t *wbase) {
    w_seekbar_t *w = (w_seekbar_t *)wbase;
    if (w->timer) {
        g_source_remove (w->timer);
        w->timer = 0;
    }
}

ddb_gtkui_widget_t *
w_seekbar_create (void) {
    w_seekbar_t *w = malloc (sizeof (w_seekbar_t));
    memset (w, 0, sizeof (w_seekbar_t));
    w->base.widget = gtk_event_box_new ();
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_add_events (GTK_WIDGET (w->base.widget), GDK_SCROLL_MASK);
#endif
    w->base.message = w_seekbar_message;
    w->base.destroy = w_seekbar_destroy;
    w->base.init = w_seekbar_init;
    w->seekbar = ddb_seekbar_new ();
    gtk_widget_set_size_request (w->base.widget, 20, 16);
    w->last_songpos = -1;
    ddb_seekbar_init_signals (DDB_SEEKBAR (w->seekbar), w->base.widget);
    gtk_widget_show (w->seekbar);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->seekbar);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

// play toolbar
ddb_gtkui_widget_t *
w_playtb_create (void) {
    w_playtb_t *w = malloc (sizeof (w_playtb_t));
    memset (w, 0, sizeof (w_playtb_t));
    w->base.widget = gtk_hbox_new (FALSE, 0);
    w->base.flags = DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE;
    gtk_widget_show (w->base.widget);

    GtkWidget *stopbtn;
    GtkWidget *image128;
    GtkWidget *playbtn;
    GtkWidget *image2;
    GtkWidget *pausebtn;
    GtkWidget *image3;
    GtkWidget *prevbtn;
    GtkWidget *image4;
    GtkWidget *nextbtn;
    GtkWidget *image5;


    stopbtn = gtk_button_new ();
    gtk_widget_show (stopbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), stopbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus(stopbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (stopbtn), GTK_RELIEF_NONE);

    image128 = gtk_image_new_from_stock ("gtk-media-stop", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image128);
    gtk_container_add (GTK_CONTAINER (stopbtn), image128);

    playbtn = gtk_button_new ();
    gtk_widget_show (playbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), playbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus(playbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (playbtn), GTK_RELIEF_NONE);

    image2 = gtk_image_new_from_stock ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image2);
    gtk_container_add (GTK_CONTAINER (playbtn), image2);

    pausebtn = gtk_button_new ();
    gtk_widget_show (pausebtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), pausebtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus(pausebtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (pausebtn), GTK_RELIEF_NONE);

    image3 = gtk_image_new_from_stock ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image3);
    gtk_container_add (GTK_CONTAINER (pausebtn), image3);

    prevbtn = gtk_button_new ();
    gtk_widget_show (prevbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), prevbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus(prevbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (prevbtn), GTK_RELIEF_NONE);

    image4 = gtk_image_new_from_stock ("gtk-media-previous", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image4);
    gtk_container_add (GTK_CONTAINER (prevbtn), image4);

    nextbtn = gtk_button_new ();
    gtk_widget_show (nextbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), nextbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus(nextbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (nextbtn), GTK_RELIEF_NONE);

    image5 = gtk_image_new_from_stock ("gtk-media-next", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image5);
    gtk_container_add (GTK_CONTAINER (nextbtn), image5);
    w_override_signals (w->base.widget, w);

    g_signal_connect ((gpointer) stopbtn, "clicked",
            G_CALLBACK (on_stopbtn_clicked),
            NULL);
    g_signal_connect ((gpointer) playbtn, "clicked",
            G_CALLBACK (on_playbtn_clicked),
            NULL);
    g_signal_connect ((gpointer) pausebtn, "clicked",
            G_CALLBACK (on_pausebtn_clicked),
            NULL);
    g_signal_connect ((gpointer) prevbtn, "clicked",
            G_CALLBACK (on_prevbtn_clicked),
            NULL);
    g_signal_connect ((gpointer) nextbtn, "clicked",
            G_CALLBACK (on_nextbtn_clicked),
            NULL);
    return (ddb_gtkui_widget_t*)w;
}

// volumebar
static gboolean
redraw_volumebar_cb (gpointer data) {
    w_volumebar_t *w = data;
    gtk_widget_queue_draw (w->volumebar);
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (w->volumebar, s);
    gtk_widget_trigger_tooltip_query (w->volumebar);
    return FALSE;
}

static int
w_volumebar_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_bar_override_conf(conf_str) || gtkui_bar_colors_conf(conf_str)) {
                g_idle_add (redraw_volumebar_cb, w);
            }
        }
        break;
    case DB_EV_VOLUMECHANGED:
        g_idle_add (redraw_volumebar_cb, w);
        break;
    }
    return 0;
}

ddb_gtkui_widget_t *
w_volumebar_create (void) {
    w_volumebar_t *w = malloc (sizeof (w_volumebar_t));
    memset (w, 0, sizeof (w_volumebar_t));
    w->base.widget = gtk_event_box_new ();
    w->base.message = w_volumebar_message;
    w->volumebar = ddb_volumebar_new ();
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_events (GTK_WIDGET (w->base.widget), gtk_widget_get_events (GTK_WIDGET (w->base.widget)) | GDK_SCROLL_MASK);
#endif
    ddb_volumebar_init_signals (DDB_VOLUMEBAR (w->volumebar), w->base.widget);
    gtk_widget_show (w->volumebar);
    gtk_widget_set_size_request (w->base.widget, 70, -1);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->volumebar);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

// chiptune voice ctl
static void
on_voice_toggled (GtkToggleButton *togglebutton, gpointer user_data) {
    w_ctvoices_t *w = user_data;
    int voices = 0;
    for (int i = 0; i < 8; i++) {
        int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w->voices[i]));
        voices |= active << i;
    }
    deadbeef->conf_set_int ("chip.voices", voices);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

ddb_gtkui_widget_t *
w_ctvoices_create (void) {
    w_ctvoices_t *w = malloc (sizeof (w_ctvoices_t));
    memset (w, 0, sizeof (w_ctvoices_t));
    w->base.widget = gtk_event_box_new ();
    GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (w->base.widget), hbox);

    GtkWidget *label = gtk_label_new_with_mnemonic (_("Voices:"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    int voices = deadbeef->conf_get_int ("chip.voices", 0xff);
    for (int i = 0; i < 8; i++) {
        w->voices[i] = gtk_check_button_new ();
        gtk_widget_show (w->voices[i]);
        gtk_box_pack_start (GTK_BOX (hbox), w->voices[i], FALSE, FALSE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w->voices[i]), voices & (1<<i));
        g_signal_connect ((gpointer) w->voices[i], "toggled", G_CALLBACK (on_voice_toggled), w);
    }

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}


////// Log viewer widget

static void
logviewer_logger_callback (struct DB_plugin_s *plugin, uint32_t layers, const char *text, void *ctx);


static void
w_logviewer_init (struct ddb_gtkui_widget_s *widget) {
    GtkTextBuffer *buffer;
    w_logviewer_t *w = (w_logviewer_t *)widget;
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w->textview));
    gtk_text_buffer_set_text (buffer, "Log\n", -1);
}

static void
w_logviewer_scroll_changed (GtkAdjustment *adjustment, gpointer user_data)
{
    w_logviewer_t *w = (w_logviewer_t *)user_data;

    if (gtk_adjustment_get_value (adjustment) >=
        gtk_adjustment_get_upper (adjustment) -
        gtk_adjustment_get_page_size (adjustment) - 1e-12)
    {
        w->scroll_bottomed = 1;
    } else {
        w->scroll_bottomed= 0;
    }
}


static gboolean
logviewer_addtext_cb (gpointer data) {
    logviewer_addtexts_t *s = (logviewer_addtexts_t *)data;
    w_logviewer_t *w = s->w;

    GtkTextBuffer *buffer;
    GtkTextIter iter;
    size_t len;
    len = strlen(s->text_to_add);
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w->textview));
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert (buffer, &iter, s->text_to_add, (gint)len);
    // Make sure it ends on a newline
    if (s->text_to_add[len-1] != '\n') {
        gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, "\n", 1);
    }
    if (w->scroll_bottomed) {
        gtk_text_buffer_get_end_iter(buffer, &iter);
        GtkTextMark *mark = gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (w->textview), mark);
    }
    free (s->text_to_add);
    free(s);
    return FALSE;
}

static void
logviewer_logger_callback (struct DB_plugin_s *plugin, uint32_t layers, const char *text, void *ctx) {
    logviewer_addtexts_t *s = malloc (sizeof (logviewer_addtexts_t));
    s->w = (w_logviewer_t *)ctx;
    s->text_to_add = strdup(text);
    g_idle_add(logviewer_addtext_cb, (gpointer)s);
}

void
w_logviewer_destroy (ddb_gtkui_widget_t *w) {
    // This is only called if removing widget in design mode
    deadbeef->log_viewer_unregister (logviewer_logger_callback, w);
    if (w_logviewer_instancecount > 0) {
        w_logviewer_instancecount--;
    }
}

ddb_gtkui_widget_t *
w_logviewer_create (void) {
    w_logviewer_t *w = malloc (sizeof (w_logviewer_t));
    memset (w, 0, sizeof (w_logviewer_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_logviewer_init;
    w->base.destroy = w_logviewer_destroy;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->textview = gtk_text_view_new ();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(w->textview), FALSE);
    //gtk_widget_set_size_request(w->textview, 320, 240);
    gtk_widget_show (w->textview);
    gtk_container_add (GTK_CONTAINER (scroll), w->textview);

    w_override_signals (w->base.widget, w);

    GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment ( GTK_SCROLLED_WINDOW (scroll));
    w->scroll_bottomed=1;
    g_signal_connect (adjustment, "value-changed", G_CALLBACK (w_logviewer_scroll_changed), w);

    deadbeef->log_viewer_register (logviewer_logger_callback, w);

    w_logviewer_instancecount++;

    return (ddb_gtkui_widget_t *)w;
}

gboolean
w_logviewer_is_present(void) {
    return w_logviewer_instancecount > 0;
}
