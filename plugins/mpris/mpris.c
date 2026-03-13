/*
    MPRIS plugin for DeaDBeeF Player
    Copyright (C) Peter Lamby and other contributors
    See the file COPYING for more details
*/

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "mprisServer.h"
#include "logging.h"

DB_functions_t *deadbeef;
DB_misc_t plugin;

static GThread *mprisThread;
static struct MprisData mprisData;
static int oldLoopStatus = -1;
static int oldShuffleStatus = -1;

static int onStart() {
    oldLoopStatus = mprisData.deadbeef->conf_get_int("playback.loop", 0);
    oldShuffleStatus = mprisData.deadbeef->conf_get_int("playback.order", PLAYBACK_ORDER_LINEAR);
    mprisData.previousAction = mprisData.deadbeef->conf_get_int(SETTING_PREVIOUS_ACTION, PREVIOUS_ACTION_PREV_OR_RESTART);

#if (GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 32)
    mprisThread = g_thread_create(startServer, (void *)&mprisData, TRUE, NULL);
#else
    mprisThread = g_thread_new("mpris-listener", startServer, (void *)&mprisData);
#endif
    return 0;
}

static int onStop() {
    debug("Emitting stop event on player exit\n");
    emitPlaybackStatusChanged(OUTPUT_STATE_STOPPED, &mprisData);
    
    stopServer();

#if (GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 32)
    g_thread_join(mprisThread);
#else
    g_thread_unref(mprisThread);
#endif

    if (mprisData.artworkData.artwork) {
        free(mprisData.artworkData.path);
        free(mprisData.artworkData.default_path);
        mprisData.artworkData.path = NULL;
        mprisData.artworkData.default_path = NULL;
    }

    return 0;
}

static int onDisconnect() {
    if (mprisData.artworkData.artwork) {
        mprisData.artworkData.artwork->cancel_queries_with_source_id(mprisData.artworkData.source_id);
    }
    return 0;
}

static int onConnect() {
    mprisData.prevOrRestart = NULL;

    ddb_artwork_plugin_t *artworkPlugin = (ddb_artwork_plugin_t *)mprisData.deadbeef->plug_get_for_id ("artwork2");

    if (artworkPlugin != NULL) {
        debug("artwork plugin detected... album art support enabled\n");
        if (artworkPlugin) {
            mprisData.artworkData.artwork = artworkPlugin;
            mprisData.artworkData.source_id = artworkPlugin->allocate_source_id();
            mprisData.artworkData.path = NULL;
            mprisData.artworkData.default_path = malloc(PATH_MAX);
            if (mprisData.artworkData.default_path) {
                strcpy(mprisData.artworkData.default_path,"file://");
                size_t offset = strlen("file://");
                artworkPlugin->default_image_path(mprisData.artworkData.default_path + offset, PATH_MAX - offset);
            }
        }
    } else {
        debug("artwork plugin not detected... album art support disabled\n");
    }

    DB_plugin_t **plugins = mprisData.deadbeef->plug_get_list();
    for (int i = 0; plugins[i]; i++) {
        if (plugins[i]->get_actions != NULL) {
            for (DB_plugin_action_t *dbaction = plugins[i]->get_actions(NULL); dbaction; dbaction = dbaction->next) {
                if (strcmp(dbaction->name, "prev_or_restart") == 0) {
                    debug("prev_or_restart command detected... previous or restart support enabled\n");
                    mprisData.prevOrRestart = dbaction;
                    break;
                }
            }

            if (mprisData.prevOrRestart != NULL) {
                break;
            }
        }
    }


    if (mprisData.prevOrRestart == NULL) {
        debug("prev_or_restart command not detected... previous or restart support disabled\n");
    }

    return 0;
}

