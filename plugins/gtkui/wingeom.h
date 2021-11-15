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

#ifndef __WINGEOM_H
#define __WINGEOM_H

#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(4,0,0)
#include "gtk4compat.h"
#endif

void
wingeom_save (GtkWidget *widget, const char *name);

void
wingeom_save_max (GdkEventWindowState *event, GtkWidget *widget, const char *name);

void
wingeom_restore (GtkWidget *win, const char *name, int dx, int dy, int dw, int dh, int dmax);

#endif
