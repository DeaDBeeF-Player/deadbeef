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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include <deadbeef/deadbeef.h>
#include "gtkui.h"
#include "pluginconf.h"

static ddb_dsp_context_t *chain;
static GtkWidget *prefwin;
static GtkWidget *dsp_popup;

static ddb_dsp_context_t *
dsp_clone (ddb_dsp_context_t *from) {
    ddb_dsp_context_t *dsp = from->plugin->open ();
    char param[2000];
    if (from->plugin->num_params) {
        int n = from->plugin->num_params ();
        for (int i = 0; i < n; i++) {
            from->plugin->get_param (from, i, param, sizeof (param));
            dsp->plugin->set_param (dsp, i, param);
        }
    }
    dsp->enabled = from->enabled;
    return dsp;
}

static void
fill_dsp_chain (GtkListStore *mdl) {
    ddb_dsp_context_t *dsp = chain;
    while (dsp) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        gtk_list_store_set (mdl, &iter, 0, dsp->plugin->plugin.name, -1);
        dsp = dsp->next;
    }
}

static int dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

static int
scandir_preset_filter (const struct dirent *ent) {
    char *ext = strrchr (ent->d_name, '.');
    if (ext && !strcasecmp (ext, ".txt")) {
        return 1;
    }
    return 0;
}

static void
dsp_fill_preset_list (GtkWidget *combobox) {
    // fill list of presets
    GtkListStore *mdl;
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combobox)));
    gtk_list_store_clear (mdl);
    struct dirent **namelist = NULL;
    char path[1024];
    if (snprintf (path, sizeof (path), "%s/presets/dsp", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG)) > 0) {
        int n = scandir (path, &namelist, scandir_preset_filter, dirent_alphasort);
        int i;
        for (i = 0; i < n; i++) {
            char title[100];
            strcpy (title, namelist[i]->d_name);
            char *e = strrchr (title, '.');
            if (e) {
                *e = 0;
            }
            GtkTreeIter iter;
            gtk_list_store_append (mdl, &iter);
            gtk_list_store_set (mdl, &iter, 0, title, -1);
            free (namelist[i]);
        }
        free (namelist);
    }

    // set last preset name
    GtkWidget *entry = gtk_bin_get_child (GTK_BIN (combobox));
    if (entry) {
        deadbeef->conf_lock ();
        gtk_entry_set_text (GTK_ENTRY (entry), deadbeef->conf_get_str_fast ("gtkui.conf_dsp_preset", ""));
        deadbeef->conf_unlock ();
    }
}

static void
on_dsp_list_view_sel_changed(GtkTreeSelection *treeselection, gpointer user_data)
{
    GtkWidget *configbtn = lookup_widget (prefwin, "dsp_configure_toolbtn");
    GtkWidget *removebtn = lookup_widget (prefwin, "dsp_remove_toolbtn");
    GtkWidget *upbtn = lookup_widget (prefwin, "dsp_up_toolbtn");
    GtkWidget *downbtn = lookup_widget (prefwin, "dsp_down_toolbtn");

    GtkTreeModel *mdl;
    GtkTreeIter iter;

    gboolean has_selection = gtk_tree_selection_get_selected (treeselection, &mdl, &iter);
    if (has_selection) {
        int count = gtk_tree_model_iter_n_children (mdl, NULL);
        GtkTreePath *path = gtk_tree_model_get_path (mdl, &iter);
        int *ind = gtk_tree_path_get_indices (path);
        gtk_widget_set_sensitive (upbtn, *ind > 0);
        gtk_widget_set_sensitive (downbtn, *ind < count-1);
    } else {
        gtk_widget_set_sensitive (upbtn, FALSE);
        gtk_widget_set_sensitive (downbtn, FALSE);
    }

    gtk_widget_set_sensitive (configbtn, has_selection);
    gtk_widget_set_sensitive (removebtn, has_selection);
}

static void
update_streamer (void) {
    deadbeef->streamer_set_dsp_chain (chain);
}

static int
listview_get_index (GtkWidget *list) {
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (!path) {
        // nothing selected
        return -1;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);
    return idx;
}

