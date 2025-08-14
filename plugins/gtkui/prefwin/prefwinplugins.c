/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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
#include <assert.h>
#include <gtk/gtk.h>
#include <string.h>
#include "../support.h"
#include "../interface.h"
#include "../ctmapping.h"
#include "../gtkui.h"
#include "../pluginconf.h"
#include "prefwin.h"
#include "prefwinplugins.h"


enum {
    PLUGIN_LIST_COL_TITLE,
    PLUGIN_LIST_COL_IDX,
    PLUGIN_LIST_COL_BUILTIN,
    PLUGIN_LIST_COL_HASCONFIG
};

static GtkWidget *prefwin;
static GtkListStore *pluginliststore;
static GtkTreeModelFilter *pluginliststore_filtered;
static GtkMenu *pluginlistmenu;
static GtkWidget *copyright_window;

void
on_only_show_plugins_with_configuration1_activate
(GtkMenuItem     *menuitem,
 gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    int active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    if (active) {
        gtk_tree_view_set_model (tree, GTK_TREE_MODEL (pluginliststore_filtered));
    } else {
        gtk_tree_view_set_model (tree, GTK_TREE_MODEL (pluginliststore));
    }
}

gboolean
on_pref_pluginlist_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 3) {
        gtk_menu_popup (GTK_MENU (pluginlistmenu), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
        return TRUE;
    }

    return FALSE;
}

static void
plugin_pref_prop_changed_cb(ddb_pluginprefs_dialog_t *make_dialog_conf) {
    apply_conf (GTK_WIDGET (make_dialog_conf->content), &make_dialog_conf->dialog_conf, 0);
}

static void
gtkui_conf_get_str (const char *key, char *value, int len, const char *def) {
    deadbeef->conf_get_str (key, def, value, len);
}

void
on_pref_pluginlist_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    GtkTreeIter iter;
    GtkTreeModel *model;
    int idx;
    model = gtk_tree_view_get_model (treeview);
    if (!gtk_tree_model_get_iter (model, &iter, path)) {
        return;
    }

    gtk_tree_model_get (model, &iter, PLUGIN_LIST_COL_IDX, &idx, -1);

    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[idx];

    assert (p);
    GtkWidget *w = prefwin;
    assert (w);

    char s[20];
    snprintf (s, sizeof (s), "%d.%d", p->version_major, p->version_minor);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "plug_version")), s);

    if (p->descr) {
        GtkTextView *tv = GTK_TEXT_VIEW (lookup_widget (w, "plug_description"));

        GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

        gtk_text_buffer_set_text (buffer, p->descr, (gint)strlen(p->descr));
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (tv), buffer);
        g_object_unref (buffer);
    }

    GtkWidget *link = lookup_widget (w, "weblink");
    if (p->website) {
        gtk_link_button_set_uri (GTK_LINK_BUTTON(link), p->website);
        gtk_widget_set_sensitive (link, TRUE);
    }
    else {
        gtk_link_button_set_uri (GTK_LINK_BUTTON(link), "");
        gtk_widget_set_sensitive (link, FALSE);
    }

    GtkTextView *lictv = GTK_TEXT_VIEW (lookup_widget (w, "plug_license"));
    if (p->copyright) {
        GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

        gtk_text_buffer_set_text (buffer, p->copyright, (gint)strlen(p->copyright));
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (lictv), buffer);
        g_object_unref (buffer);
    }
    else {
        gtk_text_view_set_buffer(lictv, NULL);
    }

    GtkWidget *plugin_actions_btnbox = lookup_widget (w, "plugin_actions_btnbox");

    GtkWidget *container = (lookup_widget (w, "plug_conf_dlg_viewport"));
    GtkWidget *child = gtk_bin_get_child (GTK_BIN (container));
    if (child)
        gtk_widget_destroy(child);

    if (p->configdialog) {
        ddb_dialog_t conf = {
            .title = p->name,
            .layout = p->configdialog,
            .set_param = deadbeef->conf_set_str,
            .get_param = gtkui_conf_get_str,
        };
        ddb_pluginprefs_dialog_t make_dialog_conf = {
            .dialog_conf = conf,
            .parent = prefwin,
            .prop_changed = plugin_pref_prop_changed_cb,
        };
        GtkWidget *box = gtk_vbox_new(FALSE, 0);
        gtk_widget_show (box);
        if (user_data == (void *)1) {
            apply_conf (box, &conf, 1);
        }
        make_dialog_conf.containerbox = box;
        gtk_container_add (GTK_CONTAINER (container), box);
        gtkui_make_dialog (&make_dialog_conf);

        gtk_widget_show (plugin_actions_btnbox);
    } else {
        gtk_widget_hide (plugin_actions_btnbox);
    }
}

void
on_plugin_conf_reset_btn_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    on_pref_pluginlist_cursor_changed(treeview, (void *)1);
}

void
on_pref_pluginlist_row_activated       (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 0);
}

void
on_plugin_conf_tab_btn_clicked         (GtkRadioButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 0);
}


void
on_plugin_info_tab_btn_clicked         (GtkRadioButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 1);
}


void
on_plugin_license_tab_btn_clicked      (GtkRadioButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_current_page (notebook, 2);
}

