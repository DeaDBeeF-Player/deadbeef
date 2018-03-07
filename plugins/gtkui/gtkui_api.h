/*
  gtkui_api.h -- API of the DeaDBeeF GTK UI plugin
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2013 Alexey Yakovenko

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

// gtkui.version_major=2 corresponds to deadbeef 0.6
// this is the version which has added design mode.
// it's guaranteed that the API will remain backwards compatible
// in minor releases (2.x)

// gtkui plugin id has been changed to gtkui_1, to avoid loading broken plugins.
// please DON'T simply patch your plugin to load gtkui_1 instead of gtkui.
// for information, about how to port your plugin to the new API correctly,
// and to learn more about design mode programming,
// please visit the following page:
// http://github.com/DeaDBeeF-Player/deadbeef/wiki/Porting-GUI-plugins-to-deadbeef-from-0.5.x-to-0.6.0

#if defined(GTK_CHECK_VERSION)
#if GTK_CHECK_VERSION(3,0,0)
#define DDB_GTKUI_PLUGIN_ID "gtkui3_1"
#else
#define DDB_GTKUI_PLUGIN_ID "gtkui_1"
#endif
#else
typedef void GtkWidget;
#endif

#define DDB_GTKUI_API_VERSION_MAJOR 2
#define DDB_GTKUI_API_VERSION_MINOR 3

#define DDB_GTKUI_DEPRECATED(x)

#ifdef __GNUC__
// avoid including glibc headers, this is not very portable
#if defined __GNUC__ && defined __GNUC_MINOR__
# define __GNUC_PREREQ(maj, min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define __GNUC_PREREQ(maj, min) 0
#endif
#undef DDB_GTKUI_DEPRECATED
#if __GNUC_PREREQ(4,5)
#define DDB_GTKUI_DEPRECATED(x) __attribute__ ((deprecated(x)))
#else
#define DDB_GTKUI_DEPRECATED(x) __attribute__ ((deprecated))
#endif
#endif

#ifndef DDB_GTKUI_API_LEVEL
#define DDB_GTKUI_API_LEVEL (DDB_GTKUI_API_VERSION_MAJOR * 100 + DB_API_VERSION_MINOR)
#endif

#if (DDB_WARN_DEPRECATED && DDB_GTKUI_API_LEVEL >= 202)
#define DEPRECATED_202 DDB_GTKUI_DEPRECATED("since GTKUI API 2.2")
#else
#define DEPRECATED_202
#endif

#if (DDB_GTKUI_API_LEVEL >= 201)
// added in API 2.1 (deadbeef-0.6.2)
#define DDB_GTKUI_CONF_LAYOUT "gtkui.layout.0.6.2"
#endif

// this flag tells that the widget should be added to h/vboxes with expand=FALSE
#define DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE 0x00000001

// widget config string must look like that:
// type key1=value1 key2=value2... { child widgets }
//
// the default widget loader will ignore all key-value pairs,
// so it's your custom loader's responsibility to handle them
// you can find out how to write custom loaders in gtkui sources,
// look e.g. for the "w_splitter_load"

typedef struct ddb_gtkui_widget_s {
    const char *type;

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

    // return the container widget of a composite widget
    // e.g. HBox is contained in EventBox, this function should return the HBox
    // the default implementation will always return the toplevel widget
    GtkWidget * (*get_container) (struct ddb_gtkui_widget_s *w);

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


// flags for passing to w_reg_widget

// tell the widget manager, that this widget can only have single instance
#define DDB_WF_SINGLE_INSTANCE 0x00000001

typedef struct {
    DB_gui_t gui;

    // returns main window ptr
    GtkWidget * (*get_mainwin) (void);

    // register new widget type;
    // type strings are passed at the end of argument list terminated with NULL
    // for example:
    // w_reg_widget("My Visualization", 0, my_viz_create, "my_viz_ng", "my_viz", NULL);
    // this call will register new type "my_viz_ng", with support for another
    // "my_viz" type string
    void (*w_reg_widget) (const char *title, uint32_t flags, ddb_gtkui_widget_t *(*create_func) (void), ...);

    // unregister existing widget type
    void (*w_unreg_widget) (const char *type);

    // this must be called from your <widget>_create for design mode support
    void (*w_override_signals) (GtkWidget *w, gpointer user_data);

    // returns 1 if a widget of specified type is registered
    int (*w_is_registered) (const char *type);

    // returns the toplevel widget
    ddb_gtkui_widget_t * (*w_get_rootwidget) (void);

    // enter/exit design mode
    void (*w_set_design_mode) (int active);

    // check whether we are in design mode
    int (*w_get_design_mode) (void);

    // create a widget of specified type
    ddb_gtkui_widget_t * (*w_create) (const char *type);

    // destroy the widget
    void (*w_destroy) (ddb_gtkui_widget_t *w);

    // append the widget to the container
    void (*w_append) (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

    // replace existing child widget in the container with another widget
    void (*w_replace) (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *from, ddb_gtkui_widget_t *to);

    // remove the widget from its container
    void (*w_remove) (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

    // return the container widget of a composite widget
    // e.g. HBox is contained in EventBox, this function should return the HBox
    // the default implementation will always return the toplevel widget
    GtkWidget * (*w_get_container) (ddb_gtkui_widget_t *w);

    // function to create the standard playlist context menu (the same as
    // appears when right-clicked on playlist tab)
    GtkWidget* (*create_pltmenu) (int plt_idx);

    // return a cover art pixbuf, if available.
    // if not available, the requested cover will be loaded asyncronously.
    // the callback will be called when the requested cover is available,
    // in which case you will need to call the get_cover_art_pixbuf again from
    // the callback.
    // get_cover_art_pixbuf is deprecated in API 2.2.
    // in new code, use get_cover_art_primary to get the large single cover art image,
    // and get_cover_art_thumb to get one of many smaller cover art images.
    GdkPixbuf *(*get_cover_art_pixbuf) (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data) DEPRECATED_202;

    // get_default_cover_pixbuf returns the default cover art image
    GdkPixbuf *(*cover_get_default_pixbuf) (void);

#if (DDB_GTKUI_API_LEVEL >= 202)
    // added in API 2.2 (deadbeef-0.7)
    GdkPixbuf *(*get_cover_art_primary) (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data);
    GdkPixbuf *(*get_cover_art_thumb) (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data);
    // adds a hook to be called before the main loop starts running, but after
    // the window was created.
    void (*add_window_init_hook) (void (*callback) (void *userdata), void *userdata);
#endif

#if (DDB_GTKUI_API_LEVEL >= 203)
    // Status icon plugin support functions
    void (*mainwin_toggle_visible) (void);
    void (*show_traymenu) (void);

    // Tell GTKUI that the standard status icon must be hidden, because another
    // plugin wants to make it in a different way
    void (*override_builtin_statusicon) (int override_);
#endif
#if (DDB_GTKUI_API_LEVEL >= 204)
    // copy and paste actions
    // plt_idx: the playlist to copy/cut from or paste to
    // ctx: DDB_ACTION_CTX_SELECTION to copy/cut/paste tracks, DDB_ACTION_CTX_PLAYLIST to copy/cut/paste a playlist
    void (*copy_selection) (ddb_playlist_t *plt, int ctx);
    void (*cut_selection) (ddb_playlist_t *plt, int ctx);
    void (*paste_selection) (ddb_playlist_t *plt, int ctx);
#endif
} ddb_gtkui_t;

#endif
