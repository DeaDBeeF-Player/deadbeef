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

#include "../../deadbeef.h"

static DB_misc_t plugin;
static DB_functions_t *deadbeef;
static int finished;
static Display *disp;
static intptr_t loop_tid;

#define MAX_COMMAND_COUNT 256

typedef struct {
    char *name;
    KeySym keysym;
} xkey_t;

#define KEY( kname, kcode ) { .name=kname, .keysym=kcode },

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
get_keycode( Display *disp, const char* name ) {
    static int first_kk, last_kk;
    static KeySym* syms;
    static int ks_per_kk;
    static int first_time = 1;
    int i, j, ks;

    if ( first_time )
    {
        XDisplayKeycodes( disp, &first_kk, &last_kk );

        syms = XGetKeyboardMapping( disp, first_kk, last_kk - first_kk, &ks_per_kk );
        first_time = 0;
    }

    for ( i = 0; i < last_kk-first_kk; i++ )
    {
        KeySym sym = *(syms + i*ks_per_kk);
        for ( ks = 0; ks < sizeof( keys ) / sizeof( xkey_t ); ks++ )
        {
            if ( (keys[ ks ].keysym == sym) && ( 0 == strcmp(name, keys[ ks ].name) ) )
            {
                return i+first_kk;
            }
        }
    }
    return 0;
}

static char*
trim(char* s)
{
    char *h, *t;
    
    for ( h = s; *h == ' ' || *h == '\t'; h++ );
    for ( t = s + strlen(s); *t == ' ' || *t == '\t'; t-- );
    *(t+1) = 0;
    return h;
}

static void
cmd_seek_fwd() {
        deadbeef->playback_set_pos( deadbeef->playback_get_pos() + 5 );
}

static void
cmd_seek_back() {
        deadbeef->playback_set_pos( deadbeef->playback_get_pos() - 5 );
}

static void
cmd_volume_up() {
        deadbeef->volume_set_db( deadbeef->volume_get_db() + 2 );
}

static void
cmd_volume_down() {
        deadbeef->volume_set_db( deadbeef->volume_get_db() - 2 );
}

static command_func_t
get_command( const char* command )
{
    if ( 0 == strcasecmp( command, "toggle_pause" ) )
        return deadbeef->playback_pause;

    if ( 0 == strcasecmp( command, "play" ) )
        return deadbeef->playback_play;

    if ( 0 == strcasecmp( command, "prev" ) )
        return deadbeef->playback_prev;

    if ( 0 == strcasecmp( command, "next" ) )
        return deadbeef->playback_next;

    if ( 0 == strcasecmp( command, "stop" ) )
        return deadbeef->playback_stop;

    if ( 0 == strcasecmp( command, "play_random" ) )
        return deadbeef->playback_random;

    if ( 0 == strcasecmp( command, "seek_fwd" ) )
        return cmd_seek_fwd;

    if ( 0 == strcasecmp( command, "seek_back" ) )
        return cmd_seek_back;

    if ( 0 == strcasecmp( command, "volume_up" ) )
        return cmd_volume_up;

    if ( 0 == strcasecmp( command, "volume_down" ) )
        return cmd_volume_down;

    return NULL;
}

