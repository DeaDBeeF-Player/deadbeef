#include "../../deadbeef.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <ctype.h>
#include "gtkui.h"
#include "ddblistview.h"
#include "progress.h"

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
    gtkpl_set_cursor (PL_MAIN, 0);
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

void
gtkpl_add_fm_dropped_files (char *ptr, int length, int drop_y) {
    // FIXME: port
#if 0
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    g_idle_add (progress_show_idle, NULL);

//    int drop_row = drop_y / rowheight + ddb_get_vscroll_pos (pl);
    DdbListviewIter iter = ddb_listview_get_iter_from_coord (0, drop_y);
    drop_before = ((DdbListviewIter )iter);
    int drop_row = deadbeef->pl_get_idx_of (drop_before);
//    DdbListviewIter drop_before = deadbeef->pl_get_for_idx_and_iter (drop_row, PL_MAIN);
    DdbListviewIter after = NULL;
    if (drop_before) {
        after = PL_PREV (drop_before, PL_MAIN);
        UNREF (drop_before);
        drop_before = NULL;
    }
    else {
        after = PL_TAIL (ps->iterator);
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
                if (after) {
                    UNREF (after);
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
        UNREF (after);
    }
    g_idle_add (progress_hide_idle, NULL);
#endif
}

struct fmdrop_data {
    char *mem;
    int length;
    int drop_y;
};

static void
fmdrop_worker (void *ctx) {
    struct fmdrop_data *data = (struct fmdrop_data *)ctx;
    gtkpl_add_fm_dropped_files (data->mem, data->length, data->drop_y);
    free (data);
}

void
gtkui_receive_fm_drop (char *mem, int length, int drop_y) {
    struct fmdrop_data *data = malloc (sizeof (struct fmdrop_data));
    if (!data) {
        fprintf (stderr, "gtkui_receive_fm_drop: malloc failed\n");
        return;
    }
    data->mem = mem;
    data->length = length;
    data->drop_y = drop_y;
    deadbeef->thread_start (fmdrop_worker, data);
}
