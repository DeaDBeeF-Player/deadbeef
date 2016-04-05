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
#include "../libparser/parser.h"
#include "actions.h"
#include "actionhandlers.h"
#include "../../strdupa.h"
#include <jansson.h>

#define min(x,y) ((x)<(y)?(x):(y))
//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

// disable custom title function, until we have new title formatting (0.7)
#define DISABLE_CUSTOM_TITLE

// playlist theming
GtkWidget *theme_button;
GtkWidget *theme_treeview;
static GdkPixbuf *play16_pixbuf;
static GdkPixbuf *pause16_pixbuf;
static GdkPixbuf *buffering16_pixbuf;

static int clicked_idx = -1;

void
pl_common_init(void)
{
    play16_pixbuf = create_pixbuf("play_16.png");
    pause16_pixbuf = create_pixbuf("pause_16.png");
    buffering16_pixbuf = create_pixbuf("buffering_16.png");

    gtkui_groups_pinned = deadbeef->conf_get_int ("playlist.pin.groups", 0);

    theme_treeview = gtk_tree_view_new ();
    gtk_widget_show (theme_treeview);
    gtk_widget_set_can_focus (theme_treeview, FALSE);
    GtkWidget *vbox1 = lookup_widget (mainwin, "vbox1");
    gtk_box_pack_start (GTK_BOX (vbox1), theme_treeview, FALSE, FALSE, 0);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (theme_treeview), TRUE);

    theme_button = mainwin;//lookup_widget (mainwin, "stopbtn");
}

void
pl_common_free (void)
{
    if (theme_treeview) {
        gtk_widget_destroy (theme_treeview);
        theme_treeview = NULL;
    }

    g_object_unref(play16_pixbuf);
    g_object_unref(pause16_pixbuf);
    g_object_unref(buffering16_pixbuf);
}

#define COL_CONF_BUFFER_SIZE 10000

int
rewrite_column_config (DdbListview *listview, const char *name) {
    char *buffer = malloc (COL_CONF_BUFFER_SIZE);
    strcpy (buffer, "[");
    char *p = buffer+1;
    int n = COL_CONF_BUFFER_SIZE-3;

    int cnt = ddb_listview_column_get_count (listview);
    for (int i = 0; i < cnt; i++) {
        const char *title;
        int width;
        int align;
        col_info_t *info;
        int minheight;
        int color_override;
        GdkColor color;
        ddb_listview_column_get_info (listview, i, &title, &width, &align, &minheight, &color_override, &color, (void **)&info);

        char *esctitle = parser_escape_string (title);
        char *escformat = info->format ? parser_escape_string (info->format) : NULL;

        size_t written = snprintf (p, n, "{\"title\":\"%s\",\"id\":\"%d\",\"format\":\"%s\",\"size\":\"%d\",\"align\":\"%d\",\"color_override\":\"%d\",\"color\":\"#ff%02x%02x%02x\"}%s", esctitle, info->id, escformat ? escformat : "", width, align, color_override, color.red>>8, color.green>>8, color.blue>>8, i < cnt-1 ? "," : "");
        free (esctitle);
        if (escformat) {
            free (escformat);
        }
        p += written;
        n -= written;
        if (n <= 0) {
            fprintf (stderr, "Column configuration is too large, doesn't fit in the buffer. Won't be written.\n");
            return -1;
        }
    }
    strcpy (p, "]");
    deadbeef->conf_set_str (name, buffer);
    deadbeef->conf_save ();
    return 0;
}

static gboolean
redraw_playlist_cb (gpointer user_data) {
    DDB_LISTVIEW(user_data)->cover_size = DDB_LISTVIEW(user_data)->new_cover_size;
    gtk_widget_queue_draw (GTK_WIDGET(user_data));
    return FALSE;
}

static gboolean
tf_redraw_cb (gpointer user_data) {
    DdbListview *lv = user_data;

    ddb_listview_draw_row (lv, lv->tf_redraw_track_idx, lv->tf_redraw_track);
    lv->tf_redraw_track_idx = -1;
    if (lv->tf_redraw_track) {
        lv->binding->unref (lv->tf_redraw_track);
        lv->tf_redraw_track = NULL;
    }
    DDB_LISTVIEW(user_data)->tf_redraw_timeout_id = 0;
    return FALSE;
}

