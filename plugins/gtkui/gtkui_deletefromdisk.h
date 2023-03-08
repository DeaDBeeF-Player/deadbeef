/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#ifndef gtkui_deletefromdisk_h
#define gtkui_deletefromdisk_h

#include "../../src/shared/deletefromdisk.h"

void
gtkui_warning_message_for_ctx (ddbDeleteFromDiskController_t ctl, ddb_action_context_t ctx, unsigned trackcount, ddbDeleteFromDiskControllerWarningCallback_t callback);

int
gtkui_delete_file (ddbDeleteFromDiskController_t ctl, const char *uri);

#endif /* gtkui_deletefromdisk_h */
