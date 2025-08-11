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

#include <stdlib.h>
#include <string.h>
#include "gtkScriptableListEditViewController.h"
#include "gtkScriptableListEditWindowController.h"
#include "gtkScriptablePropertySheetEditViewController.h"
#include "gtkScriptablePropertySheetEditWindowController.h"
#include "../../../gettext.h"

struct gtkScriptableListEditViewController_t {
    scriptableItem_t *scriptable;
    GtkWidget *view;
    GtkTreeView *tree_view;
    GtkListStore *list_store;
    gboolean is_reloading;

    GtkCellRenderer *cell_renderer;

    GtkWidget *add_button;
    GtkWidget *remove_button;
    GtkWidget *config_button;
    GtkWidget *duplicate_button;

    gtkScriptableListEditViewControllerDelegate_t *delegate;
    void *context;

    gtkScriptableListEditWindowController_t *list_editor_window_controller;
    gtkScriptableListEditWindowControllerDelegate_t list_editor_window_delegate;

    gtkScriptablePropertySheetEditWindowController_t *property_sheet_editor_window_controller;
    gtkScriptablePropertySheetEditWindowControllerDelegate_t property_sheet_editor_window_delegate;

};

static void
_reload_data (gtkScriptableListEditViewController_t *self);

static int
_get_selected_index (gtkScriptableListEditViewController_t *self);

static void
_add_did_activate (GtkButton *button, gpointer user_data);

static void
_remove_did_activate (GtkButton *button, gpointer user_data);

static void
_config_did_activate (GtkButton *button, gpointer user_data);

static void
_duplicate_did_activate (GtkButton *button, gpointer user_data);

static void
_list_scriptable_did_change (
    gtkScriptableListEditWindowController_t *view_controller,
    gtkScriptableChange_t change_type,
    void *context);

static void
_list_selection_did_change (GtkTreeSelection *treeselection, gpointer user_data);

static void
_list_editor_window_did_close (gtkScriptableListEditWindowController_t *controller, void *context);

static void
_property_sheet_scriptable_did_change (
                             gtkScriptablePropertySheetEditWindowController_t *view_controller,
                             gtkScriptableChange_t change_type,
                             void *context);
static void
_property_sheet_editor_window_did_close (gtkScriptablePropertySheetEditWindowController_t *controller, void *context);

static void
_init_treeview_cell_from_scriptable_item (
    gtkScriptableListEditViewController_t *self,
    GtkTreeIter *iter,
    scriptableItem_t *item);

static GtkWidget *
_create_tool_button_with_image_name (const char *image_name);

static void
_did_edit_name (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data);

static void
_did_reorder_items (GtkWidget *widget, GdkDragContext *context, gpointer user_data);

static void
_display_alert (const char *title, const char *message, const char *secondary_message, GtkWindow *modalForWindow);

gtkScriptableListEditViewController_t *
gtkScriptableListEditViewControllerNew (void) {
    gtkScriptableListEditViewController_t *self = calloc (1, sizeof (gtkScriptableListEditViewController_t));

    self->list_editor_window_delegate.scriptable_did_change = _list_scriptable_did_change;
    self->list_editor_window_delegate.window_did_close = _list_editor_window_did_close;

    self->property_sheet_editor_window_delegate.scriptable_did_change = _property_sheet_scriptable_did_change;
    self->property_sheet_editor_window_delegate.window_did_close = _property_sheet_editor_window_did_close;

    return self;
}

void
gtkScriptableListEditViewControllerFree (gtkScriptableListEditViewController_t *self) {
    g_object_unref (self->view);
    free (self);
}

void
gtkScriptableListEditViewControllerSetDelegate (
    gtkScriptableListEditViewController_t *self,
    gtkScriptableListEditViewControllerDelegate_t *delegate,
    void *context) {
    self->delegate = delegate;
    self->context = context;
}

