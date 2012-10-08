/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <gdk/gdkkeysyms.h>
#include "gtkui.h"
#include "plcommon.h"
#include "coverart.h"
#include "drawing.h"
#include "trkproperties.h"
#include "mainplaylist.h"
#include "support.h"
#include "interface.h"
#include "parser.h"
#include "actions.h"
#include "search.h"

#define min(x,y) ((x)<(y)?(x):(y))
//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

char group_by_str[MAX_GROUP_BY_STR];

extern GtkWidget *theme_treeview;
extern GdkPixbuf *play16_pixbuf;
extern GdkPixbuf *pause16_pixbuf;
extern GdkPixbuf *buffering16_pixbuf;

static int clicked_idx = -1;

void
write_column_config (const char *name, int idx, const char *title, int width, int align_right, int id, const char *format) {
    char key[128];
    char value[128];
    snprintf (key, sizeof (key), "%s.column.%02d", name, idx);
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

void draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, DdbListviewIter group_it, int column, int group_y, int x, int y, int width, int height) {
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
#if GTK_CHECK_VERSION(3,0,0)
            cairo_rectangle (cr, x, y, width, height);
            cairo_clip (cr);
            gtk_paint_flat_box (gtk_widget_get_style (theme_treeview), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, theme_treeview, "cell_even_ruled", x-1, y, width+2, height);
            cairo_reset_clip (cr);
#else
            GdkRectangle clip = {
                .x = x,
                .y = y,
                .width = width,
                .height = height,
            };
            gtk_paint_flat_box (gtk_widget_get_style (theme_treeview), gtk_widget_get_window (listview->list), GTK_STATE_NORMAL, GTK_SHADOW_NONE, &clip, theme_treeview, "cell_even_ruled", x-1, y, width+2, height);
#endif
        }
        else {
            GdkColor clr;
            gtkui_get_listview_even_row_color (&clr);
            cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
            cairo_rectangle (cr, x, y, width, height);
            cairo_fill (cr);
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
                const char *album = deadbeef->pl_find_meta (group_it, "album");
                const char *artist = deadbeef->pl_find_meta (group_it, "artist");
                if (!album || !*album) {
                    album = deadbeef->pl_find_meta (group_it, "title");
                }
                GdkPixbuf *pixbuf = get_cover_art (deadbeef->pl_find_meta (((DB_playItem_t *)group_it), ":URI"), artist, album, art_width);
                if (pixbuf) {
                    int pw = gdk_pixbuf_get_width (pixbuf);
                    int ph = gdk_pixbuf_get_height (pixbuf);
                    if (sy < ph)
                    {
                        pw = min (art_width, pw);
                        ph -= sy;
                        ph = min (ph, h);
                        gdk_cairo_set_source_pixbuf (cr, pixbuf, (x + ART_PADDING_HORZ)-0, (art_y)-sy);
                        cairo_rectangle (cr, x + ART_PADDING_HORZ, art_y, pw, ph);
                        cairo_fill (cr);
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
        gdk_cairo_set_source_pixbuf (cr, pixbuf, x + cwidth/2 - 8, y + height/2 - 8);
        cairo_rectangle (cr, x + cwidth/2 - 8, y + height/2 - 8, 16, 16);
        cairo_fill (cr);
    }
    else if (it) {
        char text[1024];
        deadbeef->pl_format_title (it, -1, text, sizeof (text), cinf->id, cinf->format);
        GdkColor *color = NULL;
        if (theming) {
            if (deadbeef->pl_is_selected (it)) {
                color = &gtk_widget_get_style (theme_treeview)->text[GTK_STATE_SELECTED];
            }
            else {
                color = &gtk_widget_get_style (theme_treeview)->text[GTK_STATE_NORMAL];
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
        draw_set_fg_color (&listview->listctx, fg);

        draw_init_font (&listview->listctx, gtk_widget_get_style (GTK_WIDGET (listview)));
        if (gtkui_embolden_current_track && it && it == playing_track) {
            draw_init_font_bold (&listview->listctx);
        }
        if (calign_right) {
            draw_text (&listview->listctx, x+5, y + 3, cwidth-10, 1, text);
        }
        else {
            draw_text (&listview->listctx, x + 5, y + 3, cwidth-10, 0, text);
        }
        if (gtkui_embolden_current_track && it && it == playing_track) {
            draw_init_font_normal (&listview->listctx);
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
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (menuitem), "ps"));
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_playqueue_push (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    main_refresh ();
    search_redraw ();
}

void
main_remove_from_playback_queue_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (menuitem), "ps"));
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_playqueue_remove (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    main_refresh ();
    search_redraw ();
}

void
main_reload_metadata_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (menuitem), "ps"));
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        deadbeef->pl_lock ();
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        deadbeef->pl_unlock ();

        if (match) {
            uint32_t f = deadbeef->pl_get_item_flags (it);
            if (!(f & DDB_IS_SUBTRACK)) {
                f &= ~DDB_TAG_MASK;
                deadbeef->pl_set_item_flags (it, f);
                DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
                for (int i = 0; decoders[i]; i++) {
                    if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                        if (decoders[i]->read_metadata) {
                            decoders[i]->read_metadata (it);
                        }
                        break;
                    }
                }
            }
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    main_refresh ();
    search_redraw ();
    trkproperties_fill_metadata ();
}

