/*
    hotkey action handlers for deadbeef hotkeys plugin
    Copyright (C) 2009-2013 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

int
action_jump_to_current_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
    return 0;
}

static DB_playItem_t*
skip_to_get_track_helper (void) {
    DB_playItem_t *current = deadbeef->streamer_get_playing_track_safe ();
    if (!current) {
        return NULL;
    }

    ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();
    ddb_playlist_t *plt = deadbeef->pl_get_playlist (current);

    DB_playItem_t *it = NULL;
    if (plt && plt_curr && plt != plt_curr) {
        deadbeef->pl_item_unref (current);
        it = deadbeef->plt_get_first (plt_curr, PL_MAIN);
        while (it) {
            if (deadbeef->pl_is_selected (it)) {
                break;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    else {
        it = current;
    }

    if (plt) {
        deadbeef->plt_unref (plt);
    }
    if (plt_curr) {
        deadbeef->plt_unref (plt_curr);
    }
    return it;
}

static void
skip_to_prev_helper (const char *meta) {
    if (!meta) {
        return;
    }
    deadbeef->pl_lock ();
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == DDB_PLAYBACK_STATE_STOPPED) {
        deadbeef->pl_unlock ();
        return;
    }

    DB_playItem_t *it = skip_to_get_track_helper ();
    if (!it) {
        deadbeef->pl_unlock ();
        return;
    }

    const char *cur_meta = deadbeef->pl_find_meta_raw (it, meta);
    int c = 0;
    while (it) {
        DB_playItem_t *prev = deadbeef->pl_get_prev (it, PL_MAIN);
        if (!prev) {
            if (c == 1) {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of (it), 0);
            }
            deadbeef->pl_item_unref (it);
            break;
        }
        const char *prev_meta = deadbeef->pl_find_meta_raw (prev, meta);
        if (cur_meta != prev_meta) {
            if (c == 0) {
                cur_meta = prev_meta;
                c = 1;
            }
            else {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of (it), 0);
                deadbeef->pl_item_unref (it);
                deadbeef->pl_item_unref (prev);
                break;
            }
        }
        deadbeef->pl_item_unref (it);
        it = prev;
    }
    deadbeef->pl_unlock ();
}

static void
skip_to_next_helper (const char *meta) {
    if (!meta) {
        return;
    }
    deadbeef->pl_lock ();
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == DDB_PLAYBACK_STATE_STOPPED) {
        deadbeef->pl_unlock ();
        return;
    }

    DB_playItem_t *it = skip_to_get_track_helper ();
    if (!it) {
        deadbeef->pl_unlock ();
        return;
    }

    const char *cur_meta = deadbeef->pl_find_meta_raw (it, meta);
    while (it) {
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        if (!next) {
            deadbeef->pl_item_unref (it);
            break;
        }
        const char *next_meta = deadbeef->pl_find_meta_raw (next, meta);
        if (cur_meta != next_meta) {
            deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of (next), 0);
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (next);
            break;
        }
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
}

int
action_skip_to_next_album_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    skip_to_next_helper ("album");
    return 0;
}

int
action_skip_to_next_genre_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    skip_to_next_helper ("genre");
    return 0;
}

int
action_skip_to_next_composer_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    skip_to_next_helper ("composer");
    return 0;
}

int
action_skip_to_prev_album_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    skip_to_prev_helper ("album");
    return 0;
}

int
action_skip_to_prev_genre_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    skip_to_prev_helper ("genre");
    return 0;
}

int
action_skip_to_prev_composer_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    skip_to_prev_helper ("composer");
    return 0;
}

int
action_skip_to_next_artist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    deadbeef->pl_lock ();
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == DDB_PLAYBACK_STATE_STOPPED) {
        deadbeef->pl_unlock ();
        return 0;
    }

    DB_playItem_t *it = skip_to_get_track_helper ();
    if (!it) {
        deadbeef->pl_unlock ();
        return 0;
    }

    const char *cur_artist = deadbeef->pl_find_meta_raw (it, "band");
    if (!cur_artist) {
        cur_artist = deadbeef->pl_find_meta_raw (it, "album artist");
    }
    if (!cur_artist) {
        cur_artist = deadbeef->pl_find_meta_raw (it, "albumartist");
    }
    if (!cur_artist) {
        cur_artist = deadbeef->pl_find_meta_raw (it, "artist");
    }
    while (it) {
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        if (!next) {
            deadbeef->pl_item_unref (it);
            break;
        }
        const char *next_artist = deadbeef->pl_find_meta_raw (next, "band");
        if (!next_artist) {
            next_artist = deadbeef->pl_find_meta_raw (next, "album artist");
        }
        if (!next_artist) {
            next_artist = deadbeef->pl_find_meta_raw (next, "albumartist");
        }
        if (!next_artist) {
            next_artist = deadbeef->pl_find_meta_raw (next, "artist");
        }

        if (cur_artist != next_artist) {
            deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of (next), 0);
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (next);
            break;
        }
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    return 0;
}

int
action_skip_to_prev_artist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    deadbeef->pl_lock ();
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == DDB_PLAYBACK_STATE_STOPPED) {
        deadbeef->pl_unlock ();
        return 0;
    }

    DB_playItem_t *it = skip_to_get_track_helper ();
    if (!it) {
        deadbeef->pl_unlock ();
        return 0;
    }

    const char *cur_artist = deadbeef->pl_find_meta_raw (it, "band");
    if (!cur_artist) {
        cur_artist = deadbeef->pl_find_meta_raw (it, "album artist");
    }
    if (!cur_artist) {
        cur_artist = deadbeef->pl_find_meta_raw (it, "albumartist");
    }
    if (!cur_artist) {
        cur_artist = deadbeef->pl_find_meta_raw (it, "artist");
    }
    int c = 0;
    while (it) {
        DB_playItem_t *prev = deadbeef->pl_get_prev (it, PL_MAIN);
        if (!prev) {
            if (c == 1) {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of (it), 0);
            }
            deadbeef->pl_item_unref (it);
            break;
        }
        const char *prev_artist = deadbeef->pl_find_meta_raw (prev, "band");
        if (!prev_artist) {
            prev_artist = deadbeef->pl_find_meta_raw (prev, "album artist");
        }
        if (!prev_artist) {
            prev_artist = deadbeef->pl_find_meta_raw (prev, "albumartist");
        }
        if (!prev_artist) {
            prev_artist = deadbeef->pl_find_meta_raw (prev, "artist");
        }

        if (cur_artist != prev_artist) {
            if (c == 0) {
                cur_artist = prev_artist;
                c = 1;
            }
            else {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of (it), 0);
                deadbeef->pl_item_unref (it);
                deadbeef->pl_item_unref (prev);
                break;
            }
        }
        deadbeef->pl_item_unref (it);
        it = prev;
    }
    deadbeef->pl_unlock ();
    return 0;
}

int
action_reload_metadata_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        deadbeef->pl_lock ();
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match;
        if (ctx == DDB_ACTION_CTX_PLAYLIST) {
            match = deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        }
        else {
            match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        }
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
action_next_playlist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int tab = deadbeef->plt_get_curr_idx ();

    if (tab == deadbeef->plt_get_count ()-1) {
        tab = 0;
    }
    else {
        tab++;
    }

    deadbeef->plt_set_curr_idx (tab);

    return 0;
}

int
action_prev_playlist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int tab = deadbeef->plt_get_curr_idx ();

    if (tab == 0) {
        tab = deadbeef->plt_get_count ()-1;
    }
    else {
        tab--;
    }

    deadbeef->plt_set_curr_idx (tab);

    return 0;
}

int
action_playlist1_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 0;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist2_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 1;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist3_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 2;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist4_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 3;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist5_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 4;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist6_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 5;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist7_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 6;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist8_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 7;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist9_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 8;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_playlist10_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int pl = 9;
    if (pl < deadbeef->plt_get_count ()) {
        deadbeef->plt_set_curr_idx (pl);
    }
    return 0;
}

int
action_sort_randomize_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_sort_by_date_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%year%", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_sort_by_artist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%artist%", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_sort_by_album_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%album%", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_sort_by_tracknr_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%tracknumber%", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_sort_by_title_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%title%", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_invert_selection_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        else {
            deadbeef->pl_set_selected (it, 1);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
    return 0;
}

int
action_clear_playlist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    deadbeef->pl_clear ();
    deadbeef->pl_save_current ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_toggle_in_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->action_get_playlist ();

    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        if (ctx == DDB_ACTION_CTX_PLAYLIST || (ctx == DDB_ACTION_CTX_SELECTION && deadbeef->pl_is_selected (it))) {
            if(deadbeef->playqueue_test(it) != -1)
                deadbeef->playqueue_remove (it);
            else
                deadbeef->playqueue_push (it);

        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    deadbeef->plt_unref (plt);
    return 0;
}

int
action_move_tracks_up_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playItem_t *playing_track = NULL;
    if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        playing_track = deadbeef->streamer_get_playing_track_safe ();
    }

    deadbeef->pl_lock ();

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    DB_playItem_t *it;

    int it_count = 0;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        it_count = deadbeef->pl_getselcount ();
        if (it_count) {
            int i = 0;
            uint32_t indexes[it_count];
            memset (indexes, 0, sizeof (indexes));

            it = deadbeef->plt_get_first (plt, PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    indexes[i] = deadbeef->pl_get_idx_of (it);
                    i++;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
            DB_playItem_t *drop_before = deadbeef->pl_get_for_idx (indexes[0]-1);
            if (drop_before) {
                deadbeef->plt_move_items (plt, PL_MAIN, plt, drop_before, indexes, it_count);
                deadbeef->pl_item_unref (drop_before);
            }
        }
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        if (playing_track) {
            it_count = 1;
            uint32_t indexes[1];
            indexes[0] = deadbeef->pl_get_idx_of (playing_track);
            DB_playItem_t *drop_before = deadbeef->pl_get_prev (playing_track,PL_MAIN);
            if (drop_before) {
                deadbeef->plt_move_items (plt, PL_MAIN, plt, drop_before, indexes, it_count);
                deadbeef->pl_item_unref (drop_before);
            }
        }
    }

    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->pl_unlock ();

    if (playing_track != NULL) {
        deadbeef->pl_item_unref (playing_track);
    }

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_move_tracks_down_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playItem_t *playing_track = NULL;
    if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        playing_track = deadbeef->streamer_get_playing_track_safe ();
    }

    deadbeef->pl_lock ();

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    DB_playItem_t *it;

    int it_count = 0;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        int it_count = deadbeef->pl_getselcount ();
        if (it_count) {
            int i = 0;
            uint32_t indexes[it_count];
            memset (indexes, 0, sizeof (indexes));

            it = deadbeef->plt_get_first (plt, PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    indexes[i] = deadbeef->pl_get_idx_of (it);
                    i++;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
            DB_playItem_t *drop_before = deadbeef->pl_get_for_idx (indexes[i-1]+2);
            deadbeef->plt_move_items (plt, PL_MAIN, plt, drop_before, indexes, it_count);
            if (drop_before)
                deadbeef->pl_item_unref (drop_before);
        }
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        if (playing_track) {
            it_count = 1;
            uint32_t indexes[1];
            indexes[0] = deadbeef->pl_get_idx_of (playing_track);
            DB_playItem_t *drop_before = deadbeef->pl_get_for_idx (indexes[0]+2);
            deadbeef->plt_move_items (plt, PL_MAIN, plt, drop_before, indexes, it_count);
            if (drop_before)
                deadbeef->pl_item_unref (drop_before);
        }
    }

    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->pl_unlock ();

    if (playing_track != NULL) {
        deadbeef->pl_item_unref (playing_track);
    }

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

int
action_add_to_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->action_get_playlist ();

    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        if (ctx == DDB_ACTION_CTX_PLAYLIST || (ctx == DDB_ACTION_CTX_SELECTION && deadbeef->pl_is_selected (it))) {
            deadbeef->playqueue_push (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    deadbeef->plt_unref (plt);

    return 0;
}

int
action_prepend_to_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->action_get_playlist ();

    DB_playItem_t *it = deadbeef->plt_get_last (plt, PL_MAIN);
    while (it) {
        if (ctx == DDB_ACTION_CTX_PLAYLIST || (ctx == DDB_ACTION_CTX_SELECTION && deadbeef->pl_is_selected (it))) {
            deadbeef->playqueue_insert_at (0, it);
        }
        DB_playItem_t *prev = deadbeef->pl_get_prev (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = prev;
    }

    deadbeef->plt_unref (plt);
    return 0;
}

int
action_remove_from_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->action_get_playlist ();

    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        if (ctx == DDB_ACTION_CTX_PLAYLIST || (ctx == DDB_ACTION_CTX_SELECTION && deadbeef->pl_is_selected (it))) {
            deadbeef->playqueue_remove (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    deadbeef->plt_unref (plt);
    return 0;
}

int
action_toggle_mute_handler (DB_plugin_action_t *act, ddb_action_context_t ctx) {
    int mute = 1-deadbeef->audio_is_mute ();
    deadbeef->audio_set_mute (mute);
    return 0;
}

int
action_prev_or_restart_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        float dur = deadbeef->pl_get_item_duration (it);
        deadbeef->pl_item_unref (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            if (pos > 3) {
                deadbeef->sendmessage (DB_EV_SEEK, 0, 0, 0);
                return 0;
            }
        }
    }
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
    return 0;
}

static void
_copy_playlist_int (ddb_playlist_t *src, ddb_playlist_t *dst) {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->plt_get_first (src, PL_MAIN);
    DB_playItem_t *after = NULL;
    while (it) {
        DB_playItem_t *it_new = deadbeef->pl_item_alloc ();
        deadbeef->pl_item_copy (it_new, it);
        deadbeef->plt_insert_item (dst, after, it_new);

        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        if (after) {
            deadbeef->pl_item_unref (after);
        }
        after = it_new;
        deadbeef->pl_item_unref (it);
        it = next;
    }
    if (after) {
        deadbeef->pl_item_unref (after);
    }
    deadbeef->pl_unlock ();
    deadbeef->plt_save_config (dst);
}

static int
_copy_playlist (ddb_playlist_t *plt) {
    char orig_title[100];
    deadbeef->plt_get_title (plt, orig_title, sizeof (orig_title));

    int cnt = deadbeef->plt_get_count ();
    int i;
    int idx = 0;
    for (;;) {
        char name[100];
        if (!idx) {
            snprintf (name, sizeof (name), _("Copy of %s"), orig_title);
        }
        else {
            snprintf (name, sizeof (name), _("Copy of %s (%d)"), orig_title, idx);
        }
        deadbeef->pl_lock ();
        for (i = 0; i < cnt; i++) {
            char t[100];
            ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
            deadbeef->plt_get_title (plt, t, sizeof (t));
            deadbeef->plt_unref (plt);
            if (!strcasecmp (t, name)) {
                break;
            }
        }
        deadbeef->pl_unlock ();
        if (i == cnt) {
            int new_plt_idx = deadbeef->plt_add (cnt, name);
            if (new_plt_idx < 0) {
                return -1;
            }
            ddb_playlist_t *new_plt = deadbeef->plt_get_for_idx (new_plt_idx);
            if (!new_plt) {
                return -1;
            }
            _copy_playlist_int(plt, new_plt);
            return new_plt_idx;
        }
        idx++;
    }
    return -1;
}


int
action_duplicate_playlist_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (plt == NULL) {
        return -1;
    }

    int playlist = _copy_playlist (plt);
    if (playlist != -1) {
        deadbeef->plt_set_curr_idx (playlist);
    }
    deadbeef->plt_unref (plt);
    return 0;
}
