/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GDK_WINDOW_H__
#define __GDK_WINDOW_H__

#include <gdk/gdkdrawable.h>
#include <gdk/gdktypes.h>
#include <gdk/gdkevents.h>

G_BEGIN_DECLS

typedef struct _GdkGeometry           GdkGeometry;
typedef struct _GdkWindowAttr	      GdkWindowAttr;
typedef struct _GdkPointerHooks	      GdkPointerHooks;

/* Classes of windows.
 *   InputOutput: Almost every window should be of this type. Such windows
 *		  receive events and are also displayed on screen.
 *   InputOnly: Used only in special circumstances when events need to be
 *		stolen from another window or windows. Input only windows
 *		have no visible output, so they are handy for placing over
 *		top of a group of windows in order to grab the events (or
 *		filter the events) from those windows.
 */
typedef enum
{
  GDK_INPUT_OUTPUT,
  GDK_INPUT_ONLY
} GdkWindowClass;

/* Types of windows.
 *   Root: There is only 1 root window and it is initialized
 *	   at startup. Creating a window of type GDK_WINDOW_ROOT
 *	   is an error.
 *   Toplevel: Windows which interact with the window manager.
 *   Child: Windows which are children of some other type of window.
 *	    (Any other type of window). Most windows are child windows.
 *   Dialog: A special kind of toplevel window which interacts with
 *	     the window manager slightly differently than a regular
 *	     toplevel window. Dialog windows should be used for any
 *	     transient window.
 *   Foreign: A window that actually belongs to another application
 */
typedef enum
{
  GDK_WINDOW_ROOT,
  GDK_WINDOW_TOPLEVEL,
  GDK_WINDOW_CHILD,
  GDK_WINDOW_DIALOG,
  GDK_WINDOW_TEMP,
  GDK_WINDOW_FOREIGN
} GdkWindowType;

/* Window attribute mask values.
 *   GDK_WA_TITLE: The "title" field is valid.
 *   GDK_WA_X: The "x" field is valid.
 *   GDK_WA_Y: The "y" field is valid.
 *   GDK_WA_CURSOR: The "cursor" field is valid.
 *   GDK_WA_COLORMAP: The "colormap" field is valid.
 *   GDK_WA_VISUAL: The "visual" field is valid.
 */
typedef enum
{
  GDK_WA_TITLE	   = 1 << 1,
  GDK_WA_X	   = 1 << 2,
  GDK_WA_Y	   = 1 << 3,
  GDK_WA_CURSOR	   = 1 << 4,
  GDK_WA_COLORMAP  = 1 << 5,
  GDK_WA_VISUAL	   = 1 << 6,
  GDK_WA_WMCLASS   = 1 << 7,
  GDK_WA_NOREDIR   = 1 << 8,
  GDK_WA_TYPE_HINT = 1 << 9
} GdkWindowAttributesType;

/* Size restriction enumeration.
 */
typedef enum
{
  GDK_HINT_POS	       = 1 << 0,
  GDK_HINT_MIN_SIZE    = 1 << 1,
  GDK_HINT_MAX_SIZE    = 1 << 2,
  GDK_HINT_BASE_SIZE   = 1 << 3,
  GDK_HINT_ASPECT      = 1 << 4,
  GDK_HINT_RESIZE_INC  = 1 << 5,
  GDK_HINT_WIN_GRAVITY = 1 << 6,
  GDK_HINT_USER_POS    = 1 << 7,
  GDK_HINT_USER_SIZE   = 1 << 8
} GdkWindowHints;


/* Window type hints.
 * These are hints for the window manager that indicate
 * what type of function the window has. The window manager
 * can use this when determining decoration and behaviour
 * of the window. The hint must be set before mapping the
 * window.
 *
 *   Normal: Normal toplevel window
 *   Dialog: Dialog window
 *   Menu: Window used to implement a menu.
 *   Toolbar: Window used to implement toolbars.
 */
