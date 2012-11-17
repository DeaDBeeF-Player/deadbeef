/*
    gtkui hotkey handlers
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

#ifndef __ACTIONHANDLERS_H
#define __ACTIONHANDLERS_H

gboolean
action_open_files_handler_cb (void *userdata);

int
action_open_files_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_quit_handler_cb (void *user_data);

int
action_quit_handler (DB_plugin_action_t *act, int ctx);

gboolean
action_add_files_handler_cb (void *user_data);

int
action_add_files_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_add_folders_handler_cb (void *user_data);

int
action_add_folders_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_deselect_all_handler_cb (void *user_data);

int
action_deselect_all_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_select_all_handler_cb (void *user_data);

int
action_select_all_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_new_playlist_handler_cb (void *user_data);

int
action_new_playlist_handler (struct DB_plugin_action_s *action, int ctx);

int
action_remove_current_playlist_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_toggle_mainwin_handler_cb (void *user_data);

int
action_toggle_mainwin_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_show_mainwin_handler_cb (void *user_data);

int
action_show_mainwin_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_hide_mainwin_handler_cb (void *user_data);

int
action_hide_mainwin_handler (struct DB_plugin_action_s *action, int ctx);

gboolean
action_add_location_handler_cb (void *user_data);

int
action_add_location_handler (DB_plugin_action_t *act, int ctx);

gboolean
action_show_help_handler_cb (void *user_data);

int
action_show_help_handler (DB_plugin_action_t *act, int ctx);

int
action_remove_from_playlist_handler (DB_plugin_action_t *act, int ctx);

#endif