static void
redraw_playlist (void *user_data) {
    g_idle_add (redraw_playlist_cb, user_data);
}

static gboolean
redraw_playlist_single_cb (gpointer user_data) {
    gtk_widget_queue_draw (GTK_WIDGET(user_data));
    g_object_unref (GTK_WIDGET (user_data));
    return FALSE;
}

static void
redraw_playlist_single (void *user_data) {
    g_object_ref (GTK_WIDGET (user_data));
    g_idle_add (redraw_playlist_single_cb, user_data);
}

#define ART_PADDING_HORZ 8
#define ART_PADDING_VERT 0

static gboolean
deferred_cover_load_cb (void *ctx) {
    DdbListview *lv = ctx;
    lv->cover_refresh_timeout_id = 0;
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (lv);
    // find 1st group
    DdbListviewGroup *grp = lv->groups;
    int idx = 0;
    int grp_y = 0;
    while (grp && grp_y + grp->height < lv->scrollpos) {
        grp_y += grp->height;
        idx += grp->num_items + 1;
        grp = grp->next;
    }
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (lv), &a);
    while (grp && grp_y < a.height + lv->scrollpos) {
        // render title
        DdbListviewIter group_it = grp->head;
        int grpheight = grp->height;
        if (grp_y >= a.height + lv->scrollpos) {
            break;
        }
        const char *album = deadbeef->pl_find_meta (group_it, "album");
        const char *artist = deadbeef->pl_find_meta (group_it, "artist");
        if (!album || !*album) {
            album = deadbeef->pl_find_meta (group_it, "title");
        }

        grp_y += grpheight;
        grp = grp->next;
        int last = 0;
        if (!grp || grp_y >= a.height + lv->scrollpos) {
            last = 1;
        }
        GdkPixbuf *pixbuf = get_cover_art_thumb (deadbeef->pl_find_meta (((DB_playItem_t *)group_it), ":URI"), artist, album, lv->new_cover_size, NULL, NULL);
        if (last) {
            queue_cover_callback (redraw_playlist, lv);
        }
        if (pixbuf) {
            g_object_unref (pixbuf);
        }
    }
    deadbeef->pl_unlock ();

    return FALSE;
}

