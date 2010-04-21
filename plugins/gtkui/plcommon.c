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
#include <unistd.h>
#include "gtkui.h"
#include "plcommon.h"
#include "coverart.h"
#include "drawing.h"
#include "trkproperties.h"
#include "mainplaylist.h"
#include "support.h"
#include "interface.h"
#include "parser.h"

#define min(x,y) ((x)<(y)?(x):(y))
//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

extern GtkWidget *theme_treeview;
extern GdkPixbuf *play16_pixbuf;
extern GdkPixbuf *pause16_pixbuf;
extern GdkPixbuf *buffering16_pixbuf;

void
write_column_config (const char *name, int idx, const char *title, int width, int align_right, int id, const char *format) {
    char key[128];
    char value[128];
    snprintf (key, sizeof (key), "%s.column.%d", name, idx);
    snprintf (value, sizeof (value), "\"%s\" \"%s\" %d %d %d", title, format ? format : "", id, width, align_right);
    deadbeef->conf_set_str (key, value);
}

void
rewrite_column_config (DdbListview *listview, const char *name) {
    char key[128];
    snprintf (key, sizeof (key), "%s.column.", name);
    deadbeef->conf_remove_items (key);

    int cnt = ddb_listview_column_get_count (listview);
    for (int i = 0; i < cnt; i++) {
        const char *title;
        int width;
        int align_right;
        col_info_t *info;
        int minheight;
        ddb_listview_column_get_info (listview, i, &title, &width, &align_right, &minheight, (void **)&info);
        write_column_config (name, i, title, width, align_right, info->id, info->format);
    }
}

#define ART_PADDING_HORZ 8
#define ART_PADDING_VERT 0

void draw_column_data (DdbListview *listview, GdkDrawable *drawable, DdbListviewIter it, DdbListviewIter group_it, int column, int group_y, int x, int y, int width, int height) {
    const char *ctitle;
    int cwidth;
    int calign_right;
    col_info_t *cinf;
    int minheight;
    int res = ddb_listview_column_get_info (listview, column, &ctitle, &cwidth, &calign_right, &minheight, (void **)&cinf);
    if (res == -1) {
        return;
    }
    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();
	int theming = !gtkui_override_listview_colors ();

    if (cinf->id == DB_COLUMN_ALBUM_ART) {
        if (theming) {
            gtk_paint_flat_box (theme_treeview->style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, theme_treeview, "cell_even_ruled", x, y, width, height);
        }
        else {
            GdkGC *gc = gdk_gc_new (drawable);
            GdkColor clr;
            gdk_gc_set_rgb_fg_color (gc, (gtkui_get_listview_even_row_color (&clr), &clr));
            gdk_draw_rectangle (drawable, gc, TRUE, x, y, width, height);
            g_object_unref (gc);
        }
        int art_width = width - ART_PADDING_HORZ * 2;
        int art_y = y; // dest y
        int art_h = height;
        int sy; // source y
        if (group_y < ART_PADDING_VERT) {
            art_y = y - group_y + ART_PADDING_VERT;
            art_h = height - (art_y - y);
            sy = group_y;
        }
        else {
            sy = group_y - ART_PADDING_VERT;
        }
        if (art_width > 0) {
            if (group_it) {
                int h = cwidth - group_y;
                h = min (height, art_h);
//                gdk_draw_rectangle (drawable, GTK_WIDGET (listview)->style->white_gc, TRUE, x, y, width, h);
                const char *album = deadbeef->pl_find_meta (group_it, "album");
                const char *artist = deadbeef->pl_find_meta (group_it, "artist");
                if (!album || !*album) {
                    album = deadbeef->pl_find_meta (group_it, "title");
                }
                GdkPixbuf *pixbuf = get_cover_art (((DB_playItem_t *)group_it)->fname, artist, album, art_width);
                if (pixbuf) {
                    int pw = gdk_pixbuf_get_width (pixbuf);
                    int ph = gdk_pixbuf_get_height (pixbuf);
                    if (sy < ph)
                    {
                        pw = min (art_width, pw);
                        ph -= sy;
                        ph = min (ph, h);
                        gdk_draw_pixbuf (drawable, GTK_WIDGET (listview)->style->white_gc, pixbuf, 0, sy, x + ART_PADDING_HORZ, art_y, pw, ph, GDK_RGB_DITHER_NONE, 0, 0);
//                        gdk_draw_rectangle (drawable, GTK_WIDGET (listview)->style->black_gc, FALSE, x + ART_PADDING_HORZ, art_y, pw, ph);
                    }
                    g_object_unref (pixbuf);
                }
            }
        }
    }
    else if (it && it == playing_track && cinf->id == DB_COLUMN_PLAYING) {
        int paused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED;
        int buffering = !deadbeef->streamer_ok_to_read (-1);
        GdkPixbuf *pixbuf;
        if (paused) {
            pixbuf = pause16_pixbuf;
        }
        else if (!buffering) {
            pixbuf = play16_pixbuf;
        }
        else {
            pixbuf = buffering16_pixbuf;
        }
        gdk_pixbuf_render_to_drawable (pixbuf, drawable, GTK_WIDGET (listview)->style->black_gc, 0, 0, x + cwidth/2 - 8, y + height/2 - 8, 16, 16, GDK_RGB_DITHER_NONE, 0, 0);
    }
    else if (it) {
        char text[1024];
        deadbeef->pl_format_title (it, -1, text, sizeof (text), cinf->id, cinf->format);
        GdkColor *color = NULL;
        if (theming) {
            if (deadbeef->pl_is_selected (it)) {
                color = &theme_treeview->style->text[GTK_STATE_SELECTED];
            }
            else {
                color = &theme_treeview->style->text[GTK_STATE_NORMAL];
            }
        }
        else {
            GdkColor clr;
            if (deadbeef->pl_is_selected (it)) {
                color = (gtkui_get_listview_selected_text_color (&clr), &clr);
            }
            else {
                color = (gtkui_get_listview_text_color (&clr), &clr);
            }
        }
        float fg[3] = {(float)color->red/0xffff, (float)color->green/0xffff, (float)color->blue/0xffff};
        draw_set_fg_color (fg);

        draw_init_font (GTK_WIDGET (listview)->style);
        if (calign_right) {
            draw_text (x+5, y + height/2 - draw_get_font_size ()/2 - 2, cwidth-10, 1, text);
        }
        else {
            draw_text (x + 5, y + height/2 - draw_get_font_size ()/2 - 2, cwidth-10, 0, text);
        }
    }
    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }
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
main_reload_metadata_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (menuitem), "ps"));
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it) && deadbeef->is_local_file (it->fname) && it->decoder_id) {
            DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, it->decoder_id)) {
                    if (decoders[i]->read_metadata) {
                        decoders[i]->read_metadata (it);
                    }
                    break;
                }
            }
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    playlist_refresh ();
}

