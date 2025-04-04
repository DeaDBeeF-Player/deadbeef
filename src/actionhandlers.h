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

#ifndef __HOTKEYS_ACTIONHANDLERS_H
#define __HOTKEYS_ACTIONHANDLERS_H

int
action_jump_to_current_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_next_album_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_next_artist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_next_composer_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_next_genre_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_prev_album_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_prev_artist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_prev_composer_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_skip_to_prev_genre_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_reload_metadata_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_next_playlist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_prev_playlist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist1_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist2_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist3_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist4_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist5_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist6_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist7_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist8_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist9_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_playlist10_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_sort_randomize_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_sort_by_date_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_sort_by_artist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_sort_by_album_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_sort_by_tracknr_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_sort_by_title_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_invert_selection_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_clear_playlist_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_toggle_in_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_move_tracks_up_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_move_tracks_down_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_add_to_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_prepend_to_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_remove_from_playqueue_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_toggle_mute_handler (DB_plugin_action_t *act, ddb_action_context_t ctx);

int
action_prev_or_restart_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx);

int
action_duplicate_playlist_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx);

#endif
