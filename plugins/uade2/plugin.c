/*
    ddb_input_uade2 - UADE input plugin for DeaDBeeF player
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>
    based on UADE2 plugin for Audacious, Copyright (C) 2005-2006  Heikki Orsila, UADE TEAM

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include "../../deadbeef.h"
#include "uadeipc.h"
#include "eagleplayer.h"
#include "uadeconfig.h"
#include "uadecontrol.h"
#include "uadeconstants.h"
#include "ossupport.h"
#include "uadeconf.h"
#include "effects.h"
#include "sysincludes.h"
#include "songdb.h"
#include "songinfo.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static char configname[PATH_MAX];
static struct uade_config config_backup;
static time_t config_load_time;
static time_t md5_load_time;
static char md5name[PATH_MAX];
static char songconfname[PATH_MAX];
static char gui_filename[PATH_MAX];

static const char *
get_uade_base_conf_dir (void) {
    return UADE_CONFIG_BASE_DIR;
}

static void load_content_db(void)
{
  struct stat st;
  time_t curtime = time(NULL);
  char name[PATH_MAX];

  if (curtime)
    md5_load_time = curtime;

  if (md5name[0] == 0) {
    char *home = uade_open_create_home();
    if (home)
      snprintf(md5name, sizeof md5name, "%s/.uade2/contentdb", home);
  }

  /* User database has priority over global database, so we read it first */
  if (md5name[0]) {
    if (stat(md5name, &st) == 0) {
      if (uade_read_content_db(md5name))
	return;
    } else {
      FILE *f = fopen(md5name, "w");
      if (f)
	fclose(f);
      uade_read_content_db(md5name);
    }
  }

  snprintf(name, sizeof name, "%s/contentdb.conf", get_uade_base_conf_dir ());
  if (stat(name, &st) == 0)
    uade_read_content_db(name);
}

/* xmms initializes uade by calling this function */
static void uade_init(void)
{
    static int initialized = 0;
    if (!initialized) {
        char *home;
        int config_loaded;

        config_load_time = time(NULL);

        config_loaded = uade_load_initial_config(configname, sizeof configname,
                &config_backup, NULL);

        load_content_db();

        uade_load_initial_song_conf(songconfname, sizeof songconfname,
                &config_backup, NULL);

        home = uade_open_create_home();

        if (home != NULL) {
            /* If config exists in home, ignore global uade.conf. */
            snprintf(configname, sizeof configname, "%s/.uade2/uade.conf", home);
        }

        if (config_loaded == 0) {
            fprintf(stderr, "No config file found for UADE XMMS plugin. Will try to load config from\n");
            fprintf(stderr, "$HOME/.uade2/uade.conf in the future.\n");
        }
        initialized = 1;
    }
}

static void uade_get_song_info(const char *filename, char **title, int *length)
{
  char tempname[PATH_MAX];
  const char *t;

  if (strncmp(filename, "uade://", 7) == 0)
    filename += 7;

  strlcpy(tempname, filename, sizeof tempname);
  t = basename(tempname);
  if (t == NULL)
    t = filename;
  if ((*title = strdup(t)) == NULL)
    trace("Not enough memory for song info.\n");
  *length = -1;
}

int uade_get_max_subsong(struct uade_state *state, int def)
{
    int subsong;
    subsong = -1;
    if (state->song != NULL)
      subsong = state->song->max_subsong;
    if (subsong == -1)
	subsong = def;
    return subsong;
}


int uade_get_min_subsong(struct uade_state *state, int def)
{
    int subsong;
    subsong = -1;
    if (state->song != NULL)
      subsong = state->song->min_subsong;
    if (subsong == -1)
	subsong = def;
    return subsong;
}

DB_functions_t *deadbeef;
static DB_decoder_t plugin;

static const char *exts[] = { NULL };
static const char *prefixes[] = { "mod", NULL };
static const char *filetypes[] = { "UADE", NULL };

#define UADE_BUFFER_SIZE 100000

typedef struct {
    DB_fileinfo_t info;
    struct uade_state state;
    int controlstate;
    int record_playtime;
    int remaining;
    char buffer[UADE_BUFFER_SIZE];
    int subsong_end;
    int song_end_trigger;
    int64_t skip_bytes;
    int abort_playing;
    int uade_seek_forward;
} uade_info_t;

