#include "../../deadbeef.h"

#include <stdio.h>
#include <libnotify/notify.h>

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

static int
songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    DB_playItem_t *track = deadbeef->pl_get_for_idx (ev->to);
    const char *artist = deadbeef->pl_find_meta (track, "artist");
    const char *album = deadbeef->pl_find_meta (track, "album");
    const char *title = deadbeef->pl_find_meta (track, "title");
    char body [1024];
    GError *err;
    snprintf (body, sizeof (body), "%s - %s", artist, album);
    NotifyNotification *ntf = notify_notification_new (title, body, NULL, NULL);
    notify_notification_set_timeout (ntf, NOTIFY_EXPIRES_DEFAULT);
    notify_notification_show (ntf, NULL);
    return 0;
}

static int
notification_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (songchanged), 0);
}

static int
notification_start (void) {
    notify_init ("deadbeef");

    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (songchanged), 0);
    return 1;
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
    .plugin.stop = notification_stop
};

