/*
  gtkui_api.h -- API of the DeaDBeeF GTK UI plugin
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2021 Oleksiy Yakovenko

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

#include <stdint.h>

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
#define DDB_GTKUI_API_VERSION_MINOR 6

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
#define DDB_GTKUI_API_LEVEL (DDB_GTKUI_API_VERSION_MAJOR * 100 + DDB_GTKUI_API_VERSION_MINOR)
#endif

#if (DDB_WARN_DEPRECATED && DDB_GTKUI_API_LEVEL >= 202)
#define DEPRECATED_202 DDB_GTKUI_DEPRECATED("since GTKUI API 2.2")
#else
#define DEPRECATED_202
#endif

#if (DDB_WARN_DEPRECATED && DDB_GTKUI_API_LEVEL >= 204)
#define DEPRECATED_204 DDB_GTKUI_DEPRECATED("since GTKUI API 2.4")
#else
#define DEPRECATED_204
#endif

#if (DDB_WARN_DEPRECATED && DDB_GTKUI_API_LEVEL >= 205)
#define DEPRECATED_205 DDB_GTKUI_DEPRECATED("since GTKUI API 2.5")
#else
#define DEPRECATED_205
#endif

// this flag tells that the widget should be added to h/vboxes with expand=FALSE
enum {
    DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE = 1<<0,
};

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
    // strncat (s, "x=100 y=200", sz);
    void (*save) (struct ddb_gtkui_widget_s *w, char *s, int sz) DEPRECATED_205;

    // this is to read custom widget parameters, e.g. width and height;
    // you will be passed a string looking like "x=100 y=200 {"
    // you will need to read params, and return the new pointer, normally it
    // should be pointing to the "{"
    //
    // type string is necessary for backwards compatibility, so that load
    // function knows which type it's loading
    const char *(*load) (struct ddb_gtkui_widget_s *w, const char *type, const char *s) DEPRECATED_205;

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

typedef struct {
    /// Size must be set to the size of this struct
    size_t _size;

    /// Load settings from a NULL-terminated list of interleaved key and value strings
    void (*deserialize_from_keyvalues)(ddb_gtkui_widget_t *widget, const char **keyvalues);

    /// Save settings to a NULL-terminated list of interleaved key and value strings.
    char const **(*serialize_to_keyvalues)(ddb_gtkui_widget_t *widget);

    /// Free the keyvalues list returned by @c serialize_to_keyvalues
    void (*free_serialized_keyvalues)(ddb_gtkui_widget_t *widget, char const **keyvalues);
} ddb_gtkui_widget_extended_api_t;

// flags for passing to w_reg_widget

enum {
    /// The widget can only have single instance
    DDB_WF_SINGLE_INSTANCE = 1<<0,
    /// This allows to use the ddb_gtkui_widget_extended_api_t API added in GTKUI 2.5.
    /// Such widget must provide the @c ddb_gtkui_widget_extended_api_t struct immediately after the @c ddb_gtkui_widget_t struct
    DDB_WF_SUPPORTS_EXTENDED_API = 1<<1,
};

typedef struct {
    DB_gui_t gui;

    // returns main window ptr
    GtkWidget * (*get_mainwin) (void);

    /// Register new widget type.
    /// @param title Display name of the widget, that will show up in the UI
    /// @param flags should be a combination of the DDB_WF_* values.
    /// @param create_func will be called to create the widget.
    ///
    /// The remaining arguments are a NULL-terminated list of strings, which determine the valid type names. At list one type name is required. This is the value that will be used for serialization.
    /// Example:
    /// w_reg_widget("My Visualization", 0, my_viz_create, "my_viz_ng", "my_viz", NULL);
    /// this call will register new type "my_viz_ng", with support for another
    /// "my_viz" type string
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

    /// Obsolete: returns NULL
    GdkPixbuf *(*get_cover_art_pixbuf) (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data);
    /// Obsolete: returns NULL
    GdkPixbuf *(*cover_get_default_pixbuf) (void);

#if (DDB_GTKUI_API_LEVEL >= 202)
    /// Obsolete: returns NULL
    GdkPixbuf *(*get_cover_art_primary) (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data);

    /// Obsolete: returns NULL
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
#if (DDB_GTKUI_API_LEVEL >= 205)
    /// Get flags passed to @c w_reg_widget for this type.
    uint32_t (*w_get_type_flags) (const char *type);
#endif
#if (DDB_GTKUI_API_LEVEL >= 206)
    /// Deserialize a widget from the config.
    /// Works similarly as the conf_get_* functions.
    /// Should be called from the main thread, and no need for conf_lock/conf_unlock.
    /// May return NULL on errors or if the key is not found in the config.
    ddb_gtkui_widget_t *(*w_load_layout_from_conf_key) (const char *key);

    /// Serialize a widget to the config.
    /// Works similarly as the conf_set_* functions.
    /// val must be non-NULL.
    /// Returns 0 if successful.
    int (*w_save_layout_to_conf_key) (const char *key, ddb_gtkui_widget_t *val);

    /// Sends a message to a widget and its children recursively.
    void (*w_send_message) (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);
#endif
} ddb_gtkui_t;

#endif
