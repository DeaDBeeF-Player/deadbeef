/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  dsp preset manager

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
#ifndef __DSPPRESET_H
#define __DSPPRESET_H

void
dsp_preset_free (ddb_dsp_context_t *head);

int
dsp_preset_load (const char *fname, ddb_dsp_context_t **head);

int
dsp_preset_save (const char *path, ddb_dsp_context_t *head);

#endif