//***********************
//* Handels signals for *
//* - Playback status   *
//* - Metadata          *
//* - Volume            *
//* - Seeked            *
//* - Loop status       *
//* - Shuffle status    *
//***********************
static int handleEvent (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    DB_functions_t *deadbeef = mprisData.deadbeef;

    switch (id) {
        case DB_EV_SEEKED:
            debug("DB_EV_SEEKED event received\n");
            emitSeeked(((ddb_event_playpos_t *) ctx)->playpos);
            break;
        case DB_EV_TRACKINFOCHANGED:
            debug("DB_EV_TRACKINFOCHANGED event received\n");
            emitMetadataChanged(-1, &mprisData);
            emitCanGoChanged(&mprisData);
            emitSeeked(deadbeef->streamer_get_playpos());
            break;
        case DB_EV_SELCHANGED:
        case DB_EV_PLAYLISTSWITCHED:
            emitCanGoChanged(&mprisData);
            break;
        case DB_EV_SONGSTARTED:
            debug("DB_EV_SONGSTARTED event received\n");
            emitMetadataChanged(-1, &mprisData);
            emitPlaybackStatusChanged(OUTPUT_STATE_PLAYING, &mprisData);
            break;
        case DB_EV_PAUSED:
            debug("DB_EV_PAUSED event received\n");
            emitPlaybackStatusChanged(p1 ? OUTPUT_STATE_PAUSED : OUTPUT_STATE_PLAYING, &mprisData);
            break;
        case DB_EV_STOP:
            debug("DB_EV_STOP event received\n");
            emitPlaybackStatusChanged(OUTPUT_STATE_STOPPED, &mprisData);
            break;
        case DB_EV_VOLUMECHANGED:
            debug("DB_EV_VOLUMECHANGED event received\n");
            emitVolumeChanged(deadbeef->volume_get_db());
            break;
        case DB_EV_CONFIGCHANGED:
            debug("DB_EV_CONFIGCHANGED event received\n");
            if (oldShuffleStatus != -1) {
                int newLoopStatus = mprisData.deadbeef->conf_get_int("playback.loop", PLAYBACK_MODE_LOOP_ALL);
                int newShuffleStatus = mprisData.deadbeef->conf_get_int("playback.order", PLAYBACK_ORDER_LINEAR);

                if (newLoopStatus != oldLoopStatus) {
                    debug("LoopStatus changed %d\n", newLoopStatus);
                    emitLoopStatusChanged(oldLoopStatus = newLoopStatus);
                } if (newShuffleStatus != oldShuffleStatus) {
                    debug("ShuffleStatus changed %d\n", newShuffleStatus);
                    emitShuffleStatusChanged(oldShuffleStatus = newShuffleStatus);
                }

                mprisData.previousAction = mprisData.deadbeef->conf_get_int(SETTING_PREVIOUS_ACTION, PREVIOUS_ACTION_PREV_OR_RESTART);
            }
            if (deadbeef->conf_get_int ("mpris.trace", 0)) {
                plugin.plugin.flags |= DDB_PLUGIN_FLAG_LOGGING;
            }
            else {
                plugin.plugin.flags &= ~DDB_PLUGIN_FLAG_LOGGING;
            }
            break;
        default:
            break;
    }

    return 0;
}

#define STR(x) #x
#define XSTR(x) STR(x)

static const char settings_dlg[] =
    "property \"\\\"Previous\\\" action behavior\" select[2] " SETTING_PREVIOUS_ACTION " " XSTR(PREVIOUS_ACTION_PREV_OR_RESTART) " \"Previous\" \"Previous or restart current track\";"
    "property \"Enable logging\" checkbox mpris.trace 0;\n"
    "property \"Disable shuffle and repeat\" checkbox mpris.disable_shuffle_repeat 0;\n"
;


DB_misc_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 2,
    .plugin.version_minor = 0,
    .plugin.id = "mpris",
    .plugin.name ="MPRIS (multimedia keys support)",
    .plugin.descr =
        "Control playback using multimedia keys.\n"
        "\n"
        "This requires a running MPRIS server.\n"
        "GNOME and KDE already have one / no configuration necessary.\n"
        "Otherwise you may need to install and configure it manually.\n"
        ,
    .plugin.copyright =
        "MPRIS plugin for DeaDBeeF Player\n"
        "Copyright (C) 2014-2021 Peter Lamby <peterlamby@web.de>\n"
        "And other contributors:\n"
        "* Abi Hafshin Alfarouq\n"
        "* Evgeniy Gerasimenko\n"
        "* Evgeny Kravchenko\n"
        "* George Borisov\n"
        "* Jakub Wasylków\n"
        "* Jan Tojnar\n"
        "* Michael Livshin\n"
        "* Nicolai Syvertsen\n"
        "* Oleksiy Yakovenko\n"
        "* Toad King\n"
        "* yut23\n"
        "\n"
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation, either version 2 of the License, or\n"
        "(at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
        ,
    .plugin.website = "https://github.com/DeaDBeeF-Player/deadbeef-mpris2-plugin",
    .plugin.start = onStart,
    .plugin.stop = onStop,
    .plugin.connect = onConnect,
    .plugin.disconnect = onDisconnect,
    .plugin.configdialog = settings_dlg,
    .plugin.message = handleEvent,
};

DB_plugin_t * mpris_load (DB_functions_t *ddb) {
    deadbeef = mprisData.deadbeef = ddb;

    return DB_PLUGIN(&plugin);
}
