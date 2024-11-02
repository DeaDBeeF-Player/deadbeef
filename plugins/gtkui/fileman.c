/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#include <deadbeef/deadbeef.h>
#include <dispatch/dispatch.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "gtkui.h"
#include "playlist/ddblistview.h"
#include "progress.h"
#include "support.h"
#include "undointegration.h"

static void
gtkpl_adddir_cb (gpointer data, gpointer userdata) {
    ddb_playlist_t *plt = userdata;
    deadbeef->plt_add_dir2 (0, plt, data, NULL, NULL);
    g_free (data);
}

void
gtkpl_add_dirs (ddb_playlist_t *plt, GSList *lst) {
    int empty = 0 == deadbeef->plt_get_item_count (plt, PL_MAIN);
    if (g_slist_length (lst) == 1
            && deadbeef->conf_get_int ("gtkui.name_playlist_from_folder", 1)) {
        char t[1000];
        if (!deadbeef->plt_get_title (plt, t, sizeof (t))) {
            char *def = _("New Playlist");
            if (!strncmp (t, def, strlen (def)) || empty) {
                const char *folder = strrchr ((char*)lst->data, G_DIR_SEPARATOR);
                if (!folder) {
                    folder = lst->data;
                }
                deadbeef->plt_set_title (plt, folder+1);
            }
        }
    }
    g_slist_foreach(lst, gtkpl_adddir_cb, plt);
}

static void
gtkpl_addfile_cb (gpointer data, gpointer userdata) {
    ddb_playlist_t *plt = userdata;
    deadbeef->plt_add_file2 (0, plt, data, NULL, 0);
    g_free (data);
}

static void
gtkpl_add_files (ddb_playlist_t *plt, GSList *lst) {
    g_slist_foreach(lst, gtkpl_addfile_cb, plt);
    g_slist_free (lst);
}

void
gtkui_add_dirs (GSList *lst) {
    ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();
    ddb_playlist_t *plt = deadbeef->plt_alloc ("add-dirs");

    if (deadbeef->plt_add_files_begin (plt_curr, 0) < 0) {
        deadbeef->plt_unref (plt);
        deadbeef->plt_unref (plt_curr);
        g_slist_free (lst);
        return;
    }

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        gtkpl_add_dirs (plt, lst);

        // TODO: handle cancel

        gtkui_dispatch_on_main(^{
            ddb_playItem_t *tail = deadbeef->plt_get_tail_item(plt_curr, PL_MAIN);
            ddb_undo->set_action_name (_("Add Folders"));
            deadbeef->plt_move_all_items (plt_curr, plt, tail);
            if (tail != NULL) {
                deadbeef->pl_item_unref (tail);
            }

            deadbeef->plt_save_config (plt_curr);
            deadbeef->plt_add_files_end (plt_curr, 0);
            deadbeef->plt_unref (plt_curr);
            deadbeef->plt_unref (plt);
            g_slist_free (lst);
        });
    });
}

void
gtkui_add_files (struct _GSList *lst) {
    ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();
    ddb_playlist_t *plt = deadbeef->plt_alloc ("add-files");

    if (deadbeef->plt_add_files_begin (plt_curr, 0) < 0) {
        g_slist_free (lst);
        deadbeef->plt_unref (plt_curr);
        deadbeef->plt_unref (plt);
        return;
    }

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        gtkpl_add_files (plt, lst);
        gtkui_dispatch_on_main (^{
            ddb_playItem_t *tail = deadbeef->plt_get_tail_item (plt_curr, PL_MAIN);
            ddb_undo->set_action_name (_("Add Files"));
            deadbeef->plt_move_all_items (plt_curr, plt, tail);
            if (tail != NULL) {
                deadbeef->pl_item_unref (tail);
                tail = NULL;
            }

            deadbeef->plt_save_config (plt_curr);
            deadbeef->plt_add_files_end (plt_curr, 0);
            deadbeef->plt_unref (plt_curr);
            deadbeef->plt_unref (plt);
        });
    });
}

void
gtkui_open_files (struct _GSList *lst) {
    ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();
    deadbeef->plt_clear (plt_curr);
    deadbeef->plt_unref (plt_curr);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

    gtkui_add_files(lst);
}

