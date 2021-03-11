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

#include <jansson.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>
#include "gtkui.h"
#include "../../strdupa.h"
#include "../libparser/parser.h"
#include "../../shared/deletefromdisk.h"
#include "actions.h"
#include "actionhandlers.h"
#include "clipboard.h"
#include "coverart.h"
#include "drawing.h"
#include "interface.h"
#include "plcommon.h"
#include "support.h"
#include "trkproperties.h"
#include "../../shared/tftintutil.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

// disable custom title function, until we have new title formatting (0.7)
#define DISABLE_CUSTOM_TITLE

#define SUBGROUP_DELIMITER "|||"

#if !GTK_CHECK_VERSION(3,22,0)
#define gtk_menu_popup_at_pointer(menu,trigger_event) gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time())
#endif

typedef struct {
    int id;
    char *format;
    char *sort_format;
    char *bytecode;
    char *sort_bytecode;
    int cover_size;
    int new_cover_size;
    int cover_load_timeout_id;
    DdbListview *listview;
} col_info_t;

// playlist theming
GtkWidget *theme_button;
GtkWidget *theme_treeview;
static GdkPixbuf *play16_pixbuf;
static GdkPixbuf *pause16_pixbuf;
static GdkPixbuf *buffering16_pixbuf;

static int clicked_idx = -1;

struct pl_preset_column_format {
    enum pl_column_t id;
    char *title;
    char *format;
};

#define PRESET_COLUMN_NUMITEMS 14

static struct pl_preset_column_format pl_preset_column_formats[PRESET_COLUMN_NUMITEMS];

static ddbUtilTrackList_t _menuTrackList;

static void
_capture_selected_track_list (void) {
    if (_menuTrackList != NULL) {
        ddbUtilTrackListFree(_menuTrackList);
        _menuTrackList = NULL;
    }

    ddb_playItem_t **tracks = NULL;

    ddb_playlist_t *plt = deadbeef->plt_get_curr();

    deadbeef->pl_lock ();

    ddb_playItem_t *current = deadbeef->streamer_get_playing_track ();
    int current_idx = -1;

    int count = deadbeef->plt_getselcount(plt);
    int all_idx = 0;
    int idx = 0;
    if (count) {
        tracks = calloc (sizeof (ddb_playItem_t *), count);

        ddb_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            ddb_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            if (current != NULL && it == current) {
                current_idx = all_idx;
            }
            if (deadbeef->pl_is_selected (it)) {
                tracks[idx++] = it;
            }
            else {
                deadbeef->pl_item_unref (it);
            }
            it = next;
            all_idx++;
        }
    }


    deadbeef->pl_unlock ();

    _menuTrackList = ddbUtilTrackListInitWithWithTracks(ddbUtilTrackListAlloc(), plt, DDB_ACTION_CTX_SELECTION, tracks, count, current, current_idx);

    if (current) {
        deadbeef->pl_item_unref (current);
        current = NULL;
    }

    for (int i = 0; i < idx; i++) {
        deadbeef->pl_item_unref (tracks[i]);
    }

    free (tracks);

    deadbeef->plt_unref (plt);
}

static void
init_preset_column_struct(void) {
    // There can only be one instance of columns of type DB_COLUMN_FILENUMBER, DB_COLUMN_PLAYING, DB_COLUMN_ALBUM_ART and DB_COLUMN_CUSTOM
    struct pl_preset_column_format data[] = {
        {DB_COLUMN_FILENUMBER, _("Item Index"), NULL},
        {DB_COLUMN_PLAYING, _("Playing"), NULL},
        {DB_COLUMN_ALBUM_ART, _("Album Art"), NULL},
        {DB_COLUMN_STANDARD, _("Artist - Album"), COLUMN_FORMAT_ARTISTALBUM},
        {DB_COLUMN_STANDARD, _("Artist"), COLUMN_FORMAT_ARTIST},
        {DB_COLUMN_STANDARD, _("Album"), COLUMN_FORMAT_ALBUM},
        {DB_COLUMN_STANDARD, _("Title"), COLUMN_FORMAT_TITLE},
        {DB_COLUMN_STANDARD, _("Year"), COLUMN_FORMAT_YEAR},
        {DB_COLUMN_STANDARD, _("Duration"), COLUMN_FORMAT_LENGTH},
        {DB_COLUMN_STANDARD, _("Track Number"), COLUMN_FORMAT_TRACKNUMBER},
        {DB_COLUMN_STANDARD, _("Band / Album Artist"), COLUMN_FORMAT_BAND},
        {DB_COLUMN_STANDARD, _("Codec"), COLUMN_FORMAT_CODEC},
        {DB_COLUMN_STANDARD, _("Bitrate"), COLUMN_FORMAT_BITRATE},
        {DB_COLUMN_CUSTOM, _("Custom"), NULL}
    };
    memcpy(pl_preset_column_formats, data, sizeof(pl_preset_column_formats));
}

int
find_first_preset_column_type (int type) {
    int idx = -1;
    for (int i = 0; i < PRESET_COLUMN_NUMITEMS; i++) {
        if (pl_preset_column_formats[i].id == type) {
            idx = i;
            break;
        }
    }
    return idx;
}

void
pl_common_init(void)
{
    play16_pixbuf = create_pixbuf("play_16.png");
    pause16_pixbuf = create_pixbuf("pause_16.png");
    buffering16_pixbuf = create_pixbuf("buffering_16.png");

    theme_treeview = gtk_tree_view_new();
    gtk_widget_show(theme_treeview);
    gtk_widget_set_can_focus(theme_treeview, FALSE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(theme_treeview), TRUE);
    gtk_box_pack_start(GTK_BOX(gtk_bin_get_child(GTK_BIN(mainwin))), theme_treeview, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION(3,0,0)
    GtkStyleContext *context = gtk_widget_get_style_context(theme_treeview);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_CELL);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_VIEW);
#endif
    theme_button = mainwin;

    init_preset_column_struct();
}

void
pl_common_free (void)
{
    if (play16_pixbuf) {
        g_object_unref(play16_pixbuf);
    }
    if (pause16_pixbuf) {
        g_object_unref(pause16_pixbuf);
    }
    if (buffering16_pixbuf) {
        g_object_unref(buffering16_pixbuf);
    }

    if (_menuTrackList) {
        ddbUtilTrackListFree(_menuTrackList);
        _menuTrackList = NULL;
    }
}

