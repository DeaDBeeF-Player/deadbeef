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
#include "gtkScriptableListEditWindowController.h"
#include "gtkScriptableListEditViewController.h"
#include "../../../gettext.h"
#include "../support.h"

struct gtkScriptableListEditWindowController_t {
    scriptableItem_t *scriptable;
    GtkWidget *window;
    GtkWidget *reset_button;
    gtkScriptableListEditViewController_t *content_view_controller;
    gtkScriptableListEditViewControllerDelegate_t content_view_controller_delegate;

    gtkScriptableListEditWindowControllerDelegate_t *delegate;
    void *context;
};

static void
_reset_did_activate (GtkButton* button, gpointer user_data);

static void
_close_did_activate (GtkButton* button, gpointer user_data);

static void
_window_did_close (GObject *object, gpointer user_data);

static gboolean
_did_press_key (GtkWidget* window, GdkEventKey *event, gpointer user_data);

static void
_scriptable_did_change (gtkScriptableListEditViewController_t *view_controller, gtkScriptableChange_t change_type, void *context);

gtkScriptableListEditWindowController_t *
gtkScriptableListEditWindowControllerNew (void) {
    gtkScriptableListEditWindowController_t *self = calloc (1, sizeof  (gtkScriptableListEditWindowController_t));

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    self->window = window;
    g_object_ref (window);

    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    gtkScriptableListEditViewController_t *content_view_controller = gtkScriptableListEditViewControllerNew();

    self->content_view_controller_delegate.scriptable_did_change = _scriptable_did_change;
    gtkScriptableListEditViewControllerSetDelegate(content_view_controller, &self->content_view_controller_delegate, self);

    gtkScriptableListEditViewControllerLoad(content_view_controller);
    self->content_view_controller = content_view_controller;

    GtkWidget *content_view =
    gtkScriptableListEditViewControllerGetView(content_view_controller);

    gtk_box_pack_start (GTK_BOX(vbox), content_view, TRUE, TRUE, 0);

    GtkWidget *separator = gtk_hseparator_new ();
    gtk_widget_show (separator);
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

    GtkWidget *button_padding_box = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (button_padding_box);
    gtk_box_pack_start (GTK_BOX (vbox), button_padding_box, FALSE, FALSE, 8);

    GtkWidget *button_box = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (button_box);
    gtk_box_pack_start (GTK_BOX (button_padding_box), button_box, TRUE, TRUE, 8);

    GtkWidget *reset_button = gtk_button_new_with_label(_("Reset"));
    gtk_widget_show (reset_button);
    gtk_box_pack_start(GTK_BOX(button_box), reset_button, FALSE, FALSE, 0);
    self->reset_button = reset_button;

    GtkWidget *close_button = gtk_button_new_with_label(_("Close"));
    gtk_widget_show (close_button);
    gtk_box_pack_end(GTK_BOX(button_box), close_button, FALSE, FALSE, 0);

    g_signal_connect ((gpointer)reset_button, "clicked", G_CALLBACK (_reset_did_activate), self);

    g_signal_connect ((gpointer)close_button, "clicked", G_CALLBACK (_close_did_activate), self);

    g_signal_connect ((gpointer)window, "destroy", G_CALLBACK (_window_did_close), self);

    g_signal_connect((gpointer)window, "key_press_event", G_CALLBACK(_did_press_key), self);

    return self;
}

void
gtkScriptableListEditWindowControllerFree (gtkScriptableListEditWindowController_t *self) {
    gtkScriptableListEditViewControllerFree (self->content_view_controller);
    g_object_unref (self->window);
    free (self);
}

void
gtkScriptableListEditWindowControllerRunModal(gtkScriptableListEditWindowController_t *self, GtkWindow *modal_parent) {
    gtk_window_set_transient_for(GTK_WINDOW(self->window), modal_parent);
    gtk_widget_show(self->window);
    gtk_window_set_modal(GTK_WINDOW(self->window), TRUE);
}

void
gtkScriptableListEditWindowControllerSetScriptable(gtkScriptableListEditWindowController_t *self, scriptableItem_t *scriptable) {
    self->scriptable = scriptable;

    gboolean resettable = FALSE;

    if (scriptable != NULL) {
        resettable = 0 != (scriptableItemFlags(scriptable) & SCRIPTABLE_FLAG_CAN_RESET);
    }

    if (resettable) {
        gtk_widget_show(self->reset_button);
    }
    else {
        gtk_widget_hide(self->reset_button);
    }

    gtkScriptableListEditViewControllerSetScriptable(self->content_view_controller, scriptable);
}

void
gtkScriptableListEditWindowControllerSetTitle(gtkScriptableListEditWindowController_t *self, const char *title) {
    gtk_window_set_title(GTK_WINDOW(self->window), title);
}

void
gtkScriptableListEditWindowControllerSetDelegate(gtkScriptableListEditWindowController_t *self, gtkScriptableListEditWindowControllerDelegate_t *delegate, void *context) {
    self->delegate = delegate;
    self->context = context;
}

static void
_reset_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableListEditWindowController_t *self = user_data;
    scriptableItemReset(self->scriptable);
    gtkScriptableListEditViewControllerSetScriptable(self->content_view_controller, self->scriptable);
    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change(self, ScriptableItemChangeUpdate, self->context);
    }
}

static void
_window_did_close (GObject *object, gpointer user_data) {
    gtkScriptableListEditWindowController_t *self = user_data;
    if (self->delegate != NULL && self->delegate->window_did_close != NULL) {
        self->delegate->window_did_close(self, self->context);
    }
}

static void
_close_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableListEditWindowController_t *self = user_data;
    gtk_widget_destroy(GTK_WIDGET(self->window));
}

static gboolean
_did_press_key (GtkWidget* window, GdkEventKey *event, gpointer user_data) {
    gtkScriptableListEditWindowController_t *self = user_data;
    if (event->keyval == GDK_Escape) {
        gtk_widget_destroy(GTK_WIDGET(self->window));
        return TRUE;
    }
    return FALSE;
}

static void
_scriptable_did_change (gtkScriptableListEditViewController_t *view_controller, gtkScriptableChange_t change_type, void *context) {
    gtkScriptableListEditWindowController_t *self = context;
    if (self->delegate != NULL && self->delegate->scriptable_did_change != NULL) {
        self->delegate->scriptable_did_change(self, change_type, self->context);
    }
}

