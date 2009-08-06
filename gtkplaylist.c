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

extern GtkWidget *mainwin;
static GdkPixmap *backbuf;
static int rowheight = 17;
static int scrollpos = 0;
static int playlist_row = -1;
static double playlist_clicktime = 0;
static double ps_lastpos[2];
static int shift_sel_anchor = -1;

#define ncolumns 5
#define colname_max 100

int refit_header = 1;

const char *colnames[ncolumns] = {
    "Playing Status",
    "Album / Title",
    "Track №",
    "Title / Track Artist",
    "Duration"
};
char colnames_fitted[ncolumns][colname_max];

int colwidths[] = {
    50, 200, 50, 200, 50
};

int
fit_text (cairo_t *cr, char *out, int len, const char *in, int width) {
    int l = strlen (in);
    len--;
    l = min (len, l);
    strncpy (out, in, l);
    out[l] = 0;
    int w = 0;

    char *p = &out[l-1];
    p = g_utf8_find_prev_char (out, p);
    int processed = 0;
    for (;;) {
        cairo_text_extents_t e;
        cairo_text_extents (cr, out, &e);
        w = e.width;// + e.x_bearing + e.x_advance;
        if (e.width <= width && (processed == 0 || processed >= 3)) {
            break;
        }
        char *prev = g_utf8_find_prev_char (out, p);

        if (!prev) {
            break;
        }
        int i;
        for (i = 0; i < p-prev; i++) {
            prev[i] = '.';
        }
        processed += p-prev;
        p = prev;
        if (processed >= 3) {
            for (int i = 0; i < 3; i++) {
                p[i] = '.';
            }
            p[3] = 0;
        }
    }
    return w;
}

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
redraw_ps_row_novis (GtkWidget *widget, int row) {
	cairo_t *cr;
	cr = gdk_cairo_create (backbuf);
	if (!cr) {
		return;
	}
	//cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

//    cairo_select_font_face (cr, "fixed", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	playItem_t *it = ps_get_for_idx (row);
//    printf ("redraw row %d (selected = %d, cursor = %d)\n", row, it->selected, playlist_row == row ? 1 : 0);
	if (it) {
        draw_ps_row_back (backbuf, cr, row, it);
        draw_ps_row (backbuf, cr, row, it);
    }
    cairo_destroy (cr);
}

void
redraw_ps_row (GtkWidget *widget, int row) {
    int x, y, w, h;

    x = 0;
    y = (row  - scrollpos) * rowheight;
    w = widget->allocation.width;
    h = rowheight;

    redraw_ps_row_novis (widget, row);
	gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, x, y, x, y, w, h);
	//gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
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
        cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
        cairo_rectangle (cr, 0, row * rowheight - scrollpos * rowheight, width, rowheight-1);
        cairo_set_line_width (cr, 1);
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
    cairo_set_font_size (cr, rowheight-4);
    // draw as columns
    const char *columns[ncolumns] = {
        "",
        ps_find_meta (it, "artist"),
        ps_find_meta (it, "track"),
        ps_find_meta (it, "title"),
        "0:00"
    };
    int x = 0;
#if 1
    for (int i = 0; i < ncolumns; i++) {
        char str[512];
        if (i > 0) {
            int w = fit_text (cr, str, 512, columns[i], colwidths[i]-6);
//            printf ("draw %s -> %s\n", columns[i], str);
            if (i == 2) {
                text_draw (cr, x + colwidths[i] - w - 3, row * rowheight - scrollpos * rowheight, str);
            }
            else {
                text_draw (cr, x + 3, row * rowheight - scrollpos * rowheight, str);
            }
        }
        x += colwidths[i] + 2;
    }
#endif
#if 0
    char dname[512];
    ps_format_item_display_name (it, dname, 512);
    text_draw (cr, rowheight, row * rowheight - scrollpos * rowheight, dname);
