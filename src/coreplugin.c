/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

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

#include "coreplugin.h"
#include "actionhandlers.h"

static DB_functions_t *deadbeef;

int
action_play_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    // NOTE: this function is copied as on_playbtn_clicked in gtkui
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == DDB_PLAYBACK_STATE_PAUSED) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cur != -1) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cur, PL_MAIN);
            ddb_playItem_t *it_playing = deadbeef->streamer_get_playing_track_safe ();
            if (it) {
                deadbeef->pl_item_unref (it);
            }
            if (it_playing) {
                deadbeef->pl_item_unref (it_playing);
            }
            if (it != it_playing) {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
            }
            else {
                deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
            }
        }
        else {
            deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
        }
        deadbeef->plt_unref (plt);
    }
    else {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = -1;
        if (plt) {
            cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
            deadbeef->plt_unref (plt);
        }
        if (cur == -1) {
            cur = 0;
        }
        deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
    }
    return 0;
}

int
action_prev_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
    return 0;
}

int
action_next_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
    return 0;
}

int
action_stop_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
    return 0;
}

int
action_toggle_pause_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
    return 0;
}

int
action_play_pause_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    ddb_playback_state_t state = deadbeef->get_output ()->state ();
    if (state == DDB_PLAYBACK_STATE_PLAYING) {
        deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
    }
    else {
        deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    }
    return 0;
}

int
action_play_random_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_PLAY_RANDOM, 0, 0, 0);
    return 0;
}

int
action_play_random_album_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_PLAY_RANDOM_ALBUM, 0, 0, 0);
    return 0;
}

int
action_play_next_album_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_PLAY_NEXT_ALBUM, 0, 0, 0);
    return 0;
}

int
action_play_prev_album_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->sendmessage (DB_EV_PLAY_PREV_ALBUM, 0, 0, 0);
    return 0;
}

int
action_seek_5p_forward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        deadbeef->pl_lock ();
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            pos += dur * 0.05f;
            if (pos > dur) {
                pos = dur;
            }
            deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(pos * 1000), 0);
        }
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
    }
    return 0;
}

int
action_seek_5p_backward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        deadbeef->pl_lock ();
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            pos -= dur * 0.05f;
            if (pos < 0) {
                pos = 0;
            }
            deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(pos * 1000), 0);
        }
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
    }
    return 0;
}

int
action_seek_1p_forward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        deadbeef->pl_lock ();
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            pos += dur * 0.01f;
            if (pos > dur) {
                pos = dur;
            }
            deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(pos * 1000), 0);
        }
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
    }
    return 0;
}

int
action_seek_1p_backward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        deadbeef->pl_lock ();
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            pos -= dur * 0.01f;
            if (pos < 0) {
                pos = 0;
            }
            deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(pos * 1000), 0);
        }
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
    }
    return 0;
}

static int
seek_sec (float sec) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        deadbeef->pl_lock ();
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            pos += sec;
            if (pos > dur) {
                pos = dur;
            }
            if (pos < 0) {
                pos = 0;
            }
            deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(pos * 1000), 0);
        }
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
    }
    return 0;
}

int
action_seek_1s_forward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    return seek_sec (1.f);
}

int
action_seek_1s_backward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    return seek_sec (-1.f);
}

int
action_seek_5s_forward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    return seek_sec (5.f);
}

int
action_seek_5s_backward_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    return seek_sec (-5.f);
}

int
action_volume_up_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->volume_set_db (deadbeef->volume_get_db () + 1);
    return 0;
}

int
action_volume_down_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    deadbeef->volume_set_db (deadbeef->volume_get_db () - 1);
    return 0;
}

int
action_toggle_stop_after_current_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    int var = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    deadbeef->conf_set_int ("playlist.stop_after_current", !var);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    return 0;
}

int
action_toggle_stop_after_queue_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    int var = deadbeef->conf_get_int ("playlist.stop_after_queue", 0);
    deadbeef->conf_set_int ("playlist.stop_after_queue", !var);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    return 0;
}

int
action_toggle_stop_after_album_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    int var = deadbeef->conf_get_int ("playlist.stop_after_album", 0);
    deadbeef->conf_set_int ("playlist.stop_after_album", !var);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    return 0;
}

static DB_plugin_action_t action_prev_or_restart = { .title = "Playback/Previous or Restart Current Track",
        .name = "prev_or_restart",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_prev_or_restart_cb,
    .next = NULL };