static col_info_t *
create_col_info (DdbListview *listview, int id) {
    col_info_t *info = malloc(sizeof(col_info_t));
    memset(info, '\0', sizeof(col_info_t));
    info->id = id;
    info->cover_size = -1;
    info->new_cover_size = -1;
    info->listview = listview;
    return info;
}

void
pl_common_free_col_info (void *data) {
    if (!data) {
        return;
    }

    col_info_t *info = data;
    if (info->format) {
        free (info->format);
    }
    if (info->bytecode) {
        free (info->bytecode);
    }
    if (info->sort_format) {
        free (info->sort_format);
    }
    if (info->sort_bytecode) {
        free (info->sort_bytecode);
    }
    if (info->cover_load_timeout_id) {
        g_source_remove(info->cover_load_timeout_id);
        info->cover_load_timeout_id = 0;
    }
    free (info);
}

#define COL_CONF_BUFFER_SIZE 10000

int
pl_common_rewrite_column_config (DdbListview *listview, const char *name) {
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
        int color_override;
        GdkColor color;
        ddb_listview_column_get_info (listview, i, &title, &width, &align, NULL, NULL, &color_override, &color, (void **)&info);

        char *esctitle = parser_escape_string (title);
        char *escformat = info->format ? parser_escape_string (info->format) : NULL;
        char *escsortformat = info->sort_format ? parser_escape_string (info->sort_format) : NULL;

        size_t written = snprintf (p, n, "{\"title\":\"%s\",\"id\":\"%d\",\"format\":\"%s\",\"sort_format\":\"%s\",\"size\":\"%d\",\"align\":\"%d\",\"color_override\":\"%d\",\"color\":\"#ff%02x%02x%02x\"}%s", esctitle, info->id, escformat ? escformat : "", escsortformat ? escsortformat : "", width, align, color_override, color.red>>8, color.green>>8, color.blue>>8, i < cnt-1 ? "," : "");
        free (esctitle);
        if (escformat) {
            free (escformat);
        }
        if (escsortformat) {
            free (escsortformat);
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
tf_redraw_cb (gpointer user_data) {
    DdbListview *lv = user_data;

    ddb_listview_draw_row (lv, lv->tf_redraw_track_idx, lv->tf_redraw_track);
    lv->tf_redraw_track_idx = -1;
    if (lv->tf_redraw_track) {
        lv->binding->unref (lv->tf_redraw_track);
        lv->tf_redraw_track = NULL;
    }
    lv->tf_redraw_timeout_id = 0;
    return FALSE;
}

#define ART_PADDING_HORZ 8
#define ART_PADDING_VERT 0

static int
min_group_height(void *user_data, int width) {
    col_info_t *info = (col_info_t *)user_data;
    if (info->id == DB_COLUMN_ALBUM_ART) {
        return width - 2*ART_PADDING_HORZ + 2*ART_PADDING_VERT;
    }
    return 0;
}

///// cover art display
int
pl_common_is_album_art_column (void *user_data) {
    col_info_t *info = (col_info_t *)user_data;
    return info->id == DB_COLUMN_ALBUM_ART;
}

static GdkPixbuf *
get_cover_art (DB_playItem_t *it, int width, int height, void (*callback)(void *), void *user_data) {
    deadbeef->pl_lock();
    const char *uri = deadbeef->pl_find_meta(it, ":URI");
    const char *album = deadbeef->pl_find_meta(it, "album");
    const char *artist = deadbeef->pl_find_meta(it, "artist");
    if (!album || !*album) {
        album = deadbeef->pl_find_meta(it, "title");
    }
    GdkPixbuf *pixbuf = get_cover_art_thumb_by_size(uri, artist, album, width, height, callback, user_data);
    deadbeef->pl_unlock();
    return pixbuf;
}

static gboolean
cover_invalidate_cb (void *user_data) {
    col_info_t *info = user_data;
    info->cover_size = info->new_cover_size;
    ddb_listview_invalidate_album_art_columns(info->listview);
    return FALSE;
}

static void
cover_invalidate (void *user_data) {
    g_idle_add(cover_invalidate_cb, user_data);
}

static gboolean
cover_load (void *user_data) {
    col_info_t *info = user_data;
    info->cover_load_timeout_id = 0;

    ddb_listview_groupcheck(info->listview);
    DdbListviewGroup *group = info->listview->groups;
    int group_y = 0;
    while (group && group_y + group->height < info->listview->scrollpos) {
        group_y += group->height;
        group = group->next;
    }

    GtkAllocation a;
    gtk_widget_get_allocation(info->listview->list, &a);
    int end_pos = info->listview->scrollpos + a.height;
    while (group && group_y < end_pos) {
        GdkPixbuf *pixbuf = get_cover_art(group->head, info->new_cover_size, info->new_cover_size, NULL, NULL);
        if (pixbuf) {
            g_object_unref(pixbuf);
        }

        group_y += group->height;
        group = group->next;
    }
    queue_cover_callback(cover_invalidate, info);

    return FALSE;
}

static void
cover_draw_cairo (GdkPixbuf *pixbuf, int x, int min_y, int max_y, int width, int height, cairo_t *cr, int filter) {
    int pw = gdk_pixbuf_get_width(pixbuf);
    int ph = gdk_pixbuf_get_height(pixbuf);
    int real_y = min(min_y, max_y - ph);
    cairo_save(cr);
    cairo_rectangle(cr, x, min_y, width, max_y - min_y);
    cairo_translate (cr, x, real_y);
    if (pw > width || ph > height || pw < width && ph < height) {
        const double scale = min(width/(double)pw, height/(double)ph);
        cairo_translate(cr, (width - width*scale)/2., min(min_y, max_y - ph*scale) - real_y);
        cairo_scale(cr, scale, scale);
        cairo_pattern_set_filter(cairo_get_source(cr), filter);
    }
    gdk_cairo_set_source_pixbuf(cr, pixbuf, (width - pw)/2., 0);
    cairo_fill(cr);
    cairo_restore(cr);
}

static void
cover_draw_anything (DB_playItem_t *it, int x, int min_y, int max_y, int width, int height, cairo_t *cr, void *user_data) {
    GdkPixbuf *pixbuf = get_cover_art(it, -1, -1, NULL, NULL);
    if (!pixbuf) {
        pixbuf = get_cover_art(it, width, height, cover_invalidate, user_data);
    }
    if (pixbuf) {
        cover_draw_cairo(pixbuf, x, min_y, max_y, width, height, cr, CAIRO_FILTER_FAST);
        g_object_unref(pixbuf);
    }
}

static void
cover_draw_exact (DB_playItem_t *it, int x, int min_y, int max_y, int width, int height, cairo_t *cr, void *user_data) {
    GdkPixbuf *pixbuf = get_cover_art(it, width, height, cover_invalidate, user_data);
    if (!pixbuf) {
        pixbuf = get_cover_art(it, -1, -1, NULL, NULL);
    }
    if (pixbuf) {
        cover_draw_cairo(pixbuf, x, min_y, max_y, width, height, cr, CAIRO_FILTER_BEST);
        g_object_unref(pixbuf);
    }
}

void
pl_common_draw_album_art (DdbListview *listview, cairo_t *cr, DB_playItem_t *it, void *user_data, int min_y, int next_y, int x, int y, int width, int height) {
    int art_width = width - ART_PADDING_HORZ * 2;
    int art_height = height - ART_PADDING_VERT * 2;
    if (art_width < 8 || art_height < 8 || !it) {
        return;
    }

    col_info_t *info = user_data;

    int art_x = x + ART_PADDING_HORZ;
    min_y += ART_PADDING_VERT;
    if (info->cover_size == art_width) {
        cover_draw_exact(it, art_x, min_y, next_y, art_width, art_height, cr, user_data);
    }
    else {
        cover_draw_anything(it, art_x, min_y, next_y, art_width, art_height, cr, user_data);
        if (info->cover_load_timeout_id) {
            g_source_remove(info->cover_load_timeout_id);
        }
        info->cover_load_timeout_id = g_timeout_add(1000, cover_load, user_data);
        info->new_cover_size = art_width;
    }
}

#define CHANNEL_BLENDR(CHANNELA,CHANNELB,BLEND) ( \
	sqrt(((1.0 - BLEND) * (CHANNELA * CHANNELA)) + (BLEND * (CHANNELB * CHANNELB))) \
)



PangoAttrList *
convert_escapetext_to_pango_attrlist (char *text, char **plainString, float *fg, float *bg, float *highlight) {
    const int maxTintStops = 100;
    tint_stop_t tintStops[maxTintStops];
    int numTintStops;

    numTintStops = calculate_tint_stops_from_string (text, tintStops, maxTintStops, plainString);

    guint strLength = strlen(*plainString);

    PangoAttrList *lst = pango_attr_list_new ();
    PangoAttribute *attr = NULL;

    // add attributes
    for (guint i = 0; i < numTintStops; i++) {
        int index0 = tintStops[i].index;
        guint len = strLength - index0;

        GdkColor finalColor = { .red = fg[0]*65535, .green = fg[1]*65535, .blue = fg[2]*65535};
        if (tintStops[i].has_rgb) {
            finalColor.red = tintStops[i].r * 65535/255.;
            finalColor.green = tintStops[i].g * 65535/255.;
            finalColor.blue = tintStops[i].b * 65535/255.;
        }

        int tint = tintStops[i].tint;

        if (tint >= 1 && tint <= 3) {
            const float blend[] = {.50f, .25f, 0};
            finalColor.red   = CHANNEL_BLENDR(highlight[0], finalColor.red/65535., blend[tint-1]) * 65535;
            finalColor.green = CHANNEL_BLENDR(highlight[1], finalColor.green/65535., blend[tint-1]) * 65535;
            finalColor.blue  = CHANNEL_BLENDR(highlight[2], finalColor.blue/65535., blend[tint-1]) * 65535;

        } else if (tint >= -3 && tint <= -1) {
            const float blend[] = {.30f, .60f, .80f};
            finalColor.red   = CHANNEL_BLENDR(finalColor.red/65535., bg[0], blend[-tint-1]) * 65535;
            finalColor.green = CHANNEL_BLENDR(finalColor.green/65535., bg[1], blend[-tint-1]) * 65535;
            finalColor.blue  = CHANNEL_BLENDR(finalColor.blue/65535., bg[2], blend[-tint-1]) * 65535;
        }

        attr = pango_attr_foreground_new (finalColor.red, finalColor.green, finalColor.blue);
        attr->start_index = index0;
        attr->end_index = index0 + len;
        pango_attr_list_insert (lst, attr);
    }

    return lst;
}

void
pl_common_draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int iter, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height, int even) {
    col_info_t *info = user_data;

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();

    if (!gtkui_unicode_playstate && it && it == playing_track && info->id == DB_COLUMN_PLAYING) {
        int paused = deadbeef->get_output ()->state () == DDB_PLAYBACK_STATE_PAUSED;
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

        if (pixbuf) {
            gdk_cairo_set_source_pixbuf (cr, pixbuf, x + width/2 - 8, y + height/2 - 8);
            cairo_rectangle (cr, x + width/2 - 8, y + height/2 - 8, 16, 16);
            cairo_fill (cr);
        }
    }
    else if (it) {
        char text[1024] = "";
        int is_dimmed = 0;
        if (it == playing_track && info->id == DB_COLUMN_PLAYING) {
            int paused = deadbeef->get_output ()->state () == DDB_PLAYBACK_STATE_PAUSED;
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
                .id = info->id,
                .idx = idx,
                .flags = DDB_TF_CONTEXT_HAS_ID | DDB_TF_CONTEXT_HAS_INDEX,
            };
            if (!deadbeef->pl_is_selected (it)) {
                ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
            }
            deadbeef->tf_eval (&ctx, info->bytecode, text, sizeof (text));
            is_dimmed = ctx.dimmed;
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
        if (!gtkui_override_listview_colors ()) {
            if (deadbeef->pl_is_selected (it)) {
                color = &gtk_widget_get_style (theme_treeview)->text[GTK_STATE_SELECTED];
            }
            else {
                if (fg_clr) {
                    color = fg_clr;
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
                if (fg_clr) {
                    color = fg_clr;
                }
                else {
                    color = (gtkui_get_listview_playing_text_color (&clr), &clr);
                }
            }
            else {
                if (fg_clr) {
                    color = fg_clr;
                }
                else {
                    color = (gtkui_get_listview_text_color (&clr), &clr);
                }
            }
        }
        float fg[3] = {(float)color->red/0xffff, (float)color->green/0xffff, (float)color->blue/0xffff};
        draw_set_fg_color (&listview->listctx, fg);

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
        cairo_save(cr);
        cairo_rectangle(cr, x+5, y, width-10, height);
        cairo_clip(cr);

        GdkColor *highlight_color;
        GdkColor hlclr;
        if (!gtkui_override_listview_colors ()) {
            highlight_color = &gtk_widget_get_style(theme_treeview)->fg[GTK_STATE_NORMAL];
        }
        else {
            highlight_color = (gtkui_get_listview_group_text_color (&hlclr), &hlclr);
        }

        float highlight[] = {highlight_color->red/65535., highlight_color->green/65535., highlight_color->blue/65535.};

        GdkColor *background_color;
        GdkColor bgclr;

        if (!gtkui_override_listview_colors ()) {
            if (deadbeef->pl_is_selected (it)) {
                background_color = &gtk_widget_get_style (theme_treeview)->bg[GTK_STATE_SELECTED];
            } else {
                background_color = &gtk_widget_get_style(theme_treeview)->bg[GTK_STATE_NORMAL];
            }
        } else {
            if (deadbeef->pl_is_selected (it)) {
                gtkui_get_listview_selection_color (&bgclr);
            } else {
                if (even) {
                    gtkui_get_listview_even_row_color (&bgclr);
                } else {
                    gtkui_get_listview_odd_row_color (&bgclr);
                }
            }
            background_color = &bgclr;
        }
        float bg[] = {background_color->red/65535., background_color->green/65535., background_color->blue/65535.};

        if (is_dimmed) {
            char *plainString;
            PangoAttrList *attrs = convert_escapetext_to_pango_attrlist(text, &plainString, fg, bg, highlight);
            pango_layout_set_attributes (listview->listctx.pangolayout, attrs);
            pango_attr_list_unref(attrs);
            draw_text_custom (&listview->listctx, x + 5, y + 3, width-10, align, DDB_LIST_FONT, bold, italic, plainString);
            free (plainString);
            pango_layout_set_attributes (listview->listctx.pangolayout, NULL);
        } else {
            draw_text_custom (&listview->listctx, x + 5, y + 3, width-10, align, DDB_LIST_FONT, bold, italic, text);
        }

        cairo_restore(cr);
    }
    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }
}

