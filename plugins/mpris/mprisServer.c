/*
    MPRIS plugin for DeaDBeeF Player
    Copyright (C) Peter Lamby and other contributors
    See the file COPYING for more details
*/

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

#include <glib.h>
#include <gio/gio.h>

#include "logging.h"
#include "mprisServer.h"

extern DB_functions_t *deadbeef;
extern DB_misc_t plugin;

#define BUS_NAME "org.mpris.MediaPlayer2.DeaDBeeF"
#define OBJECT_NAME "/org/mpris/MediaPlayer2"
#define PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"
#define PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define CURRENT_TRACK -1

typedef GVariant* (*ProduceVariantCb)(const char *valueStr);

struct MetaFormatRecord {
    const char *fieldName;
    const char *valueFormat;
    const ProduceVariantCb produceVariantCb;
    char *bytecode;
};

static const char xmlForNode[] =
    "<node name='/org/mpris/MediaPlayer2'>"
    "    <interface name='org.mpris.MediaPlayer2'>"
    "        <method name='Raise'/>"
    "        <method name='Quit'/>"
    "        <property access='read'    name='CanQuit'             type='b'/>"
    "        <property access='read'    name='CanRaise'            type='b'/>"
    "        <property access='read'    name='HasTrackList'        type='b'/>"
    "        <property access='read'    name='Identity'            type='s'/>"
    "        <property access='read' name='DesktopEntry'        type='s'/>"
    "        <property access='read'    name='SupportedUriSchemes' type='as'/>"
    "        <property access='read'    name='SupportedMimeTypes'  type='as'/>"
    "    </interface>"
    "    <interface name='org.mpris.MediaPlayer2.Player'>"
    "        <method name='Next'/>"
    "        <method name='Previous'/>"
    "        <method name='Pause'/>"
    "        <method name='PlayPause'/>"
    "        <method name='Stop'/>"
    "        <method name='Play'/>"
    "        <method name='Seek'>"
    "            <arg name='Offset'      type='x'/>"
    "        </method>"
    "        <method name='SetPosition'>"
    "            <arg name='TrackId'     type='o'/>"
    "            <arg name='Position'    type='x'/>"
    "        </method>"
    "        <method name='OpenUri'>"
    "            <arg name='Uri'         type='s'/>"
    "        </method>"
    "        <signal name='Seeked'>"
    "            <arg name='Position'    type='x' direction='out'/>"
    "        </signal>"
    "        <property access='read'         name='PlaybackStatus' type='s'/>"
    "        <property access='readwrite' name='LoopStatus'     type='s'/>"
    "        <property access='readwrite' name='Rate'           type='d'/>"
    "        <property access='readwrite' name='Shuffle'        type='b'/>"
    "        <property access='read'      name='Metadata'       type='a{sv}'/>"
    "        <property access='readwrite' name='Volume'         type='d'/>"
    "        <property access='read'      name='Position'       type='x'>"
    "            <annotation name='org.freedesktop.DBus.Property.EmitsChangedSignal' value='false'/>"
    "        </property>"
    "        <property access='read'         name='MinimumRate'   type='d'/>"
    "        <property access='read'         name='MaximumRate'   type='d'/>"
    "        <property access='read'         name='CanGoNext'     type='b'/>"
    "        <property access='read'         name='CanGoPrevious' type='b'/>"
    "        <property access='read'         name='CanPlay'       type='b'/>"
    "        <property access='read'         name='CanPause'      type='b'/>"
    "        <property access='read'         name='CanSeek'       type='b'/>"
    "        <property access='read'         name='CanControl'    type='b'>"
    "            <annotation name='org.freedesktop.DBus.Property.EmitsChangedSignal' value='false'/>"
    "        </property>"
    "    </interface>"
    "</node>";

static GDBusConnection *globalConnection = NULL;
static GMainLoop *loop;

static gboolean bytecodeCompiled;

static GVariant* produceScalarString(const char *valueStr) {
    return g_variant_new_string(valueStr);
}

static GVariant* produceSingleStringArray(const char *valueStr) {
    GVariantBuilder arrayBuilder;
    g_variant_builder_init(&arrayBuilder, G_VARIANT_TYPE("as"));

    g_variant_builder_add(&arrayBuilder, "s", valueStr);

    return g_variant_builder_end(&arrayBuilder);
}

