/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  routines for converting between wave formats and channel configurations

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

#ifndef __PREMIX_H
#define __PREMIX_H

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

// @returns number of output bytes
int
pcm_convert (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int inputsize);

#ifdef __cplusplus
}
#endif

#endif
