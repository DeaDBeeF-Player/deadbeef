#ifndef __PLUGINS_H
#define __PLUGINS_H
#include "deadbeef.h"

void
plug_ev_subscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data);

void
plug_ev_unsubscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data);

void
plug_trigger_event (int ev);

void
plug_load_all (void);

void
plug_unload_all (void);

void
plug_md5 (uint8_t sig[16], const char *in, int len);

void
plug_md5_to_str (char *str, const uint8_t sig[16]);

void
plug_playback_next (void);

void
plug_playback_prev (void);

void
plug_playback_pause (void);

void 
plug_playback_stop (void);

void 
plug_playback_play (void);

void 
plug_quit (void);

float
plug_playback_get_pos (void);

void
plug_playback_set_pos (float pos);

#endif // __PLUGINS_H