static GVariant* produceScalarInt(const char *valueStr) {
    gint32 value = atoi(valueStr);

    if (value <= 0) {
        return NULL;
    }

    return g_variant_new_int32(value);
}

static char* chompCr(char *str) {
    size_t len = strlen(str);

    if (len > 0 && str[len - 1] == '\r') {
        str[len - 1] = '\0';
    }

    return str;
}

static GVariant* produceArrayOfTokens(const char *valueStr) {
    char **tokens = g_strsplit(valueStr, "\n", -1);

    GVariantBuilder arrayBuilder;
    g_variant_builder_init(&arrayBuilder, G_VARIANT_TYPE("as"));

    for (char **token = tokens; *token; token++) {
        g_variant_builder_add(&arrayBuilder, "s", chompCr(*token));
    }

    g_strfreev(tokens);

    return g_variant_builder_end(&arrayBuilder);
}

static struct MetaFormatRecord metaFormatRecords[] = {
    { "xesam:title",          "%title%",                                                 produceScalarString      },
    { "xesam:album",          "%album%",                                                 produceScalarString      },
    { "xesam:artist",         "$if(%artist%,%artist%,Unknown Artist)",                   produceSingleStringArray },
    { "xesam:albumArtist",    "%album artist%",                                          produceSingleStringArray },
    { "xesam:trackNumber",    "%track number%",                                          produceScalarInt         },
    { "xesam:genre",          "%genre%",                                                 produceArrayOfTokens     },
    { "xesam:contentCreated", "%date%",                                                  produceScalarString      }, //TODO format date
    { "xesam:asText",         "$meta(unsynced lyrics)",                                  produceScalarString      },
    { "xesam:comment",        "$meta(comment)",                                          produceSingleStringArray },
    { "xesam:url",            "$if($strcmp($left(%_path_raw%,1),/),file://)%_path_raw%", produceScalarString      },
    { NULL                                                                                                        }
};

static void compileTfBytecode(DB_functions_t *deadbeef) {
    debug("Compiling tf bytecode\n");
    for (struct MetaFormatRecord *record = metaFormatRecords; record->fieldName; record++) {
        record->bytecode = deadbeef->tf_compile(record->valueFormat);
        assert(record->bytecode);
    }
}

static void freeTfBytecode(DB_functions_t *deadbeef) {
    debug("Freeing tf bytecode\n");
    for (struct MetaFormatRecord *record = metaFormatRecords; record->fieldName; record++) {
        deadbeef->tf_free(record->bytecode);
    }
}

static void coverartCallback(int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    struct MprisData *mprisData = (struct MprisData*) query->user_data;
    if (query->flags != DDB_ARTWORK_FLAG_CANCELLED) {
        char * cover_path = NULL;
        if (cover && cover->cover_found) {
            size_t prefix = strlen("file://");
            cover_path = malloc(strlen(cover->image_filename) + prefix + 1);
            strcpy(cover_path,"file://");
            strcpy(cover_path + prefix, cover->image_filename);
            debug("Loaded cover for %x, (path: %s)\n", query->track, cover_path);
        }

        // Replace cover
        char *old_cover = mprisData->artworkData.path;
        mprisData->artworkData.path = cover_path;
        free(old_cover);
        emitMetadataChanged(-1, mprisData);
    }

    if (cover) {
        mprisData->artworkData.artwork->cover_info_release(cover);
    }
    free(query);
}

