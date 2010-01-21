#include "../../deadbeef.h"

#include <stdio.h>
#include <stdlib.h>

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

#define DEFAULT_COMMAND "notify-send '%t' '%a - %b'"

static const char settings_dlg[] =
    "property Command entry notification.command \"" DEFAULT_COMMAND "\";\n"
;

static void
show_notification (DB_playItem_t *track)
{
    char cmd [1024];
    deadbeef->pl_format_title (track, cmd, sizeof (cmd), -1, deadbeef->conf_get_str ("notification.command", DEFAULT_COMMAND));
    //system (cmd);
}

static int
songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    DB_playItem_t *track = deadbeef->pl_get_for_idx (ev->to);
    if (track) {
        show_notification (track);
    }
    return 0;
}

static int
notification_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (songchanged), 0);
    return 0;
}

static int
notification_start (void) {
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (songchanged), 0);
    return 0;
}

DB_plugin_t *
notification_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
static DB_misc_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "Current track notification",
    .plugin.descr = "Displays notification when current track is changed",
    .plugin.author = "Viktor Semykin",
    .plugin.email = "thesame.ml@gmail.com",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = notification_start,
    .plugin.stop = notification_stop,
    .plugin.configdialog = settings_dlg
};

