/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2019 Oleksiy Yakovenko and other contributors

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
*/

#ifndef eqpreset_h
#define eqpreset_h

void
eq_preset_save (const char *fname);

int
eq_preset_load (const char *fname, float *preamp, float values[18]);

int
eq_preset_load_fb2k (const char *fname, float values[18]);

#endif /* eqpreset_h */
