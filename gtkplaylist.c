/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include "gtkplaylist.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "playlist.h"
#include "playback.h"
#include "codec.h"
#include "common.h"
#include "messagepump.h"
#include "messages.h"
#include "streamer.h"
#include "search.h"
#include "progress.h"
#include "drawing.h"
#include "session.h"

// orange on dark color scheme
float colo_dark_orange[COLO_COUNT][3] = {
    { 0x7f/255.f, 0x7f/255.f, 0x7f/255.f }, // cursor
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // odd
    { 0x21/255.f, 0x23/255.f, 0x1f/255.f }, // even
    { 0xaf/255.f, 0xa7/255.f, 0x9e/255.f }, // sel odd
    { 0xa7/255.f, 0x9f/255.f, 0x96/255.f }, // sel even
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // text
    { 0,          0,          0          }, // sel text
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // seekbar back
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // seekbar front
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // volumebar back
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // volumebar front
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // dragdrop marker
};

float colo_white_blue[COLO_COUNT][3] = {
    { 0x7f/255.f, 0x7f/255.f, 0x7f/255.f }, // cursor
    { 1,          1,          1          }, // odd
    { 0xea/255.f, 0xeb/255.f, 0xec/255.f }, // even
    { 0x24/255.f, 0x89/255.f, 0xb8/255.f }, // sel odd
    { 0x20/255.f, 0x85/255.f, 0xb4/255.f }, // sel even
    { 0,          0,          0          }, // text
    { 1,          1,          1          }, // sel text
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // seekbar back
    { 0x24/255.f, 0x89/255.f, 0xb8/255.f }, // seekbar front
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // volumebar back
    { 0x24/255.f, 0x89/255.f, 0xb8/255.f }, // volumebar front
    { 0x09/255.f, 0x22/255.f, 0x3a/255.f }, // dragdrop marker
};

// current color scheme
float colo_current[COLO_COUNT][3];

// playlist row height
int rowheight = -1;

const char *colnames[pl_ncolumns] = {
    "Playing",
    "Artist / Album",
    "Track №",
    "Title / Track Artist",
    "Duration"
};

static uintptr_t play16_pixbuf;
static uintptr_t pause16_pixbuf;

// that must be called before gtk_init
void
gtkpl_init (void) {
    //memcpy (colo_current, colo_system_gtk, sizeof (colo_current));
    //memcpy (colo_current, colo_dark_orange, sizeof (colo_current));
    play16_pixbuf = draw_load_pixbuf ("play_16.png");
    pause16_pixbuf = draw_load_pixbuf ("pause_16.png");
    rowheight = draw_get_font_size () + 10;
    memcpy (colo_current, colo_white_blue, sizeof (colo_current));
}

void
theme_set_cairo_source_rgb (cairo_t *cr, int col) {
    cairo_set_source_rgb (cr, colo_current[col][0], colo_current[col][1], colo_current[col][2]);
}

void
theme_set_fg_color (int col) {
    draw_set_fg_color (colo_current[col]);
}

void
theme_set_bg_color (int col) {
    draw_set_bg_color (colo_current[col]);
}

#if 0
static int avg_glyph_width = -1;
static int tridot_len = -1;

int
gtkpl_fit_text (char *out, int *dotpos, int len, const char *in, int width) {
    if (avg_glyph_width == -1) {
        int h;
        draw_get_text_extents ("a", 1, &avg_glyph_width, &h);
        tridot_len = strlen ("…");
    }
    int l = strlen (in);
    len--;
    l = min (len, l);
    strncpy (out, in, l);
    out[l] = 0;
    int w = 0;

    if (dotpos) {
        *dotpos = -1;
    }
    return width;
#if 1
    // initial approximation
    int u8len = g_utf8_strlen (in, l);
    int apx_len = width / avg_glyph_width;
    char *p;
    if (apx_len >= u8len) {
        int h;
        draw_get_text_extents (out, l, &w, &h);
        if (w <= width) {
            return w;
        }
        // start from end
        p = g_utf8_find_prev_char (out, &out[l]);
    }
    else {
        p = &out[l];
        while (u8len > apx_len) {
            p = g_utf8_find_prev_char (out, p);
            l = p - out;
            if (!p) {
                strcpy (out, "…");
                return tridot_len;
            }
            u8len--;
        }
        for (;;) {
            int h;
            draw_get_text_extents (out, l, &w, &h);
            if (w > width) {
                break;
            }
            char *next = g_utf8_find_next_char (p, NULL);
            if (!next) {
                break;
            }
            if (next - out == l) {
                break;
            }
            l = next - out;
            p = next;
        }
    }
#else
    char *p = &out[l];
    p = g_utf8_find_prev_char (out, p);
#endif
    for (;;) {
        int h;
        draw_get_text_extents (out, l, &w, &h);
        if (w <= width) {
            break;
        }
        char *prev = g_utf8_find_prev_char (out, p);

        if (!prev) {
            break;
        }
        strcpy (prev, "…");
        l = prev - out + tridot_len;
        p = prev;
        if (dotpos) {
            *dotpos = p-out;
        }
    }
    return w;
}
#endif

void
gtkpl_setup_scrollbar (gtkplaylist_t *ps) {
    GtkWidget *playlist = ps->playlist;
    int h = playlist->allocation.height / rowheight;
    int size = (*ps->pcount);
    if (h >= size) {
        size = 0;
    }
    GtkWidget *scroll = ps->scrollbar;
    if (ps->row >= (*ps->pcount)) {
        ps->row = (*ps->pcount) - 1;
    }
    if (ps->scrollpos > (*ps->pcount)-ps->nvisiblerows+1) {
        int n = (*ps->pcount) - ps->nvisiblerows + 1;
        ps->scrollpos = max (0, n);
        gtk_range_set_value (GTK_RANGE (scroll), ps->scrollpos);
    }
    if (size == 0) {
        gtk_widget_hide (scroll);
    }
    else {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, size, 1, h, h);
        gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
        gtk_widget_show (scroll);
    }
}

