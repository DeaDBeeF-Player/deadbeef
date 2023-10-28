/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2023 Oleksiy Yakovenko and other contributors

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

#include "gtkScriptableListEditViewController.h"
#include "gtkScriptableListEditWindowController.h"
#include "../../../gettext.h"

struct gtkScriptableListEditViewController_t {
    scriptableItem_t *scriptable;
    GtkWidget *view;
    GtkTreeView *tree_view;
    GtkListStore *list_store;
    gtkScriptableListEditViewControllerDelegate_t *delegate;
    void *context;

    gtkScriptableListEditWindowController_t *list_editor_window_controller;
};

static void
_reload_data(gtkScriptableListEditViewController_t *self);

static void
_add_did_activate (GtkButton* button, gpointer user_data);

static void
_remove_did_activate (GtkButton* button, gpointer user_data);

static void
_config_did_activate (GtkButton* button, gpointer user_data);

static void
_duplicate_did_activate (GtkButton* button, gpointer user_data);

static GtkWidget *
_create_tool_button_with_image_name (GtkIconSize icon_size, const char *image_name) {
    GtkToolItem *button = gtk_tool_button_new (NULL, "");
#if GTK_CHECK_VERSION(3,0,0)
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON (button), image_name);
#else
    GtkWidget *image = gtk_image_new_from_stock (image_name, icon_size);
    gtk_widget_show (image);
    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON(button), image);
#endif
    return GTK_WIDGET(button);
}

gtkScriptableListEditViewController_t *
gtkScriptableListEditViewControllerNew (void) {
    gtkScriptableListEditViewController_t *self = calloc (1, sizeof  (gtkScriptableListEditViewController_t));

    return self;
}

void
gtkScriptableListEditViewControllerFree (gtkScriptableListEditViewController_t *self) {
    g_object_unref (self->view);
    free (self);
}

void
gtkScriptableListEditViewControllerSetDelegate(gtkScriptableListEditViewController_t *self, gtkScriptableListEditViewControllerDelegate_t *delegate) {
    self->delegate = delegate;
}

void
gtkScriptableListEditViewControllerLoad (gtkScriptableListEditViewController_t *self) {
    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    g_object_ref(vbox);
    self->view = vbox;

    GtkWidget *scroll_view = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll_view);
    gtk_box_pack_start (GTK_BOX (vbox), scroll_view, TRUE, TRUE, 0);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_view), GTK_SHADOW_IN);
    gtk_widget_set_size_request(scroll_view, 300, 100);

    GtkWidget *list_view = gtk_tree_view_new ();
    gtk_widget_show (list_view);
    gtk_container_add(GTK_CONTAINER(scroll_view), list_view);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list_view), FALSE);
    self->tree_view = GTK_TREE_VIEW(list_view);

    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    self->list_store = store;
    gtk_tree_view_set_model(GTK_TREE_VIEW(list_view), GTK_TREE_MODEL(store));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), column, 0);

    GtkWidget *button_box = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (button_box);
    gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *toolbar = gtk_toolbar_new ();
    gtk_widget_show (toolbar);
    gtk_box_pack_start (GTK_BOX (button_box), toolbar, FALSE, FALSE, 0);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), FALSE);

    gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

#if GTK_CHECK_VERSION(3,0,0)
    const char *add_icon = "list-add-symbolic";
    const char *remove_icon = "list-remove-symbolic";
    const char *preferences_icon = "preferences-system-symbolic";
    const char *copy_icon = "edit-copy-symbolic";
#else
    const char *add_icon = "gtk-add";
    const char *remove_icon = "gtk-remove";
    const char *preferences_icon = "gtk-preferences";
    const char *copy_icon = "gtk-copy";
#endif

    GtkIconSize icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar));

    GtkWidget *add_button = _create_tool_button_with_image_name(icon_size, add_icon);
    gtk_widget_show (add_button);
    gtk_container_add (GTK_CONTAINER (toolbar), add_button);

    GtkWidget *remove_button = _create_tool_button_with_image_name(icon_size, remove_icon);
    gtk_widget_show (remove_button);
    gtk_container_add (GTK_CONTAINER (toolbar), remove_button);

    GtkWidget *config_button = _create_tool_button_with_image_name(icon_size, preferences_icon);
    gtk_widget_show (config_button);
    gtk_container_add (GTK_CONTAINER (toolbar), config_button);

    GtkWidget *duplicate_button = _create_tool_button_with_image_name(icon_size, copy_icon);
    gtk_widget_show (duplicate_button);
    gtk_container_add (GTK_CONTAINER (toolbar), duplicate_button);

    if (self->delegate != NULL && self->delegate->add_buttons != NULL) {
        self->delegate->add_buttons(self, GTK_BOX(button_box));
    }

    g_signal_connect ((gpointer)add_button, "clicked", G_CALLBACK (_add_did_activate), self);

    g_signal_connect ((gpointer)remove_button, "clicked", G_CALLBACK (_remove_did_activate), self);

    g_signal_connect ((gpointer)config_button, "clicked", G_CALLBACK (_config_did_activate), self);

    g_signal_connect ((gpointer)duplicate_button, "clicked", G_CALLBACK (_duplicate_did_activate), self);


    _reload_data(self);
}

