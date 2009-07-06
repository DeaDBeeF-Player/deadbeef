#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include "gtkplaylist.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "playlist.h"
#include "psdl.h"
#include "codec.h"
#include "common.h"
#include "messagepump.h"
#include "messages.h"

extern GtkWidget *mainwin;
static GdkPixmap *backbuf;
static int rowheight = 17;
static int scrollpos = 0;
static int playlist_row = -1;
static double playlist_clicktime = 0;
static double ps_lastpos[2];

static void
text_draw (cairo_t *cr, int x, int y, const char *text) {
    cairo_move_to (cr, x, y+rowheight-3);
    cairo_show_text (cr, text);
}

void
gtkps_setup_scrollbar (void) {
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
    int h = playlist->allocation.height / rowheight;
    int size = ps_getcount ();
    if (h >= size) {
        size = 0;
    }
    GtkWidget *scroll = lookup_widget (mainwin, "playscroll");
//    gtk_range_set_range (GTK_RANGE (scroll), 0, size);
//    gtk_range_set_increments (GTK_RANGE (scroll), 1, h);
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, size, 1, h, h);
    gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
}

void
redraw_ps_row (GtkWidget *widget, int row) {
    int x, y, w, h;

    x = 0;
    y = (row  - scrollpos) * rowheight;
    w = widget->allocation.width;
    h = rowheight;

	cairo_t *cr;
	cr = gdk_cairo_create (backbuf);
	if (!cr) {
		return;
	}
	//cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

//    cairo_select_font_face (cr, "fixed", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
//    cairo_set_font_size (cr, rowheight);

	playItem_t *it = ps_get_for_idx (row);
	if (it) {
        draw_ps_row_back (backbuf, cr, row);
        draw_ps_row (backbuf, cr, row, it);
    }
    cairo_destroy (cr);
	gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, x, y, x, y, w, h);
}

void
draw_ps_row_back (GdkDrawable *drawable, cairo_t *cr, int row) {
	// draw background
	float w;
	int start, end;
	int startx, endx;
	int width, height;
	gdk_drawable_get_size (drawable, &width, &height);
	w = width;
	if (row == playlist_row) {
        cairo_set_source_rgb (cr, 0x7f/255.f, 0x7f/255.f, 0x7f/255.f);
        cairo_rectangle (cr, 0, row * rowheight - scrollpos * rowheight, width, rowheight);
        cairo_fill (cr);
        cairo_set_source_rgb (cr, 0xae/255.f, 0xa6/255.f, 0x9d/255.f);
        cairo_rectangle (cr, 1, row * rowheight - scrollpos * rowheight+1, width-2, rowheight-2);
        cairo_fill (cr);
    }
    else {
        if (row % 2) {
            cairo_set_source_rgb (cr, 0x1d/255.f, 0x1f/255.f, 0x1b/255.f);
            cairo_rectangle (cr, 0, row * rowheight - scrollpos * rowheight, w, rowheight);
            cairo_fill (cr);
        }
        else {
            cairo_set_source_rgb (cr, 0x21/255.f, 0x23/255.f, 0x1f/255.f);
            cairo_rectangle (cr, 0, row * rowheight - scrollpos * rowheight, width, rowheight);
            cairo_fill (cr);
        }
    }
//	start = min (vbstart[1], vbend[1]);
//	end = max (vbstart[1], vbend[1]);
//	startx = min (vbstart[0], vbend[0]);
//	endx = max (vbstart[0], vbend[0]);
//	if (tracker_vbmode && row >= start && row <= end) { // hilight selected notes
//		cairo_set_source_rgb (cr, 0.0, 0.44, 0.0);
//		cairo_rectangle (cr, startx * colwidth * COL_NUM_CHARS + colwidth * 3, row * rowheight - scrollpos * rowheight, (endx - startx + 1) * colwidth * COL_NUM_CHARS, rowheight);
//		cairo_fill (cr);
//	}
}

void
draw_ps_row (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it) {
	int width, height;
	gdk_drawable_get_size (drawable, &width, &height);
    if (it == playlist_current) {
        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_rectangle (cr, 3, row * rowheight - scrollpos * rowheight + 3, rowheight-6, rowheight-6);
        cairo_fill (cr);
    }
	if (row == playlist_row) {
        cairo_set_source_rgb (cr, 0, 0, 0);
    }
    else {
        cairo_set_source_rgb (cr, 0xf4/255.f, 0x7e/255.f, 0x46/255.f);
    }
    text_draw (cr, rowheight, row * rowheight - scrollpos * rowheight, it->displayname);
}


void
draw_playlist (GtkWidget *widget, int x, int y, int w, int h) {
	cairo_t *cr;
	cr = gdk_cairo_create (backbuf);
	if (!cr) {
		return;
	}
	//cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

//    cairo_select_font_face (cr, "fixed", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
//    cairo_set_font_size (cr, rowheight);
	int row;
	int row1;
	int row2;
	int row2_full;
	row1 = max (0, y / rowheight + scrollpos);
	row2 = min (ps_getcount (), (y+h) / rowheight + scrollpos + 1);
	row2_full = (y+h) / rowheight + scrollpos + 1;
	// draw background
	for (row = row1; row < row2_full; row++) {
		draw_ps_row_back (backbuf, cr, row);
	}
	playItem_t *it = ps_get_for_idx (scrollpos);
	for (row = row1; row < row2; row++) {
		draw_ps_row (backbuf, cr, row, it);
		it = it->next;
	}

    cairo_destroy (cr);
}

void
gtkps_reconf (GtkWidget *widget) {
    gtkps_setup_scrollbar ();
    if (backbuf) {
        g_object_unref (backbuf);
    }
    backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);
    draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
}