void
gtkpl_setup_hscrollbar (gtkplaylist_t *ps) {
    GtkWidget *playlist = ps->playlist;
    int w = playlist->allocation.width;
    int size = 0;
    int i;
    for (i = 0; i < pl_ncolumns; i++) {
        size += ps->colwidths[i];
    }
    if (w >= size) {
        size = 0;
    }
    GtkWidget *scroll = ps->hscrollbar;
    if (ps->hscrollpos >= size-w) {
        int n = size-w-1;
        ps->hscrollpos = max (0, n);
        gtk_range_set_value (GTK_RANGE (scroll), ps->hscrollpos);
    }
    if (size == 0) {
        gtk_widget_hide (scroll);
    }
    else {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, size, 1, w, w);
        gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
        gtk_widget_show (scroll);
    }
}

void
gtkpl_redraw_pl_row_novis (gtkplaylist_t *ps, int row, playItem_t *it) {
    draw_begin ((uintptr_t)ps->backbuf);
	if (it) {
        gtkpl_draw_pl_row_back (ps, row, it);
        gtkpl_draw_pl_row (ps, row, it);
    }
    draw_end ();
}

void
gtkpl_redraw_pl_row (gtkplaylist_t *ps, int row, playItem_t *it) {
    int x, y, w, h;
    GtkWidget *widget = ps->playlist;
    x = 0;
    y = (row  - ps->scrollpos) * rowheight;
    w = widget->allocation.width;
    h = rowheight;

    gtkpl_redraw_pl_row_novis (ps, row, it);
	gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, x, y, x, y, w, h);
	//gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
}

void
gtkpl_draw_pl_row_back (gtkplaylist_t *ps, int row, playItem_t *it) {
	// draw background
	float w;
	int start, end;
	int startx, endx;
	int width, height;
	draw_get_canvas_size ((uintptr_t)ps->backbuf, &width, &height);
	w = width;
	if (it && ((it->selected && ps->multisel) || (row == ps->row && !ps->multisel))) {
        if (row % 2) {
            theme_set_fg_color (COLO_PLAYLIST_SEL_EVEN);
        }
        else {
            theme_set_fg_color (COLO_PLAYLIST_SEL_ODD);
        }
        draw_rect (0, row * rowheight - ps->scrollpos * rowheight, width, rowheight, 1);
    }
    else {
        if (row % 2) {
            theme_set_fg_color (COLO_PLAYLIST_EVEN);
        }
        else {
            theme_set_fg_color (COLO_PLAYLIST_ODD);
        }
        draw_rect (0, row * rowheight - ps->scrollpos * rowheight, width, rowheight, 1);
    }
	if (row == ps->row) {
        theme_set_fg_color (COLO_PLAYLIST_CURSOR);
        draw_rect (0, row * rowheight - ps->scrollpos * rowheight, width, rowheight-1, 0);
    }
}

void
gtkpl_draw_pl_row (gtkplaylist_t *ps, int row, playItem_t *it) {
    if (row-ps->scrollpos >= ps->nvisiblerows || row-ps->scrollpos < 0) {
//        fprintf (stderr, "WARNING: attempt to draw row outside of screen bounds (%d)\n", row-ps->scrollpos);
        return;
    }
	int width, height;
	draw_get_canvas_size ((uintptr_t)ps->backbuf, &width, &height);
    if (it == playlist_current_ptr && ps->colwidths[0] > 0/* && !p_isstopped ()*/) {
        uintptr_t pixbuf = p_ispaused () ? pause16_pixbuf : play16_pixbuf;
        draw_pixbuf ((uintptr_t)ps->backbuf, pixbuf, ps->colwidths[0]/2-8-ps->hscrollpos, (row - ps->scrollpos) * rowheight + rowheight/2 - 8, 0, 0, 16, 16);
    }
	if (it && ((it->selected && ps->multisel) || (row == ps->row && !ps->multisel))) {
        if (row % 2) {
            theme_set_bg_color (COLO_PLAYLIST_SEL_EVEN);
        }
        else {
            theme_set_bg_color (COLO_PLAYLIST_SEL_ODD);
        }
        theme_set_fg_color (COLO_PLAYLIST_SEL_TEXT);
    }
    else {
        if (row % 2) {
            theme_set_bg_color (COLO_PLAYLIST_EVEN);
        }
        else {
            theme_set_bg_color (COLO_PLAYLIST_ODD);
        }
        theme_set_fg_color (COLO_PLAYLIST_TEXT);
    }
    // draw as columns
    char dur[10] = "-:--";
    if (it) {
        if (it->duration >= 0) {
            int min = (int)it->duration/60;
            int sec = (int)(it->duration-min*60);
            snprintf (dur, 10, "%d:%02d", min, sec);
        }
    }

    char artistalbum[1024];
    const char *artist = pl_find_meta (it, "artist");
    if (!artist) {
        artist = "?";
    }
    const char *album = pl_find_meta (it, "album");
    if (!album) {
        album = "?";
    }
    const char *track = pl_find_meta (it, "track");
    if (!track) {
        track = "";
    }
    const char *title = pl_find_meta (it, "title");
    if (!title) {
        title = "?";
    }
    snprintf (artistalbum, 1024, "%s - %s", artist, album);
    const char *columns[pl_ncolumns] = {
        "",
        artistalbum,
        track,
        title,
        dur
    };
    int x = -ps->hscrollpos;
    for (int i = 0; i < pl_ncolumns; i++) {
//        char str[512];
        if (i > 0) {
            
            int dotpos;
            int cidx = ((row-ps->scrollpos) * pl_ncolumns + i) * 3;
#if 0
            if (!ps->fmtcache[cidx + 2]) {
//                gtkpl_set_cairo_font (cr);
                ps->fmtcache[cidx + 1] = gtkpl_fit_text (str, &dotpos, 512, columns[i], ps->colwidths[i]-10);
                ps->fmtcache[cidx + 0] = dotpos;
                ps->fmtcache[cidx + 2] = 1;

            }
            else {
                // reconstruct from cache
                dotpos = ps->fmtcache[cidx + 0];
                strncpy (str, columns[i], 512);
                if (dotpos >= 0) {
                    strcpy (str+dotpos, "…");
                }
            }
#endif
            //int w = ps->fmtcache[cidx + 1];
//            printf ("draw %s -> %s\n", columns[i], str);
            if (i == 2) {
                draw_text_with_colors (x+5, row * rowheight - ps->scrollpos * rowheight + rowheight/2 - draw_get_font_size ()/2, ps->colwidths[i]-10, 1, columns[i]);
            }
            else {
                draw_text_with_colors (x + 5, row * rowheight - ps->scrollpos * rowheight + rowheight/2 - draw_get_font_size ()/2, ps->colwidths[i]-10, 0, columns[i]);
            }
        }
        x += ps->colwidths[i];
    }
}


