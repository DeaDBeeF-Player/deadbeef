/*
    Hotkeys plugin for DeaDBeeF
    Copyright (C) 2009 Viktor Semykin <thesame.ml@gmail.com>

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

#include "hotkeys.h"
#include "../../deadbeef.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_hotkeys_plugin_t plugin;
static DB_functions_t *deadbeef;
static int finished;
static Display *disp;
static intptr_t loop_tid;
static int need_reset = 0;

#define MAX_COMMAND_COUNT 256

typedef struct {
    const char *name;
    KeySym keysym;
} xkey_t;

#define KEY(kname, kcode) { .name=kname, .keysym=kcode },

static const xkey_t keys[] = {
    #include "keysyms.inc"
};

typedef struct command_s {
    int keycode;
    int modifier;
    DB_plugin_action_t *action;
} command_t;

static command_t commands [MAX_COMMAND_COUNT];
static int command_count = 0;

static int
get_keycode (Display *disp, const char* name, KeySym *syms, int first_kk, int last_kk, int ks_per_kk) {
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

/*
    FIXME: This function has many common code with plcommon.c
    and it does full traverse of playlist twice
*/
static void
cmd_invoke_plugin_command (DB_plugin_action_t *action)
{
    trace ("We're here to invoke action %s / %s\n", action->title, action->name);

    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;

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


    if (action->flags & DB_ACTION_COMMON)
    {
        //Simply call common action
        action->callback (action, NULL);
        return;
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
    if ((selected_count > 1) && (!(action->flags & DB_ACTION_ALLOW_MULTIPLE_TRACKS)))
    {
        trace ("Hotkeys: action %s not allowed for multiple tracks\n", action->name);
        return;
    }

    //So, action is allowed, do it.

    if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS)
    {
        action->callback (action, NULL);
        return;
    }

    pit = deadbeef->pl_get_first (PL_MAIN);
    while (pit) {
        if (deadbeef->pl_is_selected (pit))
        {
            action->callback (action, pit);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (pit, PL_MAIN);
        deadbeef->pl_item_unref (pit);
        pit = next;
    }
}

static DB_plugin_action_t *
get_action (const char* command)
{
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->get_actions) {
            DB_plugin_action_t *actions = p->get_actions (NULL);
            while (actions) {
                if (actions->name && !strcasecmp (command, actions->name)) {
                    return actions;
                }
                actions = actions->next;
            }
        }
    }

    return NULL;
}

static int
read_config (Display *disp)
{
    int ks_per_kk;
    int first_kk, last_kk;
    KeySym* syms;

    XDisplayKeycodes (disp, &first_kk, &last_kk);
    syms = XGetKeyboardMapping (disp, first_kk, last_kk - first_kk, &ks_per_kk);

    DB_conf_item_t *item = deadbeef->conf_find ("hotkeys.", NULL);
    while (item) {
        if (command_count == MAX_COMMAND_COUNT)
        {
            fprintf (stderr, "hotkeys: maximum number (%d) of commands exceeded\n", MAX_COMMAND_COUNT);
            break;
        }

        command_t *cmd_entry = &commands[ command_count ];
        cmd_entry->modifier = 0;
        cmd_entry->keycode = 0;

        size_t l = strlen (item->value);
        char param[l+1];
        memcpy (param, item->value, l+1);
        
        char* colon = strchr (param, ':');
        if (!colon)
        {
            fprintf (stderr, "hotkeys: bad config option %s %s\n", item->key, item->value);
            continue;
        }
        char* command = colon+1;
        *colon = 0;

        int done = 0;
        char* p;
        char* space = param - 1;
        do {
            p = space+1;
            space = strchr (p, ' ');
            if (space)
                *space = 0;
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
                command = trim (command);
                cmd_entry->action = get_action (command);
                if (!cmd_entry->action)
                {
                    trace ("hotkeys: Unknown command <%s> while parsing %s %s\n", command,  item->key, item->value);
                }
                else {
                    command_count++;
                }
            }
        }
        item = deadbeef->conf_find ("hotkeys.", item);
    }
    XFree (syms);
    int i;
    // need to grab it here to prevent gdk_x_error from being called while we're
    // doing it on other thread
    for (i = 0; i < command_count; i++) {
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
    char buffer[1024];
    XGetErrorText (d, evt->error_code, buffer, sizeof (buffer));
    trace ("hotkeys: xlib error: %s\n", buffer);

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
                        cmd_invoke_plugin_command (commands[i].action);
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

void
hotkeys_reset (void) {
    need_reset = 1;
    trace ("hotkeys: reset flagged\n");
}

int
action_play_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    return 0;
}

