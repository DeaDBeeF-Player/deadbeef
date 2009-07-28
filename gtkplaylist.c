#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
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
//    printf ("redraw row %d (selected = %d, cursor = %d)\n", row, it->selected, playlist_row == row ? 1 : 0);
	if (it) {
        draw_ps_row_back (backbuf, cr, row, it);
        draw_ps_row (backbuf, cr, row, it);
    }
    cairo_destroy (cr);
	gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, x, y, x, y, w, h);
}

void
draw_ps_row_back (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it) {
	// draw background
	float w;
	int start, end;
	int startx, endx;
	int width, height;
	gdk_drawable_get_size (drawable, &width, &height);
	w = width;
	if (it && it->selected) {
        if (row % 2) {
            cairo_set_source_rgb (cr, 0xa7/255.f, 0x9f/255.f, 0x96/255.f);
        }
        else {
            cairo_set_source_rgb (cr, 0xaf/255.f, 0xa7/255.f, 0x9e/255.f);
        }
        cairo_rectangle (cr, 0, row * rowheight - scrollpos * rowheight, width, rowheight);
        cairo_fill (cr);
    }
    else {
        if (row % 2) {
            cairo_set_source_rgb (cr, 0x1d/255.f, 0x1f/255.f, 0x1b/255.f);
        }
        else {
            cairo_set_source_rgb (cr, 0x21/255.f, 0x23/255.f, 0x1f/255.f);
        }
        cairo_rectangle (cr, 0, row * rowheight - scrollpos * rowheight, width, rowheight);
        cairo_fill (cr);
    }
	if (row == playlist_row) {
        cairo_set_source_rgb (cr, 0x7f/255.f, 0x7f/255.f, 0x7f/255.f);
        cairo_rectangle (cr, 0, row * rowheight - scrollpos * rowheight, width, rowheight);
        cairo_stroke (cr);
    }
}

