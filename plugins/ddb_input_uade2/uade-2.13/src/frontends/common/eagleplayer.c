/* 
 * Loads contents of 'eagleplayer.conf'. The file formats are
 * specified in doc/uade123.1.
 *
 * Copyright 2005-2007 Heikki Orsila <heikki.orsila@iki.fi>
 *
 * This source code module is dual licensed under GPL and Public Domain.
 * Hence you may use _this_ module (not another code module) in any you
 * want in your projects.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>

#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "eagleplayer.h"
#include "ossupport.h"
#include "amifilemagic.h"
#include "uadeconf.h"
#include "unixatomic.h"
#include "songdb.h"
#include "support.h"
#include "uadestate.h"


#define OPTION_DELIMITER ","

#define MAX_SUFFIX_LENGTH 16

#define eperror(fmt, args...) do { uadeerror("Eagleplayer.conf error on line %zd: " fmt, lineno, ## args); } while (0)


/* Table for associating eagleplayer.conf, song.conf and uade.conf options
 * together.
 */
const struct epconfattr epconf[] = {
	{.s = "a500",               .e = ES_A500,                .o = UC_FILTER_TYPE, .c = "a500"},
	{.s = "a1200",              .e = ES_A1200,               .o = UC_FILTER_TYPE, .c = "a1200"},
	{.s = "always_ends",        .e = ES_ALWAYS_ENDS,         .o = UC_DISABLE_TIMEOUTS},
	{.s = "broken_song_end",    .e = ES_BROKEN_SONG_END,     .o = UC_NO_EP_END},
	{.s = "detect_format_by_content", .e = ES_CONTENT_DETECTION,   .o = UC_CONTENT_DETECTION},
	{.s = "detect_format_by_name",    .e = ES_NAME_DETECTION,      .o = 0},
	{.s = "ignore_player_check",.e = ES_IGNORE_PLAYER_CHECK, .o = UC_IGNORE_PLAYER_CHECK},
	{.s = "led_off",            .e = ES_LED_OFF,             .o = UC_FORCE_LED_OFF},
	{.s = "led_on",             .e = ES_LED_ON,              .o = UC_FORCE_LED_ON},
	{.s = "never_ends",         .e = ES_NEVER_ENDS,          .o = 0},
	{.s = "no_ep_end_detect",   .e = ES_BROKEN_SONG_END,     .o = UC_NO_EP_END},
	{.s = "no_filter",          .e = ES_NO_FILTER,           .o = UC_NO_FILTER},
	{.s = "no_headphones",      .e = ES_NO_HEADPHONES,       .o = UC_NO_HEADPHONES},
	{.s = "no_panning",         .e = ES_NO_PANNING,          .o = UC_NO_PANNING},
	{.s = "no_postprocessing",  .e = ES_NO_POSTPROCESSING,   .o = UC_NO_POSTPROCESSING},
	{.s = "ntsc",               .e = ES_NTSC,                .o = UC_NTSC},
	{.s = "one_subsong",        .e = ES_ONE_SUBSONG,         .o = UC_ONE_SUBSONG},
	{.s = "pal",                .e = ES_PAL,                 .o = UC_PAL},
	{.s = "reject",             .e = ES_REJECT,              .o = 0},
	{.s = "speed_hack",         .e = ES_SPEED_HACK,          .o = UC_SPEED_HACK},
	{.s = NULL}
};


/* Variables for eagleplayer.conf and song.conf */
static const struct epconfattr epconf_variables[] = {
	{.s = "epopt",           .t = UA_STRING, .e = ES_EP_OPTION},
	{.s = "gain",            .t = UA_STRING, .e = ES_GAIN},
	{.s = "interpolator",    .t = UA_STRING, .e = ES_RESAMPLER},
	{.s = "panning",         .t = UA_STRING, .e = ES_PANNING},
	{.s = "player",          .t = UA_STRING, .e = ES_PLAYER},
	{.s = "resampler",       .t = UA_STRING, .e = ES_RESAMPLER},
	{.s = "silence_timeout", .t = UA_STRING, .e = ES_SILENCE_TIMEOUT},
	{.s = "subsong_timeout", .t = UA_STRING, .e = ES_SUBSONG_TIMEOUT},
	{.s = "subsongs",        .t = UA_STRING, .e = ES_SUBSONGS},
	{.s = "timeout",         .t = UA_STRING, .e = ES_TIMEOUT},
	{.s = NULL}
};