static DB_plugin_action_t action_duplicate_playlist = { .title = "Duplicate Playlist",
        .name = "duplicate_playlist",
        .flags = DB_ACTION_PLAYLIST | DB_ACTION_ADD_MENU,
        .callback2 = action_duplicate_playlist_cb,
    .next = &action_prev_or_restart };

static DB_plugin_action_t action_reload_metadata = { .title = "Reload Metadata",
        .name = "reload_metadata",
        .flags = DB_ACTION_MULTIPLE_TRACKS,
        .callback2 = action_reload_metadata_handler,
    .next = &action_duplicate_playlist };

static DB_plugin_action_t action_jump_to_current = { .title = "Playback/Jump to Currently Playing Track",
        .name = "jump_to_current_track",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_jump_to_current_handler,
    .next = &action_reload_metadata };

static DB_plugin_action_t action_skip_to_prev_genre = { .title = "Playback/Skip to/Previous Genre",
        .name = "skip_to_prev_genre",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_prev_genre_handler,
    .next = &action_jump_to_current };

static DB_plugin_action_t action_skip_to_prev_composer = { .title = "Playback/Skip to/Previous Composer",
        .name = "skip_to_prev_composer",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_prev_composer_handler,
    .next = &action_skip_to_prev_genre };

static DB_plugin_action_t action_skip_to_prev_artist = { .title = "Playback/Skip to/Previous Artist",
        .name = "skip_to_prev_artist",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_prev_artist_handler,
    .next = &action_skip_to_prev_composer };

static DB_plugin_action_t action_skip_to_prev_album = { .title = "Playback/Skip to/Previous Album",
        .name = "skip_to_prev_album",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_prev_album_handler,
    .next = &action_skip_to_prev_artist };

static DB_plugin_action_t action_skip_to_next_genre = { .title = "Playback/Skip to/Next Genre",
        .name = "skip_to_next_genre",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_next_genre_handler,
    .next = &action_skip_to_prev_album };

static DB_plugin_action_t action_skip_to_next_composer = { .title = "Playback/Skip to/Next Composer",
        .name = "skip_to_next_composer",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_next_composer_handler,
    .next = &action_skip_to_next_genre };

static DB_plugin_action_t action_skip_to_next_artist = { .title = "Playback/Skip to/Next Artist",
        .name = "skip_to_next_artist",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_next_artist_handler,
    .next = &action_skip_to_next_composer };

static DB_plugin_action_t action_skip_to_next_album = { .title = "Playback/Skip to/Next Album",
        .name = "skip_to_next_album",
        .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
        .callback2 = action_skip_to_next_album_handler,
    .next = &action_skip_to_next_artist };

static DB_plugin_action_t action_next_playlist = { .title = "Next Playlist",
        .name = "next_playlist",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_next_playlist_handler,
    .next = &action_skip_to_next_album };

static DB_plugin_action_t action_prev_playlist = { .title = "Previous Playlist",
        .name = "prev_playlist",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_prev_playlist_handler,
    .next = &action_next_playlist };

static DB_plugin_action_t action_playlist10 = { .title = "Switch to Playlist 10",
        .name = "playlist10",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist10_handler,
    .next = &action_prev_playlist };

static DB_plugin_action_t action_playlist9 = { .title = "Switch to Playlist 9",
        .name = "playlist9",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist9_handler,
    .next = &action_playlist10 };

static DB_plugin_action_t action_playlist8 = { .title = "Switch to Playlist 8",
        .name = "playlist8",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist8_handler,
    .next = &action_playlist9 };

static DB_plugin_action_t action_playlist7 = { .title = "Switch to Playlist 7",
        .name = "playlist7",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist7_handler,
    .next = &action_playlist8 };

static DB_plugin_action_t action_playlist6 = { .title = "Switch to Playlist 6",
        .name = "playlist6",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist6_handler,
    .next = &action_playlist7 };

static DB_plugin_action_t action_playlist5 = { .title = "Switch to Playlist 5",
        .name = "playlist5",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist5_handler,
    .next = &action_playlist6 };

static DB_plugin_action_t action_playlist4 = { .title = "Switch to Playlist 4",
        .name = "playlist4",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist4_handler,
    .next = &action_playlist5 };

static DB_plugin_action_t action_playlist3 = { .title = "Switch to Playlist 3",
        .name = "playlist3",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist3_handler,
    .next = &action_playlist4 };

static DB_plugin_action_t action_playlist2 = { .title = "Switch to Playlist 2",
        .name = "playlist2",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist2_handler,
    .next = &action_playlist3 };

