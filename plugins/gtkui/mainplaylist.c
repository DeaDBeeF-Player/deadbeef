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
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "gtkui.h"
#include "ddblistview.h"
#include "mainplaylist.h"
#include "interface.h"
#include "support.h"
#include "drawing.h"
#include "trkproperties.h"

#pragma GCC optimize("O0")

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static uintptr_t play16_pixbuf;
static uintptr_t pause16_pixbuf;
static uintptr_t buffering16_pixbuf;

static int
main_get_count (void) {
    return deadbeef->pl_getcount (PL_MAIN);
}

static int
main_get_sel_count (void) {
    return deadbeef->pl_getselcount ();
}

static int
main_get_cursor (void) {
    return deadbeef->pl_get_cursor (PL_MAIN);
}

static void
main_set_cursor (int cursor) {
    return deadbeef->pl_set_cursor (PL_MAIN, cursor);
}

static DdbListviewIter main_head (void) {
    return (DdbListviewIter)deadbeef->pl_get_first (PL_MAIN);
}

static DdbListviewIter main_tail (void) {
    return (DdbListviewIter)deadbeef->pl_get_last(PL_MAIN);
}

static DdbListviewIter main_next (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_next(it, PL_MAIN);
}

static DdbListviewIter main_prev (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_prev(it, PL_MAIN);
}

static DdbListviewIter main_get_for_idx (int idx) {
    return deadbeef->pl_get_for_idx_and_iter (idx, PL_MAIN);
}

static int main_get_idx (DdbListviewIter it) {
    DB_playItem_t *c = deadbeef->pl_get_first (PL_MAIN);
    int idx = 0;
    while (c && c != it) {
        DB_playItem_t *next = deadbeef->pl_get_next (c, PL_MAIN); 
        deadbeef->pl_item_unref (c);
        c = next;
        idx++;
    }
    if (!c) {
        return -1;
    }
    deadbeef->pl_item_unref (c);
    return idx;
}

void main_drag_n_drop (DdbListviewIter before, uint32_t *indices, int length) {
    deadbeef->pl_move_items (PL_MAIN, (DB_playItem_t *)before, indices, length);
}

void main_external_drag_n_drop (DdbListviewIter before, char *mem, int length) {
    gtkui_receive_fm_drop ((DB_playItem_t *)before, mem, length);
}

gboolean
playlist_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer unused)
{
    GtkWidget *pl = lookup_widget (mainwin, "playlist");
    DB_playItem_t *item = (DB_playItem_t *)ddb_listview_get_iter_from_coord (DDB_LISTVIEW (pl), 0, y);
    if (item && item->fname) {
        gtk_tooltip_set_text (tooltip, item->fname);
        return TRUE;
    }
    return FALSE;
}

// columns

typedef struct {
    int id;
    char *format;
} col_info_t;

void
main_col_sort (int col, int sort_order, void *user_data) {
    col_info_t *c = (col_info_t*)user_data;
    deadbeef->pl_sort (PL_MAIN, c->id, c->format, sort_order-1);
}

static DdbListview *last_playlist;
static int active_column;