void
gtkpl_draw_playlist (gtkplaylist_t *ps, int x, int y, int w, int h) {
    GtkWidget *widget = ps->playlist;
    if (!ps->backbuf) {
        return;
    }
#if 0
    if (!ps->fmtcache && ps->nvisiblerows > 0 && pl_ncolumns > 0) {
        ps->fmtcache = malloc (ps->nvisiblerows * pl_ncolumns * 3 * sizeof (int16_t));
        memset (ps->fmtcache, 0, ps->nvisiblerows * pl_ncolumns * 3 * sizeof (int16_t));
    }
#endif
    draw_begin ((uintptr_t)ps->backbuf);
	int row;
	int row1;
	int row2;
	int row2_full;
	row1 = max (0, y / rowheight + ps->scrollpos);
	row2 = min ((*ps->pcount), (y+h) / rowheight + ps->scrollpos + 1);
	row2_full = (y+h) / rowheight + ps->scrollpos + 1;
	// draw background
	playItem_t *it = gtkpl_get_for_idx (ps, ps->scrollpos);
	playItem_t *it_copy = it;
	for (row = row1; row < row2_full; row++) {
		gtkpl_draw_pl_row_back (ps, row, it);
		if (it) {
            it = it->next[ps->iterator];
        }
	}
	it = it_copy;
	int idx = 0;
	for (row = row1; row < row2; row++, idx++) {
        gtkpl_draw_pl_row (ps, row, it);
        it = it->next[ps->iterator];
	}

    draw_end ();
}

void
gtkpl_configure (gtkplaylist_t *ps) {
    gtkpl_setup_scrollbar (ps);
    gtkpl_setup_hscrollbar (ps);
    GtkWidget *widget = ps->playlist;
    if (ps->backbuf) {
        g_object_unref (ps->backbuf);
        ps->backbuf = NULL;
    }
#if 0
    if (ps->fmtcache) {
        free (ps->fmtcache);
        ps->fmtcache = NULL;
    }
#endif
    ps->nvisiblerows = ceil (widget->allocation.height / (float)rowheight);
    ps->nvisiblefullrows = floor (widget->allocation.height / (float)rowheight);
    ps->backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);

    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
}

// change properties
gboolean
on_playlist_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
//    extern void main_playlist_init (GtkWidget *widget);
//    main_playlist_init (widget);
    GTKPL_PROLOGUE;
    gtkpl_configure (ps);
    return FALSE;
}

void
gtkpl_expose (gtkplaylist_t *ps, int x, int y, int w, int h) {
    GtkWidget *widget = ps->playlist;
    if (widget->window) {
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, x, y, x, y, w, h);
    }
}

void
gtkpl_expose_header (gtkplaylist_t *ps, int x, int y, int w, int h) {
    GtkWidget *widget = ps->header;
	gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf_header, x, y, x, y, w, h);
}

void
gtkpl_select_single (gtkplaylist_t *ps, int sel) {
    if (!ps->multisel) {
        return;
    }
    int idx=0;
    GtkWidget *widget = ps->playlist;
    for (playItem_t *it = playlist_head[ps->iterator]; it; it = it->next[ps->iterator], idx++) {
        if (idx == sel) {
            if (!it->selected) {
                it->selected = 1;
                gtkpl_redraw_pl_row (ps, idx, it);
            }
        }
        else if (it->selected) {
            it->selected = 0;
            gtkpl_redraw_pl_row (ps, idx, it);
        }
    }
}

// {{{ expected behaviour for mouse1 without modifiers:
//   {{{ [+] if clicked unselected item:
//       unselect all
//       select clicked item
//       ps->row = clicked
//       redraw
//       start 'area selection' mode
//   }}}
//   {{{ [+] if clicked selected item:
//       ps->row = clicked
//       redraw
//       wait until next release or motion event, whichever is 1st
//       if release is 1st:
//           unselect all except clicked, redraw
//       else if motion is 1st:
//           enter drag-drop mode
//   }}}
// }}}
static int areaselect = 0;
static int areaselect_x = -1;
static int areaselect_y = -1;
static int areaselect_dx = -1;
static int areaselect_dy = -1;
static int dragwait = 0;
static int shift_sel_anchor = -1;

