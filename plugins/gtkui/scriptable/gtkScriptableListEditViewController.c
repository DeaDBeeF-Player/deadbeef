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

struct gtkScriptableListEditViewController_t {
    scriptableItem_t *scriptable;
    GtkWidget *view;
    gtkScriptableListEditViewControllerDelegate_t *delegate;
    void *context;
};

gtkScriptableListEditViewController_t *
gtkScriptableListEditViewControllerNew (void) {
    gtkScriptableListEditViewController_t *self = calloc (1, sizeof  (gtkScriptableListEditViewController_t));

    GtkWidget *vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);
    self->view = vbox;

    GtkWidget *list_view = gtk_tree_view_new ();
    gtk_widget_show (list_view);
    gtk_box_pack_start(GTK_BOX(vbox), list_view, TRUE, TRUE, 0);

    GtkWidget *button_box = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (button_box);
    gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *add_button = gtk_button_new_with_label ("➕");
    gtk_widget_show (add_button);
    GtkWidget *remove_button = gtk_button_new_with_label ("➖");
    gtk_widget_show (remove_button);
    GtkWidget *config_button = gtk_button_new_with_label ("⚙️");
    gtk_widget_show (config_button);
    GtkWidget *duplicate_button = gtk_button_new_with_label ("👯‍♀️");
    gtk_widget_show (duplicate_button);
    GtkWidget *savepreset_button = gtk_button_new_with_label ("💾");
    gtk_widget_show (savepreset_button);

    gtk_box_pack_start (GTK_BOX (button_box), add_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (button_box), remove_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (button_box), config_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (button_box), duplicate_button, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (button_box), savepreset_button, FALSE, FALSE, 0);

    return self;
}

void
gtkScriptableListEditViewControllerFree (gtkScriptableListEditViewController_t *self) {
    g_object_unref (self->view);
    free (self);
}

