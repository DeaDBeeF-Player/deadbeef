/*
    Hotkeys plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2012-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(__MINGW32__) || defined(__APPLE__)
#define NO_XLIB_H
#endif
#ifndef NO_XLIB_H
#include <X11/Xlib.h>
#endif
#include <ctype.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "../libparser/parser.h"
#include "hotkeys.h"
#include <deadbeef/deadbeef.h>
#include "actionhandlers.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_hotkeys_plugin_t plugin;
DB_functions_t *deadbeef;

#ifndef NO_XLIB_H
static int finished;
static Display *disp;
static intptr_t loop_tid;
static int need_reset = 0;
#endif

#define MAX_COMMAND_COUNT 256

typedef struct {
    const char *name;
    int keysym;
#ifndef NO_XLIB_H
    int keycode; // after mapping
#endif
} xkey_t;

#define KEY(kname, kcode) { .name=kname, .keysym=kcode},

static xkey_t keys[] = {
    #include "keysyms.inc"
};

typedef struct command_s {
    int keycode;
#ifndef NO_XLIB_H
    int x11_keycode;
#endif
    int modifier;
    ddb_action_context_t ctx;
    int isglobal;
    DB_plugin_action_t *action;
} command_t;

static command_t commands [MAX_COMMAND_COUNT];
static int command_count = 0;

#ifndef NO_XLIB_H
static void
init_mapped_keycodes (Display *disp, Atom *syms, int first_kk, int last_kk, int ks_per_kk) {
    int i, ks;
    for (i = 0; i < last_kk-first_kk; i++)
    {
        int sym = * (syms + i*ks_per_kk);
        for (ks = 0; keys[ks].name; ks++)
        {
            if (keys[ ks ].keysym == sym)
            {
                keys[ks].keycode = i+first_kk;
            }
        }
    }
}
#endif

static int
get_keycode (const char* name) {
    for (int i = 0; keys[i].name; i++) {
        if (!strcmp (name, keys[i].name)) {
            trace ("init: key %s code %x\n", name, keys[i].keysym);
            return keys[i].keysym;
        }
    }
    return 0;
}

#ifndef NO_XLIB_H
static void
cmd_invoke_plugin_command (DB_plugin_action_t *action, ddb_action_context_t ctx)
{
    if (action->callback) {
        if (ctx == DDB_ACTION_CTX_MAIN) {
            // collect stuff for 1.4 user data

            // common action
            if (action->flags & DB_ACTION_COMMON)
            {
                action->callback (action, NULL);
                return;
            }

            // playlist action
            if (action->flags & DB_ACTION_PLAYLIST)
            {
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt) {
                    action->callback (action, plt);
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

            if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS)
            {
                action->callback (action, NULL);
            }
            else {
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
        }
    }
    else {
        action->callback2 (action, ctx);
    }
}
#endif

static DB_plugin_action_t *
find_action_by_name (const char *command) {
    // find action with this name, and add to list
    DB_plugin_action_t *actions = NULL;
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->get_actions) {
            actions = p->get_actions (NULL);
            while (actions) {
                if (actions->name && actions->title && !strcasecmp (actions->name, command)) {
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

#ifndef NO_XLIB_H
static int
get_x11_keycode (const char *name, Atom *syms, int first_kk, int last_kk, int ks_per_kk) {
    int i, ks;

    for (i = 0; i < last_kk-first_kk; i++)
    {
        int sym = * (syms + i*ks_per_kk);
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

static int
read_config (Display *disp) {
    int ks_per_kk;
    int first_kk, last_kk;
    Atom* syms;

    XDisplayKeycodes (disp, &first_kk, &last_kk);
    syms = XGetKeyboardMapping (disp, first_kk, last_kk - first_kk, &ks_per_kk);
#else
#define ShiftMask       (1<<0)
#define LockMask        (1<<1)
#define ControlMask     (1<<2)
#define Mod1Mask        (1<<3)
#define Mod2Mask        (1<<4)
#define Mod3Mask        (1<<5)
#define Mod4Mask        (1<<6)
#define Mod5Mask        (1<<7)
    int ks_per_kk = -1;
    int first_kk = -1, last_kk = -1;
    int* syms = NULL;
static int
read_config (void) {
#endif
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
            trace ("hotkeys: invalid ctx %d\n", cmd_entry->ctx);
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
        cmd_entry->action = find_action_by_name (token);
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
                    cmd_entry->keycode = get_keycode (p);
#ifndef NO_XLIB_H
                    cmd_entry->x11_keycode = get_x11_keycode (p, syms, first_kk, last_kk, ks_per_kk);
                    trace ("%s: kc=%d, xkc=%d\n", p, cmd_entry->keycode, cmd_entry->x11_keycode);
#endif
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
#ifndef NO_XLIB_H
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
            XGrabKey (disp, commands[i].x11_keycode, commands[i].modifier | flags, DefaultRootWindow (disp), False, GrabModeAsync, GrabModeAsync);
        }
    }
#endif

    return 0;
}

DB_plugin_t *
hotkeys_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static void
cleanup (void) {
    command_count = 0;
#ifndef NO_XLIB_H
    if (disp) {
        XCloseDisplay (disp);
        disp = NULL;
    }
#endif
}

#ifndef NO_XLIB_H
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
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-hotkeys", 0, 0, 0, 0);
#endif

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
                    XUngrabKey (disp, commands[i].x11_keycode, commands[i].modifier | flags, DefaultRootWindow (disp));
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
                // ignore caps/scroll/numlock
                state &= (ShiftMask|ControlMask|Mod1Mask|Mod4Mask);
                trace ("hotkeys: key %d mods %X (%X)\n", event.xkey.keycode, state, event.xkey.state);
                trace ("filtered state=%X\n", state);
                for (i = 0; i < command_count; i++) {
                    if ( (event.xkey.keycode == commands[ i ].x11_keycode) &&
                         (state == commands[ i ].modifier))
                    {
                        trace ("matches to commands[%d]!\n", i);
                        cmd_invoke_plugin_command (commands[i].action, commands[i].ctx);
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
#endif

static int
hotkeys_connect (void) {
#ifndef NO_XLIB_H
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
    Atom* syms;
    XDisplayKeycodes (disp, &first_kk, &last_kk);
    syms = XGetKeyboardMapping (disp, first_kk, last_kk - first_kk, &ks_per_kk);
    init_mapped_keycodes (disp, syms, first_kk, last_kk, ks_per_kk);
    XFree (syms);
    XSync (disp, 0);
    loop_tid = deadbeef->thread_start (hotkeys_event_loop, 0);
#else
    read_config ();
#endif
    return 0;
}

static int
hotkeys_message(uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (id == DB_EV_PLUGINSLOADED) {
        hotkeys_connect();
    }
    return 0;
}

static int
hotkeys_disconnect (void) {
#ifndef NO_XLIB_H
    if (loop_tid) {
        finished = 1;
        deadbeef->thread_join (loop_tid);
    }
#endif
    cleanup ();
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

    if (key < 0x7f && isupper (key)) {
        key = tolower (key);
    }

    int keycode = key;

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
#ifndef NO_XLIB_H
    need_reset = 1;
    trace ("hotkeys: reset flagged\n");
#else
    // When not using X11, it's assumed that this is always called from UI branch.
    read_config();
#endif
}

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
    var = 1 - var;
    deadbeef->conf_set_int ("playlist.stop_after_current", var);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    return 0;
}

int
action_toggle_stop_after_album_cb (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    int var = deadbeef->conf_get_int ("playlist.stop_after_album", 0);
    var = 1 - var;
    deadbeef->conf_set_int ("playlist.stop_after_album", var);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    return 0;
}

static DB_plugin_action_t action_prev_or_restart = {
    .title = "Playback/Previous or Restart Current Track",
    .name = "prev_or_restart",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_prev_or_restart_cb,
    .next = NULL
};

static DB_plugin_action_t action_duplicate_playlist = {
    .title = "Duplicate Playlist",
    .name = "duplicate_playlist",
    .flags = DB_ACTION_PLAYLIST | DB_ACTION_ADD_MENU,
    .callback2 = action_duplicate_playlist_cb,
    .next = &action_prev_or_restart
};

static DB_plugin_action_t action_reload_metadata = {
    .title = "Reload Metadata",
    .name = "reload_metadata",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback2 = action_reload_metadata_handler,
    .next = &action_duplicate_playlist
};

static DB_plugin_action_t action_jump_to_current = {
    .title = "Playback/Jump to Currently Playing Track",
    .name = "jump_to_current_track",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_jump_to_current_handler,
    .next = &action_reload_metadata
};

static DB_plugin_action_t action_skip_to_prev_genre = {
    .title = "Playback/Skip to/Previous Genre",
    .name = "skip_to_prev_genre",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_prev_genre_handler,
    .next = &action_jump_to_current
};

static DB_plugin_action_t action_skip_to_prev_composer = {
    .title = "Playback/Skip to/Previous Composer",
    .name = "skip_to_prev_composer",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_prev_composer_handler,
    .next = &action_skip_to_prev_genre
};

static DB_plugin_action_t action_skip_to_prev_artist = {
    .title = "Playback/Skip to/Previous Artist",
    .name = "skip_to_prev_artist",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_prev_artist_handler,
    .next = &action_skip_to_prev_composer
};

static DB_plugin_action_t action_skip_to_prev_album = {
    .title = "Playback/Skip to/Previous Album",
    .name = "skip_to_prev_album",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_prev_album_handler,
    .next = &action_skip_to_prev_artist
};

static DB_plugin_action_t action_skip_to_next_genre = {
    .title = "Playback/Skip to/Next Genre",
    .name = "skip_to_next_genre",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_next_genre_handler,
    .next = &action_skip_to_prev_album
};

static DB_plugin_action_t action_skip_to_next_composer = {
    .title = "Playback/Skip to/Next Composer",
    .name = "skip_to_next_composer",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_next_composer_handler,
    .next = &action_skip_to_next_genre
};

static DB_plugin_action_t action_skip_to_next_artist = {
    .title = "Playback/Skip to/Next Artist",
    .name = "skip_to_next_artist",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_next_artist_handler,
    .next = &action_skip_to_next_composer
};

static DB_plugin_action_t action_skip_to_next_album = {
    .title = "Playback/Skip to/Next Album",
    .name = "skip_to_next_album",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = action_skip_to_next_album_handler,
    .next = &action_skip_to_next_artist
};

static DB_plugin_action_t action_next_playlist = {
    .title = "Next Playlist",
    .name = "next_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_next_playlist_handler,
    .next = &action_skip_to_next_album
};

static DB_plugin_action_t action_prev_playlist = {
    .title = "Previous Playlist",
    .name = "prev_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_prev_playlist_handler,
    .next = &action_next_playlist
};

static DB_plugin_action_t action_playlist10 = {
    .title = "Switch to Playlist 10",
    .name = "playlist10",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist10_handler,
    .next = &action_prev_playlist
};

static DB_plugin_action_t action_playlist9 = {
    .title = "Switch to Playlist 9",
    .name = "playlist9",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist9_handler,
    .next = &action_playlist10
};

static DB_plugin_action_t action_playlist8 = {
    .title = "Switch to Playlist 8",
    .name = "playlist8",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist8_handler,
    .next = &action_playlist9
};

static DB_plugin_action_t action_playlist7 = {
    .title = "Switch to Playlist 7",
    .name = "playlist7",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist7_handler,
    .next = &action_playlist8
};

static DB_plugin_action_t action_playlist6 = {
    .title = "Switch to Playlist 6",
    .name = "playlist6",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist6_handler,
    .next = &action_playlist7
};

static DB_plugin_action_t action_playlist5 = {
    .title = "Switch to Playlist 5",
    .name = "playlist5",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist5_handler,
    .next = &action_playlist6
};

static DB_plugin_action_t action_playlist4 = {
    .title = "Switch to Playlist 4",
    .name = "playlist4",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist4_handler,
    .next = &action_playlist5
};

static DB_plugin_action_t action_playlist3 = {
    .title = "Switch to Playlist 3",
    .name = "playlist3",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist3_handler,
    .next = &action_playlist4
};

static DB_plugin_action_t action_playlist2 = {
    .title = "Switch to Playlist 2",
    .name = "playlist2",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist2_handler,
    .next = &action_playlist3
};

static DB_plugin_action_t action_playlist1 = {
    .title = "Switch to Playlist 1",
    .name = "playlist1",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playlist1_handler,
    .next = &action_playlist2
};

static DB_plugin_action_t action_sort_randomize = {
    .title = "Edit/Sort Randomize",
    .name = "sort_randomize",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_sort_randomize_handler,
    .next = &action_playlist1
};

static DB_plugin_action_t action_sort_by_date = {
    .title = "Edit/Sort by Date",
    .name = "sort_date",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_sort_by_date_handler,
    .next = &action_sort_randomize
};

static DB_plugin_action_t action_sort_by_artist = {
    .title = "Edit/Sort by Artist",
    .name = "sort_artist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_sort_by_artist_handler,
    .next = &action_sort_by_date
};


static DB_plugin_action_t action_sort_by_album = {
    .title = "Edit/Sort by Album",
    .name = "sort_album",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_sort_by_album_handler,
    .next = &action_sort_by_artist
};

static DB_plugin_action_t action_sort_by_tracknr = {
    .title = "Edit/Sort by Track Number",
    .name = "sort_tracknr",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_sort_by_tracknr_handler,
    .next = &action_sort_by_album
};

static DB_plugin_action_t action_sort_by_title = {
    .title = "Edit/Sort by Title",
    .name = "sort_title",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_sort_by_title_handler,
    .next = &action_sort_by_tracknr
};

static DB_plugin_action_t action_invert_selection = {
    .title = "Edit/Invert Selection",
    .name = "invert_selection",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_invert_selection_handler,
    .next = &action_sort_by_title
};

static DB_plugin_action_t action_clear_playlist = {
    .title = "Edit/Clear Playlist",
    .name = "clear_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_clear_playlist_handler,
    .next = &action_invert_selection
};

static DB_plugin_action_t action_remove_from_playqueue = {
    .title = "Playback/Remove from Playback Queue",
    .name = "remove_from_playback_queue",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback2 = action_remove_from_playqueue_handler,
    .next = &action_clear_playlist
};

static DB_plugin_action_t action_add_to_playqueue = {
    .title = "Playback/Add to Playback Queue",
    .name = "add_to_playback_queue",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback2 = action_add_to_playqueue_handler,
    .next = &action_remove_from_playqueue
};

static DB_plugin_action_t action_play_next = {
    .title = "Playback/Add to Front of Playback Queue",
    .name = "add_to_front_of_playback_queue",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback2 = action_prepend_to_playqueue_handler,
    .next = &action_add_to_playqueue
};

static DB_plugin_action_t action_toggle_in_playqueue = {
    .title = "Playback/Toggle in Playback Queue",
    .name = "toggle_in_playback_queue",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST,
    .callback2 = action_toggle_in_playqueue_handler,
    .next = &action_play_next
};

static DB_plugin_action_t action_move_tracks_down = {
    .title = "Move/Move Tracks Down",
    .name = "move_tracks_down",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST,
    .callback2 = action_move_tracks_down_handler,
    .next = &action_toggle_in_playqueue
};

static DB_plugin_action_t action_move_tracks_up = {
    .title = "Move/Move Tracks Up",
    .name = "move_tracks_up",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST,
    .callback2 = action_move_tracks_up_handler,
    .next = &action_move_tracks_down
};

static DB_plugin_action_t action_toggle_mute = {
    .title = "Playback/Toggle Mute",
    .name = "toggle_mute",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_mute_handler,
    .next = &action_move_tracks_up
};

static DB_plugin_action_t action_play = {
    .title = "Playback/Play",
    .name = "play",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_play_cb,
    .next = &action_toggle_mute
};

static DB_plugin_action_t action_stop = {
    .title = "Playback/Stop",
    .name = "stop",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_stop_cb,
    .next = &action_play
};

static DB_plugin_action_t action_prev = {
    .title = "Playback/Previous",
    .name = "prev",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_prev_cb,
    .next = &action_stop
};

static DB_plugin_action_t action_next = {
    .title = "Playback/Next",
    .name = "next",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_next_cb,
    .next = &action_prev
};

static DB_plugin_action_t action_toggle_pause = {
    .title = "Playback/Toggle Pause",
    .name = "toggle_pause",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_pause_cb,
    .next = &action_next
};

static DB_plugin_action_t action_play_pause = {
    .title = "Playback/Play\\/Pause",
    .name = "play_pause",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_play_pause_cb,
    .next = &action_toggle_pause
};

static DB_plugin_action_t action_play_random = {
    .title = "Playback/Play Random",
    .name = "playback_random",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_play_random_cb,
    .next = &action_play_pause
};

static DB_plugin_action_t action_seek_1s_forward = {
    .title = "Playback/Seek 1s Forward",
    .name = "seek_1s_fwd",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_1s_forward_cb,
    .next = &action_play_random
};

static DB_plugin_action_t action_seek_1s_backward = {
    .title = "Playback/Seek 1s Backward",
    .name = "seek_1s_back",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_1s_backward_cb,
    .next = &action_seek_1s_forward
};

static DB_plugin_action_t action_seek_5s_forward = {
    .title = "Playback/Seek 5s Forward",
    .name = "seek_5s_fwd",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_5s_forward_cb,
    .next = &action_seek_1s_backward
};

static DB_plugin_action_t action_seek_5s_backward = {
    .title = "Playback/Seek 5s Backward",
    .name = "seek_5s_back",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_5s_backward_cb,
    .next = &action_seek_5s_forward
};


static DB_plugin_action_t action_seek_1p_forward = {
    .title = "Playback/Seek 1% Forward",
    .name = "seek_1p_fwd",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_1p_forward_cb,
    .next = &action_seek_5s_backward
};

static DB_plugin_action_t action_seek_1p_backward = {
    .title = "Playback/Seek 1% Backward",
    .name = "seek_1p_back",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_1p_backward_cb,
    .next = &action_seek_1p_forward
};

static DB_plugin_action_t action_seek_5p_forward = {
    .title = "Playback/Seek 5% Forward",
    .name = "seek_5p_fwd",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_5p_forward_cb,
    .next = &action_seek_1p_backward
};

static DB_plugin_action_t action_seek_5p_backward = {
    .title = "Playback/Seek 5% Backward",
    .name = "seek_5p_back",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_seek_5p_backward_cb,
    .next = &action_seek_5p_forward
};

static DB_plugin_action_t action_volume_up = {
    .title = "Playback/Volume Up",
    .name = "volume_up",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_volume_up_cb,
    .next = &action_seek_5p_backward
};

static DB_plugin_action_t action_volume_down = {
    .title = "Playback/Volume Down",
    .name = "volume_down",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_volume_down_cb,
    .next = &action_volume_up
};

static DB_plugin_action_t action_toggle_stop_after_current = {
    .title = "Playback/Toggle Stop After Current Track",
    .name = "toggle_stop_after_current",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_stop_after_current_cb,
    .next = &action_volume_down
};

static DB_plugin_action_t action_toggle_stop_after_album = {
    .title = "Playback/Toggle Stop After Current Album",
    .name = "toggle_stop_after_album",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_stop_after_album_cb,
    .next = &action_toggle_stop_after_current
};

static DB_plugin_action_t *
hotkeys_get_actions (DB_playItem_t *it)
{
    return &action_toggle_stop_after_album;
}

// define plugin interface
static DB_hotkeys_plugin_t plugin = {
    .misc.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .misc.plugin.api_vminor = DB_API_VERSION_MINOR,
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
        "Copyright (C) 2012-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
    .misc.plugin.stop = hotkeys_disconnect,
    .get_name_for_keycode = hotkeys_get_name_for_keycode,
    .get_action_for_keycombo = hotkeys_get_action_for_keycombo,
    .reset = hotkeys_reset,
    .misc.plugin.message = hotkeys_message,
};

