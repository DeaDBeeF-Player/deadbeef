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

struct gtkScriptableSelectViewController_t {
    scriptableItem_t *scriptable;
    GtkWidget *view;
    GtkWidget *comboBox;
    GtkWidget *editButton;

    gtkScriptableSelectViewControllerDelegate_t *delegate;
    void *context;
};

static void
_selection_did_change (GtkComboBox* self, gpointer user_data);

static void
_edit_did_activate (GtkButton* self, gpointer user_data);


gtkScriptableSelectViewController_t *
gtkScriptableSelectViewControllerNew(void) {
    gtkScriptableSelectViewController_t *self = calloc (1, sizeof (gtkScriptableSelectViewController_t));


    GtkWidget *hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox);
    GtkWidget *comboBox = gtk_combo_box_text_new();
    gtk_widget_show(comboBox);
    GtkWidget *button = gtk_button_new();
    gtk_widget_show(button);
    GtkWidget *image = gtk_image_new_from_icon_name(GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);

    gtk_box_pack_start(GTK_BOX(hbox), comboBox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

    self->view = hbox;
    self->comboBox = comboBox;
    self->editButton = button;

    g_signal_connect ((gpointer)comboBox, "changed", G_CALLBACK (_selection_did_change), self);

    g_signal_connect ((gpointer)button, "clicked", G_CALLBACK (_edit_did_activate), self);

    g_object_ref(hbox);

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
    // FIXME: delete all items
//    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(self->comboBox));

    if (self->scriptable == NULL) {
        return;
    }

    for (scriptableItem_t *item = scriptableItemChildren(self->scriptable)
         ; item != NULL
         ; item = scriptableItemNext(item)) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->comboBox), scriptableItemPropertyValueForKey(item, "name"));
    }
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
    if (self->delegate == NULL) {
        return;
    }

    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(self->comboBox));
    scriptableItem_t *item = scriptableItemChildAtIndex(self->scriptable, active);

    self->delegate->selectionDidChange(self, item, self->context);
}

static void
_edit_did_activate (GtkButton* button, gpointer user_data) {
    // TODO: open editor
}