static void
on_dsp_popup_menu_item_activate(GtkMenuItem* self, gpointer user_data)
{
    // create new instance of the selected plugin
    struct DB_dsp_s* dsp = user_data;
    ddb_dsp_context_t *inst=NULL;
    if (dsp && dsp->open) {
        inst = dsp->open();
    }
    if (inst) {
        GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
        int idx = listview_get_index (list);

        // append to DSP chain at index
        ddb_dsp_context_t *p = chain;
        ddb_dsp_context_t *prev = NULL;
        int i = idx;
        while (p && i--) {
            prev = p;
            p = p->next;
        }
        if (p) {
            if (prev) {
                inst->next = p->next;
                prev->next = p;
                p->next = inst;
            }
            else {
                inst->next = p->next;
                chain->next = inst;
            }
        } else {
            chain = inst;
        }

        // reinit list of instances
        GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
        gtk_list_store_clear (mdl);
        fill_dsp_chain (mdl);
        // Update cursor to newly inserted item
        GtkTreePath *path = gtk_tree_path_new_from_indices (idx+1, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
        gtk_tree_path_free (path);

        update_streamer ();
    }
    else {
        fprintf (stderr, "prefwin: failed to add DSP plugin to chain\n");
    }
}

GtkWidget *make_dsp_popup_menu(void)
{
    struct DB_dsp_s **dsp = deadbeef->plug_get_dsp_list ();
    int i;
    GtkWidget *menu = gtk_menu_new();
    for (i = 0; dsp[i]; i++) {
        GtkWidget *item = gtk_menu_item_new_with_label(dsp[i]->plugin.name);
        gtk_widget_show (item);
        g_signal_connect(G_OBJECT (item), "activate",
            G_CALLBACK (on_dsp_popup_menu_item_activate), (void*)dsp[i]);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }
    return menu;
}

static void
on_dsp_popup_hide(GtkWidget* attach_widget, GtkMenu* menu)
{
    GtkWidget *togglebtn = lookup_widget (prefwin, "dsp_add_toolbtn");
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (togglebtn), FALSE);
}

#if GTK_CHECK_VERSION(3,0,0)
static void
_dsp_use_symbolic_icons(void) {
    GtkWidget *addbtn = lookup_widget (prefwin, "dsp_add_toolbtn");
    GtkWidget *configbtn = lookup_widget (prefwin, "dsp_configure_toolbtn");
    GtkWidget *removebtn = lookup_widget (prefwin, "dsp_remove_toolbtn");
    GtkWidget *upbtn = lookup_widget (prefwin, "dsp_up_toolbtn");
    GtkWidget *downbtn = lookup_widget (prefwin, "dsp_down_toolbtn");

    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (addbtn), NULL);
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (addbtn), "list-add-symbolic");

    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (configbtn), NULL);
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (configbtn), "preferences-system-symbolic");

    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (removebtn), NULL);
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (removebtn), "list-remove-symbolic");

    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (upbtn), NULL);
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (upbtn), "go-up-symbolic");

    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (downbtn), NULL);
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (downbtn), "go-down-symbolic");
}
#endif

void
dsp_setup_init (GtkWidget *_prefwin) {
    prefwin = _prefwin;
    // copy current dsp chain
    ddb_dsp_context_t *streamer_chain = deadbeef->streamer_get_dsp_chain ();

    ddb_dsp_context_t *tail = NULL;
    while (streamer_chain) {
        ddb_dsp_context_t *new = dsp_clone (streamer_chain);
        if (tail) {
            tail->next = new;
            tail = new;
        }
        else {
            chain = tail = new;
        }
        streamer_chain = streamer_chain->next;
    }

    // fill dsp_listview
    GtkWidget *listview = lookup_widget (prefwin, "dsp_listview");
    GtkTreeSelection *listview_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
    g_signal_connect ((gpointer)listview_sel, "changed", G_CALLBACK (on_dsp_list_view_sel_changed), NULL);


    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Plugin"), title_cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (listview), GTK_TREE_VIEW_COLUMN (col));
    GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (listview), GTK_TREE_MODEL (mdl));

    fill_dsp_chain (mdl);
    GtkTreePath *path = gtk_tree_path_new_from_indices (0, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (listview), path, NULL, FALSE);
    gtk_tree_path_free (path);

    GtkWidget *combobox = lookup_widget (prefwin, "dsp_preset");
    dsp_fill_preset_list (combobox);

    // Make dsp popup menu
    dsp_popup = make_dsp_popup_menu ();
    g_signal_connect ((gpointer)dsp_popup, "hide", G_CALLBACK (on_dsp_popup_hide), NULL);
    gtk_menu_attach_to_widget (GTK_MENU (dsp_popup), prefwin, NULL);

    // Styling
    GtkWidget *dsp_toolbar = lookup_widget (prefwin, "dsp_toolbar");
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (dsp_toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

#if GTK_CHECK_VERSION(3,0,0)
    _dsp_use_symbolic_icons ();
#endif
}

