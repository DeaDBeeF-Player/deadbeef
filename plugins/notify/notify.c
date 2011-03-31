/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../deadbeef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gettext.h"
#include "../artwork/artwork.h"

#define E_NOTIFICATION_BUS_NAME "org.freedesktop.Notifications"
#define E_NOTIFICATION_INTERFACE "org.freedesktop.Notifications"
#define E_NOTIFICATION_PATH "/org/freedesktop/Notifications"

DB_functions_t *deadbeef;
DB_misc_t plugin;
DB_artwork_plugin_t *artwork_plugin;

static dbus_uint32_t replaces_id = 0;

#define NOTIFY_DEFAULT_TITLE "%t"
#define NOTIFY_DEFAULT_CONTENT "%a - %b"

static void
notify_thread (void *ctx) {

    DBusMessage *msg = (DBusMessage*) ctx;
    DBusMessage *reply = NULL;

    DBusError error;
    dbus_error_init (&error);
    DBusConnection *conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if(dbus_error_is_set (&error)) {
        fprintf(stderr, "connection failed: %s",error.message);
        dbus_error_free(&error);
        dbus_message_unref (msg);
        deadbeef->thread_exit(NULL);
    }

    reply = dbus_connection_send_with_reply_and_block (conn, msg, -1, &error);
    if (dbus_error_is_set (&error)) {
        fprintf(stderr, "send_with_reply_and_block error: (%s)\n", error.message); 
        dbus_error_free(&error);
        dbus_message_unref (msg);
        deadbeef->thread_exit(NULL);
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
    deadbeef->thread_exit(NULL);

}

#if 0
static void
notify_marshal_dict_byte(DBusMessageIter *iter, const char *key, char value)
{
  DBusMessageIter entry, variant;

  if (!key || !value) return;

  dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "y", &variant);
  dbus_message_iter_append_basic(&variant, DBUS_TYPE_BYTE, &value);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(iter, &entry);
}

static void
notify_marshal_dict_string(DBusMessageIter *iter, const char *key, const char *value)
{
  DBusMessageIter entry, variant;

  dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, "sv", &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
  dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(iter, &entry);
}
#endif

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
        else if (*src == '\'') {
            if (end - dst < 6) {
                break;
            }
            strcpy (dst, "&apos;");
            dst += 6;
            src++;
        }
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
//    show_notification (track);
}

static void show_notification (DB_playItem_t *track) {
    char title[1024];
    char content[1024];
    deadbeef->conf_lock ();
    deadbeef->pl_format_title (track, -1, title, sizeof (title), -1, deadbeef->conf_get_str_fast ("notify.format", NOTIFY_DEFAULT_TITLE));
    deadbeef->pl_format_title (track, -1, content, sizeof (content), -1, deadbeef->conf_get_str_fast ("notify.format_content", NOTIFY_DEFAULT_CONTENT));
    deadbeef->conf_unlock ();

    // escape &
//    char esc_title[1024];
    char esc_content[1024];
//    esc_xml (title, esc_title, sizeof (esc_title));
    esc_xml (content, esc_content, sizeof (esc_content));
    DBusMessage *msg = dbus_message_new_method_call (E_NOTIFICATION_BUS_NAME, E_NOTIFICATION_PATH, E_NOTIFICATION_INTERFACE, "Notify");

    const char *v_appname = "DeaDBeeF";
    dbus_uint32_t v_id = 0;
    char *v_iconname = NULL;
    if (deadbeef->conf_get_int("notify.albumart", 0) && artwork_plugin) {
        const char *album = deadbeef->pl_find_meta (track, "album");
        const char *artist = deadbeef->pl_find_meta (track, "artist");
        v_iconname = artwork_plugin->get_album_art (deadbeef->pl_find_meta (track, ":URI"), artist, album, deadbeef->conf_get_int ("notify.albumart_size", 64), cover_avail_callback, NULL);
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

    intptr_t tid = 0;
    if ((tid=deadbeef->thread_start(notify_thread, msg)) != 0) {
        dbus_message_ref (msg);
        deadbeef->thread_detach (tid);  
    }
    dbus_message_unref (msg);
    if (v_iconname) {
        free (v_iconname);
    }
}

static int
on_songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    if (ev->to && deadbeef->conf_get_int ("notify.enable", 0)) {
        DB_playItem_t *track = ev->to;
        if (track) {
            show_notification (track);
        }
    }
    return 0;
}

int
notify_start (void) {
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (on_songchanged), 0);
    return 0;
}

int
notify_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (on_songchanged), 0);
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
    "property \"Notification title format\" entry notify.format \"" NOTIFY_DEFAULT_TITLE "\";\n"
    "property \"Notification content format\" entry notify.format_content \"" NOTIFY_DEFAULT_CONTENT "\";\n"
    "property \"Show album art\" checkbox notify.albumart 1;\n"
    "property \"Album art size (px)\" entry notify.albumart_size 64;\n"
;

DB_misc_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.id = "notify",
    .plugin.name = "OSD Notify",
    .plugin.descr = "Displays notifications when new track starts, using system notification daemon",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
};

DB_plugin_t *
notify_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return &plugin.plugin;
}
