/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  replaygain support

  Copyright (C) 2009-2013 Alexey Yakovenko

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Alexey Yakovenko waker@users.sourceforge.net
*/
#ifndef __REPLAYGAIN_H
#define __REPLAYGAIN_H

#include "deadbeef.h"

void
replaygain_init_settings (ddb_replaygain_settings_t *settings, playItem_t *it);

void
replaygain_apply (ddb_waveformat_t *fmt, char *bytes, int numbytes);

void
replaygain_apply_with_settings (ddb_replaygain_settings_t *settings, ddb_waveformat_t *fmt, char *bytes, int numbytes);

void
replaygain_set (int mode, int scale, float preamp, float global_preamp);

void
replaygain_set_values (float albumgain, float albumpeak, float trackgain, float trackpeak);

struct ddb_replaygain_params_s;

void
apply_replay_gain_int8 (struct ddb_replaygain_params_s *params, char *bytes, int size);

void
apply_replay_gain_int16 (struct ddb_replaygain_params_s *params, char *bytes, int size);

void
apply_replay_gain_int24 (struct ddb_replaygain_params_s *params, char *bytes, int size);

void
apply_replay_gain_int32 (struct ddb_replaygain_params_s *params, char *bytes, int size);

void
apply_replay_gain_float32 (struct ddb_replaygain_params_s *params, char *bytes, int size);

#endif
