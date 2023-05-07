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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <deadbeef/deadbeef.h>
#include "../../gettext.h"

#include "callbacks.h"
#include "gtkui.h"
#include "interface.h"
#include "playlist/ddblistview.h"
#include "playlist/mainplaylist.h"
#include "playlist/playlistcontroller.h"
#include "playlist/playlistrenderer.h"
#include "playlist/plcommon.h"
#include "plmenu.h"
#include "search.h"
#include "support.h"
#include "wingeom.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static GtkWidget *searchwin;
static playlist_controller_t *_playlist_controller;
static int refresh_source_id = 0;
static char *window_title_bytecode = NULL;

static DdbListview *
playlist_visible (void) {
    if (searchwin) {
        GdkWindow *window = gtk_widget_get_window(searchwin);
        if (window) {
            if (!(gdk_window_get_state(window) & GDK_WINDOW_STATE_ICONIFIED) && gtk_widget_get_visible(searchwin)) {
                return DDB_LISTVIEW(lookup_widget(searchwin, "searchlist"));
            }
        }
    }
    return NULL;
}

static void
search_process (DdbListview *listview, ddb_playlist_t *plt) {
    GtkEntry *entry = GTK_ENTRY(lookup_widget(searchwin, "searchentry"));
    const gchar *text = gtk_entry_get_text(entry);
    deadbeef->plt_search_process2 (plt, text, 0);
    ddb_listview_col_sort_update (listview);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SEARCHRESULT, 0);

    int row = deadbeef->pl_get_cursor (PL_SEARCH);
    if (row >= deadbeef->pl_getcount (PL_SEARCH)) {
        deadbeef->pl_set_cursor (PL_SEARCH, deadbeef->pl_getcount (PL_SEARCH) - 1);
    }
    ddb_listview_list_setup (listview, ddb_listview_get_scroll_pos(listview));
    ddb_listview_refresh (listview, DDB_REFRESH_LIST);

    char title[1024] = "";
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .plt = plt,
        .iter = PL_SEARCH
    };
    deadbeef->tf_eval (&ctx, window_title_bytecode, title, sizeof (title));

    gtk_window_set_title (GTK_WINDOW (searchwin), title);

}

static gboolean
search_start_cb (gpointer p) {
    GtkWidget *entry = lookup_widget (searchwin, "searchentry");
    if (!playlist_visible ()) {
        DdbListview *listview = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
        refresh_source_id = 0;
        ddb_listview_clear_sort (listview);
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_search_reset (plt);
            deadbeef->plt_deselect_all (plt);
            deadbeef->plt_unref (plt);
        }
        wingeom_restore (searchwin, "searchwin", -1, -1, 450, 150, 0);
        gtk_widget_show (searchwin);
        gtk_entry_set_text (GTK_ENTRY (entry), "");
        ddb_listview_refresh (listview, DDB_REFRESH_CONFIG);
    }
    gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
    gtk_widget_grab_focus (entry);
    gtk_window_present (GTK_WINDOW (searchwin));
    return FALSE;
}

void
search_start (void) {
    if (searchwin) {
        g_idle_add (search_start_cb, NULL);
    }
}

void
search_destroy (void) {
    if (searchwin) {
        ddb_listview_size_columns_without_scrollbar (DDB_LISTVIEW (lookup_widget (searchwin, "searchlist")));
        gtk_widget_destroy (searchwin);
        searchwin = NULL;

        playlist_controller_free(_playlist_controller);
        _playlist_controller = NULL;
    }
    if (window_title_bytecode) {
        deadbeef->tf_free (window_title_bytecode);
        window_title_bytecode = NULL;
    }
}

static DB_playItem_t *
next_playitem (DB_playItem_t *it) {
    DB_playItem_t *next = deadbeef->pl_get_next (it, PL_SEARCH);
    deadbeef->pl_item_unref (it);
    return next;
}

static gboolean
paused_cb (gpointer p) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe();
    if (it) {
        int idx = deadbeef->pl_get_idx_of_iter(it, PL_SEARCH);
        if (idx != -1) {
            ddb_listview_draw_row(DDB_LISTVIEW(p), idx, (DdbListviewIter)it);
        }
        deadbeef->pl_item_unref(it);
    }
    return FALSE;
}

