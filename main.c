#include <gtk/gtk.h>
#include "interface.h"
#include "support.h"
#include <stdio.h>
#include "playlist.h"
#include "psdl.h"
#include "unistd.h"


GtkWidget *mainwin;

int
main (int argc, char *argv[]) {
    if (argc <= 1) {
        printf ("syntax: deadbeef <filename>\n");
        return -1;
    }
    psdl_init ();
    if (!ps_add_file (argv[1])) {
        printf ("playing %s\n", argv[1]);
        psdl_play (playlist_head);
    }
    else {
        printf ("failed to play %s\n", argv[1]);
    }
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
    psdl_free ();
    ps_free ();
    return 0;
}