static int ufcompare(const void *a, const void *b);
static struct eagleplayerstore *read_eagleplayer_conf(const char *filename);


static struct eagleplayer *get_eagleplayer(const char *extension,
					   struct eagleplayerstore *playerstore);


static int load_playerstore(struct uade_state *state)
{
	static int warnings = 1;
	char formatsfile[PATH_MAX];

	if (state->playerstore == NULL) {
		snprintf(formatsfile, sizeof(formatsfile),
			 "%s/eagleplayer.conf", state->config.basedir.name);

		state->playerstore = read_eagleplayer_conf(formatsfile);
		if (state->playerstore == NULL) {
			if (warnings) {
				fprintf(stderr,	"Tried to load eagleplayer.conf from %s, but failed\n",	formatsfile);
			}
			warnings = 0;
			return 0;
		}

		if (state->config.verbose)
			fprintf(stderr, "Loaded eagleplayer.conf: %s\n",
				formatsfile);
	}

	return 1;
}


static struct eagleplayer *analyze_file_format(int *content,
					       const char *modulename,
					       struct uade_state *state)
{
	struct stat st;
	char ext[MAX_SUFFIX_LENGTH];
	FILE *f;
	struct eagleplayer *contentcandidate = NULL;
	struct eagleplayer *namecandidate = NULL;
	char *prefix, *postfix, *t;
	size_t bufsize, bytesread;
	uint8_t buf[8192];

	*content = 0;

	if ((f = fopen(modulename, "rb")) == NULL)
		return NULL;

	if (fstat(fileno(f), &st))
		uadeerror("Very weird stat error: %s (%s)\n", modulename, strerror(errno));

	bufsize = sizeof buf;
	bytesread = atomic_fread(buf, 1, bufsize, f);
	fclose(f);
	if (bytesread == 0)
		return NULL;
	memset(&buf[bytesread], 0, bufsize - bytesread);

	uade_filemagic(buf, bytesread, ext, st.st_size, modulename, state->config.verbose);

	if (strcmp(ext, "reject") == 0)
		return NULL;

	if (ext[0] != 0 && state->config.verbose)
		fprintf(stderr, "Content recognized: %s (%s)\n", ext, modulename);

	if (strcmp(ext, "packed") == 0)
		return NULL;

	if (!load_playerstore(state))
		return NULL;

	/* First do filename detection (we'll later do content detection) */
	t = xbasename(modulename);

	if (strlcpy((char *) buf, t, sizeof buf) >= sizeof buf)
		return NULL;

	t = strchr((char *) buf, '.');
	if (t == NULL)
		return NULL;

	*t = 0;
	prefix = (char *) buf;

	if (strlen(prefix) < MAX_SUFFIX_LENGTH)
		namecandidate = get_eagleplayer(prefix, state->playerstore);

	if (namecandidate == NULL) {
		/* Try postfix */
		t = xbasename(modulename);
		strlcpy((char *) buf, t, sizeof buf);
		postfix = strrchr((char *) buf, '.') + 1; /* postfix != NULL */

		if (strlen(postfix) < MAX_SUFFIX_LENGTH)
			namecandidate = get_eagleplayer(postfix, state->playerstore);
	}

	/* If filemagic found a match, we'll use player plugins associated with
	   that extension */
	if (ext[0]) {
		contentcandidate = get_eagleplayer(ext, state->playerstore);
		if (contentcandidate != NULL) {
			/* Do not recognize name detectable eagleplayers by
			   content */
			if (namecandidate == NULL ||
			    (namecandidate->flags & ES_NAME_DETECTION) == 0) {
				*content = 1;
				return contentcandidate;
			}
		} else {
			if (state->config.verbose)
				fprintf(stderr,	"%s not in eagleplayer.conf\n", ext);
		}
	}