void
gtkps_expose (GtkWidget       *widget, int x, int y, int w, int h) {
	gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, x, y, x, y, w, h);
}

void
gtkps_mouse1_clicked (GtkWidget *widget, int ex, int ey, double time) {
    // remember mouse coords for doubleclick detection
    ps_lastpos[0] = ex;
    ps_lastpos[1] = ey;
    // select item
    int y = ey/rowheight + scrollpos;
    if (y < 0 || y >= ps_getcount ()) {
        y = -1;
    }

    if (playlist_row != y) {
        int old = playlist_row;
        playlist_row = y;
        if (old != -1) {
            redraw_ps_row (widget, old);
        }
        if (playlist_row != -1) {
            redraw_ps_row (widget, playlist_row);
        }
    }

    if (time - playlist_clicktime < 0.5
            && fabs(ps_lastpos[0] - ex) < 3
            && fabs(ps_lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (playlist_row != -1) {
#if 0
            playItem_t *it = ps_get_for_idx (playlist_row);
            if (it) {
                playItem_t *prev = playlist_current;
                playlist_current = it;
                if (playlist_current != prev) {
                    if (prev) {
                        redraw_ps_row (widget, ps_get_idx_of (prev));
                    }
                }
                messagepump_push (M_PLAYSONG, 0, 0, 0);
            }
#endif
            messagepump_push (M_PLAYSONGNUM, 0, playlist_row, 0);
        }


        // prevent next click to trigger doubleclick
        playlist_clicktime = time-1;
    }
    else {
        playlist_clicktime = time;
    }
}

void
gtkps_scroll (int newscroll) {
    if (newscroll != scrollpos) {
        scrollpos = newscroll;
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
        gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
    }
}

void
gtkps_playsong (void) {
    if (psdl_ispaused ()) {
        printf ("unpause\n");
        psdl_unpause ();
    }
    else if (playlist_current) {
        printf ("restart\n");
        codec_lock ();
        psdl_stop ();
        psdl_play (playlist_current);
        codec_unlock ();
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        redraw_ps_row (widget, ps_get_idx_of (playlist_current));
    }
    else if (playlist_row != -1) {
        printf ("start under cursor\n");
        playItem_t *it = ps_get_for_idx (playlist_row);
        if (it) {
            codec_lock ();
            psdl_stop ();
            psdl_play (it);
            codec_unlock ();
            playlist_current = it;
        }
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        redraw_ps_row (widget, playlist_row);
    }
    else {
        printf ("play 1st in list\n");
        playlist_current = playlist_head;
        if (playlist_current) {
            codec_lock ();
            psdl_stop ();
            psdl_play (playlist_current);
            codec_unlock ();
            GtkWidget *widget = lookup_widget (mainwin, "playlist");
            redraw_ps_row (widget, ps_get_idx_of (playlist_current));
        }
    }
}

void
gtkps_prevsong (void) {
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    playItem_t *prev = playlist_current;

    if (playlist_current) {
        playlist_current = playlist_current->prev;
    }
    if (!playlist_current) {
        playlist_current = playlist_tail;
    }
    if (playlist_current) {
        psdl_stop ();
        psdl_play (playlist_current);
    }
    if (playlist_current != prev) {
        if (prev) {
            redraw_ps_row (widget, ps_get_idx_of (prev));
        }
        if (playlist_current) {
            redraw_ps_row (widget, ps_get_idx_of (playlist_current));
        }
    }
}

void
gtkps_nextsong (void) {
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    playItem_t *prev = playlist_current;
    if (playlist_current) {
        playlist_current = playlist_current->next;
    }
    if (!playlist_current) {
        playlist_current = playlist_head;
    }
    if (playlist_current) {
        codec_lock ();
        psdl_stop ();
        psdl_play (playlist_current);
        codec_unlock ();
    }
    if (playlist_current != prev) {
        if (prev) {
            redraw_ps_row (widget, ps_get_idx_of (prev));
        }
        if (playlist_current) {
            redraw_ps_row (widget, ps_get_idx_of (playlist_current));
        }
    }
}

void
gtkps_randomsong (void) {
    if (!ps_getcount ()) {
        return;
    }
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    playItem_t *prev = playlist_current;
    int r = rand () % ps_getcount ();
    playItem_t *it = ps_get_for_idx (r);
    if (it) {
        playlist_current = it;
    }
    else {
        playlist_current = NULL;
    }
    if (playlist_current) {
        codec_lock ();
        psdl_stop ();
        psdl_play (playlist_current);
        codec_unlock ();
    }
    if (playlist_current != prev) {
        if (prev) {
            redraw_ps_row (widget, ps_get_idx_of (prev));
        }
        if (playlist_current) {
            redraw_ps_row (widget, ps_get_idx_of (playlist_current));
        }
    }
}

void
gtkps_stopsong (void) {
    psdl_stop ();
}

void
gtkps_pausesong (void) {
    if (psdl_ispaused ()) {
        psdl_unpause ();
    }
    else {
        psdl_pause ();
    }
}

void
gtkps_playsongnum (int idx) {
    playItem_t *it = ps_get_for_idx (playlist_row);
    if (it) {
        if (it != playlist_current) {
            GtkWidget *widget = lookup_widget (mainwin, "playlist");
            int prev = -1;
            if (playlist_current) {
                prev = ps_get_idx_of (playlist_current);
            }
            playlist_current = it;
            if (prev != -1) {
                redraw_ps_row (widget, prev);
            }
            redraw_ps_row (widget, idx);
        }
        codec_lock ();
        psdl_stop ();
        psdl_play (playlist_current);
        codec_unlock ();
    }
}