void
gtkScriptableListEditViewControllerLoad (gtkScriptableListEditViewController_t *self) {
    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    g_object_ref (vbox);
    self->view = vbox;

    GtkWidget *scroll_view = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll_view);
    gtk_box_pack_start (GTK_BOX (vbox), scroll_view, TRUE, TRUE, 0);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_view), GTK_SHADOW_IN);
    gtk_widget_set_size_request (scroll_view, 300, 100);

    GtkWidget *list_view = gtk_tree_view_new ();
    gtk_widget_show (list_view);
    gtk_container_add (GTK_CONTAINER (scroll_view), list_view);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);
    self->tree_view = GTK_TREE_VIEW (list_view);

    GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    self->list_store = store;
    gtk_tree_view_set_model (GTK_TREE_VIEW (list_view), GTK_TREE_MODEL (store));
    g_signal_connect ((gpointer)list_view, "drag_end", G_CALLBACK (_did_reorder_items), self);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    self->cell_renderer = renderer;

    g_signal_connect ((gpointer)renderer, "edited", G_CALLBACK (_did_edit_name), self);

    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), column, 0);

    GtkWidget *button_box = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (button_box);
    gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *toolbar = gtk_hbox_new(TRUE, 0);
    gtk_widget_show (toolbar);
    gtk_box_pack_start (GTK_BOX (button_box), toolbar, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3, 0, 0)
    const char *add_icon = "list-add-symbolic";
    const char *remove_icon = "list-remove-symbolic";
    const char *preferences_icon = "document-edit-symbolic";
    const char *copy_icon = "edit-copy-symbolic";
#else
    const char *add_icon = "list-add";
    const char *remove_icon = "list-remove";
    const char *preferences_icon = "gtk-preferences";
    const char *copy_icon = "edit-copy";
#endif

    GtkWidget *add_button = _create_tool_button_with_image_name (add_icon);
    gtk_widget_show (add_button);
    gtk_container_add (GTK_CONTAINER (toolbar), add_button);
    self->add_button = add_button;

    GtkWidget *remove_button = _create_tool_button_with_image_name (remove_icon);
    gtk_widget_show (remove_button);
    gtk_container_add (GTK_CONTAINER (toolbar), remove_button);
    self->remove_button = remove_button;

    GtkWidget *config_button = _create_tool_button_with_image_name (preferences_icon);
    gtk_widget_show (config_button);
    gtk_container_add (GTK_CONTAINER (toolbar), config_button);
    self->config_button = config_button;

    GtkWidget *duplicate_button = _create_tool_button_with_image_name (copy_icon);
    gtk_widget_show (duplicate_button);
    gtk_container_add (GTK_CONTAINER (toolbar), duplicate_button);
    self->duplicate_button = duplicate_button;

    if (self->delegate != NULL && self->delegate->add_buttons != NULL) {
        self->delegate->add_buttons (self, GTK_BOX (button_box), self->context);
    }

    g_signal_connect ((gpointer)add_button, "clicked", G_CALLBACK (_add_did_activate), self);

    g_signal_connect ((gpointer)remove_button, "clicked", G_CALLBACK (_remove_did_activate), self);

    g_signal_connect ((gpointer)config_button, "clicked", G_CALLBACK (_config_did_activate), self);

    g_signal_connect ((gpointer)duplicate_button, "clicked", G_CALLBACK (_duplicate_did_activate), self);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
    g_signal_connect ((gpointer)selection, "changed", G_CALLBACK (_list_selection_did_change), self);

    _reload_data (self);
}

GtkWidget *
gtkScriptableListEditViewControllerGetView (gtkScriptableListEditViewController_t *self) {
    return self->view;
}