static GtkWidget*
find_popup (GtkWidget *widget)
{
    GtkWidget *parent = widget;
    do {
        widget = parent;
        if (GTK_IS_MENU (widget))
            parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
        else
            parent = gtk_widget_get_parent (widget);
        if (!parent)
            parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
    } while (parent);

    return widget;
}

static DdbListview *
get_context_menu_listview (GtkMenuItem *menuitem) {
    return DDB_LISTVIEW (g_object_get_data (G_OBJECT (find_popup (GTK_WIDGET (menuitem))), "ps"));
}

static int
get_context_menu_column (GtkMenuItem *menuitem) {
    return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (gtk_widget_get_parent (GTK_WIDGET (menuitem))), "column"));
}

static void
add_to_playback_queue_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *listview = get_context_menu_listview (menuitem);
    DB_playItem_t *it = listview->binding->head ();
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_push (it);
        }
        DB_playItem_t *next = listview->binding->next (it);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

static void
remove_from_playback_queue_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *listview = get_context_menu_listview (menuitem);
    DB_playItem_t *it = listview->binding->head ();
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_remove (it);
        }
        DB_playItem_t *next = listview->binding->next (it);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

void
on_cut_activate (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        clipboard_cut_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

void
on_copy_activate (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        clipboard_copy_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

void
on_paste_activate (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        clipboard_paste_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

static void
reload_metadata_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    DdbListview *listview = get_context_menu_listview (menuitem);
    DB_playItem_t *it = listview->binding->head ();
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
        DB_playItem_t *next = listview->binding->next (it);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_save_current ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

static void
properties_activate                (GtkMenuItem     *menuitem,
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

static void
on_remove2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    get_context_menu_listview (menuitem)->binding->delete_selected ();
    deadbeef->pl_save_current();
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
static void
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

static void
on_remove_from_disk_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    delete_from_disk_with_track_list(_menuTrackList);
}

static void
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

void
list_empty_region_context_menu (DdbListview *listview) {
    GtkWidget *playlist_menu;
    GtkWidget *paste;
    GtkWidget *paste_image;
    GtkAccelGroup *accel_group = NULL;
    accel_group = gtk_accel_group_new ();

    playlist_menu = gtk_menu_new ();

    paste = gtk_image_menu_item_new_with_mnemonic (_("_Paste"));
    gtk_widget_show (paste);
    gtk_container_add (GTK_CONTAINER (playlist_menu), paste);
    g_object_set_data (G_OBJECT (paste), "ps", listview);
    gtk_widget_add_accelerator (paste, "activate", accel_group, GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    if (clipboard_is_clipboard_data_available ()) {
        gtk_widget_set_sensitive (paste, TRUE);
    }
    else {
        gtk_widget_set_sensitive (paste, FALSE);
    }

    paste_image = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_MENU);
    gtk_widget_show (paste_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste), paste_image);

    g_signal_connect ((gpointer) paste, "activate",
            G_CALLBACK (on_paste_activate),
            NULL);

    gtk_menu_popup_at_pointer (GTK_MENU (playlist_menu), NULL);
}

void
list_context_menu (DdbListview *listview, DdbListviewIter it, int idx, int iter) {
    _capture_selected_track_list();

    clicked_idx = deadbeef->pl_get_idx_of (it);
    GtkWidget *playlist_menu;
    GtkWidget *add_to_playback_queue1;
    GtkWidget *remove_from_playback_queue1;
    GtkWidget *separator;
    GtkWidget *remove2;
    GtkWidget *remove_from_disk;
    GtkWidget *separator8;
    GtkWidget *cut;
    GtkWidget *cut_image;
    GtkWidget *copy;
    GtkWidget *copy_image;
    GtkWidget *paste;
    GtkWidget *paste_image;
    GtkWidget *separator9;
    GtkWidget *properties1;
    GtkWidget *reload_metadata;

    GtkAccelGroup *accel_group = NULL;
    accel_group = gtk_accel_group_new ();

#ifndef DISABLE_CUSTOM_TITLE
    GtkWidget *set_custom_title;
#endif

    playlist_menu = gtk_menu_new ();
    g_object_set_data (G_OBJECT (playlist_menu), "ps", listview);

    add_to_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Add To Playback Queue"));
    gtk_widget_show (add_to_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), add_to_playback_queue1);

    remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Remove From Playback Queue"));
    if (listview->binding->sel_count () > 1) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int pqlen = deadbeef->playqueue_get_count ();
        int no_playqueue_items = 1;
        for (int i = 0; i < pqlen && no_playqueue_items; i++) {
            DB_playItem_t *pqitem = deadbeef->playqueue_get_item (i);
            if (deadbeef->pl_get_playlist (pqitem) == plt && listview->binding->is_selected (pqitem)) {
                no_playqueue_items = 0;
            }
            listview->binding->unref (pqitem);
        }
        if (no_playqueue_items) {
            gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
        }
    }
    else if (deadbeef->playqueue_test (it) == -1) {
        gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
    }
    gtk_widget_show (remove_from_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_playback_queue1);

    reload_metadata = gtk_menu_item_new_with_mnemonic (_("Reload Metadata"));
    gtk_widget_show (reload_metadata);
    gtk_container_add (GTK_CONTAINER (playlist_menu), reload_metadata);

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

    cut = gtk_image_menu_item_new_with_mnemonic (_("Cu_t"));
    gtk_widget_show (cut);
    gtk_container_add (GTK_CONTAINER (playlist_menu), cut);
    g_object_set_data (G_OBJECT (cut), "ps", listview);
    gtk_widget_add_accelerator (cut, "activate", accel_group, GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    cut_image = gtk_image_new_from_stock ("gtk-cut", GTK_ICON_SIZE_MENU);
    gtk_widget_show (cut_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (cut), cut_image);

    copy = gtk_image_menu_item_new_with_mnemonic (_("_Copy"));
    gtk_widget_show (copy);
    gtk_container_add (GTK_CONTAINER (playlist_menu), copy);
    g_object_set_data (G_OBJECT (copy), "ps", listview);
    gtk_widget_add_accelerator (copy, "activate", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    copy_image = gtk_image_new_from_stock ("gtk-copy", GTK_ICON_SIZE_MENU);
    gtk_widget_show (copy_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (copy), copy_image);

    paste = gtk_image_menu_item_new_with_mnemonic (_("_Paste"));
    if (iter != PL_SEARCH) {
        gtk_widget_show (paste);
    }
    else {
        gtk_widget_hide (paste);
    }
    gtk_container_add (GTK_CONTAINER (playlist_menu), paste);
    g_object_set_data (G_OBJECT (paste), "ps", listview);
    gtk_widget_add_accelerator (paste, "activate", accel_group, GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    if (clipboard_is_clipboard_data_available ()) {
        gtk_widget_set_sensitive (paste, TRUE);
    }
    else {
        gtk_widget_set_sensitive (paste, FALSE);
    }

    paste_image = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_MENU);
    gtk_widget_show (paste_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste), paste_image);

    separator9 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator9);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator9);
    gtk_widget_set_sensitive (separator9, FALSE);

    remove2 = gtk_menu_item_new_with_mnemonic (_("Remove"));
    gtk_widget_show (remove2);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);

    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);

    if (!hide_remove_from_disk) {
        remove_from_disk = gtk_menu_item_new_with_mnemonic (_("Remove From Disk"));
        gtk_widget_show (remove_from_disk);
        gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_disk);
    }

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

    int selected_count = 0;
    DB_playItem_t *pit = listview->binding->head ();
    DB_playItem_t *selected = NULL;
    while (pit) {
        if (deadbeef->pl_is_selected (pit))
        {
            if (!selected)
                selected = pit;
            selected_count++;
        }
        DB_playItem_t *next = listview->binding->next (pit);
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

                    popup = GTK_WIDGET (g_object_get_data (G_OBJECT (find_popup (prev_menu)), name));
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

    g_signal_connect ((gpointer) add_to_playback_queue1, "activate",
            G_CALLBACK (add_to_playback_queue_activate),
            NULL);
    g_signal_connect ((gpointer) remove_from_playback_queue1, "activate",
            G_CALLBACK (remove_from_playback_queue_activate),
            NULL);
    g_signal_connect ((gpointer) reload_metadata, "activate",
            G_CALLBACK (reload_metadata_activate),
            NULL);
    g_signal_connect ((gpointer) cut, "activate",
            G_CALLBACK (on_cut_activate),
            NULL);
    g_signal_connect ((gpointer) copy, "activate",
            G_CALLBACK (on_copy_activate),
            NULL);
    g_signal_connect ((gpointer) paste, "activate",
            G_CALLBACK (on_paste_activate),
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
            G_CALLBACK (properties_activate),
            NULL);

    gtk_menu_popup_at_pointer (GTK_MENU (playlist_menu), NULL);
}