void
dsp_setup_free (void) {
    while (chain) {
        ddb_dsp_context_t *next = chain->next;
        chain->plugin->close (chain);
        chain = next;
    }
    prefwin = NULL;
}

void
on_dsp_remove_toolbtn_clicked          (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }

    ddb_dsp_context_t *p = chain;
    ddb_dsp_context_t *prev = NULL;
    int i = idx;
    while (p && i--) {
        prev = p;
        p = p->next;
    }
    if (p) {
        if (prev) {
            prev->next = p->next;
        }
        else {
            chain = p->next;
        }
        p->plugin->close (p);
        GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
        gtk_list_store_clear (mdl);
        fill_dsp_chain (mdl);
        GtkTreePath *path = gtk_tree_path_new_from_indices (idx, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
        gtk_tree_path_free (path);
        update_streamer ();
    }
}

static ddb_dsp_context_t *current_dsp_context = NULL;

void
dsp_ctx_set_param (const char *key, const char *value) {
    current_dsp_context->plugin->set_param (current_dsp_context, atoi (key), value);
}

void
dsp_ctx_get_param (const char *key, char *value, int len, const char *def) {
    strncpy (value, def, len);
    current_dsp_context->plugin->get_param (current_dsp_context, atoi (key), value, len);
}

int
button_cb (int btn, void *ctx) {
    if (btn == ddb_button_apply) {
        update_streamer ();
    }
    return 1;
}

void
on_dsp_configure_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }
    ddb_dsp_context_t *p = chain;
    int i = idx;
    while (p && i--) {
        p = p->next;
    }
    if (!p || !p->plugin->configdialog) {
        return;
    }
    current_dsp_context = p;
    ddb_dialog_t conf = {
        .title = p->plugin->plugin.name,
        .layout = p->plugin->configdialog,
        .set_param = dsp_ctx_set_param,
        .get_param = dsp_ctx_get_param,
    };
    int response = gtkui_run_dialog (prefwin, &conf, 0, button_cb, NULL);
    if (response == ddb_button_ok) {
        update_streamer ();
    }
    current_dsp_context = NULL;
}

static int
swap_items (GtkWidget *list, int idx) {
    ddb_dsp_context_t *prev = NULL;
    ddb_dsp_context_t *p = chain;

    int n = idx;
    while (n > 0 && p) {
        prev = p;
        p = p->next;
        n--;
    }

    if (!p || !p->next) {
        return -1;
    }

    ddb_dsp_context_t *moved = p->next;

    if (!moved) {
        return -1;
    }

    ddb_dsp_context_t *last = moved ? moved->next : NULL;

    if (prev) {
        p->next = last;
        prev->next = moved;
        moved->next = p;
    }
    else {
        p->next = last;
        chain = moved;
        moved->next = p;
    }
    GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
    gtk_list_store_clear (mdl);
    fill_dsp_chain (mdl);
    return 0;
}


void
on_dsp_up_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx <= 0) {
        return;
    }

    if (-1 == swap_items (list, idx-1)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx-1, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
    gtk_tree_path_free (path);
    update_streamer ();
}


void
on_dsp_down_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }

    if (-1 == swap_items (list, idx)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx+1, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
    gtk_tree_path_free (path);
    update_streamer ();
}

void
on_dsp_preset_changed                  (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    GtkWidget *entry = gtk_bin_get_child (GTK_BIN (combobox));
    if (entry) {
        deadbeef->conf_set_str ("gtkui.conf_dsp_preset", gtk_entry_get_text (GTK_ENTRY (entry)));
    }
}