#endif
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
	int idx = 0;
	for (row = row1; row < row2; row++, idx++) {
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
gtkps_select_single (int sel) {
    int idx=0;
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    for (playItem_t *it = playlist_head; it; it = it->next, idx++) {
        if (idx == sel) {
            if (!it->selected) {
                it->selected = 1;
                redraw_ps_row (widget, idx);
            }
        }
        else if (it->selected) {
            it->selected = 0;
            redraw_ps_row (widget, idx);
        }
    }
}

// {{{ expected behaviour for mouse1 without modifiers:
//   {{{ [+] if clicked unselected item:
//       unselect all
//       select clicked item
//       playlist_row = clicked
//       redraw
//       start 'area selection' mode
//   }}}
//   {{{ [+] if clicked selected item:
//       playlist_row = clicked
//       redraw
//       wait until next release or motion event, whichever is 1st
//       if release is 1st:
//           unselect all except clicked, redraw
//       else if motion is 1st:
//           enter drag-drop mode
//   }}}
// }}}
int areaselect = 0;
int areaselect_x = -1;
int areaselect_y = -1;
int areaselect_dx = -1;
int areaselect_dy = -1;
int dragwait = 0;
void
gtkps_mouse1_pressed (int state, int ex, int ey, double time) {
    // cursor must be set here, but selection must be handled in keyrelease
    if (ps_getcount () == 0) {
        return;
    }
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
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
    shift_sel_anchor = playlist_row;
    // handle selection
    if (!(state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)))
    {
        playItem_t *it = ps_get_for_idx (sel);
        if (!it || !it->selected) {
            // reset selection, and set it to single item
            gtkps_select_single (sel);
            areaselect = 1;
            areaselect_x = ex;
            areaselect_y = ey;
            areaselect_dx = -1;
            areaselect_dy = -1;
            shift_sel_anchor = playlist_row;
        }
        else {
            dragwait = 1;
            redraw_ps_row (widget, prev);
            redraw_ps_row (widget, playlist_row);
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
gtkps_mouse1_released (int state, int ex, int ey, double time) {
    if (dragwait) {
        dragwait = 0;
        int y = ey/rowheight + scrollpos;
        gtkps_select_single (y);
    }
    else if (areaselect) {
        areaselect = 0;
    }
}

void
gtkps_draw_areasel (GtkWidget *widget, int x, int y) {
    // erase previous rect using 4 blits from backbuffer
    if (areaselect_dx != -1) {
        int sx = min (areaselect_x, areaselect_dx);
        int sy = min (areaselect_y, areaselect_dy);
        int dx = max (areaselect_x, areaselect_dx);
        int dy = max (areaselect_y, areaselect_dy);
        int w = dx - sx + 1;
        int h = dy - sy + 1;
        //gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, sx, sy, sx, sy, dx - sx + 1, dy - sy + 1);
        gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, sx, sy, sx, sy, w, 1);
        gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, sx, sy, sx, sy, 1, h);
        gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, sx, sy + h - 1, sx, sy + h - 1, w, 1);
        gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, sx + w - 1, sy, sx + w - 1, sy, 1, h);
    }
    areaselect_dx = x;
    areaselect_dy = y;
	cairo_t *cr;
	cr = gdk_cairo_create (widget->window);
	if (!cr) {
		return;
	}
    cairo_set_source_rgb (cr, 1.f, 1.f, 1.f);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width (cr, 1);
    int sx = min (areaselect_x, x);
    int sy = min (areaselect_y, y);
    int dx = max (areaselect_x, x);
    int dy = max (areaselect_y, y);
    cairo_rectangle (cr, sx, sy, dx-sx, dy-sy);
    cairo_stroke (cr);
    cairo_destroy (cr);
}

