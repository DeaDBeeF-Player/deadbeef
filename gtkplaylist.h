#ifndef __GTKPLAYLIST_H
#define __GTKPLAYLIST_H

#include <gtk/gtk.h>
#include "playlist.h"

void
gtkps_nextsong (void);

void
redraw_ps_row (GtkWidget *widget, int row);

void
gtkps_setup_scrollbar (void);

void
draw_ps_row_back (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it);

void
draw_ps_row (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it);

void
draw_playlist (GtkWidget *widget, int x, int y, int w, int h);

void
gtkps_reconf (GtkWidget *widget);

void
gtkps_expose (GtkWidget       *widget, int x, int y, int w, int h);

void
gtkps_mouse1_clicked (GtkWidget *widget, int state, int ex, int ey, double time);

void
gtkps_scroll (int newscroll);

void
gtkps_stopsong (void);

void
gtkps_playsong (void);

void
gtkps_prevsong (void);

void
gtkps_nextsong (void);

void
gtkps_randomsong (void);

void
gtkps_pausesong (void);

void
gtkps_playsongnum (int idx);

void
gtkps_update_songinfo (void);

void
gtkps_songchanged (int from, int to);

#endif
