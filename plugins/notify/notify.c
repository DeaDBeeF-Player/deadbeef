/*
    OSD Notification plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Alexey Yakovenko and contributors

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <dbus/dbus.h>
#include <dispatch/dispatch.h>
#include "../../deadbeef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gettext.h"
#include "../artwork-legacy/artwork.h"

#define E_NOTIFICATION_BUS_NAME "org.freedesktop.Notifications"
#define E_NOTIFICATION_INTERFACE "org.freedesktop.Notifications"
#define E_NOTIFICATION_PATH "/org/freedesktop/Notifications"

static DB_functions_t *deadbeef;
static DB_misc_t plugin;
static DB_artwork_plugin_t *artwork_plugin;
static dispatch_queue_t queue;
static DB_playItem_t *last_track = NULL;
static time_t request_timer = 0;
static int terminate;

static dbus_uint32_t replaces_id = 0;

#define NOTIFY_DEFAULT_TITLE "%title%"
#define NOTIFY_DEFAULT_CONTENT "%artist% - %album%"

static char *tf_title;
static char *tf_content;

static void
show_notification (DB_playItem_t *track);

static void
notify_send (DBusMessage *msg) {
    DBusMessage *reply = NULL;

    DBusError error;
    dbus_error_init (&error);
    DBusConnection *conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if(dbus_error_is_set (&error)) {
        fprintf(stderr, "connection failed: %s",error.message);
        dbus_error_free(&error);
        dbus_message_unref (msg);
        return;
    }

    reply = dbus_connection_send_with_reply_and_block (conn, msg, -1, &error);
    if (dbus_error_is_set (&error)) {
        fprintf(stderr, "send_with_reply_and_block error: (%s)\n", error.message); 
        dbus_error_free(&error);
        dbus_message_unref (msg);
        return;
    }

    if (reply != NULL) {
        // Process the reply message
        DBusMessageIter args;

        dbus_uint32_t id = 0;
        if (dbus_message_iter_init(reply, &args)) {
            if (DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(&args)) {
                dbus_message_iter_get_basic(&args, &id);
                if (id != replaces_id) {
                    replaces_id = id;
                }
                dbus_message_unref (reply);
            } else {
                fprintf(stderr, "Argument is not uint32\n"); 
            }
        } else {
            fprintf(stderr, "Reply has no arguments\n"); 
        }
    } 

    dbus_message_unref (msg);
    dbus_connection_unref (conn);
}

static void
esc_xml (const char *cmd, char *esc, int size) {
    const char *src = cmd;
    char *dst = esc;
    char *end = dst + size - 1;
    while (*src && dst < end) {
        if (*src == '&') {
            if (end - dst < 5) {
                break;
            }
            strcpy (dst, "&amp;");
            dst += 5;
            src++;
        }
        else if (*src == '<') {
            if (end - dst < 4) {
                break;
            }
            strcpy (dst, "&lt;");
            dst += 4;
            src++;
        }
        else if (*src == '>') {
            if (end - dst < 4) {
                break;
            }
            strcpy (dst, "&gt;");
            dst += 4;
            src++;
        }
#if 0 // &apos; is not supported by KDE
        else if (*src == '\'') {
            if (end - dst < 6) {
                break;
            }
            strcpy (dst, "&apos;");
            dst += 6;
            src++;
        }
#endif
        else if (*src == '"') {
            if (end - dst < 6) {
                break;
            }
            strcpy (dst, "&quot;");
            dst += 6;
            src++;
        }
        else if (*src == '\\' && *(src+1) == 'n') {
            strcpy (dst, "\n");
            dst++;
            src+=2;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

static void
cover_avail_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    if (!fname) {
        // Give up
        return;
    }
    if (time (NULL) - request_timer >= 4) {
        return;
    }

    deadbeef->pl_lock ();
    ddb_playItem_t *track = last_track;
    last_track = NULL;
    deadbeef->pl_unlock ();

    if (track) {
        dispatch_async (queue, ^{
            show_notification (track);
            deadbeef->pl_item_unref (track);
        });
    }
}

static void
show_notification (DB_playItem_t *track) {
    char title[1024];
    char content[1024];

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = track,
        .flags = DDB_TF_CONTEXT_MULTILINE | DDB_TF_CONTEXT_NO_DYNAMIC,
    };

    deadbeef->tf_eval (&ctx, tf_title, title, sizeof (title));
    deadbeef->tf_eval (&ctx, tf_content, content, sizeof (content));

    char esc_content[1024];
    esc_xml (content, esc_content, sizeof (esc_content));
    DBusMessage *msg = dbus_message_new_method_call (E_NOTIFICATION_BUS_NAME, E_NOTIFICATION_PATH, E_NOTIFICATION_INTERFACE, "Notify");

    deadbeef->pl_lock ();
    if (last_track) {
        deadbeef->pl_item_unref (last_track);
        last_track = 0;
    }
    last_track = track;
    deadbeef->pl_item_ref (last_track);
    request_timer = time (NULL);
    deadbeef->pl_unlock ();

    const char *v_appname = "DeaDBeeF";
    dbus_uint32_t v_id = 0;
    char *v_iconname = NULL;
    if (deadbeef->conf_get_int("notify.albumart", 0) && artwork_plugin) {
        deadbeef->pl_lock ();
        const char *album = deadbeef->pl_find_meta (track, "album");
        const char *artist = deadbeef->pl_find_meta (track, "artist");
        const char *fname = deadbeef->pl_find_meta (track, ":URI");
        if (!album || !*album) {
            album = deadbeef->pl_find_meta (track, "title");
        }
        v_iconname = artwork_plugin->get_album_art (fname, artist, album, deadbeef->conf_get_int ("notify.albumart_size", 64), cover_avail_callback, NULL);
        deadbeef->pl_unlock ();
    }
    if (!v_iconname) {
        v_iconname = strdup ("deadbeef");
    }
    const char *v_summary = title;
    const char *v_body = esc_content;
    dbus_int32_t v_timeout = -1;

    dbus_message_append_args (msg
            , DBUS_TYPE_STRING, &v_appname
            , DBUS_TYPE_UINT32, &replaces_id
            , DBUS_TYPE_STRING, &v_iconname
            , DBUS_TYPE_STRING, &v_summary
            , DBUS_TYPE_STRING, &v_body
            , DBUS_TYPE_INVALID
            );

    DBusMessageIter iter, sub;
    // actions
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &sub);
    dbus_message_iter_close_container(&iter, &sub);
    // hints
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &sub);
    dbus_message_iter_close_container(&iter, &sub);

    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &v_timeout);
    if (v_iconname) {
        free (v_iconname);
    }

    notify_send (msg);
}

static int
on_songstarted (ddb_event_track_t *ev) {
    if (ev->track && deadbeef->conf_get_int ("notify.enable", 0)) {
        DB_playItem_t *track = ev->track;
        if (track) {
            deadbeef->pl_item_ref (track);
            if (terminate) {
                deadbeef->pl_item_unref (track);
                return 0;
            }
            dispatch_async (queue, ^{
                show_notification (track);
                deadbeef->pl_item_unref (track);
            });
        }
    }
    return 0;
}

static void
init_tf (void) {
    if (tf_title) {
        deadbeef->tf_free (tf_title);
    }
    if (tf_content) {
        deadbeef->tf_free (tf_content);
    }
    char format[200];
    deadbeef->conf_get_str ("notify.format_title_tf", NOTIFY_DEFAULT_TITLE, format, sizeof (format));
    tf_title = deadbeef->tf_compile (format);
    deadbeef->conf_get_str ("notify.format_content_tf", NOTIFY_DEFAULT_CONTENT, format, sizeof (format));
    tf_content = deadbeef->tf_compile (format);
}

static int
notify_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SONGSTARTED:
        on_songstarted ((ddb_event_track_t *)ctx);
        break;
    case DB_EV_CONFIGCHANGED:
        init_tf ();
        break;
    }
    return 0;
}

static void
import_legacy_tf (const char *key_from, const char *key_to) {
    deadbeef->conf_lock ();
    if (!deadbeef->conf_get_str_fast (key_to, NULL)
            && deadbeef->conf_get_str_fast (key_from, NULL)) {
        char old[200], new[200];
        deadbeef->conf_get_str (key_from, "", old, sizeof (old));
        deadbeef->tf_import_legacy (old, new, sizeof (new));
        deadbeef->conf_set_str (key_to, new);
    }
    deadbeef->conf_unlock ();
}

int
notify_start (void) {
    queue = dispatch_queue_create("OSDNotifyQueue", NULL);
    import_legacy_tf ("notify.format", "notify.format_title_tf");
    import_legacy_tf ("notify.format_content", "notify.format_content_tf");
    return 0;
}

int
notify_stop (void) {
    terminate = 1;
    dispatch_release (queue);
    queue = NULL;

    deadbeef->pl_lock ();
    if (last_track) {
        deadbeef->pl_item_unref (last_track);
        last_track = NULL;
    }
    deadbeef->pl_unlock ();

    if (tf_title) {
        deadbeef->tf_free (tf_title);
        tf_title = NULL;
    }
    if (tf_content) {
        deadbeef->tf_free (tf_content);
        tf_content = NULL;
    }
    return 0;
}

static int
notify_connect (void) {
    artwork_plugin = (DB_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork");
    return 0;
}

static int
notify_disconnect (void) {
    artwork_plugin = NULL;
    return 0;
}

static const char settings_dlg[] =
    "property \"Enable\" checkbox notify.enable 0;\n"
    "property \"Notification title format\" entry notify.format_title_tf \"" NOTIFY_DEFAULT_TITLE "\";\n"
    "property \"Notification content format\" entry notify.format_content_tf \"" NOTIFY_DEFAULT_CONTENT "\";\n"
    "property \"Show album art\" checkbox notify.albumart 1;\n"
    "property \"Album art size (px)\" entry notify.albumart_size 64;\n"
;

static DB_misc_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.id = "notify",
    .plugin.name = "OSD Notify",
    .plugin.descr = "Displays notifications when new track starts.\nRequires dbus and notification daemon to be running.\nNotification daemon should be provided by your desktop environment.\n",
    .plugin.copyright = 
        "OSD Notification plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Alexey Yakovenko and contributors\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sourceforge.net",
    .plugin.start = notify_start,
    .plugin.stop = notify_stop,
    .plugin.connect = notify_connect,
    .plugin.disconnect = notify_disconnect,
    .plugin.configdialog = settings_dlg,
    .plugin.message = notify_message,
};

DB_plugin_t *
notify_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return &plugin.plugin;
}