typedef enum
{
  GDK_WINDOW_TYPE_HINT_NORMAL,
  GDK_WINDOW_TYPE_HINT_DIALOG,
  GDK_WINDOW_TYPE_HINT_MENU,		/* Torn off menu */
  GDK_WINDOW_TYPE_HINT_TOOLBAR,
  GDK_WINDOW_TYPE_HINT_SPLASHSCREEN,
  GDK_WINDOW_TYPE_HINT_UTILITY,
  GDK_WINDOW_TYPE_HINT_DOCK,
  GDK_WINDOW_TYPE_HINT_DESKTOP,
  GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU,	/* A drop down menu (from a menubar) */
  GDK_WINDOW_TYPE_HINT_POPUP_MENU,	/* A popup menu (from right-click) */
  GDK_WINDOW_TYPE_HINT_TOOLTIP,
  GDK_WINDOW_TYPE_HINT_NOTIFICATION,
  GDK_WINDOW_TYPE_HINT_COMBO,
  GDK_WINDOW_TYPE_HINT_DND
} GdkWindowTypeHint;

/* The next two enumeration values current match the
 * Motif constants. If this is changed, the implementation
 * of gdk_window_set_decorations/gdk_window_set_functions
 * will need to change as well.
 */
typedef enum
{
  GDK_DECOR_ALL		= 1 << 0,
  GDK_DECOR_BORDER	= 1 << 1,
  GDK_DECOR_RESIZEH	= 1 << 2,
  GDK_DECOR_TITLE	= 1 << 3,
  GDK_DECOR_MENU	= 1 << 4,
  GDK_DECOR_MINIMIZE	= 1 << 5,
  GDK_DECOR_MAXIMIZE	= 1 << 6
} GdkWMDecoration;

typedef enum
{
  GDK_FUNC_ALL		= 1 << 0,
  GDK_FUNC_RESIZE	= 1 << 1,
  GDK_FUNC_MOVE		= 1 << 2,
  GDK_FUNC_MINIMIZE	= 1 << 3,
  GDK_FUNC_MAXIMIZE	= 1 << 4,
  GDK_FUNC_CLOSE	= 1 << 5
} GdkWMFunction;

/* Currently, these are the same values numerically as in the
 * X protocol. If you change that, gdkwindow-x11.c/gdk_window_set_geometry_hints()
 * will need fixing.
 */
typedef enum
{
  GDK_GRAVITY_NORTH_WEST = 1,
  GDK_GRAVITY_NORTH,
  GDK_GRAVITY_NORTH_EAST,
  GDK_GRAVITY_WEST,
  GDK_GRAVITY_CENTER,
  GDK_GRAVITY_EAST,
  GDK_GRAVITY_SOUTH_WEST,
  GDK_GRAVITY_SOUTH,
  GDK_GRAVITY_SOUTH_EAST,
  GDK_GRAVITY_STATIC
} GdkGravity;


typedef enum
{
  GDK_WINDOW_EDGE_NORTH_WEST,
  GDK_WINDOW_EDGE_NORTH,
  GDK_WINDOW_EDGE_NORTH_EAST,
  GDK_WINDOW_EDGE_WEST,
  GDK_WINDOW_EDGE_EAST,
  GDK_WINDOW_EDGE_SOUTH_WEST,
  GDK_WINDOW_EDGE_SOUTH,
  GDK_WINDOW_EDGE_SOUTH_EAST  
} GdkWindowEdge;

struct _GdkWindowAttr
{
  gchar *title;
  gint event_mask;
  gint x, y;
  gint width;
  gint height;
  GdkWindowClass wclass;
  GdkVisual *visual;
  GdkColormap *colormap;
  GdkWindowType window_type;
  GdkCursor *cursor;
  gchar *wmclass_name;
  gchar *wmclass_class;
  gboolean override_redirect;
  GdkWindowTypeHint type_hint;
};

struct _GdkGeometry
{
  gint min_width;
  gint min_height;
  gint max_width;
  gint max_height;
  gint base_width;
  gint base_height;
  gint width_inc;
  gint height_inc;
  gdouble min_aspect;
  gdouble max_aspect;
  GdkGravity win_gravity;
};

