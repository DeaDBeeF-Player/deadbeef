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

typedef void (*command_func_t) (void);

typedef struct {
    int keycode;
    int modifier;
    command_func_t func;
} command_t;

static command_t commands [MAX_COMMAND_COUNT];
static int command_count = 0;


typedef struct {
    char* name;
    void (*func) (void);
} known_command_t;

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

static void
cmd_seek_fwd () {
    deadbeef->playback_set_pos (deadbeef->playback_get_pos () + 5);
}

static void
cmd_seek_back () {
    deadbeef->playback_set_pos (deadbeef->playback_get_pos () - 5);
}

static void
cmd_volume_up () {
    deadbeef->volume_set_db (deadbeef->volume_get_db () + 2);
}

static void
cmd_volume_down () {
    deadbeef->volume_set_db (deadbeef->volume_get_db () - 2);
}

static void
cmd_stop_after_current () {
    int var = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    var = 1 - var;
    deadbeef->conf_set_int ("playlist.stop_after_current", var);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}

static command_func_t
get_command (const char* command)
{
    if (!strcasecmp (command, "toggle_pause"))
        return deadbeef->playback_pause;

    else if (!strcasecmp (command, "play"))
        return deadbeef->playback_play;

    else if (!strcasecmp (command, "prev"))
        return deadbeef->playback_prev;

    else if (!strcasecmp (command, "next"))
        return deadbeef->playback_next;

    else if (!strcasecmp (command, "stop"))
        return deadbeef->playback_stop;

    else if (!strcasecmp (command, "play_random"))
        return deadbeef->playback_random;

    else if (!strcasecmp (command, "seek_fwd"))
        return cmd_seek_fwd;

    else if (!strcasecmp (command, "seek_back"))
        return cmd_seek_back;

    else if (!strcasecmp (command, "volume_up"))
        return cmd_volume_up;

    else if (!strcasecmp (command, "volume_down"))
        return cmd_volume_down;

    else if (!strcasecmp (command, "toggle_stop_after_current"))
        return cmd_stop_after_current;

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
//        fprintf (stderr, "hotkeys: adding %s %s\n", item->key, item->value);
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
                    fprintf (stderr, "hotkeys: got 0 from get_keycode while adding hotkey: %s %s\n", item->key, item->value);
                    break;
                }
            }
        } while (!done);

        if (done) {
            if (cmd_entry->keycode == 0) {
                fprintf (stderr, "hotkeys: Key not found while parsing %s %s\n", item->key, item->value);
            }
            else {
                command = trim (command);
                cmd_entry->func = get_command (command);
                if (!cmd_entry->func)
                {
                    fprintf (stderr, "hotkeys: Unknown command <%s> while parsing %s %s\n", command,  item->key, item->value);
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
        XGrabKey (disp, commands[i].keycode, commands[i].modifier, DefaultRootWindow (disp), False, GrabModeAsync, GrabModeAsync);
    }
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
    fprintf (stderr, "hotkeys: xlib error: %s\n", buffer);
}

static void
hotkeys_event_loop (void *unused) {
    int i;

    while (!finished) {
        if (need_reset) {
            trace ("hotkeys: reinitializing\n");
            XSetErrorHandler (x_err_handler);
            for (int i = 0; i < command_count; i++) {
                XUngrabKey (disp, commands[i].keycode, commands[i].modifier, DefaultRootWindow (disp));
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
                        commands[i].func ();
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
hotkeys_start (void) {
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
}

static int
hotkeys_stop (void) {
    if (loop_tid) {
        finished = 1;
        deadbeef->thread_join (loop_tid);
        cleanup ();
    }
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

// define plugin interface
static DB_hotkeys_plugin_t plugin = {
    .misc.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .misc.plugin.api_vminor = DB_API_VERSION_MINOR,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.id = "hotkeys",
    .misc.plugin.name = "Global hotkeys support",
    .misc.plugin.descr = "Allows to control player with global hotkeys",
    .misc.plugin.author = "Viktor Semykin",
    .misc.plugin.email = "thesame.ml@gmail.com",
    .misc.plugin.website = "http://deadbeef.sf.net",
    .misc.plugin.start = hotkeys_start,
    .misc.plugin.stop = hotkeys_stop,
    .get_name_for_keycode = hotkeys_get_name_for_keycode,
    .reset = hotkeys_reset,
};

