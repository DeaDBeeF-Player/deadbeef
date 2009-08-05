#ifndef __PLAYBACK_H
#define __PLAYBACK_H

#if USE_SDL
#include "psdl.h"
#define p_init psdl_init
#define p_free psdl_free
#define p_play psdl_play
#define p_stop psdl_stop
#define p_ispaused psdl_ispaused
#define p_pause psdl_pause
#define p_unpause psdl_unpause
#define p_set_volume psdl_set_volume
#define p_get_rate psdl_get_rate
#else
#include "palsa.h"
#define p_init palsa_init
#define p_free palsa_free
#define p_play palsa_play
#define p_stop palsa_stop
#define p_ispaused palsa_ispaused
#define p_pause palsa_pause
#define p_unpause palsa_unpause
#define p_set_volume palsa_set_volume
#define p_get_rate palsa_get_rate
#endif

#endif // __PLAYBACK_H
