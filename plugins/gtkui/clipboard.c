/*
    Clipboard management
    Copyright (C) 2016 Christian Boxdörfer

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

#include <string.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <stdlib.h>
#include "../../gettext.h"
#include <deadbeef/deadbeef.h>
#include "gtkui.h"
#include "playlist/ddblistview.h"
#include <sys/stat.h>

typedef struct {
    char *plt_title;
    DB_playItem_t **tracks;
    int num_tracks;
    int cut;
} clipboard_data_context_t;

static clipboard_data_context_t *current_clipboard_data = NULL;
static int current_clipboard_refcount = 0;

enum {
    DDB_URI_LIST = 1,
    URI_LIST,
    GNOME_COPIED_FILES,
    N_CLIPBOARD_TARGETS
};

static GtkTargetEntry targets[]=
{
    {TARGET_URIS, GTK_TARGET_SAME_WIDGET, DDB_URI_LIST},
    {"text/uri-list", 0, URI_LIST},
    {"x-special/gnome-copied-files", 0, GNOME_COPIED_FILES},
};

static void
clipboard_write_uri_list (clipboard_data_context_t *ctx, GString* buf)
{
    if (ctx->tracks && buf) {
        for (int i = 0; i < ctx->num_tracks; i++) {
            char *str = g_filename_to_uri (deadbeef->pl_find_meta (ctx->tracks[i],":URI"), NULL, NULL);
            g_string_append (buf, str);
            g_free (str);
            if (i < ctx->num_tracks - 1) {
                g_string_append (buf, "\r\n");
            }
        }
    }
}

static void
clipboard_write_gnome_uri_list (clipboard_data_context_t *ctx, GString* buf)
{
    if (ctx->tracks && buf) {
        for (int i = 0; i < ctx->num_tracks; i++) {
            char *str = g_filename_to_uri (deadbeef->pl_find_meta (ctx->tracks[i],":URI"), NULL, NULL);
            g_string_append (buf, str);
            g_free (str);
            if (i < ctx->num_tracks - 1) {
                g_string_append_c (buf, '\n');
            }
        }
    }
}

static void
clipboard_get_clipboard_data (GtkClipboard *clip, GtkSelectionData *sel, guint info, gpointer user_data)
{
    clipboard_data_context_t *clip_ctx = (clipboard_data_context_t *)user_data;
    GdkAtom target = gtk_selection_data_get_target (sel);

    GString *uri_list = g_string_sized_new (clip_ctx->num_tracks * 256);
    guchar *buf = NULL;
    gint buf_len = 0;
    if (info == DDB_URI_LIST) {
        buf = (guchar *)clip_ctx;
        buf_len = sizeof (clipboard_data_context_t);
    }
    else if (info == GNOME_COPIED_FILES) {
        g_string_append (uri_list, clip_ctx->cut ? "cut\n" : "copy\n");
        clipboard_write_gnome_uri_list (clip_ctx, uri_list);
        buf = (guchar *)uri_list->str;
        buf_len = (gint)(uri_list->len + 1);
    }
    else /* text/uri-list format */ {
        clipboard_write_uri_list (clip_ctx, uri_list);
        buf = (guchar *)uri_list->str;
        buf_len = (gint)(uri_list->len + 1);
    }
    gtk_selection_data_set (sel, target, 8, buf, buf_len);
    g_string_free (uri_list, TRUE);
}

static void
clipboard_free (GtkClipboard *clipboard,
                gpointer      user_data)
{
    clipboard_data_context_t *clip_ctx = (clipboard_data_context_t *)user_data;
    if (clip_ctx) {
        if (clip_ctx->tracks) {
            for (int i = 0; i < clip_ctx->num_tracks; i++) {
                if (clip_ctx->tracks[i]) {
                    deadbeef->pl_item_unref (clip_ctx->tracks[i]);
                }
            }
            free (clip_ctx->tracks);
            clip_ctx->tracks = NULL;
        }
        if (clip_ctx->plt_title) {
            free (clip_ctx->plt_title);
            clip_ctx->plt_title = NULL;
        }
        clip_ctx->num_tracks = 0;
        clip_ctx->cut = 0;
        free (clip_ctx);
        clip_ctx = NULL;
    }
    current_clipboard_refcount--;
}

void
clipboard_free_current (void)
{
    if (current_clipboard_refcount > 0) {
        clipboard_free (NULL, current_clipboard_data);
    }
}

static gboolean
clipboard_cut_or_copy_files (GtkWidget* src_widget, clipboard_data_context_t *ctx)
{
    GdkDisplay *display = src_widget ? gtk_widget_get_display (src_widget) : gdk_display_get_default();
    GtkClipboard *clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_CLIPBOARD);
    gboolean ret;
    ret = gtk_clipboard_set_with_data (clipboard, targets, G_N_ELEMENTS(targets),
                                      clipboard_get_clipboard_data, clipboard_free, ctx);
    return ret;
}

