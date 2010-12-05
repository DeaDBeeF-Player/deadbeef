/* Handle uade.conf file

   Copyright (C) 2005 Heikki Orsila <heikki.orsila@iki.fi>

   This source code module is dual licensed under GPL and Public Domain.
   Hence you may use _this_ module (not another code module) in any way you
   want in your projects.
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ossupport.h"
#include "uadeconf.h"
#include "uadeconfig.h"
#include "amigafilter.h"
#include "uadeconstants.h"
#include "songdb.h"
#include "uadeutils.h"
#include "support.h"

static int uade_set_silence_timeout(struct uade_config *uc, const char *value);
static int uade_set_subsong_timeout(struct uade_config *uc, const char *value);
static int uade_set_timeout(struct uade_config *uc, const char *value);


struct uade_conf_opts {
	char *str;
	int l;
	enum uade_option e;
};

/* List of uade.conf options. The list includes option name, minimum
   string match length for the option name and its enum code. */
static const struct uade_conf_opts uadeconfopts[] = {
	{.str = "action_keys",           .l = 2,  .e = UC_ACTION_KEYS},
	{.str = "ao_option",             .l = 2,  .e = UC_AO_OPTION},
	{.str = "buffer_time",           .l = 1,  .e = UC_BUFFER_TIME},
	{.str = "cygwin",                .l = 1,  .e = UC_CYGWIN_DRIVE_WORKAROUND},
	{.str = "detect_format_by_detection", .l = 18, .e = UC_CONTENT_DETECTION},
	{.str = "disable_timeout",       .l = 1,  .e = UC_DISABLE_TIMEOUTS},
	{.str = "enable_timeout",        .l = 2,  .e = UC_ENABLE_TIMEOUTS},
	{.str = "ep_option",             .l = 2,  .e = UC_EAGLEPLAYER_OPTION},
	{.str = "filter_type",           .l = 2,  .e = UC_FILTER_TYPE},
	{.str = "force_led_off",         .l = 12, .e = UC_FORCE_LED_OFF},
	{.str = "force_led_on",          .l = 12, .e = UC_FORCE_LED_ON},
	{.str = "force_led",             .l = 9,  .e = UC_FORCE_LED},
	{.str = "frequency",             .l = 2,  .e = UC_FREQUENCY},
	{.str = "gain",                  .l = 1,  .e = UC_GAIN},
	{.str = "headphones",            .l = 11, .e = UC_HEADPHONES},
	{.str = "headphones2",           .l = 11, .e = UC_HEADPHONES2},
	{.str = "headphone",             .l = 11, .e = UC_HEADPHONES},
	{.str = "ignore_player_check",   .l = 2,  .e = UC_IGNORE_PLAYER_CHECK},
	{.str = "interpolator",          .l = 2,  .e = UC_RESAMPLER},
	{.str = "magic_detection",       .l = 1,  .e = UC_CONTENT_DETECTION},
	{.str = "no_ep_end_detect",      .l = 4,  .e = UC_NO_EP_END},
	{.str = "no_filter",             .l = 4,  .e = UC_NO_FILTER},
	{.str = "no_song_end",           .l = 4,  .e = UC_NO_EP_END},
	{.str = "normalise",             .l = 1,  .e = UC_NORMALISE},
	{.str = "ntsc",                  .l = 2,  .e = UC_NTSC},
	{.str = "one_subsong",           .l = 1,  .e = UC_ONE_SUBSONG},
	{.str = "pal",                   .l = 3,  .e = UC_PAL},
	{.str = "panning_value",         .l = 3,  .e = UC_PANNING_VALUE},
	{.str = "random_play",           .l = 3,  .e = UC_RANDOM_PLAY},
	{.str = "recursive_mode",        .l = 3,  .e = UC_RECURSIVE_MODE},
	{.str = "resampler",             .l = 3,  .e = UC_RESAMPLER},
	{.str = "silence_timeout_value", .l = 2,  .e = UC_SILENCE_TIMEOUT_VALUE},
	{.str = "song_title",            .l = 2,  .e = UC_SONG_TITLE},
	{.str = "speed_hack",            .l = 2,  .e = UC_SPEED_HACK},
	{.str = "subsong_timeout_value", .l = 2,  .e = UC_SUBSONG_TIMEOUT_VALUE},
	{.str = "timeout_value",         .l = 1,  .e = UC_TIMEOUT_VALUE},
	{.str = "verbose",               .l = 1,  .e = UC_VERBOSE},
	{.str = NULL} /* END OF LIST */
};