static DB_plugin_action_t action_playlist1 = { .title = "Switch to Playlist 1",
        .name = "playlist1",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_playlist1_handler,
    .next = &action_playlist2 };

static DB_plugin_action_t action_sort_randomize = { .title = "Edit/Sort Randomize",
        .name = "sort_randomize",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_sort_randomize_handler,
    .next = &action_playlist1 };

static DB_plugin_action_t action_sort_by_date = { .title = "Edit/Sort by Date",
        .name = "sort_date",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_sort_by_date_handler,
    .next = &action_sort_randomize };

static DB_plugin_action_t action_sort_by_artist = { .title = "Edit/Sort by Artist",
        .name = "sort_artist",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_sort_by_artist_handler,
    .next = &action_sort_by_date };

static DB_plugin_action_t action_sort_by_album = { .title = "Edit/Sort by Album",
        .name = "sort_album",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_sort_by_album_handler,
    .next = &action_sort_by_artist };

static DB_plugin_action_t action_sort_by_tracknr = { .title = "Edit/Sort by Track Number",
        .name = "sort_tracknr",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_sort_by_tracknr_handler,
    .next = &action_sort_by_album };

static DB_plugin_action_t action_sort_by_title = { .title = "Edit/Sort by Title",
        .name = "sort_title",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_sort_by_title_handler,
    .next = &action_sort_by_tracknr };

static DB_plugin_action_t action_invert_selection = { .title = "Edit/Invert Selection",
        .name = "invert_selection",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_invert_selection_handler,
    .next = &action_sort_by_title };

static DB_plugin_action_t action_clear_playlist = { .title = "Edit/Clear Playlist",
        .name = "clear_playlist",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_clear_playlist_handler,
    .next = &action_invert_selection };

static DB_plugin_action_t action_remove_from_playqueue = { .title = "Playback/Remove from Playback Queue",
        .name = "remove_from_playback_queue",
        .flags = DB_ACTION_MULTIPLE_TRACKS,
        .callback2 = action_remove_from_playqueue_handler,
    .next = &action_clear_playlist };

static DB_plugin_action_t action_add_to_playqueue = { .title = "Playback/Add to Playback Queue",
        .name = "add_to_playback_queue",
        .flags = DB_ACTION_MULTIPLE_TRACKS,
        .callback2 = action_add_to_playqueue_handler,
    .next = &action_remove_from_playqueue };

static DB_plugin_action_t action_play_next = { .title = "Playback/Add to Front of Playback Queue",
        .name = "add_to_front_of_playback_queue",
        .flags = DB_ACTION_MULTIPLE_TRACKS,
        .callback2 = action_prepend_to_playqueue_handler,
    .next = &action_add_to_playqueue };

static DB_plugin_action_t action_toggle_in_playqueue = { .title = "Playback/Toggle in Playback Queue",
        .name = "toggle_in_playback_queue",
        .flags = DB_ACTION_MULTIPLE_TRACKS |
    DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST,
        .callback2 = action_toggle_in_playqueue_handler,
    .next = &action_play_next };

static DB_plugin_action_t action_move_tracks_down = { .title = "Move/Move Tracks Down",
        .name = "move_tracks_down",
        .flags = DB_ACTION_MULTIPLE_TRACKS |
    DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST,
        .callback2 = action_move_tracks_down_handler,
    .next = &action_toggle_in_playqueue };

static DB_plugin_action_t action_move_tracks_up = { .title = "Move/Move Tracks Up",
        .name = "move_tracks_up",
        .flags =
    DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST,
        .callback2 = action_move_tracks_up_handler,
    .next = &action_move_tracks_down };

static DB_plugin_action_t action_toggle_mute = { .title = "Playback/Toggle Mute",
        .name = "toggle_mute",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_toggle_mute_handler,
    .next = &action_move_tracks_up };

static DB_plugin_action_t action_play = { .title = "Playback/Play",
        .name = "play",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_play_cb,
    .next = &action_toggle_mute };

static DB_plugin_action_t action_stop = { .title = "Playback/Stop",
        .name = "stop",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_stop_cb,
    .next = &action_play };

static DB_plugin_action_t action_prev = { .title = "Playback/Previous",
        .name = "prev",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_prev_cb,
    .next = &action_stop };

static DB_plugin_action_t action_next = { .title = "Playback/Next",
        .name = "next",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_next_cb,
    .next = &action_prev };