void
main_properties_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    show_track_properties_dlg ();
}

void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_clear ();
    deadbeef->pl_save_all ();
    main_refresh ();
    search_refresh ();
}

void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int cursor = deadbeef->pl_delete_selected ();
    deadbeef->pl_save_all ();
    main_refresh ();
    search_redraw ();
}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    deadbeef->pl_crop_selected ();
    deadbeef->pl_save_all ();
    main_refresh ();
    search_redraw ();
}

void
on_remove2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int cursor = deadbeef->pl_delete_selected ();
    deadbeef->pl_save_all ();
    main_refresh ();
    search_redraw ();
}

void
on_remove_from_disk_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *widget = GTK_WIDGET (menuitem);

    if (deadbeef->conf_get_int ("gtkui.delete_files_ask", 1)) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Delete files from disk"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Files will be lost. Proceed?\n(This dialog can be turned off in GTKUI plugin settings)"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return;
        }
    }

    deadbeef->pl_lock ();

    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        const char *uri = deadbeef->pl_find_meta (it, ":URI");
        if (deadbeef->pl_is_selected (it) && deadbeef->is_local_file (uri)) {
            unlink (uri);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    int cursor = deadbeef->pl_delete_selected ();
    deadbeef->pl_save_all ();
    deadbeef->pl_unlock ();

    main_refresh ();
    search_redraw ();
}

void
actionitem_activate (GtkMenuItem     *menuitem,
                     DB_plugin_action_t *action)
{
    // Plugin can handle all tracks by itself
    if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS)
    {
        action->callback (action, NULL);
        return;
    }

    // For single-track actions just invoke it with first selected track
    if (!(action->flags & DB_ACTION_ALLOW_MULTIPLE_TRACKS))
    {
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (clicked_idx, PL_MAIN);
        action->callback (action, it);
        deadbeef->pl_item_unref (it);
        return;
    }
    
    //We end up here if plugin won't traverse tracks and we have to do it ourselves
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it))
            action->callback (action, it);
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

#define HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    g_object_ref (widget), (GDestroyNotify) g_object_unref)


static GtkWidget*
find_popup                          (GtkWidget       *widget,
                                        const gchar     *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = gtk_widget_get_parent (widget);
      if (!parent)
        parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  return found_widget;
}

