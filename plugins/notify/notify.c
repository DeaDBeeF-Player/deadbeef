/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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

#define E_NOTIFICATION_BUS_NAME "org.freedesktop.Notifications"
#define E_NOTIFICATION_INTERFACE "org.freedesktop.Notifications"
#define E_NOTIFICATION_PATH "/org/freedesktop/Notifications"

DB_functions_t *deadbeef;
DB_misc_t plugin;

#define NOTIFY_DEFAULT_FORMAT "%a - %t"

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

static int
on_songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    if (ev->to && deadbeef->conf_get_int ("notify.enable", 0)) {
        DB_playItem_t *track = ev->to;
        if (track) {
            char cmd[1024];
            deadbeef->pl_format_title (track, -1, cmd, sizeof (cmd), -1, deadbeef->conf_get_str ("notify.format", NOTIFY_DEFAULT_FORMAT));

            // escape &
            char esc[1024];

            char *src = cmd;
            char *dst = esc;
            char *end = dst + sizeof (esc) - 1;
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
                else {
                    *dst++ = *src++;
                }
            }
            *dst = 0;

            DBusError error;
            dbus_error_init (&error);
            DBusConnection *conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
            if(conn == NULL) {
                printf("connection failed: %s",error.message);
                exit(1);
            }
            DBusMessage *msg = dbus_message_new_method_call (E_NOTIFICATION_BUS_NAME, E_NOTIFICATION_PATH, E_NOTIFICATION_INTERFACE, "Notify");

            const char *v_appname = "DeaDBeeF";
            dbus_uint32_t v_id = 0;
            const char *v_iconname = "deadbeef";
            const char *v_summary = _("DeaDBeeF now playing");
            const char *v_body = esc;
            dbus_int32_t v_timeout = -1;

            dbus_message_append_args (msg
                    , DBUS_TYPE_STRING, &v_appname
                    , DBUS_TYPE_UINT32, &v_id
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

            int serial;
            dbus_bool_t retval = dbus_connection_send(conn,msg,&serial);
            dbus_connection_flush (conn);
            dbus_message_unref (msg);
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

static const char settings_dlg[] =
    "property \"Enable\" checkbox notify.enable 0;\n"
    "property \"Notification format\" entry notify.format \"" NOTIFY_DEFAULT_FORMAT "\";\n"
;

DB_misc_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.id = "notify",
    .plugin.name = "OSD Notify",
    .plugin.descr = "Displays notifications on track changes using system notification daemon",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sourceforge.net",
    .plugin.start = notify_start,
    .plugin.stop = notify_stop,
    .plugin.configdialog = settings_dlg,
};

DB_plugin_t *
notify_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return &plugin.plugin;
}
