#ifndef _UADE_MAIN_H_
#define _UADE_MAIN_H_

#include <limits.h>
#include <stdlib.h>

#include "uadeipc.h"

struct uade_song {
  char playername[PATH_MAX];       /* filename of eagleplayer */
  char modulename[PATH_MAX];       /* filename of song */
  char scorename[PATH_MAX];        /* filename of score file */

  int min_subsong;
  int max_subsong;
  int cur_subsong;
};


void uade_change_subsong(int subs);
void uade_check_sound_buffers(int bytes);
void uade_send_debug(const char *fmt, ...);
void uade_get_amiga_message(void);
void uade_handle_r_state(void);
void uade_option(int, char**); /* handles command line parameters */
void uade_reset(void);
void uade_send_amiga_message(int msgtype);
void uade_set_automatic_song_end(int song_end_possible);
void uade_set_ntsc(int usentsc);
void uade_song_end(char *reason, int kill_it);
void uade_swap_buffer_bytes(void *data, int bytes);

extern int uade_audio_output;
extern int uade_audio_skip;
extern int uade_debug;
extern int uade_local_sound;
extern int uade_read_size;
extern int uade_reboot;
extern int uade_time_critical;

extern struct uade_ipc uadeipc;

#endif