GVariant* getMetadataForTrack(int track_id, struct MprisData *mprisData) {
    int id;
    DB_playItem_t *track = NULL;
    DB_functions_t *deadbeef = mprisData->deadbeef;
    ddb_playlist_t *pl = NULL;
    GVariant *tmp;
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    int playlistIndex;

    track = deadbeef->streamer_get_playing_track();
    if (track) {
        pl = deadbeef->plt_get_for_idx(deadbeef->streamer_get_current_playlist());
        id = deadbeef->plt_get_item_idx(pl, track, PL_MAIN);
        playlistIndex = deadbeef->streamer_get_current_playlist();
    }
    if (pl) {
        deadbeef->plt_unref(pl);
    }

    if (track != NULL) {
        char buf[500];
        int buf_size = sizeof(buf);
        int64_t duration = deadbeef->pl_get_item_duration(track) * 1000000;
        const char *album = deadbeef->pl_find_meta(track, "album");
        const char *artist = deadbeef->pl_find_meta(track, "artist");
        const char *uri = deadbeef->pl_find_meta(track, ":URI");

        deadbeef->pl_lock();

        sprintf(buf, "/DeaDBeeF/%d/%d", playlistIndex, id);
        debug("get Metadata trackid: %s\n", buf);
        g_variant_builder_add(builder, "{sv}", "mpris:trackid", g_variant_new("o", buf));

        debug("get Metadata duration: %" PRId64 "\n", duration);
        if (duration > 0) {
            g_variant_builder_add(builder, "{sv}", "mpris:length", g_variant_new("x", duration));
        }

        if (mprisData->artworkData.artwork != NULL) {
            const char *albumArtUri = NULL;
            ArtworkData_t *art_data = &(mprisData->artworkData);

            if (art_data->track == track) {
                if (art_data->path) {
                    debug("cover for %s ready. Artwork is: %s\n", album, art_data->path);
                    albumArtUri = art_data->path;
                }
                else {
                    debug("Cover not found, using default (path: %s)\n", art_data->default_path);
                    albumArtUri = art_data->default_path;
                }
            }
            else {
                ddb_cover_query_t *artworkQuery = calloc(sizeof(ddb_cover_query_t),1);
                if (artworkQuery) {
                    debug("getting cover for album %s\n", album);
                    artworkQuery->_size = sizeof(ddb_cover_query_t);
                    artworkQuery->track = track;
                    artworkQuery->user_data = mprisData;
                    artworkQuery->source_id = art_data->source_id;
                    art_data->artwork->cover_get(artworkQuery, coverartCallback);
                    // Replace track so that artwork won't be called multiple times
                    art_data->track = track;
                }
            }

            if (albumArtUri != NULL) {
                g_variant_builder_add(builder, "{sv}", "mpris:artUrl", g_variant_new("s", albumArtUri));
            }
        }

        // init on first access
        if (!bytecodeCompiled) {
            compileTfBytecode(deadbeef);
            bytecodeCompiled = TRUE;
        }

        for (struct MetaFormatRecord *record = metaFormatRecords; record->fieldName; record++) {
            assert(record->valueFormat);
            assert(record->produceVariantCb);
            assert(record->bytecode);

            ddb_tf_context_t ctx = {
                sizeof(ddb_tf_context_t),
                DDB_TF_CONTEXT_NO_DYNAMIC | DDB_TF_CONTEXT_MULTILINE,
                track,
                NULL,
                0,
                0,
                PL_MAIN,
                0
            };

            if (deadbeef->tf_eval(&ctx, record->bytecode, buf, buf_size) < 0) {
                error("failed to produce string for field %s\n", record->fieldName);
                continue;
            }

            if (g_str_equal(buf, "")) {
                debug("resulting string is empty, skipping %s field\n", record->fieldName);
                continue;
            }

            size_t len = strlen(buf);
            const char *end = NULL;
            if (!(g_utf8_validate (buf, -1, &end) && (end == buf + len))) {
                debug("meta string for '%s' failed utf8 validation: %s\n", record->fieldName, buf);
                continue;
            }

            debug("got string '%s' for field %s\n", buf, record->fieldName);

            GVariant *variant = record->produceVariantCb(buf);
            if (!variant) {
                debug("can't convert string '%s' to proper variant, skipping %s field\n", buf, record->fieldName);
                continue;
            }

            g_variant_builder_add(builder, "{sv}", record->fieldName, variant);
        }

        deadbeef->pl_unlock();
        deadbeef->pl_item_unref(track);
    } else {
        debug("get Metadata trackid: /org/mpris/MediaPlayer2/TrackList/NoTrack\n");
        g_variant_builder_add(builder, "{sv}", "mpris:trackid", g_variant_new("o", "/org/mpris/MediaPlayer2/TrackList/NoTrack"));
    }
    tmp = g_variant_builder_end(builder);
    g_variant_builder_unref(builder);

    return tmp;
}

gboolean deadbeef_can_seek(DB_functions_t *deadbeef) {
    gboolean can_seek = FALSE;
    DB_output_t *output = deadbeef->get_output();
    if (output) {
        DB_playItem_t *track = deadbeef->streamer_get_playing_track();
        if (track) {
            can_seek = deadbeef->pl_get_item_duration(track) > 0;
            deadbeef->pl_item_unref(track);
        }
    }
    return can_seek;
}

