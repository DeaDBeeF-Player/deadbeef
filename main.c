#include <stdio.h>
#include "playlist.h"
#include "psdl.h"
#include "unistd.h"

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
        sleep (10);
    }
    else {
        printf ("failed to play %s\n", argv[1]);
    }
    psdl_free ();
    return 0;
}