void
gtkpl_mouse1_pressed (gtkplaylist_t *ps, int state, int ex, int ey, double time) {
    // cursor must be set here, but selection must be handled in keyrelease
    if ((*ps->pcount) == 0) {
        return;
    }
    GtkWidget *widget = ps->playlist;
    // remember mouse coords for doubleclick detection
    ps->lastpos[0] = ex;
    ps->lastpos[1] = ey;
    // select item
    int y = ey/rowheight + ps->scrollpos;
    if (y < 0 || y >= (*ps->pcount)) {
        y = -1;
    }

    if (time - ps->clicktime < 0.5
            && fabs(ps->lastpos[0] - ex) < 3
            && fabs(ps->lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (ps->row != -1) {
            gtkplaylist_t main_playlist;
            playItem_t *it = gtkpl_get_for_idx (ps, ps->row);
            it->selected = 1;
            int r = pl_get_idx_of (it);
            messagepump_push (M_PLAYSONGNUM, 0, r, 0);
            return;
        }


        // prevent next click to trigger doubleclick
        ps->clicktime = time-1;
    }
    else {
        ps->clicktime = time;
    }

    int sel = y;
    if (y == -1) {
        y = (*ps->pcount) - 1;
    }
    int prev = ps->row;
    ps->row = y;
    shift_sel_anchor = ps->row;
    // handle multiple selection
    if (ps->multisel) {
        if (!(state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)))
        {
            playItem_t *it = gtkpl_get_for_idx (ps, sel);
            if (!it || !it->selected) {
                // reset selection, and set it to single item
                gtkpl_select_single (ps, sel);
                areaselect = 1;
                areaselect_x = ex;
                areaselect_y = ey;
                areaselect_dx = -1;
                areaselect_dy = -1;
                shift_sel_anchor = ps->row;
            }
            else {
                dragwait = 1;
                gtkpl_redraw_pl_row (ps, prev, gtkpl_get_for_idx (ps, prev));
                if (ps->row != prev) {
                    gtkpl_redraw_pl_row (ps, ps->row, gtkpl_get_for_idx (ps, ps->row));
                }
            }
        }
        else if (state & GDK_CONTROL_MASK) {
            // toggle selection
            if (y != -1) {
                playItem_t *it = gtkpl_get_for_idx (ps, y);
                if (it) {
                    it->selected = 1 - it->selected;
                    gtkpl_redraw_pl_row (ps, y, it);
                }
            }
        }
        else if (state & GDK_SHIFT_MASK) {
            // select range
            int start = min (prev, ps->row);
            int end = max (prev, ps->row);
            int idx = 0;
            for (playItem_t *it = playlist_head[ps->iterator]; it; it = it->next[ps->iterator], idx++) {
                if (idx >= start && idx <= end) {
                    if (!it->selected) {
                        it->selected = 1;
                        gtkpl_redraw_pl_row (ps, idx, it);
                    }
                }
                else {
                    if (it->selected) {
                        it->selected = 0;
                        gtkpl_redraw_pl_row (ps, idx, it);
                    }
                }
            }
        }
        if (ps->row != -1 && sel == -1) {
            gtkpl_redraw_pl_row (ps, ps->row, gtkpl_get_for_idx (ps, ps->row));
        }
    }
    else {
        if (ps->row != -1) {
            gtkpl_redraw_pl_row (ps, ps->row, gtkpl_get_for_idx (ps, ps->row));
        }
    }
    if (prev != -1 && prev != ps->row) {
        gtkpl_redraw_pl_row (ps, prev, gtkpl_get_for_idx (ps, prev));
    }

}

void
gtkpl_mouse1_released (gtkplaylist_t *ps, int state, int ex, int ey, double time) {
    if (dragwait) {
        dragwait = 0;
        int y = ey/rowheight + ps->scrollpos;
        gtkpl_select_single (ps, y);
    }
    else if (areaselect) {
        areaselect = 0;
    }
}

#if 0
void
gtkpl_draw_areasel (GtkWidget *widget, int x, int y) {
    // erase previous rect using 4 blits from ps->backbuffer
    if (areaselect_dx != -1) {
        int sx = min (areaselect_x, areaselect_dx);
        int sy = min (areaselect_y, areaselect_dy);
        int dx = max (areaselect_x, areaselect_dx);
        int dy = max (areaselect_y, areaselect_dy);
        int w = dx - sx + 1;
        int h = dy - sy + 1;
        //gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy, sx, sy, dx - sx + 1, dy - sy + 1);
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy, sx, sy, w, 1);
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy, sx, sy, 1, h);
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy + h - 1, sx, sy + h - 1, w, 1);
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx + w - 1, sy, sx + w - 1, sy, 1, h);
    }
    areaselect_dx = x;
    areaselect_dy = y;
	cairo_t *cr;
	cr = gdk_cairo_create (widget->window);
	if (!cr) {
		return;
	}
	theme_set_fg_color (COLO_PLAYLIST_CURSOR);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width (cr, 1);
    int sx = min (areaselect_x, x);
    int sy = min (areaselect_y, y);
    int dx = max (areaselect_x, x);
    int dy = max (areaselect_y, y);
    cairo_rectangle (cr, sx, sy, dx-sx, dy-sy);
    cairo_stroke (cr);
    gtkpl_cairo_destroy (cr);
}
#endif

void
gtkpl_mousemove (gtkplaylist_t *ps, GdkEventMotion *event) {
    if (dragwait) {
        GtkWidget *widget = ps->playlist;
        if (gtk_drag_check_threshold (widget, ps->lastpos[0], event->x, ps->lastpos[1], event->y)) {
            dragwait = 0;
            GtkTargetEntry entry = {
                .target = "STRING",
                .flags = GTK_TARGET_SAME_WIDGET,
                .info = TARGET_SAMEWIDGET
            };
            GtkTargetList *lst = gtk_target_list_new (&entry, 1);
            gtk_drag_begin (widget, lst, GDK_ACTION_MOVE, TARGET_SAMEWIDGET, (GdkEvent *)event);
        }
    }
    else if (areaselect) {
        GtkWidget *widget = ps->playlist;
        int y = event->y/rowheight + ps->scrollpos;
        if (y != shift_sel_anchor) {
            int start = min (y, shift_sel_anchor);
            int end = max (y, shift_sel_anchor);
            int idx=0;
            for (playItem_t *it = playlist_head[ps->iterator]; it; it = it->next[ps->iterator], idx++) {
                if (idx >= start && idx <= end) {
                    it->selected = 1;
                    gtkpl_redraw_pl_row (ps, idx, it);
                }
                else if (it->selected)
                {
                    it->selected = 0;
                    gtkpl_redraw_pl_row (ps, idx, it);
                }
            }
        }
        // debug only
        // gtkpl_draw_areasel (widget, event->x, event->y);
    }
}

