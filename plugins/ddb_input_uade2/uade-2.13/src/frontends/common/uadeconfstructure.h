#ifndef _UADECONF_STRUCTURE_H_
#define _UADECONF_STRUCTURE_H_

#include <limits.h>

enum uade_option {
	UC_ACTION_KEYS = 0x1000,
	UC_AO_OPTION,
	UC_BASE_DIR,
	UC_BUFFER_TIME,
	UC_CONTENT_DETECTION,
	UC_CYGWIN_DRIVE_WORKAROUND,
	UC_DISABLE_TIMEOUTS,
	UC_ENABLE_TIMEOUTS,
	UC_EAGLEPLAYER_OPTION,
	UC_FILTER_TYPE,
	UC_FORCE_LED_OFF,
	UC_FORCE_LED_ON,
	UC_FORCE_LED,
	UC_FREQUENCY,
	UC_GAIN,
	UC_HEADPHONES,
	UC_HEADPHONES2,
	UC_IGNORE_PLAYER_CHECK,
	UC_NO_FILTER,
	UC_NO_HEADPHONES,
	UC_NO_PANNING,
	UC_NO_POSTPROCESSING,
	UC_NO_EP_END,
	UC_NORMALISE,
	UC_NTSC,
	UC_ONE_SUBSONG,
	UC_PAL,
	UC_PANNING_VALUE,
	UC_RANDOM_PLAY,
	UC_RECURSIVE_MODE,
	UC_RESAMPLER,
	UC_SILENCE_TIMEOUT_VALUE,
	UC_SONG_TITLE,
	UC_SPEED_HACK,
	UC_SUBSONG_TIMEOUT_VALUE,
	UC_TIMEOUT_VALUE,
	UC_USE_TEXT_SCOPE,
	UC_VERBOSE
};

struct uade_dir {
	char name[PATH_MAX];
};

struct uade_ep_options {
	char o[256];
	size_t s;
};

struct uade_ao_options {
	char o[256];
};

#define UADE_CHAR_CONFIG(x) char x; char x##_set;
#define UADE_FLOAT_CONFIG(x) float x; char x##_set;
#define UADE_INT_CONFIG(x) int x; char x##_set;

/* All the options are put into an instance of this structure.
 * There can be many structures, one for uade.conf and the other for
 * command line options. Then these structures are then merged together
 * to know the complete behavior for each case. Note, these structures
 * can be conflicting, so the options are merged in following order
 * so that the last merge will determine true behavior:
 *
 *     1. set uade.conf options
 *     2. set eagleplayer attributes
 *     3. set song attributes
 *     4. set command line options
 *
 * Merging works by looking at X_set members of this structure. X_set
 * member indicates that feature X has explicitly been set, so the
 * merge will notice the change in value.
 */
struct uade_config {
	UADE_CHAR_CONFIG(action_keys);

	struct uade_ao_options ao_options;
	char ao_options_set;

	struct uade_dir basedir;
	char basedir_set;

	UADE_INT_CONFIG(buffer_time);
	UADE_CHAR_CONFIG(content_detection);
	UADE_CHAR_CONFIG(cygwin_drive_workaround);

	struct uade_ep_options ep_options;
	char ep_options_set;

	UADE_CHAR_CONFIG(filter_type);
	UADE_INT_CONFIG(frequency);
	UADE_CHAR_CONFIG(led_forced);
	UADE_CHAR_CONFIG(led_state);

	UADE_CHAR_CONFIG(gain_enable);
	/* should be removed of uade_effect integrated */
	UADE_FLOAT_CONFIG(gain);

	UADE_CHAR_CONFIG(headphones);
	UADE_CHAR_CONFIG(headphones2);
	UADE_CHAR_CONFIG(ignore_player_check);

	char *resampler;
	char resampler_set;

	UADE_CHAR_CONFIG(no_ep_end);
	UADE_CHAR_CONFIG(no_filter);
	UADE_CHAR_CONFIG(no_postprocessing);

	UADE_CHAR_CONFIG(normalise);
	/* no normalise_parameter_set entry, use manual merging code */
	char *normalise_parameter;

	UADE_CHAR_CONFIG(one_subsong);
	UADE_FLOAT_CONFIG(panning);		/* should be removed */
	UADE_CHAR_CONFIG(panning_enable);
	UADE_CHAR_CONFIG(random_play);
	UADE_CHAR_CONFIG(recursive_mode);
	UADE_INT_CONFIG(silence_timeout);

	char *song_title;
	char song_title_set;

	UADE_CHAR_CONFIG(speed_hack);
	UADE_INT_CONFIG(subsong_timeout);
	UADE_INT_CONFIG(timeout);
	UADE_CHAR_CONFIG(use_text_scope);
	UADE_CHAR_CONFIG(use_timeouts);
	UADE_CHAR_CONFIG(use_ntsc);
	UADE_CHAR_CONFIG(verbose);
};

#endif