/* Map an uade.conf option to an enum */
static enum uade_option map_str_to_option(const char *key)
{
	size_t i;

	for (i = 0; uadeconfopts[i].str != NULL; i++) {
		if (strncmp(key, uadeconfopts[i].str, uadeconfopts[i].l) == 0)
			return uadeconfopts[i].e;
	}

	return 0;
}

/* The function sets the default options. No *_set variables are set because
   we don't want any option to become mergeable by default. See
   uade_merge_configs(). */
void uade_config_set_defaults(struct uade_config *uc)
{
	memset(uc, 0, sizeof(*uc));
	uc->action_keys = 1;
	strlcpy(uc->basedir.name, UADE_CONFIG_BASE_DIR,	sizeof uc->basedir.name);
	uade_set_filter_type(uc, NULL);
	uc->frequency = UADE_DEFAULT_FREQUENCY;
	uc->gain = 1.0;
	uc->panning = 0.7;
	uc->silence_timeout = 20;
	uc->subsong_timeout = 512;
	uc->timeout = -1;
	uc->use_timeouts = 1;
}

double uade_convert_to_double(const char *value, double def, double low,
			      double high, const char *type)
{
	char *endptr, *newvalue;
	char newseparator;
	double v;

	if (value == NULL)
		return def;

	v = strtod(value, &endptr);

	/* Decimal separator conversion, if needed */
	if (*endptr == ',' || *endptr == '.') {
		newvalue = strdup(value);
		if (newvalue == NULL)
			uade_error("Out of memory\n");

		newseparator = (*endptr == ',') ? '.' : ',';

		newvalue[(intptr_t) endptr - (intptr_t) value] = newseparator;

		v = strtod(newvalue, &endptr);
		free(newvalue);
	}

	if (*endptr != 0 || v < low || v > high) {
		fprintf(stderr, "Invalid %s value: %s\n", type, value);
		v = def;
	}

	return v;
}

static void uade_add_ep_option(struct uade_ep_options *opts, const char *s)
{
	size_t freespace = sizeof(opts->o) - opts->s;

	if (strlcpy(&opts->o[opts->s], s, freespace) >= freespace) {
		fprintf(stderr, "Warning: uade eagleplayer option overflow: %s\n", s);
		return;
	}

	opts->s += strlen(s) + 1;
}