void
gtkScriptableListEditViewControllerSetScriptable (
    gtkScriptableListEditViewController_t *self,
    scriptableItem_t *scriptable) {
    self->scriptable = scriptable;

    gboolean editable = 0 != (scriptableItemFlags (scriptable) & SCRIPTABLE_FLAG_CAN_RENAME);

    GValue val = { 0 };
    g_value_init (&val, G_TYPE_BOOLEAN);
    g_value_set_boolean (&val, editable);
    g_object_set_property (G_OBJECT (self->cell_renderer), "editable", &val);
    g_value_unset (&val);

    gboolean reorderable = 0 != (scriptableItemFlags (scriptable) & SCRIPTABLE_FLAG_IS_REORDABLE);

    gtk_tree_view_set_reorderable (self->tree_view, reorderable);

    _reload_data (self);
}

scriptableItem_t *
gtkScriptableListEditViewControllerGetScriptable (gtkScriptableListEditViewController_t *self) {
    return self->scriptable;
}

static void
_update_buttons (gtkScriptableListEditViewController_t *self) {
    int selected_index = _get_selected_index (self);
    gboolean enable = selected_index != -1;
    gboolean editable = FALSE;

    if (selected_index != -1) {
        scriptableItem_t *item = scriptableItemChildAtIndex (self->scriptable, selected_index);
        uint64_t flag = scriptableItemFlags (item) & SCRIPTABLE_FLAG_IS_LIST;
        const char *dlg = scriptableItemConfigDialog (item);

        editable = flag != 0 || dlg != NULL;
    }

    gtk_widget_set_sensitive (self->remove_button, enable);
    gtk_widget_set_sensitive (self->config_button, enable && editable);
    gtk_widget_set_sensitive (self->duplicate_button, enable);
}

static void
_reload_data (gtkScriptableListEditViewController_t *self) {
    gtk_list_store_clear (self->list_store);

    if (self->scriptable == NULL) {
        return;
    }

    self->is_reloading = TRUE;

    scriptableItem_t *item = scriptableItemChildren (self->scriptable);
    while (item != NULL) {
        GtkTreeIter iter;
        gtk_list_store_append (self->list_store, &iter);

        _init_treeview_cell_from_scriptable_item (self, &iter, item);

        item = scriptableItemNext (item);
    }

    self->is_reloading = FALSE;

    _update_buttons (self);
}

static int
_get_selected_index (gtkScriptableListEditViewController_t *self) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection (self->tree_view);
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (self->tree_view));
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
        return -1;
    }
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
    gint *indices = gtk_tree_path_get_indices (path);
    gint index_count = gtk_tree_path_get_depth (path);
    if (index_count != 1) {
        return -1;
    }

    return indices[0];
}

static int
_insertion_index (gtkScriptableListEditViewController_t *self) {
    int cnt = scriptableItemNumChildren (self->scriptable);
    int index = _get_selected_index (self);
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
_init_treeview_cell_from_scriptable_item (
    gtkScriptableListEditViewController_t *self,
    GtkTreeIter *iter,
    scriptableItem_t *item) {
    char *text = scriptableItemFormattedName (item);
    gtk_list_store_set (self->list_store, iter, 0, text, 1, item, -1);

    scriptableItem_t *ppp;
    gtk_tree_model_get (GTK_TREE_MODEL (self->list_store), iter, 1, &ppp, -1);

    free (text);
}

static void
_insert_node_at_selection (gtkScriptableListEditViewController_t *self, scriptableItem_t *node) {
    int index = _insertion_index (self);

    gboolean have_sibling = FALSE;
    GtkTreeIter sibling;
    GtkTreeIter iter;
    if (index != -1) {
        GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
        have_sibling = gtk_tree_model_get_iter (GTK_TREE_MODEL (self->list_store), &sibling, path);
        gtk_tree_path_free (path);
    }

    if (have_sibling) {
        gtk_list_store_insert_before (self->list_store, &iter, &sibling);
    }
    else {
        gtk_list_store_append (self->list_store, &iter);
    }

    _init_treeview_cell_from_scriptable_item (self, &iter, node);

    scriptableItemInsertSubItemAtIndex (self->scriptable, node, (unsigned int)index);

    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change (self, ScriptableItemChangeUpdate, self->context);
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection (self->tree_view);
    gtk_tree_selection_select_iter (selection, &iter);
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->list_store), &iter);
    gtk_tree_view_scroll_to_cell (self->tree_view, path, NULL, FALSE, FALSE, FALSE);
    gtk_tree_path_free (path);

    _update_buttons (self);
}