//  This code is only here so you can programmatically switch page
//  of the notebook and have the radio buttons update accordingly.
void
on_plugin_notebook_switch_page         (GtkNotebook     *notebook,
                                        GtkWidget       *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkToggleButton *plugin_conf_tab_btn = GTK_TOGGLE_BUTTON (lookup_widget (w, "plugin_conf_tab_btn"));
    GtkToggleButton *plugin_info_tab_btn = GTK_TOGGLE_BUTTON (lookup_widget (w, "plugin_info_tab_btn"));
    GtkToggleButton *plugin_license_tab_btn = GTK_TOGGLE_BUTTON (lookup_widget (w, "plugin_license_tab_btn"));

    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string ("switch_page");
    g_signal_handlers_block_matched ((gpointer)notebook, mask, detail, 0, NULL, NULL, NULL);

    switch (page_num) {
    case 0:
        gtk_toggle_button_set_active (plugin_conf_tab_btn, 1);
        break;
    case 1:
        gtk_toggle_button_set_active (plugin_info_tab_btn, 1);
        break;
    case 2:
        gtk_toggle_button_set_active (plugin_license_tab_btn, 1);
    }
    g_signal_handlers_unblock_matched ((gpointer)notebook, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_init_plugins_tab (GtkWidget *_prefwin) {
    GtkWidget *w = prefwin = _prefwin;
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
    // Order is: title, index, builtin, hasconfig
    GtkListStore *store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN);
    pluginliststore = store;
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Title"), rend_text, "text", PLUGIN_LIST_COL_TITLE, "weight", PLUGIN_LIST_COL_BUILTIN, NULL);
    gtk_tree_view_append_column (tree, col);
    gtk_tree_view_set_headers_visible (tree, FALSE);
    g_object_set (G_OBJECT (rend_text), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    const char *plugindir = deadbeef->get_system_dir (DDB_SYS_DIR_PLUGIN);
    for (i = 0; plugins[i]; i++) {
        GtkTreeIter it;
        const char *pluginpath;
        gtk_list_store_append (store, &it);
        pluginpath = deadbeef->plug_get_path_for_plugin_ptr (plugins[i]);
        if (!pluginpath) {
            pluginpath = plugindir;
        }
        gtk_list_store_set (store, &it,
                            PLUGIN_LIST_COL_TITLE, plugins[i]->name,
                            PLUGIN_LIST_COL_IDX, i,
                            PLUGIN_LIST_COL_BUILTIN, strstr(pluginpath, plugindir) ? PANGO_WEIGHT_NORMAL : PANGO_WEIGHT_BOLD,
                            PLUGIN_LIST_COL_HASCONFIG, plugins[i]->configdialog ? 1 : 0,
                            -1);
    }
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store), PLUGIN_LIST_COL_TITLE, GTK_SORT_ASCENDING);

    // Create a filtered model, then we switch between the two models to show/hide configurable plugins
    pluginliststore_filtered = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new (GTK_TREE_MODEL(store), NULL));
    gtk_tree_model_filter_set_visible_column (pluginliststore_filtered, PLUGIN_LIST_COL_HASCONFIG);

    gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));

    pluginlistmenu = GTK_MENU(create_plugin_list_popup_menu ());
    gtk_menu_attach_to_widget (GTK_MENU (pluginlistmenu), GTK_WIDGET (tree), NULL);

    // We do this here so one can leave the tabs visible in the Glade designer for convenience.
    GtkNotebook *notebook = GTK_NOTEBOOK (lookup_widget (w, "plugin_notebook"));
    gtk_notebook_set_show_tabs(notebook, FALSE);
    gtk_notebook_set_current_page(notebook, 0);

    // Some styling changes since Glade doesn't support setting it
#if GTK_CHECK_VERSION(3,12,0)
    GtkButtonBox *bbox = GTK_BUTTON_BOX (lookup_widget (w, "plugin_tabbtn_hbtnbox"));
    gtk_button_box_set_layout (bbox, GTK_BUTTONBOX_EXPAND);
#endif
}

void
prefwin_free_plugins (void) {
    prefwin = NULL;
    copyright_window = NULL;
    pluginliststore = NULL;
    pluginliststore_filtered = NULL;
}

static void
show_copyright_window (const char *text, const char *title, GtkWidget **pwindow) {
    if (*pwindow) {
        return;
    }
    GtkWidget *widget = *pwindow = create_helpwindow ();
    g_object_set_data (G_OBJECT (widget), "pointer", pwindow);
    g_signal_connect (widget, "delete_event", G_CALLBACK (on_gtkui_info_window_delete), pwindow);
    gtk_window_set_title (GTK_WINDOW (widget), title);
    gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (prefwin));
    GtkWidget *txt = lookup_widget (widget, "helptext");
    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

    gtk_text_buffer_set_text (buffer, text, (gint)strlen(text));
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (txt), buffer);
    g_object_unref (buffer);
    gtk_widget_show (widget);
}

void
on_plug_copyright_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(lookup_widget (prefwin, "pref_pluginlist"));
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[*indices];
    g_free (indices);
    assert (p);

    if (p->copyright) {
        show_copyright_window (p->copyright, "Copyright", &copyright_window);
    }
}