static gboolean deadbeef_hasselectedorplayingtrack(struct MprisData *userData, int offset) {
    DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;
    ddb_playlist_t *pl = NULL;
    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track();
    int idx;
    if (playing_track) {
        pl = deadbeef->plt_get_for_idx(deadbeef->streamer_get_current_playlist());
        if (pl) {
            idx = deadbeef->plt_get_item_idx(pl, playing_track, PL_MAIN) + offset;
        }
        deadbeef->pl_item_unref(playing_track);
    } else {
        pl = deadbeef->plt_get_curr();
        if (pl) {
            idx = deadbeef->plt_get_cursor(pl, PL_MAIN) + offset;
        }
    }

    if (pl) {
        DB_playItem_t *track = deadbeef->plt_get_item_for_idx(pl, idx, PL_MAIN);
        deadbeef->plt_unref(pl);
        if (track) {
            deadbeef->pl_item_unref(track);
            return TRUE;
        }
    }
    return FALSE;
}

static void onRootMethodCallHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
                                    const char *interfaceName, const char *methodName, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, void *userData) {
    debug("Method call on root interface. sender: %s, methodName %s\n", sender, methodName);
    DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;

    if (strcmp(methodName, "Quit") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
        deadbeef->sendmessage(DB_EV_TERMINATE, 0, 0, 0);
    } else if (strcmp(methodName, "Raise") == 0) {
        GDesktopAppInfo *dskapp = g_desktop_app_info_new ("deadbeef.desktop");
        if (dskapp) {
            g_app_info_launch ((GAppInfo *) dskapp, NULL, NULL, NULL);
            g_object_unref (dskapp);
        } else {
            deadbeef->sendmessage (DB_EV_ACTIVATED, 0, 0, 0);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        debug("Error! Unsupported method. %s.%s\n", interfaceName, methodName);
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
                                              "Method %s.%s not supported", interfaceName, methodName);
    }
}

static GVariant* onRootGetPropertyHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
                                          const char *interfaceName, const char *propertyName, GError **error,
                                          void *userData) {
    debug("Get property call on root interface. sender: %s, propertyName: %s\n", sender, propertyName);
    GVariant *result = NULL;

    if (strcmp(propertyName, "CanQuit") == 0) {
        result = g_variant_new_boolean(TRUE);
    } else if (strcmp(propertyName, "CanRaise") == 0) {
        result = g_variant_new_boolean(TRUE);
    } else if (strcmp(propertyName, "HasTrackList") == 0) {
        result = g_variant_new_boolean(FALSE);
    } else if (strcmp(propertyName, "Identity") == 0) {
        result = g_variant_new_string("DeaDBeeF");
    } else if (strcmp(propertyName, "DesktopEntry") == 0) {
        result = g_variant_new_string("deadbeef");
    } else if (strcmp(propertyName, "SupportedUriSchemes") == 0) {
        GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
        //TODO find uri schemata
        g_variant_builder_add(builder, "s", "file");
        g_variant_builder_add(builder, "s", "http");
        g_variant_builder_add(builder, "s", "cdda");
        result = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else if (strcmp(propertyName, "SupportedMimeTypes") == 0){
        GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));

        // MIME types from the .desktop file
        g_variant_builder_add(builder, "s", "audio/aac");
        g_variant_builder_add(builder, "s", "audio/aacp");
        g_variant_builder_add(builder, "s", "audio/x-it");
        g_variant_builder_add(builder, "s", "audio/x-flac");
        g_variant_builder_add(builder, "s", "audio/x-mod");
        g_variant_builder_add(builder, "s", "audio/mpeg");
        g_variant_builder_add(builder, "s", "audio/x-mpeg");
        g_variant_builder_add(builder, "s", "audio/x-mpegurl");
        g_variant_builder_add(builder, "s", "audio/mp3");
        g_variant_builder_add(builder, "s", "audio/prs.sid");
        g_variant_builder_add(builder, "s", "audio/x-scpls");
        g_variant_builder_add(builder, "s", "audio/x-s3m");
        g_variant_builder_add(builder, "s", "application/ogg");
        g_variant_builder_add(builder, "s", "application/x-ogg");
        g_variant_builder_add(builder, "s", "audio/x-vorbis+ogg");
        g_variant_builder_add(builder, "s", "audio/ogg");
        g_variant_builder_add(builder, "s", "audio/wma");
        g_variant_builder_add(builder, "s", "audio/x-xm");
        result = g_variant_builder_end(builder);
    }
    return result;
}