DB_fileinfo_t *
uadeplug_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (uade_info_t));
    uade_info_t *info = (uade_info_t *)_info;
    memset (info, 0, sizeof (uade_info_t));
    return _info;
}

static int
uadeplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    uade_info_t *info = (uade_info_t *)_info;

    uade_init ();
    char modulename[PATH_MAX];
    char playername[PATH_MAX];
    char scorename[PATH_MAX];
    char gui_module_filename[PATH_MAX];
    char gui_player_filename[PATH_MAX];

    info->state.config = config_backup;
    info->state.validconfig = 1;

    printf ("probing the file\n");
    int ret = uade_is_our_file(it->fname, 0, &info->state);

    if (!ret) {
        trace ("not uade file\n");
        return -1;
    }
    strlcpy(modulename, it->fname, sizeof modulename);
    trace ("modulename: %s\n", modulename);
    strlcpy(gui_module_filename, it->fname, sizeof gui_module_filename);
    trace ("gui_module_fname: %s\n", gui_module_filename);

    snprintf(scorename, sizeof scorename, "%s/score", get_uade_base_conf_dir ());
    trace ("scorename: %s\n", scorename);

    if (strcmp(info->state.ep->playername, "custom") == 0) {
        strlcpy(playername, modulename, sizeof playername);
        modulename[0] = 0;
        gui_module_filename[0] = 0;
    } else {
        snprintf(playername, sizeof playername, "%s/players/%s", get_uade_base_conf_dir (), info->state.ep->playername);
    }
    trace ("playername: %s\n", playername);

    if (!uade_alloc_song(&info->state, it->fname)) {
        trace ("uade_alloc_song fail\n");
        return -1;
    }

    uade_set_ep_attributes(&info->state);

    uade_set_song_attributes(&info->state, playername, sizeof playername);

    uade_set_effects(&info->state);

    strlcpy(gui_player_filename, playername, sizeof gui_player_filename);

    if (!info->state.pid) {
        char configname[PATH_MAX];
        snprintf(configname, sizeof configname, "%s/uaerc", UADE_CONFIG_BASE_DIR);
        uade_spawn(&info->state, UADE_CONFIG_UADE_CORE, configname);
    }

    printf ("uade_song_initialization\n");
    ret = uade_song_initialization(scorename, playername, modulename, &info->state);
    if (ret) {
        if (ret != UADECORE_CANT_PLAY && ret != UADECORE_INIT_ERROR) {
            fprintf(stderr, "Can not initialize song. Unknown error.\n");
            return -1;
        }
        uade_unalloc_song(&info->state);
        return -1;
    }
    printf ("init done\n");
    int minsong = uade_get_min_subsong (&info->state, 0);
    int maxsong = uade_get_max_subsong (&info->state, 0);
	info->state.song->cur_subsong = it->tracknum;
	uade_change_subsong(&info->state);

    _info->fmt.bps = 16;
    _info->fmt.channels = UADE_CHANNELS;
    _info->fmt.samplerate = info->state.config.frequency;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    _info->plugin = &plugin;
    info->record_playtime = 1;
    info->controlstate = UADE_S_STATE;

    return 0;
}

// free everything allocated in _init
static void
uadeplug_free (DB_fileinfo_t *_info) {
    uade_info_t *info = (uade_info_t *)_info;
    if (info) {
        uade_unalloc_song(&info->state);
        if (info->state.pid) {
            kill(info->state.pid, SIGTERM);
        }
        free (info);
    }
}

