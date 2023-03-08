/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  playback queue management

  Copyright (C) 2009-2015 Oleksiy Yakovenko

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

#ifndef __deadbeef__playqueue__
#define __deadbeef__playqueue__

#include "playlist.h"

#ifdef __cplusplus
extern "C" {
#endif

int
playqueue_push (playItem_t *it);

void
playqueue_clear (void);

void
playqueue_pop (void);

void
playqueue_remove (playItem_t *it);

int
playqueue_test (playItem_t *it);

playItem_t *
playqueue_getnext (void);

int
playqueue_getcount (void);

playItem_t *
playqueue_get_item (int i);

void
playqueue_remove_nth (int n);

void
playqueue_insert_at (int n, playItem_t *it);

#ifdef __cplusplus
}
#endif

#endif /* defined(__deadbeef__playqueue__) */
