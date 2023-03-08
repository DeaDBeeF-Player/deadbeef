/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  pcm volume manipulation routines

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#ifndef __VOLUME_H
#define __VOLUME_H

void
volume_set_db (float dB);

float
volume_get_db (void);

void
volume_set_amp (float amp);

float
volume_get_amp (void);

float
db_to_amp (float dB);

float
amp_to_db (float amp);

float
volume_get_min_db (void);

void
audio_set_mute (int mute);

int
audio_is_mute (void);

#endif // __VOLUME_H