static void
_create_node_with_type (gtkScriptableListEditViewController_t *self, const char *type) {
    scriptableItem_t *node = scriptableItemCreateItemOfType (self->scriptable, type);

    _insert_node_at_selection (self, node);
}

static void
_menu_create_item_activate (GtkMenuItem *menu_item, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;

    const char *type = g_object_get_data (G_OBJECT (menu_item), "item_type");
    if (type == NULL) {
        return;
    }

    _create_node_with_type (self, type);
}

static GtkWidget *
_get_create_item_menu (gtkScriptableListEditViewController_t *self) {
    scriptableStringListItem_t *names = scriptableItemFactoryItemNames (self->scriptable);
    if (names == NULL) {
        return NULL;
    }

    scriptableStringListItem_t *types = scriptableItemFactoryItemTypes (self->scriptable);
    if (types == NULL) {
        scriptableStringListFree (names);
        return NULL;
    }

    GtkWidget *menu = gtk_menu_new ();

    int index = 0;
    scriptableStringListItem_t *n = names;
    scriptableStringListItem_t *t = types;
    while (n) {
        GtkWidget *item = gtk_menu_item_new_with_label (n->str);
        gtk_widget_show (item);
        g_object_set_data_full (G_OBJECT (item), "item_type", strdup (t->str), free);
        g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (_menu_create_item_activate), self);
        gtk_container_add (GTK_CONTAINER (menu), item);
        n = n->next;
        t = t->next;
        index++;
    }

    scriptableStringListFree (names);
    names = NULL;

    scriptableStringListFree (types);
    types = NULL;

    return menu;
}

static void
_add_did_activate (GtkButton *button, gpointer user_data) {
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

        _create_node_with_type (self, types->str);

        scriptableStringListFree (names);
        scriptableStringListFree (types);

        return;
    }

    GtkWidget *menu = _get_create_item_menu (self);
    if (menu == NULL) {
        return;
    }

    gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (button), NULL);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
}

static void
_select_item_at_index (gtkScriptableListEditViewController_t *self, int index) {
    GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
    if (path == NULL) {
        return;
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection (self->tree_view);

    GtkTreeIter iter;
    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->list_store), &iter, path)) {
        gtk_tree_selection_select_iter (selection, &iter);
    }

    gtk_tree_path_free (path);
}

static void
_remove_did_activate (GtkButton *button, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;

    int index = _get_selected_index (self);
    if (index < 0) {
        return;
    }

    GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
    if (path == NULL) {
        return;
    }

    GtkTreeIter iter;
    gboolean have_iter = gtk_tree_model_get_iter (GTK_TREE_MODEL (self->list_store), &iter, path);
    gtk_tree_path_free (path);

    if (!have_iter) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex (self->scriptable, index);
    if (!item) {
        return;
    }

    gtk_list_store_remove (self->list_store, &iter);
    scriptableItemRemoveSubItem (self->scriptable, item);

    // select the same row
    _select_item_at_index (self, index);

    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change (self, ScriptableItemChangeUpdate, self->context);
    }

    _update_buttons (self);
}