	if (state->config.verbose)
		fprintf(stderr, "Format detection by filename\n");

	return namecandidate;
}


static void handle_attribute(struct uade_attribute **attributelist,
			     const struct epconfattr *attr,
			     char *item, size_t len, size_t lineno)
{
	struct uade_attribute *a;
	char *str, *endptr;
	int success = 0;

	if (item[len] != '=') {
		fprintf(stderr, "Invalid song item: %s\n", item);
		return;
	}
	str = item + len + 1;

	if ((a = calloc(1, sizeof *a)) == NULL)
		eperror("No memory for song attribute.\n");

	switch (attr->t) {
	case UA_DOUBLE:
		a->d = strtod(str, &endptr);
		if (*endptr == 0)
			success = 1;
		break;
	case UA_INT:
		a->i = strtol(str, &endptr, 10);
		if (*endptr == 0)
			success = 1;
		break;
	case UA_STRING:
		a->s = strdup(str);
		if (a->s == NULL)
			eperror("Out of memory allocating string option for song\n");
		success = 1;
		break;
	default:
		fprintf(stderr, "Unknown song option: %s\n",
			item);
		break;
	}

	if (success) {
		a->type = attr->e;
		a->next = *attributelist;
		*attributelist = a;
	} else {
		fprintf(stderr, "Invalid song option: %s\n", item);
		free(a);
	}
}


int uade_song_and_player_attribute(struct uade_attribute **attributelist,
				   int *flags, char *item, size_t lineno)
{
	size_t i, len;

	for (i = 0; epconf[i].s != NULL; i++) {
		if (strcasecmp(item, epconf[i].s) == 0) {
			*flags |= epconf[i].e;
			return 1;
		}
	}

	for (i = 0; epconf_variables[i].s != NULL; i++) {
		len = strlen(epconf_variables[i].s);
		if (strncasecmp(item, epconf_variables[i].s, len) != 0)
			continue;

		handle_attribute(attributelist, &epconf_variables[i],
				 item, len, lineno);
		return 1;
	}

	return 0;
}

/* Compare function for bsearch() and qsort() to sort eagleplayers with
   respect to name extension. */
static int ufcompare(const void *a, const void *b)
{
	const struct eagleplayermap *ua = a;
	const struct eagleplayermap *ub = b;

	return strcasecmp(ua->extension, ub->extension);
}

int uade_is_our_file(const char *modulename, int scanmode,
		     struct uade_state *state)
{
	int content;
	struct eagleplayer *ep;

	ep = analyze_file_format(&content, modulename, state);

	if (!scanmode)
		state->ep = ep;

	if (ep == NULL)
		return 0;

	if (content)
		return 1;

	if (state->config.content_detection && content == 0)
		return 0;

	if ((ep->flags & ES_CONTENT_DETECTION) != 0)
		return 0;

	return 1;
}

static struct eagleplayer *get_eagleplayer(const char *extension,
					   struct eagleplayerstore *ps)
{
	struct eagleplayermap *uf = ps->map;
	struct eagleplayermap *f;
	struct eagleplayermap key = {.extension = (char *)extension };

	f = bsearch(&key, uf, ps->nextensions, sizeof(uf[0]), ufcompare);
	if (f == NULL)
		return NULL;

	return f->player;
}

/* Read eagleplayer.conf. */
static struct eagleplayerstore *read_eagleplayer_conf(const char *filename)
{
	FILE *f;
	struct eagleplayer *p;
	size_t allocated;
	size_t lineno = 0;
	struct eagleplayerstore *ps = NULL;
	size_t exti;
	size_t i, j;
	int epwarning;

	f = fopen(filename, "r");
	if (f == NULL)
		goto error;

	ps = calloc(1, sizeof ps[0]);
	if (ps == NULL)
		eperror("No memory for ps.");