void
draw_album_art (DdbListview *listview, cairo_t *cr, DdbListviewIter group_it, int column, int group_pinned, int grp_next_y, int x, int y, int width, int height) {
    const char *ctitle;
    int cwidth;
    int calign_right;
    col_info_t *cinf;
    int minheight;
    int color_override;
    GdkColor fg_clr;
    int res = ddb_listview_column_get_info (listview, column, &ctitle, &cwidth, &calign_right, &minheight, &color_override, &fg_clr, (void **)&cinf);
    if (res == -1) {
        return;
    }

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();
    int theming = !gtkui_override_listview_colors ();

    if (cinf->id == DB_COLUMN_ALBUM_ART) {
        if (theming) {
#if GTK_CHECK_VERSION(3,0,0)
            cairo_save (cr);
            cairo_rectangle (cr, x, y, width, MAX (height,minheight));
            cairo_clip (cr);
            gtk_paint_flat_box (gtk_widget_get_style (theme_treeview), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, theme_treeview, "cell_even_ruled", x-1, y, width+2, MAX (height,minheight));
            cairo_restore (cr);
#else
            GdkRectangle clip = {
                .x = x,
                .y = y,
                .width = width,
                .height = MAX (height,minheight),
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
        int real_art_width = width - ART_PADDING_HORZ * 2;
        if (real_art_width > 7 && group_it) {
            const char *album = deadbeef->pl_find_meta (group_it, "album");
            const char *artist = deadbeef->pl_find_meta (group_it, "artist");
            if (!album || !*album) {
                album = deadbeef->pl_find_meta (group_it, "title");
            }
            if (listview->new_cover_size != real_art_width) {
                listview->new_cover_size = real_art_width;
                if (listview->cover_refresh_timeout_id) {
                    g_source_remove (listview->cover_refresh_timeout_id);
                    listview->cover_refresh_timeout_id = 0;
                }
                if (listview->cover_size == -1) {
                    listview->cover_size = real_art_width;
                }
                else {
                    if (!listview->cover_refresh_timeout_id) {
                        listview->cover_refresh_timeout_id = g_timeout_add (1000, deferred_cover_load_cb, listview);
                    }
                }
            }

            // destination coordinates and sizes
            int art_x = x + ART_PADDING_HORZ;
            int art_y = y + ART_PADDING_VERT;
            int art_w = listview->cover_size;
            int art_h = height;

            GdkPixbuf *pixbuf = get_cover_art_thumb (deadbeef->pl_find_meta (((DB_playItem_t *)group_it), ":URI"), artist, album, real_art_width == art_w ? art_w : -1, redraw_playlist_single, listview);
            if (pixbuf) {
                art_w = gdk_pixbuf_get_width (pixbuf);
                art_h = gdk_pixbuf_get_height (pixbuf);

                int draw_pinned = (y - listview->grouptitle_height < real_art_width && group_pinned == 1 && gtkui_groups_pinned) ? 1 : 0;

                if (art_y > -(real_art_width + listview->grouptitle_height) || draw_pinned) {
                    float art_scale = real_art_width;
                    if (art_w < art_h) {
                        art_scale /= (float)art_h;
                    }
                    else {
                        art_scale /= (float)art_w;
                    }

                    art_w *= art_scale;
                    art_h *= art_scale;

                    cairo_save (cr);
                    if (draw_pinned) {
                        if (grp_next_y <= art_h + listview->grouptitle_height + ART_PADDING_VERT) {
                            cairo_translate (cr, art_x, grp_next_y - art_h);
                        }
                        else {
                            cairo_translate (cr, art_x, listview->grouptitle_height + ART_PADDING_VERT);
                        }
                    }
                    else {
                        cairo_translate (cr, art_x, art_y);
                    }
                    cairo_rectangle (cr, 0, 0, art_w, art_h);
                    cairo_scale (cr, art_scale, art_scale);
                    gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
                    cairo_pattern_set_filter (cairo_get_source(cr), gtkui_is_default_pixbuf (pixbuf) ? CAIRO_FILTER_BEST : CAIRO_FILTER_FAST);
                    cairo_fill (cr);
                    cairo_restore (cr);
                }
                g_object_unref (pixbuf);
            }
        }
    }
    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }
}

void
draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int column, int iter, int x, int y, int width, int height) {
    const char *ctitle;
    int cwidth;
    int calign_right;
    col_info_t *cinf;
    int minheight;
    int color_override;
    GdkColor fg_clr;
    int res = ddb_listview_column_get_info (listview, column, &ctitle, &cwidth, &calign_right, &minheight, &color_override, &fg_clr, (void **)&cinf);
    if (res == -1) {
        return;
    }

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();
    int theming = !gtkui_override_listview_colors ();

    if (!gtkui_unicode_playstate && it && it == playing_track && cinf->id == DB_COLUMN_PLAYING) {
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
        char text[1024] = "";
        if (it == playing_track && cinf->id == DB_COLUMN_PLAYING) {
            int paused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED;
            int buffering = !deadbeef->streamer_ok_to_read (-1);
            if (paused) {
                strcpy (text, "||");
            }
            else if (!buffering) {
                strcpy (text, "►");
            }
            else {
                strcpy (text, "⋯");
            }
        }
        else {
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .it = it,
                .plt = deadbeef->plt_get_curr (),
                .iter = iter,
                .id = cinf->id,
                .idx = idx,
                .flags = DDB_TF_CONTEXT_HAS_ID | DDB_TF_CONTEXT_HAS_INDEX,
            };
            deadbeef->tf_eval (&ctx, cinf->bytecode, text, sizeof (text));
            if (ctx.update > 0) {
                ddb_listview_cancel_autoredraw (listview);
                if ((ctx.flags & DDB_TF_CONTEXT_HAS_INDEX) && ctx.iter == PL_MAIN) {
                    listview->tf_redraw_track_idx = ctx.idx;
                }
                else {
                    listview->tf_redraw_track_idx = deadbeef->plt_get_item_idx (ctx.plt, it, ctx.iter);
                }
                listview->tf_redraw_timeout_id = g_timeout_add (ctx.update, tf_redraw_cb, listview);
                listview->tf_redraw_track = it;
                deadbeef->pl_item_ref (it);
            }
            if (ctx.plt) {
                deadbeef->plt_unref (ctx.plt);
                ctx.plt = NULL;
            }
            char *lb = strchr (text, '\r');
            if (lb) {
                *lb = 0;
            }
            lb = strchr (text, '\n');
            if (lb) {
                *lb = 0;
            }
        }
        GdkColor *color = NULL;
        if (theming) {
            if (deadbeef->pl_is_selected (it)) {
                color = &gtk_widget_get_style (theme_treeview)->text[GTK_STATE_SELECTED];
            }
            else {
                if (color_override) {
                    color = &fg_clr;
                }
                else {
                    color = &gtk_widget_get_style (theme_treeview)->text[GTK_STATE_NORMAL];
                }
            }
        }
        else {
            GdkColor clr;
            if (deadbeef->pl_is_selected (it)) {
                color = (gtkui_get_listview_selected_text_color (&clr), &clr);
            }
            else if (it && it == playing_track) {
                if (color_override) {
                    color = &fg_clr;
                }
                else {
                    color = (gtkui_get_listview_playing_text_color (&clr), &clr);
                }
            }
            else {
                if (color_override) {
                    color = &fg_clr;
                }
                else {
                    color = (gtkui_get_listview_text_color (&clr), &clr);
                }
            }
        }
        float fg[3] = {(float)color->red/0xffff, (float)color->green/0xffff, (float)color->blue/0xffff};
        draw_set_fg_color (&listview->listctx, fg);

        draw_init_font (&listview->listctx, DDB_LIST_FONT, 0);
        int bold = 0;
        int italic = 0;
        if (deadbeef->pl_is_selected (it)) {
            bold = gtkui_embolden_selected_tracks;
            italic = gtkui_italic_selected_tracks;
        }
        if (it == playing_track) {
            bold = gtkui_embolden_current_track;
            italic = gtkui_italic_current_track;
        }
        draw_text_custom (&listview->listctx, x + 5, y + 3, cwidth-10, calign_right, DDB_LIST_FONT, bold, italic, text);
    }
    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }
}