static const GDBusInterfaceVTable rootInterfaceVTable = {
    onRootMethodCallHandler,
    onRootGetPropertyHandler,
    NULL
};

static void onPlayerMethodCallHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
                                      const char *interfaceName, const char *methodName, GVariant *parameters,
                                      GDBusMethodInvocation *invocation, void *userData) {
    debug("Method call on Player interface. sender: %s, methodName %s\n", sender, methodName);
    debug("Parameter signature is %s\n", g_variant_get_type_string (parameters));
    struct MprisData *mprisData = (struct MprisData *)userData;
    DB_functions_t *deadbeef = mprisData->deadbeef;

    if (strcmp(methodName, "Next") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
        deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
    } else if (strcmp(methodName, "Previous") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);

        if (mprisData->previousAction == PREVIOUS_ACTION_PREV_OR_RESTART && mprisData->prevOrRestart && mprisData->prevOrRestart->callback2 != NULL) {
            mprisData->prevOrRestart->callback2(mprisData->prevOrRestart, DDB_ACTION_CTX_MAIN);
        } else {
            deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
        }
    } else if (strcmp(methodName, "Pause") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
        deadbeef->sendmessage(DB_EV_PAUSE, 0, 0, 0);
    } else if (strcmp(methodName, "PlayPause") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);

        if (deadbeef->get_output()->state() == OUTPUT_STATE_PLAYING) {
            deadbeef->sendmessage(DB_EV_PAUSE, 0, 0, 0);
        } else {
            deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
        }

    } else if (strcmp(methodName, "Stop") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
        deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
    } else if (strcmp(methodName, "Play") == 0) {
        if (deadbeef->get_output()->state() != OUTPUT_STATE_PLAYING)
            deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (strcmp(methodName, "Seek") == 0) {
        DB_playItem_t *track = deadbeef->streamer_get_playing_track();

        if (track != NULL) {
            float durationInMilliseconds = deadbeef->pl_get_item_duration(track) * 1000.0;
            float positionInMilliseconds= deadbeef->streamer_get_playpos() * 1000.0;
            int64_t offsetInMicroseconds;
            g_variant_get(parameters, "(x)", &offsetInMicroseconds);
            float offsetInMilliseconds = offsetInMicroseconds / 1000.0;

            float newPositionInMilliseconds = positionInMilliseconds + offsetInMilliseconds;
            if (newPositionInMilliseconds < 0) {
                newPositionInMilliseconds = 0;
            }
            if (newPositionInMilliseconds > durationInMilliseconds) {
                deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
            } else {
                deadbeef->sendmessage(DB_EV_SEEK, 0, newPositionInMilliseconds, 0);
            }

            deadbeef->pl_item_unref(track);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (strcmp(methodName, "SetPosition") == 0) {
        int64_t position = 0;
        const char *trackId = NULL;

        g_variant_get(parameters, "(&ox)", &trackId, &position);
        debug("Set %s position %d.\n", trackId, position);

        DB_playItem_t *track = deadbeef->streamer_get_playing_track();
        if (track != NULL) {
            ddb_playlist_t *pl = deadbeef->plt_get_for_idx(deadbeef->streamer_get_current_playlist());
            int playid = deadbeef->plt_get_item_idx(pl, track, PL_MAIN);
            int playlistIndex = deadbeef->streamer_get_current_playlist();
            char buf[200];
            sprintf(buf, "/DeaDBeeF/%d/%d", playlistIndex, playid);
            if (strcmp(buf, trackId) == 0) {
                deadbeef->sendmessage(DB_EV_SEEK, 0, position / 1000.0, 0);
            }
            deadbeef->pl_item_unref(track);
            deadbeef->plt_unref(pl);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (strcmp(methodName, "OpenUri") == 0) {
        const char *uri = NULL;

        g_variant_get(parameters, "(&s)", &uri);
        debug("OpenUri: %s\n", uri);
        //TODO it is probably better to create a new playlist for that. Maybe we can even play it without playlist
        ddb_playlist_t *pl = deadbeef->plt_get_curr();
        int ret = deadbeef->plt_add_file2(0, pl, uri, NULL, NULL);
        if (ret == 0) {
            ddb_playlist_t *pl = deadbeef->plt_get_curr();
            DB_playItem_t *track = deadbeef->plt_get_last(pl, PL_MAIN);
            int track_id = deadbeef->plt_get_item_idx(pl, track, PL_MAIN);
            deadbeef->plt_unref(pl);
            deadbeef->pl_item_unref(track);
            deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, track_id, 0);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        debug("Error! Unsupported method. %s.%s\n", interfaceName, methodName);
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
                                              "Method %s.%s not supported", interfaceName, methodName);
    }
}

static GVariant* onPlayerGetPropertyHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
                                            const char *interfaceName, const char *propertyName, GError **error,
                                            void *userData) {
    debug("Get property call on Player interface. sender: %s, propertyName: %s\n", sender, propertyName);
    DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;
    GVariant *result = NULL;

    if (strcmp(propertyName, "PlaybackStatus") == 0) {
        DB_output_t *output = deadbeef->get_output();

        if (output != NULL) {
            switch (output->state()) {
            case OUTPUT_STATE_PLAYING:
                result = g_variant_new_string("Playing");
                break;
            case OUTPUT_STATE_PAUSED:
                result = g_variant_new_string("Paused");
                break;
            case OUTPUT_STATE_STOPPED:
            default:
                result = g_variant_new_string("Stopped");
                break;
            }
        } else {
            result = g_variant_new_string("Stopped");
        }
    } else if (strcmp(propertyName, "LoopStatus") == 0) {
        int loop = deadbeef->conf_get_int("playback.loop", PLAYBACK_MODE_LOOP_ALL);

        switch (loop) {
        case PLAYBACK_MODE_NOLOOP:
            result = g_variant_new_string("None");
            break;
        case PLAYBACK_MODE_LOOP_ALL:
            result = g_variant_new_string("Playlist");
            break;
        case PLAYBACK_MODE_LOOP_SINGLE:
            result = g_variant_new_string("Track");
            break;
        default:
            result = g_variant_new_string("None");
            break;
        }
    } else if (strcmp(propertyName, "Rate") == 0
            || strcmp(propertyName, "MaximumRate") == 0
            || strcmp(propertyName, "MinimumRate") == 0) {
        result = g_variant_new("d", 1.0);
    } else if (strcmp(propertyName, "Shuffle") == 0) {
        if (deadbeef->conf_get_int("playback.order", PLAYBACK_ORDER_LINEAR) == PLAYBACK_ORDER_LINEAR) {
            result = g_variant_new_boolean(FALSE);
        } else {
            result = g_variant_new_boolean(TRUE);
        }
    } else if (strcmp(propertyName, "Metadata") == 0) {
        result = getMetadataForTrack(CURRENT_TRACK, userData);
    } else if (strcmp(propertyName, "Volume") == 0) {
        float volume = (deadbeef->volume_get_db() * 0.02) + 1;

        result = g_variant_new("d", volume);
    } else if (strcmp(propertyName, "Position") == 0) {
        DB_playItem_t *track = deadbeef->streamer_get_playing_track();

        if (track == NULL) {
            result = g_variant_new("x", 0);
        } else {
            float positionInSeconds = deadbeef->streamer_get_playpos();

            result = g_variant_new("x", (uint64_t)(positionInSeconds * 1000000.0));
            deadbeef->pl_item_unref(track);
        }
    } else if (strcmp(propertyName, "CanGoNext") == 0) {
        result = g_variant_new_boolean(deadbeef_hasselectedorplayingtrack(userData, 1));
    } else if (strcmp(propertyName, "CanGoPrevious") == 0) {
        result = g_variant_new_boolean(deadbeef_hasselectedorplayingtrack(userData, -1));
    } else if (strcmp(propertyName, "CanPlay") == 0) {
        result = g_variant_new_boolean(deadbeef_hasselectedorplayingtrack(userData, 0));
    } else if (strcmp(propertyName, "CanPause") == 0) {
        result = g_variant_new_boolean(TRUE);
    } else if (strcmp(propertyName, "CanSeek") == 0) {
        result = g_variant_new_boolean(deadbeef_can_seek(deadbeef));
    } else if (strcmp(propertyName, "CanControl") == 0) {
        result = g_variant_new_boolean(TRUE);
    }
    return result;
}

static int onPlayerSetPropertyHandler(GDBusConnection *connection, const char *sender, const char *objectPath,
                                      const char *interfaceName, const char *propertyName, GVariant *value,
                                      GError **error, gpointer userData) {
    debug("Set property call on Player interface. sender: %s, propertyName: %s\n", sender, propertyName);
    DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;

    if (strcmp(propertyName, "LoopStatus") == 0) {
        if (deadbeef->conf_get_int("mpris.disable_shuffle_repeat", 0)) {
            return TRUE;
        }
        char *status;
        g_variant_get(value, "s", &status);
        if (status != NULL) {
            debug("status is %s\n", status);
            if (strcmp(status, "None") == 0) {
                deadbeef->conf_set_int("playback.loop", PLAYBACK_MODE_NOLOOP);
            } else if (strcmp(status, "Playlist") == 0) {
                deadbeef->conf_set_int("playback.loop", PLAYBACK_MODE_LOOP_ALL);
            } else if (strcmp(status, "Track") == 0) {
                deadbeef->conf_set_int("playback.loop", PLAYBACK_MODE_LOOP_SINGLE);
            }

            deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
        }
    } else if (strcmp(propertyName, "Rate") == 0) {
        debug("Setting the rate is not supported\n");
    } else if (strcmp(propertyName, "Shuffle") == 0) {
        if (deadbeef->conf_get_int("mpris.disable_shuffle_repeat", 0)) {
            return TRUE;
        }
        if (g_variant_get_boolean(value)) {
            deadbeef->conf_set_int("playback.order", PLAYBACK_ORDER_SHUFFLE_TRACKS);
        } else {
            deadbeef->conf_set_int("playback.order", PLAYBACK_ORDER_LINEAR);
        }
        deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
    } else if (strcmp(propertyName, "Volume") == 0) {
        double volume = g_variant_get_double(value);
        if (volume > 1.0) {
            volume = 1.0;
        } else if (volume < 0.0) {
            volume = 0.0;
        }
        float newVolume = ((float)volume * 50) - 50;

        deadbeef->volume_set_db(newVolume);
    }

    return TRUE;
}

static const GDBusInterfaceVTable playerInterfaceVTable = {
    onPlayerMethodCallHandler,
    onPlayerGetPropertyHandler,
    onPlayerSetPropertyHandler
};

//***********
//* SIGNALS *
//***********
void emitVolumeChanged(float volume) {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    volume = (volume * 0.02) + 1;
    debug("Volume property changed: %f\n", volume);

    g_variant_builder_add(builder, "{sv}", "Volume", g_variant_new("d", volume));
    GVariant *signal[] = {
        g_variant_new_string(PLAYER_INTERFACE),
        g_variant_builder_end(builder),
        g_variant_new_strv(NULL, 0)
    };

    g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
                                  g_variant_new_tuple(signal, 3), NULL);

    g_variant_builder_unref(builder);
}

void emitSeeked(float position) {
    int64_t positionInMicroseconds = position * 1000000.0;
    debug("Seeked to %" PRId64 "\n", positionInMicroseconds);

    g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PLAYER_INTERFACE, "Seeked",
                                  g_variant_new("(x)", positionInMicroseconds), NULL);
}