struct _GdkPointerHooks 
{
  GdkWindow* (*get_pointer)       (GdkWindow	   *window,
			           gint	           *x,
			           gint   	   *y,
			           GdkModifierType *mask);
  GdkWindow* (*window_at_pointer) (GdkScreen       *screen, /* unused */
                                   gint            *win_x,
                                   gint            *win_y);
};

typedef struct _GdkWindowObject GdkWindowObject;
typedef struct _GdkWindowObjectClass GdkWindowObjectClass;

#define GDK_TYPE_WINDOW              (gdk_window_object_get_type ())
#define GDK_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WINDOW, GdkWindow))
#define GDK_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WINDOW, GdkWindowObjectClass))
#define GDK_IS_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WINDOW))
#define GDK_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WINDOW))
#define GDK_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_WINDOW, GdkWindowObjectClass))
#define GDK_WINDOW_OBJECT(object)    ((GdkWindowObject *) GDK_WINDOW (object))

struct _GdkWindowObject
{
  GdkDrawable parent_instance;

  GdkDrawable *impl; /* window-system-specific delegate object */  
  
  GdkWindowObject *parent;

  gpointer user_data;

  gint x;
  gint y;
  
  gint extension_events;

  GList *filters;
  GList *children;

  GdkColor bg_color;
  GdkPixmap *bg_pixmap;
  
  GSList *paint_stack;
  
  GdkRegion *update_area;
  guint update_freeze_count;
  
  guint8 window_type;
  guint8 depth;
  guint8 resize_count;

  GdkWindowState state;
  
  guint guffaw_gravity : 1;
  guint input_only : 1;
  guint modal_hint : 1;
  guint composited : 1;
  
  guint destroyed : 2;

  guint accept_focus : 1;
  guint focus_on_map : 1;
  guint shaped : 1;
  
  GdkEventMask event_mask;

  guint update_and_descendants_freeze_count;
};

struct _GdkWindowObjectClass
{
  GdkDrawableClass parent_class;
};

/* Windows
 */
GType         gdk_window_object_get_type       (void) G_GNUC_CONST;
GdkWindow*    gdk_window_new                   (GdkWindow     *parent,
                                                GdkWindowAttr *attributes,
                                                gint           attributes_mask);
void          gdk_window_destroy               (GdkWindow     *window);
GdkWindowType gdk_window_get_window_type       (GdkWindow     *window);
GdkWindow*    gdk_window_at_pointer            (gint          *win_x,
                                                gint          *win_y);
void          gdk_window_show                  (GdkWindow     *window);
void          gdk_window_hide                  (GdkWindow     *window);
void          gdk_window_withdraw              (GdkWindow     *window);
void          gdk_window_show_unraised         (GdkWindow     *window);
void          gdk_window_move                  (GdkWindow     *window,
                                                gint           x,
                                                gint           y);
void          gdk_window_resize                (GdkWindow     *window,
                                                gint           width,
                                                gint           height);
void          gdk_window_move_resize           (GdkWindow     *window,
                                                gint           x,
                                                gint           y,
                                                gint           width,
                                                gint           height);
void          gdk_window_reparent              (GdkWindow     *window,
                                                GdkWindow     *new_parent,
                                                gint           x,
                                                gint           y);
void          gdk_window_clear                 (GdkWindow     *window);
void          gdk_window_clear_area            (GdkWindow     *window,
                                                gint           x,
                                                gint           y,
                                                gint           width,
                                                gint           height);
void          gdk_window_clear_area_e          (GdkWindow     *window,
                                                gint           x,
                                                gint           y,
                                                gint           width,
                                                gint           height);
void          gdk_window_raise                 (GdkWindow     *window);
void          gdk_window_lower                 (GdkWindow     *window);
void          gdk_window_focus                 (GdkWindow     *window,
                                                guint32        timestamp);
void          gdk_window_set_user_data         (GdkWindow     *window,
                                                gpointer       user_data);
void          gdk_window_set_override_redirect (GdkWindow     *window,
                                                gboolean       override_redirect);
void          gdk_window_set_accept_focus      (GdkWindow     *window,
					        gboolean       accept_focus);