void
gtkui_add_location (const char *path, const char *custom_title) {
    ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();
    ddb_playlist_t *plt = deadbeef->plt_alloc ("add-location");

    if (deadbeef->plt_add_files_begin (plt_curr, 0) < 0) {
        deadbeef->plt_unref (plt);
        deadbeef->plt_unref (plt_curr);
        return;
    }

    char *custom_title_copy = NULL;
    if (custom_title != NULL) {
        custom_title_copy = strdup (custom_title);
    }

    char *path_copy = strdup (path);

    dispatch_async (dispatch_get_global_queue (DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        DB_playItem_t *tail = deadbeef->plt_get_last (plt, PL_MAIN);
        DB_playItem_t *it = deadbeef->plt_insert_file2 (0, plt, tail, path_copy, NULL, NULL, NULL);

        if (tail) {
            deadbeef->pl_item_unref (tail);
            tail = NULL;
        }

        if (it == NULL) {
            deadbeef->plt_add_files_end (plt_curr, 0);

            free (path_copy);
            free (custom_title_copy);
            deadbeef->plt_unref (plt);
            deadbeef->plt_unref (plt_curr);

            return;
        }

        deadbeef->pl_item_ref (it);

        gtkui_dispatch_on_main (^{
            if (custom_title_copy != NULL) {
                deadbeef->pl_replace_meta (it, ":CUSTOM_TITLE", custom_title_copy);
            }

            ddb_playItem_t *tail = deadbeef->plt_get_tail_item (plt_curr, PL_MAIN);
            deadbeef->plt_move_all_items (plt_curr, plt, tail);
            if (tail != NULL) {
                deadbeef->pl_item_unref (tail);
            }

            deadbeef->plt_save_config (plt_curr);
            ddb_undo->set_action_name (_("Add Location"));
            deadbeef->plt_add_files_end (plt_curr, 0);

            free (path_copy);
            free (custom_title_copy);
            deadbeef->pl_item_unref (it);
            deadbeef->plt_unref (plt);
            deadbeef->plt_unref (plt_curr);
        });
    });
}

void
strcopy_special (char *dest, const char *src, int len) {
    while (len > 0) {
        if (*src == '%' && len >= 3) {
            int charcode = 0;
            int byte;
            byte = tolower (src[2]);
            if (byte >= '0' && byte <= '9') {
                charcode = byte - '0';
            }
            else if (byte >= 'a' && byte <= 'f') {
                charcode = byte - 'a' + 10;
            }
            else {
                charcode = '?';
            }
            if (charcode != '?') {
                byte = tolower (src[1]);
                if (byte >= '0' && byte <= '9') {
                    charcode |= (byte - '0') << 4;
                }
                else if (byte >= 'a' && byte <= 'f') {
                    charcode |= (byte - 'a' + 10) << 4;
                }
                else {
                    charcode = '?';
                }
            }
            *dest = charcode;
            dest++;
            src += 3;
            len -= 3;
            continue;
        }
        else {
            *dest++ = *src++;
            len--;
        }
    }
    *dest = 0;
}

static void
set_dnd_cursor (ddb_playItem_t *first) {
    if (first == NULL) {
        deadbeef->pl_set_cursor (PL_MAIN, -1);
        return;
    }
    int cursor = deadbeef->pl_get_idx_of (first);
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
}

static ddb_playItem_t *
gtkpl_add_fm_dropped_files (ddb_playlist_t *plt, DB_playItem_t *drop_before, const char *ptr, int length) {
    DdbListviewIter first = NULL;
    DdbListviewIter after = NULL;
    if (drop_before) {
        after = deadbeef->pl_get_prev (drop_before, PL_MAIN);
    }
    else {
        after = deadbeef->plt_get_last (plt, PL_MAIN);
    }
    const uint8_t *p = (const uint8_t*)ptr;
    while (*p) {
        const uint8_t *pe = p;
        while (*pe && *pe >= ' ') {
            pe++;
        }
        if (pe - p < 4096 && pe - p > 7) {
            char fname[(int)(pe - p)+1];
            strcopy_special (fname, (const char *)p, (int)(pe-p));
            int abort = 0;
            DdbListviewIter inserted = deadbeef->plt_insert_dir2 (0, plt, after, fname, &abort, NULL, NULL);
            if (!inserted && !abort) {
                inserted = deadbeef->plt_insert_file2 (0, plt, after, fname, &abort, NULL, NULL);
                if (!inserted && !abort) {
                    inserted = deadbeef->plt_load2 (0, plt, after, fname, &abort, NULL, NULL);
                }
            }
            if (inserted) {
                if (!first) {
                    first = inserted;
                }
                if (after) {
                    deadbeef->pl_item_unref (after);
                }
                after = inserted;
                deadbeef->pl_item_ref (after);
            }
        }
        p = pe;
        // skip whitespace
        while (*p && *p <= ' ') {
            p++;
        }
    }
    if (after) {
        deadbeef->pl_item_unref (after);
    }

    if (first != NULL) {
        deadbeef->pl_item_ref (first);
    }
    return first;
}