static gboolean
row_redraw_cb (gpointer p) {
    DB_playItem_t *it = (DB_playItem_t *)p;
    DdbListview *listview = playlist_visible();
    if (listview) {
        int idx = deadbeef->pl_get_idx_of_iter(it, PL_SEARCH);
        if (idx != -1) {
            ddb_listview_draw_row(listview, idx, it);
        }
    }
    deadbeef->pl_item_unref(it);
    return FALSE;
}

static gboolean
list_redraw_cb (gpointer p) {
    ddb_listview_refresh (DDB_LISTVIEW(p), DDB_REFRESH_LIST);
    return FALSE;
}

static gboolean
header_redraw_cb (gpointer p) {
    ddb_listview_refresh (DDB_LISTVIEW(p), DDB_REFRESH_COLUMNS);
    return FALSE;
}

static gboolean
songstarted_cb (gpointer p) {
    DB_playItem_t *it = p;
    DdbListview *listview = playlist_visible();
    if (listview) {
        int idx = deadbeef->pl_get_idx_of_iter (it, PL_SEARCH);
        if (idx != -1) {
            if (!gtkui_listview_busy) {
                if (deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)) {
                    ddb_listview_select_single (listview, idx);
                    deadbeef->pl_set_cursor (PL_SEARCH, idx);
                }
                if (deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)) {
                    ddb_listview_scroll_to (listview, idx);
                }
            }
            ddb_listview_draw_row (listview, idx, it);
        }
    }
    deadbeef->pl_item_unref (it);
    return FALSE;
}

static void
set_cursor (DdbListview *listview, DB_playItem_t *it) {
    int new_cursor = deadbeef->pl_get_idx_of_iter (it, PL_SEARCH);
    if (new_cursor != -1) {
        int cursor = deadbeef->pl_get_cursor (PL_SEARCH);
        if (new_cursor != cursor) {
            deadbeef->pl_set_cursor (PL_SEARCH, new_cursor);
            ddb_listview_draw_row (listview, new_cursor, NULL);
            if (cursor != -1) {
                ddb_listview_draw_row (listview, cursor, NULL);
            }
        }
        ddb_listview_scroll_to (listview, new_cursor);
    }
}

static gboolean
cursor_moved_cb (gpointer p) {
    DdbListview *listview = playlist_visible();
    if (listview) {
        set_cursor (listview, p);
    }
    deadbeef->pl_item_unref (p);
    return FALSE;
}

static gboolean
focus_selection_cb (gpointer data) {
    DdbListview *listview = playlist_visible();
    if (listview) {
        deadbeef->pl_lock ();
        DB_playItem_t *it = deadbeef->pl_get_first (PL_SEARCH);
        while (it && !deadbeef->pl_is_selected (it)) {
            it = next_playitem (it);
        }
        if (it) {
            set_cursor (listview, it);
            deadbeef->pl_item_unref (it);
        }
        deadbeef->pl_unlock ();
    }
    return FALSE;
}

static gboolean
trackfocus_cb (gpointer p) {
    DdbListview *listview = playlist_visible();
    if (listview) {
        DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
        if (it) {
            deadbeef->pl_lock ();
            int idx = deadbeef->pl_get_idx_of_iter (it, PL_SEARCH);
            if (idx != -1) {
                ddb_listview_select_single (listview, idx);
                deadbeef->pl_set_cursor (PL_SEARCH, idx);
                ddb_listview_scroll_to (listview, idx);
            }
            deadbeef->pl_unlock ();
            deadbeef->pl_item_unref (it);
        }
    }
    return FALSE;
}

static gboolean
configchanged_cb (gpointer p) {
    ddb_listview_refresh (DDB_LISTVIEW(p), DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_CONFIG);
    return FALSE;
}

static gboolean
refresh_cb (gpointer p) {
    refresh_source_id = 0;
    DdbListview *listview = playlist_visible();
    if (listview) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            search_process (listview, plt);
            deadbeef->plt_unref (plt);
        }
    }
    return FALSE;
}

void
search_submit_refresh (void) {
    if (refresh_source_id == 0) {
        refresh_source_id = g_idle_add (refresh_cb, NULL);
    }
}