static char *
strtok_stringdelim_r (char *str, const char *delim,  char **next_start)
{
    char *end;

    if (*next_start) {
        str = *next_start;
    }

    if (!str || str[0] == 0) {
        *next_start = NULL;
        return NULL;
    }

    char *next = strstr(str, delim);

    if (next) {
        *next = 0;
        *next_start = next + strlen(delim);
    }
    else {
        *next_start = str + strlen(str);
    }

    return str;
}

static void
groups_changed (DdbListview *listview, const char *format)
{
    if (!format) {
        return;
    }
    DdbListviewGroupFormat *fmt = listview->group_formats;
    while (fmt) {
        DdbListviewGroupFormat *next_fmt = fmt->next;
        free (fmt->format);
        free (fmt->bytecode);
        free (fmt);
        fmt = next_fmt;
    }
    listview->group_formats = NULL;
    char *esc_format = parser_escape_string (format);
    char quoted_format[strlen (esc_format) + 3];
    snprintf (quoted_format, sizeof (quoted_format), "\"%s\"", esc_format);
    listview->binding->groups_changed (quoted_format);
    free (esc_format);

    char *mutable_format = strdup (format);
    fmt = NULL;
    char *saveptr = NULL;
    char *token;
    while ((token = strtok_stringdelim_r(mutable_format, SUBGROUP_DELIMITER, &saveptr)) != NULL) {
        if (strlen(token) > 0) {
            DdbListviewGroupFormat *new_fmt = calloc(sizeof(DdbListviewGroupFormat), 1);
            if (!fmt) {
                listview->group_formats = new_fmt;
                fmt = listview->group_formats;
            }
            else {
                fmt->next = new_fmt;
                fmt = fmt->next;
            }
            fmt->format = strdup (token);
            fmt->bytecode = deadbeef->tf_compile (fmt->format);
        }
    }
    free (mutable_format);

    // always have at least one
    if (!listview->group_formats) {
        listview->group_formats = calloc(sizeof(DdbListviewGroupFormat), 1);
        fmt = listview->group_formats;
        fmt->format = strdup ("");
        fmt->bytecode = deadbeef->tf_compile (fmt->format);
    }

    ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_LIST);
}