GtkWidget *
gtkScriptableListEditViewControllerGetView(gtkScriptableListEditViewController_t *self) {
    return self->view;
}

void
gtkScriptableListEditViewControllerSetScriptable(gtkScriptableListEditViewController_t *self, scriptableItem_t *scriptable) {
    self->scriptable = scriptable;
    _reload_data(self);
}

static void
_update_buttons(gtkScriptableListEditViewController_t *self) {
    // FIXME: impl
}

static void
_reload_data(gtkScriptableListEditViewController_t *self) {
    gtk_list_store_clear(self->list_store);

    if (self->scriptable == NULL) {
        return;
    }

    scriptableItem_t *item = scriptableItemChildren(self->scriptable);
    while (item != NULL) {
        GtkTreeIter iter;
        gtk_list_store_append (self->list_store, &iter);

        char *text = scriptableItemFormattedName(item);

        gtk_list_store_set (self->list_store, &iter, 0, text, -1);

        free (text);

        item = scriptableItemNext(item);
    }

    _update_buttons(self);
}

static int
_get_selected_index(gtkScriptableListEditViewController_t *self) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection (self->tree_view);
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model(self->tree_view));
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
        return -1;
    }
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
    gint *indices = gtk_tree_path_get_indices (path);
    gint index_count = gtk_tree_path_get_depth(path);
    if (index_count != 1) {
        return -1;
    }

    return indices[0];
}

static int
_insertion_index(gtkScriptableListEditViewController_t *self) {
    int cnt = scriptableItemNumChildren(self->scriptable);
    int index = _get_selected_index(self);
    if (cnt == 0) {
        return -1;
    }
    else if (index < 0) {
        index = cnt;
    }
    else {
        index++;
    }
    return index;
}

static void
_insert_node_at_selection (gtkScriptableListEditViewController_t *self, scriptableItem_t *node) {
    int index = _insertion_index(self);

    gboolean have_sibling = FALSE;
    GtkTreeIter sibling;
    GtkTreeIter iter;
    if (index != -1) {
        GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
        have_sibling = gtk_tree_model_get_iter(GTK_TREE_MODEL(self->list_store), &sibling, path);
        gtk_tree_path_free(path);
    }

    if (have_sibling) {
        gtk_list_store_insert_before(self->list_store, &iter, &sibling);
    }
    else {
        gtk_list_store_append(self->list_store, &iter);
    }

    char *text = scriptableItemFormattedName(node);
    gtk_list_store_set (self->list_store, &iter, 0, text, -1);
    free (text);

    scriptableItemInsertSubItemAtIndex(self->scriptable, node, (unsigned int)index);

    // FIXME: notify delegate

    GtkTreeSelection *selection = gtk_tree_view_get_selection (self->tree_view);
    gtk_tree_selection_select_iter(selection, &iter);
    GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(self->list_store), &iter);
    gtk_tree_view_scroll_to_cell(self->tree_view, path, NULL, FALSE, FALSE, FALSE);
    gtk_tree_path_free(path);

    _update_buttons(self);
}

static void
_create_node_with_type (gtkScriptableListEditViewController_t *self, const char *type) {
    scriptableItem_t *node = scriptableItemCreateItemOfType(self->scriptable, type);

    _insert_node_at_selection(self, node);
 }

static void
_menu_create_item_activate(GtkMenuItem* menu_item, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;

    const char *type = g_object_get_data(G_OBJECT(menu_item), "item_type");
    if (type == NULL) {
        return;
    }

    _create_node_with_type(self, type);
}

