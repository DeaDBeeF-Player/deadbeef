/*
    KDE StatusNotifier plugin for DeaDBeeF
    Copyright (C) 2015 Giulio Bernardi

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

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <deadbeef/deadbeef.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "../gtkui/gtkui_api.h"
#include "ddb_statusnotifier.h"
#include "statusnotifier.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(...)

DB_functions_t *deadbeef;
static DB_statusnotifier_plugin_t plugin;
static ddb_gtkui_t *gtkui_plugin;

static StatusNotifierItem *notifier;
static int sn_plugin_enabled = 1;
static int sn_plugin_initialized = 0;

static int
sn_plugin_stop (void) {
    trace ("DDB_SN: plugin stop\n");
    if (notifier) {
        sn_destroy (notifier);
    }
    notifier = NULL;
    sn_finalize();
    return 0;
}

static void
notifier_create_status_icon_from_icon_name (const char * icon_name);

static gboolean
sn_plugin_setup (void *ctx) {
    sn_plugin_initialized = 1;
    if (!gtkui_plugin) {
        gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id ("gtkui_1");
        if (!gtkui_plugin) {
            gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id ("gtkui3_1");
        }
    }
    if (!gtkui_plugin) {
        trace ("DDB_SN: failed to connect to gtkui plugin\n");
        return FALSE;
    }
    sn_plugin_enabled  = deadbeef->conf_get_int ("statusnotifier.enable", 1);
    if (!sn_plugin_enabled) {
        if (notifier) {
            sn_destroy (notifier);
            notifier = NULL;
        }
        gtkui_plugin->override_builtin_statusicon (0);
        return FALSE;
    }
    gtkui_plugin->override_builtin_statusicon (1);
    trace ("DDB_SN: sn_plugin_setup ()\n");

    if (!notifier) {
        notifier_create_status_icon_from_icon_name ("deadbeef");
    }
    return FALSE;
}

static void
on_notifier_activate (StatusNotifierItem *sn, int x, int y) {
    gtkui_plugin->mainwin_toggle_visible ();
}

static void
on_notifier_secondary_activate (StatusNotifierItem *sn, int x, int y) {
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
}

static void
on_notifier_reg_failed (StatusNotifierItem *sn, char *iconstr) {
    fprintf (stderr,"Cannot create status notifier, falling back to GtkStatusIcon\n");
    gtkui_plugin->override_builtin_statusicon (0);
}

static void
on_notifier_popup_menu (StatusNotifierItem *sn, int x, int y) {
    gtkui_plugin->show_traymenu ();
}

static void
on_notifier_scroll (StatusNotifierItem *sn, int delta, SN_SCROLLDIR orientation) {
    if (deadbeef->conf_get_int ("tray.scroll_changes_track", 0)) {
        if (delta > 0) {
            deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
        }
        else {
            deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
        }
        return;
    }

    float vol = deadbeef->volume_get_db ();
    int sens = deadbeef->conf_get_int ("gtkui.tray_volume_sensitivity", 1);
    if (orientation == Horizontal) {
        delta = -delta;
    }
    if (delta < 0) {
        vol -= sens;
    }
    else if (delta > 0) {
        vol += sens;
    }

    if (vol > 0) {
        vol = 0;
    }
    else if (vol < deadbeef->volume_get_min_db ()) {
        vol = deadbeef->volume_get_min_db ();
    }
    deadbeef->volume_set_db (vol);
}

static void
notifier_initialize_status_icon (const char * iconstr) {
    trace ("DDB_SN: connecting button tray signals\n");
    sn_hook_on_registration_error (notifier,
            (cb_registration_error)on_notifier_reg_failed,
            g_strdup (iconstr),g_free);
    sn_hook_on_context_menu (notifier,on_notifier_popup_menu);
    sn_hook_on_activate (notifier,on_notifier_activate);
    sn_hook_on_secondary_activate (notifier,on_notifier_secondary_activate);
    sn_hook_on_scroll (notifier,on_notifier_scroll);
    sn_register_item (notifier);
}

static void
notifier_create_status_icon_from_file (const char * iconfile) {
    GError *err = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (iconfile, &err);
    notifier = sn_create_with_icondata ("deadbeef-notifier",ApplicationStatus,pixbuf);
    sn_set_tooltip_icondata (notifier,pixbuf);
    notifier_initialize_status_icon (iconfile);
}

static void
notifier_create_status_icon_from_icon_name (const char * icon_name) {
    notifier = sn_create_with_iconname ("deadbeef-notifier", ApplicationStatus, icon_name);
    sn_set_tooltip_iconname (notifier,icon_name);
    notifier_initialize_status_icon (icon_name);
}

static void
notifier_set_status_icon_tooltip (const char *title, const char *text) {
    sn_set_tooltip_title (notifier, title);
    sn_set_tooltip_text (notifier, text);
}

static gboolean
notifier_is_status_icon_allocated (void) {
    return notifier != NULL ? TRUE : FALSE;
}

static int
sn_plugin_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    int enabled;
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        enabled = deadbeef->conf_get_int ("statusnotifier.enable", 1);
        if (sn_plugin_initialized && sn_plugin_enabled != enabled) {
            sn_plugin_enabled = enabled;
            g_idle_add (sn_plugin_setup, NULL);
        }
        break;
    case DB_EV_PLUGINSLOADED:
        g_type_init ();
        g_idle_add (sn_plugin_setup, NULL);
        break;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Enable\" checkbox statusnotifier.enable 1;\n"
;

static DB_statusnotifier_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.version_major = 0,
    .plugin.plugin.version_minor = 1,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "statusnotifier",
    .plugin.plugin.name = "KDE Status Notifier",
    .plugin.plugin.descr = "System tray icon support for KDE 5",
    .plugin.plugin.copyright =
        "KDE StatusNotifier plugin for DeaDBeeF\n"
        "Copyright (C) 2015 Giulio Bernardi\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.stop = sn_plugin_stop,
    .plugin.plugin.configdialog = settings_dlg,
    .plugin.plugin.message = sn_plugin_message,
};

DB_plugin_t *
statusnotifier_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