gboolean
list_handle_keypress (DdbListview *ps, int keyval, int state, int iter) {
    int prev = ps->binding->cursor ();
    int cursor = prev;
    GtkWidget *range = ps->scrollbar;
    GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (range));

    state &= (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD4_MASK);

    if (state & GDK_CONTROL_MASK) {
        int res = 0;
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            if (keyval == GDK_c) {
                clipboard_copy_selection (plt, DDB_ACTION_CTX_SELECTION);
                res = 1;
            }
            else if (keyval == GDK_v && iter != PL_SEARCH) {
                clipboard_paste_selection (plt, DDB_ACTION_CTX_SELECTION);
                res = 1;
            }
            else if (keyval == GDK_x) {
                clipboard_cut_selection (plt, DDB_ACTION_CTX_SELECTION);
                res = 1;
            }
            deadbeef->plt_unref (plt);
            return res;
        }
    }

    if (state & ~GDK_SHIFT_MASK) {
        return FALSE;
    }

    if (keyval == GDK_Down) {
        if (cursor < ps->binding->count () - 1) {
            cursor++;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
        }
    }
    else if (keyval == GDK_Up) {
        if (cursor > 0) {
            cursor--;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
            if (cursor < 0 && ps->binding->count () > 0) {
                cursor = 0;
            }
        }
    }
    else if (keyval == GDK_Page_Down) {
        if (cursor < ps->binding->count () - 1) {
            cursor += 10;
            if (cursor >= ps->binding->count ()) {
                cursor = ps->binding->count () - 1;
            }
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
        }
    }
    else if (keyval == GDK_Page_Up) {
        if (cursor > 0) {
            cursor -= 10;
            if (cursor < 0) {
                gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
                cursor = 0;
            }
        }
        else {
            if (cursor < 0 && ps->binding->count () > 0) {
                cursor = 0;
            }
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
        }
    }
    else if (keyval == GDK_End) {
        cursor = ps->binding->count () - 1;
        gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
    }
    else if (keyval == GDK_Home) {
        cursor = 0;
        gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
    }
    else {
        return FALSE;
    }

    if (state & GDK_SHIFT_MASK) {
        if (cursor != prev) {
            int newscroll = ps->scrollpos;
            int cursor_scroll = ddb_listview_get_row_pos (ps, cursor, NULL);
            if (cursor_scroll < ps->scrollpos) {
                newscroll = cursor_scroll;
            }
            else if (cursor_scroll >= ps->scrollpos + ps->list_height) {
                newscroll = cursor_scroll - ps->list_height + 1;
                if (newscroll < 0) {
                    newscroll = 0;
                }
            }
            if (ps->scrollpos != newscroll) {
                GtkWidget *range = ps->scrollbar;
                gtk_range_set_value (GTK_RANGE (range), newscroll);
            }

            // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
            int start = min (cursor, ps->shift_sel_anchor);
            int end = max (cursor, ps->shift_sel_anchor);

            ddb_listview_select_range (ps, start, end);
            ddb_listview_update_cursor (ps, cursor);
        }
    }
    else {
        ps->shift_sel_anchor = cursor;
        ddb_listview_set_cursor_and_scroll (ps, cursor);
    }

    return TRUE;
}