void
append_column_from_textdef (DdbListview *listview, const uint8_t *def) {
    // syntax: "title" "format" id width alignright
    char token[MAX_TOKEN];
    const char *p = def;
    char title[MAX_TOKEN];
    int id;
    char fmt[MAX_TOKEN];
    int width;
    int align;

    parser_init ();

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    strcpy (title, token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    strcpy (fmt, token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    id = atoi (token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    width = atoi (token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    align = atoi (token);

    col_info_t *inf = malloc (sizeof (col_info_t));
    memset (inf, 0, sizeof (col_info_t));
    inf->format = strdup (fmt);
    inf->id = id;
    ddb_listview_column_append (listview, title, width, align, inf);
}

#if 0
void
gtkpl_column_update_config (const char *title, gtkpl_column_t * c, int idx) {
    char key[128];
    char value[128];
    snprintf (key, sizeof (key), "%s.column.%d", title, idx);
    snprintf (value, sizeof (value), "\"%s\" \"%s\" %d %d %d", c->title, c->format ? c->format : "", c->id, c->width, c->align_right);
    deadbeef->conf_set_str (key, value);
}

void
gtkpl_column_rewrite_config (gtkpl_column_t *head, const char *title) {
    char key[128];
    snprintf (key, sizeof (key), "%s.column.", title);
    deadbeef->conf_remove_items (key);

    gtkpl_column_t * c;
    int i = 0;
    for (c = head; c; c = c->next, i++) {
        gtkpl_column_update_config (title, c, i);
    }
}
#endif
void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_window_set_title (GTK_WINDOW (dlg), "Add column");
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), 0);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        if (id >= DB_COLUMN_ID_MAX) {
            id = -1;
        }
        col_info_t *inf = malloc (sizeof (col_info_t));
        memset (inf, 0, sizeof (col_info_t));
        inf->format = strdup (format);
        inf->id = id;
        ddb_listview_column_insert (last_playlist, active_column, title, 100, align, inf);
        ddb_listview_refresh (last_playlist, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
    }
    gtk_widget_destroy (dlg);
}


void
on_edit_column_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (!active_column)
        return;
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_window_set_title (GTK_WINDOW (dlg), "Edit column");

    const char *title;
    int width;
    int align_right;
    col_info_t *inf;
    int res = ddb_listview_column_get_info (last_playlist, active_column, &title, &width, &align_right, (void **)&inf);
    if (res == -1) {
        trace ("attempted to edit non-existing column\n");
        return;
    }

    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), title);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "format")), inf->format);
    if (inf->id == -1) {
        gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), DB_COLUMN_ID_MAX);
    }
    else {
        gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), inf->id);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), align_right);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        if (id >= DB_COLUMN_ID_MAX) {
            id = -1;
        }
        free (inf->format);
        inf->format = strdup (format);
        inf->id = id;
        ddb_listview_column_set_info (last_playlist, active_column, title, width, align, inf);

        ddb_listview_refresh (last_playlist, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
    }
    gtk_widget_destroy (dlg);
}


void
on_remove_column_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (!active_column)
        return;

    ddb_listview_column_remove (last_playlist, active_column);
    ddb_listview_refresh (last_playlist, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
}

void main_handle_doubleclick (DdbListview *listview, DdbListviewIter iter, int idx) {
    deadbeef->sendmessage (M_PLAYSONGNUM, 0, idx, 0);
}

void main_selection_changed (DdbListviewIter it, int idx) {
    DdbListview *search = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    ddb_listview_draw_row (search, idx, it);
}

void main_draw_column_data (DdbListview *listview, GdkDrawable *drawable, DdbListviewIter it, int column, int x, int y, int width, int height) {
    const char *ctitle;
    int cwidth;
    int calign_right;
    col_info_t *cinf;
    int res = ddb_listview_column_get_info (listview, column, &ctitle, &cwidth, &calign_right, (void **)&cinf);
    if (res == -1) {
        return;
    }
    if (it == deadbeef->streamer_get_playing_track () && cinf->id == DB_COLUMN_PLAYING) {
        int paused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED;
        int buffering = !deadbeef->streamer_ok_to_read (-1);
        uintptr_t pixbuf;
        if (paused) {
            pixbuf = pause16_pixbuf;
        }
        else if (!buffering) {
            pixbuf = play16_pixbuf;
        }
        else {
            pixbuf = buffering16_pixbuf;
        }
        draw_pixbuf ((uintptr_t)drawable, pixbuf, x + cwidth/2 - 8, y + height/2 - 8, 0, 0, 16, 16);
    }
    else {
        char text[1024];
        deadbeef->pl_format_title (it, -1, text, sizeof (text), cinf->id, cinf->format);

        if (calign_right) {
            draw_text (x+5, y + height/2 - draw_get_font_size ()/2 - 2, cwidth-10, 1, text);
        }
        else {
            draw_text (x + 5, y + height/2 - draw_get_font_size ()/2 - 2, cwidth-10, 0, text);
        }
    }
}

