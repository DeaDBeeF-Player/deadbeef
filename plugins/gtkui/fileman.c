#include "../../deadbeef.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <ctype.h>
#include "gtkui.h"
#include "ddblistview.h"
#include "progress.h"
#include "support.h"

static gboolean
progress_show_idle (gpointer data) {
    progress_show ();
    return FALSE;
}

static gboolean
set_progress_text_idle (gpointer data) {
    const char *text = (const char *)data;
    progress_settext (text);
    return FALSE;
}

int
gtkpl_add_file_info_cb (DB_playItem_t *it, void *data) {
    if (progress_is_aborted ()) {
        return -1;
    }
    g_idle_add (set_progress_text_idle, it->fname);
    return 0;
}

static gboolean
progress_hide_idle (gpointer data) {
    progress_hide ();
    playlist_refresh ();
    return FALSE;
}

void
gtkpl_add_dir (DdbListview *ps, char *folder) {
    g_idle_add (progress_show_idle, NULL);
    deadbeef->pl_add_dir (folder, gtkpl_add_file_info_cb, NULL);
    g_free (folder);
    g_idle_add (progress_hide_idle, NULL);
}

static void
gtkpl_adddir_cb (gpointer data, gpointer userdata) {
    deadbeef->pl_add_dir (data, gtkpl_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_dirs (GSList *lst) {
    g_idle_add (progress_show_idle, NULL);
    g_slist_foreach(lst, gtkpl_adddir_cb, NULL);
    g_slist_free (lst);
    g_idle_add (progress_hide_idle, NULL);
}

static void
gtkpl_addfile_cb (gpointer data, gpointer userdata) {
    deadbeef->pl_add_file (data, gtkpl_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_files (GSList *lst) {
    g_idle_add (progress_show_idle, NULL);
    g_slist_foreach(lst, gtkpl_addfile_cb, NULL);
    g_slist_free (lst);
    g_idle_add (progress_hide_idle, NULL);
}

static void
add_dirs_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_dirs (lst);
}

void
gtkui_add_dirs (GSList *lst) {
    deadbeef->thread_start (add_dirs_worker, lst);
}

static void
add_files_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_files (lst);
}

void
gtkui_add_files (struct _GSList *lst) {
    deadbeef->thread_start (add_files_worker, lst);
}

static void
open_files_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_files (lst);
    extern GtkWidget *mainwin;
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_set_cursor (pl, 0);
    deadbeef->sendmessage (M_PLAYSONG, 0, 0, 0);
}

void
gtkui_open_files (struct _GSList *lst) {
    deadbeef->pl_clear ();
    deadbeef->thread_start (open_files_worker, lst);
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
    int cursor = deadbeef->pl_get_idx_of (DB_PLAYITEM (data));
    ddb_listview_set_cursor (listview, cursor);
    return FALSE;
}

void
gtkpl_add_fm_dropped_files (DB_playItem_t *drop_before, char *ptr, int length) {
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    g_idle_add (progress_show_idle, NULL);

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
            DdbListviewIter inserted = deadbeef->pl_insert_dir (after, fname, &abort, gtkpl_add_file_info_cb, NULL);
            if (!inserted && !abort) {
                inserted = deadbeef->pl_insert_file (after, fname, &abort, gtkpl_add_file_info_cb, NULL);
            }
            if (inserted) {
                if (!first) {
                    first = inserted;
                }
                if (after) {
                    deadbeef->pl_item_unref (after);
                }
                after = inserted;
            }
        }
        p = pe;
        // skip whitespace
        while (*p && *p <= ' ') {
            p++;
        }
    }
    free (ptr);

    if (after) {
        deadbeef->pl_item_unref (after);
    }
    g_idle_add (progress_hide_idle, NULL);
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
    deadbeef->thread_start (fmdrop_worker, data);
}