static void
on_group_by_none_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    groups_changed (get_context_menu_listview (menuitem) , "");
}

static void
on_pin_groups_active                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int old_val = deadbeef->conf_get_int ("playlist.pin.groups", 0);
    deadbeef->conf_set_int ("playlist.pin.groups", old_val ? 0 : 1);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)"playlist.pin.groups", 0, 0);
    gtk_check_menu_item_toggled(GTK_CHECK_MENU_ITEM(menuitem));
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref(plt);
    }
}

static void
on_group_by_artist_date_album_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    groups_changed (get_context_menu_listview (menuitem), "%album artist% - ['['%year%']' ]%album%");
}

static void
on_group_by_artist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    groups_changed (get_context_menu_listview (menuitem), "%artist%");
}


static void
_make_format (char *format, size_t size, DdbListviewGroupFormat *fmt) {
    format[0] = 0;
    while (fmt) {
        if (format[0] != 0) {
            strncat(format, SUBGROUP_DELIMITER, size - strlen(format) - 1);
        }
        strncat(format, fmt->format, size - strlen(format) - 1);
        fmt = fmt->next;
    }
}

static void
on_group_by_custom_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_groupbydlg ();

    DdbListview *listview = get_context_menu_listview (menuitem);
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    GtkWidget *entry = lookup_widget (dlg, "format");
    char format[1024];

    _make_format(format, sizeof (format), listview->group_formats);

    gtk_entry_set_text (GTK_ENTRY (entry), format);
//    gtk_window_set_title (GTK_WINDOW (dlg), "Group by");
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));

    if (response == GTK_RESPONSE_OK) {
        const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_modified (plt);
            deadbeef->plt_unref (plt);
        }
        groups_changed (listview, text);
    }
    gtk_widget_destroy (dlg);
}

int
pl_common_load_column_config (DdbListview *listview, const char *key) {
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
        json_t *title, *align, *id, *format, *sort_format, *width, *color_override, *color;
        json_t *data = json_array_get (root, i);
        if (!json_is_object (data)) {
            goto error;
        }
        title = json_object_get (data, "title");
        align = json_object_get (data, "align");
        id = json_object_get (data, "id");
        format = json_object_get (data, "format");
        sort_format = json_object_get (data, "sort_format");
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
        const char *ssort_format = NULL;
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
        if (json_is_string (sort_format)) {
            ssort_format = json_string_value (sort_format);
            if (!ssort_format[0]) {
                ssort_format = NULL;
            }
        }
        if (json_is_string (width)) {
            iwidth = atoi (json_string_value (width));
            if (iwidth < 0) {
                iwidth = 50;
            }
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

        col_info_t *inf = create_col_info(listview, iid);
        if (sformat) {
            inf->format = strdup (sformat);
            inf->bytecode = deadbeef->tf_compile (inf->format);
        }
        if (ssort_format) {
            inf->sort_format = strdup (ssort_format);
            inf->sort_bytecode = deadbeef->tf_compile (inf->sort_format);
        }
        ddb_listview_column_append (listview, stitle, iwidth, ialign, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, icolor_override, gdkcolor, inf);
    }
    json_decref(root);
    return 0;
error:
    fprintf (stderr, "%s config variable contains invalid data, ignored\n", key);
    json_decref(root);
    return -1;
}

