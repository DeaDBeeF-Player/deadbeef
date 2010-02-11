#include "../../deadbeef.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include "gtkui.h"
#include "gtkplaylist.h"

static void
add_dirs_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_dirs (&main_playlist, lst);
}

void
gtkui_add_dirs (GSList *lst) {
    deadbeef->thread_start (add_dirs_worker, lst);
}

static void
add_files_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_files (&main_playlist, lst);
}

void
gtkui_add_files (struct _GSList *lst) {
    deadbeef->thread_start (add_files_worker, lst);
}

static void
open_files_worker (void *data) {
    GSList *lst = (GSList *)data;
    gtkpl_add_files (&main_playlist, lst);
    gtkpl_set_cursor (PL_MAIN, 0);
    deadbeef->sendmessage (M_PLAYSONG, 0, 0, 0);
}

void
gtkui_open_files (struct _GSList *lst) {
    deadbeef->pl_clear ();
    deadbeef->thread_start (open_files_worker, lst);
}

struct fmdrop_data {
    char *mem;
    int length;
    int drop_y;
};

static void
fmdrop_worker (void *ctx) {
    struct fmdrop_data *data = (struct fmdrop_data *)ctx;
    gtkpl_add_fm_dropped_files (&main_playlist, data->mem, data->length, data->drop_y);
    free (data);
}

void
gtkui_receive_fm_drop (char *mem, int length, int drop_y) {
    struct fmdrop_data *data = malloc (sizeof (struct fmdrop_data));
    if (!data) {
        fprintf (stderr, "gtkui_receive_fm_drop: malloc failed\n");
        return;
    }
    data->mem = mem;
    data->length = length;
    data->drop_y = drop_y;
    deadbeef->thread_start (fmdrop_worker, data);
}
