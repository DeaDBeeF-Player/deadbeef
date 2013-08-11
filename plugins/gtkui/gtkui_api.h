/*
  gtkui_api.h -- API of the DeaDBeeF GTK UI plugin
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2012 Alexey Yakovenko

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

#define GTKUI_API_VERSION 0

#if GTK_CHECK_VERSION(3,0,0)
#define GTKUI_PLUGIN_ID "gtkui3_1"
#else
#define GTKUI_PLUGIN_ID "gtkui_1"
#endif

typedef struct ddb_gtkui_widget_s {
    const char *type;
    char *name;

    struct ddb_gtkui_widget_s *parent;

    GtkWidget *widget;
    
    uint32_t flags;

    // all the functions here are overloads, so they are not mandatory
    // they can be implemented to add custom code to normal widget code
    // they can be NULL if you don't need them, or you can set them to
    // standard functions (more below)

    // this function will be called after the widget is visible and needs to
    // [re]initialize itself
    // e.g. splitter widget sets the grip position in the init
    void (*init) (struct ddb_gtkui_widget_s *container);

    // save your custom parameters in the string using strncat
    // for example, if you need to write width and height:
    // strncat (s, "100 200", sz);
    void (*save) (struct ddb_gtkui_widget_s *w, char *s, int sz);

    // this is to read custom widget parameters, e.g. width and height;
    // you will be passed a string looking like "100 200 {"
    // you will need to read params, and return the new pointer, normally it
    // should be pointing to the "{"
    //
    // type string is necessary for backwards compatibility, so that load
    // function knows which type it's loading
    const char *(*load) (struct ddb_gtkui_widget_s *w, const char *type, const char *s);

    // custom destructor code
    void (*destroy) (struct ddb_gtkui_widget_s *w);

    // custom append code
    // if left NULL, appending will not be supported
    // you should use standard w_container_add if your widget is derived from
    // GTK_CONTAINER
    void (*append) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child);

    // custom remove code
    // you should use w_container_remove if your widget is derived from
    // GTK_CONTAINER
    void (*remove) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child);

    // custom replace code
    // default replace will call remove;destroy;append
    // but you can override if you need smarter behaviour
    // look at the splitter and tabs implementation for more details
    void (*replace) (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child, struct ddb_gtkui_widget_s *newchild);

    // implement this if you want to handle deadbeef broadcast messages/events
    int (*message) (struct ddb_gtkui_widget_s *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);

    // this will be called to setup the menu widget in design mode
    void (*initmenu) (struct ddb_gtkui_widget_s *w, GtkWidget *menu);

    // this will be called to setup the child menu widget in design mode
    // for example, to add "expand"/"fill" options for hbox/vbox children
    void (*initchildmenu) (struct ddb_gtkui_widget_s *w, GtkWidget *menu);

    // you shouldn't touch this list normally, the system takes care of it
    struct ddb_gtkui_widget_s *children;
    struct ddb_gtkui_widget_s *next; // points to next widget in the same container
} ddb_gtkui_widget_t;

typedef struct {
    DB_gui_t gui;

    // returns main window ptr
    GtkWidget * (*get_mainwin) (void);

    // register new widget type;
    // type strings are passed at the end of argument list terminated with NULL
    // for example:
    // w_reg_widget("My Visualization", my_viz_create, "my_viz_ng", "my_viz", NULL);
    // this call will register new type "my_viz_ng", with support for another
    // "my_viz" type string
    void (*w_reg_widget) (const char *title, ddb_gtkui_widget_t *(*create_func) (void), ...);

    // unregister existing widget type
    void (*w_unreg_widget) (const char *type);

    // this must be called from your <widget>_create for design mode support
    void (*w_override_signals) (GtkWidget *w, gpointer user_data);

    // returns 1 if a widget of specified type is registered
    int (*w_is_registered) (const char *type);

    // returns the toplevel widget
    ddb_gtkui_widget_t * (*w_get_rootwidget) (void);

    // create a widget of specified type
    ddb_gtkui_widget_t * (*w_create) (const char *type);

    // set widget name
    void (*w_set_name) (ddb_gtkui_widget_t *w, const char *name);

    // destroy the widget
    void (*w_destroy) (ddb_gtkui_widget_t *w);

    // append the widget to the container
    void (*w_append) (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

    // replace existing child widget in the container with another widget
    void (*w_replace) (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *from, ddb_gtkui_widget_t *to);

    // remove the widget from its container
    void (*w_remove) (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

    // function to create the standard playlist context menu (the same as
    // appears when right-clicked on playlist tab)
    GtkWidget* (*create_pltmenu) (int plt_idx);
} ddb_gtkui_t;
#endif
