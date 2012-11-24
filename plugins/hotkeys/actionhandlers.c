/*
    hotkey action handlers for deadbeef hotkeys plugin
    Copyright (C) 2009-2012 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include "../../gettext.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

int
action_jump_to_current_handler (DB_plugin_action_t *act, int ctx) {
    deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
    return 0;
}

int
action_reload_metadata_handler (DB_plugin_action_t *act, int ctx) {
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        deadbeef->pl_lock ();
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        deadbeef->pl_unlock ();

        if (match) {
            uint32_t f = deadbeef->pl_get_item_flags (it);
            if (!(f & DDB_IS_SUBTRACK)) {
                f &= ~DDB_TAG_MASK;
                deadbeef->pl_set_item_flags (it, f);
                DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
                for (int i = 0; decoders[i]; i++) {
                    if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                        if (decoders[i]->read_metadata) {
                            decoders[i]->read_metadata (it);
                        }
                        break;
                    }
                }
            }
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
    return 0;
}

int
action_next_playlist_handler (DB_plugin_action_t *act, int ctx) {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab < deadbeef->plt_get_count ()-1) {
        tab++;
        deadbeef->plt_set_curr_idx (tab);
        deadbeef->conf_set_int ("playlist.current", tab);
    }
    return 0;
}

int
action_prev_playlist_handler (DB_plugin_action_t *act, int ctx) {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab > 0) {
        tab--;
        deadbeef->plt_set_curr_idx (tab);
        deadbeef->conf_set_int ("playlist.current", tab);
    }
    return 0;
}

int
action_playlist1_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 0;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist2_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 1;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist3_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 2;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist4_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 3;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist5_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 4;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist6_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 5;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist7_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 6;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist8_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 7;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist9_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 8;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_playlist10_handler (DB_plugin_action_t *act, int ctx) {
    int pl = 9;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return 0;
}

int
action_sort_randomize_handler (DB_plugin_action_t *act, int ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return 0;
}

int
action_sort_by_date_handler (DB_plugin_action_t *act, int ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%y", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return 0;
}

int
action_sort_by_artist_handler (DB_plugin_action_t *act, int ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%a", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return 0;
}

int
action_sort_by_album_handler (DB_plugin_action_t *act, int ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%b", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return 0;
}

int
action_sort_by_tracknr_handler (DB_plugin_action_t *act, int ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%n", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return 0;
}

int
action_sort_by_title_handler (DB_plugin_action_t *act, int ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%t", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return 0;
}
