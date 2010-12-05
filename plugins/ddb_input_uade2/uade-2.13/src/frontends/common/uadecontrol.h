#ifndef _UADE_CONTROL_
#define _UADE_CONTROL_

#include <sys/types.h>

#include <uadestate.h>

enum {
	UADECORE_INIT_OK = 0,
	UADECORE_INIT_ERROR,
	UADECORE_CANT_PLAY
};

void uade_change_subsong(struct uade_state *state);
int uade_read_request(struct uade_ipc *ipc);
void uade_send_filter_command(struct uade_state *state);
void uade_set_subsong(int subsong, struct uade_ipc *ipc);
int uade_song_initialization(const char *scorename, const char *playername,
			     const char *modulename,
			     struct uade_state *state);
void uade_spawn(struct uade_state *state, const char *uadename,
		const char *configname);

#endif