struct fmdrop_data {
    char *mem;
    int length;
    DB_playItem_t *drop_before;
};

void
gtkui_receive_fm_drop (DB_playItem_t *before, char *mem, int length) {
    struct fmdrop_data *data = calloc (1, sizeof (struct fmdrop_data));
    data->mem = mem;
    data->length = length;
    if (before) {
        deadbeef->pl_item_ref (before);
    }
    data->drop_before = before;

    ddb_playlist_t *plt = deadbeef->plt_alloc ("receive-drag-drop");
    ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();

    if (deadbeef->plt_add_files_begin (plt_curr, 0) < 0) {
        if (data->drop_before) {
            deadbeef->pl_item_unref (data->drop_before);
        }
        free (mem);
        free (data);
        deadbeef->plt_unref (plt);
        deadbeef->plt_unref (plt_curr);
        return;
    }


    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        ddb_playItem_t *first = gtkpl_add_fm_dropped_files (plt, NULL, data->mem, data->length);

        gtkui_dispatch_on_main(^{
            ddb_undo->set_action_name (_("Drag & Drop"));

            ddb_playItem_t *after;
            if (data->drop_before != NULL) {
                after = deadbeef->pl_get_prev(data->drop_before, PL_MAIN);
            }
            else {
                after = deadbeef->plt_get_last(plt_curr, PL_MAIN);
            }
            deadbeef->plt_move_all_items (plt_curr, plt, after);
            if (after != NULL) {
                deadbeef->pl_item_unref (after);
            }

            deadbeef->plt_save_config (plt_curr);
            deadbeef->plt_add_files_end (plt_curr, 0);
            set_dnd_cursor (first);

            if (first != NULL) {
                deadbeef->pl_item_unref (first);
            }
            if (data->drop_before) {
                deadbeef->pl_item_unref (data->drop_before);
            }
            free (mem);
            free (data);
            deadbeef->plt_unref (plt);
            deadbeef->plt_unref (plt_curr);
        });
    });
}

#if GTK_CHECK_VERSION(3,20,0)
#    ifdef __MINGW32__
#        define USE_GTK_NATIVE_FILE_CHOOSER
#    endif
#else // Make sure that native file chooser is off for GTK2
#    undef USE_GTK_NATIVE_FILE_CHOOSER
#endif

#ifdef USE_GTK_NATIVE_FILE_CHOOSER
static GtkFileFilter *
set_file_filter (GtkFileChooser *dlg, const char *name) {
    if (!name) {
        name = _("Supported sound formats");
    }

    GtkFileFilter* flt;

    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, name);

    // Add supported extensions
        DB_decoder_t **codecs = deadbeef->plug_get_decoder_list ();
        for (int i = 0; codecs[i]; i++) {
            if (codecs[i]->exts && codecs[i]->insert) {
                const char **exts = codecs[i]->exts;
                char buf[100];
                for (int e = 0; exts[e]; e++) {
                    snprintf (buf, sizeof (buf), "*.%s", exts[e]);
                    gtk_file_filter_add_pattern (flt, buf);
                }
            }
        }
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);

    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("All files (*)"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    return flt;
}

#else