static int handle_attributes(struct uade_config *uc, struct uade_song *us,
			     char *playername, size_t playernamelen,
			     int flags, struct uade_attribute *attributelist)
{
	struct uade_attribute *a;
	size_t i;

	for (i = 0; epconf[i].s != NULL; i++) {

		if (epconf[i].o == 0)
			continue;

		if ((flags & epconf[i].e) == 0)
			continue;

		uade_set_config_option(uc, epconf[i].o, epconf[i].c);
	}

	if (flags & ES_NEVER_ENDS)
		fprintf(stderr, "uade: ES_NEVER_ENDS is not implemented. What should it do?\n");

	if (flags & ES_REJECT)
		return -1;

	a = attributelist;

	while (a != NULL) {

		switch (a->type) {
		case ES_EP_OPTION:
			if (uc->verbose)
				fprintf(stderr, "Using eagleplayer option %s\n", a->s);
			uade_add_ep_option(&us->ep_options, a->s);
			break;

		case ES_GAIN:
			uade_set_config_option(uc, UC_GAIN, a->s);
			break;

		case ES_RESAMPLER:
			uade_set_config_option(uc, UC_RESAMPLER, a->s);
			break;

		case ES_PANNING:
			uade_set_config_option(uc, UC_PANNING_VALUE, a->s);
			break;

		case ES_PLAYER:
			if (playername) {
				snprintf(playername, playernamelen, "%s/players/%s", uc->basedir.name, a->s);
			} else {
				fprintf(stderr, "Error: attribute handling was given playername == NULL.\n");
			}
			break;

		case ES_SILENCE_TIMEOUT:
			uade_set_config_option(uc, UC_SILENCE_TIMEOUT_VALUE, a->s);
			break;

		case ES_SUBSONGS:
			fprintf(stderr, "Subsongs not implemented.\n");
			break;

		case ES_SUBSONG_TIMEOUT:
			uade_set_config_option(uc, UC_SUBSONG_TIMEOUT_VALUE, a->s);
			break;

		case ES_TIMEOUT:
			uade_set_config_option(uc, UC_TIMEOUT_VALUE, a->s);
			break;

		default:
			fprintf(stderr,	"Unknown song attribute integer: 0x%x\n", a->type);
			break;
		}

		a = a->next;
	}

	return 0;
}

int uade_set_song_attributes(struct uade_state *state,
			     char *playername, size_t playernamelen)
{
	struct uade_song *us = state->song;
	struct uade_config *uc = &state->config;

	if (us->normalisation)
		uade_set_config_option(uc, UC_NORMALISE, us->normalisation);

	return handle_attributes(uc, us, playername, playernamelen,
				 us->flags, us->songattributes);
}

int uade_load_config(struct uade_config *uc, const char *filename)
{
	char line[256];
	FILE *f;
	char *key, *value;
	int linenumber = 0;
	enum uade_option opt;

	if ((f = fopen(filename, "r")) == NULL)
		return 0;

	uade_config_set_defaults(uc);

	while (xfgets(line, sizeof(line), f) != NULL) {
		linenumber++;

		/* Skip comment lines */
		if (line[0] == '#')
			continue;

		if (!get_two_ws_separated_fields(&key, &value, line))
			continue; /* Skip an empty line */

		opt = map_str_to_option(key);

		if (opt) {
			uade_set_config_option(uc, opt, value);
		} else {
			fprintf(stderr,	"Unknown config key in %s on line %d: %s\n", filename, linenumber, key);
		}
	}

	fclose(f);
	return 1;
}

int uade_load_initial_config(char *uadeconfname, size_t maxlen,
			     struct uade_config *uc, struct uade_config *ucbase)
{
	int loaded;
	char *home;

	assert(maxlen > 0);
	uadeconfname[0] = 0;

	uade_config_set_defaults(uc);

	loaded = 0;

	/* First try to load from forced base dir (testing mode) */
	if (ucbase != NULL && ucbase->basedir_set) {
		snprintf(uadeconfname, maxlen, "%s/uade.conf",
			 ucbase->basedir.name);
		loaded = uade_load_config(uc, uadeconfname);
	}

	home = uade_open_create_home();

	/* Second, try to load config from ~/.uade2/uade.conf */
	if (loaded == 0 && home != NULL) {
		snprintf(uadeconfname, maxlen, "%s/.uade2/uade.conf", home);
		loaded = uade_load_config(uc, uadeconfname);
	}

	/* Third, try to load from install path */
	if (loaded == 0) {
		snprintf(uadeconfname, maxlen, "%s/uade.conf",
			 uc->basedir.name);
		loaded = uade_load_config(uc, uadeconfname);
	}

	return loaded;
}

int uade_load_initial_song_conf(char *songconfname, size_t maxlen,
				struct uade_config *uc,
				struct uade_config *ucbase)
{
	int loaded = 0;
	char *home;

	assert(maxlen > 0);
	songconfname[0] = 0;

	/* Used for testing */
	if (ucbase != NULL && ucbase->basedir_set) {
		snprintf(songconfname, maxlen, "%s/song.conf",
			 ucbase->basedir.name);
		loaded = uade_read_song_conf(songconfname);
	}