void          gdk_window_set_focus_on_map      (GdkWindow     *window,
					        gboolean       focus_on_map);
void          gdk_window_add_filter            (GdkWindow     *window,
                                                GdkFilterFunc  function,
                                                gpointer       data);
void          gdk_window_remove_filter         (GdkWindow     *window,
                                                GdkFilterFunc  function,
                                                gpointer       data);
void          gdk_window_scroll                (GdkWindow *window,
                                                gint       dx,
                                                gint       dy);
void	      gdk_window_move_region           (GdkWindow *window,
						GdkRegion *region,
						gint       dx,
						gint       dy);

/* 
 * This allows for making shaped (partially transparent) windows
 * - cool feature, needed for Drag and Drag for example.
 *  The shape_mask can be the mask
 *  from gdk_pixmap_create_from_xpm.   Stefan Wille
 */
void gdk_window_shape_combine_mask  (GdkWindow	    *window,
                                     GdkBitmap	    *mask,
                                     gint	     x,
                                     gint	     y);
void gdk_window_shape_combine_region (GdkWindow	    *window,
                                      GdkRegion     *shape_region,
                                      gint	     offset_x,
                                      gint	     offset_y);

/*
 * This routine allows you to quickly take the shapes of all the child windows
 * of a window and use their shapes as the shape mask for this window - useful
 * for container windows that dont want to look like a big box
 * 
 * - Raster
 */
void gdk_window_set_child_shapes (GdkWindow *window);

void gdk_window_set_composited   (GdkWindow *window,
                                  gboolean composited);

/*
 * This routine allows you to merge (ie ADD) child shapes to your
 * own window's shape keeping its current shape and ADDING the child
 * shapes to it.
 * 
 * - Raster
 */
void gdk_window_merge_child_shapes (GdkWindow *window);

void gdk_window_input_shape_combine_mask   (GdkWindow *window,
					    GdkBitmap *mask,
					    gint       x,
					    gint       y);
void gdk_window_input_shape_combine_region (GdkWindow *window,
                                            GdkRegion *shape_region,
                                            gint       offset_x,
                                            gint       offset_y);
void gdk_window_set_child_input_shapes     (GdkWindow *window);
void gdk_window_merge_child_input_shapes   (GdkWindow *window);


/*
 * Check if a window has been shown, and whether all its
 * parents up to a toplevel have been shown, respectively.
 * Note that a window that is_viewable below is not necessarily
 * viewable in the X sense.
 */
gboolean gdk_window_is_visible     (GdkWindow *window);
gboolean gdk_window_is_viewable    (GdkWindow *window);

GdkWindowState gdk_window_get_state (GdkWindow *window);

/* Set static bit gravity on the parent, and static
 * window gravity on all children.
 */
gboolean gdk_window_set_static_gravities (GdkWindow *window,
					  gboolean   use_static);   

/* Functions to create/lookup windows from their native equivalents */ 
#ifndef GDK_MULTIHEAD_SAFE
GdkWindow*    gdk_window_foreign_new (GdkNativeWindow anid);
GdkWindow*    gdk_window_lookup      (GdkNativeWindow anid);
#endif
GdkWindow    *gdk_window_foreign_new_for_display (GdkDisplay      *display,
						  GdkNativeWindow  anid);
GdkWindow*    gdk_window_lookup_for_display (GdkDisplay      *display,
					     GdkNativeWindow  anid);


/* GdkWindow */

#ifndef GDK_DISABLE_DEPRECATED
void	      gdk_window_set_hints	 (GdkWindow	  *window,
					  gint		   x,
					  gint		   y,
					  gint		   min_width,
					  gint		   min_height,
					  gint		   max_width,
					  gint		   max_height,
					  gint		   flags);
#endif
void              gdk_window_set_type_hint (GdkWindow        *window,
                                            GdkWindowTypeHint hint);
GdkWindowTypeHint gdk_window_get_type_hint (GdkWindow        *window);

void          gdk_window_set_modal_hint   (GdkWindow       *window,
                                           gboolean         modal);

