#include "../../deadbeef.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "gtkui.h"
#include "ddblistview.h"
#include "progress.h"
#include "support.h"

void
gtkpl_add_dir (DdbListview *ps, char *folder) {
    gtkui_original_pl_add_dir (folder, gtkui_add_file_info_cb, NULL);
    g_free (folder);
}

static void
gtkpl_adddir_cb (gpointer data, gpointer userdata) {
    gtkui_original_pl_add_dir (data, gtkui_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_dirs (GSList *lst) {
    deadbeef->pl_lock ();
    deadbeef->pl_add_files_begin (deadbeef->plt_get_curr ());
    if (g_slist_length (lst) == 1
            && deadbeef->conf_get_int ("gtkui.name_playlist_from_folder", 0)) {
        ddb_playlist_t *p = deadbeef->plt_get_curr ();
        if (p) {
            char t[1000];
            if (!deadbeef->plt_get_title (p, t, sizeof (t))) {
                char *def = _("New Playlist");
                if (!strncmp (t, def, strlen (def))) {
                    const char *folder = strrchr ((char*)lst->data, '/');
                    if (!folder) {
                        folder = lst->data;
                    }
                    deadbeef->plt_set_title (p, folder+1);
                }
            }
            deadbeef->plt_unref (p);
        }
    }
    deadbeef->pl_unlock ();
    g_slist_foreach(lst, gtkpl_adddir_cb, NULL);
    g_slist_free (lst);
    deadbeef->pl_add_files_end ();
}

static void
gtkpl_addfile_cb (gpointer data, gpointer userdata) {
    gtkui_original_pl_add_file (data, gtkui_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_files (GSList *lst) {
    deadbeef->pl_add_files_begin (deadbeef->plt_get_curr ());
    g_slist_foreach(lst, gtkpl_addfile_cb, NULL);
    g_slist_free (lst);
    deadbeef->pl_add_files_end ();
}

static void
add_dirs_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_dirs (lst);
}

void
gtkui_add_dirs (GSList *lst) {
    intptr_t tid = deadbeef->thread_start (add_dirs_worker, lst);
    deadbeef->thread_detach (tid);
}

static void
add_files_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_files (lst);
}

void
gtkui_add_files (struct _GSList *lst) {
    deadbeef->pl_add_files_begin (deadbeef->plt_get_curr ());
    intptr_t tid = deadbeef->thread_start (add_files_worker, lst);
    deadbeef->thread_detach (tid);
}

static void
open_files_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_files (lst);
    gtkui_playlist_changed ();
    extern GtkWidget *mainwin;
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_set_cursor (pl, 0);
    deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 1, 0);
}

void
gtkui_open_files (struct _GSList *lst) {
    deadbeef->pl_clear ();
    playlist_refresh ();

    intptr_t tid = deadbeef->thread_start (open_files_worker, lst);
    deadbeef->thread_detach (tid);
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

static gboolean
set_dnd_cursor_idle (gpointer data) {
    DdbListview *listview = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    if (!data) {
        ddb_listview_set_cursor (listview, -1);
        return FALSE;
    }
    int cursor = deadbeef->pl_get_idx_of (DB_PLAYITEM (data));
    ddb_listview_set_cursor (listview, cursor);
    return FALSE;
}

void
gtkpl_add_fm_dropped_files (DB_playItem_t *drop_before, char *ptr, int length) {
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    deadbeef->pl_add_files_begin (deadbeef->plt_get_curr ());

    DdbListviewIter first = NULL;
    DdbListviewIter after = NULL;
    if (drop_before) {
        after = deadbeef->pl_get_prev (drop_before, PL_MAIN);
    }
    else {
        after = deadbeef->pl_get_last (PL_MAIN);
    }
    const uint8_t *p = (const uint8_t*)ptr;
    while (*p) {
        const uint8_t *pe = p;
        while (*pe && *pe > ' ') {
            pe++;
        }
        if (pe - p < 4096 && pe - p > 7) {
            char fname[(int)(pe - p)];
            strcopy_special (fname, p, pe-p);
            //strncpy (fname, p, pe - p);
            //fname[pe - p] = 0;
            int abort = 0;
            DdbListviewIter inserted = deadbeef->pl_insert_dir (after, fname, &abort, gtkui_add_file_info_cb, NULL);
            if (!inserted && !abort) {
                inserted = deadbeef->pl_insert_file (after, fname, &abort, gtkui_add_file_info_cb, NULL);
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
    free (ptr);

    deadbeef->pl_add_files_end ();
    g_idle_add (set_dnd_cursor_idle, first);
}

struct fmdrop_data {
    char *mem;
    int length;
    DB_playItem_t *drop_before;
};

static void
fmdrop_worker (void *ctx) {
    struct fmdrop_data *data = (struct fmdrop_data *)ctx;
    gtkpl_add_fm_dropped_files (data->drop_before, data->mem, data->length);
    if (data->drop_before) {
        deadbeef->pl_item_unref (data->drop_before);
    }
    free (data);
}

void
gtkui_receive_fm_drop (DB_playItem_t *before, char *mem, int length) {
    struct fmdrop_data *data = malloc (sizeof (struct fmdrop_data));
    if (!data) {
        fprintf (stderr, "gtkui_receive_fm_drop: malloc failed\n");
        return;
    }
    data->mem = mem;
    data->length = length;
    if (before) {
        deadbeef->pl_item_ref (before);
    }
    data->drop_before = before;
    // since it happens in separate thread, we need to addref
    intptr_t tid = deadbeef->thread_start (fmdrop_worker, data);
    deadbeef->thread_detach (tid);
}
