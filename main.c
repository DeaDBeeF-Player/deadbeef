#include <gtk/gtk.h>
#include <stdio.h>
#include <stdint.h>
#include "interface.h"
#include "support.h"
#include "playlist.h"
#include "psdl.h"
#include "unistd.h"
#include "threading.h"
#include "messagepump.h"
#include "messages.h"
#include "gtkplaylist.h"
#include "codec.h"

GtkWidget *mainwin;

int psdl_terminate = 0;

void
psdl_thread (uintptr_t ctx) {
    psdl_init ();
    while (!psdl_terminate) {
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            switch (msg) {
            case M_SONGFINISHED:
                // play next song in playlists
                GDK_THREADS_ENTER();
                gtkps_nextsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_PLAYSONG:
                GDK_THREADS_ENTER();
                gtkps_playsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_STOPSONG:
                GDK_THREADS_ENTER();
                gtkps_stopsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_NEXTSONG:
                GDK_THREADS_ENTER();
                gtkps_nextsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_PREVSONG:
                GDK_THREADS_ENTER();
                gtkps_prevsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_PAUSESONG:
                GDK_THREADS_ENTER();
                gtkps_pausesong ();
                GDK_THREADS_LEAVE();
                break;
            case M_PLAYRANDOM:
                GDK_THREADS_ENTER();
                gtkps_randomsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_SONGSEEK:
                if (playlist_current && playlist_current->codec) {
                    psdl_pause ();
                    playlist_current->codec->seek (p1 / 1000.f);
                    psdl_unpause ();
                }
                break;
            }
        }
        usleep(10);
        // handle message pump here
    }
    psdl_free ();
    ps_free ();
}

int
main (int argc, char *argv[]) {
    messagepump_init ();
    thread_start (psdl_thread, 0);

    g_thread_init (NULL);
    gdk_threads_init ();
    gdk_threads_enter ();
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
    gdk_threads_leave ();
    messagepump_free ();
    psdl_terminate = 1;
    return 0;
}