void gdk_window_set_skip_taskbar_hint (GdkWindow *window,
                                       gboolean   skips_taskbar);
void gdk_window_set_skip_pager_hint   (GdkWindow *window,
                                       gboolean   skips_pager);
void gdk_window_set_urgency_hint      (GdkWindow *window,
				       gboolean   urgent);

void          gdk_window_set_geometry_hints (GdkWindow        *window,
					     GdkGeometry      *geometry,
					     GdkWindowHints    geom_mask);
void          gdk_set_sm_client_id         (const gchar *sm_client_id);

void	      gdk_window_begin_paint_rect   (GdkWindow    *window,
					     GdkRectangle *rectangle);
void	      gdk_window_begin_paint_region (GdkWindow    *window,
					     GdkRegion    *region);
void	      gdk_window_end_paint          (GdkWindow    *window);

void	      gdk_window_set_title	   (GdkWindow	  *window,
					    const gchar	  *title);
void          gdk_window_set_role          (GdkWindow       *window,
					    const gchar     *role);
void          gdk_window_set_startup_id    (GdkWindow       *window,
					    const gchar     *startup_id);					  
void          gdk_window_set_transient_for (GdkWindow       *window, 
					    GdkWindow       *parent);
void	      gdk_window_set_background	 (GdkWindow	  *window,
					  const GdkColor  *color);
void	      gdk_window_set_back_pixmap (GdkWindow	  *window,
					  GdkPixmap	  *pixmap,
					  gboolean	   parent_relative);
void	      gdk_window_set_cursor	 (GdkWindow	  *window,
					  GdkCursor	  *cursor);
void	      gdk_window_get_user_data	 (GdkWindow	  *window,
					  gpointer	  *data);
void	      gdk_window_get_geometry	 (GdkWindow	  *window,
					  gint		  *x,
					  gint		  *y,
					  gint		  *width,
					  gint		  *height,
					  gint		  *depth);