static gboolean
clipboard_get_selected_tracks (clipboard_data_context_t *ctx, ddb_playlist_t *plt)
{
    if (plt == NULL) {
        return FALSE;
    }

    deadbeef->pl_lock ();
    int num = deadbeef->plt_getselcount (plt);

    if (num <= 0) {
        deadbeef->pl_unlock ();
        return FALSE;
    }

    ctx->tracks = malloc (sizeof (DB_playItem_t *) * num);
    if (!ctx->tracks) {
        fprintf (stderr, "gtkui: failed to alloc %d bytes to store selected tracks\n", (int)(num * sizeof (void *)));
        deadbeef->pl_unlock ();
        return FALSE;
    }

    int n = 0;
    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it) && n < num) {
            deadbeef->pl_item_ref (it);
            ctx->tracks[n++] = it;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    ctx->num_tracks = num;

    deadbeef->pl_unlock ();
    return TRUE;
}

static gboolean
clipboard_get_all_tracks (clipboard_data_context_t *ctx, ddb_playlist_t *plt)
{
    if (plt == NULL) {
        return FALSE;
    }

    deadbeef->pl_lock ();

    char plt_title[1000] = "";
    deadbeef->plt_get_title (plt, plt_title, sizeof (plt_title));
    ctx->plt_title = strdup (plt_title);
    int num_tracks = deadbeef->plt_get_item_count (plt, PL_MAIN);

    if (num_tracks <= 0) {
        deadbeef->pl_unlock ();
        return FALSE;
    }

    ctx->tracks = malloc (sizeof (DB_playItem_t *) * num_tracks);
    if (!ctx->tracks) {
        fprintf (stderr, "gtkui: failed to alloc %d bytes to store playlist tracks\n", (int)(num_tracks * sizeof (void *)));
        deadbeef->pl_unlock ();
        return FALSE;
    }

    int n = 0;
    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        deadbeef->pl_item_ref (it);
        ctx->tracks[n++] = it;
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    ctx->num_tracks = num_tracks;

    deadbeef->pl_unlock ();
    return TRUE;
}

static void
clipboard_delete_selected_tracks (ddb_playlist_t *plt)
{
    if (plt == NULL) {
        return;
    }

    int cursor = deadbeef->plt_delete_selected (plt);

    deadbeef->plt_set_cursor (plt, PL_MAIN, cursor);
    deadbeef->plt_save_config (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

static void
clipboard_delete_playlist (ddb_playlist_t *plt)
{
    if (plt == NULL) {
        return;
    }
    int idx = deadbeef->plt_get_idx (plt);
    if (idx != -1) {
        deadbeef->plt_remove (idx);
    }
}

void
clipboard_cut_selection (ddb_playlist_t *plt, int ctx) {

    if (plt == NULL) {
        return;
    }

    clipboard_data_context_t *clip_ctx = malloc (sizeof (clipboard_data_context_t));

    // save ptr to free on exit
    current_clipboard_data = clip_ctx;
    current_clipboard_refcount++;

    clip_ctx->plt_title = NULL;

    int result = 0;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        result = clipboard_get_selected_tracks (clip_ctx, plt);
        if (result) {
            clipboard_delete_selected_tracks (plt);
        }
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        result = clipboard_get_all_tracks (clip_ctx, plt);
        if (result) {
            clipboard_delete_playlist (plt);
        }
    }

    if (result) {
        clip_ctx->cut = 0;
        clipboard_cut_or_copy_files (mainwin, clip_ctx);
    }
}

void
clipboard_copy_selection (ddb_playlist_t *plt, int ctx) {
    if (plt == NULL) {
        return;
    }

    clipboard_data_context_t *clip_ctx = malloc (sizeof (clipboard_data_context_t));

    // save ptr to free on exit
    current_clipboard_data = clip_ctx;
    current_clipboard_refcount++;

    clip_ctx->plt_title = NULL;

    int result = 0;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        result = clipboard_get_selected_tracks (clip_ctx, plt);
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        result = clipboard_get_all_tracks (clip_ctx, plt);
    }

    if (result) {
        clip_ctx->cut = 0;
        clipboard_cut_or_copy_files (mainwin, clip_ctx);
    }
}

static void
clipboard_activate_dest_playlist (const char *pdata, ddb_playlist_t *plt, int ctx)
{
    if (plt == NULL) {
        // no playlist set, use current one
        deadbeef->plt_set_curr (plt);
        return;
    }
    clipboard_data_context_t *clip_ctx = (clipboard_data_context_t *)pdata;
    if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        int playlist = -1;
        if (clip_ctx && clip_ctx->plt_title) {
            int cnt = deadbeef->plt_get_count ();
            playlist = deadbeef->plt_add (cnt, clip_ctx->plt_title);
        }
        else {
            playlist = gtkui_add_new_playlist ();
        }
        if (playlist != -1) {
            deadbeef->plt_set_curr_idx (playlist);
        }
    }
}