static gboolean
file_filter_func (const GtkFileFilterInfo *filter_info, gpointer data) {
    // get ext
    const char *p = strrchr (filter_info->filename, '.');
    if (!p) {
        return FALSE;
    }
    p++;

    // get beginning of fname
    const char *fn = strrchr (filter_info->filename, '/');
    if (!fn) {
        fn = filter_info->filename;
    }
    else {
        fn++;
    }

    if (!strcasecmp (p, "cue")) {
        return TRUE;
    }

    DB_decoder_t **codecs = deadbeef->plug_get_decoder_list ();
    for (int i = 0; codecs[i]; i++) {
        if (codecs[i]->exts && codecs[i]->insert) {
            const char **exts = codecs[i]->exts;
            for (int e = 0; exts[e]; e++) {
                if (!strcasecmp (exts[e], p)) {
                    return TRUE;
                }
            }
        }
        if (codecs[i]->prefixes && codecs[i]->insert) {
            const char **prefixes = codecs[i]->prefixes;
            for (int e = 0; prefixes[e]; e++) {
                if (!strncasecmp (prefixes[e], fn, strlen(prefixes[e])) && *(fn + strlen (prefixes[e])) == '.') {
                    return TRUE;
                }
            }
        }
    }

    // test container (vfs) formats
    DB_vfs_t **vfsplugs = deadbeef->plug_get_vfs_list ();
    for (int i = 0; vfsplugs[i]; i++) {
        if (vfsplugs[i]->is_container) {
            if (vfsplugs[i]->is_container (filter_info->filename)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static GtkFileFilter *
set_file_filter (GtkFileChooser *dlg, const char *name) {
    if (!name) {
        name = _("Supported sound formats");
    }

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, name);

    gtk_file_filter_add_custom (flt, GTK_FILE_FILTER_FILENAME, file_filter_func, NULL, NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("All files (*)"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    return flt;
}
#endif //USE_GTK_NATIVE_FILE_CHOOSER

#ifdef USE_GTK_NATIVE_FILE_CHOOSER

static GtkFileFilter *
set_file_filter_loadplaylist (GtkFileChooser *dlg) {
    static char extlist[10000];

    // Build extension list
    if (extlist[0] == '\0') {

        DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
        for (int i = 0; plug[i]; i++) {
            if (plug[i]->extensions && plug[i]->load) {
                const char **exts = plug[i]->extensions;
                if (exts) {
                    for (int e = 0; exts[e]; e++) {
                        char buf[100];
                        snprintf (buf, sizeof (buf), "*.%s;", exts[e]);
                        strcat (extlist, buf);
                    }
                }
            }
        }
    }

    GtkFileFilter* flt;

    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("Supported playlist formats"));
    gtk_file_filter_add_pattern (flt, extlist);
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);

    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("All files"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    return flt;
}

#else

static gboolean
playlist_filter_func (const GtkFileFilterInfo *filter_info, gpointer data) {
    // get ext
    const char *p = strrchr (filter_info->filename, '.');
    if (!p) {
        return FALSE;
    }
    p++;
    DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
    for (int i = 0; plug[i]; i++) {
        if (plug[i]->extensions && plug[i]->load) {
            const char **exts = plug[i]->extensions;
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    if (!strcasecmp (exts[e], p)) {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

static void
set_file_filter_loadplaylist(GtkFileChooser *dlg)
{
    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("Supported playlist formats"));
    gtk_file_filter_add_custom (flt, GTK_FILE_FILTER_FILENAME, playlist_filter_func, NULL, NULL);
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("Other files (*)"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
}

#endif //USE_GTK_NATIVE_FILE_CHOOSER

static void
set_file_filter_saveplaylist (GtkFileChooser *dlg) {
    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("DeaDBeeF playlist files (*.dbpl)"));
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
    for (int i = 0; plug[i]; i++) {
        if (plug[i]->extensions && plug[i]->load) {
            const char **exts = plug[i]->extensions;
            if (exts && plug[i]->save) {
                for (int e = 0; exts[e]; e++) {
                    char s[100];
                    flt = gtk_file_filter_new ();
                    gtk_file_filter_set_name (flt, exts[e]);
                    snprintf (s, sizeof (s), "*.%s", exts[e]);
                    gtk_file_filter_add_pattern (flt, s);
                    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
                }
            }
        }
    }
}

#ifndef USE_GTK_NATIVE_FILE_CHOOSER
static void
on_follow_symlinks_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("add_folders_follow_symlinks", gtk_toggle_button_get_active (togglebutton));
}
#endif

static GtkFileChooser *
get_file_chooser  (const gchar          *title,
                          GtkFileChooserAction action,
                          gboolean             select_multiple)
{
#ifdef USE_GTK_NATIVE_FILE_CHOOSER
    GtkFileChooserNative *dlg = gtk_file_chooser_native_new (title, GTK_WINDOW (mainwin), action, NULL, NULL);
#else
    GtkWidget *dlg = gtk_file_chooser_dialog_new (title, GTK_WINDOW (mainwin), action, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    if (action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
        GtkWidget *box = gtk_hbox_new (FALSE, 8);
        gtk_widget_show (box);

        GtkWidget *check = gtk_check_button_new_with_mnemonic (_("Follow symlinks"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), deadbeef->conf_get_int ("add_folders_follow_symlinks", 0));
        g_signal_connect ((gpointer) check, "toggled",
                G_CALLBACK (on_follow_symlinks_toggled),
                NULL);
        gtk_widget_show (check);
        gtk_box_pack_start (GTK_BOX (box), check, FALSE, FALSE, 0);

        gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dlg), box);
    }
#endif
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), select_multiple);

    return GTK_FILE_CHOOSER (dlg);
}

static int
run_file_chooser(GtkFileChooser *dlg)
{
#ifdef USE_GTK_NATIVE_FILE_CHOOSER
    return gtk_native_dialog_run (GTK_NATIVE_DIALOG  (dlg));
#else
    return gtk_dialog_run (GTK_DIALOG (dlg));
#endif
}

static void
destroy_file_chooser(GtkFileChooser *dlg)
{
#ifdef USE_GTK_NATIVE_FILE_CHOOSER
    gtk_native_dialog_destroy (GTK_NATIVE_DIALOG (dlg));
#else
    gtk_widget_destroy (GTK_WIDGET (dlg));
#endif
}


GSList *
show_file_chooser (const gchar          *title,
                   enum GtkuiFileChooserType type,
                   gboolean             select_multiple)
{
    GtkFileChooserAction action;
    switch (type) {
    case GTKUI_FILECHOOSER_OPENFOLDER:
        action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
        break;
    case GTKUI_FILECHOOSER_OPENFILE:
    case GTKUI_FILECHOOSER_LOADPLAYLIST:
        action = GTK_FILE_CHOOSER_ACTION_OPEN;
        break;
    case GTKUI_FILECHOOSER_SAVEPLAYLIST:
        action = GTK_FILE_CHOOSER_ACTION_SAVE;
        break;
    }

    GtkFileChooser *dlg = get_file_chooser(title, action, select_multiple);

    switch (type) {
    case GTKUI_FILECHOOSER_OPENFILE:
        set_file_filter (dlg, NULL);
        break;
    case GTKUI_FILECHOOSER_LOADPLAYLIST:
        set_file_filter_loadplaylist(dlg);
        break;
    case GTKUI_FILECHOOSER_SAVEPLAYLIST:
        gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "untitled.dbpl");
        set_file_filter_saveplaylist(dlg);
        break;
    case GTKUI_FILECHOOSER_OPENFOLDER:
        break;
    }

    const char *conf_lastdir;
    switch (type) {
    case GTKUI_FILECHOOSER_OPENFOLDER:
    case GTKUI_FILECHOOSER_OPENFILE:
        conf_lastdir = "filechooser.lastdir";
        break;
    case GTKUI_FILECHOOSER_LOADPLAYLIST:
    case GTKUI_FILECHOOSER_SAVEPLAYLIST:
        conf_lastdir = "filechooser.playlist.lastdir";
        break;
    }

    // restore folder
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast (conf_lastdir, ""));
    deadbeef->conf_unlock ();

    int response = run_file_chooser(dlg);

    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str (conf_lastdir, folder);
        g_free (folder);
    }
    GSList *lst = NULL;
    if (response == GTK_RESPONSE_ACCEPT)
    {
        lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
#if defined USE_GTK_NATIVE_FILE_CHOOSER && defined __MINGW32__
        // workaround: Gtk's win32 file chooser uses g_slist_prepend internally and forgets to reverse it before returning
        lst = g_slist_reverse (lst);
#endif
    }

    destroy_file_chooser (dlg);
    return lst;
}

char *
gtkui_trim_whitespace(char *p, size_t len) {
    char *meta = p;
    while (*meta == ' ') {
        meta++;
    }
    char *tail = p + len - 1;
    while (tail > p && ((uint8_t)*tail) <= 0x20) {
        *tail = 0;
        tail--;
    }
    return meta;
}