static void
main_add_to_playback_queue_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_push (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
}

void
main_remove_from_playback_queue_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_remove (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
}

void
main_reload_metadata_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
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
    deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
}

void
main_properties_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_show_track_properties_handler (NULL, DDB_ACTION_CTX_SELECTION);
}

void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_clear ();
    deadbeef->pl_save_current ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_remove_from_playlist_handler (NULL, DDB_ACTION_CTX_SELECTION);
}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_crop_selected_handler (NULL, 0);
}

void
on_remove2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int cursor = deadbeef->pl_delete_selected ();
    deadbeef->pl_save_current ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

static void
on_toggle_set_custom_title (GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.location_set_custom_title", active);

    GtkWidget *ct = lookup_widget (GTK_WIDGET (user_data), "custom_title");
    gtk_widget_set_sensitive (ct, active);

    deadbeef->conf_save ();
}

#ifndef DISABLE_CUSTOM_TITLE
void
on_set_custom_title_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    DdbListview *lv = user_data;
    int idx = lv->binding->cursor ();
    if (idx < 0) {
        return;
    }
    DdbListviewIter it = lv->binding->get_for_idx (idx);
    if (!it) {
        return;
    }

    GtkWidget *dlg = create_setcustomtitledlg ();
    GtkWidget *sct = lookup_widget (dlg, "set_custom_title");
    GtkWidget *ct = lookup_widget (dlg, "custom_title");
    if (deadbeef->conf_get_int ("gtkui.location_set_custom_title", 0)) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sct), TRUE);
        gtk_widget_set_sensitive (ct, TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sct), FALSE);
        gtk_widget_set_sensitive (ct, FALSE);
    }
    deadbeef->pl_lock ();
    const char *custom_title = deadbeef->pl_find_meta ((DB_playItem_t *)it, ":CUSTOM_TITLE");
    if (custom_title) {
        custom_title = strdupa (custom_title);
    }
    else {
        custom_title = "";
    }
    deadbeef->pl_unlock ();

    g_signal_connect ((gpointer) sct, "toggled",
            G_CALLBACK (on_toggle_set_custom_title),
            dlg);
    gtk_entry_set_text (GTK_ENTRY (ct), custom_title);

    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        if (it && deadbeef->conf_get_int ("gtkui.location_set_custom_title", 0)) {
            deadbeef->pl_replace_meta ((DB_playItem_t *)it, ":CUSTOM_TITLE", gtk_entry_get_text (GTK_ENTRY (ct)));
        }
        else {
            deadbeef->pl_delete_meta ((DB_playItem_t *)it, ":CUSTOM_TITLE");
        }
    }
    gtk_widget_destroy (dlg);
    lv->binding->unref (it);
}
#endif

