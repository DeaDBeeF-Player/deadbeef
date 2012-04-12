/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __GTKUIGL_H
#define __GTKUIGL_H

#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>
//#include <GL/glx.h>
//#include <GL/glxext.h>
//
//#ifndef GLX_SGI_swap_control
//typedef int ( * PFNGLXSWAPINTERVALSGIPROC) (int interval);
//#endif
//extern PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;

int
gtkui_gl_init (void);

#endif