static void
init_column (col_info_t *inf, int id, const char *format, const char *sort_format) {
    if (inf->format) {
        free (inf->format);
        inf->format = NULL;
    }
    if (inf->sort_format) {
        free (inf->sort_format);
        inf->sort_format = NULL;
    }
    if (inf->bytecode) {
        deadbeef->tf_free (inf->bytecode);
        inf->bytecode = NULL;
    }
    if (inf->sort_bytecode) {
        deadbeef->tf_free (inf->sort_bytecode);
        inf->sort_bytecode = NULL;
    }

    inf->id = pl_preset_column_formats[id].id;

    if (pl_preset_column_formats[id].id == DB_COLUMN_CUSTOM) {
        inf->format = strdup (format);
    } else if (pl_preset_column_formats[id].id == DB_COLUMN_STANDARD && pl_preset_column_formats[id].format) {
        inf->format = strdup (pl_preset_column_formats[id].format);
    }

    if (inf->format) {
        inf->bytecode = deadbeef->tf_compile (inf->format);
    }

    if (sort_format) {
        inf->sort_format = strdup(sort_format);
        inf->sort_bytecode = deadbeef->tf_compile (inf->sort_format);
    }
}

static void
populate_column_id_combo_box(GtkComboBoxText *combo_box) {
    for (int i = 0; i < PRESET_COLUMN_NUMITEMS; i++) {
        gtk_combo_box_text_append_text (combo_box, pl_preset_column_formats[i].title);
    }
}

int editcolumn_title_changed = 0;

static void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    editcolumn_title_changed = 0;
    GdkColor color;
    gtkui_get_listview_text_color (&color);

    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Add column"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    populate_column_id_combo_box (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")), 0);

    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &color);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        const gchar *sort_format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "sort_format")));
        int sel = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));

        int clr_override = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")));
        GdkColor clr;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &clr);

        col_info_t *inf = create_col_info(user_data, 0);
        init_column (inf, sel, format, sort_format);

        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        DdbListview *listview = get_context_menu_listview (menuitem);
        int before = get_context_menu_column (menuitem);
        ddb_listview_column_insert (listview, before, title, 100, align, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, clr_override, clr, inf);
        ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL);
    }
    gtk_widget_destroy (dlg);
}

static void
on_edit_column_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int active_column = get_context_menu_column (menuitem);
    if (active_column == -1)
        return;

    DdbListview *listview = get_context_menu_listview (menuitem);
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Edit column"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    populate_column_id_combo_box (GTK_COMBO_BOX (lookup_widget (dlg, "id")));

    const char *title;
    int width;
    int align_right;
    col_info_t *inf;
    int color_override;
    GdkColor color;
    int res = ddb_listview_column_get_info (listview, active_column, &title, &width, &align_right, NULL, NULL, &color_override, &color, (void **)&inf);
    if (res == -1) {
        trace ("attempted to edit non-existing column\n");
        return;
    }

    int idx = -1;

    // get index for id
    if (inf->id == DB_COLUMN_STANDARD) {
        for (int i = 0; i < PRESET_COLUMN_NUMITEMS; i++) {
            if (pl_preset_column_formats[i].id == DB_COLUMN_STANDARD && inf->format &&
                pl_preset_column_formats[i].format && !strcmp (inf->format, pl_preset_column_formats[i].format)) {
                idx = i;
                break;
            }
        }
        // old style custom column
        if (idx == -1) {
            idx = find_first_preset_column_type(DB_COLUMN_CUSTOM);
        }
    } else {
        idx = find_first_preset_column_type(inf->id);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), idx);
    if (idx == PRESET_COLUMN_NUMITEMS-1) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "format")), inf->format);
    }
    if (inf->sort_format) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "sort_format")), inf->sort_format);
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
        const gchar *sort_format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "sort_format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));

        int clr_override = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")));
        GdkColor clr;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &clr);

        init_column (inf, id, format, sort_format);
        ddb_listview_column_set_info (listview, active_column, title, width, align, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, clr_override, clr, inf);

        ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST);
    }
    gtk_widget_destroy (dlg);
}


static void
on_remove_column_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int active_column = get_context_menu_column (menuitem);
    if (active_column == -1)
        return;

    DdbListview *listview = get_context_menu_listview (menuitem);
    ddb_listview_column_remove (listview, active_column);
    ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL);
}

static GtkWidget*
create_headermenu (DdbListview *listview, int column, int groupby)
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

    if (column == -1) {
        gtk_widget_set_sensitive (edit_column, FALSE);
        gtk_widget_set_sensitive (remove_column, FALSE);
    }

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
                      listview);
    g_signal_connect ((gpointer) edit_column, "activate",
                      G_CALLBACK (on_edit_column_activate),
                      listview);
    g_signal_connect ((gpointer) remove_column, "activate",
                      G_CALLBACK (on_remove_column_activate),
                      listview);

    return headermenu;
}

void
pl_common_header_context_menu (DdbListview *ps, int column) {
    GtkWidget *menu = create_headermenu (ps, column, 1);
    g_object_set_data (G_OBJECT (menu), "ps", ps);
    g_object_set_data (G_OBJECT (menu), "column", GINT_TO_POINTER (column));

    gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

void
pl_common_add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, const char *sort_format, int align_right) {
    if (!format) {
        format = "";
    }
    if (!sort_format) {
        sort_format = "";
    }
    col_info_t *inf = create_col_info(listview, id);
    inf->format = strdup (format);
    inf->bytecode = deadbeef->tf_compile (inf->format);
    inf->sort_format = strdup (sort_format);
    inf->sort_bytecode = deadbeef->tf_compile (inf->sort_format);
    GdkColor color = { 0, 0, 0, 0 };
    ddb_listview_column_append (listview, title, width, align_right, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, 0, color, inf);
}

int
pl_common_get_group (DdbListview *listview, DdbListviewIter it, char *str, int size, int index) {
    if (!listview->group_formats->format || !listview->group_formats->format[0]) {
        return -1;
    }
    DdbListviewGroupFormat *fmt = listview->group_formats;
    while (index > 0) {
        index--;
        fmt = fmt->next;
        if (fmt == NULL) {
            return -1;
        }
    }
    if (fmt->bytecode) {
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .it = it,
            .plt = deadbeef->plt_get_curr (),
            .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
        };
        deadbeef->tf_eval (&ctx, fmt->bytecode, str, size);
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
    return fmt->next == NULL ? 0 : 1;
}