void
on_remove_from_disk_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_delete_from_disk_handler_cb ((void *)(intptr_t)DDB_ACTION_CTX_SELECTION);
}

void
actionitem_activate (GtkMenuItem     *menuitem,
                     DB_plugin_action_t *action)
{
    if (action->callback) {
        gtkui_exec_action_14 (action, clicked_idx);
    }
    else {
        action->callback2 (action, DDB_ACTION_CTX_SELECTION);
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
    int inqueue = deadbeef->playqueue_test (it);
    GtkWidget *playlist_menu;
    GtkWidget *add_to_playback_queue1;
    GtkWidget *remove_from_playback_queue1;
    GtkWidget *separator;
    GtkWidget *remove2;
    GtkWidget *remove_from_disk;
    GtkWidget *separator8;
    GtkWidget *properties1;
    GtkWidget *reload_metadata;
#ifndef DISABLE_CUSTOM_TITLE
    GtkWidget *set_custom_title;
#endif

    playlist_menu = gtk_menu_new ();
    add_to_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Add To Playback Queue"));
    gtk_widget_show (add_to_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), add_to_playback_queue1);
    g_object_set_data (G_OBJECT (add_to_playback_queue1), "ps", listview);

    remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Remove From Playback Queue"));
    if (inqueue == -1) {
        gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
    }
    gtk_widget_show (remove_from_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_playback_queue1);
    g_object_set_data (G_OBJECT (remove_from_playback_queue1), "ps", listview);

    reload_metadata = gtk_menu_item_new_with_mnemonic (_("Reload Metadata"));
    gtk_widget_show (reload_metadata);
    gtk_container_add (GTK_CONTAINER (playlist_menu), reload_metadata);
    g_object_set_data (G_OBJECT (reload_metadata), "ps", listview);

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

    remove2 = gtk_menu_item_new_with_mnemonic (_("Remove"));
    gtk_widget_show (remove2);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);
    g_object_set_data (G_OBJECT (remove2), "ps", listview);

    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);

    if (!hide_remove_from_disk) {
        remove_from_disk = gtk_menu_item_new_with_mnemonic (_("Remove From Disk"));
        gtk_widget_show (remove_from_disk);
        gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_disk);
        g_object_set_data (G_OBJECT (remove_from_disk), "ps", listview);
    }

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

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
            if ((action->flags & DB_ACTION_COMMON) || !((action->callback2 && (action->flags & DB_ACTION_ADD_MENU)) || action->callback) || !(action->flags & (DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK)))
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
            if ((selected_count > 1 && !(action->flags & DB_ACTION_MULTIPLE_TRACKS)) ||
                (action->flags & DB_ACTION_DISABLED)) {
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

#ifndef DISABLE_CUSTOM_TITLE
    set_custom_title = gtk_menu_item_new_with_mnemonic (_("Set Custom Title"));
    gtk_widget_show (set_custom_title);
    gtk_container_add (GTK_CONTAINER (playlist_menu), set_custom_title);
    if (selected_count != 1) {
        gtk_widget_set_sensitive (GTK_WIDGET (set_custom_title), FALSE);
    }

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);
#endif

    properties1 = gtk_menu_item_new_with_mnemonic (_("Track Properties"));
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
#ifndef DISABLE_CUSTOM_TITLE
    g_signal_connect ((gpointer) set_custom_title, "activate",
            G_CALLBACK (on_set_custom_title_activate),
            listview);
#endif
    g_signal_connect ((gpointer) properties1, "activate",
            G_CALLBACK (main_properties_activate),
            NULL);
    gtk_menu_popup (GTK_MENU (playlist_menu), NULL, NULL, NULL/*popup_menu_position_func*/, listview, 0, gtk_get_current_event_time());
}

static DdbListview *last_playlist;
static int active_column;

void
on_group_by_none_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    last_playlist->binding->groups_changed (last_playlist, "");

    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    main_refresh ();
}

void
on_pin_groups_active                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int old_val = deadbeef->conf_get_int ("playlist.pin.groups", 0);
    deadbeef->conf_set_int ("playlist.pin.groups", old_val ? 0 : 1);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    gtk_check_menu_item_toggled(GTK_CHECK_MENU_ITEM(menuitem));
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref(plt);
    }
    main_refresh ();
}