void main_draw_group_title (DdbListview *listview, GdkDrawable *drawable, DdbListviewIter it, int x, int y, int width, int height) {
    char str[1024];
    deadbeef->pl_format_title ((DB_playItem_t *)it, -1, str, sizeof (str), -1, "%a - [%y] %b");
    float clr[] = {0, 0.1, 0.5};
    draw_set_fg_color (clr);
    draw_text (x + 5, y + height/2 - draw_get_font_size ()/2 - 2, width-10, 0, str);
}

#if 0
void
on_group_by_none_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->plt_group_by (NULL);
    main_refresh ();
}

void
on_group_by_artist_date_album_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->plt_group_by ("%a - [%y] %b");
    main_refresh ();
}

void
on_group_by_artist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->plt_group_by ("%a");
    main_refresh ();
}
#endif

GtkWidget*
create_headermenu (void)
{
  GtkWidget *headermenu;
  GtkWidget *add_column;
  GtkWidget *edit_column;
  GtkWidget *remove_column;
  GtkWidget *separator;
  GtkWidget *group_by;
  GtkWidget *group_by_menu;
  GtkWidget *none;
  GtkWidget *artist_date_album;
  GtkWidget *artist;

  headermenu = gtk_menu_new ();

  add_column = gtk_menu_item_new_with_mnemonic ("Add column");
  gtk_widget_show (add_column);
  gtk_container_add (GTK_CONTAINER (headermenu), add_column);

  edit_column = gtk_menu_item_new_with_mnemonic ("Edit column");
  gtk_widget_show (edit_column);
  gtk_container_add (GTK_CONTAINER (headermenu), edit_column);

  remove_column = gtk_menu_item_new_with_mnemonic ("Remove column");
  gtk_widget_show (remove_column);
  gtk_container_add (GTK_CONTAINER (headermenu), remove_column);

#if 0
  separator = gtk_separator_menu_item_new ();
  gtk_widget_show (separator);
  gtk_container_add (GTK_CONTAINER (headermenu), separator);
  gtk_widget_set_sensitive (separator, FALSE);

  group_by = gtk_menu_item_new_with_mnemonic ("Group by");
  gtk_widget_show (group_by);
  gtk_container_add (GTK_CONTAINER (headermenu), group_by);

  group_by_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (group_by), group_by_menu);

  none = gtk_menu_item_new_with_mnemonic ("None");
  gtk_widget_show (none);
  gtk_container_add (GTK_CONTAINER (group_by_menu), none);

  artist_date_album = gtk_menu_item_new_with_mnemonic ("Artist/Date/Album");
  gtk_widget_show (artist_date_album);
  gtk_container_add (GTK_CONTAINER (group_by_menu), artist_date_album);

  artist = gtk_menu_item_new_with_mnemonic ("Artist");
  gtk_widget_show (artist);
  gtk_container_add (GTK_CONTAINER (group_by_menu), artist);
#endif

  g_signal_connect ((gpointer) add_column, "activate",
                    G_CALLBACK (on_add_column_activate),
                    NULL);
  g_signal_connect ((gpointer) edit_column, "activate",
                    G_CALLBACK (on_edit_column_activate),
                    NULL);
  g_signal_connect ((gpointer) remove_column, "activate",
                    G_CALLBACK (on_remove_column_activate),
                    NULL);

#if 0
  g_signal_connect ((gpointer) remove_column, "activate",
                    G_CALLBACK (on_group_by_none_activate),
                    NULL);

  g_signal_connect ((gpointer) remove_column, "activate",
                    G_CALLBACK (on_group_by_artist_date_album_activate),
                    NULL);

  g_signal_connect ((gpointer) remove_column, "activate",
                    G_CALLBACK (on_group_by_artist_activate),
                    NULL);
#endif

  return headermenu;
}

void
main_header_context_menu (DdbListview *ps, int column) {
    GtkWidget *menu = create_headermenu ();
    last_playlist = ps;
    active_column = column;
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, ps, 3, gtk_get_current_event_time());
}

void
main_add_to_playback_queue_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (menuitem), "ps"));
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_playqueue_push (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    playlist_refresh ();
}