void	      gdk_window_get_position	 (GdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
gint	      gdk_window_get_origin	 (GdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);

#if !defined (GDK_DISABLE_DEPRECATED) || defined (GTK_COMPILATION)
/* Used by gtk_handle_box_button_changed () */
gboolean      gdk_window_get_deskrelative_origin (GdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
#endif

void	      gdk_window_get_root_origin (GdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
void          gdk_window_get_frame_extents (GdkWindow     *window,
                                            GdkRectangle  *rect);
GdkWindow*    gdk_window_get_pointer	 (GdkWindow	  *window,
					  gint		  *x,
					  gint		  *y,
					  GdkModifierType *mask);
GdkWindow *   gdk_window_get_parent      (GdkWindow       *window);
GdkWindow *   gdk_window_get_toplevel    (GdkWindow       *window);

GList *	      gdk_window_get_children	 (GdkWindow	  *window);
GList *       gdk_window_peek_children   (GdkWindow       *window);
GdkEventMask  gdk_window_get_events	 (GdkWindow	  *window);
void	      gdk_window_set_events	 (GdkWindow	  *window,
					  GdkEventMask	   event_mask);

void          gdk_window_set_icon_list   (GdkWindow       *window,
					  GList           *pixbufs);
void	      gdk_window_set_icon	 (GdkWindow	  *window, 
					  GdkWindow	  *icon_window,
					  GdkPixmap	  *pixmap,
					  GdkBitmap	  *mask);
void	      gdk_window_set_icon_name	 (GdkWindow	  *window, 
					  const gchar	  *name);
void	      gdk_window_set_group	 (GdkWindow	  *window, 
					  GdkWindow	  *leader);
GdkWindow*    gdk_window_get_group	 (GdkWindow	  *window);
void	      gdk_window_set_decorations (GdkWindow	  *window,
					  GdkWMDecoration  decorations);
gboolean      gdk_window_get_decorations (GdkWindow       *window,
					  GdkWMDecoration *decorations);
void	      gdk_window_set_functions	 (GdkWindow	  *window,
					  GdkWMFunction	   functions);
#ifndef GDK_MULTIHEAD_SAFE
GList *       gdk_window_get_toplevels   (void);
#endif

void          gdk_window_beep            (GdkWindow       *window);
void          gdk_window_iconify         (GdkWindow       *window);
void          gdk_window_deiconify       (GdkWindow       *window);
void          gdk_window_stick           (GdkWindow       *window);
void          gdk_window_unstick         (GdkWindow       *window);
void          gdk_window_maximize        (GdkWindow       *window);
void          gdk_window_unmaximize      (GdkWindow       *window);
void          gdk_window_fullscreen      (GdkWindow       *window);
void          gdk_window_unfullscreen    (GdkWindow       *window);
void          gdk_window_set_keep_above  (GdkWindow       *window,
                                          gboolean         setting);
void          gdk_window_set_keep_below  (GdkWindow       *window,
                                          gboolean         setting);
void          gdk_window_set_opacity     (GdkWindow       *window,
                                          gdouble          opacity);
void          gdk_window_register_dnd    (GdkWindow       *window);

void gdk_window_begin_resize_drag (GdkWindow     *window,
                                   GdkWindowEdge  edge,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);
void gdk_window_begin_move_drag   (GdkWindow     *window,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);

/* Interface for dirty-region queueing */
void       gdk_window_invalidate_rect           (GdkWindow    *window,
					         GdkRectangle *rect,
					         gboolean      invalidate_children);
void       gdk_window_invalidate_region         (GdkWindow    *window,
					         GdkRegion    *region,
					         gboolean      invalidate_children);
void       gdk_window_invalidate_maybe_recurse  (GdkWindow *window,
						 GdkRegion *region,
						 gboolean (*child_func) (GdkWindow *, gpointer),
						 gpointer   user_data);
GdkRegion *gdk_window_get_update_area     (GdkWindow    *window);

void       gdk_window_freeze_updates      (GdkWindow    *window);
void       gdk_window_thaw_updates        (GdkWindow    *window);

void       gdk_window_freeze_toplevel_updates_libgtk_only (GdkWindow *window);
void       gdk_window_thaw_toplevel_updates_libgtk_only   (GdkWindow *window);

void       gdk_window_process_all_updates (void);
void       gdk_window_process_updates     (GdkWindow    *window,
					   gboolean      update_children);

/* Enable/disable flicker, so you can tell if your code is inefficient. */
void       gdk_window_set_debug_updates   (gboolean      setting);

void       gdk_window_constrain_size      (GdkGeometry  *geometry,
                                           guint         flags,
                                           gint          width,
                                           gint          height,
                                           gint         *new_width,
                                           gint         *new_height);

void gdk_window_get_internal_paint_info (GdkWindow    *window,
					 GdkDrawable **real_drawable,
					 gint         *x_offset,
					 gint         *y_offset);

void gdk_window_enable_synchronized_configure (GdkWindow *window);
void gdk_window_configure_finished            (GdkWindow *window);

#ifndef GDK_MULTIHEAD_SAFE
GdkPointerHooks *gdk_set_pointer_hooks (const GdkPointerHooks *new_hooks);   
#endif /* GDK_MULTIHEAD_SAFE */

GdkWindow *gdk_get_default_root_window (void);

#ifndef GDK_DISABLE_DEPRECATED
#define GDK_ROOT_PARENT()             (gdk_get_default_root_window ())
#define gdk_window_get_size            gdk_drawable_get_size
#define gdk_window_get_type            gdk_window_get_window_type
#define gdk_window_get_colormap        gdk_drawable_get_colormap
#define gdk_window_set_colormap        gdk_drawable_set_colormap
#define gdk_window_get_visual          gdk_drawable_get_visual
#define gdk_window_ref                 gdk_drawable_ref
#define gdk_window_unref               gdk_drawable_unref

#define gdk_window_copy_area(drawable,gc,x,y,source_drawable,source_x,source_y,width,height) \
   gdk_draw_pixmap(drawable,gc,source_drawable,source_x,source_y,x,y,width,height)
#endif /* GDK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __GDK_WINDOW_H__ */
