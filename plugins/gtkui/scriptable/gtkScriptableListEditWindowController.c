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

#include "gtkScriptableListEditWindowController.h"
#include "gtkScriptableListEditViewController.h"
#include "../../../gettext.h"

struct gtkScriptableListEditWindowController_t {
    scriptableItem_t *scriptable;
    GtkWidget *window;
    gtkScriptableListEditViewController_t *content_view_controller;
    gtkScriptableListEditWindowControllerDelegate_t *delegate;
    void *context;
};

static void
_reset_did_activate (GtkButton* button, gpointer user_data);

static void
_close_did_activate (GtkButton* button, gpointer user_data);

gtkScriptableListEditWindowController_t *
gtkScriptableListEditWindowControllerNew (void) {
    gtkScriptableListEditWindowController_t *self = calloc (1, sizeof  (gtkScriptableListEditWindowController_t));

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    self->window = window;

    gtk_window_set_title (GTK_WINDOW (window), "Scriptable list edit");
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    gtkScriptableListEditViewController_t *content_view_controller = gtkScriptableListEditViewControllerNew();

    // view delegate needs to be set here, if necessary

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

    GtkWidget *close_button = gtk_button_new_with_label(_("Close"));
    gtk_widget_show (close_button);
    gtk_box_pack_end(GTK_BOX(button_box), close_button, FALSE, FALSE, 0);


    g_signal_connect ((gpointer)reset_button, "clicked", G_CALLBACK (_reset_did_activate), self);

    g_signal_connect ((gpointer)close_button, "clicked", G_CALLBACK (_close_did_activate), self);

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
    gtkScriptableListEditViewControllerSetScriptable(self->content_view_controller, scriptable);
}


static void
_reset_did_activate (GtkButton* button, gpointer user_data) {
//    gtkScriptableListEditWindowController_t *self = user_data;
}

static void
_close_did_activate (GtkButton* button, gpointer user_data) {
    gtkScriptableListEditWindowController_t *self = user_data;
    gtk_widget_hide(self->window);
}