static int
read_config( Display *disp )
{
    char param[ 256 ];
    char config[1024];
    snprintf (config, 1024, "%s/hotkeys", deadbeef->get_config_dir());
    FILE *cfg_file = fopen (config, "rt");
    if (!cfg_file) {
        fprintf (stderr, "hotkeys: failed open %s\n", config);
        return -1;
    }

    int line_nr = 0;

    while ( fgets( param, sizeof(param), cfg_file ) )
    {
        line_nr++;
        if ( command_count == MAX_COMMAND_COUNT )
        {
            fprintf( stderr, "hotkeys: [Config line %d] Maximum count (%d) of commands exceeded\n", line_nr, MAX_COMMAND_COUNT );
            break;
        }

        command_t *cmd_entry = &commands[ command_count ];
        cmd_entry->modifier = 0;
        cmd_entry->keycode = 0;
        
        param[ strlen( param )-1 ] = 0; //terminating \n
        char* colon = strchr( param, ':' );
        if ( !colon )
        {
            fprintf( stderr, "hotkeys: [Config line %d] Wrong config line\n", line_nr );
            continue;
        }
        char* command = colon+1;
        *colon = 0;

        int modifier = 0;
        char* key = NULL;

        int done = 0;
        char* p;
        char* space = param - 1;
        do {
            p = space+1;
            space = strchr( p, ' ' );
            if ( space )
                *space = 0;
            else
                done = 1;

            if ( 0 == strcasecmp( p, "Ctrl" ) )
                cmd_entry->modifier |= ControlMask;

            else if ( 0 == strcasecmp( p, "Alt" ) )
                cmd_entry->modifier |= Mod1Mask;

            else if ( 0 == strcasecmp( p, "Shift" ) )
                cmd_entry->modifier |= ShiftMask;

            else if ( 0 == strcasecmp( p, "Super" ) ) {
                cmd_entry->modifier |= Mod2Mask;
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
                    cmd_entry->keycode = get_keycode( disp, p );
                }
                if ( !cmd_entry->keycode )
                {
                    fprintf( stderr, "hotkeys: [Config line %d] Unknown key: <%s>\n", line_nr, key );
                    continue;
                }
            }
        }
        while ( !done );

        if ( cmd_entry->keycode == 0 )
        {
            fprintf( stderr, "hotkeys: [Config line %d] Key not specified\n", line_nr );
            continue;
        }

        command = trim (command);
        cmd_entry->func = get_command( command );
        if ( !cmd_entry->func )
        {
            fprintf( stderr, "hotkeys: [Config line %d] Unknown command <%s>\n", line_nr, command );
            continue;        
        }
        command_count++;
    }
}

DB_plugin_t *
hotkeys_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static void
cleanup() {
    command_count = 0;
    XCloseDisplay( disp );
}

static void
hotkeys_event_loop( uintptr_t unused ) {
    int i;

    for ( i = 0; i < command_count; i++ )
        XGrabKey( disp, commands[ i ].keycode, commands[ i ].modifier, DefaultRootWindow( disp ), False, GrabModeAsync, GrabModeAsync );

    while (!finished) {
        XEvent event;

        while ( XPending( disp ) )
        {
            XNextEvent( disp, &event );

            if ( event.xkey.type == KeyPress )
            {
                for ( i = 0; i < command_count; i++ )
                    if ( (event.xkey.keycode == commands[ i ].keycode) &&
                         ((event.xkey.state & commands[ i ].modifier) == commands[ i ].modifier) )
                    {
                        commands[i].func();
                        break;
                    }
            }
        }
        usleep( 200 * 1000 );
    }
}

static int
x_err_handler (Display *disp, XErrorEvent *evt) {
    fprintf( stderr, "hotkeys: We got an Xlib error. Most probably one or more of your hotkeys won't work\n" );
}

static int
hotkeys_start (void) {
    finished = 0;
    loop_tid = 0;
    disp = XOpenDisplay( NULL );
    if ( !disp )
    {
        fprintf( stderr, "Could not open display\n" );
        return -1;
    }
    XSetErrorHandler( x_err_handler );

    read_config( disp );
    if (command_count > 0) {
        loop_tid = deadbeef->thread_start( hotkeys_event_loop, 0 );
    }
    else {
        cleanup();
    }
}

static int
hotkeys_stop (void) {
    if (loop_tid) {
        finished = 1;
        deadbeef->thread_join( loop_tid );
        cleanup();
    }
}

// define plugin interface
static DB_misc_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "Global Hotkeys",
    .plugin.descr = "Allows to control player using xlib global hotkeys",
    .plugin.author = "Viktor Semykin",
    .plugin.email = "thesame.ml@gmail.com",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = hotkeys_start,
    .plugin.stop = hotkeys_stop
};