int
action_prev_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
    return 0;
}

int
action_next_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
    return 0;
}

int
action_stop_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
    return 0;
}

int
action_toggle_pause_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
    return 0;
}

int
action_play_pause_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
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
action_play_random_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->sendmessage (DB_EV_PLAY_RANDOM, 0, 0, 0);
    return 0;
}

int
action_seek_forward_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->playback_set_pos (deadbeef->playback_get_pos () + 5);
    return 0;
}

int
action_seek_backward_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->playback_set_pos (deadbeef->playback_get_pos () - 5);
    return 0;
}

int
action_volume_up_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->volume_set_db (deadbeef->volume_get_db () + 2);
    return 0;
}

int
action_volume_down_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    deadbeef->volume_set_db (deadbeef->volume_get_db () - 2);
    return 0;
}

int
action_toggle_stop_after_current_cb (struct DB_plugin_action_s *action, DB_playItem_t *it) {
    int var = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    var = 1 - var;
    deadbeef->conf_set_int ("playlist.stop_after_current", var);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    return 0;
}

static DB_plugin_action_t action_play = {
    .title = "Play",
    .name = "play",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK (action_play_cb),
    .next = NULL
};

static DB_plugin_action_t action_stop = {
    .title = "Stop",
    .name = "stop",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_stop_cb),
    .next = &action_play
};

static DB_plugin_action_t action_prev = {
    .title = "Previous",
    .name = "prev",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_prev_cb),
    .next = &action_stop
};

static DB_plugin_action_t action_next = {
    .title = "Next",
    .name = "next",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_next_cb),
    .next = &action_prev
};

static DB_plugin_action_t action_toggle_pause = {
    .title = "Toggle Pause",
    .name = "toggle_pause",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_toggle_pause_cb),
    .next = &action_next
};

static DB_plugin_action_t action_play_pause = {
    .title = "Play\\/Pause",
    .name = "play_pause",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_play_pause_cb),
    .next = &action_toggle_pause
};

static DB_plugin_action_t action_play_random = {
    .title = "Play Random",
    .name = "playback_random",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_play_random_cb),
    .next = &action_play_pause
};

static DB_plugin_action_t action_seek_forward = {
    .title = "Seek Forward",
    .name = "seek_fwd",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_seek_forward_cb),
    .next = &action_play_random
};

static DB_plugin_action_t action_seek_backward = {
    .title = "Seek Backward",
    .name = "seek_back",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_seek_backward_cb),
    .next = &action_seek_forward
};

static DB_plugin_action_t action_volume_up = {
    .title = "Volume Up",
    .name = "volume_up",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_volume_up_cb),
    .next = &action_seek_backward
};

static DB_plugin_action_t action_volume_down = {
    .title = "Volume Down",
    .name = "volume_down",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_volume_down_cb),
    .next = &action_volume_up
};

static DB_plugin_action_t action_toggle_stop_after_current = {
    .title = "Toggle Stop After Current",
    .name = "toggle_stop_after_current",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(action_toggle_stop_after_current_cb),
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
    .misc.plugin.api_vminor = 0,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 0,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.id = "hotkeys",
    .misc.plugin.name = "Global hotkeys support",
    .misc.plugin.descr = "Allows one to control player with global hotkeys",
    .misc.plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .reset = hotkeys_reset,
};