void
draw_ps_row (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it) {
	int width, height;
	gdk_drawable_get_size (drawable, &width, &height);
    if (it == playlist_current_ptr) {
        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_rectangle (cr, 3, row * rowheight - scrollpos * rowheight + 3, rowheight-6, rowheight-6);
        cairo_fill (cr);
    }
	if (it && it->selected) {
        cairo_set_source_rgb (cr, 0, 0, 0);
    }
    else {
        cairo_set_source_rgb (cr, 0xf4/255.f, 0x7e/255.f, 0x46/255.f);
    }
    char dname[512];
    ps_format_item_display_name (it, dname, 512);
    text_draw (cr, rowheight, row * rowheight - scrollpos * rowheight, dname);
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
	playItem_t *it = ps_get_for_idx (scrollpos);
	playItem_t *it_copy = it;
	for (row = row1; row < row2_full; row++) {
		draw_ps_row_back (backbuf, cr, row, it);
		if (it) {
            it = it->next;
        }
	}
	it = it_copy;
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
gtkps_mouse1_clicked (GtkWidget *widget, int state, int ex, int ey, double time) {
    if (ps_getcount () == 0) {
        return;
    }
    // remember mouse coords for doubleclick detection
    ps_lastpos[0] = ex;
    ps_lastpos[1] = ey;
    // select item
    int y = ey/rowheight + scrollpos;
    if (y < 0 || y >= ps_getcount ()) {
        y = -1;
    }

    if (time - playlist_clicktime < 0.5
            && fabs(ps_lastpos[0] - ex) < 3
            && fabs(ps_lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (playlist_row != -1) {
            messagepump_push (M_PLAYSONGNUM, 0, playlist_row, 0);
        }


        // prevent next click to trigger doubleclick
        playlist_clicktime = time-1;
    }
    else {
        playlist_clicktime = time;
    }

    int sel = y;
    if (y == -1) {
        y = ps_getcount () - 1;
    }
    int prev = playlist_row;
    playlist_row = y;
    // handle selection
    if (!(state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)))
    {
        // reset selection, and set it to single item
        int idx=0;
        for (playItem_t *it = playlist_head; it; it = it->next, idx++) {
            if (idx == sel) {
                if (!it->selected) {
                    it->selected = 1;
//                    if (idx != y && idx != playlist_row) {
                        redraw_ps_row (widget, idx);
//                    }
                }
            }
            else if (it->selected) {
                it->selected = 0;
//                if (idx != y && idx != playlist_row) {
                    redraw_ps_row (widget, idx);
//                }
            }
        }
    }
    else if (state & GDK_CONTROL_MASK) {
        // toggle selection
        if (y != -1) {
            playItem_t *it = ps_get_for_idx (y);
            if (it) {
                it->selected = 1 - it->selected;
                redraw_ps_row (widget, y);
            }
        }
    }
    else if (state & GDK_SHIFT_MASK) {
        // select range
        int start = min (prev, playlist_row);
        int end = max (prev, playlist_row);
        int idx = 0;
        for (playItem_t *it = playlist_head; it; it = it->next, idx++) {
            if (idx >= start && idx <= end) {
                if (!it->selected) {
                    it->selected = 1;
                    redraw_ps_row (widget, idx);
                }
            }
            else {
                if (it->selected) {
                    it->selected = 0;
                    redraw_ps_row (widget, idx);
                }
            }
        }
    }

    if (prev != -1 && prev != playlist_row) {
        redraw_ps_row (widget, prev);
    }
    if (playlist_row != -1 && sel == -1) {
        redraw_ps_row (widget, playlist_row);
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
    else if (playlist_current_ptr) {
        printf ("restart\n");
        ps_start_current ();
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        redraw_ps_row (widget, ps_get_idx_of (playlist_current_ptr));
    }
    else if (playlist_row != -1) {
        printf ("start under cursor\n");
        playItem_t *it = ps_get_for_idx (playlist_row);
        if (it) {
            ps_set_current (it);
        }
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        redraw_ps_row (widget, playlist_row);
    }
    else {
        printf ("play 1st in list\n");
        ps_set_current (playlist_head);
        if (playlist_current_ptr) {
            GtkWidget *widget = lookup_widget (mainwin, "playlist");
            redraw_ps_row (widget, ps_get_idx_of (playlist_current_ptr));
        }
    }
}

void
gtkps_prevsong (void) {
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    playItem_t *prev = playlist_current_ptr;

    if (playlist_current_ptr) {
        printf ("gtkps_prevsong\n");
        ps_set_current (playlist_current_ptr->prev);
    }
    if (!playlist_current_ptr) {
        printf ("gtkps_prevsong2\n");
        ps_set_current (playlist_tail);
    }
    if (playlist_current_ptr != prev) {
        if (prev) {
            redraw_ps_row (widget, ps_get_idx_of (prev));
        }
        if (playlist_current_ptr) {
            redraw_ps_row (widget, ps_get_idx_of (playlist_current_ptr));
        }
    }
}

void
gtkps_nextsong (void) {
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    playItem_t *prev = playlist_current_ptr;
    if (playlist_current_ptr) {
        printf ("gtkps_nextsong\n");
        ps_set_current (playlist_current_ptr->next);
    }
    if (!playlist_current_ptr) {
        printf ("gtkps_nextsong2\n");
        ps_set_current (playlist_head);
    }
    if (playlist_current_ptr != prev) {
        if (prev) {
            redraw_ps_row (widget, ps_get_idx_of (prev));
        }
        if (playlist_current_ptr) {
            redraw_ps_row (widget, ps_get_idx_of (playlist_current_ptr));
        }
    }
}

void
gtkps_randomsong (void) {
    if (!ps_getcount ()) {
        return;
    }
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    playItem_t *prev = playlist_current_ptr;
    int r = rand () % ps_getcount ();
    playItem_t *it = ps_get_for_idx (r);
    if (it) {
        printf ("gtkps_randomsong\n");
        ps_set_current (it);
    }
    else {
        printf ("gtkps_randomsong2\n");
        ps_set_current (NULL);
    }
    if (playlist_current_ptr != prev) {
        if (prev) {
            redraw_ps_row (widget, ps_get_idx_of (prev));
        }
        if (playlist_current_ptr) {
            redraw_ps_row (widget, ps_get_idx_of (playlist_current_ptr));
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
        if (it != playlist_current_ptr) {
            GtkWidget *widget = lookup_widget (mainwin, "playlist");
            int prev = -1;
            if (playlist_current_ptr) {
                prev = ps_get_idx_of (playlist_current_ptr);
            }
            printf ("gtkps_playsongnum\n");
            ps_set_current (it);
            if (prev != -1) {
                redraw_ps_row (widget, prev);
            }
            redraw_ps_row (widget, idx);
        }
    }
}

static int sb_context_id = -1;
static char sb_text[512];
static int last_songpos = -1;

void
gtkps_update_songinfo (void) {
    if (!mainwin) {
        return;
    }
    char sbtext_new[512] = "-";
    int songpos = 0;
    if (psdl_ispaused ()) {
        strcpy (sbtext_new, "Paused");
        songpos = 0;
    }
    else if (playlist_current.codec) {
        codec_lock ();
        codec_t *c = playlist_current.codec;
        int minpos = c->info.position / 60;
        int secpos = c->info.position - minpos * 60;
        int mindur = c->info.duration / 60;
        int secdur = c->info.duration - mindur * 60;
        const char *mode = c->info.channels == 1 ? "Mono" : "Stereo";
        int samplerate = c->info.samplesPerSecond;
        int bitspersample = c->info.bitsPerSample;
        float pos = c->info.position;
        int dur = c->info.duration;
        songpos = pos * 1000 / dur;
        codec_unlock ();

        snprintf (sbtext_new, 512, "%dHz | %d bit | %s | Position %d:%02d | Duration %d:%02d | %d songs total", samplerate, bitspersample, mode, minpos, secpos, mindur, secdur, ps_getcount ());
    }
    else {
        strcpy (sbtext_new, "Stopped");
    }

    if (strcmp (sbtext_new, sb_text)) {
        strcpy (sb_text, sbtext_new);

        // form statusline
        GDK_THREADS_ENTER();
        // FIXME: don't update if window is not visible
        GtkStatusbar *sb = GTK_STATUSBAR (lookup_widget (mainwin, "statusbar"));
        if (sb_context_id == -1) {
            sb_context_id = gtk_statusbar_get_context_id (sb, "msg");
        }

        gtk_statusbar_pop (sb, sb_context_id);
        gtk_statusbar_push (sb, sb_context_id, sb_text);

        GDK_THREADS_LEAVE();
    }

    if (songpos != last_songpos) {
        last_songpos = songpos;
        extern int g_disable_seekbar_handler;
        g_disable_seekbar_handler = 1;
        GtkRange *seekbar = GTK_RANGE (lookup_widget (mainwin, "playpos"));
        gtk_range_set_value (seekbar, songpos);
        g_disable_seekbar_handler = 0;
    }
}

void
gtkps_songchanged (int from, int to) {
    if (from >= 0 || to >= 0) {
        GDK_THREADS_ENTER();
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        if (!widget) {
            return;
        }
        if (from >= 0) {
            redraw_ps_row (widget, from);
        }
        if (to >= 0) {
            redraw_ps_row (widget, to);
        }
        GDK_THREADS_LEAVE();
    }
}
