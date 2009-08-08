#include <gtk/gtk.h>
#include <stdio.h>
#include <stdint.h>
#include "interface.h"
#include "support.h"
#include "playlist.h"
#include "playback.h"
#include "unistd.h"
#include "threading.h"
#include "messagepump.h"
#include "messages.h"
#include "gtkplaylist.h"
#include "codec.h"
#include "streamer.h"

GtkWidget *mainwin;

int psdl_terminate = 0;

void
psdl_thread (uintptr_t ctx) {
    p_play ();
    while (!psdl_terminate) {
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            switch (msg) {
            case M_SONGCHANGED:
                gtkps_songchanged (p1, p2);
                break;
            case M_PLAYSONG:
                GDK_THREADS_ENTER();
                gtkps_playsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_PLAYSONGNUM:
                GDK_THREADS_ENTER();
                gtkps_playsongnum (p1);
                GDK_THREADS_LEAVE();
                break;
            case M_STOPSONG:
                GDK_THREADS_ENTER();
                gtkps_stopsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_NEXTSONG:
                GDK_THREADS_ENTER();
                p_stop ();
                ps_nextsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_PREVSONG:
                GDK_THREADS_ENTER();
                p_stop ();
                ps_prevsong ();
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
                if (playlist_current.codec) {
                    p_pause ();
                    codec_lock ();
                    playlist_current.codec->seek (p1 / 1000.f);
                    codec_unlock ();
                    p_unpause ();
                }
                break;
            }
        }
        usleep(10000);
        gtkps_update_songinfo ();
        // handle message pump here
    }
}

int
main (int argc, char *argv[]) {
    messagepump_init ();
    codec_init_locking ();
    streamer_init ();
    p_init ();
    thread_start (psdl_thread, 0);

    g_thread_init (NULL);
    gdk_threads_init ();
    gdk_threads_enter ();
    gtk_set_locale ();
    gtk_init (&argc, &argv);

    mainwin = create_mainwin ();
    gtk_widget_show (mainwin);
    gtk_main ();
    gdk_threads_leave ();
    messagepump_free ();
    psdl_terminate = 1;
    p_free ();
    streamer_free ();
    codec_free_locking ();
    ps_free ();
    return 0;
}