int
search_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    DdbListview *listview = playlist_visible();
    if (!listview) {
        return 0;
    }

    switch (id) {
        case DB_EV_PAUSED:
            g_idle_add(paused_cb, listview);
            break;
        case DB_EV_SONGFINISHED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                deadbeef->pl_item_ref(ev->track);
                g_idle_add(row_redraw_cb, ev->track);
            }
            break;
        }
        case DB_EV_SONGSTARTED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                deadbeef->pl_item_ref(ev->track);
                g_idle_add(songstarted_cb, ev->track);
            }
            break;
        }
        case DB_EV_TRACKINFOCHANGED:
            if ((p1 == DDB_PLAYLIST_CHANGE_SELECTION && p2 != PL_SEARCH) || p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
                ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
                if (ev->track) {
                    deadbeef->pl_item_ref (ev->track);
                    g_idle_add(row_redraw_cb, ev->track);
                }
            }
            else if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
                search_submit_refresh();
            }
            break;
        case DB_EV_PLAYLISTCHANGED:
            if ((p1 == DDB_PLAYLIST_CHANGE_SELECTION && p2 != PL_SEARCH) || p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
                g_idle_add(list_redraw_cb, listview);
            }
            else if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
                search_submit_refresh();
            }
            break;
        case DB_EV_PLAYLISTSWITCHED:
            search_submit_refresh();
            break;
        case DB_EV_FOCUS_SELECTION:
            g_idle_add (focus_selection_cb, NULL);
            break;
        case DB_EV_TRACKFOCUSCURRENT:
            g_idle_add (trackfocus_cb, NULL);
            break;
        case DB_EV_CURSOR_MOVED:
            if (p1 != PL_SEARCH) {
                ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
                if (ev->track) {
                    deadbeef->pl_item_ref (ev->track);
                    g_idle_add (cursor_moved_cb, ev->track);
                }
            }
            break;
        case DB_EV_CONFIGCHANGED:
            if (ctx) {
                char *conf_str = (char *)ctx;
                if (gtkui_listview_override_conf(conf_str) || gtkui_listview_font_conf(conf_str)) {
                    g_idle_add(configchanged_cb, listview);
                }
                else if (gtkui_listview_colors_conf(conf_str)) {
                    g_idle_add (list_redraw_cb, listview);
                    g_idle_add (header_redraw_cb, listview);
                }
                else if (gtkui_listview_font_style_conf(conf_str) || !strcmp (conf_str, "playlist.pin.groups")) {
                    g_idle_add (list_redraw_cb, listview);
                }
                else if (gtkui_tabstrip_override_conf(conf_str) || gtkui_tabstrip_colors_conf(conf_str)) {
                    g_idle_add (header_redraw_cb, listview);
                }
            }
            break;
    }
    return 0;
}

///////// searchwin playlist navigation and rendering

void
on_searchentry_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
    DdbListview *listview = playlist_visible();
    if (listview) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_deselect_all (plt);
            search_process (listview, plt);
            for (DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_SEARCH); it; it = next_playitem (it)) {
                deadbeef->pl_set_selected (it, 1);
            }
            deadbeef->plt_unref (plt);
        }
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
        DB_playItem_t *head = deadbeef->pl_get_first (PL_SEARCH);
        if (head) {
            ddb_event_track_t *event = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_CURSOR_MOVED);
            event->track = head;
            deadbeef->event_send ((ddb_event_t *)event, PL_SEARCH, 0);
        }
    }
}

void
on_searchentry_activate                (GtkEntry        *entry,
                                        gpointer         user_data)
{
    // Never happens, intercepted and passed to hotkeys
    return;
}

gboolean
on_searchwin_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (event->keyval == GDK_Escape) {
        gtk_widget_hide (searchwin);
        return TRUE;
    }
    if (event->keyval == GDK_Return || event->keyval == GDK_ISO_Enter || event->keyval == GDK_KP_Enter) {
        return on_mainwin_key_press_event (widget, event, user_data);
    }
    return FALSE;
}

gboolean
on_searchwin_configure_event           (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "searchwin");
    return FALSE;
}

gboolean
on_searchwin_window_state_event        (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "searchwin");
    return FALSE;
}

void
search_playlist_init (GtkWidget *mainwin) {
    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));
    window_title_bytecode = deadbeef->tf_compile (_("Search [(%list_total% results)]"));
    DdbListview *listview = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    _playlist_controller = playlist_controller_new(listview, TRUE);
}
