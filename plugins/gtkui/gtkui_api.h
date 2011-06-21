/*
  gtkui_api.h -- API of the DeaDBeeF GTK UI plugin
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2011 Alexey Yakovenko

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

  Note: DeaDBeeF player itself uses different license
*/
#ifndef __GTKUI_API_H
#define __GTKUI_API_H

#define GTKUI_API_VERSION 1 // for compile-time checking

typedef struct ddb_gtkui_widget_s {
    const char *type;
    
    struct ddb_gtkui_widget_s *parent;

    GtkWidget *widget;
    
    uint32_t flags;

    void (*destroy) (struct ddb_gtkui_widget_s *w);

    void (*append) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child);
    void (*remove) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child);
    void (*replace) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child, struct ddb_gtkui_widget_s *newchild);

    int (*message) (struct ddb_gtkui_widget_s *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);

    struct ddb_gtkui_widget_s *children;
    struct ddb_gtkui_widget_s *next; // points to next widget in the same container
} ddb_gtkui_widget_t;

typedef struct {
    DB_gui_t gui;
    int api_version;
    GtkWidget * (*get_mainwin) (void);
    void (*reg_widget) (const char *type, ddb_gtkui_widget_t *(*create_func) (void));
    void (*unreg_widget) (const char *type);
    ddb_gtkui_widget_t * (*get_root_widget) (void);
} ddb_gtkui_t;

#endif
