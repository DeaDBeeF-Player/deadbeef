/*
    OSD Notification plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko and contributors

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
#include <Block.h>
#include <dbus/dbus.h>
#include <dispatch/dispatch.h>
#include <deadbeef/deadbeef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "../../gettext.h"
#include "../artwork/artwork.h"

#define min(x, y) ((x) < (y) ? (x) : (y))
#define MAX_ALBUM_ART_FILE_SIZE (40 * 1024 * 1024)
#define E_NOTIFICATION_BUS_NAME "org.freedesktop.Notifications"
#define E_NOTIFICATION_INTERFACE "org.freedesktop.Notifications"
#define E_NOTIFICATION_PATH "/org/freedesktop/Notifications"

static DB_functions_t *deadbeef;
static DB_misc_t plugin;
static ddb_artwork_plugin_t *artwork_plugin;
static dispatch_queue_t queue;
static DB_playItem_t *last_track = NULL;
static dbus_uint32_t _replaces_id;
static time_t request_timer = 0;
static int terminate;

#define NOTIFY_DEFAULT_TITLE "%title%"
#define NOTIFY_DEFAULT_CONTENT "%artist%$if($and(%artist%,%album%), - ,)%album%"

static char *tf_title;
static char *tf_content;

// @return new replaces_id
static dbus_uint32_t
show_notification (DB_playItem_t *track, char *image_filename, dbus_uint32_t replaces_id, int force);

static dbus_uint32_t
notify_send (DBusMessage *msg, dbus_uint32_t replaces_id) {
    DBusMessage *reply = NULL;

    DBusError error;
    dbus_error_init (&error);
    DBusConnection *conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set (&error)) {
        fprintf (stderr, "connection failed: %s", error.message);
        dbus_error_free (&error);
        dbus_message_unref (msg);
        return 0;
    }

    reply = dbus_connection_send_with_reply_and_block (conn, msg, -1, &error);
    if (dbus_error_is_set (&error)) {
        fprintf (stderr, "send_with_reply_and_block error: (%s)\n", error.message);
        dbus_error_free (&error);
        dbus_message_unref (msg);
        return 0;
    }

    dbus_uint32_t id = 0;
    if (reply != NULL) {
        // Process the reply message
        DBusMessageIter args;

        if (dbus_message_iter_init (reply, &args)) {
            if (DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type (&args)) {
                dbus_message_iter_get_basic (&args, &id);
                dbus_message_unref (reply);
            }
            else {
                fprintf (stderr, "Argument is not uint32\n");
            }
        }
        else {
            fprintf (stderr, "Reply has no arguments\n");
        }
    }

    dbus_message_unref (msg);
    dbus_connection_unref (conn);
    return id;
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
        else if (*src == '\\' && *(src + 1) == 'n') {
            strcpy (dst, "\n");
            dst++;
            src += 2;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

static void
_cover_loaded_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    void (^completion_block) (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) =
        (void (^) (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover))query->user_data;
    completion_block (error, query, cover);
    Block_release (completion_block);
}

static char *
_buffer_from_file (const char *fname, long *psize) {
    char *buffer = NULL;
    FILE *fp = fopen (fname, "rb");
    if (fp == NULL) {
        return NULL;
    }
    if (fseek (fp, 0, SEEK_END) < 0) {
        goto error;
    }
    long size = ftell (fp);
    if (size <= 0 || size > MAX_ALBUM_ART_FILE_SIZE) {
        goto error; // we don't really want to load ultra-high-res images
    }
    rewind (fp);

    buffer = malloc (size);
    if (buffer == NULL) {
        goto error;
    }

    if (fread (buffer, 1, size, fp) != size) {
        goto error;
    }

    fclose (fp);

    *psize = size;
    return buffer;

error:
    if (fp != NULL) {
        fclose (fp);
    }
    free (buffer);
    return NULL;
}

static GdkPixbuf *
_create_scaled_image (GdkPixbuf *image, int width, int height) {
    int originalWidth = gdk_pixbuf_get_width (image);
    int originalHeight = gdk_pixbuf_get_height (image);

    if (originalWidth <= width && originalHeight <= height) {
        g_object_ref (image);
        return image;
    }

    gboolean has_alpha = gdk_pixbuf_get_has_alpha (image);
    int bits_per_sample = gdk_pixbuf_get_bits_per_sample (image);

    GdkPixbuf *scaled_image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, bits_per_sample, width, height);

    double scale_x = (double)width / (double)originalWidth;
    double scale_y = (double)height / (double)originalHeight;

    gdk_pixbuf_scale (image, scaled_image, 0, 0, width, height, 0, 0, scale_x, scale_y, GDK_INTERP_BILINEAR);

    return scaled_image;
}

static void
_desired_size_for_image_size (
    int image_width,
    int image_height,
    int avail_width,
    int avail_height,
    int *result_width,
    int *result_height) {
    double scale = min ((double)avail_width / (double)image_width, (double)avail_height / (double)image_height);

    *result_width = image_width * scale;
    *result_height = image_height * scale;
}

static GdkPixbuf *
_load_image (const char *image_filename) {
    GdkPixbuf *img = NULL;

    long size = 0;
    char *buf = _buffer_from_file (image_filename, &size);
    if (buf != NULL) {
        GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
        gdk_pixbuf_loader_write (loader, (const guchar *)buf, size, NULL);
        gdk_pixbuf_loader_close (loader, NULL);
        img = gdk_pixbuf_loader_get_pixbuf (loader);
        free (buf);
    }

    if (img == NULL) {
        return NULL;
    }

    int max_image_size = deadbeef->conf_get_int ("notify.albumart_size", 64);
    if (max_image_size < 32) {
        max_image_size = 32;
    }
    else if (max_image_size > 100) {
        max_image_size = 100;
    }

    // downscale
    int orig_width = gdk_pixbuf_get_width (img);
    int orig_height = gdk_pixbuf_get_height (img);

    if (orig_width > max_image_size || orig_height > max_image_size) {
        int new_width = max_image_size;
        int new_height = max_image_size;

        int result_width;
        int result_height;

        _desired_size_for_image_size (orig_width, orig_height, new_width, new_height, &result_width, &result_height);

        GdkPixbuf *scaled_img = _create_scaled_image (img, result_width, result_height);
        g_object_unref (img);
        img = scaled_img;
    }

    return img;
}

static dbus_uint32_t
show_notification (DB_playItem_t *track, char *image_filename, dbus_uint32_t replaces_id, int force) {
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
    DBusMessage *msg =
        dbus_message_new_method_call (E_NOTIFICATION_BUS_NAME, E_NOTIFICATION_PATH, E_NOTIFICATION_INTERFACE, "Notify");

    if (replaces_id == 0) {
        time_t new_time = time (NULL);
        if (last_track == track && !force) {
            if (new_time - request_timer < 1) {
                return replaces_id; // same track, less than 1 sec after previous notification -- pass
            }
        }
        else {
            if (last_track) {
                deadbeef->pl_item_unref (last_track);
                last_track = 0;
            }
            last_track = track;
            deadbeef->pl_item_ref (last_track);
        }
        request_timer = new_time;
    }

    const char *v_appname = "DeaDBeeF";

    bool should_wait_for_cover = !image_filename && deadbeef->conf_get_int ("notify.albumart", 0) && artwork_plugin;
    bool should_apply_kde_fix = deadbeef->conf_get_int ("notify.fix_kde_5_23_5", 0) ? true : false;

    // KDE won't re-display notification via reuse,
    // so don't show it now and wait for cover callback.
    if (!(should_wait_for_cover && should_apply_kde_fix)) {
        char *v_iconname = ""; // image_filename ?: "deadbeef";
        const char *v_summary = title;
        const char *v_body = esc_content;
        dbus_int32_t v_timeout = -1;

        GdkPixbuf *img = _load_image (image_filename);
        if (!img) {
            v_iconname = "deadbeef";
        }

        DBusMessageIter iter, sub;

        dbus_message_append_args (
            msg,
            DBUS_TYPE_STRING,
            &v_appname,
            DBUS_TYPE_UINT32,
            &replaces_id,
            DBUS_TYPE_STRING,
            &v_iconname,
            DBUS_TYPE_STRING,
            &v_summary,
            DBUS_TYPE_STRING,
            &v_body,
            DBUS_TYPE_INVALID);

        // actions
        dbus_message_iter_init_append (msg, &iter);
        dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "s", &sub);
        dbus_message_iter_close_container (&iter, &sub);

        // hints
        dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "{sv}", &sub);

        if (img != NULL) {
            dbus_int32_t width = gdk_pixbuf_get_width (img);
            dbus_int32_t height = gdk_pixbuf_get_height (img);
            dbus_int32_t stride = gdk_pixbuf_get_rowstride (img);
            dbus_bool_t has_alpha = gdk_pixbuf_get_has_alpha (img);
            dbus_int32_t bits_per_sample = gdk_pixbuf_get_bits_per_sample (img);
            dbus_int32_t channels = gdk_pixbuf_get_n_channels (img);
            guchar *image_bytes = gdk_pixbuf_get_pixels (img);

            DBusMessageIter dict_entry_sub;
            dbus_message_iter_open_container (&sub, DBUS_TYPE_DICT_ENTRY, 0, &dict_entry_sub);

            {
                char *v_image_data = "image-data";
                dbus_message_iter_append_basic (&dict_entry_sub, DBUS_TYPE_STRING, &v_image_data);

                DBusMessageIter value_sub;

                dbus_message_iter_open_container (&dict_entry_sub, DBUS_TYPE_VARIANT, "(iiibiiay)", &value_sub);

                {

                    DBusMessageIter image_sub;
                    dbus_message_iter_open_container (&value_sub, DBUS_TYPE_STRUCT, NULL, &image_sub);

                    {

                        dbus_message_iter_append_basic (&image_sub, DBUS_TYPE_INT32, &width);
                        dbus_message_iter_append_basic (&image_sub, DBUS_TYPE_INT32, &height);
                        dbus_message_iter_append_basic (&image_sub, DBUS_TYPE_INT32, &stride);
                        dbus_message_iter_append_basic (&image_sub, DBUS_TYPE_BOOLEAN, &has_alpha);
                        dbus_message_iter_append_basic (&image_sub, DBUS_TYPE_INT32, &bits_per_sample);
                        dbus_message_iter_append_basic (&image_sub, DBUS_TYPE_INT32, &channels);

                        DBusMessageIter data_sub;

                        dbus_message_iter_open_container (
                            &image_sub,
                            DBUS_TYPE_ARRAY,
                            DBUS_TYPE_BYTE_AS_STRING,
                            &data_sub);

                        {
                            dbus_message_iter_append_fixed_array (
                                &data_sub,
                                DBUS_TYPE_BYTE,
                                &image_bytes,
                                stride * height);
                        }

                        dbus_message_iter_close_container (&image_sub, &data_sub);
                    }

                    dbus_message_iter_close_container (&value_sub, &image_sub);
                }

                dbus_message_iter_close_container (&dict_entry_sub, &value_sub);
            }

            dbus_message_iter_close_container (&sub, &dict_entry_sub);

            g_object_unref (img);
        }

        dbus_message_iter_close_container (&iter, &sub);

        dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &v_timeout);

        replaces_id = notify_send (msg, replaces_id);
    }

    if (should_wait_for_cover) {
        ddb_cover_query_t *query = calloc (sizeof (ddb_cover_query_t), 1);
        query->_size = sizeof (ddb_cover_query_t);
        query->track = track;
        deadbeef->pl_item_ref (track);
        query->source_id = 0;

        void (^completion_block) (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) =
            ^(int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
                if (!(query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
                    char *image_filename;
                    if (cover != NULL && cover->image_filename != NULL) {
                        image_filename = strdup (cover->image_filename);
                    }
                    else {
                        image_filename = strdup ("deadbeef");
                    }
                    // redisplay notification with the album art
                    ddb_playItem_t *track = query->track;
                    deadbeef->pl_item_ref (track);
                    dispatch_async (queue, ^{
                        _replaces_id = show_notification (track, image_filename, replaces_id, 1);
                        free (image_filename);
                        deadbeef->pl_item_unref (track);
                    });
                }
                deadbeef->pl_item_unref (query->track);
                free (query);
                if (cover != NULL) {
                    artwork_plugin->cover_info_release (cover);
                }
            };

        query->user_data = (dispatch_block_t)Block_copy (completion_block);
        artwork_plugin->cover_get (query, _cover_loaded_callback);
    }

    return replaces_id;
}

static int
on_songstarted (DB_playItem_t *track) {
    if (track && deadbeef->conf_get_int ("notify.enable", 0)) {
        if (track) {
            if (terminate) {
                return 0;
            }
            deadbeef->pl_item_ref (track);
            dispatch_async (queue, ^{
                bool should_apply_kde_fix = deadbeef->conf_get_int ("notify.fix_kde_5_23_5", 0) ? true : false;

                // KDE fix: don't reuse notifications;
                // Other environments: need to reuse

                _replaces_id = show_notification (track, NULL, should_apply_kde_fix ? 0 : _replaces_id, 0);
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
    case DB_EV_SONGSTARTED: {
        ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
        on_songstarted (ev->track);
    } break;
    case DB_EV_SONGCHANGED: {
        ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
        on_songstarted (ev->to);
    } break;
    case DB_EV_CONFIGCHANGED:
        init_tf ();
        break;
    }
    return 0;
}

int
notify_start (void) {
    queue = dispatch_queue_create ("OSDNotifyQueue", NULL);
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
    artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
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
    "property \"Don't reuse notifications (KDE quirk)\" checkbox notify.fix_kde_5_23_5 0;\n";

static DB_misc_t plugin = {
    DDB_PLUGIN_SET_API_VERSION.plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.id = "notify",
    .plugin.name = "OSD Notify",
    .plugin.descr =
        "Displays notifications when new track starts.\nRequires dbus and notification daemon to be running.\nNotification daemon should be provided by your desktop environment.\n",
    .plugin.copyright = "OSD Notification plugin for DeaDBeeF Player\n"
                        "Copyright (C) 2009-2014 Oleksiy Yakovenko and contributors\n"
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
                        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n",
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