#if 0
// experimental code to position the popup at the item
static void
popup_menu_position_func (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data) {
    // find 1st selected item
    DdbListview *lv = user_data;
    int winx, winy;
    gdk_window_get_position (gtk_widget_get_window (GTK_WIDGET (lv->list)), &winx, &winy);
    DdbListviewIter it = lv->binding->head ();
    int idx = 0;
    while (it) {
        if (lv->binding->is_selected (it)) {
            break;
        }
        DdbListviewIter next = lv->binding->next (it);
        lv->binding->unref (it);
        it = next;
        idx++;
    }
    if (it) {
        // get Y position
        *y = ddb_listview_get_row_pos (lv, idx) + winy;
        lv->binding->unref (it);
    }
    else {
        *y = winy; // mouse_y
    }
    *x = winx; // mouse_x
    *push_in = TRUE;
}
#endif

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
    add_to_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Add to playback queue"));
    gtk_widget_show (add_to_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), add_to_playback_queue1);
    g_object_set_data (G_OBJECT (add_to_playback_queue1), "ps", listview);

    remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Remove from playback queue"));
    if (inqueue == -1) {
        gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
    }
    gtk_widget_show (remove_from_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_playback_queue1);
    g_object_set_data (G_OBJECT (remove_from_playback_queue1), "ps", listview);

    reload_metadata = gtk_menu_item_new_with_mnemonic (_("Reload metadata"));
    gtk_widget_show (reload_metadata);
    gtk_container_add (GTK_CONTAINER (playlist_menu), reload_metadata);
    g_object_set_data (G_OBJECT (reload_metadata), "ps", listview);


    separator9 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator9);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator9);
    gtk_widget_set_sensitive (separator9, FALSE);

    remove2 = gtk_menu_item_new_with_mnemonic (_("Remove"));
    gtk_widget_show (remove2);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);
    g_object_set_data (G_OBJECT (remove2), "ps", listview);

    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);

    if (!hide_remove_from_disk) {
        remove_from_disk = gtk_menu_item_new_with_mnemonic (_("Remove from disk"));
        gtk_widget_show (remove_from_disk);
        gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_disk);
        g_object_set_data (G_OBJECT (remove_from_disk), "ps", listview);
    }

    separator8 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator8);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
    gtk_widget_set_sensitive (separator8, FALSE);

    int selected_count = 0;
    DB_playItem_t *pit = deadbeef->pl_get_first (PL_MAIN);
    DB_playItem_t *selected = NULL;
    while (pit) {
        if (deadbeef->pl_is_selected (pit))
        {
            if (!selected)
                selected = pit;
            selected_count++;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (pit, PL_MAIN);
        deadbeef->pl_item_unref (pit);
        pit = next;
    }

    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;
    int added_entries = 0;
    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (selected);
        DB_plugin_action_t *action;

        int count = 0;
        for (action = actions; action; action = action->next)
        {
            if (action->flags & (DB_ACTION_COMMON | DB_ACTION_PLAYLIST))
                continue;
            
            // create submenus (separated with '/')
            const char *prev = action->title;
            while (*prev && *prev == '/') {
                prev++;
            }
            
            GtkWidget *popup = NULL;
            
            for (;;) {
                const char *slash = strchr (prev, '/');
                if (slash && *(slash-1) != '\\') {
                    char name[slash-prev+1];
                    // replace \/ with /
                    const char *p = prev;
                    char *t = name;
                    while (*p && p < slash) {
                        if (*p == '\\' && *(p+1) == '/') {
                            *t++ = '/';
                            p += 2;
                        }
                        else {
                            *t++ = *p++;
                        }
                    }
                    *t = 0;

                    // add popup
                    GtkWidget *prev_menu = popup ? popup : playlist_menu;

                    popup = find_popup (prev_menu, name);
                    if (!popup) {
                        GtkWidget *item = gtk_image_menu_item_new_with_mnemonic (_(name));
                        gtk_widget_show (item);
                        gtk_container_add (GTK_CONTAINER (prev_menu), item);
                        popup = gtk_menu_new ();
                        HOOKUP_OBJECT (prev_menu, popup, name);
                        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), popup);
                    }
                }
                else {
                    break;
                }
                prev = slash+1;
            }


            count++;
            added_entries++;
            GtkWidget *actionitem;

            // replace \/ with /
            const char *p = popup ? prev : action->title;
            char title[strlen (p)+1];
            char *t = title;
            while (*p) {
                if (*p == '\\' && *(p+1) == '/') {
                    *t++ = '/';
                    p += 2;
                }
                else {
                    *t++ = *p++;
                }
            }
            *t = 0;

            actionitem = gtk_menu_item_new_with_mnemonic (_(title));
            gtk_widget_show (actionitem);
            gtk_container_add (popup ? GTK_CONTAINER (popup) : GTK_CONTAINER (playlist_menu), actionitem);
            g_object_set_data (G_OBJECT (actionitem), "ps", listview);

            g_signal_connect ((gpointer) actionitem, "activate",
                    G_CALLBACK (actionitem_activate),
                    action);
            if (!(
                ((selected_count == 1) && (action->flags & DB_ACTION_SINGLE_TRACK)) ||
                ((selected_count > 1) && (action->flags & DB_ACTION_ALLOW_MULTIPLE_TRACKS))
                ) ||
                action->flags & DB_ACTION_DISABLED)
            {
                gtk_widget_set_sensitive (GTK_WIDGET (actionitem), FALSE);
            }
        }
        if (count > 0 && deadbeef->conf_get_int ("gtkui.action_separators", 0))
        {
            separator8 = gtk_separator_menu_item_new ();
            gtk_widget_show (separator8);
            gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
            gtk_widget_set_sensitive (separator8, FALSE);
        }
    }
    if (added_entries > 0 && !deadbeef->conf_get_int ("gtkui.action_separators", 0))
    {
        separator8 = gtk_separator_menu_item_new ();
        gtk_widget_show (separator8);
        gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
        gtk_widget_set_sensitive (separator8, FALSE);
    }


    properties1 = gtk_menu_item_new_with_mnemonic (_("Properties"));
    gtk_widget_show (properties1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), properties1);
    g_object_set_data (G_OBJECT (properties1), "ps", listview);

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
    if (!hide_remove_from_disk) {
        g_signal_connect ((gpointer) remove_from_disk, "activate",
                G_CALLBACK (on_remove_from_disk_activate),
                NULL);
    }
    g_signal_connect ((gpointer) properties1, "activate",
            G_CALLBACK (main_properties_activate),
            NULL);
    gtk_menu_popup (GTK_MENU (playlist_menu), NULL, NULL, NULL/*popup_menu_position_func*/, listview, 0, gtk_get_current_event_time());
}

