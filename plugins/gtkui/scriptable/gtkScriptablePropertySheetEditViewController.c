/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

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

#include "gtkScriptablePropertySheetEditViewController.h"
#include "../pluginconf.h"
#include <string.h>
#include <stdlib.h>

struct gtkScriptablePropertySheetEditViewController_t {
    scriptableItem_t *scriptable;
    GtkWidget *view;
    GtkWidget *content_view;

    gtkui_script_datamodel_t script_model;

    gtkScriptablePropertySheetEditViewControllerDelegate_t *delegate;
    void *context;
};

gtkScriptablePropertySheetEditViewController_t *
gtkScriptablePropertySheetEditViewControllerNew (void) {
    gtkScriptablePropertySheetEditViewController_t *self = calloc (1, sizeof (gtkScriptablePropertySheetEditViewController_t));

    GtkWidget *view = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(view);
    self->view = view;

    return self;
}

void
gtkScriptablePropertySheetEditViewControllerFree (gtkScriptablePropertySheetEditViewController_t *self) {
    g_object_unref (self->view);
    free (self);
}

static void
_set_param (void *context, const char *key, const char *value) {
    scriptableItem_t *item = context;
    scriptableItemSetPropertyValueForKey(item, value, key);
}

static void
_get_param (void *context, const char *key, char *value, int len, const char *def) {
    scriptableItem_t *item = context;
    const char *itemValue = scriptableItemPropertyValueForKey(item, key);
    if (itemValue == NULL) {
        itemValue = def;
    }
    *value = 0;
    strncat(value, itemValue, len);
}

void
gtkScriptablePropertySheetEditViewControllerSetScriptable (
                                                           gtkScriptablePropertySheetEditViewController_t *self,
                                                           scriptableItem_t *scriptable) {
    self->scriptable = scriptable;

    if (self->content_view != NULL) {
        gtk_container_remove(GTK_CONTAINER(self->view), self->content_view);
        self->content_view = NULL;
    }

    if (scriptable == NULL) {
        return;
    }

    const char *layout = scriptableItemConfigDialog(self->scriptable);

    if (layout == NULL) {
        return;
    }

    self->script_model.context = scriptable;
    self->script_model.updates_immediately = TRUE;
    self->script_model.set_param = _set_param;
    self->script_model.get_param = _get_param;

    GtkWidget *content = gtkui_create_ui_from_script(layout, &self->script_model, NULL);
    gtk_widget_show(content);
    self->content_view = content;
    gtk_box_pack_start(GTK_BOX(self->view), self->content_view, TRUE, TRUE, 0);
}

void
gtkScriptablePropertySheetEditViewControllerSetDelegate (
                                                         gtkScriptablePropertySheetEditViewController_t *self,
                                                         gtkScriptablePropertySheetEditViewControllerDelegate_t *delegate,
                                                         void *context) {
    self->delegate = delegate;
    self->context = context;
}

GtkWidget *
gtkScriptablePropertySheetEditViewControllerGetView (gtkScriptablePropertySheetEditViewController_t *self) {
    return self->view;
}