void
gtkpl_handle_scroll_event (gtkplaylist_t *ps, int direction) {
    GtkWidget *range = ps->scrollbar;;
    GtkWidget *playlist = ps->playlist;
    int h = playlist->allocation.height / rowheight;
    int size = (*ps->pcount);
    if (h >= size) {
        size = 0;
    }
    if (size == 0) {
        return;
    }
    // pass event to scrollbar
    GtkAdjustment* adj = gtk_range_get_adjustment (GTK_RANGE (range));
    int newscroll = gtk_range_get_value (GTK_RANGE (range));
    if (direction == GDK_SCROLL_UP) {
        newscroll -= 2;
    }
    else if (direction == GDK_SCROLL_DOWN) {
        newscroll += 2;
    }
    gtk_range_set_value (GTK_RANGE (range), newscroll);
}

void
gtkpl_scroll (gtkplaylist_t *ps, int newscroll) {
    if (newscroll != ps->scrollpos) {
#if 0
        int d = abs (newscroll - ps->scrollpos);
        if (abs (newscroll - ps->scrollpos) < ps->nvisiblerows) {
            // move untouched cache part
            // and invalidate changed part
            if (newscroll < ps->scrollpos) {
                //printf ("scroll up\n");
                int r;
                for (r = ps->nvisiblerows-1; r >= d; r--) {
                    memcpy (&ps->fmtcache[r * pl_ncolumns * 3], &ps->fmtcache[(r - d) * pl_ncolumns * 3], sizeof (int16_t) * 3 * pl_ncolumns);
                }
                for (r = 0; r < d; r++) {
                    memset (&ps->fmtcache[r * pl_ncolumns * 3], 0, sizeof (int16_t) * 3 * pl_ncolumns);
                }
            }
            else {
                //printf ("scroll down\n");
                int r;
                for (r = 0; r < ps->nvisiblerows-d; r++) {
                    memcpy (&ps->fmtcache[r * pl_ncolumns * 3], &ps->fmtcache[(r + d) * pl_ncolumns * 3], sizeof (int16_t) * 3 * pl_ncolumns);
                }
                for (r = ps->nvisiblerows-d; r < ps->nvisiblerows; r++) {
                    memset (&ps->fmtcache[r * pl_ncolumns * 3], 0, sizeof (int16_t) * 3 * pl_ncolumns);
                }
            }
        }
        else {
            // invalidate entire cache
            memset (ps->fmtcache, 0, sizeof (int16_t) * 3 * pl_ncolumns * ps->nvisiblerows);
        }
#endif
        ps->scrollpos = newscroll;
        GtkWidget *widget = ps->playlist;
        gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
    }
}

void
gtkpl_hscroll (gtkplaylist_t *ps, int newscroll) {
    if (newscroll != ps->hscrollpos) {
        ps->hscrollpos = newscroll;
        GtkWidget *widget = ps->playlist;
        gtkpl_header_draw (ps);
        gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
    }
}

void
gtkpl_randomsong (void) {
    p_stop ();
    pl_randomsong ();
}

void
gtkpl_playsongnum (int idx) {
    p_stop ();
    streamer_set_nextsong (idx, 1);
}

void
gtkpl_songchanged (gtkplaylist_t *ps, int from, int to) {
    if (!dragwait && to != -1) {
        GtkWidget *widget = ps->playlist;
        if (session_get_scroll_follows_playback ()) {
            if (to < ps->scrollpos || to >= ps->scrollpos + ps->nvisiblefullrows) {
                gtk_range_set_value (GTK_RANGE (ps->scrollbar), to - ps->nvisiblerows/2);
            }
        }
    }

    if (from >= 0) {
        gtkpl_redraw_pl_row (ps, from, gtkpl_get_for_idx (ps, from));
    }
    if (to >= 0) {
        gtkpl_redraw_pl_row (ps, to, gtkpl_get_for_idx (ps, to));
    }
}

