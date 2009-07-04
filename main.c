#include <gtk/gtk.h>
#include <stdio.h>
#include <stdint.h>
#include "interface.h"
#include "support.h"
#include "playlist.h"
#include "psdl.h"
#include "unistd.h"
#include "threading.h"

GtkWidget *mainwin;

int psdl_terminate = 0;

void
psdl_thread (uintptr_t ctx) {
    psdl_init ();
    while (!psdl_terminate) {
        sleep(1);
        // handle message pump here
    }
    psdl_free ();
    ps_free ();
}

int
main (int argc, char *argv[]) {
    if (argc <= 1) {
        printf ("syntax: deadbeef <filename>\n");
        return -1;
    }

    thread_start (psdl_thread, 0);

    gtk_set_locale ();
    gtk_init (&argc, &argv);

    /*
     * The following code was added by Glade to create one of each component
     * (except popup menus), just so that you see something after building
     * the project. Delete any components that you don't want shown initially.
     */
    mainwin = create_mainwin ();
    gtk_widget_show (mainwin);
    gtk_main ();
    psdl_terminate = 1;
    return 0;
}
