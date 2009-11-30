#include "../../deadbeef.h"
#include <gtk/gtk.h>
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
    deadbeef->sendmessage (M_PLAYSONG, 0, 0, 0);
}

void
gtkui_open_files (struct _GSList *lst) {
    deadbeef->pl_free ();
    deadbeef->thread_start (open_files_worker, lst);
}