void
gtkpl_keypress (gtkplaylist_t *ps, int keyval, int state) {
    GtkWidget *widget = ps->playlist;
    GtkWidget *range = ps->scrollbar;
    int prev = ps->row;
    int newscroll = ps->scrollpos;
// C-f is now handled by gtk
//    if ((keyval == GDK_F || keyval == GDK_f) && (state & GDK_CONTROL_MASK)) {
//        search_start ();       
//    }
//    else
//    if ((keyval == GDK_A || keyval == GDK_a) && (state & GDK_CONTROL_MASK)) {
//        // select all
//        pl_select_all ();
//        gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
//        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
//        return;
//    }
//    else if ((keyval == GDK_P || keyval == GDK_p) && (state & GDK_CONTROL_MASK)) {
//        messagepump_push (M_PAUSESONG, 0, 0, 0);
//    }
//    else
//    if (keyval == GDK_Return && ps->row != -1) {
//        messagepump_push (M_PLAYSONGNUM, 0, ps->row, 0);
//        return;
//    }
//    else
//    if (keyval == GDK_Delete) {
//        pl_delete_selected ();
//        playlist_refresh ();
//        return;
//    }
//    else
    if (keyval == GDK_Down && ps->row < (*ps->pcount) - 1) {
        ps->row++;
        if (ps->row > ps->scrollpos + widget->allocation.height / rowheight - 1) {
            newscroll = ps->row - widget->allocation.height / rowheight + 1;
        }
    }
    else if (keyval == GDK_Up && ps->row > 0) {
        ps->row--;
        if (ps->row < ps->scrollpos) {
            newscroll = ps->row;
        }
    }
    else if (keyval == GDK_Page_Down && ps->row < (*ps->pcount) - 1) {
        ps->row += 10;
        if (ps->row >= (*ps->pcount)) {
            ps->row = (*ps->pcount) - 1;
        }
        if (ps->row > ps->scrollpos + widget->allocation.height / rowheight - 1) {
            newscroll = ps->row - widget->allocation.height / rowheight + 1;
        }
    }
    else if (keyval == GDK_Page_Up && ps->row > 0) {
        ps->row -= 10;
        if (ps->row < 0) {
            ps->row = 0;
        }
        if (ps->row < ps->scrollpos) {
            newscroll = ps->row;
        }
    }
    else if (keyval == GDK_End && ps->row != (*ps->pcount) - 1) {
        ps->row = (*ps->pcount) - 1;
        if (ps->row > ps->scrollpos + widget->allocation.height / rowheight - 1) {
            newscroll = ps->row - widget->allocation.height / rowheight + 1;
        }
    }
    else if (keyval == GDK_Home && ps->row != 0) {
        ps->row = 0;
        if (ps->row < ps->scrollpos) {
            newscroll = ps->row;
        }
    }
    if (state & GDK_SHIFT_MASK) {
        // select all between shift_sel_anchor and ps->row
        if (prev != ps->row) {
            int minvis = ps->scrollpos;
            int maxvis = ps->scrollpos + ps->nvisiblerows-1;
            int start = min (ps->row, shift_sel_anchor);
            int end = max (ps->row, shift_sel_anchor);
            int idx=0;
            for (playItem_t *it = playlist_head[ps->iterator]; it; it = it->next[ps->iterator], idx++) {
                if (idx >= start && idx <= end) {
                    it->selected = 1;
                    if (idx >= minvis && idx <= maxvis) {
                        if (newscroll == ps->scrollpos) {
                            gtkpl_redraw_pl_row (ps, idx, it);
                        }
                        else {
                            gtkpl_redraw_pl_row_novis (ps, idx, it);
                        }
                    }
                }
                else if (it->selected)
                {
                    it->selected = 0;
                    if (idx >= minvis && idx <= maxvis) {
                        if (newscroll == ps->scrollpos) {
                            gtkpl_redraw_pl_row (ps, idx, it);
                        }
                        else {
                            gtkpl_redraw_pl_row_novis (ps, idx, it);
                        }
                    }
                }
            }
        }
    }
    else {
        // reset selection, set new single cursor/selection
        if (prev != ps->row) {
            int minvis = ps->scrollpos;
            int maxvis = ps->scrollpos + ps->nvisiblerows-1;
            shift_sel_anchor = ps->row;
            int idx=0;
            for (playItem_t *it = playlist_head[ps->iterator]; it; it = it->next[ps->iterator], idx++) {
                if (idx == ps->row) {
                    if (!it->selected) {
                        it->selected = 1;
                        if (idx >= minvis && idx <= maxvis) {
                            if (newscroll == ps->scrollpos) {
                                gtkpl_redraw_pl_row (ps, idx, it);
                            }
                            else {
                                gtkpl_redraw_pl_row_novis (ps, idx, it);
                            }
                        }
                    }
                }
                else if (it->selected) {
                    it->selected = 0;
                    if (idx >= minvis && idx <= maxvis) {
                        if (newscroll == ps->scrollpos) {
                            gtkpl_redraw_pl_row (ps, idx, it);
                        }
                        else {
                            gtkpl_redraw_pl_row_novis (ps, idx, it);
                        }
                    }
                }
            }
        }
    }
    if (newscroll != ps->scrollpos) {
        gtk_range_set_value (GTK_RANGE (range), newscroll);
    }
}

static int drag_motion_y = -1;

void
gtkpl_track_dragdrop (gtkplaylist_t *ps, int y) {
    GtkWidget *widget = ps->playlist;
    if (drag_motion_y != -1) {
        // erase previous track
        gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, drag_motion_y * rowheight-3, 0, drag_motion_y * rowheight-3, widget->allocation.width, 7);

    }
    if (y == -1) {
        drag_motion_y = -1;
        return;
    }
    draw_begin ((uintptr_t)widget->window);
    drag_motion_y = y / rowheight;

    theme_set_fg_color (COLO_DRAGDROP_MARKER);
    draw_rect (0, drag_motion_y * rowheight-1, widget->allocation.width, 3, 1);
    draw_rect (0, drag_motion_y * rowheight-3, 3, 7, 1);
    draw_rect (widget->allocation.width-3, drag_motion_y * rowheight-3, 3, 7, 1);
    draw_end ();
}

void
gtkpl_handle_drag_drop (gtkplaylist_t *ps, int drop_y, uint32_t *d, int length) {
    int drop_row = drop_y / rowheight + ps->scrollpos;
    playItem_t *drop_before = gtkpl_get_for_idx (ps, drop_row);
    while (drop_before && drop_before->selected) {
        drop_before = drop_before->next[ps->iterator];
    }
    // unlink items from playlist, and link together
    playItem_t *head = NULL;
    playItem_t *tail = NULL;
    int processed = 0;
    int idx = 0;
    playItem_t *next = NULL;
    for (playItem_t *it = playlist_head[ps->iterator]; it && processed < length; it = next, idx++) {
        //            printf ("idx: %d\n", d[i]);
        next = it->next[ps->iterator];
        if (idx == d[processed]) {
            if (it->prev[ps->iterator]) {
                it->prev[ps->iterator]->next[ps->iterator] = it->next[ps->iterator];
            }
            else {
                playlist_head[ps->iterator] = it->next[ps->iterator];
            }
            if (it->next[ps->iterator]) {
                it->next[ps->iterator]->prev[ps->iterator] = it->prev[ps->iterator];
            }
            else {
                playlist_tail[ps->iterator] = it->prev[ps->iterator];
            }
            if (tail) {
                tail->next[ps->iterator] = it;
                it->prev[ps->iterator] = tail;
                tail = it;
            }
            else {
                head = tail = it;
                it->prev[ps->iterator] = it->next[ps->iterator] = NULL;
            }
            processed++;
        }
    }
    // find insertion point
    playItem_t *drop_after = NULL;
    if (drop_before) {
        drop_after = drop_before->prev[ps->iterator];
    }
    else {
        drop_after = playlist_tail[ps->iterator];
    }
    // insert in between
    head->prev[ps->iterator] = drop_after;
    if (drop_after) {
        drop_after->next[ps->iterator] = head;
    }
    else {
        playlist_head[ps->iterator] = head;
    }
    tail->next[ps->iterator] = drop_before;
    if (drop_before) {
        drop_before->prev[ps->iterator] = tail;
    }
    else {
        playlist_tail[ps->iterator] = tail;
    }
}