static void
_config_did_activate (GtkButton *button, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;

    int index = _get_selected_index (self);
    if (index < 0) {
        return;
    }

    scriptableItem_t *item = scriptableItemChildAtIndex (self->scriptable, (unsigned int)index);

    if (scriptableItemFlags (item) & SCRIPTABLE_FLAG_IS_LIST) {
        // recurse!
        self->list_editor_window_controller = gtkScriptableListEditWindowControllerNew ();
        gtkScriptableListEditWindowControllerSetScriptable (self->list_editor_window_controller, item);

        char *title = gtkScriptableEditDialogTitleForItem (item);
        gtkScriptableListEditWindowControllerSetTitle (self->list_editor_window_controller, title);
        free (title);

        gtkScriptableListEditWindowControllerSetDelegate (
            self->list_editor_window_controller,
            &self->list_editor_window_delegate,
            self);

        gtkScriptableListEditWindowControllerRunModal (
            self->list_editor_window_controller,
            GTK_WINDOW (gtk_widget_get_toplevel (self->view)));
    }
    else {
        self->property_sheet_editor_window_controller = gtkScriptablePropertySheetEditWindowControllerNew();
        gtkScriptablePropertySheetEditWindowControllerSetScriptable(self->property_sheet_editor_window_controller, item);

        char *title = gtkScriptableEditDialogTitleForItem (item);
        gtkScriptablePropertySheetEditWindowControllerSetTitle (self->property_sheet_editor_window_controller, title);
        free (title);

        gtkScriptablePropertySheetEditWindowControllerSetDelegate (
                                                          self->property_sheet_editor_window_controller,
                                                          &self->property_sheet_editor_window_delegate,
                                                          self);

        gtkScriptablePropertySheetEditWindowControllerRunModal (
                                                       self->property_sheet_editor_window_controller,
                                                       GTK_WINDOW (gtk_widget_get_toplevel (self->view)));
    }
}

static void
_duplicate_did_activate (GtkButton *button, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;
    int index = _get_selected_index (self);
    if (index == -1) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex (self->scriptable, (unsigned int)index);

    scriptableItem_t *duplicate = scriptableItemClone (item);
    char name[100];
    snprintf (name, sizeof (name), _ ("%s (Copy)"), scriptableItemPropertyValueForKey (item, "name"));
    scriptableItemSetUniqueNameUsingPrefixAndRoot (duplicate, name, self->scriptable);

    _insert_node_at_selection (self, duplicate);
}

static void
_list_selection_did_change (GtkTreeSelection *treeselection, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;
    _update_buttons (self);
}

static GtkWidget *
_create_tool_button_with_image_name (const char *image_name) {
    GtkWidget *img = gtk_image_new_from_icon_name(image_name, GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget *btn = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(btn), img);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "flat");
#else
    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
#endif
    return btn;
}

static void
_did_edit_name (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;

    GtkTreePath *treepath = gtk_tree_path_new_from_string (path);

    if (treepath == NULL) {
        return;
    }

    gint *indices = gtk_tree_path_get_indices (treepath);
    int index = indices[0];

    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter (GTK_TREE_MODEL (self->list_store), &iter, treepath);
    gtk_tree_path_free (treepath);
    treepath = NULL;

    if (!valid) {
        return;
    }

    scriptableItem_t *item = scriptableItemChildAtIndex (self->scriptable, index);
    if (item == NULL) {
        return;
    }

    const char *name = scriptableItemPropertyValueForKey (item, "name");
    if (!strcmp (name, new_text)) {
        return; // name unchanged
    }

    // NOTE: there's no guaranteed way to prevent GTK from exiting edit mode,
    // so when error occurs -- reset the text back to unedited variant,
    // and don't attempt to re-enter edit mode.

    if (!(scriptableItemFlags (scriptableItemParent (item)) & SCRIPTABLE_FLAG_ALLOW_NON_UNIQUE_KEYS) &&
        scriptableItemContainsSubItemWithName (scriptableItemParent (item), new_text)) {
        _display_alert (
            _ ("Can't do that"),
            _ ("Preset with this name already exists."),
            _ ("Try a different name."),
            GTK_WINDOW (gtk_widget_get_toplevel (self->view)));
    }
    else if (!scriptableItemIsSubItemNameAllowed (scriptableItemParent (item), new_text)) {
        _display_alert (
            _ ("Can't do that"),
            _ ("This name is not allowed."),
            _ ("Try a different name."),
            GTK_WINDOW (gtk_widget_get_toplevel (self->view)));
    }
    else {
        scriptableItemSetPropertyValueForKey (item, new_text, "name");
        _init_treeview_cell_from_scriptable_item (self, &iter, item);

        if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
            self->delegate->scriptable_did_change (self, ScriptableItemChangeUpdate, self->context);
        }
    }
}

