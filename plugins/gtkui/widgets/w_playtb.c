/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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
#include <string.h>
#include "../callbacks.h"
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_playtb.h"

typedef struct {
    ddb_gtkui_widget_t base;
} w_playtb_t;

// play toolbar
ddb_gtkui_widget_t *
w_playtb_create (void) {
    w_playtb_t *w = malloc (sizeof (w_playtb_t));
    memset (w, 0, sizeof (w_playtb_t));
    w->base.widget = gtk_hbox_new (FALSE, 0);
    w->base.flags = DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE;
    gtk_widget_show (w->base.widget);

    GtkWidget *stopbtn;
    GtkWidget *image128;
    GtkWidget *playbtn;
    GtkWidget *image2;
    GtkWidget *pausebtn;
    GtkWidget *image3;
    GtkWidget *prevbtn;
    GtkWidget *image4;
    GtkWidget *nextbtn;
    GtkWidget *image5;

    stopbtn = gtk_button_new ();
    gtk_widget_show (stopbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), stopbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus (stopbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (stopbtn), GTK_RELIEF_NONE);

    image128 = gtk_image_new_from_stock ("gtk-media-stop", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image128);
    gtk_container_add (GTK_CONTAINER (stopbtn), image128);

    playbtn = gtk_button_new ();
    gtk_widget_show (playbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), playbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus (playbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (playbtn), GTK_RELIEF_NONE);

    image2 = gtk_image_new_from_stock ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image2);
    gtk_container_add (GTK_CONTAINER (playbtn), image2);

    pausebtn = gtk_button_new ();
    gtk_widget_show (pausebtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), pausebtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus (pausebtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (pausebtn), GTK_RELIEF_NONE);

    image3 = gtk_image_new_from_stock ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image3);
    gtk_container_add (GTK_CONTAINER (pausebtn), image3);

    prevbtn = gtk_button_new ();
    gtk_widget_show (prevbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), prevbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus (prevbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (prevbtn), GTK_RELIEF_NONE);

    image4 = gtk_image_new_from_stock ("gtk-media-previous", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image4);
    gtk_container_add (GTK_CONTAINER (prevbtn), image4);

    nextbtn = gtk_button_new ();
    gtk_widget_show (nextbtn);
    gtk_box_pack_start (GTK_BOX (w->base.widget), nextbtn, FALSE, FALSE, 0);
    gtk_widget_set_can_focus (nextbtn, FALSE);
    gtk_button_set_relief (GTK_BUTTON (nextbtn), GTK_RELIEF_NONE);

    image5 = gtk_image_new_from_stock ("gtk-media-next", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image5);
    gtk_container_add (GTK_CONTAINER (nextbtn), image5);
    w_override_signals (w->base.widget, w);

    g_signal_connect ((gpointer)stopbtn, "clicked", G_CALLBACK (on_stopbtn_clicked), NULL);
    g_signal_connect ((gpointer)playbtn, "clicked", G_CALLBACK (on_playbtn_clicked), NULL);
    g_signal_connect ((gpointer)pausebtn, "clicked", G_CALLBACK (on_pausebtn_clicked), NULL);
    g_signal_connect ((gpointer)prevbtn, "clicked", G_CALLBACK (on_prevbtn_clicked), NULL);
    g_signal_connect ((gpointer)nextbtn, "clicked", G_CALLBACK (on_nextbtn_clicked), NULL);
    return (ddb_gtkui_widget_t *)w;
}