int clicked_idx = -1;

void
main_properties_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (clicked_idx, PL_MAIN);
    if (!it) {
        fprintf (stderr, "attempt to view properties of non-existing item\n");
        return;
    }
    show_track_properties_dlg (it);
    deadbeef->pl_item_unref (it);
}

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
on_remove_from_disk_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *widget = GTK_WIDGET (menuitem);

    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "Delete files from disk");
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), "Files will be lost. Proceed?");
    gtk_window_set_title (GTK_WINDOW (dlg), "Warning");

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    if (response != GTK_RESPONSE_YES) {
        return;
    }

    deadbeef->pl_lock ();
    deadbeef->plt_lock ();

    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it) && deadbeef->is_local_file (it->fname)) {
            unlink (it->fname);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    int cursor = deadbeef->pl_delete_selected ();
    deadbeef->plt_unlock ();
    deadbeef->pl_unlock ();

    main_refresh ();
    search_refresh ();
}

void
list_context_menu (DdbListview *listview, DdbListviewIter it, int idx) {
    clicked_idx = deadbeef->pl_get_idx_of (it);
    int inqueue = deadbeef->pl_playqueue_test (it);
    GtkWidget *playlist_menu;
    GtkWidget *add_to_playback_queue1;
    GtkWidget *remove_from_playback_queue1;
    GtkWidget *separator9;
    GtkWidget *remove2;
    GtkWidget *remove_from_disk;
    GtkWidget *separator8;
    GtkWidget *properties1;
    GtkWidget *reload_metadata;

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

    reload_metadata = gtk_menu_item_new_with_mnemonic ("Reload metadata");
    gtk_widget_show (reload_metadata);
    gtk_container_add (GTK_CONTAINER (playlist_menu), reload_metadata);
    gtk_object_set_data (GTK_OBJECT (reload_metadata), "ps", listview);


    separator9 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator9);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator9);
    gtk_widget_set_sensitive (separator9, FALSE);

    remove2 = gtk_menu_item_new_with_mnemonic ("Remove");
    gtk_widget_show (remove2);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);
    gtk_object_set_data (GTK_OBJECT (remove2), "ps", listview);

    remove_from_disk = gtk_menu_item_new_with_mnemonic ("Remove from disk");
    gtk_widget_show (remove_from_disk);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_disk);
    gtk_object_set_data (GTK_OBJECT (remove_from_disk), "ps", listview);

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
    g_signal_connect ((gpointer) reload_metadata, "activate",
            G_CALLBACK (main_reload_metadata_activate),
            NULL);
    g_signal_connect ((gpointer) remove2, "activate",
            G_CALLBACK (on_remove2_activate),
            NULL);
    g_signal_connect ((gpointer) remove_from_disk, "activate",
            G_CALLBACK (on_remove_from_disk_activate),
            NULL);
    g_signal_connect ((gpointer) properties1, "activate",
            G_CALLBACK (main_properties_activate),
            NULL);
    gtk_menu_popup (GTK_MENU (playlist_menu), NULL, NULL, NULL, listview, 0, gtk_get_current_event_time());
}