	/* Avoid unwanted home directory creation for test mode */
	if (loaded)
		return loaded;

	home = uade_open_create_home();

	/* Try to load from home dir */
	if (loaded == 0 && home != NULL) {
		snprintf(songconfname, maxlen, "%s/.uade2/song.conf", home);
		loaded = uade_read_song_conf(songconfname);
	}

	/* No? Try install path */
	if (loaded == 0) {
		snprintf(songconfname, maxlen, "%s/song.conf",
			 uc->basedir.name);
		loaded = uade_read_song_conf(songconfname);
	}

	return loaded;
}

void uade_merge_configs(struct uade_config *ucd, const struct uade_config *ucs)
{
#define MERGE_OPTION(y) do { if (ucs->y##_set) ucd->y = ucs->y; } while (0)

	MERGE_OPTION(action_keys);
	MERGE_OPTION(ao_options);
	MERGE_OPTION(basedir);
	MERGE_OPTION(buffer_time);
	MERGE_OPTION(content_detection);
	MERGE_OPTION(cygwin_drive_workaround);
	MERGE_OPTION(ep_options);
	MERGE_OPTION(filter_type);
	MERGE_OPTION(frequency);
	MERGE_OPTION(gain);
	MERGE_OPTION(gain_enable);
	MERGE_OPTION(headphones);
	MERGE_OPTION(headphones2);
	MERGE_OPTION(ignore_player_check);
	MERGE_OPTION(led_forced);
	MERGE_OPTION(led_state);
	MERGE_OPTION(no_ep_end);
	MERGE_OPTION(no_filter);
	MERGE_OPTION(no_postprocessing);

	/* Special merge -> don't use MERGE_OPTION macro */
	if (ucs->normalise_set && ucs->normalise) {
		ucd->normalise = 1;
		if (ucs->normalise_parameter != NULL)
			ucd->normalise_parameter = ucs->normalise_parameter;
	}

	MERGE_OPTION(one_subsong);
	MERGE_OPTION(panning);
	MERGE_OPTION(panning_enable);
	MERGE_OPTION(random_play);
	MERGE_OPTION(recursive_mode);
	MERGE_OPTION(resampler);
	MERGE_OPTION(silence_timeout);
	MERGE_OPTION(song_title);
	MERGE_OPTION(speed_hack);
	MERGE_OPTION(subsong_timeout);

	MERGE_OPTION(timeout);
	MERGE_OPTION(use_timeouts);
	if (ucs->timeout_set) {
		ucd->use_timeouts = 1;
		ucd->use_timeouts_set = 1;
	}

	MERGE_OPTION(use_text_scope);
	MERGE_OPTION(use_ntsc);
	MERGE_OPTION(verbose);
}

char *uade_open_create_home(void)
{
	/* Create ~/.uade2 directory if it does not exist */
	char *home = getenv("HOME");
	if (home) {
		char name[PATH_MAX];
		struct stat st;
		snprintf(name, sizeof name, "%s/.uade2", home);
		if (stat(name, &st) != 0)
			mkdir(name, S_IRUSR | S_IWUSR | S_IXUSR);
	}

	return home;
}

int uade_parse_subsongs(int **subsongs, char *option)
{
	char substr[256];
	char *sp, *str;
	size_t pos;
	int nsubsongs;

	nsubsongs = 0;
	*subsongs = NULL;

	if (strlcpy(substr, option, sizeof subsongs) >= sizeof subsongs) {
		fprintf(stderr, "Too long a subsong option: %s\n", option);
		return -1;
	}

	sp = substr;
	while ((str = strsep(&sp, ",")) != NULL) {
		if (*str == 0)
			continue;
		nsubsongs++;
	}

	*subsongs = malloc((nsubsongs + 1) * sizeof((*subsongs)[0]));
	if (*subsongs == NULL) {
		fprintf(stderr, "No memory for subsongs.\n");
		return -1;
	}

	strlcpy(substr, option, sizeof subsongs);

	pos = 0;
	sp = substr;
	while ((str = strsep(&sp, ",")) != NULL) {
		if (*str == 0)
			continue;
		(*subsongs)[pos] = atoi(str);
		pos++;
	}

	(*subsongs)[pos] = -1;
	assert(pos == nsubsongs);

	return nsubsongs;
}