	allocated = 16;
	if ((ps->players = malloc(allocated * sizeof(ps->players[0]))) == NULL)
		eperror("No memory for eagleplayer.conf file.\n");

	while (1) {
		char **items;
		size_t nitems;

		items = read_and_split_lines(&nitems, &lineno, f, UADE_WS_DELIMITERS);
		if (items == NULL)
			break;

		assert(nitems > 0);

		if (ps->nplayers == allocated) {
			allocated *= 2;
			ps->players = realloc(ps->players, allocated * sizeof(ps->players[0]));
			if (ps->players == NULL)
				eperror("No memory for players.");
		}

		p = &ps->players[ps->nplayers];
		ps->nplayers++;

		memset(p, 0, sizeof p[0]);

		p->playername = strdup(items[0]);
		if (p->playername == NULL)
			uadeerror("No memory for playername.\n");

		for (i = 1; i < nitems; i++) {

			if (strncasecmp(items[i], "prefixes=", 9) == 0) {
				char prefixes[UADE_LINESIZE];
				char *prefixstart = items[i] + 9;
				char *sp, *s;
				size_t pos;

				assert(p->nextensions == 0 && p->extensions == NULL);

				p->nextensions = 0;
				strlcpy(prefixes, prefixstart,
					sizeof(prefixes));
				sp = prefixes;
				while ((s = strsep(&sp, OPTION_DELIMITER)) != NULL) {
					if (*s == 0)
						continue;
					p->nextensions++;
				}

				p->extensions =
				    malloc((p->nextensions +
					    1) * sizeof(p->extensions[0]));
				if (p->extensions == NULL)
					eperror("No memory for extensions.");

				pos = 0;
				sp = prefixstart;
				while ((s = strsep(&sp, OPTION_DELIMITER)) != NULL) {
					if (*s == 0)
						continue;

					p->extensions[pos] = strdup(s);
					if (s == NULL)
						eperror("No memory for prefix.");
					pos++;
				}
				p->extensions[pos] = NULL;
				assert(pos == p->nextensions);

				continue;
			}

			if (strncasecmp(items[i], "comment:", 7) == 0)
				break;

			if (uade_song_and_player_attribute(&p->attributelist, &p->flags, items[i], lineno))
				continue;

			fprintf(stderr, "Unrecognized option: %s\n", items[i]);
		}

		for (i = 0; items[i] != NULL; i++)
			free(items[i]);

		free(items);
	}

	fclose(f);

	if (ps->nplayers == 0) {
		free(ps->players);
		free(ps);
		return NULL;
	}

	for (i = 0; i < ps->nplayers; i++)
		ps->nextensions += ps->players[i].nextensions;

	ps->map = malloc(sizeof(ps->map[0]) * ps->nextensions);
	if (ps->map == NULL)
		eperror("No memory for extension map.");

	exti = 0;
	epwarning = 0;
	for (i = 0; i < ps->nplayers; i++) {
		p = &ps->players[i];
		if (p->nextensions == 0) {
			if (epwarning == 0) {
				fprintf(stderr,
					"uade warning: %s eagleplayer lacks prefixes in "
					"eagleplayer.conf, which makes it unusable for any kind of "
					"file type detection. If you don't want name based file type "
					"detection for a particular format, use content_detection "
					"option for the line in eagleplayer.conf.\n",
					ps->players[i].playername);
				epwarning = 1;
			}
			continue;
		}
		for (j = 0; j < p->nextensions; j++) {
			assert(exti < ps->nextensions);
			ps->map[exti].player = p;
			ps->map[exti].extension = p->extensions[j];
			exti++;
		}
	}

	assert(exti == ps->nextensions);

	/* Make the extension map bsearch() ready */
	qsort(ps->map, ps->nextensions, sizeof(ps->map[0]), ufcompare);

	return ps;

 error:
	if (ps)
		free(ps->players);
	free(ps);
	if (f != NULL)
		fclose(f);
	return NULL;
}
