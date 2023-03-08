/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>
#include "../covermanager/covermanager.h"
#include "../covermanager/gobjcache.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "playlistrenderer.h"
#include "plcommon.h"

#define min(x,y) ((x)<(y)?(x):(y))

extern DB_functions_t *deadbeef;

static gboolean
tf_redraw_cb (gpointer user_data) {
    DdbListview *lv = user_data;
    ddb_listview_redraw_tf(lv);
    return FALSE;
}

void
pl_common_draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int iter, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height, int even) {
    col_info_t *info = user_data;

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track_safe ();

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
                int idx;

                if ((ctx.flags & DDB_TF_CONTEXT_HAS_INDEX) && ctx.iter == PL_MAIN) {
                    idx = ctx.idx;
                }
                else {
                    idx = deadbeef->plt_get_item_idx (ctx.plt, it, ctx.iter);
                }

                ddb_listview_schedule_draw_tf(listview, idx, g_timeout_add (ctx.update, tf_redraw_cb, listview), it);
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
        GdkColor temp_color;
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
            if (deadbeef->pl_is_selected (it)) {
                color = ((void)(gtkui_get_listview_selected_text_color (&temp_color)), &temp_color);
            }
            else if (it && it == playing_track) {
                if (fg_clr) {
                    color = fg_clr;
                }
                else {
                    color = ((void)(gtkui_get_listview_playing_text_color (&temp_color)), &temp_color);
                }
            }
            else {
                if (fg_clr) {
                    color = fg_clr;
                }
                else {
                    color = ((void)(gtkui_get_listview_text_color (&temp_color)), &temp_color);
                }
            }
        }
        float fg[3] = {(float)color->red/0xffff, (float)color->green/0xffff, (float)color->blue/0xffff};
        drawctx_t * const listctx = ddb_listview_get_listctx(listview);
        draw_set_fg_color (listctx, fg);

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
            highlight_color = ((void)(gtkui_get_listview_group_text_color (&hlclr)), &hlclr);
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

            drawctx_t * const listctx = ddb_listview_get_listctx(listview);

            pango_layout_set_attributes (listctx->pangolayout, attrs);
            pango_attr_list_unref(attrs);
            draw_text_custom (listctx, x + 5, y + 3, width-10, align, DDB_LIST_FONT, bold, italic, plainString);
            free (plainString);
            pango_layout_set_attributes (listctx->pangolayout, NULL);
        } else {
            draw_text_custom (listctx, x + 5, y + 3, width-10, align, DDB_LIST_FONT, bold, italic, text);
        }

        cairo_restore(cr);
    }
    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }
}

void
main_draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height, int even)
{
    pl_common_draw_column_data (listview, cr, it, idx, PL_MAIN, align, user_data, fg_clr, x, y, width, height, even);
}

void
main_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int x, int y, int width, int height, int group_depth)
{
    pl_common_draw_group_title (listview, drawable, it, PL_MAIN, x, y, width, height, group_depth);
}

static void
cover_draw_cairo (GdkPixbuf *pixbuf, int x, int min_y, int max_y, int width, int height, cairo_t *cr, int filter) {
    int pw = gdk_pixbuf_get_width(pixbuf);
    int ph = gdk_pixbuf_get_height(pixbuf);
    int real_y = min(min_y, max_y - ph);
    cairo_save(cr);
    cairo_rectangle(cr, x, min_y, width, max_y - min_y);
    cairo_translate (cr, x, real_y);
    if (pw > width || ph > height || (pw < width && ph < height)) {
        const double scale = min(width/(double)pw, height/(double)ph);
        cairo_translate(cr, (width - width*scale)/2., min(min_y, max_y - ph*scale) - real_y);
        cairo_scale(cr, scale, scale);
        cairo_pattern_set_filter(cairo_get_source(cr), filter);
    }
    gdk_cairo_set_source_pixbuf(cr, pixbuf, (width - pw)/2., 0);
    cairo_fill(cr);
    cairo_restore(cr);
}

void
pl_common_draw_album_art (DdbListview *listview, cairo_t *cr, DdbListviewGroup *grp, void *user_data, int min_y, int next_y, int x, int y, int width, int height, int alignment) {
    int art_width = width - ART_PADDING_HORZ * 2;
    int art_height = height - ART_PADDING_VERT * 2;
    if (art_width < 8 || art_height < 8 || !grp->head) {
        return;
    }

    DB_playItem_t *it = (DB_playItem_t *)grp->head;
    if (it == NULL) {
        return;
    }

    covermanager_t *cm = covermanager_shared();

    GdkPixbuf *image = NULL;

    deadbeef->pl_item_ref (it);
    image = covermanager_cover_for_track(cm, it, 0, ^(GdkPixbuf *img) { // img only valid in this block
        deadbeef->pl_item_unref (it);

        gtk_widget_queue_draw(GTK_WIDGET(listview));
        // FIXME: redraw only the group rect
    });
    if (image != NULL) { // completion block won't be called
        deadbeef->pl_item_unref (it);
        it = NULL;
    }
    if (image == NULL) {
        // FIXME: the problem here is that if the cover is not found (yet) -- it won't draw anything, but the rect is already invalidated, and will come out as background color
        return;
    }

    // image is retained: release at the end

    int art_x = x + ART_PADDING_HORZ;
    min_y += ART_PADDING_VERT;

    GtkAllocation size = {0};
    size.width = gdk_pixbuf_get_width(image);
    size.height = gdk_pixbuf_get_height(image);
    GtkAllocation availableSize = {0};
    availableSize.width = art_width;
    availableSize.height = art_height;
    GtkAllocation desiredSize = covermanager_desired_size_for_image_size(cm, size, availableSize);

    GdkPixbuf *scaled_image = covermanager_create_scaled_image(cm, image, desiredSize);
    int scaled_image_width = gdk_pixbuf_get_width(scaled_image);
    int scaled_image_height = gdk_pixbuf_get_height(scaled_image);

    // center horizontally
    if (size.width < size.height) {
        if (alignment == 1) { // align
            art_x += art_width - scaled_image_width;
        }
        else if (alignment == 2) { // center
            art_x += art_width/2 - scaled_image_width/2;
        }
    }

    cover_draw_cairo(scaled_image, art_x, min_y, next_y, scaled_image_width, scaled_image_height, cr, CAIRO_FILTER_FAST);
    g_object_unref(scaled_image);

    gobj_unref(image);
    image = NULL;
}