void
on_playlist_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    // invalidate entire cache - slow, but rare
    //memset (ps->fmtcache, 0, sizeof (int16_t) * 3 * pl_ncolumns * ps->nvisiblerows);
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
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

int
gtkpl_add_file_info_cb (playItem_t *it, void *data) {
    if (progress_is_aborted ()) {
        return -1;
    }
    GDK_THREADS_ENTER();
    progress_settext (it->fname);
    GDK_THREADS_LEAVE();
#if 0
        GtkEntry *e = (GtkEntry *)data;
        GDK_THREADS_ENTER();
        gtk_entry_set_text (GTK_ENTRY (e), it->fname);
        GDK_THREADS_LEAVE();
        usleep (100);
        countdown = 10;
#endif
    return 0;
}

void
gtkpl_add_fm_dropped_files (gtkplaylist_t *ps, char *ptr, int length, int drop_y) {
    GDK_THREADS_ENTER();
    progress_show ();
    GDK_THREADS_LEAVE();

    int drop_row = drop_y / rowheight + ps->scrollpos;
    playItem_t *drop_before = gtkpl_get_for_idx (ps, drop_row);
    playItem_t *after = NULL;
    if (drop_before) {
        after = drop_before->prev[ps->iterator];
    }
    else {
        after = playlist_tail[ps->iterator];
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
            playItem_t *inserted = pl_insert_dir (after, fname + 7, &abort, gtkpl_add_file_info_cb, NULL);
            if (!inserted && !abort) {
                inserted = pl_insert_file (after, fname + 7, &abort, gtkpl_add_file_info_cb, NULL);
            }
            if (inserted) {
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

    GDK_THREADS_ENTER();
    progress_hide ();
    playlist_refresh ();
    GDK_THREADS_LEAVE();
}

void
gtkpl_handle_fm_drag_drop (gtkplaylist_t *ps, int drop_y, void *ptr, int length) {
    // this happens when dropped from file manager
    char *mem = malloc (length);
    memcpy (mem, ptr, length);
    // we don't pass control structure, but there's only one drag-drop view currently
    messagepump_push (M_FMDRAGDROP, (uintptr_t)mem, length, drop_y);
}

void
gtkpl_header_draw (gtkplaylist_t *ps) {
    GtkWidget *widget = ps->header;
    int x = -ps->hscrollpos;
    int w = 100;
    int h = widget->allocation.height;
    const char *detail = "toolbar";

    // fill background
    gdk_draw_rectangle (ps->backbuf_header, widget->style->bg_gc[0], TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
    for (int i = 0; i < pl_ncolumns; i++) {
        if (x >= widget->allocation.width) {
            break;
        }
        w = ps->colwidths[i];
        if (w > 0) {
            gtk_paint_box (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, NULL, detail, x, 0, w - 2, h);
            gtk_paint_vline (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, NULL, NULL, NULL, 0, h, x+w - 2);
        }
        x += w;
    }
    if (x < widget->allocation.width) {
        gtk_paint_box (widget->style, ps->backbuf_header, GTK_STATE_INSENSITIVE, GTK_SHADOW_OUT, NULL, NULL, detail, x, 0, widget->allocation.width-x, h);
    }
    draw_begin ((uintptr_t)ps->backbuf_header);
    x = -ps->hscrollpos;
    for (int i = 0; i < pl_ncolumns; i++) {
        if (x >= widget->allocation.width) {
            break;
        }
        w = ps->colwidths[i];
        if (w > 0) {
#if 0
            if (!ps->header_fitted[i]) {
                gtkpl_fit_text (ps->colnames_fitted[i], NULL, pl_colname_max, colnames[i], ps->colwidths[i]-10);
                ps->header_fitted[i] = 1;
            }
#endif
            GdkColor *gdkfg = &widget->style->fg[0];
            float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
            draw_set_fg_color (fg);
            draw_text (x + 5, h/2-draw_get_font_size()/2, ps->colwidths[i]-10, 0, colnames[i]);
        }
        x += w;
    }
    draw_end ();
}

gboolean
on_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    gtkpl_header_draw (ps);
    gtkpl_expose_header (ps, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}


gboolean
on_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    if (ps->backbuf_header) {
        g_object_unref (ps->backbuf_header);
        ps->backbuf_header = NULL;
    }
    ps->backbuf_header = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);
    gtkpl_header_draw (ps);
    return FALSE;
}


GdkCursor* cursor_sz;
GdkCursor* cursor_drag;
int header_dragging = -1;
int header_sizing = -1;
int header_dragpt[2];

void
on_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data)
{
    // create cursor for sizing headers
    int h = draw_get_font_size ();
    gtk_widget_set_size_request (widget, -1, h + 10);
    cursor_sz = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    cursor_drag = gdk_cursor_new (GDK_FLEUR);
}