void
main_remove_from_playback_queue_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (menuitem), "ps"));
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_playqueue_remove (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    playlist_refresh ();
}

void
main_properties_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (deadbeef->pl_get_cursor (PL_MAIN), PL_MAIN);
    if (!it) {
        fprintf (stderr, "attempt to view properties of non-existing item\n");
        return;
    }
    show_track_properties_dlg (it);
}

// FIXME: wrong place for these functions
void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_clear ();
    main_refresh ();
    search_refresh ();
}

void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int cursor = deadbeef->pl_delete_selected ();
    main_refresh ();
    search_refresh ();
}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    deadbeef->pl_crop_selected ();
    main_refresh ();
    search_refresh ();
}

void
on_remove2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *widget = GTK_WIDGET (menuitem);
    int cursor = deadbeef->pl_delete_selected ();
    main_refresh ();
    search_refresh ();
}

void
main_list_context_menu (DdbListview *listview, DdbListviewIter it, int idx) {
    int inqueue = deadbeef->pl_playqueue_test (it);
    GtkWidget *playlist_menu;
    GtkWidget *add_to_playback_queue1;
    GtkWidget *remove_from_playback_queue1;
    GtkWidget *separator9;
    GtkWidget *remove2;
    GtkWidget *separator8;
    GtkWidget *properties1;

    playlist_menu = gtk_menu_new ();
    add_to_playback_queue1 = gtk_menu_item_new_with_mnemonic ("Add to playback queue");
    gtk_widget_show (add_to_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), add_to_playback_queue1);
    gtk_object_set_data (GTK_OBJECT (add_to_playback_queue1), "ps", listview);

    remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic ("Remove from playback queue");
    if (inqueue == -1) {
        gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
    }
    gtk_widget_show (remove_from_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_playback_queue1);
    gtk_object_set_data (GTK_OBJECT (remove_from_playback_queue1), "ps", listview);

    separator9 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator9);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator9);
    gtk_widget_set_sensitive (separator9, FALSE);

    remove2 = gtk_menu_item_new_with_mnemonic ("Remove");
    gtk_widget_show (remove2);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);
    gtk_object_set_data (GTK_OBJECT (remove2), "ps", listview);

    separator8 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator8);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
    gtk_widget_set_sensitive (separator8, FALSE);

    properties1 = gtk_menu_item_new_with_mnemonic ("Properties");
    gtk_widget_show (properties1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), properties1);
    gtk_object_set_data (GTK_OBJECT (properties1), "ps", listview);

    g_signal_connect ((gpointer) add_to_playback_queue1, "activate",
            G_CALLBACK (main_add_to_playback_queue_activate),
            NULL);
    g_signal_connect ((gpointer) remove_from_playback_queue1, "activate",
            G_CALLBACK (main_remove_from_playback_queue_activate),
            NULL);
    g_signal_connect ((gpointer) remove2, "activate",
            G_CALLBACK (on_remove2_activate),
            NULL);
    g_signal_connect ((gpointer) properties1, "activate",
            G_CALLBACK (main_properties_activate),
            NULL);
    gtk_menu_popup (GTK_MENU (playlist_menu), NULL, NULL, NULL, listview, 0, gtk_get_current_event_time());
}

void
main_delete_selected (void) {
    deadbeef->pl_delete_selected ();
    main_refresh ();
    search_refresh ();
}

void
main_select (DdbListviewIter it, int sel) {
    deadbeef->pl_set_selected ((DB_playItem_t *)it, sel);
}

int
main_is_selected (DdbListviewIter it) {
    return deadbeef->pl_is_selected ((DB_playItem_t *)it);
}

void
main_get_group (DdbListviewIter it, char *str, int size) {
    deadbeef->pl_format_title ((DB_playItem_t *)it, -1, str, size, -1, "%a - [%y] %b");
}