void
on_group_by_artist_date_album_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    last_playlist->binding->groups_changed (last_playlist, "%album artist% - ['['%year%']' ]%album%");
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
    last_playlist->binding->groups_changed (last_playlist, "%artist%");
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
    if (last_playlist->group_format) {
        gtk_entry_set_text (GTK_ENTRY (entry), last_playlist->group_format);
    }
    else {
        gtk_entry_set_text (GTK_ENTRY (entry), "");
    }
//    gtk_window_set_title (GTK_WINDOW (dlg), "Group by");
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));

    if (response == GTK_RESPONSE_OK) {
        const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
        last_playlist->binding->groups_changed (last_playlist, text);
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_modified (plt);
            deadbeef->plt_unref (plt);
        }
        main_refresh ();
    }
    gtk_widget_destroy (dlg);
}

void
set_last_playlist_cm (DdbListview *pl) {
    last_playlist = pl;
}

void
set_active_column_cm (int col) {
    active_column = col;
}

int
load_column_config (DdbListview *listview, const char *key) {
    deadbeef->conf_lock ();
    const char *json = deadbeef->conf_get_str_fast (key, NULL);
    json_error_t error;
    if (!json) {
        deadbeef->conf_unlock ();
        return -1;
    }
    json_t *root = json_loads (json, 0, &error);
    deadbeef->conf_unlock ();
    if(!root) {
        printf ("json parse error for config variable %s\n", key);
        return -1;
    }
    if (!json_is_array (root))
    {
        goto error;
    }
    for (int i = 0; i < json_array_size(root); i++) {
        json_t *title, *align, *id, *format, *width, *color_override, *color;
        json_t *data = json_array_get (root, i);
        if (!json_is_object (data)) {
            goto error;
        }
        title = json_object_get (data, "title");
        align = json_object_get (data, "align");
        id = json_object_get (data, "id");
        format = json_object_get (data, "format");
        width = json_object_get (data, "size");
        color_override = json_object_get (data, "color_override");
        color = json_object_get (data, "color");
        if (!json_is_string (title)
                || !json_is_string (id)
                || !json_is_string (width)
           ) {
            goto error;
        }
        const char *stitle = NULL;
        int ialign = -1;
        int iid = -1;
        const char *sformat = NULL;
        int iwidth = 0;
        int icolor_override = 0;
        const char *scolor = NULL;
        GdkColor gdkcolor = {0};
        stitle = json_string_value (title);
        if (json_is_string (align)) {
            ialign = atoi (json_string_value (align));
        }
        if (json_is_string (id)) {
            iid = atoi (json_string_value (id));
        }
        if (json_is_string (format)) {
            sformat = json_string_value (format);
            if (!sformat[0]) {
                sformat = NULL;
            }
        }
        if (json_is_string (width)) {
            iwidth = atoi (json_string_value (width));
        }
        if (json_is_string (color_override)) {
            icolor_override = atoi (json_string_value (color_override));
        }
        if (json_is_string (color)) {
            scolor = json_string_value (color);
            int r, g, b, a;
            if (4 == sscanf (scolor, "#%02x%02x%02x%02x", &a, &r, &g, &b)) {
                gdkcolor.red = r<<8;
                gdkcolor.green = g<<8;
                gdkcolor.blue = b<<8;
            }
            else {
                icolor_override = 0;
            }
        }

        col_info_t *inf = malloc (sizeof (col_info_t));
        memset (inf, 0, sizeof (col_info_t));
        inf->id = iid;
        if (sformat) {
            inf->format = strdup (sformat);
            inf->bytecode = deadbeef->tf_compile (inf->format);
        }
        ddb_listview_column_append (listview, stitle, iwidth, ialign, inf->id == DB_COLUMN_ALBUM_ART ? iwidth : 0, icolor_override, gdkcolor, inf);
    }
    json_decref(root);
    return 0;
error:
    fprintf (stderr, "%s config variable contains invalid data, ignored\n", key);
    json_decref(root);
    return -1;
}

