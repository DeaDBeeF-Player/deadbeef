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
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "../../deadbeef.h"
#include "ddb_statusnotifier.h"
#include "statusnotifier.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

DB_functions_t *deadbeef;
static DB_statusnotifier_plugin_t plugin;
static ddb_gtkui_t *gtkui_plugin;

static StatusNotifierItem *notifier;
static statusicon_functions_t *gtk_statusicon_functions;
static statusicon_functions_t status_notifier_functions;
static statusicon_functions_t **statusicon_funcptr;

int sn_plugin_stop(void) {
    trace("DDB_SN: plugin stop\n");
    if (notifier)
        sn_destroy(notifier);
    notifier = NULL;
    return 0;
}

int sn_plugin_connect(void) {
    trace("DDB_SN: plugin connect\n");
    if (gtkui_plugin) {
        return 0;
    }
    const DB_plugin_t *plugin = deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID);
    if (plugin) {
        gtkui_plugin = (ddb_gtkui_t *) plugin;
    }
    if (!gtkui_plugin) {
        trace("DDB_SN: failed to connect to gtkui plugin\n");
        return -1;
    }

    return 0;
}

void sn_plugin_setup(statusicon_functions_t **functions) {
    trace("DDB_SN: sn_plugin_setup()\n");
    if (sn_plugin_connect()!=0) {
        return;
    }

    gtk_statusicon_functions = *functions;
    statusicon_funcptr = functions;
    *functions = &status_notifier_functions;
    trace("DDB_SN: status icon functions hooked\n");
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
setup_fallback_gtk_status_icon(StatusNotifierItem *sn, char *str) {
    *statusicon_funcptr = gtk_statusicon_functions;
    const char * icon_name = sn_get_icon_name(sn);
    if (icon_name == NULL || strcmp(icon_name,str)) {
        gtk_statusicon_functions->create_status_icon_from_file(str);
    }
    else {
        gtk_statusicon_functions->create_status_icon_from_icon_name(str);
    }
    int hide_tray_icon = deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0);
    gtk_statusicon_functions->set_status_icon_visible(hide_tray_icon ? FALSE : TRUE);
}

static void
on_notifier_reg_failed (StatusNotifierItem *sn, char *iconstr) {
    fprintf (stderr,"Cannot create status notifier, falling back to GtkStatusIcon\n");
    setup_fallback_gtk_status_icon(sn,iconstr);
}

static void
on_notifier_popup_menu (StatusNotifierItem *sn, int x, int y) {
    GtkWidget *traymenu = gtkui_plugin->get_traymenu();
    gtk_menu_popup (GTK_MENU (traymenu), NULL, NULL,NULL, NULL,0, gtk_get_current_event_time());
}

static void
on_notifier_scroll(StatusNotifierItem *sn, int delta, SN_SCROLLDIR orientation) {
    if (orientation==Horizontal)
        delta=-delta;
    delta = delta < 0 ? -1 : delta > 0 ? 1 : 0;
    gtkui_plugin->trayicon_do_scroll(delta);
}

static void notifier_initialize_status_icon(char * iconstr) {
    trace ("DDB_SN: connecting button tray signals\n");
    sn_hook_on_registration_error(notifier,
            (cb_registration_error)on_notifier_reg_failed,
            g_strdup(iconstr),g_free);
    sn_hook_on_context_menu(notifier,on_notifier_popup_menu);
    sn_hook_on_activate(notifier,on_notifier_activate);
    sn_hook_on_secondary_activate(notifier,on_notifier_secondary_activate);
    sn_hook_on_scroll(notifier,on_notifier_scroll);
    sn_register_item(notifier);
}

static void notifier_create_status_icon_from_file(char * iconfile) {
    GError *err;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (iconfile, &err);
    notifier = sn_create_with_icondata("deadbeef-notifier",ApplicationStatus,pixbuf);
    sn_set_tooltip_icondata(notifier,pixbuf);
    notifier_initialize_status_icon(iconfile);
}

static void notifier_create_status_icon_from_icon_name(char * icon_name) {
    notifier = sn_create_with_iconname("deadbeef-notifier", ApplicationStatus, icon_name);
    sn_set_tooltip_iconname(notifier,icon_name);
    notifier_initialize_status_icon(icon_name);
}

static void notifier_set_status_icon_visible(gboolean visible) {
    SN_STATUS status =
            (visible == FALSE) ? Passive : Active;
    sn_set_status(notifier,status);
}

static void notifier_set_status_icon_tooltip(const char *title, const char *text) {
    sn_set_tooltip_title (notifier, title);
    sn_set_tooltip_text (notifier, text);
}

static gboolean
notifier_is_status_icon_allocated(void) {
    return notifier != NULL ? TRUE : FALSE;
}

static statusicon_functions_t status_notifier_functions = {
        .is_status_icon_allocated = notifier_is_status_icon_allocated,
        .set_status_icon_visible = notifier_set_status_icon_visible,
        .create_status_icon_from_file = notifier_create_status_icon_from_file,
        .create_status_icon_from_icon_name = notifier_create_status_icon_from_icon_name,
        .set_status_icon_tooltip = notifier_set_status_icon_tooltip
};

#if GTK_CHECK_VERSION(3,0,0)
#define SN_PLUGIN_NAME "KDE Status Notifier (GTK3 UI)"
#define SN_PLUGIN_DESC "System tray icon support for KDE 5 (GTK3 UI)"
#else
#define SN_PLUGIN_NAME "KDE Status Notifier (GTK2 UI)"
#define SN_PLUGIN_DESC "System tray icon support for KDE 5 (GTK2 UI)"
#endif


static DB_statusnotifier_plugin_t plugin = {
    .plugin.plugin.api_vmajor = 1,
    .plugin.plugin.api_vminor = 8,
    .plugin.plugin.version_major = 0,
    .plugin.plugin.version_minor = 1,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = SN_PLUGIN_ID,
    .plugin.plugin.name = SN_PLUGIN_NAME,
    .plugin.plugin.descr = SN_PLUGIN_DESC,
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
    .plugin.plugin.connect = sn_plugin_connect,
    .setup = sn_plugin_setup,
};

DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
statusnotifier_gtk3_load (DB_functions_t *api) {
#else
statusnotifier_gtk2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