extern const char *group_by_str;

void
on_group_by_none_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    group_by_str = "";
    deadbeef->conf_set_str ("playlist.group_by", group_by_str);
    main_refresh ();
}

void
on_group_by_artist_date_album_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    group_by_str = "%a - [%y] %b";
    deadbeef->conf_set_str ("playlist.group_by", group_by_str);
    main_refresh ();
}

void
on_group_by_artist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    group_by_str = "%a";
    deadbeef->conf_set_str ("playlist.group_by", group_by_str);
    main_refresh ();
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
    ddb_listview_column_append (listview, title, width, align, id == DB_COLUMN_ALBUM_ART ? width : 0, inf);
}

void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
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
        ddb_listview_column_insert (last_playlist, active_column, title, 100, align, id == DB_COLUMN_ALBUM_ART ? 100 : 0, inf);
        ddb_listview_refresh (last_playlist, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
    }
    gtk_widget_destroy (dlg);
}


void
on_edit_column_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (active_column == -1)
        return;
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), "Edit column");

    const char *title;
    int width;
    int align_right;
    col_info_t *inf;
    int minheight;
    int res = ddb_listview_column_get_info (last_playlist, active_column, &title, &width, &align_right, &minheight, (void **)&inf);
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
        ddb_listview_column_set_info (last_playlist, active_column, title, width, align, id == DB_COLUMN_ALBUM_ART ? width : 0, inf);

        ddb_listview_refresh (last_playlist, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
    }
    gtk_widget_destroy (dlg);
}


void
on_remove_column_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (active_column == -1)
        return;

    ddb_listview_column_remove (last_playlist, active_column);
    ddb_listview_refresh (last_playlist, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
}

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

  g_signal_connect ((gpointer) add_column, "activate",
                    G_CALLBACK (on_add_column_activate),
                    NULL);
  g_signal_connect ((gpointer) edit_column, "activate",
                    G_CALLBACK (on_edit_column_activate),
                    NULL);
  g_signal_connect ((gpointer) remove_column, "activate",
                    G_CALLBACK (on_remove_column_activate),
                    NULL);

  g_signal_connect ((gpointer) none, "activate",
                    G_CALLBACK (on_group_by_none_activate),
                    NULL);

  g_signal_connect ((gpointer) artist_date_album, "activate",
                    G_CALLBACK (on_group_by_artist_date_album_activate),
                    NULL);

  g_signal_connect ((gpointer) artist, "activate",
                    G_CALLBACK (on_group_by_artist_activate),
                    NULL);

  return headermenu;
}

void
header_context_menu (DdbListview *ps, int column) {
    GtkWidget *menu = create_headermenu ();
    last_playlist = ps;
    active_column = column;
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, ps, 3, gtk_get_current_event_time());
}

void
add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, int align_right) {
    if (!format) {
        format = "";
    }
    col_info_t *inf = malloc (sizeof (col_info_t));
    memset (inf, 0, sizeof (col_info_t));
    inf->id = id;
    inf->format = strdup (format);
    ddb_listview_column_append (listview, title, width, align_right, id == DB_COLUMN_ALBUM_ART ? width : 0, inf);
}

