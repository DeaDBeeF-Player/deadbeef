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
#include "../../../gettext.h"

struct gtkScriptableListEditViewController_t {
    scriptableItem_t *scriptable;
    GtkWidget *view;
    GtkListStore *list_store;
    gtkScriptableListEditViewControllerDelegate_t *delegate;
    void *context;
};

static void
_reload_data(gtkScriptableListEditViewController_t *self);

static GtkWidget *
_create_tool_button_with_image_name (GtkIconSize icon_size, const char *image_name) {
    GtkToolItem *button = gtk_toggle_tool_button_new ();
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), "");
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
}