void
pl_common_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int iter, int x, int y, int width, int height, int group_depth) {
    if (listview->group_formats->format && listview->group_formats->format[0]) {
        char str[1024] = "";
        DdbListviewGroupFormat *fmt = listview->group_formats;
        while (group_depth--) {
            fmt = fmt->next;
        }
        int isdimmed=0;
        if (fmt->bytecode) {
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .it = it,
                .plt = deadbeef->plt_get_curr (),
                .flags = DDB_TF_CONTEXT_NO_DYNAMIC | DDB_TF_CONTEXT_TEXT_DIM,
                .iter = iter,
            };
            deadbeef->tf_eval (&ctx, fmt->bytecode, str, sizeof (str));
            if (ctx.plt) {
                deadbeef->plt_unref (ctx.plt);
                ctx.plt = NULL;
            }
            isdimmed = ctx.dimmed;


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
        GdkColor clr;
        if (theming) {
            clr = gtk_widget_get_style(theme_treeview)->fg[GTK_STATE_NORMAL];
        }
        else {
            gtkui_get_listview_group_text_color (&clr);
        }
        float rgb[] = {clr.red/65535., clr.green/65535., clr.blue/65535.};
        draw_set_fg_color (&listview->grpctx, rgb);

        int ew;
        int text_width = width-x-10;
        if (text_width > 0) {
            if (isdimmed) {
                GdkColor *highlight_color;
                GdkColor hlclr;
                if (!gtkui_override_listview_colors ()) {
                    highlight_color = &gtk_widget_get_style(theme_treeview)->fg[GTK_STATE_NORMAL];
                }
                else {
                    highlight_color = (gtkui_get_listview_group_text_color (&hlclr), &hlclr);
                }

                float highlight[] = {highlight_color->red/65535., highlight_color->green/65535., highlight_color->blue/65535.};

                GdkColor *background_color;
                GdkColor bgclr;

                if (!gtkui_override_listview_colors ()) {
                    background_color = &gtk_widget_get_style(theme_treeview)->bg[GTK_STATE_NORMAL];
                } else {
                    gtkui_get_listview_odd_row_color (&bgclr);
                    background_color = &bgclr;
                }
                float bg[] = {background_color->red/65535., background_color->green/65535., background_color->blue/65535.};

                PangoAttrList *attrs;
                char *plainString;

                attrs = convert_escapetext_to_pango_attrlist(str, &plainString, rgb, bg, highlight);
                pango_layout_set_attributes (listview->grpctx.pangolayout, attrs);
                pango_attr_list_unref(attrs);
                draw_text_custom (&listview->grpctx, x + 5, y + height/2 - draw_get_listview_rowheight (&listview->grpctx)/2 + 3, text_width, 0, DDB_GROUP_FONT, 0, 0, plainString);
                free (plainString);
                pango_layout_set_attributes (listview->grpctx.pangolayout, NULL);
            } else {
                draw_text_custom (&listview->grpctx, x + 5, y + height/2 - draw_get_listview_rowheight (&listview->grpctx)/2 + 3, text_width, 0, DDB_GROUP_FONT, 0, 0, str);
            }
            draw_get_layout_extents (&listview->grpctx, &ew, NULL);

            int len = strlen (str);
            int line_x = x + 10 + ew + (len ? ew / len / 2 : 0);
            if (line_x+20 < x + width) {
                draw_line (&listview->grpctx, line_x, y+height/2, x+width, y+height/2);
            }
        }
    }
}

void
pl_common_selection_changed (DdbListview *ps, int iter, DB_playItem_t *it) {
    if (it) {
        ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_TRACKINFOCHANGED);
        ev->track = it;
        deadbeef->pl_item_ref(ev->track);
        deadbeef->event_send((ddb_event_t *)ev, DDB_PLAYLIST_CHANGE_SELECTION, iter);
    }
    else {
        deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, (uintptr_t)ps, DDB_PLAYLIST_CHANGE_SELECTION, iter);
    }
}

void
pl_common_col_sort (int sort_order, int iter, void *user_data) {
    col_info_t *c = (col_info_t*)user_data;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    char *format = (c->sort_format && strlen(c->sort_format)) ? c->sort_format : c->format;
    deadbeef->plt_sort_v2 (plt, iter, c->id, format, sort_order == 2 ? DDB_SORT_DESCENDING : DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
}

void
pl_common_set_group_format (DdbListview *listview, const char *format_conf, const char *artwork_level_conf,  const char *subgroup_padding_conf) {
    deadbeef->conf_lock ();
    char *format = strdup (deadbeef->conf_get_str_fast (format_conf, ""));
    listview->artwork_subgroup_level = deadbeef->conf_get_int (artwork_level_conf, 0);
    listview->subgroup_title_padding = deadbeef->conf_get_int (subgroup_padding_conf, 10);
    deadbeef->conf_unlock ();
    parser_unescape_quoted_string (format);
    listview->group_formats = NULL;

    DdbListviewGroupFormat *fmt = NULL;
    char *saveptr = NULL;
    char *token;
    while ((token = strtok_stringdelim_r(format, SUBGROUP_DELIMITER, &saveptr)) != NULL) {
        if (strlen(token) > 0) {
            DdbListviewGroupFormat *new_fmt = calloc(sizeof(DdbListviewGroupFormat), 1);
            if (!fmt) {
                listview->group_formats = new_fmt;
                fmt = listview->group_formats;
            }
            else {
                fmt->next = new_fmt;
                fmt = fmt->next;
            }
            fmt->format = strdup (token);
            fmt->bytecode = deadbeef->tf_compile (fmt->format);
        }
    }

    free (format);

    // always have at least one
    if (!listview->group_formats) {
        listview->group_formats = calloc(sizeof(DdbListviewGroupFormat), 1);
        fmt = listview->group_formats;
        fmt->format = strdup ("");
        fmt->bytecode = deadbeef->tf_compile (fmt->format);
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
        format = COLUMN_FORMAT_ARTISTALBUM;
        break;
    case DB_COLUMN_ARTIST:
        format = COLUMN_FORMAT_ARTIST;
        break;
    case DB_COLUMN_ALBUM:
        format = COLUMN_FORMAT_ALBUM;
        break;
    case DB_COLUMN_TITLE:
        format = COLUMN_FORMAT_TITLE;
        break;
    case DB_COLUMN_DURATION:
        format = COLUMN_FORMAT_LENGTH;
        break;
    case DB_COLUMN_TRACK:
        format = COLUMN_FORMAT_TRACKNUMBER;
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