float last_header_motion_ev = -1;
int prev_header_x = -1;

gboolean
on_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    if (header_dragging >= 0) {
        gdk_window_set_cursor (widget->window, cursor_drag);
    }
    else if (header_sizing >= 0) {
        // limit event rate
        if (event->time - last_header_motion_ev < 20 || prev_header_x == event->x) {
            return FALSE;
        }
        //printf ("%f\n", event->time - last_header_motion_ev);
        last_header_motion_ev = event->time;
        prev_header_x = event->x;
        gdk_window_set_cursor (widget->window, cursor_sz);
        // get column start pos
        int x = -ps->hscrollpos;
        for (int i = 0; i < header_sizing; i++) {
            int w = ps->colwidths[i];
            x += w;
        }
        int newx = event->x > x + 40 ? event->x : x + 40;
        ps->colwidths[header_sizing] = newx - x;
        //printf ("ev->x = %d, w = %d\n", (int)event->x, newx - x - 2);
#if 0
        ps->header_fitted[header_sizing] = 0;
        for (int k = 0; k < ps->nvisiblerows; k++) {
            int cidx = (k * pl_ncolumns + header_sizing) * 3;
            ps->fmtcache[cidx+2] = 0;
        }
#endif
        gtkpl_setup_hscrollbar (ps);
        gtkpl_header_draw (ps);
        gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
        gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    }
    else {
        int x = -ps->hscrollpos;
        for (int i = 0; i < pl_ncolumns; i++) {
            int w = ps->colwidths[i];
            if (w > 0) { // ignore collapsed columns (hack for search window)
                if (event->x >= x + w - 2 && event->x <= x + w) {
                    gdk_window_set_cursor (widget->window, cursor_sz);
                    break;
                }
                else {
                    gdk_window_set_cursor (widget->window, NULL);
                }
            }
            else {
                gdk_window_set_cursor (widget->window, NULL);
            }
            x += w;
        }
    }
    return FALSE;
}

gboolean
on_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    if (event->button == 1) {
        // start sizing/dragging
        header_dragging = -1;
        header_sizing = -1;
        header_dragpt[0] = event->x;
        header_dragpt[1] = event->y;
        int x = -ps->hscrollpos;
        for (int i = 0; i < pl_ncolumns; i++) {
            int w = ps->colwidths[i];
            if (event->x >= x + w - 2 && event->x <= x + w) {
                header_sizing = i;
                header_dragging = -1;
                break;
            }
            else {
                header_dragging = i;
            }
            x += w;
        }
    }
    prev_header_x = -1;
    last_header_motion_ev = -1;
    return FALSE;
}

gboolean
on_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    header_dragging = -1;
    header_sizing = -1;
    int x = 0;
    for (int i = 0; i < pl_ncolumns; i++) {
        int w = ps->colwidths[i];
        if (event->x >= x + w - 2 && event->x <= x + w) {
            gdk_window_set_cursor (widget->window, cursor_sz);
            break;
        }
        else {
            gdk_window_set_cursor (widget->window, NULL);
        }
        x += w;
    }
    return FALSE;
}

void
gtkpl_add_dir (gtkplaylist_t *ps, char *folder) {
    GDK_THREADS_ENTER();
    progress_show ();
    GDK_THREADS_LEAVE();
    pl_add_dir (folder, gtkpl_add_file_info_cb, NULL);
    g_free (folder);
    GDK_THREADS_ENTER();
    progress_hide ();
    playlist_refresh ();
    GDK_THREADS_LEAVE();
}

static void
gtkpl_adddir_cb (gpointer data, gpointer userdata) {
    pl_add_dir (data, gtkpl_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_dirs (gtkplaylist_t *ps, GSList *lst) {
    GDK_THREADS_ENTER();
    progress_show ();
    GDK_THREADS_LEAVE();
    g_slist_foreach(lst, gtkpl_adddir_cb, NULL);
    g_slist_free (lst);
    GDK_THREADS_ENTER();
    progress_hide ();
    playlist_refresh ();
    GDK_THREADS_LEAVE();
}

static void
gtkpl_addfile_cb (gpointer data, gpointer userdata) {
    pl_add_file (data, gtkpl_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_files (gtkplaylist_t *ps, GSList *lst) {
    GDK_THREADS_ENTER();
    progress_show ();
    GDK_THREADS_LEAVE();
    g_slist_foreach(lst, gtkpl_addfile_cb, NULL);
    g_slist_free (lst);
    GDK_THREADS_ENTER();
    progress_hide ();
    playlist_refresh ();
    GDK_THREADS_LEAVE();
}

void
gtkpl_playsong (gtkplaylist_t *ps) {
    if (p_ispaused ()) {
        p_unpause ();
    }
    else if (ps->row != -1) {
        p_stop ();
        streamer_set_nextsong (ps->row, 1);
    }
    else {
        streamer_set_nextsong (0, 1);
    }
}

int
gtkpl_get_idx_of (gtkplaylist_t *ps, playItem_t *it) {
    playItem_t *c = playlist_head[ps->iterator];
    int idx = 0;
    while (c && c != it) {
        c = c->next[ps->iterator];
        idx++;
    }
    if (!c) {
        return -1;
    }
    return idx;
}

playItem_t *
gtkpl_get_for_idx (gtkplaylist_t *ps, int idx) {
    playItem_t *it = playlist_head[ps->iterator];
    while (idx--) {
        if (!it)
            return NULL;
        it = it->next[ps->iterator];
    }
    return it;
}

void
playlist_refresh (void) {
    extern gtkplaylist_t main_playlist;
    gtkplaylist_t *ps = &main_playlist;
    //memset (ps->fmtcache, 0, sizeof (int16_t) * 3 * pl_ncolumns * ps->nvisiblerows);
    gtkpl_setup_scrollbar (ps);
    GtkWidget *widget = ps->playlist;
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    search_refresh ();
}