static void
clipboard_received_uri_list (const char *pdata, int length)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        int cursor = deadbeef->plt_get_cursor (plt, PL_MAIN);
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
        if (it) {
            gchar *ptr = (char *)pdata;
            if (ptr && length > 0) {
                char *mem = malloc (length+1);
                memcpy (mem, ptr, length);
                mem[length] = 0;
                // use drop procedure
                gtkui_receive_fm_drop (it, mem, length);
            }
            deadbeef->pl_item_unref (it);
        }
        deadbeef->plt_unref (plt);
    }
}

static void
clipboard_received_ddb_uri_list (const char *pdata)
{
    clipboard_data_context_t *ctx = (clipboard_data_context_t *)pdata;
    DB_playItem_t **tracks = ctx->tracks;
    int num_tracks = ctx->num_tracks;
    if (!tracks || num_tracks <= 0) {
        return;
    }
    deadbeef->pl_lock ();

    ddb_playlist_t *plt = deadbeef->plt_get_curr ();

    if (plt) {
        int insert_pos = MAX (-1, deadbeef->plt_get_cursor (plt, PL_MAIN) - 1);
        deadbeef->plt_deselect_all (plt);
        for (int i = 0; i < num_tracks; i++) {
            DB_playItem_t *it = tracks[i];
            if (!it) {
                printf ("gtkui paste: warning: item %d not found\n", i);
                continue;
            }
            DB_playItem_t *it_new = deadbeef->pl_item_alloc ();
            deadbeef->pl_item_copy (it_new, it);
            deadbeef->pl_set_selected (it_new, 1);

            DB_playItem_t *after = deadbeef->pl_get_for_idx_and_iter (insert_pos++, PL_MAIN);
            deadbeef->plt_insert_item (plt, after, it_new);
            deadbeef->pl_item_unref (it_new);
            if (after) {
                deadbeef->pl_item_unref (after);
            }
        }
        deadbeef->pl_unlock ();
        deadbeef->plt_save_config (plt);
        deadbeef->plt_unref (plt);
    }
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

static GdkAtom target_atom[N_CLIPBOARD_TARGETS];

static gboolean got_atoms = FALSE;

static void
clipboard_check_atoms (void)
{
    if (!got_atoms) {
        for (int i = 0; i < N_CLIPBOARD_TARGETS; i++) {
            target_atom[i] = GDK_NONE;
        }
        for (int i = 0; i < G_N_ELEMENTS (targets); i++) {
            target_atom[targets[i].info] = gdk_atom_intern_static_string (targets[i].target);
        }
        got_atoms = TRUE;
    }
}

void
clipboard_paste_selection (ddb_playlist_t *plt, int ctx)
{
    if (!plt) {
        return;
    }
    GdkDisplay *display = mainwin ? gtk_widget_get_display (mainwin) : gdk_display_get_default();
    GtkClipboard *clip = gtk_clipboard_get_for_display (display, GDK_SELECTION_CLIPBOARD);
    int type = 0;
    GdkAtom *avail_targets = NULL;
    int n = 0;

    // get all available targets currently in the clipboard.
    if (!gtk_clipboard_wait_for_targets (clip, &avail_targets, &n)) {
        return;
    }

    clipboard_check_atoms ();

    // we prefer DDB_URI_LIST, so first check all tragets for that
    for (int i = 0; i < n; i++) {
        if (avail_targets[i] == target_atom[DDB_URI_LIST]) {
            type = DDB_URI_LIST;
            break;
        }
    }
    if (type == 0) {
        // no DDB_URI_LIST, check for other supported targets
        for (int i = 0; i < n; i++) {
            if (avail_targets[i] == target_atom[GNOME_COPIED_FILES]) {
                type = GNOME_COPIED_FILES;
                break;
            }
            else if (avail_targets[i] == target_atom[URI_LIST]) {
                type = URI_LIST;
                break;
            }
        }
    }
    g_free (avail_targets);

    if (type) {
        GtkSelectionData *data = gtk_clipboard_wait_for_contents (clip, target_atom[type]);
        const gchar *pdata = (const gchar *)gtk_selection_data_get_data (data);
        gint data_len = gtk_selection_data_get_length (data);

        switch (type) {
            case DDB_URI_LIST:
                clipboard_activate_dest_playlist (pdata, plt, ctx);
                clipboard_received_ddb_uri_list (pdata);
                break;
            case GNOME_COPIED_FILES:
                clipboard_activate_dest_playlist (NULL, plt, ctx);
                clipboard_received_uri_list (pdata, data_len);
                break;
            case URI_LIST:
                clipboard_activate_dest_playlist (NULL, plt, ctx);
                clipboard_received_uri_list (pdata, data_len);
                break;
            default:
                break;
        }
        gtk_selection_data_free (data);
    }
}

int
clipboard_is_clipboard_data_available (void)
{
    GdkDisplay *display = mainwin ? gtk_widget_get_display (mainwin) : gdk_display_get_default();
    GtkClipboard *clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_CLIPBOARD);
    clipboard_check_atoms ();
    for (int i = 0; i < N_CLIPBOARD_TARGETS; i++) {
        if (gtk_clipboard_wait_is_target_available (clipboard, target_atom[i])) {
            return 1;
        }
    }
    return 0;
}