int
uadeplug_frame (uade_info_t *info) {
    uint8_t space[UADE_MAX_MESSAGE_SIZE];
    struct uade_msg *um = (struct uade_msg *) space;
    uint16_t *sm;
    int i;
    unsigned int play_bytes, tailbytes = 0;
    uint64_t subsong_bytes = 0;
    int framesize = UADE_CHANNELS * UADE_BYTES_PER_SAMPLE;
    int left = 0;
    char gui_formatname[256];
    char gui_modulename[256];
    char gui_playername[256];
    char *reason;
    uint32_t *u32ptr;
    int frame_received = 0;

    while (!frame_received) {
        if (info->controlstate == UADE_S_STATE) {

            assert(left == 0);

            if (info->abort_playing) {
                info->record_playtime = 0;
                break;
            }

            if (info->uade_seek_forward) {
                info->skip_bytes += info->uade_seek_forward * (UADE_BYTES_PER_FRAME * info->state.config.frequency);
                info->uade_seek_forward = 0;
            }

            left = uade_read_request(&info->state.ipc);

            if (uade_send_short_message(UADE_COMMAND_TOKEN, &info->state.ipc)) {
                fprintf(stderr, "Can not send token.\n");
                return -1;
            }
            info->controlstate = UADE_R_STATE;

        } else {

            if (uade_receive_message(um, sizeof(space), &info->state.ipc) <= 0) {
                fprintf(stderr, "Can not receive events from uade\n");
                exit(-1);
            }


            switch (um->msgtype) {

            case UADE_COMMAND_TOKEN:
                info->controlstate = UADE_S_STATE;
                break;

            case UADE_REPLY_DATA:
                sm = (uint16_t *) um->data;
                for (i = 0; i < um->size; i += 2) {
                    *sm = ntohs(*sm);
                    sm++;
                }

                if (info->subsong_end) {
                    play_bytes = tailbytes;
                    tailbytes = 0;
                } else {
                    play_bytes = um->size;
                }

                if (info->subsong_end == 0 && info->song_end_trigger == 0 &&
                        uade_test_silence(um->data, play_bytes, &info->state)) {
                    info->subsong_end = 1;
                }

                subsong_bytes += play_bytes;
                info->state.song->out_bytes += play_bytes;

                if (info->skip_bytes > 0) {
                    if (play_bytes <= info->skip_bytes) {
                        info->skip_bytes -= play_bytes;
                        play_bytes = 0;
                    } else {
                        play_bytes -= info->skip_bytes;
                        info->skip_bytes = 0;
                    }
                }

                uade_effect_run(&info->state.effects, (int16_t *) um->data, play_bytes / framesize);

                // ... copy data ...
                memcpy (info->buffer + info->remaining, um->data, play_bytes);
                frame_received = 1;
                info->remaining += play_bytes;

                if (info->state.config.timeout != -1 && info->state.config.use_timeouts) {
                    if (info->song_end_trigger == 0) {
                        if (info->state.song->out_bytes / (UADE_BYTES_PER_FRAME * info->state.config.frequency) >= info->state.config.timeout) {
                            info->song_end_trigger = 1;
                            info->record_playtime = 0;
                        }
                    }
                }

                if (info->state.config.subsong_timeout != -1 && info->state.config.use_timeouts) {
                    if (info->subsong_end == 0 && info->song_end_trigger == 0) {
                        if (subsong_bytes / (UADE_BYTES_PER_FRAME * info->state.config.frequency) >= info->state.config.subsong_timeout) {
                            info->subsong_end = 1;
                            info->record_playtime = 0;
                        }
                    }
                }

                assert (left >= um->size);
                left -= um->size;
                break;

            case UADE_REPLY_FORMATNAME:
                uade_check_fix_string(um, 128);
                strlcpy(gui_formatname, (char *) um->data, sizeof gui_formatname);
                strlcpy(info->state.song->formatname, (char *) um->data, sizeof info->state.song->formatname);
                break;

            case UADE_REPLY_MODULENAME:
                uade_check_fix_string(um, 128);
                strlcpy(gui_modulename, (char *) um->data, sizeof gui_modulename);
                strlcpy(info->state.song->modulename, (char *) um->data, sizeof info->state.song->modulename);
                break;

            case UADE_REPLY_MSG:
                uade_check_fix_string(um, 128);
                trace ("Message: %s\n", (char *) um->data);
                break;

            case UADE_REPLY_PLAYERNAME:
                uade_check_fix_string(um, 128);
                strlcpy(gui_playername, (char *) um->data, sizeof gui_playername);
                strlcpy(info->state.song->playername, (char *) um->data, sizeof info->state.song->playername);
                break;

            case UADE_REPLY_SONG_END:
                if (um->size < 9) {
                    fprintf(stderr, "Invalid song end reply\n");
                    exit(-1);
                }
                tailbytes = ntohl(((uint32_t *) um->data)[0]);
                /* next ntohl() is only there for a principle. it is not useful */
                if (ntohl(((uint32_t *) um->data)[1]) == 0) {
                    /* normal happy song end. go to next subsong if any */
                    info->subsong_end = 1;
                } else {
                    /* unhappy song end (error in the 68k side). skip to next song
                       ignoring possible subsongs */
                    info->song_end_trigger = 1;
                }
                i = 0;
                reason = (char *) &um->data[8];
                while (reason[i] && i < (um->size - 8))
                    i++;
                if (reason[i] != 0 || (i != (um->size - 9))) {
                    fprintf(stderr, "Broken reason string with song end notice\n");
                    exit(-1);
                }
                /* fprintf(stderr, "Song end (%s)\n", reason); */
                break;

            case UADE_REPLY_SUBSONG_INFO:
                if (um->size != 12) {
                    fprintf(stderr, "subsong info: too short a message\n");
                    exit(-1);
                }
                u32ptr = (uint32_t *) um->data;
                info->state.song->min_subsong = ntohl(u32ptr[0]);
                info->state.song->max_subsong = ntohl(u32ptr[1]);
                info->state.song->cur_subsong = ntohl(u32ptr[2]);

                if (!(-1 <= info->state.song->min_subsong && info->state.song->min_subsong <= info->state.song->cur_subsong && info->state.song->cur_subsong <= info->state.song->max_subsong)) {
                    int tempmin = info->state.song->min_subsong, tempmax = info->state.song->max_subsong;
                    fprintf(stderr, "uade: The player is broken. Subsong info does not match with %s.\n", gui_filename);
                    info->state.song->min_subsong = tempmin <= tempmax ? tempmin : tempmax;
                    info->state.song->max_subsong = tempmax >= tempmin ? tempmax : tempmin;
                    if (info->state.song->cur_subsong > info->state.song->max_subsong)
                        info->state.song->max_subsong = info->state.song->cur_subsong;
                    else if (info->state.song->cur_subsong < info->state.song->min_subsong)
                        info->state.song->min_subsong = info->state.song->cur_subsong;
                }
                break;

            default:
                fprintf(stderr, "Expected sound data. got %d.\n", um->msgtype);
                return -1;
            }
        }
    }
    return 0;
}