static DB_plugin_action_t action_toggle_pause = { .title = "Playback/Toggle Pause",
        .name = "toggle_pause",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_toggle_pause_cb,
    .next = &action_next };

static DB_plugin_action_t action_play_pause = { .title = "Playback/Play\\/Pause",
        .name = "play_pause",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_play_pause_cb,
    .next = &action_toggle_pause };

static DB_plugin_action_t action_play_next_album = { .title = "Playback/Play Next Album",
        .name = "playback_next_album",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_play_next_album_cb,
    .next = &action_play_pause };

static DB_plugin_action_t action_play_prev_album = { .title = "Playback/Play Previous Album",
        .name = "playback_prev_album",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_play_prev_album_cb,
    .next = &action_play_next_album };

static DB_plugin_action_t action_play_random_album = { .title = "Playback/Play Random Album",
        .name = "playback_random_album",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_play_random_album_cb,
    .next = &action_play_prev_album };

static DB_plugin_action_t action_play_random = { .title = "Playback/Play Random",
        .name = "playback_random",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_play_random_cb,
    .next = &action_play_random_album };

static DB_plugin_action_t action_seek_1s_forward = { .title = "Playback/Seek 1s Forward",
        .name = "seek_1s_fwd",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_1s_forward_cb,
    .next = &action_play_random };

static DB_plugin_action_t action_seek_1s_backward = { .title = "Playback/Seek 1s Backward",
        .name = "seek_1s_back",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_1s_backward_cb,
    .next = &action_seek_1s_forward };

static DB_plugin_action_t action_seek_5s_forward = { .title = "Playback/Seek 5s Forward",
        .name = "seek_5s_fwd",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_5s_forward_cb,
    .next = &action_seek_1s_backward };

static DB_plugin_action_t action_seek_5s_backward = { .title = "Playback/Seek 5s Backward",
        .name = "seek_5s_back",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_5s_backward_cb,
    .next = &action_seek_5s_forward };

static DB_plugin_action_t action_seek_1p_forward = { .title = "Playback/Seek 1% Forward",
        .name = "seek_1p_fwd",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_1p_forward_cb,
    .next = &action_seek_5s_backward };

static DB_plugin_action_t action_seek_1p_backward = { .title = "Playback/Seek 1% Backward",
        .name = "seek_1p_back",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_1p_backward_cb,
    .next = &action_seek_1p_forward };

static DB_plugin_action_t action_seek_5p_forward = { .title = "Playback/Seek 5% Forward",
        .name = "seek_5p_fwd",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_5p_forward_cb,
    .next = &action_seek_1p_backward };

static DB_plugin_action_t action_seek_5p_backward = { .title = "Playback/Seek 5% Backward",
        .name = "seek_5p_back",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_seek_5p_backward_cb,
    .next = &action_seek_5p_forward };

static DB_plugin_action_t action_volume_up = { .title = "Playback/Volume Up",
        .name = "volume_up",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_volume_up_cb,
    .next = &action_seek_5p_backward };

static DB_plugin_action_t action_volume_down = { .title = "Playback/Volume Down",
        .name = "volume_down",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_volume_down_cb,
    .next = &action_volume_up };

static DB_plugin_action_t action_toggle_stop_after_current = { .title = "Playback/Toggle Stop After Current Track",
        .name = "toggle_stop_after_current",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_toggle_stop_after_current_cb,
    .next = &action_volume_down };

static DB_plugin_action_t action_toggle_stop_after_queue = { .title = "Playback/Toggle Stop After Queue",
        .name = "toggle_stop_after_queue",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_toggle_stop_after_queue_cb,
    .next = &action_toggle_stop_after_current };

static DB_plugin_action_t action_toggle_stop_after_album = { .title = "Playback/Toggle Stop After Current Album",
        .name = "toggle_stop_after_album",
        .flags = DB_ACTION_COMMON,
        .callback2 = action_toggle_stop_after_album_cb,
    .next = &action_toggle_stop_after_queue };

static DB_plugin_action_t *
hotkeys_get_actions (DB_playItem_t *it) {
    return &action_toggle_stop_after_album;
}

static DB_misc_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.id = "deadbeef",
    .plugin.name = "DeaDBeeF",
    .plugin.descr =
    "DeaDBeeF system component",
    .plugin.copyright =
        "DeaDBeeF -- the music player\n"
        "Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.get_actions = hotkeys_get_actions,
};

DB_plugin_t *
core_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