void uade_set_effects(struct uade_state *state)
{
	struct uade_effect *effects = &state->effects;
	struct uade_config *uc = &state->config;

	uade_effect_set_defaults(effects);

	if (uc->no_postprocessing)
		uade_effect_disable(effects, UADE_EFFECT_ALLOW);

	if (uc->gain_enable) {
		uade_effect_gain_set_amount(effects, uc->gain);
		uade_effect_enable(effects, UADE_EFFECT_GAIN);
	}

	if (uc->headphones)
		uade_effect_enable(effects, UADE_EFFECT_HEADPHONES);

	if (uc->headphones2)
		uade_effect_enable(effects, UADE_EFFECT_HEADPHONES2);

	if (uc->normalise) {
		uade_effect_normalise_unserialise(uc->normalise_parameter);
		uade_effect_enable(effects, UADE_EFFECT_NORMALISE);
	}

	if (uc->panning_enable) {
		uade_effect_pan_set_amount(effects, uc->panning);
		uade_effect_enable(effects, UADE_EFFECT_PAN);
	}

	uade_effect_set_sample_rate(effects, uc->frequency);
}

void uade_set_config_option(struct uade_config *uc, enum uade_option opt,
			    const char *value)
{
	char *endptr;
	long x;

#define SET_OPTION(opt, value) do { uc->opt = (value); uc->opt##_set = 1; } while (0)

	switch (opt) {
	case UC_ACTION_KEYS:
		if (value != NULL) {
			uc->action_keys_set = 1;
			if (!strcasecmp(value, "on") || !strcmp(value, "1")) {
				uc->action_keys = 1;
			} else if (!strcasecmp(value, "off") ||
				   !strcmp(value, "0")) {
				uc->action_keys = 0;
			} else {
				fprintf(stderr,
					"uade.conf: Unknown setting for action keys: %s\n",
					value);
			}
		}
		break;

	case UC_AO_OPTION:
		strlcat(uc->ao_options.o, value, sizeof uc->ao_options.o);
		strlcat(uc->ao_options.o, "\n", sizeof uc->ao_options.o);
		uc->ao_options_set = 1;
		break;

	case UC_BASE_DIR:
		if (value != NULL) {
			strlcpy(uc->basedir.name, value,
				sizeof uc->basedir.name);
			uc->basedir_set = 1;
		} else {
			fprintf(stderr, "uade: Passed NULL to UC_BASE_DIR.\n");
		}
		break;

	case UC_BUFFER_TIME:
		if (value != NULL) {
			uc->buffer_time_set = 1;
			uc->buffer_time = strtol(value, &endptr, 10);
			if (uc->buffer_time <= 0 || *endptr != 0) {
				fprintf(stderr, "Invalid buffer_time: %s\n",
					value);
				uc->buffer_time = 0;
			}
		} else {
			fprintf(stderr,
				"uade: Passed NULL to UC_BUFFER_TIME.\n");
		}
		break;

	case UC_CONTENT_DETECTION:
		SET_OPTION(content_detection, 1);
		break;

	case UC_CYGWIN_DRIVE_WORKAROUND:
		SET_OPTION(cygwin_drive_workaround, 1);
		break;

	case UC_DISABLE_TIMEOUTS:
		SET_OPTION(use_timeouts, 0);
		break;

	case UC_ENABLE_TIMEOUTS:
		SET_OPTION(use_timeouts, 1);
		break;

	case UC_EAGLEPLAYER_OPTION:
		if (value != NULL) {
			uade_add_ep_option(&uc->ep_options, value);
			uc->ep_options_set = 1;
		} else {
			fprintf(stderr,
				"uade: Passed NULL to UC_EAGLEPLAYER_OPTION.\n");
		}
		break;

	case UC_FILTER_TYPE:
		SET_OPTION(no_filter, 0);

		if (value != NULL) {
			if (strcasecmp(value, "none") != 0) {
				/* Filter != NONE */
				uade_set_filter_type(uc, value);
				uc->filter_type_set = 1;
			} else {
				/* Filter == NONE */
				uc->no_filter = 1;
			}
		}
		break;

	case UC_FORCE_LED:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_FORCE_LED value is NULL\n");
			break;
		}
		if (strcasecmp(value, "off") == 0 || strcmp(value, "0") == 0) {
			uc->led_state = 0;
		} else if (strcasecmp(value, "on") == 0
			   || strcmp(value, "1") == 0) {
			uc->led_state = 1;
		} else {
			fprintf(stderr, "Unknown force led argument: %s\n",
				value);
			break;
		}
		uc->led_state_set = 1;

		SET_OPTION(led_forced, 1);
		break;

	case UC_FORCE_LED_OFF:
		SET_OPTION(led_forced, 1);
		SET_OPTION(led_state, 0);
		break;

	case UC_FORCE_LED_ON:
		SET_OPTION(led_forced, 1);
		SET_OPTION(led_state, 1);
		break;

	case UC_FREQUENCY:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_FREQUENCY value is NULL\n");
			break;
		}
		x = strtol(value, &endptr, 10);
		if (*endptr != 0) {
			fprintf(stderr, "Invalid frequency number: %s\n",
				value);
			break;
		}
		/* The upper bound is NTSC Amigas bus freq */
		if (x < 1 || x > 3579545) {
			fprintf(stderr, "Frequency out of bounds: %ld\n", x);
			x = UADE_DEFAULT_FREQUENCY;
		}
		SET_OPTION(frequency, x);
		break;

	case UC_GAIN:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_GAIN value is NULL\n");
			break;
		}
		SET_OPTION(gain_enable, 1);
		SET_OPTION(gain, uade_convert_to_double(value, 1.0, 0.0, 128.0, "gain"));
		break;

	case UC_HEADPHONES:
		SET_OPTION(headphones, 1);
		break;

	case UC_HEADPHONES2:
		SET_OPTION(headphones2, 1);
		break;

	case UC_IGNORE_PLAYER_CHECK:
		SET_OPTION(ignore_player_check, 1);
		break;

	case UC_RESAMPLER:
		if (value == NULL) {
			fprintf(stderr, "uade.conf: No resampler given.\n");
			break;
		}
		uc->resampler = strdup(value);
		if (uc->resampler != NULL) {
			uc->resampler_set = 1;
		} else {
			fprintf(stderr,	"uade.conf: no memory for resampler.\n");
		}
		break;

	case UC_NO_EP_END:
		SET_OPTION(no_ep_end, 1);
		break;

	case UC_NO_FILTER:
		SET_OPTION(no_filter, 1);
		break;

	case UC_NO_HEADPHONES:
		SET_OPTION(headphones, 0);
		SET_OPTION(headphones2, 0);
		break;

	case UC_NO_PANNING:
		SET_OPTION(panning_enable, 0);
		break;

	case UC_NO_POSTPROCESSING:
		SET_OPTION(no_postprocessing, 1);
		break;

	case UC_NORMALISE:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_NORMALISE is NULL\n");
			break;
		}
		SET_OPTION(normalise, 1);
		uc->normalise_parameter = (char *) value;
		break;

	case UC_NTSC:
		SET_OPTION(use_ntsc, 1);
		break;

	case UC_ONE_SUBSONG:
		SET_OPTION(one_subsong, 1);
		break;

	case UC_PAL:
		SET_OPTION(use_ntsc, 0);
		break;

	case UC_PANNING_VALUE:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_PANNING_VALUE is NULL\n");
			break;
		}
		SET_OPTION(panning_enable, 1);
		SET_OPTION(panning, uade_convert_to_double(value, 0.0, 0.0, 2.0, "panning"));
		break;

	case UC_RANDOM_PLAY:
		SET_OPTION(random_play, 1);
		break;

	case UC_RECURSIVE_MODE:
		SET_OPTION(recursive_mode, 1);
		break;

	case UC_SILENCE_TIMEOUT_VALUE:
		if (value == NULL) {
			fprintf(stderr,
				"uade: UC_SILENCE_TIMEOUT_VALUE is NULL\n");
			break;
		}
		uade_set_silence_timeout(uc, value);
		break;

	case UC_SONG_TITLE:
		if (value == NULL) {
			fprintf(stderr, "uade: No song_title format given.\n");
			break;
		}
		if ((uc->song_title = strdup(value)) == NULL) {
			fprintf(stderr, "No memory for song title format\n");
		} else {
			uc->song_title_set = 1;
		}
		break;

	case UC_SPEED_HACK:
		SET_OPTION(speed_hack, 1);
		break;

	case UC_SUBSONG_TIMEOUT_VALUE:
		if (value == NULL) {
			fprintf(stderr,
				"uade: UC_SUBSONG_TIMEOUT_VALUE is NULL\n");
			break;
		}
		uade_set_subsong_timeout(uc, value);
		break;

	case UC_TIMEOUT_VALUE:
		if (value == NULL) {
			fprintf(stderr, "uade: UC_TIMEOUT_VALUE is NULL\n");
			break;
		}
		uade_set_timeout(uc, value);
		break;

	case UC_USE_TEXT_SCOPE:
		SET_OPTION(use_text_scope, 1);
		break;

	case UC_VERBOSE:
		SET_OPTION(verbose, 1);
		break;

	default:
		fprintf(stderr, "uade_set_config_option(): unknown enum: %d\n",
			opt);
		exit(1);
	}
}

