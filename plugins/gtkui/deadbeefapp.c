/*
    DeaDBeeF -- the music player
    GtkApplication implementation
    Copyright (C) 2017 Nicolai Syvertsen

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

#if GTK_CHECK_VERSION(3,10,0)

#include "deadbeefapp.h"
#include "gtkui.h"
#include "prefwin/prefwin.h"
#include "support.h"

struct _DeadbeefApp
{
    GtkApplication parent;

    GSimpleAction *logaction;
};

G_DEFINE_TYPE(DeadbeefApp, deadbeef_app, GTK_TYPE_APPLICATION);

DeadbeefApp *gapp;

static void
appmenu_quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
    gtkui_quit ();
}

static void
appmenu_preferences_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
    prefwin_run (-1);
}

static void
appmenu_log_change_state (GSimpleAction *action,
                GVariant      *value,
                gpointer       user_data)
{
    gboolean val = g_variant_get_boolean (value);
    gtkui_show_log_window (val);
}

static void
appmenu_about_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
    GtkWidget *item = lookup_widget (mainwin, "about1");
    gtk_menu_item_activate ( GTK_MENU_ITEM (item));
}

static GActionEntry app_entries[] = {
    { "preferences", appmenu_preferences_activated, NULL, NULL, NULL },
    { "log", NULL, NULL, "false", appmenu_log_change_state },
    { "about", appmenu_about_activated, NULL, NULL, NULL },
    { "quit", appmenu_quit_activated, NULL, NULL, NULL }
};

static void
deadbeef_app_init (DeadbeefApp *app)
{
}

static void
deadbeef_app_activate (GApplication *application) {
    if (mainwin) {
        gtk_window_present (GTK_WINDOW (mainwin));
    }
}

static void
deadbeef_app_startup (GApplication *application) {
    G_APPLICATION_CLASS (deadbeef_app_parent_class)->startup (application);

#if GTK_CHECK_VERSION(3,14,0)
    int preferappmenu = gtk_application_prefers_app_menu (GTK_APPLICATION(application));
#else
    int preferappmenu = 1;
#endif

    if (preferappmenu) {
        g_action_map_add_action_entries (G_ACTION_MAP (application), app_entries, G_N_ELEMENTS (app_entries), application);
        DEADBEEF_APP (application)->logaction = G_SIMPLE_ACTION (g_action_map_lookup_action ( G_ACTION_MAP (application), "log"));
    } else {
        gtk_application_set_app_menu (GTK_APPLICATION(application), NULL);
    }

    gtkui_mainwin_init ();
}

GSimpleAction *
deadbeef_app_get_log_action(DeadbeefApp *application) {
    return application->logaction;
}

static void
deadbeef_app_shutdown (GApplication *application) {
    G_APPLICATION_CLASS (deadbeef_app_parent_class)->shutdown (application);
}

static void
deadbeef_app_class_init (DeadbeefAppClass *class)
{
    G_APPLICATION_CLASS (class)->activate = deadbeef_app_activate;
    G_APPLICATION_CLASS (class)->startup = deadbeef_app_startup;
    G_APPLICATION_CLASS (class)->shutdown = deadbeef_app_shutdown;
}

DeadbeefApp *
deadbeef_app_new (void)
{
    return g_object_new (DEADBEEF_APP_TYPE,
                       "application-id", "music.deadbeef.player",
                       "flags", G_APPLICATION_FLAGS_NONE,
                       NULL);
}
#endif
