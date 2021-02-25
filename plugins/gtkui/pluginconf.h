/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#ifndef __PLUGINCONF_H
#define __PLUGINCONF_H

int
gtkui_run_dialog (GtkWidget *parentwin, ddb_dialog_t *conf, uint32_t buttons, int (*callback)(int button, void *ctx), void *ctx);

int
gtkui_run_dialog_root (ddb_dialog_t *conf, uint32_t buttons, int (*callback)(int button, void *ctx), void *ctx);

void
gtkui_make_dialog (GtkWidget *win, GtkWidget *containervbox, ddb_dialog_t *conf);

#endif
