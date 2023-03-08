/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <deadbeef/deadbeef.h>
#include "../../gettext.h"
#include "gtkui.h"
#include "gtkui_api.h"
#include "gtkui_deletefromdisk.h"

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t plugin;
#define trace(...) { deadbeef->log_detailed (&plugin.gui.plugin, 0, __VA_ARGS__); }

void
gtkui_warning_message_for_ctx (ddbDeleteFromDiskController_t ctl, ddb_action_context_t ctx, unsigned trackcount, ddbDeleteFromDiskControllerWarningCallback_t callback) {
    if (deadbeef->conf_get_int ("gtkui.delete_files_ask", 1)) {
        char buf[1000];
        const char *buf2 = deadbeef->conf_get_int ("gtkui.move_to_trash", 1) ?
        _(" The files will be moved to trash.\n\n(This dialog can be turned off in GTKUI plugin settings)") :
        _(" The files will be lost.\n\n(This dialog can be turned off in GTKUI plugin settings)");

        if (ctx == DDB_ACTION_CTX_SELECTION) {
            int selected_files = trackcount;
            if (selected_files == 1) {
                snprintf(buf, sizeof (buf), _("Do you really want to delete the selected file?%s"), buf2);
            } else {
                snprintf(buf, sizeof (buf), _("Do you really want to delete all %d selected files?%s"), selected_files, buf2);
            }
        }
        else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
            int files = trackcount;
            snprintf(buf, sizeof (buf), _("Do you really want to delete all %d files from the current playlist?%s"), files, buf2);
        }
        else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
            snprintf(buf, sizeof (buf), _("Do you really want to delete the currently playing file?%s"), buf2);
        }

        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, _("Delete files from disk"));

        gtk_dialog_add_button(GTK_DIALOG(dlg), _("Cancel"), GTK_RESPONSE_NO);
        gtk_dialog_add_button(GTK_DIALOG(dlg), _("Delete"), GTK_RESPONSE_YES);

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), "%s", buf);
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

#if GTK_CHECK_VERSION(3,0,0)
        GtkWidget *btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);
        gtk_style_context_add_class (gtk_widget_get_style_context(btn), "destructive-action");
#endif

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            callback(ctl, 1);
            return;
        }
    }

    callback(ctl, 0);
}

int
gtkui_delete_file (ddbDeleteFromDiskController_t ctl, const char *uri) {
    if (deadbeef->conf_get_int ("gtkui.move_to_trash", 1)) {
        GFile *file = g_file_new_for_path (uri);
        g_file_trash (file, NULL, NULL);
        g_object_unref (file);
    } else {
        (void)unlink (uri);
    }

    // check if file still exists
    struct stat buf;
    memset (&buf, 0, sizeof (buf));
    int stat_res = stat (uri, &buf);

    if (stat_res == 0) {
        trace ("Failed to delete file: %s\n", uri);
        return 0;
    }
    return 1;
}
