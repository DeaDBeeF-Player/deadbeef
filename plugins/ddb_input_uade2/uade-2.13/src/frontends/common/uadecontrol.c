/* uadecontrol.c is a helper module to control uade core through IPC:

   Copyright (C) 2005 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#include "uadecontrol.h"
#include "ossupport.h"
#include "sysincludes.h"
#include "uadeconstants.h"
#include "songdb.h"

static void subsong_control(int subsong, int command, struct uade_ipc *ipc);

void uade_change_subsong(struct uade_state *state)
{
	state->song->silence_count = 0;

	uade_lookup_volume_normalisation(state);

	subsong_control(state->song->cur_subsong, UADE_COMMAND_CHANGE_SUBSONG, &state->ipc);
}

int uade_read_request(struct uade_ipc *ipc)
{
	uint32_t left = UADE_MAX_MESSAGE_SIZE - sizeof(struct uade_msg);
	if (uade_send_u32(UADE_COMMAND_READ, left, ipc)) {
		fprintf(stderr, "\ncan not send read command\n");
		return 0;
	}
	return left;
}

static int send_ep_options(struct uade_ep_options *eo, struct uade_ipc *ipc)
{
	if (eo->s > 0) {
		size_t i = 0;
		while (i < eo->s) {
			char *s = &eo->o[i];
			size_t l = strlen(s) + 1;
			assert((i + l) <= eo->s);
			if (uade_send_string
			    (UADE_COMMAND_SET_PLAYER_OPTION, s, ipc)) {
				fprintf(stderr,
					"Can not send eagleplayer option.\n");
				return -1;
			}
			i += l;
		}
	}
	return 0;
}

void uade_send_filter_command(struct uade_state *state)
{
	struct uade_config *uadeconf = &state->config;
	struct uade_ipc *ipc = &state->ipc;

	int filter_type = uadeconf->filter_type;
	int filter_state = uadeconf->led_state;
	int force_filter = uadeconf->led_forced;

	if (uadeconf->no_filter)
		filter_type = 0;

	/* Note that filter state is not normally forced */
	filter_state = force_filter ? (2 + (filter_state & 1)) : 0;

	if (uade_send_two_u32s
	    (UADE_COMMAND_FILTER, filter_type, filter_state, ipc)) {
		fprintf(stderr, "Can not setup filters.\n");
		exit(-1);
	}
}

static void send_resampling_command(struct uade_ipc *ipc,
				    struct uade_config *uadeconf)
{
	char *mode = uadeconf->resampler;
	if (mode != NULL) {
		if (strlen(mode) == 0) {
			fprintf(stderr, "Resampling mode may not be empty.\n");
			exit(-1);
		}
		if (uade_send_string
		    (UADE_COMMAND_SET_RESAMPLING_MODE, mode, ipc)) {
			fprintf(stderr, "Can not set resampling mode.\n");
			exit(-1);
		}
	}
}

static void subsong_control(int subsong, int command, struct uade_ipc *ipc)
{
	assert(subsong >= 0 && subsong < 256);
	if (uade_send_u32(command, (uint32_t) subsong, ipc) < 0) {
		fprintf(stderr, "Could not changet subsong\n");
		exit(-1);
	}
}

void uade_set_subsong(int subsong, struct uade_ipc *ipc)
{
	subsong_control(subsong, UADE_COMMAND_SET_SUBSONG, ipc);
}

int uade_song_initialization(const char *scorename,
			     const char *playername,
			     const char *modulename,
			     struct uade_state *state)
{
	uint8_t space[UADE_MAX_MESSAGE_SIZE];
	struct uade_msg *um = (struct uade_msg *)space;
	struct uade_ipc *ipc = &state->ipc;
	struct uade_config *uc = &state->config;
	struct uade_song *us = state->song;

	if (uade_send_string(UADE_COMMAND_SCORE, scorename, ipc)) {
		fprintf(stderr, "Can not send score name.\n");
		goto cleanup;
	}

	if (uade_send_string(UADE_COMMAND_PLAYER, playername, ipc)) {
		fprintf(stderr, "Can not send player name.\n");
		goto cleanup;
	}

	if (uade_send_string(UADE_COMMAND_MODULE, modulename, ipc)) {
		fprintf(stderr, "Can not send module name.\n");
		goto cleanup;
	}

	if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc)) {
		fprintf(stderr, "Can not send token after module.\n");
		goto cleanup;
	}

    printf ("uade_song_initialization: receive message\n");
	if (uade_receive_message(um, sizeof(space), ipc) <= 0) {
		fprintf(stderr, "Can not receive acknowledgement.\n");
		goto cleanup;
	}

	if (um->msgtype == UADE_REPLY_CANT_PLAY) {
		if (uade_receive_short_message(UADE_COMMAND_TOKEN, ipc)) {
			fprintf(stderr,
				"Can not receive token in main loop.\n");
			exit(-1);
		}
		return UADECORE_CANT_PLAY;
	}

	if (um->msgtype != UADE_REPLY_CAN_PLAY) {
		fprintf(stderr, "Unexpected reply from uade: %u\n",
			(unsigned int)um->msgtype);
		goto cleanup;
	}

	if (uade_receive_short_message(UADE_COMMAND_TOKEN, ipc) < 0) {
		fprintf(stderr, "Can not receive token after play ack.\n");
		goto cleanup;
	}

	if (uc->ignore_player_check) {
		if (uade_send_short_message(UADE_COMMAND_IGNORE_CHECK, ipc) < 0) {
			fprintf(stderr, "Can not send ignore check message.\n");
			goto cleanup;
		}
	}

	if (uc->no_ep_end) {
		if (uade_send_short_message
		    (UADE_COMMAND_SONG_END_NOT_POSSIBLE, ipc) < 0) {
			fprintf(stderr,
				"Can not send 'song end not possible'.\n");
			goto cleanup;
		}
	}

	uade_send_filter_command(state);

	send_resampling_command(ipc, uc);

	if (uc->speed_hack) {
		if (uade_send_short_message(UADE_COMMAND_SPEED_HACK, ipc)) {
			fprintf(stderr, "Can not send speed hack command.\n");
			goto cleanup;
		}
	}

	if (uc->use_ntsc) {
		if (uade_send_short_message(UADE_COMMAND_SET_NTSC, ipc)) {
			fprintf(stderr, "Can not send ntsc command.\n");
			goto cleanup;
		}
	}

	if (uc->frequency != UADE_DEFAULT_FREQUENCY) {
		if (uade_send_u32
		    (UADE_COMMAND_SET_FREQUENCY, uc->frequency, ipc)) {
			fprintf(stderr, "Can not send frequency.\n");
			goto cleanup;
		}
	}

	if (uc->use_text_scope) {
		if (uade_send_short_message(UADE_COMMAND_USE_TEXT_SCOPE, ipc)) {
			fprintf(stderr,	"Can not send use text scope command.\n");
			goto cleanup;
		}
	}

	if (send_ep_options(&us->ep_options, ipc) ||
	    send_ep_options(&uc->ep_options, ipc))
		goto cleanup;

    printf ("uade_song_initialization: success\n");

	return 0;

      cleanup:
	return UADECORE_INIT_ERROR;
}

void uade_spawn(struct uade_state *state, const char *uadename,
		const char *configname)
{
	uade_arch_spawn(&state->ipc, &state->pid, uadename);

	if (uade_send_string(UADE_COMMAND_CONFIG, configname, &state->ipc)) {
		fprintf(stderr, "Can not send config name: %s\n",
			strerror(errno));
		kill(state->pid, SIGTERM);
		state->pid = 0;
		abort();
	}
}
