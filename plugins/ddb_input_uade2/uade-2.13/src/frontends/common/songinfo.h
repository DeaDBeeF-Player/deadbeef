#ifndef _UADE_SONG_INFO_
#define _UADE_SONG_INFO_

#include <stdio.h>

#include "uadestate.h"

enum song_info_type {
	UADE_MODULE_INFO = 0,
	UADE_HEX_DUMP_INFO,
	UADE_NUMBER_OF_INFOS
};

int uade_generate_song_title(char *title, size_t dstlen,
			     struct uade_state *state);
int uade_song_info(char *info, size_t maxlen, char *filename,
		   enum song_info_type type);

#endif