static GtkWidget *
_get_create_item_menu(gtkScriptableListEditViewController_t *self) {
    scriptableStringListItem_t *names = scriptableItemFactoryItemNames (self->scriptable);
    if (!names) {
        return NULL;
    }

    GtkWidget *menu = gtk_menu_new();

    int index = 0;
    scriptableStringListItem_t *n = names;
    while (n) {
        GtkWidget *item = gtk_menu_item_new_with_label(n->str);
        gtk_widget_show (item);
        g_object_set_data_full (G_OBJECT(item), "item_type", strdup(n->str), free);
        g_signal_connect(G_OBJECT (item), "activate",G_CALLBACK(_menu_create_item_activate), self);
        gtk_container_add(GTK_CONTAINER(menu), item);
        n = n->next;
        index++;
    }

    scriptableStringListFree(names);

    return menu;
}

static void
_add_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;
    scriptableStringListItem_t *names = scriptableItemFactoryItemNames (self->scriptable);
    if (!names) {
        return;
    }

    if (!names->next) {
        // single action
        scriptableStringListItem_t *types = scriptableItemFactoryItemTypes (self->scriptable);
        if (!types) {
            return;
        }

        _create_node_with_type(self, types->str);

        scriptableStringListFree (names);
        scriptableStringListFree (types);

        return;
    }

    GtkWidget *menu = _get_create_item_menu(self);
    if (menu == NULL) {
        return;
    }

    gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (button), NULL);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());

}

static void
_select_item_at_index(gtkScriptableListEditViewController_t *self, int index) {
    GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
    if (path == NULL) {
        return;
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection (self->tree_view);

    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(self->list_store), &iter, path)) {
        gtk_tree_selection_select_iter(selection, &iter);
    }

    gtk_tree_path_free(path);
}

static void
_remove_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;

    int index = _get_selected_index (self);
    if (index < 0) {
        return;
    }

    GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
    if (path == NULL) {
        return;
    }

    GtkTreeIter iter;
    gboolean have_iter = gtk_tree_model_get_iter(GTK_TREE_MODEL(self->list_store), &iter, path);
    gtk_tree_path_free(path);

    if (!have_iter) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(self->scriptable, index);
    if (!item) {
        return;
    }

    gtk_list_store_remove(self->list_store, &iter);
    scriptableItemRemoveSubItem(self->scriptable, item);

    // select the same row
    _select_item_at_index(self, index);

    // FIXME: notify delegate: scriptableItemDidChange -> ScriptableItemChangeUpdate

    _update_buttons(self);
}

// FIXME: the implementation of this function should be shared across Select and Edit
static void
_config_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;

    int index = _get_selected_index(self);
    if (index < 0) {
        return;
    }

    scriptableItem_t *item = scriptableItemChildAtIndex(self->scriptable, (unsigned int)index);

    if (scriptableItemFlags(item) & SCRIPTABLE_FLAG_IS_LIST) {
        // recurse!
        self->list_editor_window_controller = gtkScriptableListEditWindowControllerNew();
        gtkScriptableListEditWindowControllerSetScriptable(self->list_editor_window_controller, item);
        // FIXME: delegate (lifecycle management) and title
        //        self.nodeEditorWindowController.delegate = self.delegate;
        //        self.nodeEditorWindowController.window.title = @(scriptableItemPropertyValueForKey(item, "name")); // preset name
        gtkScriptableListEditWindowControllerRunModal(self->list_editor_window_controller, GTK_WINDOW(gtk_widget_get_toplevel(self->view)));
    }
    else {
        // FIXME: impl
//        self.propertiesViewController.labelFontSize = 10;
//        self.propertiesViewController.contentFontSize = 11;
//        self.propertiesViewController.unitSpacing = 4;
//        self.propertiesViewController.autoAlignLabels = NO;
//
//        self.propertiesDataSource.delegate = self;
//        self.propertiesDataSource = [[ScriptablePropertySheetDataSource alloc] initWithScriptable:item];
//
//        self.propertiesViewController.dataSource = self.propertiesDataSource;
//        self.propertiesPanelResetButton.enabled = !(scriptableItemFlags(item) & SCRIPTABLE_FLAG_IS_READONLY);
//        [self.view.window beginSheet:_propertiesPanel completionHandler:^(NSModalResponse returnCode) {
//         }];
    }
}

static void
_duplicate_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;
    int index = _get_selected_index(self);
    if (index == -1) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(self->scriptable, (unsigned int)index);

    scriptableItem_t *duplicate = scriptableItemClone(item);
    char name[100];
    snprintf (name, sizeof (name), _("%s (Copy)"), scriptableItemPropertyValueForKey(item, "name"));
    scriptableItemSetUniqueNameUsingPrefixAndRoot(duplicate, name, self->scriptable);

    _insert_node_at_selection(self, duplicate);
}
