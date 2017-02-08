/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#include "../../deadbeef.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "gtkui.h"
#include "ddblistview.h"
#include "progress.h"
#include "support.h"

//void
//gtkpl_add_dir (DdbListview *ps, char *folder) {
//    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
//    gtkui_original_plt_add_dir (plt, folder, gtkui_add_file_info_cb, NULL);
//    deadbeef->plt_unref (plt);
//    g_free (folder);
//}

static void
gtkpl_adddir_cb (gpointer data, gpointer userdata) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_add_dir2 (0, plt, data, NULL, NULL);
    deadbeef->plt_unref (plt);
    g_free (data);
}

void
gtkpl_add_dirs (GSList *lst) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    int empty = 0 == deadbeef->plt_get_item_count (plt, PL_MAIN);
    if (deadbeef->plt_add_files_begin (plt, 0) < 0) {
        deadbeef->plt_unref (plt);
        g_slist_free (lst);
        return;
    }
    deadbeef->pl_lock ();
    if (g_slist_length (lst) == 1
            && deadbeef->conf_get_int ("gtkui.name_playlist_from_folder", 1)) {
        char t[1000];
        if (!deadbeef->plt_get_title (plt, t, sizeof (t))) {
            char *def = _("New Playlist");
            if (!strncmp (t, def, strlen (def)) || empty) {
                const char *folder = strrchr ((char*)lst->data, '/');
                if (!folder) {
                    folder = lst->data;
                }
                deadbeef->plt_set_title (plt, folder+1);
            }
        }
    }
    deadbeef->pl_unlock ();
    g_slist_foreach(lst, gtkpl_adddir_cb, NULL);
    g_slist_free (lst);
    deadbeef->plt_add_files_end (plt, 0);
    deadbeef->plt_unref (plt);
}

static void
gtkpl_addfile_cb (gpointer data, gpointer userdata) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_add_file2 (0, plt, data, NULL, 0);
    deadbeef->plt_unref (plt);
    g_free (data);
}

void
gtkpl_add_files (GSList *lst) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (deadbeef->plt_add_files_begin (plt, 0) < 0) {
        g_slist_free (lst);
        deadbeef->plt_unref (plt);
        return;
    }
    g_slist_foreach(lst, gtkpl_addfile_cb, NULL);
    g_slist_free (lst);
    deadbeef->plt_add_files_end (plt, 0);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->conf_save ();
}

static void
add_dirs_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_dirs (lst);
    deadbeef->pl_save_current ();
    deadbeef->conf_save ();
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
    intptr_t tid = deadbeef->thread_start (add_files_worker, lst);
    deadbeef->thread_detach (tid);
}

static void
open_files_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_files (lst);
    deadbeef->pl_save_current ();
    deadbeef->pl_set_cursor (PL_MAIN, 0);
    deadbeef->conf_save ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, 0, 0);
}

void
gtkui_open_files (struct _GSList *lst) {
    deadbeef->pl_clear ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

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
    if (!data) {
        deadbeef->pl_set_cursor (PL_MAIN, -1);
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        return FALSE;
    }
    int cursor = deadbeef->pl_get_idx_of (DB_PLAYITEM (data));
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return FALSE;
}

void
gtkpl_add_fm_dropped_files (DB_playItem_t *drop_before, char *ptr, int length) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (deadbeef->plt_add_files_begin (plt, 0) < 0) {
        free (ptr);
        deadbeef->plt_unref (plt);
        return;
    }

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
        while (*pe && *pe >= ' ') {
            pe++;
        }
        if (pe - p < 4096 && pe - p > 7) {
            char fname[(int)(pe - p)+1];
            strcopy_special (fname, p, pe-p);
            //strncpy (fname, p, pe - p);
            //fname[pe - p] = 0;
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
    free (ptr);

    deadbeef->plt_add_files_end (plt, 0);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
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
