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
#    define NO_XLIB_H
#endif
#ifndef NO_XLIB_H
#    include <X11/Xlib.h>
#endif
#include <ctype.h>
#ifdef __linux__
#    include <sys/prctl.h>
#endif

#include "../libparser/parser.h"
#include "hotkeys.h"
#include <deadbeef/deadbeef.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt, ...)

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

#define KEY(kname, kcode) { .name = kname, .keysym = kcode },

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

static command_t commands[MAX_COMMAND_COUNT];
static int command_count = 0;

#ifndef NO_XLIB_H
static void
init_mapped_keycodes (Display *disp, Atom *syms, int first_kk, int last_kk, int ks_per_kk) {
    int i, ks;
    for (i = 0; i < last_kk - first_kk; i++) {
        int sym = *(syms + i * ks_per_kk);
        for (ks = 0; keys[ks].name; ks++) {
            if (keys[ks].keysym == sym) {
                keys[ks].keycode = i + first_kk;
            }
        }
    }
}
#endif

static int
get_keycode (const char *name) {
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
cmd_invoke_plugin_command (DB_plugin_action_t *action, ddb_action_context_t ctx) {
    if (action->callback) {
        if (ctx == DDB_ACTION_CTX_MAIN) {
            // collect stuff for 1.4 user data

            // common action
            if (action->flags & DB_ACTION_COMMON) {
                action->callback (action, NULL);
                return;
            }

            // playlist action
            if (action->flags & DB_ACTION_PLAYLIST) {
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
                if (deadbeef->pl_is_selected (pit)) {
                    if (!selected)
                        selected = pit;
                    selected_count++;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (pit, PL_MAIN);
                deadbeef->pl_item_unref (pit);
                pit = next;
            }

            //Now we're checking if action is applicable:

            if (selected_count == 0) {
                trace ("No tracks selected\n");
                return;
            }
            if ((selected_count == 1) && (!(action->flags & DB_ACTION_SINGLE_TRACK))) {
                trace ("Hotkeys: action %s not allowed for single track\n", action->name);
                return;
            }
            if ((selected_count > 1) && (!(action->flags & DB_ACTION_MULTIPLE_TRACKS))) {
                trace ("Hotkeys: action %s not allowed for multiple tracks\n", action->name);
                return;
            }

            //So, action is allowed, do it.

            if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS) {
                action->callback (action, NULL);
            }
            else {
                pit = deadbeef->pl_get_first (PL_MAIN);
                while (pit) {
                    if (deadbeef->pl_is_selected (pit)) {
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

    for (i = 0; i < last_kk - first_kk; i++) {
        int sym = *(syms + i * ks_per_kk);
        for (ks = 0; keys[ks].name; ks++) {
            if ((keys[ks].keysym == sym) && (0 == strcmp (name, keys[ks].name))) {
                return i + first_kk;
            }
        }
    }
    return 0;
}

static int
read_config (Display *disp) {
    int ks_per_kk;
    int first_kk, last_kk;
    Atom *syms;

    XDisplayKeycodes (disp, &first_kk, &last_kk);
    syms = XGetKeyboardMapping (disp, first_kk, last_kk - first_kk, &ks_per_kk);
#else
#    define ShiftMask (1 << 0)
#    define LockMask (1 << 1)
#    define ControlMask (1 << 2)
#    define Mod1Mask (1 << 3)
#    define Mod2Mask (1 << 4)
#    define Mod3Mask (1 << 5)
#    define Mod4Mask (1 << 6)
#    define Mod5Mask (1 << 7)
int ks_per_kk = -1;
int first_kk = -1, last_kk = -1;
int *syms = NULL;
static int
read_config (void) {
#endif
    DB_conf_item_t *item = deadbeef->conf_find ("hotkey.", NULL);
    while (item) {
        if (command_count == MAX_COMMAND_COUNT) {
            fprintf (stderr, "hotkeys: maximum number (%d) of commands exceeded\n", MAX_COMMAND_COUNT);
            break;
        }

        command_t *cmd_entry = &commands[command_count];
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
        char *p;
        char *space = keycombo;
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
                if (!cmd_entry->keycode) {
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
            XGrabKey (
                disp,
                commands[i].x11_keycode,
                commands[i].modifier | flags,
                DefaultRootWindow (disp),
                False,
                GrabModeAsync,
                GrabModeAsync);
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
#    if 0
    // this code crashes if gtk plugin is active
    char buffer[1024];
    XGetErrorText (d, evt->error_code, buffer, sizeof (buffer));
    trace ("hotkeys: xlib error: %s\n", buffer);
#    endif
    return 0;
}

static void
hotkeys_event_loop (void *unused) {
    int i;
#    ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-hotkeys", 0, 0, 0, 0);
#    endif

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
        while (XPending (disp)) {
            XNextEvent (disp, &event);

            if (event.xkey.type == KeyPress) {
                int state = event.xkey.state;
                // ignore caps/scroll/numlock
                state &= (ShiftMask | ControlMask | Mod1Mask | Mod4Mask);
                trace ("hotkeys: key %d mods %X (%X)\n", event.xkey.keycode, state, event.xkey.state);
                trace ("filtered state=%X\n", state);
                for (i = 0; i < command_count; i++) {
                    if ((event.xkey.keycode == commands[i].x11_keycode) && (state == commands[i].modifier)) {
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
    if (!disp) {
        fprintf (stderr, "hotkeys: could not open display\n");
        return -1;
    }
    XSetErrorHandler (x_err_handler);

    read_config (disp);

    int ks_per_kk;
    int first_kk, last_kk;
    Atom *syms;
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
hotkeys_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (id == DB_EV_PLUGINSLOADED) {
        hotkeys_connect ();
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

DB_plugin_action_t *
hotkeys_get_action_for_keycombo (int key, int mods, int isglobal, ddb_action_context_t *ctx) {
    int i;
    // find mapped keycode

    if (key < 0x7f && isupper (key)) {
        key = tolower (key);
    }

    int keycode = key;

    trace ("hotkeys: keysym 0x%X mapped to 0x%X\n", key, keycode);

    for (i = 0; i < command_count; i++) {
        trace (
            "hotkeys: command %s keycode %x mods %x\n",
            commands[i].action->name,
            commands[i].keycode,
            commands[i].modifier);
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
    read_config ();
#endif
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
        "    * added local hotkeys support\n",
    .misc.plugin.copyright = "Copyright (C) 2012-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
                             "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n",
    .misc.plugin.website = "http://deadbeef.sf.net",
    .misc.plugin.stop = hotkeys_disconnect,
    .get_name_for_keycode = hotkeys_get_name_for_keycode,
    .get_action_for_keycombo = hotkeys_get_action_for_keycombo,
    .reset = hotkeys_reset,
    .misc.plugin.message = hotkeys_message,
};