DdbListviewBinding main_binding = {
    // rows
    .count = main_get_count,
    .sel_count = main_get_sel_count,

    .cursor = main_get_cursor,
    .set_cursor = main_set_cursor,

    .head = main_head,
    .tail = main_tail,
    .next = main_next,
    .prev = main_prev,

    .get_for_idx = main_get_for_idx,
    .get_idx = main_get_idx,

    .is_selected = main_is_selected,
    .select = main_select,

    .get_group = main_get_group,

    .drag_n_drop = main_drag_n_drop,
    .external_drag_n_drop = main_external_drag_n_drop,

    .draw_column_data = main_draw_column_data,
    .draw_group_title = main_draw_group_title,

    // columns
    .col_sort = main_col_sort,
#if 0
    .col_count = main_col_count,
    .col_first = main_col_first,
    .col_next = main_col_next,
    .col_get_title = main_col_get_title,
    .col_get_width = main_col_get_width,
    .col_get_justify = main_col_get_justify,
    .col_get_sort = main_col_get_sort,
    .col_move = main_col_move,
    .col_set_width = main_col_set_width,
    .col_set_sort = main_col_set_sort,
#endif

    // callbacks
    .handle_doubleclick = main_handle_doubleclick,
    .selection_changed = main_selection_changed,
    .header_context_menu = main_header_context_menu,
    .list_context_menu = main_list_context_menu,
    .delete_selected = main_delete_selected,
};

void
add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, int align_right) {
    if (!format) {
        format = "";
    }
    col_info_t *inf = malloc (sizeof (col_info_t));
    memset (inf, 0, sizeof (col_info_t));
    inf->id = id;
    inf->format = strdup (format);
    ddb_listview_column_append (listview, title, width, align_right, inf);
}

void
main_playlist_init (GtkWidget *widget) {
    play16_pixbuf = draw_load_pixbuf ("play_16.png");
    pause16_pixbuf = draw_load_pixbuf ("pause_16.png");
    buffering16_pixbuf = draw_load_pixbuf ("buffering_16.png");
    DdbListview *playlist = DDB_LISTVIEW(widget);
    main_binding.ref = (void (*) (DdbListviewIter))deadbeef->pl_item_ref;
    main_binding.unref = (void (*) (DdbListviewIter))deadbeef->pl_item_unref;
    main_binding.is_selected = (int (*) (DdbListviewIter))deadbeef->pl_is_selected;
    ddb_listview_set_binding (playlist, &main_binding);

    DB_conf_item_t *col = deadbeef->conf_find ("playlist.column.", NULL);
    if (!col) {
        DdbListview *listview = playlist;
        // create default set of columns
        add_column_helper (listview, "Playing", 50, DB_COLUMN_PLAYING, NULL, 0);
        add_column_helper (listview, "Artist / Album", 150, DB_COLUMN_ARTIST_ALBUM, NULL, 0);
        add_column_helper (listview, "Track №", 50, DB_COLUMN_TRACK, NULL, 1);
        add_column_helper (listview, "Title / Track Artist", 150, DB_COLUMN_TITLE, NULL, 0);
        add_column_helper (listview, "Duration", 50, DB_COLUMN_DURATION, NULL, 0);
    }
    else {
        while (col) {
            append_column_from_textdef (playlist, col->value);
            col = deadbeef->conf_find ("playlist.column.", col);
        }
    }

    // FIXME: filepath should be in properties dialog, while tooltip should be
    // used to show text that doesn't fit in column width
    if (deadbeef->conf_get_int ("playlist.showpathtooltip", 0)) {
        GValue value = {0, };
        g_value_init (&value, G_TYPE_BOOLEAN);
        g_value_set_boolean (&value, TRUE);
        g_object_set_property (G_OBJECT (widget), "has-tooltip", &value);
        g_signal_connect (G_OBJECT (widget), "query-tooltip", G_CALLBACK (playlist_tooltip_handler), NULL);
    }
}

void
main_playlist_free (void) {
}

void
main_refresh (void) {
    if (mainwin && GTK_WIDGET_VISIBLE (mainwin)) {
        DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
        ddb_listview_refresh (pl, DDB_REFRESH_VSCROLL | DDB_REFRESH_LIST | DDB_EXPOSE_LIST);
    }
}