void emitMetadataChanged(int trackId, struct MprisData *userData) {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

    g_variant_builder_add(builder, "{sv}", "Metadata", getMetadataForTrack(trackId, userData));

    GVariant *signal[] = {
            g_variant_new_string(PLAYER_INTERFACE),
            g_variant_builder_end(builder),
            g_variant_new_strv(NULL, 0)
    };

    g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
                                  g_variant_new_tuple(signal, 3), NULL);

    g_variant_builder_unref(builder);
}

void emitCanGoChanged(struct MprisData *userData) {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

    g_variant_builder_add(builder, "{sv}", "CanPlay", g_variant_new_boolean(deadbeef_hasselectedorplayingtrack(userData, 0)));
    g_variant_builder_add(builder, "{sv}", "CanGoNext", g_variant_new_boolean(deadbeef_hasselectedorplayingtrack(userData, 1)));
    g_variant_builder_add(builder, "{sv}", "CanGoPrevious", g_variant_new_boolean(deadbeef_hasselectedorplayingtrack(userData, -1)));

    GVariant *signal[] = {
            g_variant_new_string(PLAYER_INTERFACE),
            g_variant_builder_end(builder),
            g_variant_new_strv(NULL, 0)
    };

    g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
                                  g_variant_new_tuple(signal, 3), NULL);

    g_variant_builder_unref(builder);
}