static void
_list_scriptable_did_change (
    gtkScriptableListEditWindowController_t *view_controller,
    gtkScriptableChange_t change_type,
    void *context) {
    gtkScriptableListEditViewController_t *self = context;
    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change (self, change_type, self->context);
    }
}

static void
_list_editor_window_did_close (gtkScriptableListEditWindowController_t *controller, void *context) {
    gtkScriptableListEditViewController_t *self = context;

    if (self->list_editor_window_controller != NULL) {
        gtkScriptableListEditWindowControllerFree (self->list_editor_window_controller);
        self->list_editor_window_controller = NULL;
    }
}


static void
_property_sheet_scriptable_did_change (
                                       gtkScriptablePropertySheetEditWindowController_t *view_controller,
                                       gtkScriptableChange_t change_type,
                                       void *context) {
    gtkScriptableListEditViewController_t *self = context;
    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change (self, change_type, self->context);
    }
}

static void
_property_sheet_editor_window_did_close (gtkScriptablePropertySheetEditWindowController_t *controller, void *context) {
    gtkScriptableListEditViewController_t *self = context;

    if (self->property_sheet_editor_window_controller != NULL) {
        gtkScriptablePropertySheetEditWindowControllerFree (self->property_sheet_editor_window_controller);
        self->property_sheet_editor_window_controller = NULL;
    }
}

static void
_did_reorder_items (GtkWidget *widget, GdkDragContext *context, gpointer user_data) {
    gtkScriptableListEditViewController_t *self = user_data;
    if (self->is_reloading) {
        return;
    }

    int position = 0;

    int count = scriptableItemNumChildren (self->scriptable);

    GtkTreeIter iter;
    gboolean res = gtk_tree_model_iter_children (GTK_TREE_MODEL (self->list_store), &iter, NULL);

    gboolean reload_needed = FALSE;

    // Resinsert all items into the model in the new order
    while (res) {
        char *str;
        scriptableItem_t *item = NULL;
        gtk_tree_model_get (GTK_TREE_MODEL (self->list_store), &iter, 0, &str, 1, &item, -1);

        if (item == NULL) {
            return;
        }

        // NOTE: GTK has a bug in drag-drop impl, especially on Mac,
        // which may result in copying items instead of moving.
        // This would run our model out of sync, and cause a crash.
        // So ensure no duplicates are made while updating the model,
        // and then reload the UI.
        if (position >= count) {
            position = count - 1;
            reload_needed = TRUE;
        }
        scriptableItemRemoveSubItem (self->scriptable, item);
        scriptableItemInsertSubItemAtIndex (self->scriptable, item, position);

        position++;
        res = gtk_tree_model_iter_next (GTK_TREE_MODEL (self->list_store), &iter);
    }

    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change (self, ScriptableItemChangeUpdate, self->context);
    }

    if (reload_needed) {
        _reload_data (self);
    }
}

static void
_display_alert (const char *title, const char *message, const char *secondary_message, GtkWindow *modalForWindow) {
    GtkWidget *dlg = gtk_message_dialog_new (
        GTK_WINDOW (modalForWindow),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK,
        "%s",
        message);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), modalForWindow);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), "%s", secondary_message);
    gtk_window_set_title (GTK_WINDOW (dlg), title);
    (void)gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}