static void
init_column (col_info_t *inf, int id, const char *format) {
    if (inf->format) {
        free (inf->format);
        inf->format = NULL;
    }
    if (inf->bytecode) {
        deadbeef->tf_free (inf->bytecode);
        inf->bytecode = NULL;
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
        inf->format = strdup ("$if(%artist%,%artist%,Unknown Artist)[ - %album%]");
        break;
    case 4:
        inf->format = strdup ("$if(%artist%,%artist%,Unknown Artist)");
        break;
    case 5:
        inf->format = strdup ("%album%");
        break;
    case 6:
        inf->format = strdup ("%title%");
        break;
    case 7:
        inf->format = strdup ("%length%");
        break;
    case 8:
        inf->format = strdup ("%tracknumber%");
        break;
    case 9:
        inf->format = strdup ("$if(%album artist%,%album artist%,Unknown Artist)");
        break;
    default:
        inf->format = strdup (format);
    }
    if (inf->format) {
        inf->bytecode = deadbeef->tf_compile (inf->format);
    }
}

int editcolumn_title_changed = 0;

void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    editcolumn_title_changed = 0;
    GdkColor color;
    gtkui_get_listview_text_color (&color);

    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Add column"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")), 0);

    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &color);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int sel = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));

        int clr_override = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")));
        GdkColor clr;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &clr);

        col_info_t *inf = malloc (sizeof (col_info_t));
        memset (inf, 0, sizeof (col_info_t));

        init_column (inf, sel, format);

        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        ddb_listview_column_insert (last_playlist, active_column, title, 100, align, inf->id == DB_COLUMN_ALBUM_ART ? 100 : 0, clr_override, clr, inf);
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
    int color_override;
    GdkColor color;
    int res = ddb_listview_column_get_info (last_playlist, active_column, &title, &width, &align_right, &minheight, &color_override, &color, (void **)&inf);
    if (res == -1) {
        trace ("attempted to edit non-existing column\n");
        return;
    }

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
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), title);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")), color_override);

    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &color);
    editcolumn_title_changed = 0;
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));

        int clr_override = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")));
        GdkColor clr;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &clr);

        init_column (inf, id, format);
        ddb_listview_column_set_info (last_playlist, active_column, title, width, align, inf->id == DB_COLUMN_ALBUM_ART ? width : 0, clr_override, clr, inf);

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
  GtkWidget *pin_groups;
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

      pin_groups = gtk_check_menu_item_new_with_mnemonic(_("Pin groups when scrolling"));
      gtk_widget_show (pin_groups);
      gtk_container_add (GTK_CONTAINER (headermenu), pin_groups);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (pin_groups), (gboolean)deadbeef->conf_get_int("playlist.pin.groups",0));

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

      g_signal_connect ((gpointer) pin_groups, "activate",
              G_CALLBACK (on_pin_groups_active),
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
add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, int align_right) {
    if (!format) {
        format = "";
    }
    col_info_t *inf = malloc (sizeof (col_info_t));
    memset (inf, 0, sizeof (col_info_t));
    inf->id = id;
    inf->format = strdup (format);
    inf->bytecode = deadbeef->tf_compile (inf->format);
    GdkColor color = { 0, 0, 0, 0 };
    ddb_listview_column_append (listview, title, width, align_right, inf->id == DB_COLUMN_ALBUM_ART ? width : 0, 0, color, inf);
}

int
pl_common_get_group (DdbListview *listview, DdbListviewIter it, char *str, int size) {
    if (!listview->group_format || !listview->group_format[0]) {
        return -1;
    }
    if (listview->group_title_bytecode) {
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .it = it,
            .plt = deadbeef->plt_get_curr (),
            .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
        };
        deadbeef->tf_eval (&ctx, listview->group_title_bytecode, str, size);
        if (ctx.plt) {
            deadbeef->plt_unref (ctx.plt);
            ctx.plt = NULL;
        }

        char *lb = strchr (str, '\r');
        if (lb) {
            *lb = 0;
        }
        lb = strchr (str, '\n');
        if (lb) {
            *lb = 0;
        }
    }
    return 0;
}