void
gtkps_mousemove (GdkEventMotion *event) {
    if (dragwait) {
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        if (gtk_drag_check_threshold (widget, ps_lastpos[0], event->x, ps_lastpos[1], event->y)) {
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
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        int y = event->y/rowheight + scrollpos;
        if (y != shift_sel_anchor) {
            int start = min (y, shift_sel_anchor);
            int end = max (y, shift_sel_anchor);
            int idx=0;
            for (playItem_t *it = playlist_head; it; it = it->next, idx++) {
                if (idx >= start && idx <= end) {
                    it->selected = 1;
                    redraw_ps_row (widget, idx);
                }
                else if (it->selected)
                {
                    it->selected = 0;
                    redraw_ps_row (widget, idx);
                }
            }
        }
        // debug only
        // gtkps_draw_areasel (widget, event->x, event->y);
    }
}

void
gtkps_handle_scroll_event (int direction) {
    GtkWidget *range = lookup_widget (mainwin, "playscroll");
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
    int h = playlist->allocation.height / rowheight;
    int size = ps_getcount ();
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
        newscroll -= 10;//gtk_adjustment_get_page_increment (adj);
    }
    else if (direction == GDK_SCROLL_DOWN) {
        newscroll += 10;//gtk_adjustment_get_page_increment (adj);
    }
    gtk_range_set_value (GTK_RANGE (range), newscroll);
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
    if (p_ispaused ()) {
        printf ("unpause\n");
        p_unpause ();
    }
    else if (playlist_current_ptr) {
        printf ("restart\n");
        streamer_set_nextsong (ps_get_idx_of (playlist_current_ptr));
#if 0
        ps_start_current ();
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        redraw_ps_row (widget, ps_get_idx_of (playlist_current_ptr));
#endif
    }
    else if (playlist_row != -1) {
        printf ("start under cursor\n");
        streamer_set_nextsong (playlist_row);
#if 0
        playItem_t *it = ps_get_for_idx (playlist_row);
        if (it) {
            ps_set_current (it);
        }
        GtkWidget *widget = lookup_widget (mainwin, "playlist");
        redraw_ps_row (widget, playlist_row);
#endif
    }
    else {
        printf ("play 1st in list\n");
        streamer_set_nextsong (0);
#if 0
        ps_set_current (playlist_head);
        if (playlist_current_ptr) {
            GtkWidget *widget = lookup_widget (mainwin, "playlist");
            redraw_ps_row (widget, ps_get_idx_of (playlist_current_ptr));
        }
#endif
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
    p_stop ();
}

void
gtkps_pausesong (void) {
    if (p_ispaused ()) {
        p_unpause ();
    }
    else {
        p_pause ();
    }
}

void
gtkps_playsongnum (int idx) {
    streamer_set_nextsong (idx);
#if 0
    playItem_t *it = ps_get_for_idx (playlist_row);
    if (it) {
        //if (it != playlist_current_ptr)
        {
            GtkWidget *widget = lookup_widget (mainwin, "playlist");
            int prev = -1;
            if (playlist_current_ptr) {
                prev = ps_get_idx_of (playlist_current_ptr);
            }
            ps_set_current (it);
            if (prev != -1) {
                redraw_ps_row (widget, prev);
            }
            redraw_ps_row (widget, idx);
        }
    }
#endif
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
    if (p_ispaused ()) {
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

        snprintf (sbtext_new, 512, "%dHz | %d bit | %s | %d:%02d / %d:%02d | %d songs total", samplerate, bitspersample, mode, minpos, secpos, mindur, secdur, ps_getcount ());
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
        if (to >= 0) {
            playItem_t *it = ps_get_for_idx (to);
            char str[600];
            char dname[512];
            ps_format_item_display_name (it, dname, 512);
            snprintf (str, 600, "DeaDBeeF - %s", dname);
            gtk_window_set_title (GTK_WINDOW (mainwin), str);
        }
        else {
            gtk_window_set_title (GTK_WINDOW (mainwin), "DeaDBeeF");
        }
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


void
gtkps_keypress (int keyval, int state) {
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    GtkWidget *range = lookup_widget (mainwin, "playscroll");
    int prev = playlist_row;
    int newscroll = scrollpos;
    if ((keyval == GDK_A || keyval == GDK_a) && (state & GDK_CONTROL_MASK)) {
        // select all
        for (playItem_t *it = playlist_head; it; it = it->next) {
            it->selected = 1;
        }
        draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
        gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
        return;
    }
    else if (keyval == GDK_Return && playlist_row != -1) {
        messagepump_push (M_PLAYSONGNUM, 0, playlist_row, 0);
        return;
    }
    else if (keyval == GDK_Delete) {
        ps_delete_selected ();
        if (playlist_row >= ps_getcount ()) {
            playlist_row = ps_getcount () - 1;
        }
        gtkps_setup_scrollbar ();
        draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
        gtkps_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
        return;
    }
    else if (keyval == GDK_Down && playlist_row < ps_getcount () - 1) {
        playlist_row++;
        if (playlist_row > scrollpos + widget->allocation.height / rowheight - 1) {
            newscroll = playlist_row - widget->allocation.height / rowheight + 1;
        }
    }
    else if (keyval == GDK_Up && playlist_row > 0) {
        playlist_row--;
        if (playlist_row < scrollpos) {
            newscroll = playlist_row;
        }
    }
    else if (keyval == GDK_Page_Down && playlist_row < ps_getcount () - 1) {
        playlist_row += 10;
        if (playlist_row >= ps_getcount ()) {
            playlist_row = ps_getcount () - 1;
        }
        if (playlist_row > scrollpos + widget->allocation.height / rowheight - 1) {
            newscroll = playlist_row - widget->allocation.height / rowheight + 1;
        }
    }
    else if (keyval == GDK_Page_Up && playlist_row > 0) {
        playlist_row -= 10;
        if (playlist_row < 0) {
            playlist_row = 0;
        }
        if (playlist_row < scrollpos) {
            newscroll = playlist_row;
        }
    }
    else if (keyval == GDK_End && playlist_row != ps_getcount () - 1) {
        playlist_row = ps_getcount () - 1;
        if (playlist_row > scrollpos + widget->allocation.height / rowheight - 1) {
            newscroll = playlist_row - widget->allocation.height / rowheight + 1;
        }
    }
    else if (keyval == GDK_Home && playlist_row != 0) {
        playlist_row = 0;
        if (playlist_row < scrollpos) {
            newscroll = playlist_row;
        }
    }
    if (state & GDK_SHIFT_MASK) {
        // select all between shift_sel_anchor and playlist_row
        if (prev != playlist_row) {
            int start = min (playlist_row, shift_sel_anchor);
            int end = max (playlist_row, shift_sel_anchor);
            int idx=0;
            for (playItem_t *it = playlist_head; it; it = it->next, idx++) {
                if (idx >= start && idx <= end) {
//                    if (!it->selected) {
                        it->selected = 1;
                        if (newscroll == scrollpos) {
                            redraw_ps_row (widget, idx);
                        }
                        else {
                            redraw_ps_row_novis (widget, idx);
                        }
//                    }
                }
                else if (it->selected)
                {
                    it->selected = 0;
                    if (newscroll == scrollpos) {
                        redraw_ps_row (widget, idx);
                    }
                    else {
                        redraw_ps_row_novis (widget, idx);
                    }
                }
            }
        }
    }
    else {
        // reset selection, set new single cursor/selection
        if (prev != playlist_row) {
            shift_sel_anchor = playlist_row;
            int idx=0;
            for (playItem_t *it = playlist_head; it; it = it->next, idx++) {
                if (idx == playlist_row) {
                    if (!it->selected) {
                        it->selected = 1;
                        if (newscroll == scrollpos) {
                            redraw_ps_row (widget, idx);
                        }
                        else {
                            redraw_ps_row_novis (widget, idx);
                        }
                    }
                }
                else if (it->selected) {
                    it->selected = 0;
                    if (newscroll == scrollpos) {
                        redraw_ps_row (widget, idx);
                    }
                    else {
                        redraw_ps_row_novis (widget, idx);
                    }
                }
            }
        }
    }
    if (newscroll != scrollpos) {
        gtk_range_set_value (GTK_RANGE (range), newscroll);
    }
}

static int drag_motion_y = -1;

void
gtkps_track_dragdrop (int y) {
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    if (drag_motion_y != -1) {
        // erase previous track
        gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, 0, drag_motion_y * rowheight-3, 0, drag_motion_y * rowheight-3, widget->allocation.width, 7);

    }
    if (y == -1) {
        drag_motion_y = -1;
        return;
    }
	cairo_t *cr;
	cr = gdk_cairo_create (widget->window);
	if (!cr) {
		return;
	}
    drag_motion_y = y / rowheight;

    cairo_set_source_rgb (cr, 0xf4/255.f, 0x7e/255.f, 0x46/255.f);
    cairo_rectangle (cr, 0, drag_motion_y * rowheight-1, widget->allocation.width, 3);
    cairo_rectangle (cr, 0, drag_motion_y * rowheight-3, 3, 7);
    cairo_rectangle (cr, widget->allocation.width-3, drag_motion_y * rowheight-3, 3, 7);
    cairo_fill (cr);
    cairo_destroy (cr);
}

void
gtkps_handle_drag_drop (int drop_y, uint32_t *d, int length) {
    int drop_row = drop_y / rowheight + scrollpos;
    playItem_t *drop_before = ps_get_for_idx (drop_row);
    while (drop_before && drop_before->selected) {
        drop_before = drop_before->next;
    }
    // unlink items from playlist, and link together
    playItem_t *head = NULL;
    playItem_t *tail = NULL;
    int processed = 0;
    int idx = 0;
    playItem_t *next = NULL;
    for (playItem_t *it = playlist_head; it && processed < length; it = next, idx++) {
        //            printf ("idx: %d\n", d[i]);
        next = it->next;
        if (idx == d[processed]) {
            if (it->prev) {
                it->prev->next = it->next;
            }
            else {
                playlist_head = it->next;
            }
            if (it->next) {
                it->next->prev = it->prev;
            }
            else {
                playlist_tail = it->prev;
            }
            if (tail) {
                tail->next = it;
                it->prev = tail;
                tail = it;
            }
            else {
                head = tail = it;
                it->prev = it->next = NULL;
            }
            processed++;
            //                extern int ps_count;
            //                ps_count--;
        }
    }
    // find insertion point
    playItem_t *drop_after = NULL;
    if (drop_before) {
        drop_after = drop_before->prev;
    }
    else {
        drop_after = playlist_tail;
    }
    // insert in between
    head->prev = drop_after;
    if (drop_after) {
        drop_after->next = head;
    }
    else {
        playlist_head = head;
    }
    tail->next = drop_before;
    if (drop_before) {
        drop_before->prev = tail;
    }
    else {
        playlist_tail = tail;
    }
}

void
strcopy_special (char *dest, const char *src, int len) {
    while (len > 0) {
        if (len >= 3 && !strncmp (src, "\%20", 3)) {
            *dest = ' ';
            dest++;
            src += 3;
            len -= 3;
        }
        else {
            *dest++ = *src++;
            len--;
        }
    }
    *dest = 0;
}

void
gtkps_handle_fm_drag_drop (int drop_y, void *ptr, int length) {
    int drop_row = drop_y / rowheight + scrollpos;
    playItem_t *drop_before = ps_get_for_idx (drop_row);
    playItem_t *after = NULL;
    if (drop_before) {
        after = drop_before->prev;
    }
    //printf ("data: %s\n", ptr);
    // this happens when dropped from file manager
    // parse, and try to add to playlist
    const gchar *p = ptr;
    while (*p) {
        const gchar *pe = p+1;
        while (*pe && *pe > ' ') {
            pe++;
        }
        if (pe - p < 4096 && pe - p > 7) {
            char fname[(int)(pe - p)];
            strcopy_special (fname, p, pe-p);
            //strncpy (fname, p, pe - p);
            //fname[pe - p] = 0;
            playItem_t *inserted = ps_insert_dir (after, fname + 7);
            if (!inserted) {
                inserted = ps_insert_file (after, fname + 7);
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
    gtkps_setup_scrollbar ();
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkps_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
}

gboolean
on_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    int x = 0;
    int w = 100;
    int h = widget->allocation.height;
    const char *detail = "toolbar";

    for (int i = 0; i < ncolumns; i++) {
        if (x >= widget->allocation.width) {
            break;
        }
        w = colwidths[i];
        gtk_paint_box (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, NULL, detail, x, 0, w, h);
        gtk_paint_vline (widget->style, widget->window, GTK_STATE_NORMAL, NULL, NULL, NULL, 0, h, x+w);
        x += w + 2;
    }
    if (x < widget->allocation.width) {
        gtk_paint_box (widget->style, widget->window, GTK_STATE_INSENSITIVE, GTK_SHADOW_OUT, NULL, NULL, detail, x, 0, widget->allocation.width-x, h);
    }
    cairo_t *cr;
    cr = gdk_cairo_create (widget->window);
    if (!cr) {
        return FALSE;
    }
    x = 0;
    for (int i = 0; i < ncolumns; i++) {
        if (x >= widget->allocation.width) {
            break;
        }
        w = colwidths[i];
        cairo_move_to (cr, x + 5, 15);
        if (refit_header) {
            fit_text (cr, colnames_fitted[i], colname_max, colnames[i], colwidths[i]-10);
        }
        cairo_show_text (cr, colnames_fitted[i]);
        x += w + 2;
    }
    refit_header = 0;
    cairo_destroy (cr);
    return FALSE;
}


gboolean
on_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    return FALSE;
}


GdkCursor* cursor_sz;

void
on_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data)
{
    // create cursor for sizing headers
    cursor_sz = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
}

gboolean
on_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    int x = 0;
    for (int i = 0; i < ncolumns; i++) {
        int w = colwidths[i];
        if (event->x >= x + w && event->x <= x+w+2) {
            gdk_window_set_cursor (widget->window, cursor_sz);
            break;
        }
        else {
            gdk_window_set_cursor (widget->window, NULL);
        }
        x += w + 2;
    }
    return FALSE;
}