void
on_group_by_none_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    strcpy (group_by_str, "");
    deadbeef->conf_set_str ("playlist.group_by", group_by_str);

    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    main_refresh ();
}

void
on_group_by_artist_date_album_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    strcpy (group_by_str, "%a - [%y] %b");
    deadbeef->conf_set_str ("playlist.group_by", group_by_str);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    main_refresh ();
}

void
on_group_by_artist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    strcpy (group_by_str, "%a");
    deadbeef->conf_set_str ("playlist.group_by", group_by_str);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    main_refresh ();
}

void
on_group_by_custom_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_groupbydlg ();

    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    GtkWidget *entry = lookup_widget (dlg, "format");
    gtk_entry_set_text (GTK_ENTRY (entry), group_by_str);
//    gtk_window_set_title (GTK_WINDOW (dlg), "Group by");
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));

    if (response == GTK_RESPONSE_OK) {
        const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
        strncpy (group_by_str, text, sizeof (group_by_str));
        group_by_str[sizeof (group_by_str)-1] = 0;
        deadbeef->conf_set_str ("playlist.group_by", group_by_str);
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_modified (plt);
            deadbeef->plt_unref (plt);
        }
        main_refresh ();
    }
    gtk_widget_destroy (dlg);
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

    enum {
        DB_COLUMN_ARTIST_ALBUM = 2,
        DB_COLUMN_ARTIST = 3,
        DB_COLUMN_ALBUM = 4,
        DB_COLUMN_TITLE = 5,
        DB_COLUMN_DURATION = 6,
        DB_COLUMN_TRACK = 7,
    };

    inf->id = -1;
    // convert IDs from pre-0.4
    switch (id) {
    case DB_COLUMN_ARTIST_ALBUM:
        inf->format = strdup ("%a - %b");
        break;
    case DB_COLUMN_ARTIST:
        inf->format = strdup ("%a");
        break;
    case DB_COLUMN_ALBUM:
        inf->format = strdup ("%b");
        break;
    case DB_COLUMN_TITLE:
        inf->format = strdup ("%t");
        break;
    case DB_COLUMN_DURATION:
        inf->format = strdup ("%l");
        break;
    case DB_COLUMN_TRACK:
        inf->format = strdup ("%n");
        break;
    default:
        inf->format = *fmt ? strdup (fmt) : NULL;
        inf->id = id;
        break;
    }
    ddb_listview_column_append (listview, title, width, align, id == DB_COLUMN_ALBUM_ART ? width : 0, inf);
}

static void
init_column (col_info_t *inf, int id, const char *format) {
    if (inf->format) {
        free (inf->format);
        inf->format = NULL;
    }

    inf->id = -1;

    switch (id) {
    case 0:
        inf->id = DB_COLUMN_FILENUMBER;
        break;
    case 1:
        inf->id = DB_COLUMN_PLAYING;
        break;
    case 2:
        inf->id = DB_COLUMN_ALBUM_ART;
        break;
    case 3:
        inf->format = strdup ("%a - %b");
        break;
    case 4:
        inf->format = strdup ("%a");
        break;
    case 5:
        inf->format = strdup ("%b");
        break;
    case 6:
        inf->format = strdup ("%t");
        break;
    case 7:
        inf->format = strdup ("%l");
        break;
    case 8:
        inf->format = strdup ("%n");
        break;
    case 9:
        inf->format = strdup ("%B");
        break;
    default:
        inf->format = strdup (format);
    }
}

int editcolumn_title_changed = 0;