void
pl_common_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int iter, int x, int y, int width, int height) {
    if (listview->group_format && listview->group_format[0]) {
        char str[1024] = "";
        if (listview->group_title_bytecode) {
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .it = it,
                .plt = deadbeef->plt_get_curr (),
                .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
                .iter = iter,
            };
            deadbeef->tf_eval (&ctx, listview->group_title_bytecode, str, sizeof (str));
            if (ctx.plt) {
                deadbeef->plt_unref (ctx.plt);
                ctx.plt = NULL;
            }


            char *lb = strchr (str, '\r');
            if (lb) {
                *lb = 0;
            }
            lb = strchr (str, '\n');
            if (lb) {
                *lb = 0;
            }
        }

        int theming = !gtkui_override_listview_colors ();
        if (theming) {
            GdkColor *clr = &gtk_widget_get_style(theme_treeview)->fg[GTK_STATE_NORMAL];
            float rgb[] = {clr->red/65535.f, clr->green/65535.f, clr->blue/65535.f};
            draw_set_fg_color (&listview->grpctx, rgb);
        }
        else {
            GdkColor clr;
            gtkui_get_listview_group_text_color (&clr);
            float rgb[] = {clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f};
            draw_set_fg_color (&listview->grpctx, rgb);
        }
        int ew;
        draw_text_custom (&listview->grpctx, x + 5, y + height/2 - draw_get_listview_rowheight (&listview->grpctx)/2 + 3, -1, 0, DDB_GROUP_FONT, 0, 0, str);
        draw_get_layout_extents (&listview->grpctx, &ew, NULL);
        int len = strlen (str);
        int line_x = x + 5 + ew + (len ? ew / len / 2 : 0);
        if (line_x < x + width) {
            draw_line (&listview->grpctx, line_x, y+height/2, x+width, y+height/2);
        }
    }
}

static int
import_column_from_0_6 (const uint8_t *def, char *json_out, int outsize) {
    // syntax: "title" "format" id width alignright
    char token[MAX_TOKEN];
    const char *p = def;
    char title[MAX_TOKEN];
    int id;
    char fmt[MAX_TOKEN];
    int width;
    int align;

    *json_out = 0;

    parser_init ();

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return 0;
    }
    strcpy (title, token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return 0;
    }
    strcpy (fmt, token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return 0;
    }
    id = atoi (token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return 0;
    }
    width = atoi (token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return 0;
    }
    align = atoi (token);

    enum {
        DB_COLUMN_ARTIST_ALBUM = 2,
        DB_COLUMN_ARTIST = 3,
        DB_COLUMN_ALBUM = 4,
        DB_COLUMN_TITLE = 5,
        DB_COLUMN_DURATION = 6,
        DB_COLUMN_TRACK = 7,
    };

    int out_id = -1;
    const char *format;
#define MAX_COLUMN_TF 2048
    char out_tf[MAX_COLUMN_TF];

    // convert IDs from pre-0.4
    switch (id) {
    case DB_COLUMN_ARTIST_ALBUM:
        format = "%artist% - %album%";
        break;
    case DB_COLUMN_ARTIST:
        format = "%artist%";
        break;
    case DB_COLUMN_ALBUM:
        format = "%album%";
        break;
    case DB_COLUMN_TITLE:
        format = "%title%";
        break;
    case DB_COLUMN_DURATION:
        format = "%length%";
        break;
    case DB_COLUMN_TRACK:
        format = "%tracknumber%";
        break;
    default:
        deadbeef->tf_import_legacy (fmt, out_tf, sizeof (out_tf));
        format = out_tf;
        out_id = id;
        break;
    }
    int ret = snprintf (json_out, outsize, "{\"title\":\"%s\",\"id\":\"%d\",\"format\":\"%s\",\"size\":\"%d\",\"align\":\"%d\"}", title, out_id, format, width, align);
    return min (ret, outsize);
}


int
import_column_config_0_6 (const char *oldkeyprefix, const char *newkey) {
    DB_conf_item_t *col = deadbeef->conf_find (oldkeyprefix, NULL);
    if (!col) {
        return 0;
    }

#define MAX_COLUMN_CONFIG 20000
    char *json = calloc (1, MAX_COLUMN_CONFIG);
    char *out = json;
    int jsonsize = MAX_COLUMN_CONFIG-1;

    *out++ = '[';
    jsonsize--;

    int idx = 0;
    while (col) {
        if (jsonsize < 2) {
            break;
        }
        if (idx != 0) {
            *out++ = ',';
            jsonsize--;
        }
        int res = import_column_from_0_6 (col->value, out, jsonsize);
        out += res;
        jsonsize -= res;
        col = deadbeef->conf_find (oldkeyprefix, col);
        idx++;
    }
    *out++ = ']';
    if (*json) {
        deadbeef->conf_set_str (newkey, json);
    }
    free (json);
    return 0;
}
