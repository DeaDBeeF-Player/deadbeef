#ifndef _UADE_EAGLEPLAYER_H_
#define _UADE_EAGLEPLAYER_H_

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "uadeconfstructure.h"

/* We maintain alphabetical order even if that forces us to renumber bits
   when a new option is added */
#define ES_A1200               (1 <<  0)
#define ES_A500                (1 <<  1)
#define ES_ALWAYS_ENDS         (1 <<  2)
#define ES_BROKEN_SONG_END     (1 <<  3)
#define ES_CONTENT_DETECTION   (1 <<  4)
#define ES_EP_OPTION           (1 <<  5)
#define ES_GAIN                (1 <<  6)
#define ES_IGNORE_PLAYER_CHECK (1 <<  7)
#define ES_LED_OFF             (1 <<  8)
#define ES_LED_ON              (1 <<  9)
#define ES_NAME_DETECTION      (1 << 10)
#define ES_NEVER_ENDS          (1 << 11)
#define ES_NO_FILTER           (1 << 12)
#define ES_NO_HEADPHONES       (1 << 13)
#define ES_NO_PANNING          (1 << 14)
#define ES_NO_POSTPROCESSING   (1 << 15)
#define ES_NTSC                (1 << 16)
#define ES_ONE_SUBSONG         (1 << 17)
#define ES_PAL                 (1 << 18)
#define ES_PANNING             (1 << 19)
#define ES_PLAYER              (1 << 20)
#define ES_REJECT              (1 << 21)
#define ES_RESAMPLER           (1 << 22)
#define ES_SILENCE_TIMEOUT     (1 << 23)
#define ES_SPEED_HACK          (1 << 24)
#define ES_SUBSONGS            (1 << 25)
#define ES_SUBSONG_TIMEOUT     (1 << 26)
#define ES_TIMEOUT             (1 << 27)

#define UADE_WS_DELIMITERS " \t\n"

struct eagleplayer {
	char *playername;
	size_t nextensions;
	char **extensions;
	int flags;
	struct uade_attribute *attributelist;
};

struct eagleplayermap {
	char *extension;
	struct eagleplayer *player;
};

struct eagleplayerstore {
	size_t nplayers;
	struct eagleplayer *players;
	size_t nextensions;
	struct eagleplayermap *map;
};

enum uade_attribute_type {
	UA_STRING = 1,
	UA_INT,
	UA_DOUBLE
};

struct uade_attribute;

struct uade_attribute {
	struct uade_attribute *next;
	enum uade_attribute_type type;
	char *s;
	int i;
	double d;
};

struct uade_song {
	char md5[33];

	char module_filename[PATH_MAX];

	char playername[256];	/* Eagleplayer name in players directory */
	char modulename[256];	/* From score */
	char formatname[256];

	uint8_t *buf;
	size_t bufsize;

	int min_subsong;
	int max_subsong;
	int cur_subsong;

	int playtime;
	int flags;
	int nsubsongs;
	uint8_t *subsongs;
	struct uade_attribute *songattributes;
	struct uade_ep_options ep_options;
	char *normalisation;

	int64_t out_bytes;

	int64_t silence_count;
};

struct epconfattr {
	char *s;                    /* config file directive/variable name */
	int e;                      /* ES_* flags for eagleplayers and songs */
        int o;                      /* UC_* flag for uade.conf option */
	char *c;                    /* constant for an UC_* flag */
	enum uade_attribute_type t; /* if variable, its special type */
};


extern const struct epconfattr epconf[];


/* FIX: A forward declaration to avoid circular dependency */
struct uade_state;

int uade_is_our_file(const char *modulename, int scanmode, struct uade_state *state);
int uade_song_and_player_attribute(struct uade_attribute **attributelist,
				   int *flags, char *item, size_t lineno);

#endif