// try decode `size' bytes
// return number of decoded bytes
// or 0 on EOF/error
static int
uadeplug_read (DB_fileinfo_t *_info, char *bytes, int size) {
    uade_info_t *info = (uade_info_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    int initsize = size;

    while (size > 0) {
        if (info->remaining) {
            int n = min (info->remaining, size);
            memcpy (bytes, info->buffer, n);
            bytes += n;
            size -= n;
            if (n < info->remaining) {
                memmove (info->buffer, info->buffer + n, info->remaining-n);
                info->remaining -= n;
                break;
            }
            info->remaining = 0;
        }

        if (uadeplug_frame (info) < 0) {
            return initsize-size;
        }
    }

    _info->readpos += ((initsize-size) / samplesize) / (float)(_info->fmt.samplerate);
    return initsize-size;
}

// seek to specified sample (frame)
// return 0 on success
// return -1 on failure
static int
uadeplug_seek_sample (DB_fileinfo_t *_info, int sample) {
    uade_info_t *info = (uade_info_t *)_info;
    
    _info->readpos = (float)sample / _info->fmt.samplerate;
    return 0;
}

// seek to specified time in seconds
// return 0 on success
// return -1 on failure
static int
uadeplug_seek (DB_fileinfo_t *_info, float time) {
    return uadeplug_seek_sample (_info, time * _info->fmt.samplerate);
}

static DB_playItem_t *
uadeplug_insert (DB_playItem_t *after, const char *fname) {
    uade_init ();

    char modulename[PATH_MAX];
    char playername[PATH_MAX];
    char scorename[PATH_MAX];
    char gui_module_filename[PATH_MAX];
    char gui_player_filename[PATH_MAX];

    struct uade_state state;
    memset (&state, 0, sizeof (state));
    state.config = config_backup;
    state.validconfig = 1;

    printf ("probing the file\n");
    int ret = uade_is_our_file(fname, 0, &state);

    if (!ret) {
        trace ("not uade file\n");
        return NULL;
    }

    strlcpy(modulename, fname, sizeof modulename);
    trace ("modulename: %s\n", modulename);
    strlcpy(gui_module_filename, fname, sizeof gui_module_filename);
    trace ("gui_module_fname: %s\n", gui_module_filename);

    snprintf(scorename, sizeof scorename, "%s/score", get_uade_base_conf_dir ());
    trace ("scorename: %s\n", scorename);

    if (strcmp(state.ep->playername, "custom") == 0) {
        strlcpy(playername, modulename, sizeof playername);
        modulename[0] = 0;
        gui_module_filename[0] = 0;
    } else {
        snprintf(playername, sizeof playername, "%s/players/%s", get_uade_base_conf_dir (), state.ep->playername);
    }
    trace ("playername: %s\n", playername);

    if (!uade_alloc_song(&state, fname)) {
        trace ("uade_alloc_song fail\n");
        return NULL;
    }

    uade_set_ep_attributes(&state);

    uade_set_song_attributes(&state, playername, sizeof playername);

    uade_set_effects(&state);

    strlcpy(gui_player_filename, playername, sizeof gui_player_filename);

    if (!state.pid) {
        char configname[PATH_MAX];
        snprintf(configname, sizeof configname, "%s/uaerc", UADE_CONFIG_BASE_DIR);
        uade_spawn(&state, UADE_CONFIG_UADE_CORE, configname);
    }

    printf ("uade_song_initialization\n");
    ret = uade_song_initialization(scorename, playername, modulename, &state);
    if (ret) {
        if (ret != UADECORE_CANT_PLAY && ret != UADECORE_INIT_ERROR) {
            fprintf(stderr, "Can not initialize song. Unknown error.\n");
            return NULL;
        }
        uade_unalloc_song(&state);
        return NULL;
    }
    printf ("init done\n");
    int minsong = uade_get_min_subsong (&state, 0);
    int maxsong = uade_get_max_subsong (&state, 0);

    char info[256];
    int playtime = state.song->playtime;

    /* Hack. Set info text and song length late because we didn't know
       subsong amounts before this. Pass zero as a length so that the
       graphical play time counter will run but seek is still enabled.
       Passing -1 as playtime would disable seeking. */
    if (playtime <= 0)
        playtime = 0;

    if (uade_generate_song_title(info, sizeof info, &state))
        strlcpy(info, gui_filename, sizeof info);

//  playhandle->set_params(playhandle, info, playtime,
//			 UADE_BYTES_PER_FRAME * state.config.frequency,
//			 state.config.frequency, UADE_CHANNELS);


    // no cuesheet, prepare track for addition
    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = filetypes[0];
    float duration = -1;
    if (playtime != -1) {
        duration = playtime / 1000.f;
    }
    deadbeef->pl_set_item_duration (it, duration);

    // title is empty, this call will set track title to filename without extension
    const char *fn = strrchr (fname, '/');
    if (fn) {
        fn++;
    }
    else {
        fn = fname;
    }
    deadbeef->pl_add_meta (it, "title", info);

    // now the track is ready, insert into playlist
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    uade_unalloc_song(&state);
    if (state.pid) {
        kill(state.pid, SIGTERM);
    }
    return after;
}

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "uade",
    .plugin.name = "UADE player",
    .plugin.descr = "amiga module player based on UADE (http://zakalwe.fi/uade/)",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = uadeplug_open,
    .init = uadeplug_init,
    .free = uadeplug_free,
    .read = uadeplug_read,
    .seek = uadeplug_seek,
    .seek_sample = uadeplug_seek_sample,
    .insert = uadeplug_insert,
    .exts = exts,
    .prefixes = prefixes,
    .filetypes = filetypes
};

DB_plugin_t *
ddb_uade2_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