void uade_set_ep_attributes(struct uade_state *state)
{
	handle_attributes(&state->config, state->song, NULL, 0, state->ep->flags, state->ep->attributelist);
}

void uade_set_filter_type(struct uade_config *uc, const char *model)
{
	uc->filter_type = FILTER_MODEL_A500;

	if (model == NULL)
		return;

	/* a500 and a500e are the same */
	if (strncasecmp(model, "a500", 4) == 0) {
		uc->filter_type = FILTER_MODEL_A500;

		/* a1200 and a1200e are the same */
	} else if (strncasecmp(model, "a1200", 5) == 0) {
		uc->filter_type = FILTER_MODEL_A1200;

	} else {
		fprintf(stderr, "Unknown filter model: %s\n", model);
	}
}

static int uade_set_silence_timeout(struct uade_config *uc, const char *value)
{
	char *endptr;
	int t;
	if (value == NULL) {
		return -1;
	}
	t = strtol(value, &endptr, 10);
	if (*endptr != 0 || t < -1) {
		fprintf(stderr, "Invalid silence timeout value: %s\n", value);
		return -1;
	}
	uc->silence_timeout = t;
	uc->silence_timeout_set = 1;
	return 0;
}

static int uade_set_subsong_timeout(struct uade_config *uc, const char *value)
{
	char *endptr;
	int t;
	if (value == NULL) {
		return -1;
	}
	t = strtol(value, &endptr, 10);
	if (*endptr != 0 || t < -1) {
		fprintf(stderr, "Invalid subsong timeout value: %s\n", value);
		return -1;
	}
	uc->subsong_timeout = t;
	uc->subsong_timeout_set = 1;
	return 0;
}

static int uade_set_timeout(struct uade_config *uc, const char *value)
{
	char *endptr;
	int t;
	if (value == NULL) {
		return -1;
	}
	t = strtol(value, &endptr, 10);
	if (*endptr != 0 || t < -1) {
		fprintf(stderr, "Invalid timeout value: %s\n", value);
		return -1;
	}
	uc->timeout = t;
	uc->timeout_set = 1;
	return 0;
}