void emitPlaybackStatusChanged(int status, struct MprisData *userData) {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    DB_functions_t *deadbeef = ((struct MprisData *)userData)->deadbeef;

    switch (status) {
        case OUTPUT_STATE_PLAYING:
            g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Playing"));
            break;
        case OUTPUT_STATE_PAUSED:
            g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Paused"));
            break;
        case OUTPUT_STATE_STOPPED:
        default:
            g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Stopped"));
            break;
    }

    g_variant_builder_add(builder, "{sv}", "CanSeek", g_variant_new_boolean(deadbeef_can_seek(deadbeef)));


    GVariant *signal[] = {
        g_variant_new_string(PLAYER_INTERFACE),
        g_variant_builder_end(builder),
        g_variant_new_strv(NULL, 0)
    };

    g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
                                  g_variant_new_tuple(signal, 3), NULL);

    g_variant_builder_unref(builder);
}

void emitLoopStatusChanged(int status) {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

    switch (status) {
    case PLAYBACK_MODE_NOLOOP:
        g_variant_builder_add(builder, "{sv}", "LoopStatus", g_variant_new_string("None"));
        break;
    case PLAYBACK_MODE_LOOP_ALL:
        g_variant_builder_add(builder, "{sv}", "LoopStatus", g_variant_new_string("Playlist"));
        break;
    case PLAYBACK_MODE_LOOP_SINGLE:
        g_variant_builder_add(builder, "{sv}", "LoopStatus", g_variant_new_string("Track"));
        break;
    default:
        g_variant_builder_add(builder, "{sv}", "LoopStatus", g_variant_new_string("None"));
        break;
    }
    GVariant *signal[] = {
        g_variant_new_string(PLAYER_INTERFACE),
        g_variant_builder_end(builder),
        g_variant_new_strv(NULL, 0)
    };

    g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
                                  g_variant_new_tuple(signal, 3), NULL);

    g_variant_builder_unref(builder);
}

