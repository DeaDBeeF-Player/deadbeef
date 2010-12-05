#ifndef _UADE_FRONTEND_CONFIG_H_
#define _UADE_FRONTEND_CONFIG_H_

#include <uadestate.h>

void uade_config_set_defaults(struct uade_config *uc);
double uade_convert_to_double(const char *value, double def,
			      double low, double high, const char *type);
int uade_load_config(struct uade_config *uc, const char *filename);
int uade_load_initial_config(char *uadeconfname, size_t maxlen,
			     struct uade_config *uc,
			     struct uade_config *ucbase);
int uade_load_initial_song_conf(char *songconfname, size_t maxlen,
				struct uade_config *uc,
				struct uade_config *ucbase);
void uade_merge_configs(struct uade_config *ucd, const struct uade_config *ucs);
char *uade_open_create_home(void);
int uade_parse_subsongs(int **subsongs, char *option);
void uade_set_config_option(struct uade_config *uc, enum uade_option opt,
			    const char *value);
void uade_set_effects(struct uade_state *state);
void uade_set_ep_attributes(struct uade_state *state);
int uade_set_song_attributes(struct uade_state *state, char *playername,
			     size_t playernamelen);
void uade_set_filter_type(struct uade_config *uc, const char *value);

#endif