void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    editcolumn_title_changed = 0;
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Add column"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), 0);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int sel = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));

        col_info_t *inf = malloc (sizeof (col_info_t));
        memset (inf, 0, sizeof (col_info_t));

        init_column (inf, sel, format);

        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        ddb_listview_column_insert (last_playlist, active_column, title, 100, align, inf->id == DB_COLUMN_ALBUM_ART ? 100 : 0, inf);
        ddb_listview_refresh (last_playlist, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL);
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
    gtk_window_set_title (GTK_WINDOW (dlg), _("Edit column"));

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
    editcolumn_title_changed = 0;
    int idx = 10;
    if (inf->id == -1) {
        if (inf->format) {
            if (!strcmp (inf->format, "%a - %b")) {
                idx = 3;
            }
            else if (!strcmp (inf->format, "%a")) {
                idx = 4;
            }
            else if (!strcmp (inf->format, "%b")) {
                idx = 5;
            }
            else if (!strcmp (inf->format, "%t")) {
                idx = 6;
            }
            else if (!strcmp (inf->format, "%l")) {
                idx = 7;
            }
            else if (!strcmp (inf->format, "%n")) {
                idx = 8;
            }
            else if (!strcmp (inf->format, "%B")) {
                idx = 9;
            }
        }
    }
    else if (inf->id <= DB_COLUMN_PLAYING) {
        idx = inf->id;
    }
    else if (inf->id == DB_COLUMN_ALBUM_ART) {
        idx = 2;
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), idx);
    if (idx == 10) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "format")), inf->format);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), align_right);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));

        init_column (inf, id, format);
        ddb_listview_column_set_info (last_playlist, active_column, title, width, align, inf->id == DB_COLUMN_ALBUM_ART ? width : 0, inf);

        ddb_listview_refresh (last_playlist, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST);
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
    ddb_listview_refresh (last_playlist, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL);
}

GtkWidget*
create_headermenu (int groupby)
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
  GtkWidget *custom;

  headermenu = gtk_menu_new ();

  add_column = gtk_menu_item_new_with_mnemonic (_("Add column"));
  gtk_widget_show (add_column);
  gtk_container_add (GTK_CONTAINER (headermenu), add_column);

  edit_column = gtk_menu_item_new_with_mnemonic (_("Edit column"));
  gtk_widget_show (edit_column);
  gtk_container_add (GTK_CONTAINER (headermenu), edit_column);

  remove_column = gtk_menu_item_new_with_mnemonic (_("Remove column"));
  gtk_widget_show (remove_column);
  gtk_container_add (GTK_CONTAINER (headermenu), remove_column);

  if (groupby) {
      separator = gtk_separator_menu_item_new ();
      gtk_widget_show (separator);
      gtk_container_add (GTK_CONTAINER (headermenu), separator);
      gtk_widget_set_sensitive (separator, FALSE);

      group_by = gtk_menu_item_new_with_mnemonic (_("Group by"));
      gtk_widget_show (group_by);
      gtk_container_add (GTK_CONTAINER (headermenu), group_by);

      group_by_menu = gtk_menu_new ();
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (group_by), group_by_menu);

      none = gtk_menu_item_new_with_mnemonic (_("None"));
      gtk_widget_show (none);
      gtk_container_add (GTK_CONTAINER (group_by_menu), none);

      artist_date_album = gtk_menu_item_new_with_mnemonic (_("Artist/Date/Album"));
      gtk_widget_show (artist_date_album);
      gtk_container_add (GTK_CONTAINER (group_by_menu), artist_date_album);

      artist = gtk_menu_item_new_with_mnemonic (_("Artist"));
      gtk_widget_show (artist);
      gtk_container_add (GTK_CONTAINER (group_by_menu), artist);

      custom = gtk_menu_item_new_with_mnemonic (_("Custom"));
      gtk_widget_show (custom);
      gtk_container_add (GTK_CONTAINER (group_by_menu), custom);

      g_signal_connect ((gpointer) none, "activate",
              G_CALLBACK (on_group_by_none_activate),
              NULL);

      g_signal_connect ((gpointer) artist_date_album, "activate",
              G_CALLBACK (on_group_by_artist_date_album_activate),
              NULL);

      g_signal_connect ((gpointer) artist, "activate",
              G_CALLBACK (on_group_by_artist_activate),
              NULL);

      g_signal_connect ((gpointer) custom, "activate",
              G_CALLBACK (on_group_by_custom_activate),
              NULL);
  }

  g_signal_connect ((gpointer) add_column, "activate",
                    G_CALLBACK (on_add_column_activate),
                    NULL);
  g_signal_connect ((gpointer) edit_column, "activate",
                    G_CALLBACK (on_edit_column_activate),
                    NULL);
  g_signal_connect ((gpointer) remove_column, "activate",
                    G_CALLBACK (on_remove_column_activate),
                    NULL);

  return headermenu;
}

void
header_context_menu (DdbListview *ps, int column) {
    GtkWidget *menu = create_headermenu (GTK_WIDGET (ps) == lookup_widget (mainwin, "playlist") ? 1 : 0);
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