void
on_dsp_preset_save_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
    const char *confdir = deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG);
    char path[1024];
    if (snprintf (path, sizeof (path), "%s/presets", confdir) < 0) {
        return;
    }
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/dsp", confdir) < 0) {
        return;
    }
    GtkWidget *combobox = lookup_widget (prefwin, "dsp_preset");
    GtkWidget *entry = gtk_bin_get_child (GTK_BIN (combobox));
    if (!entry) {
        return;
    }

    const char *text = gtk_entry_get_text (GTK_ENTRY (entry));
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/dsp/%s.txt", confdir, text) < 0) {
        return;
    }
    deadbeef->dsp_preset_save (path, chain);

    dsp_fill_preset_list (combobox);
}


void
on_dsp_preset_load_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *combobox = lookup_widget (prefwin, "dsp_preset");
    GtkWidget *entry = gtk_bin_get_child (GTK_BIN (combobox));
    if (entry) {
        const char *text = gtk_entry_get_text (GTK_ENTRY (entry));
        char path[PATH_MAX];
        if (snprintf (path, sizeof (path), "%s/presets/dsp/%s.txt", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), text) > 0) {
            ddb_dsp_context_t *new_chain = NULL;
            int res = deadbeef->dsp_preset_load (path, &new_chain);
            if (!res) {
                deadbeef->dsp_preset_free (chain);
                chain = new_chain;
                GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
                GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
                gtk_list_store_clear (mdl);
                fill_dsp_chain (mdl);
                update_streamer ();
            }
        }
    }
}

void
on_dsp_toolbtn_up_clicked              (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx <= 0) {
        return;
    }

    if (-1 == swap_items (list, idx-1)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx-1, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
    gtk_tree_path_free (path);
    update_streamer ();
}


void
on_dsp_toolbtn_down_clicked            (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }

    if (-1 == swap_items (list, idx)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx+1, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
    gtk_tree_path_free (path);
    update_streamer ();
}

static void
show_dsp_configure_dlg(void)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }
    ddb_dsp_context_t *p = chain;
    int i = idx;
    while (p && i--) {
        p = p->next;
    }
    if (!p || !p->plugin->configdialog) {
        return;
    }
    current_dsp_context = p;
    ddb_dialog_t conf = {
        .title = p->plugin->plugin.name,
        .layout = p->plugin->configdialog,
        .set_param = dsp_ctx_set_param,
        .get_param = dsp_ctx_get_param,
    };
    int response = gtkui_run_dialog (prefwin, &conf, 0, button_cb, NULL);
    if (response == ddb_button_ok) {
        update_streamer ();
    }
    current_dsp_context = NULL;
}

void
on_dsp_configure_toolbtn_clicked       (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
    show_dsp_configure_dlg ();
}

void
on_dsp_listview_row_activated          (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
    show_dsp_configure_dlg ();
}

#if !GTK_CHECK_VERSION(3,22,0)
static void
set_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  GtkWidget *button = GTK_WIDGET(user_data);
  GtkAllocation a;
  gtk_widget_get_allocation(GTK_WIDGET (button), &a);

  GtkRequisition r;
  gtk_widget_size_request(GTK_WIDGET (menu), &r);

  gdk_window_get_origin(gtk_widget_get_window(button), x, y);
  *x += a.x;
  *y += a.y - (r.height);
  *push_in = TRUE;
}
#endif

void
on_dsp_add_toolbtn_toggled             (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
    if (gtk_toggle_tool_button_get_active(toggletoolbutton)) {
#if GTK_CHECK_VERSION(3,22,0)
        gtk_menu_popup_at_widget (GTK_MENU (dsp_popup), GTK_WIDGET (toggletoolbutton), GDK_GRAVITY_NORTH_WEST, GDK_GRAVITY_SOUTH_WEST, NULL);
#else

  gtk_menu_popup(GTK_MENU (dsp_popup),
                 NULL, NULL,
                 (GtkMenuPositionFunc) set_position, toggletoolbutton,
                 0, gtk_get_current_event_time());

#endif
    }
}
