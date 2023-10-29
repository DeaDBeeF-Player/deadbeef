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

#include <gtk/gtk.h>
#include <stdlib.h>
#include "support.h"
#include "gtkScriptableSelectViewController.h"
#include "gtkScriptableListEditWindowController.h"

struct gtkScriptableSelectViewController_t {
    scriptableItem_t *scriptable;
    GtkWidget *view;
    GtkWidget *comboBox;
    GtkWidget *editButton;

    gboolean is_reloading;

    gtkScriptableListEditWindowController_t *editListWindowController;
    gtkScriptableListEditWindowControllerDelegate_t list_edit_window_delegate;

    gtkScriptableSelectViewControllerDelegate_t *delegate;
    void *context;
};

static void
_selection_did_change (GtkComboBox* self, gpointer user_data);

static void
_edit_did_activate (GtkButton* self, gpointer user_data);

static void
_list_edit_window_did_close (gtkScriptableListEditWindowController_t *controller, void *context);

static void
_reload_data(gtkScriptableSelectViewController_t *self);

static void
_scriptable_did_change (gtkScriptableListEditWindowController_t *view_controller, gtkScriptableChange_t change_type, void *context);

static void
_reload_data(gtkScriptableSelectViewController_t *self);

gtkScriptableSelectViewController_t *
gtkScriptableSelectViewControllerNew(void) {
    gtkScriptableSelectViewController_t *self = calloc (1, sizeof (gtkScriptableSelectViewController_t));

    GtkWidget *hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox);
    GtkWidget *comboBox = gtk_combo_box_text_new();
    gtk_widget_show(comboBox);
    GtkWidget *button = gtk_button_new();
    gtk_widget_show(button);

#if GTK_CHECK_VERSION(3,0,0)
    const char *edit_image = "text-editor-symbolic";
#else
    const char *edit_image = "gtk-edit";
#endif
    GtkWidget *image = gtk_image_new_from_icon_name(edit_image, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);

    gtk_box_pack_start(GTK_BOX(hbox), comboBox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

    self->view = hbox;
    self->comboBox = comboBox;
    self->editButton = button;

    g_signal_connect ((gpointer)comboBox, "changed", G_CALLBACK (_selection_did_change), self);

    g_signal_connect ((gpointer)button, "clicked", G_CALLBACK (_edit_did_activate), self);

    g_object_ref(hbox);

    self->list_edit_window_delegate.window_did_close = _list_edit_window_did_close;
    self->list_edit_window_delegate.scriptable_did_change = _scriptable_did_change;

    return self;
}

void
gtkScriptableSelectViewControllerFree(gtkScriptableSelectViewController_t *self) {
    g_object_unref(self->view);
    free (self);
}

void
gtkScriptableSelectViewControllerSetScriptable(gtkScriptableSelectViewController_t *self, scriptableItem_t *scriptable) {
    self->scriptable = scriptable;
    gtkScriptableSelectViewControllerLoad(self);
}

void
gtkScriptableSelectViewControllerSetDelegate(gtkScriptableSelectViewController_t *self, gtkScriptableSelectViewControllerDelegate_t *delegate, void *context) {
    self->delegate = delegate;
    self->context = context;
}

void
gtkScriptableSelectViewControllerLoad(gtkScriptableSelectViewController_t *self) {
    _reload_data(self);
}

GtkWidget *
gtkScriptableSelectViewControllerGetView(gtkScriptableSelectViewController_t *self) {
    return self->view;
}

void
gtkScriptableSelectViewControllerSelectItem(gtkScriptableSelectViewController_t *self, scriptableItem_t *item) {
    int index = scriptableItemIndexOfChild(self->scriptable, item);
    if (index != -1) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(self->comboBox), index);
    }
}

static void
_selection_did_change (GtkComboBox* comboBox, gpointer user_data) {
    gtkScriptableSelectViewController_t *self = user_data;
    if (self->delegate == NULL || self->is_reloading) {
        return;
    }

    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(self->comboBox));
    if (active < 0) {
        return;
    }
    scriptableItem_t *item = scriptableItemChildAtIndex(self->scriptable, active);

    self->delegate->selection_did_change(self, item, self->context);
}

static void
_list_edit_window_did_close (gtkScriptableListEditWindowController_t *controller, void *context) {
    gtkScriptableSelectViewController_t *self = context;

    if (self->editListWindowController != NULL) {
        gtkScriptableListEditWindowControllerFree(self->editListWindowController);
        self->editListWindowController = NULL;
    }
}

static void
_edit_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableSelectViewController_t *self = user_data;

    if (self->editListWindowController != NULL) {
        gtkScriptableListEditWindowControllerFree(self->editListWindowController);
        self->editListWindowController = NULL;
    }

    if (!(scriptableItemFlags(self->scriptable) & SCRIPTABLE_FLAG_IS_LIST)) {
        // This item can't be anything else than a list
        return;
    }

    self->editListWindowController = gtkScriptableListEditWindowControllerNew();
    gtkScriptableListEditWindowControllerSetScriptable(self->editListWindowController, self->scriptable);

    char *title = gtkScriptableEditDialogTitleForItem (self->scriptable);
    gtkScriptableListEditWindowControllerSetTitle(self->editListWindowController, title);
    free (title);
    gtkScriptableListEditWindowControllerSetDelegate(self->editListWindowController, &self->list_edit_window_delegate, self);

    gtkScriptableListEditWindowControllerRunModal(self->editListWindowController, GTK_WINDOW(gtk_widget_get_toplevel(self->view)));
}

static void
_scriptable_did_change (gtkScriptableListEditWindowController_t *view_controller, gtkScriptableChange_t change_type, void *context) {
    gtkScriptableSelectViewController_t *self = context;

    _reload_data(self);

    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change(self, change_type, context);
    }
}

static void
_reload_data(gtkScriptableSelectViewController_t *self) {
    self->is_reloading = TRUE;

    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(self->comboBox));

    char *selected_text = NULL;

    GtkTreeIter iter;
    gboolean res = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(self->comboBox), &iter);
    if (res) {
        gtk_tree_model_get(model, &iter, 0, &selected_text, -1);
        if (selected_text != NULL) {
            selected_text = strdup (selected_text);
        }
    }

    gtk_list_store_clear(GTK_LIST_STORE(model));

    if (self->scriptable == NULL) {
        self->is_reloading = FALSE;
        free (selected_text);
        return;
    }

    int active = -1;

    int index = 0;
    for (scriptableItem_t *item = scriptableItemChildren(self->scriptable)
         ; item != NULL
         ; item = scriptableItemNext(item), index++) {
        char *name = scriptableItemFormattedName(item);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->comboBox), name);
        free (name);

        if (selected_text != NULL && !strcmp(selected_text, name)) {
            active = index;
        }
    }
    self->is_reloading = FALSE;

    // Selected item was deleted, select the first one
    if (active == -1 && index != 0) {
        active = 0;
    }

    if (active != -1) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(self->comboBox), active);
    }
}

