/*
    Hotkeys plugin for DeaDBeeF
    Copyright (C) 2009 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <ctype.h>
#include "parser.h"

#include "hotkeys.h"
#include "../../deadbeef.h"
#include "actionhandlers.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_hotkeys_plugin_t plugin;
DB_functions_t *deadbeef;
static int finished;
static Display *disp;
static intptr_t loop_tid;
static int need_reset = 0;

#define MAX_COMMAND_COUNT 256

typedef struct {
    const char *name;
    KeySym keysym;
    int keycode; // after mapping
} xkey_t;

#define KEY(kname, kcode) { .name=kname, .keysym=kcode},

static xkey_t keys[] = {
    #include "keysyms.inc"
};

typedef struct command_s {
    int keycode;
    int modifier;
    int ctx;
    int isglobal;
    int is_14_action; // means action is coming from plugin using API 1.4 or less
    DB_plugin_action_t *action;
} command_t;

static command_t commands [MAX_COMMAND_COUNT];
static int command_count = 0;

static void
init_mapped_keycodes (Display *disp, KeySym *syms, int first_kk, int last_kk, int ks_per_kk) {
    int i, ks;
    for (i = 0; i < last_kk-first_kk; i++)
    {
        KeySym sym = * (syms + i*ks_per_kk);
        for (ks = 0; keys[ks].name; ks++)
        {
            if (keys[ ks ].keysym == sym)
            {
                keys[ks].keycode = i+first_kk;
            }
        }
    }
}

static int
get_keycode (Display *disp, const char* name, KeySym *syms, int first_kk, int last_kk, int ks_per_kk) {
    trace ("get_keycode %s\n", name);
    int i, ks;

    for (i = 0; i < last_kk-first_kk; i++)
    {
        KeySym sym = * (syms + i*ks_per_kk);
        for (ks = 0; keys[ks].name; ks++)
        {
            if ( (keys[ ks ].keysym == sym) && (0 == strcmp (name, keys[ ks ].name)))
            {
                return i+first_kk;
            }
        }
    }
    return 0;
}

static char*
trim (char* s)
{
    char *h, *t;
    
    for (h = s; *h == ' ' || *h == '\t'; h++);
    for (t = s + strlen (s); *t == ' ' || *t == '\t'; t--);
    * (t+1) = 0;
    return h;
}

typedef int (*action_callback_14_t)(struct DB_plugin_action_s *action, void *userdata);

static void
cmd_invoke_plugin_command (DB_plugin_action_t *action, int ctx, int is_14_action)
{
    if (is_14_action) {
        if (ctx == DDB_ACTION_CTX_MAIN) {
            // collect stuff for 1.4 user data

            // common action
            if (action->flags & DB_ACTION_COMMON)
            {
                ((action_callback_14_t)action->callback) (action, NULL);
                return;
            }

            // playlist action
            if (action->flags & DB_ACTION_PLAYLIST__DEPRECATED)
            {
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt) {
                    ((action_callback_14_t)action->callback) (action, plt);
                    deadbeef->plt_unref (plt);
                }
                return;
            }

            int selected_count = 0;
            DB_playItem_t *pit = deadbeef->pl_get_first (PL_MAIN);
            DB_playItem_t *selected = NULL;
            while (pit) {
                if (deadbeef->pl_is_selected (pit))
                {
                    if (!selected)
                        selected = pit;
                    selected_count++;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (pit, PL_MAIN);
                deadbeef->pl_item_unref (pit);
                pit = next;
            }

            //Now we're checking if action is applicable:

            if (selected_count == 0)
            {
                trace ("No tracks selected\n");
                return;
            }
            if ((selected_count == 1) && (!(action->flags & DB_ACTION_SINGLE_TRACK)))
            {
                trace ("Hotkeys: action %s not allowed for single track\n", action->name);
                return;
            }
            if ((selected_count > 1) && (!(action->flags & DB_ACTION_MULTIPLE_TRACKS)))
            {
                trace ("Hotkeys: action %s not allowed for multiple tracks\n", action->name);
                return;
            }

            //So, action is allowed, do it.

            if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS__DEPRECATED)
            {
                ((action_callback_14_t)action->callback) (action, NULL);
            }
            else {
                pit = deadbeef->pl_get_first (PL_MAIN);
                while (pit) {
                    if (deadbeef->pl_is_selected (pit))
                    {
                        ((action_callback_14_t)action->callback) (action, pit);
                    }
                    DB_playItem_t *next = deadbeef->pl_get_next (pit, PL_MAIN);
                    deadbeef->pl_item_unref (pit);
                    pit = next;
                }
            }
        }
    }
    else {
        action->callback (action, ctx);
    }
}

static DB_plugin_action_t *
find_action_by_name (const char *command, int *is_14_action) {
    // find action with this name, and add to list
    *is_14_action = 0;
    DB_plugin_action_t *actions = NULL;
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->get_actions) {
            actions = p->get_actions (NULL);
            while (actions) {
                if (actions->name && actions->title && !strcasecmp (actions->name, command)) {
                    if (p->api_vminor < 5) {
                        *is_14_action = 1;
                    }
                    break; // found
                }
                actions = actions->next;
            }
            if (actions) {
                break;
            }
        }
    }
    return actions;
}

static int
read_config (Display *disp)
{
    int ks_per_kk;
    int first_kk, last_kk;
    KeySym* syms;

    XDisplayKeycodes (disp, &first_kk, &last_kk);
    syms = XGetKeyboardMapping (disp, first_kk, last_kk - first_kk, &ks_per_kk);

    DB_conf_item_t *item = deadbeef->conf_find ("hotkey.", NULL);
    while (item) {
        if (command_count == MAX_COMMAND_COUNT)
        {
            fprintf (stderr, "hotkeys: maximum number (%d) of commands exceeded\n", MAX_COMMAND_COUNT);
            break;
        }

        command_t *cmd_entry = &commands[ command_count ];
        memset (cmd_entry, 0, sizeof (command_t));

        char token[MAX_TOKEN];
        char keycombo[MAX_TOKEN];
        const char *script = item->value;
        if ((script = gettoken (script, keycombo)) == 0) {
            trace ("hotkeys: unexpected eol (keycombo)\n");
            goto out;
        }
        if ((script = gettoken (script, token)) == 0) {
            trace ("hotkeys: unexpected eol (ctx)\n");
            goto out;
        }
        cmd_entry->ctx = atoi (token);
        if (cmd_entry->ctx < 0 || cmd_entry->ctx >= DDB_ACTION_CTX_COUNT) {
            trace ("hotkeys: invalid ctx %d\n");
            goto out;
        }
        if ((script = gettoken (script, token)) == 0) {
            trace ("hotkeys: unexpected eol (isglobal)\n");
            goto out;
        }
        cmd_entry->isglobal = atoi (token);
        if ((script = gettoken (script, token)) == 0) {
            trace ("hotkeys: unexpected eol (action)\n");
            goto out;
        }
        cmd_entry->action = find_action_by_name (token, &cmd_entry->is_14_action);
        if (!cmd_entry->action) {
            trace ("hotkeys: action not found %s\n", token);
            goto out;
        }

        // parse key combo
        int done = 0;
        char* p;
        char* space = keycombo;
        do {
            p = space;
            space = strchr (p, ' ');
            if (space) {
                *space = 0;
                space++;
            }
            else
                done = 1;

            if (0 == strcasecmp (p, "Ctrl"))
                cmd_entry->modifier |= ControlMask;

            else if (0 == strcasecmp (p, "Alt"))
                cmd_entry->modifier |= Mod1Mask;

            else if (0 == strcasecmp (p, "Shift"))
                cmd_entry->modifier |= ShiftMask;

            else if (0 == strcasecmp (p, "Super")) {
                cmd_entry->modifier |= Mod4Mask;
            }

            else {
                if (p[0] == '0' && p[1] == 'x') {
                    // parse hex keycode
                    int r = sscanf (p, "0x%x", &cmd_entry->keycode);
                    if (!r) {
                        cmd_entry->keycode = 0;
                    }
                }
                else {
                    // lookup name table
                    cmd_entry->keycode = get_keycode (disp, p, syms, first_kk, last_kk, ks_per_kk);
                }
                if (!cmd_entry->keycode)
                {
                    trace ("hotkeys: got 0 from get_keycode while adding hotkey: %s %s\n", item->key, item->value);
                    break;
                }
            }
        } while (!done);

        if (done) {
            if (cmd_entry->keycode == 0) {
                trace ("hotkeys: Key not found while parsing %s %s\n", item->key, item->value);
            }
            else {
                command_count++;
            }
        }
out:
        item = deadbeef->conf_find ("hotkey.", item);
    }
    XFree (syms);
    int i;
    // need to grab it here to prevent gdk_x_error from being called while we're
    // doing it on other thread
    for (i = 0; i < command_count; i++) {
        if (!commands[i].isglobal) {
            continue;
        }
        for (int f = 0; f < 16; f++) {
            uint32_t flags = 0;
            if (f & 1) {
                flags |= LockMask;
            }
            if (f & 2) {
                flags |= Mod2Mask;
            }
            if (f & 4) {
                flags |= Mod3Mask;
            }
            if (f & 8) {
                flags |= Mod5Mask;
            }
            trace ("XGrabKey %d %x\n", commands[i].keycode, commands[i].modifier | flags);
            XGrabKey (disp, commands[i].keycode, commands[i].modifier | flags, DefaultRootWindow (disp), False, GrabModeAsync, GrabModeAsync);
        }
    }

    return 0;
}

DB_plugin_t *
hotkeys_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static void
cleanup () {
    command_count = 0;
    XCloseDisplay (disp);
}

static int
x_err_handler (Display *d, XErrorEvent *evt) {
#if 0
    // this code crashes if gtk plugin is active
    char buffer[1024];
    XGetErrorText (d, evt->error_code, buffer, sizeof (buffer));
    trace ("hotkeys: xlib error: %s\n", buffer);
#endif
    return 0;
}

static void
hotkeys_event_loop (void *unused) {
    int i;

    while (!finished) {
        if (need_reset) {
            trace ("hotkeys: reinitializing\n");
            XSetErrorHandler (x_err_handler);
            for (int i = 0; i < command_count; i++) {
                for (int f = 0; f < 16; f++) {
                    uint32_t flags = 0;
                    if (f & 1) {
                        flags |= LockMask;
                    }
                    if (f & 2) {
                        flags |= Mod2Mask;
                    }
                    if (f & 4) {
                        flags |= Mod3Mask;
                    }
                    if (f & 8) {
                        flags |= Mod5Mask;
                    }
                    XUngrabKey (disp, commands[i].keycode, commands[i].modifier | flags, DefaultRootWindow (disp));
                }
            }
            memset (commands, 0, sizeof (commands));
            command_count = 0;
            read_config (disp);
            need_reset = 0;
        }

        XEvent event;
        while (XPending (disp))
        {
            XNextEvent (disp, &event);

            if (event.xkey.type == KeyPress)
            {
                int state = event.xkey.state;
                trace ("hotkeys: keypress, state=%X\n", state);
                // ignore caps/scroll/numlock
                state &= (ShiftMask|ControlMask|Mod1Mask|Mod4Mask);
                trace ("filtered state=%X\n", state);
                for (i = 0; i < command_count; i++) {
                    if ( (event.xkey.keycode == commands[ i ].keycode) &&
                         (state == commands[ i ].modifier))
                    {
                        trace ("matches to commands[%d]!\n", i);
                        cmd_invoke_plugin_command (commands[i].action, commands[i].ctx, commands->is_14_action);
                        break;
                    }
                }
                if (i == command_count) {
                    trace ("keypress doesn't match to any global hotkey\n");
                }
            }
        }
        usleep (200 * 1000);
    }
}

static int
hotkeys_connect (void) {
    finished = 0;
    loop_tid = 0;
    disp = XOpenDisplay (NULL);
    if (!disp)
    {
        fprintf (stderr, "hotkeys: could not open display\n");
        return -1;
    }
    XSetErrorHandler (x_err_handler);

    read_config (disp);

    int ks_per_kk;
    int first_kk, last_kk;
    KeySym* syms;
    XDisplayKeycodes (disp, &first_kk, &last_kk);
    syms = XGetKeyboardMapping (disp, first_kk, last_kk - first_kk, &ks_per_kk);
    init_mapped_keycodes (disp, syms, first_kk, last_kk, ks_per_kk);
    XFree (syms);
    XSync (disp, 0);
    loop_tid = deadbeef->thread_start (hotkeys_event_loop, 0);
    return 0;
}

static int
hotkeys_disconnect (void) {
    if (loop_tid) {
        finished = 1;
        deadbeef->thread_join (loop_tid);
        cleanup ();
    }

    return 0;
}

const char *
hotkeys_get_name_for_keycode (int keycode) {
    for (int i = 0; keys[i].name; i++) {
        if (keycode == keys[i].keysym) {
            return keys[i].name;
        }
    }
    return NULL;
}


DB_plugin_action_t*
hotkeys_get_action_for_keycombo (int key, int mods, int isglobal, int *ctx) {
    int i;
    // find mapped keycode

    if (isupper (key)) {
        key = tolower (key);
    }

    int keycode = 0;
    for (i = 0; keys[i].name; i++) {
        if (key == keys[i].keysym) {
            keycode = keys[i].keycode;
            break;
        }
    }
    if (!keys[i].name) {
        trace ("hotkeys: unknown keysym 0x%X\n", key);
        return NULL;
    }

    trace ("hotkeys: keysym 0x%X mapped to 0x%X\n", key, keycode);


    for (i = 0; i < command_count; i++) {
        trace ("hotkeys: command %s keycode %x mods %x\n", commands[i].action->name, commands[i].keycode, commands[i].modifier);
        if (commands[i].keycode == keycode && commands[i].modifier == mods && commands[i].isglobal == isglobal) {
            *ctx = commands[i].ctx;
            return commands[i].action;
        }
    }
    return NULL;
}

void
hotkeys_reset (void) {
    need_reset = 1;
    trace ("hotkeys: reset flagged\n");
}

int
action_play_cb (struct DB_plugin_action_s *action, int ctx) {
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == OUTPUT_STATE_PAUSED) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cur != -1) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cur, PL_MAIN);
            ddb_playItem_t *it_playing = deadbeef->streamer_get_playing_track ();
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
        deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    }
    return 0;
}

int
action_prev_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
    return 0;
}

int
action_next_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
    return 0;
}

int
action_stop_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
    return 0;
}

int
action_toggle_pause_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
    return 0;
}

int
action_play_pause_cb (struct DB_plugin_action_s *action, int ctx) {
    int state = deadbeef->get_output ()->state ();
    if (state == OUTPUT_STATE_PLAYING) {
        deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
    }
    else {
        deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    }
    return 0;
}

int
action_play_random_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->sendmessage (DB_EV_PLAY_RANDOM, 0, 0, 0);
    return 0;
}

int
action_seek_5p_forward_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            deadbeef->sendmessage (DB_EV_SEEK, 0, (pos + dur * 0.05f) * 1000, 0);
        }
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
    return 0;
}

int
action_seek_5p_backward_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            deadbeef->sendmessage (DB_EV_SEEK, 0, (pos - dur * 0.05f) * 1000, 0);
        }
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
    return 0;
}

int
action_seek_1p_forward_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            deadbeef->sendmessage (DB_EV_SEEK, 0, (pos + dur * 0.01f) * 1000, 0);
        }
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
    return 0;
}

int
action_seek_1p_backward_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        float dur = deadbeef->pl_get_item_duration (it);
        if (dur > 0) {
            float pos = deadbeef->streamer_get_playpos ();
            deadbeef->sendmessage (DB_EV_SEEK, 0, (pos - dur * 0.01f) * 1000, 0);
        }
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
    return 0;
}

int
action_volume_up_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->volume_set_db (deadbeef->volume_get_db () + 2);
    return 0;
}

int
action_volume_down_cb (struct DB_plugin_action_s *action, int ctx) {
    deadbeef->volume_set_db (deadbeef->volume_get_db () - 2);
    return 0;
}

int
action_toggle_stop_after_current_cb (struct DB_plugin_action_s *action, int ctx) {
    int var = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    var = 1 - var;
    deadbeef->conf_set_int ("playlist.stop_after_current", var);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    return 0;
}

static DB_plugin_action_t action_reload_metadata = {
    .title = "Reload metadata",
    .name = "reload_metadata",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
    .callback = action_reload_metadata_handler,
    .next = NULL
};

static DB_plugin_action_t action_jump_to_current = {
    .title = "Playback/Jump to currently playing track",
    .name = "jump_to_current_track",
    .flags = DB_ACTION_COMMON,
    .callback = action_jump_to_current_handler,
    .next = &action_reload_metadata
};

static DB_plugin_action_t action_next_playlist = {
    .title = "Edit/[stub] Next playlist",
    .name = "sort_next_playlist",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_jump_to_current
};

static DB_plugin_action_t action_prev_playlist = {
    .title = "Edit/[stub] Prev playlist",
    .name = "sort_prev_playlist",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_next_playlist
};

static DB_plugin_action_t action_sort_randomize = {
    .title = "Edit/[stub] Sort Randomize",
    .name = "sort_randomize",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_prev_playlist
};

static DB_plugin_action_t action_sort_by_date = {
    .title = "Edit/[stub] Sort by date",
    .name = "sort_date",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_sort_randomize
};

static DB_plugin_action_t action_sort_by_artist = {
    .title = "Edit/[stub] Sort by artist",
    .name = "sort_artist",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_sort_by_date
};


static DB_plugin_action_t action_sort_by_album = {
    .title = "Edit/[stub] Sort by album",
    .name = "sort_album",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_sort_by_artist
};

static DB_plugin_action_t action_sort_by_tracknr = {
    .title = "Edit/[stub] Sort by track number",
    .name = "sort_tracknr",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_sort_by_album
};

static DB_plugin_action_t action_sort_by_title = {
    .title = "Edit/[stub] Sort by title",
    .name = "sort_title",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_sort_by_tracknr
};

static DB_plugin_action_t action_invert_selection = {
    .title = "Edit/[stub] Invert Selection",
    .name = "invert_selection",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_sort_by_tracknr
};

static DB_plugin_action_t action_clear_playlist = {
    .title = "Edit/[stub] Clear playlist",
    .name = "clear_playlist",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_invert_selection
};

static DB_plugin_action_t action_remove_from_playqueue = {
    .title = "Playback/[stub] Add To Playback Queue",
    .name = "remove_from_playback_queue",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback = NULL,
    .next = &action_clear_playlist
};

static DB_plugin_action_t action_add_to_playqueue = {
    .title = "Playback/[stub] Add To Playback Queue",
    .name = "add_to_playback_queue",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback = NULL,
    .next = &action_remove_from_playqueue
};

static DB_plugin_action_t action_toggle_mute = {
    .title = "Playback/[stub] Toggle Mute",
    .name = "toggle_mute",
    .flags = DB_ACTION_COMMON,
    .callback = NULL,
    .next = &action_add_to_playqueue
};

static DB_plugin_action_t action_play = {
    .title = "Playback/Play",
    .name = "play",
    .flags = DB_ACTION_COMMON,
    .callback = action_play_cb,
    .next = &action_toggle_mute
};

static DB_plugin_action_t action_stop = {
    .title = "Playback/Stop",
    .name = "stop",
    .flags = DB_ACTION_COMMON,
    .callback = action_stop_cb,
    .next = &action_play
};

static DB_plugin_action_t action_prev = {
    .title = "Playback/Previous",
    .name = "prev",
    .flags = DB_ACTION_COMMON,
    .callback = action_prev_cb,
    .next = &action_stop
};

static DB_plugin_action_t action_next = {
    .title = "Playback/Next",
    .name = "next",
    .flags = DB_ACTION_COMMON,
    .callback = action_next_cb,
    .next = &action_prev
};

static DB_plugin_action_t action_toggle_pause = {
    .title = "Playback/Toggle Pause",
    .name = "toggle_pause",
    .flags = DB_ACTION_COMMON,
    .callback = action_toggle_pause_cb,
    .next = &action_next
};

static DB_plugin_action_t action_play_pause = {
    .title = "Playback/Play\\/Pause",
    .name = "play_pause",
    .flags = DB_ACTION_COMMON,
    .callback = action_play_pause_cb,
    .next = &action_toggle_pause
};

static DB_plugin_action_t action_play_random = {
    .title = "Playback/Play Random",
    .name = "playback_random",
    .flags = DB_ACTION_COMMON,
    .callback = action_play_random_cb,
    .next = &action_play_pause
};

static DB_plugin_action_t action_seek_1p_forward = {
    .title = "Playback/Seek 1% forward",
    .name = "seek_1p_fwd",
    .flags = DB_ACTION_COMMON,
    .callback = action_seek_1p_forward_cb,
    .next = &action_play_random
};

static DB_plugin_action_t action_seek_1p_backward = {
    .title = "Playback/Seek 1% backward",
    .name = "seek_1p_back",
    .flags = DB_ACTION_COMMON,
    .callback = action_seek_1p_backward_cb,
    .next = &action_seek_1p_forward
};

static DB_plugin_action_t action_seek_5p_forward = {
    .title = "Playback/Seek 5% forward",
    .name = "seek_5p_fwd",
    .flags = DB_ACTION_COMMON,
    .callback = action_seek_5p_forward_cb,
    .next = &action_seek_1p_backward
};

static DB_plugin_action_t action_seek_5p_backward = {
    .title = "Playback/Seek 5% backward",
    .name = "seek_5p_back",
    .flags = DB_ACTION_COMMON,
    .callback = action_seek_5p_backward_cb,
    .next = &action_seek_5p_forward
};

static DB_plugin_action_t action_volume_up = {
    .title = "Playback/Volume Up",
    .name = "volume_up",
    .flags = DB_ACTION_COMMON,
    .callback = action_volume_up_cb,
    .next = &action_seek_5p_backward
};

static DB_plugin_action_t action_volume_down = {
    .title = "Playback/Volume Down",
    .name = "volume_down",
    .flags = DB_ACTION_COMMON,
    .callback = action_volume_down_cb,
    .next = &action_volume_up
};

static DB_plugin_action_t action_toggle_stop_after_current = {
    .title = "Playback/Toggle Stop After Current",
    .name = "toggle_stop_after_current",
    .flags = DB_ACTION_COMMON,
    .callback = action_toggle_stop_after_current_cb,
    .next = &action_volume_down
};

static DB_plugin_action_t *
hotkeys_get_actions (DB_playItem_t *it)
{
    return &action_toggle_stop_after_current;
}

// define plugin interface
static DB_hotkeys_plugin_t plugin = {
    .misc.plugin.api_vmajor = 1,
    .misc.plugin.api_vminor = 5,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 1,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.id = "hotkeys",
    .misc.plugin.name = "Hotkey manager",
    .misc.plugin.descr =
        "Manages local and global hotkeys, and executes actions when the assigned key combinations are pressed\n\n"
        "This plugin has its own API, to allow 3rd party GUI plugins to reuse the code.\n"
        "Check the plugins/hotkeys/hotkeys.h in the source tree if you need this.\n\n"
        "Changes in version 1.1\n"
        "    * adaptation to new deadbeef 0.6 plugin API\n"
        "    * added local hotkeys support\n"
    ,
    .misc.plugin.copyright = 
        "Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .misc.plugin.website = "http://deadbeef.sf.net",
    .misc.plugin.get_actions = hotkeys_get_actions,
    .misc.plugin.start = hotkeys_connect,
    .misc.plugin.stop = hotkeys_disconnect,
    .get_name_for_keycode = hotkeys_get_name_for_keycode,
    .get_action_for_keycombo = hotkeys_get_action_for_keycombo,
    .reset = hotkeys_reset,
};

