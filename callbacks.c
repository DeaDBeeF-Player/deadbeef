#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "psdl.h"
#include "playlist.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

extern GtkWidget *mainwin;
static GdkPixmap *backbuf;
static int rowheight = 12;
int trackerscroll = 0;
static int playlist_row = -1;
static double playlist_clicktime = 0;
static double ps_lastpos[2];

static void
addfile_func (gpointer data, gpointer userdata) {
    ps_add_file (data);
    g_free (data);
}

static void
setup_ps_scrollbar (void) {
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
    int h = playlist->allocation.height / rowheight;
    int size = ps_getcount ();
    if (h >= size) {
        size = 0;
    }
    GtkWidget *scroll = lookup_widget (mainwin, "playscroll");
    printf ("%d\n", size);
//    gtk_range_set_range (GTK_RANGE (scroll), 0, size);
//    gtk_range_set_increments (GTK_RANGE (scroll), 1, h);
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, size, 1, h, h);
    gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
}

void
on_addbtn_clicked                      (GtkButton       *button,
        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add file(s) to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Supported music files");
    gtk_file_filter_add_pattern (flt, "*.ogg");
    gtk_file_filter_add_pattern (flt, "*.mod");
    gtk_file_filter_add_pattern (flt, "*.wav");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Other files (*)");
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        g_slist_foreach(lst, addfile_func, NULL);
        g_slist_free (lst);
    }
    gtk_widget_destroy (dlg);
    setup_ps_scrollbar ();
}


void
on_playbtn_clicked                     (GtkButton       *button,
        gpointer         user_data)
{

}


void
on_volume_value_changed                (GtkRange        *range,
        gpointer         user_data)
{
    psdl_set_volume (gtk_range_get_value (range) / 100);
}


void
on_playpos_value_changed               (GtkRange        *range,
        gpointer         user_data)
{

}


// change properties
gboolean
on_playlist_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    setup_ps_scrollbar ();
    if (backbuf) {
        g_object_unref (backbuf);
    }
    backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);

    return FALSE;
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
        cairo_set_source_rgb (cr, 0.22, 0.1, 0.1);
        cairo_rectangle (cr, 0, row * rowheight - trackerscroll * rowheight, width, rowheight);
        cairo_fill (cr);
    }
    else {
        if (row % 2) {
            cairo_set_source_rgb (cr, 0.88, 0.88, 0.88);
            cairo_rectangle (cr, 0, row * rowheight - trackerscroll * rowheight, w, rowheight);
            cairo_fill (cr);
        }
        else {
            cairo_set_source_rgb (cr, 0.77, 0.77, 0.77);
            cairo_rectangle (cr, 0, row * rowheight - trackerscroll * rowheight, width, rowheight);
            cairo_fill (cr);
        }
    }
//	start = min (vbstart[1], vbend[1]);
//	end = max (vbstart[1], vbend[1]);
//	startx = min (vbstart[0], vbend[0]);
//	endx = max (vbstart[0], vbend[0]);
//	if (tracker_vbmode && row >= start && row <= end) { // hilight selected notes
//		cairo_set_source_rgb (cr, 0.0, 0.44, 0.0);
//		cairo_rectangle (cr, startx * colwidth * COL_NUM_CHARS + colwidth * 3, row * rowheight - trackerscroll * rowheight, (endx - startx + 1) * colwidth * COL_NUM_CHARS, rowheight);
//		cairo_fill (cr);
//	}
}

static void
text_draw (cairo_t *cr, int x, int y, const char *text) {
    cairo_move_to (cr, x, y+rowheight-3);
    cairo_show_text (cr, text);
}

void
draw_ps_row (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it) {
	if (row == playlist_row) {
        cairo_set_source_rgb (cr, 1, 1, 1);
    }
    else {
        cairo_set_source_rgb (cr, 0, 0, 0);
    }
    text_draw (cr, 0, row * rowheight - trackerscroll * rowheight, it->displayname);
}

void
redraw_ps_row (GtkWidget *widget, int row) {
    int x, y, w, h;

    x = 0;
    y = (row  - trackerscroll) * rowheight;
    w = widget->allocation.width;
    h = rowheight;

	cairo_t *cr;
	cr = gdk_cairo_create (backbuf);
	if (!cr) {
		return;
	}

    cairo_select_font_face (cr, "fixed", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, rowheight);

	playItem_t *it = ps_get_for_idx (row);
	if (it) {
        draw_ps_row_back (backbuf, cr, row);
        draw_ps_row (backbuf, cr, row, it);
    }
    cairo_destroy (cr);
	gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, x, y, x, y, w, h);
}

void draw_playlist (GtkWidget *widget, int x, int y, int w, int h) {
	cairo_t *cr;
	cr = gdk_cairo_create (backbuf);
	if (!cr) {
		return;
	}

    cairo_select_font_face (cr, "fixed", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, rowheight);
	int row;
	int row1;
	int row2;
	int row2_full;
	row1 = max (0, y / rowheight + trackerscroll);
	row2 = min (ps_getcount (), (y+h) / rowheight + trackerscroll + 1);
	row2_full = (y+h) / rowheight + trackerscroll + 1;
	// draw background
	for (row = row1; row < row2_full; row++) {
		draw_ps_row_back (backbuf, cr, row);
	}
	playItem_t *it = ps_get_for_idx (trackerscroll);
	for (row = row1; row < row2; row++) {
		draw_ps_row (backbuf, cr, row, it);
		it = it->next;
	}

    cairo_destroy (cr);
	gdk_draw_drawable (widget->window, widget->style->black_gc, backbuf, x, y, x, y, w, h);
}


// redraw
gboolean
on_playlist_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    // draw visible area of playlist
    draw_playlist (widget, event->area.x, event->area.y, event->area.width, event->area.height);

    return FALSE;
}


void
on_playlist_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{

}


gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    // remember mouse coords for doubleclick detection
    ps_lastpos[0] = event->x;
    ps_lastpos[1] = event->y;
    // select item
    int y = event->y/rowheight + trackerscroll;
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

    if (event->time - playlist_clicktime < 0.5
            && fabs(ps_lastpos[0] - event->x) < 3
            && fabs(ps_lastpos[1] - event->y) < 3) {
        // doubleclick - play this item
        if (playlist_row != -1) {
            playItem_t *it = ps_get_for_idx (playlist_row);
            if (it) {
                psdl_stop ();
                psdl_play (it);
            }
        }


        // prevent next click to trigger doubleclick
        playlist_clicktime = event->time-1;
    }
    else {
        playlist_clicktime = event->time;
    }
    return FALSE;
}


void
on_playscroll_value_changed            (GtkRange        *range,
                                        gpointer         user_data)
{
    trackerscroll = gtk_range_get_value (GTK_RANGE (range));
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
    draw_playlist (playlist, 0, 0, playlist->allocation.width, playlist->allocation.height);
}