void emitShuffleStatusChanged(int status) {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

    g_variant_builder_add(builder, "{sv}", "Shuffle", g_variant_new_boolean(status != PLAYBACK_ORDER_LINEAR));
    GVariant *signal[] = {
        g_variant_new_string(PLAYER_INTERFACE),
        g_variant_builder_end(builder),
        g_variant_new_strv(NULL, 0)
    };

    g_dbus_connection_emit_signal(globalConnection, NULL, OBJECT_NAME, PROPERTIES_INTERFACE, "PropertiesChanged",
                                  g_variant_new_tuple(signal, 3), NULL);

    g_variant_builder_unref(builder);
}

static void onBusAcquiredHandler(GDBusConnection *connection, const char *name, void *userData) {
    globalConnection = connection;
    debug("Bus accquired\n");
}

static void onNameAcquiredHandler(GDBusConnection *connection, const char *name, void *userData) {
    debug("name accquired: %s\n", name);
    GDBusInterfaceInfo **interfaces = ((struct MprisData*)userData)->gdbusNodeInfo->interfaces;

    debug("Registering" OBJECT_NAME "object...\n");
    g_dbus_connection_register_object(connection, OBJECT_NAME, interfaces[0], &rootInterfaceVTable, userData, NULL,
                                      NULL);

    g_dbus_connection_register_object(connection, OBJECT_NAME, interfaces[1], &playerInterfaceVTable, userData, NULL,
                                      NULL);
}

static void onConnotConnectToBus(GDBusConnection *connection, const char *name, void *user_data){
    error("cannot connect to bus\n");
}

void* startServer(void *data) {
    int ownerId;
    GMainContext *context = g_main_context_new();
    struct MprisData *mprisData = data;


    g_main_context_push_thread_default(context);

    mprisData->gdbusNodeInfo = g_dbus_node_info_new_for_xml(xmlForNode, NULL);

    ownerId = g_bus_own_name(G_BUS_TYPE_SESSION, BUS_NAME, G_BUS_NAME_OWNER_FLAGS_REPLACE,
                             onBusAcquiredHandler, onNameAcquiredHandler, onConnotConnectToBus,
                             (void *)mprisData, NULL);

    loop = g_main_loop_new(context, FALSE);
    g_main_loop_run(loop);

    g_bus_unown_name(ownerId);
    g_dbus_node_info_unref(mprisData->gdbusNodeInfo);
    g_main_loop_unref(loop);

    freeTfBytecode(mprisData->deadbeef);

    return 0;
}

void stopServer() {
    debug("Stopping...\n");
    g_main_loop_quit(loop);
}
