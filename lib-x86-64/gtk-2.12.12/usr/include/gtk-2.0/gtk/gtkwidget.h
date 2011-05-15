/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __GTK_WIDGET_H__
#define __GTK_WIDGET_H__

#include <gdk/gdk.h>
#include <gtk/gtkaccelgroup.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkstyle.h>
#include <gtk/gtksettings.h>
#include <atk/atkobject.h>

G_BEGIN_DECLS

/* The flags that are used by GtkWidget on top of the
 * flags field of GtkObject.
 */
typedef enum
{
  GTK_TOPLEVEL         = 1 << 4,
  GTK_NO_WINDOW        = 1 << 5,
  GTK_REALIZED         = 1 << 6,
  GTK_MAPPED           = 1 << 7,
  GTK_VISIBLE          = 1 << 8,
  GTK_SENSITIVE        = 1 << 9,
  GTK_PARENT_SENSITIVE = 1 << 10,
  GTK_CAN_FOCUS        = 1 << 11,
  GTK_HAS_FOCUS        = 1 << 12,

  /* widget is allowed to receive the default via gtk_widget_grab_default
   * and will reserve space to draw the default if possible
   */
  GTK_CAN_DEFAULT      = 1 << 13,

  /* the widget currently is receiving the default action and should be drawn
   * appropriately if possible
   */
  GTK_HAS_DEFAULT      = 1 << 14,

  GTK_HAS_GRAB	       = 1 << 15,
  GTK_RC_STYLE	       = 1 << 16,
  GTK_COMPOSITE_CHILD  = 1 << 17,
  GTK_NO_REPARENT      = 1 << 18,
  GTK_APP_PAINTABLE    = 1 << 19,

  /* the widget when focused will receive the default action and have
   * HAS_DEFAULT set even if there is a different widget set as default
   */
  GTK_RECEIVES_DEFAULT = 1 << 20,

  GTK_DOUBLE_BUFFERED  = 1 << 21,
  GTK_NO_SHOW_ALL      = 1 << 22
} GtkWidgetFlags;

/* Kinds of widget-specific help */
typedef enum
{
  GTK_WIDGET_HELP_TOOLTIP,
  GTK_WIDGET_HELP_WHATS_THIS
} GtkWidgetHelpType;

/* Macro for casting a pointer to a GtkWidget or GtkWidgetClass pointer.
 * Macros for testing whether `widget' or `klass' are of type GTK_TYPE_WIDGET.
 */
#define GTK_TYPE_WIDGET			  (gtk_widget_get_type ())
#define GTK_WIDGET(widget)		  (G_TYPE_CHECK_INSTANCE_CAST ((widget), GTK_TYPE_WIDGET, GtkWidget))
#define GTK_WIDGET_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_WIDGET, GtkWidgetClass))
#define GTK_IS_WIDGET(widget)		  (G_TYPE_CHECK_INSTANCE_TYPE ((widget), GTK_TYPE_WIDGET))
#define GTK_IS_WIDGET_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_WIDGET))
#define GTK_WIDGET_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_WIDGET, GtkWidgetClass))

/* Macros for extracting various fields from GtkWidget and GtkWidgetClass.
 */
#define GTK_WIDGET_TYPE(wid)		  (GTK_OBJECT_TYPE (wid))
#define GTK_WIDGET_STATE(wid)		  (GTK_WIDGET (wid)->state)
#define GTK_WIDGET_SAVED_STATE(wid)	  (GTK_WIDGET (wid)->saved_state)

/* Macros for extracting the widget flags from GtkWidget.
 */
#define GTK_WIDGET_FLAGS(wid)		  (GTK_OBJECT_FLAGS (wid))
#define GTK_WIDGET_TOPLEVEL(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_TOPLEVEL) != 0)
#define GTK_WIDGET_NO_WINDOW(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_NO_WINDOW) != 0)
#define GTK_WIDGET_REALIZED(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_REALIZED) != 0)
#define GTK_WIDGET_MAPPED(wid)		  ((GTK_WIDGET_FLAGS (wid) & GTK_MAPPED) != 0)
#define GTK_WIDGET_VISIBLE(wid)		  ((GTK_WIDGET_FLAGS (wid) & GTK_VISIBLE) != 0)
#define GTK_WIDGET_DRAWABLE(wid)	  (GTK_WIDGET_VISIBLE (wid) && GTK_WIDGET_MAPPED (wid))
#define GTK_WIDGET_SENSITIVE(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_SENSITIVE) != 0)
#define GTK_WIDGET_PARENT_SENSITIVE(wid)  ((GTK_WIDGET_FLAGS (wid) & GTK_PARENT_SENSITIVE) != 0)
#define GTK_WIDGET_IS_SENSITIVE(wid)	  (GTK_WIDGET_SENSITIVE (wid) && \
					   GTK_WIDGET_PARENT_SENSITIVE (wid))
#define GTK_WIDGET_CAN_FOCUS(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_CAN_FOCUS) != 0)
#define GTK_WIDGET_HAS_FOCUS(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_HAS_FOCUS) != 0)
#define GTK_WIDGET_CAN_DEFAULT(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_CAN_DEFAULT) != 0)
#define GTK_WIDGET_HAS_DEFAULT(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_HAS_DEFAULT) != 0)
#define GTK_WIDGET_HAS_GRAB(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_HAS_GRAB) != 0)
#define GTK_WIDGET_RC_STYLE(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_RC_STYLE) != 0)
#define GTK_WIDGET_COMPOSITE_CHILD(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_COMPOSITE_CHILD) != 0)
#define GTK_WIDGET_APP_PAINTABLE(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_APP_PAINTABLE) != 0)
#define GTK_WIDGET_RECEIVES_DEFAULT(wid)  ((GTK_WIDGET_FLAGS (wid) & GTK_RECEIVES_DEFAULT) != 0)
#define GTK_WIDGET_DOUBLE_BUFFERED(wid)	  ((GTK_WIDGET_FLAGS (wid) & GTK_DOUBLE_BUFFERED) != 0)
  
/* Macros for setting and clearing widget flags.
 */
#define GTK_WIDGET_SET_FLAGS(wid,flag)	  G_STMT_START{ (GTK_WIDGET_FLAGS (wid) |= (flag)); }G_STMT_END
#define GTK_WIDGET_UNSET_FLAGS(wid,flag)  G_STMT_START{ (GTK_WIDGET_FLAGS (wid) &= ~(flag)); }G_STMT_END

#define GTK_TYPE_REQUISITION              (gtk_requisition_get_type ())

/* forward declaration to avoid excessive includes (and concurrent includes)
 */
typedef struct _GtkRequisition	   GtkRequisition;
typedef 	GdkRectangle	   GtkAllocation;
typedef struct _GtkSelectionData   GtkSelectionData;
typedef struct _GtkWidgetClass	   GtkWidgetClass;
typedef struct _GtkWidgetAuxInfo   GtkWidgetAuxInfo;
typedef struct _GtkWidgetShapeInfo GtkWidgetShapeInfo;
typedef struct _GtkClipboard	   GtkClipboard;
typedef struct _GtkTooltip         GtkTooltip;
typedef struct _GtkWindow          GtkWindow;
typedef void     (*GtkCallback)        (GtkWidget        *widget,
					gpointer	  data);

/* A requisition is a desired amount of space which a
 *  widget may request.
 */
struct _GtkRequisition
{
  gint width;
  gint height;
};

/* The widget is the base of the tree for displayable objects.
 *  (A displayable object is one which takes up some amount
 *  of screen real estate). It provides a common base and interface
 *  which actual widgets must adhere to.
 */
struct _GtkWidget
{
  /* The object structure needs to be the first
   *  element in the widget structure in order for
   *  the object mechanism to work correctly. This
   *  allows a GtkWidget pointer to be cast to a
   *  GtkObject pointer.
   */
  GtkObject object;
  
  /* 16 bits of internally used private flags.
   * this will be packed into the same 4 byte alignment frame that
   * state and saved_state go. we therefore don't waste any new
   * space on this.
   */
  guint16 private_flags;
  
  /* The state of the widget. There are actually only
   *  5 widget states (defined in "gtkenums.h").
   */
  guint8 state;
  
  /* The saved state of the widget. When a widget's state
   *  is changed to GTK_STATE_INSENSITIVE via
   *  "gtk_widget_set_state" or "gtk_widget_set_sensitive"
   *  the old state is kept around in this field. The state
   *  will be restored once the widget gets sensitive again.
   */
  guint8 saved_state;
  
  /* The widget's name. If the widget does not have a name
   *  (the name is NULL), then its name (as returned by
   *  "gtk_widget_get_name") is its class's name.
   * Among other things, the widget name is used to determine
   *  the style to use for a widget.
   */
  gchar *name;
  
  /*< public >*/

  /* The style for the widget. The style contains the
   *  colors the widget should be drawn in for each state
   *  along with graphics contexts used to draw with and
   *  the font to use for text.
   */
  GtkStyle *style;
  
  /* The widget's desired size.
   */
  GtkRequisition requisition;
  
  /* The widget's allocated size.
   */
  GtkAllocation allocation;
  
  /* The widget's window or its parent window if it does
   *  not have a window. (Which will be indicated by the
   *  GTK_NO_WINDOW flag being set).
   */
  GdkWindow *window;
  
  /* The widget's parent.
   */
  GtkWidget *parent;
};

struct _GtkWidgetClass
{
  /* The object class structure needs to be the first
   *  element in the widget class structure in order for
   *  the class mechanism to work correctly. This allows a
   *  GtkWidgetClass pointer to be cast to a GtkObjectClass
   *  pointer.
   */
  GtkObjectClass parent_class;

  /*< public >*/
  
  guint activate_signal;

  guint set_scroll_adjustments_signal;

  /*< private >*/
  
  /* seldomly overidden */
  void (*dispatch_child_properties_changed) (GtkWidget   *widget,
					     guint        n_pspecs,
					     GParamSpec **pspecs);

  /* basics */
  void (* show)		       (GtkWidget        *widget);
  void (* show_all)            (GtkWidget        *widget);
  void (* hide)		       (GtkWidget        *widget);
  void (* hide_all)            (GtkWidget        *widget);
  void (* map)		       (GtkWidget        *widget);
  void (* unmap)	       (GtkWidget        *widget);
  void (* realize)	       (GtkWidget        *widget);
  void (* unrealize)	       (GtkWidget        *widget);
  void (* size_request)	       (GtkWidget        *widget,
				GtkRequisition   *requisition);
  void (* size_allocate)       (GtkWidget        *widget,
				GtkAllocation    *allocation);
  void (* state_changed)       (GtkWidget        *widget,
				GtkStateType   	  previous_state);
  void (* parent_set)	       (GtkWidget        *widget,
				GtkWidget        *previous_parent);
  void (* hierarchy_changed)   (GtkWidget        *widget,
				GtkWidget        *previous_toplevel);
  void (* style_set)	       (GtkWidget        *widget,
				GtkStyle         *previous_style);
  void (* direction_changed)   (GtkWidget        *widget,
				GtkTextDirection  previous_direction);
  void (* grab_notify)         (GtkWidget        *widget,
				gboolean          was_grabbed);
  void (* child_notify)        (GtkWidget	 *widget,
				GParamSpec       *pspec);
  
  /* Mnemonics */
  gboolean (* mnemonic_activate) (GtkWidget    *widget,
				  gboolean      group_cycling);
  
  /* explicit focus */
  void     (* grab_focus)      (GtkWidget        *widget);
  gboolean (* focus)           (GtkWidget        *widget,
                                GtkDirectionType  direction);
  
  /* events */
  gboolean (* event)			(GtkWidget	     *widget,
					 GdkEvent	     *event);
  gboolean (* button_press_event)	(GtkWidget	     *widget,
					 GdkEventButton      *event);
  gboolean (* button_release_event)	(GtkWidget	     *widget,
					 GdkEventButton      *event);
  gboolean (* scroll_event)		(GtkWidget           *widget,
					 GdkEventScroll      *event);
  gboolean (* motion_notify_event)	(GtkWidget	     *widget,
					 GdkEventMotion      *event);
  gboolean (* delete_event)		(GtkWidget	     *widget,
					 GdkEventAny	     *event);
  gboolean (* destroy_event)		(GtkWidget	     *widget,
					 GdkEventAny	     *event);
  gboolean (* expose_event)		(GtkWidget	     *widget,
					 GdkEventExpose      *event);
  gboolean (* key_press_event)		(GtkWidget	     *widget,
					 GdkEventKey	     *event);
  gboolean (* key_release_event)	(GtkWidget	     *widget,
					 GdkEventKey	     *event);
  gboolean (* enter_notify_event)	(GtkWidget	     *widget,
					 GdkEventCrossing    *event);
  gboolean (* leave_notify_event)	(GtkWidget	     *widget,
					 GdkEventCrossing    *event);
  gboolean (* configure_event)		(GtkWidget	     *widget,
					 GdkEventConfigure   *event);
  gboolean (* focus_in_event)		(GtkWidget	     *widget,
					 GdkEventFocus       *event);
  gboolean (* focus_out_event)		(GtkWidget	     *widget,
					 GdkEventFocus       *event);
  gboolean (* map_event)		(GtkWidget	     *widget,
					 GdkEventAny	     *event);
  gboolean (* unmap_event)		(GtkWidget	     *widget,
					 GdkEventAny	     *event);
  gboolean (* property_notify_event)	(GtkWidget	     *widget,
					 GdkEventProperty    *event);
  gboolean (* selection_clear_event)	(GtkWidget	     *widget,
					 GdkEventSelection   *event);
  gboolean (* selection_request_event)	(GtkWidget	     *widget,
					 GdkEventSelection   *event);
  gboolean (* selection_notify_event)	(GtkWidget	     *widget,
					 GdkEventSelection   *event);
  gboolean (* proximity_in_event)	(GtkWidget	     *widget,
					 GdkEventProximity   *event);
  gboolean (* proximity_out_event)	(GtkWidget	     *widget,
					 GdkEventProximity   *event);
  gboolean (* visibility_notify_event)	(GtkWidget	     *widget,
					 GdkEventVisibility  *event);
  gboolean (* client_event)		(GtkWidget	     *widget,
					 GdkEventClient	     *event);
  gboolean (* no_expose_event)		(GtkWidget	     *widget,
					 GdkEventAny	     *event);
  gboolean (* window_state_event)	(GtkWidget	     *widget,
					 GdkEventWindowState *event);
  
  /* selection */
  void (* selection_get)           (GtkWidget          *widget,
				    GtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);
  void (* selection_received)      (GtkWidget          *widget,
				    GtkSelectionData   *selection_data,
				    guint               time_);

  /* Source side drag signals */
  void (* drag_begin)	           (GtkWidget	       *widget,
				    GdkDragContext     *context);
  void (* drag_end)	           (GtkWidget	       *widget,
				    GdkDragContext     *context);
  void (* drag_data_get)           (GtkWidget          *widget,
				    GdkDragContext     *context,
				    GtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);
  void (* drag_data_delete)        (GtkWidget	       *widget,
				    GdkDragContext     *context);

  /* Target side drag signals */
  void (* drag_leave)	           (GtkWidget	       *widget,
				    GdkDragContext     *context,
				    guint               time_);
  gboolean (* drag_motion)         (GtkWidget	       *widget,
				    GdkDragContext     *context,
				    gint                x,
				    gint                y,
				    guint               time_);
  gboolean (* drag_drop)           (GtkWidget	       *widget,
				    GdkDragContext     *context,
				    gint                x,
				    gint                y,
				    guint               time_);
  void (* drag_data_received)      (GtkWidget          *widget,
				    GdkDragContext     *context,
				    gint                x,
				    gint                y,
				    GtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);

  /* Signals used only for keybindings */
  gboolean (* popup_menu)          (GtkWidget          *widget);

  /* If a widget has multiple tooltips/whatsthis, it should show the
   * one for the current focus location, or if that doesn't make
   * sense, should cycle through them showing each tip alongside
   * whatever piece of the widget it applies to.
   */
  gboolean (* show_help)           (GtkWidget          *widget,
                                    GtkWidgetHelpType   help_type);
  
  /* accessibility support 
   */
  AtkObject*   (*get_accessible)     (GtkWidget *widget);

  void         (*screen_changed)     (GtkWidget *widget,
                                      GdkScreen *previous_screen);
  gboolean     (*can_activate_accel) (GtkWidget *widget,
                                      guint      signal_id);

  /* Sent when a grab is broken. */
  gboolean (*grab_broken_event) (GtkWidget	     *widget,
                                 GdkEventGrabBroken  *event);

  void         (* composited_changed) (GtkWidget *widget);

  gboolean     (* query_tooltip)      (GtkWidget  *widget,
				       gint        x,
				       gint        y,
				       gboolean    keyboard_tooltip,
				       GtkTooltip *tooltip);

  /* Padding for future expansion */
  void (*_gtk_reserved5) (void);
  void (*_gtk_reserved6) (void);
  void (*_gtk_reserved7) (void);
};

struct _GtkWidgetAuxInfo
{
  gint x;
  gint y;
  gint width;
  gint height;
  guint x_set : 1;
  guint y_set : 1;
};

struct _GtkWidgetShapeInfo
{
  gint16     offset_x;
  gint16     offset_y;
  GdkBitmap *shape_mask;
};

GType	   gtk_widget_get_type		  (void) G_GNUC_CONST;
GtkWidget* gtk_widget_new		  (GType		type,
					   const gchar	       *first_property_name,
					   ...);
void	   gtk_widget_destroy		  (GtkWidget	       *widget);
void	   gtk_widget_destroyed		  (GtkWidget	       *widget,
					   GtkWidget	      **widget_pointer);
#ifndef GTK_DISABLE_DEPRECATED
GtkWidget* gtk_widget_ref		  (GtkWidget	       *widget);
void	   gtk_widget_unref		  (GtkWidget	       *widget);
void	   gtk_widget_set		  (GtkWidget	       *widget,
					   const gchar         *first_property_name,
					   ...) G_GNUC_NULL_TERMINATED;
#endif /* GTK_DISABLE_DEPRECATED */
void	   gtk_widget_unparent		  (GtkWidget	       *widget);
void	   gtk_widget_show		  (GtkWidget	       *widget);
void       gtk_widget_show_now            (GtkWidget           *widget);
void	   gtk_widget_hide		  (GtkWidget	       *widget);
void	   gtk_widget_show_all		  (GtkWidget	       *widget);
void	   gtk_widget_hide_all		  (GtkWidget	       *widget);
void       gtk_widget_set_no_show_all     (GtkWidget           *widget,
					   gboolean             no_show_all);
gboolean   gtk_widget_get_no_show_all     (GtkWidget           *widget);
void	   gtk_widget_map		  (GtkWidget	       *widget);
void	   gtk_widget_unmap		  (GtkWidget	       *widget);
void	   gtk_widget_realize		  (GtkWidget	       *widget);
void	   gtk_widget_unrealize		  (GtkWidget	       *widget);

/* Queuing draws */
void	   gtk_widget_queue_draw	  (GtkWidget	       *widget);
void	   gtk_widget_queue_draw_area	  (GtkWidget	       *widget,
					   gint                 x,
					   gint                 y,
					   gint                 width,
					   gint                 height);
#ifndef GTK_DISABLE_DEPRECATED
void	   gtk_widget_queue_clear	  (GtkWidget	       *widget);
void	   gtk_widget_queue_clear_area	  (GtkWidget	       *widget,
					   gint                 x,
					   gint                 y,
					   gint                 width,
					   gint                 height);
#endif /* GTK_DISABLE_DEPRECATED */


void	   gtk_widget_queue_resize	  (GtkWidget	       *widget);
void	   gtk_widget_queue_resize_no_redraw (GtkWidget *widget);
#ifndef GTK_DISABLE_DEPRECATED
void	   gtk_widget_draw		  (GtkWidget	       *widget,
					   GdkRectangle	       *area);
#endif /* GTK_DISABLE_DEPRECATED */
void	   gtk_widget_size_request	  (GtkWidget	       *widget,
					   GtkRequisition      *requisition);
void	   gtk_widget_size_allocate	  (GtkWidget	       *widget,
					   GtkAllocation       *allocation);
void       gtk_widget_get_child_requisition (GtkWidget	       *widget,
					     GtkRequisition    *requisition);
void	   gtk_widget_add_accelerator	  (GtkWidget           *widget,
					   const gchar         *accel_signal,
					   GtkAccelGroup       *accel_group,
					   guint                accel_key,
					   GdkModifierType      accel_mods,
					   GtkAccelFlags        accel_flags);
gboolean   gtk_widget_remove_accelerator  (GtkWidget           *widget,
					   GtkAccelGroup       *accel_group,
					   guint                accel_key,
					   GdkModifierType      accel_mods);
void       gtk_widget_set_accel_path      (GtkWidget           *widget,
					   const gchar         *accel_path,
					   GtkAccelGroup       *accel_group);
const gchar* _gtk_widget_get_accel_path   (GtkWidget           *widget,
					   gboolean	       *locked);
GList*     gtk_widget_list_accel_closures (GtkWidget	       *widget);
gboolean   gtk_widget_can_activate_accel  (GtkWidget           *widget,
                                           guint                signal_id);
gboolean   gtk_widget_mnemonic_activate   (GtkWidget           *widget,
					   gboolean             group_cycling);
gboolean   gtk_widget_event		  (GtkWidget	       *widget,
					   GdkEvent	       *event);
gint       gtk_widget_send_expose         (GtkWidget           *widget,
					   GdkEvent            *event);

gboolean   gtk_widget_activate		     (GtkWidget	       *widget);
gboolean   gtk_widget_set_scroll_adjustments (GtkWidget        *widget,
					      GtkAdjustment    *hadjustment,
					      GtkAdjustment    *vadjustment);
     
void	   gtk_widget_reparent		  (GtkWidget	       *widget,
					   GtkWidget	       *new_parent);
gboolean   gtk_widget_intersect		  (GtkWidget	       *widget,
					   GdkRectangle	       *area,
					   GdkRectangle	       *intersection);
GdkRegion *gtk_widget_region_intersect	  (GtkWidget	       *widget,
					   GdkRegion	       *region);

void	gtk_widget_freeze_child_notify	  (GtkWidget	       *widget);
void	gtk_widget_child_notify		  (GtkWidget	       *widget,
					   const gchar	       *child_property);
void	gtk_widget_thaw_child_notify	  (GtkWidget	       *widget);

gboolean   gtk_widget_is_focus            (GtkWidget           *widget);
void	   gtk_widget_grab_focus	  (GtkWidget	       *widget);
void	   gtk_widget_grab_default	  (GtkWidget	       *widget);

void                  gtk_widget_set_name               (GtkWidget    *widget,
							 const gchar  *name);
G_CONST_RETURN gchar* gtk_widget_get_name               (GtkWidget    *widget);
void                  gtk_widget_set_state              (GtkWidget    *widget,
							 GtkStateType  state);
void                  gtk_widget_set_sensitive          (GtkWidget    *widget,
							 gboolean      sensitive);
void                  gtk_widget_set_app_paintable      (GtkWidget    *widget,
							 gboolean      app_paintable);
void                  gtk_widget_set_double_buffered    (GtkWidget    *widget,
							 gboolean      double_buffered);
void                  gtk_widget_set_redraw_on_allocate (GtkWidget    *widget,
							 gboolean      redraw_on_allocate);
void                  gtk_widget_set_parent             (GtkWidget    *widget,
							 GtkWidget    *parent);
void                  gtk_widget_set_parent_window      (GtkWidget    *widget,
							 GdkWindow    *parent_window);
void                  gtk_widget_set_child_visible      (GtkWidget    *widget,
							 gboolean      is_visible);
gboolean              gtk_widget_get_child_visible      (GtkWidget    *widget);

GtkWidget *gtk_widget_get_parent          (GtkWidget           *widget);
GdkWindow *gtk_widget_get_parent_window	  (GtkWidget	       *widget);

gboolean   gtk_widget_child_focus         (GtkWidget           *widget,
                                           GtkDirectionType     direction);
gboolean   gtk_widget_keynav_failed       (GtkWidget           *widget,
                                           GtkDirectionType     direction);
void       gtk_widget_error_bell          (GtkWidget           *widget);

void       gtk_widget_set_size_request    (GtkWidget           *widget,
                                           gint                 width,
                                           gint                 height);
void       gtk_widget_get_size_request    (GtkWidget           *widget,
                                           gint                *width,
                                           gint                *height);
#ifndef GTK_DISABLE_DEPRECATED
void	   gtk_widget_set_uposition	  (GtkWidget	       *widget,
					   gint			x,
					   gint			y);
void	   gtk_widget_set_usize		  (GtkWidget	       *widget,
					   gint			width,
					   gint			height);
#endif

void	   gtk_widget_set_events	  (GtkWidget	       *widget,
					   gint			events);
void       gtk_widget_add_events          (GtkWidget           *widget,
					   gint	                events);
void	   gtk_widget_set_extension_events (GtkWidget		*widget,
					    GdkExtensionMode	mode);

GdkExtensionMode gtk_widget_get_extension_events (GtkWidget	*widget);
GtkWidget*   gtk_widget_get_toplevel	(GtkWidget	*widget);
GtkWidget*   gtk_widget_get_ancestor	(GtkWidget	*widget,
					 GType		 widget_type);
GdkColormap* gtk_widget_get_colormap	(GtkWidget	*widget);
GdkVisual*   gtk_widget_get_visual	(GtkWidget	*widget);

GdkScreen *   gtk_widget_get_screen      (GtkWidget *widget);
gboolean      gtk_widget_has_screen      (GtkWidget *widget);
GdkDisplay *  gtk_widget_get_display     (GtkWidget *widget);
GdkWindow *   gtk_widget_get_root_window (GtkWidget *widget);
GtkSettings*  gtk_widget_get_settings    (GtkWidget *widget);
GtkClipboard *gtk_widget_get_clipboard   (GtkWidget *widget,
					  GdkAtom    selection);

#ifndef GTK_DISABLE_DEPRECATED
#define gtk_widget_set_visual(widget,visual)  ((void) 0)
#define gtk_widget_push_visual(visual)        ((void) 0)
#define gtk_widget_pop_visual()               ((void) 0)
#define gtk_widget_set_default_visual(visual) ((void) 0)
#endif /* GTK_DISABLE_DEPRECATED */

/* Accessibility support */
AtkObject*       gtk_widget_get_accessible               (GtkWidget          *widget);

/* The following functions must not be called on an already
 * realized widget. Because it is possible that somebody
 * can call get_colormap() or get_visual() and save the
 * result, these functions are probably only safe to
 * call in a widget's init() function.
 */
void         gtk_widget_set_colormap    (GtkWidget      *widget,
					 GdkColormap    *colormap);

gint	     gtk_widget_get_events	(GtkWidget	*widget);
void	     gtk_widget_get_pointer	(GtkWidget	*widget,
					 gint		*x,
					 gint		*y);

gboolean     gtk_widget_is_ancestor	(GtkWidget	*widget,
					 GtkWidget	*ancestor);

gboolean     gtk_widget_translate_coordinates (GtkWidget  *src_widget,
					       GtkWidget  *dest_widget,
					       gint        src_x,
					       gint        src_y,
					       gint       *dest_x,
					       gint       *dest_y);

/* Hide widget and return TRUE.
 */
gboolean     gtk_widget_hide_on_delete	(GtkWidget	*widget);

/* Widget styles.
 */
void	   gtk_widget_set_style		(GtkWidget	*widget,
					 GtkStyle	*style);
void	   gtk_widget_ensure_style	(GtkWidget	*widget);
GtkStyle*  gtk_widget_get_style		(GtkWidget	*widget);

void        gtk_widget_modify_style       (GtkWidget            *widget,
					   GtkRcStyle           *style);
GtkRcStyle *gtk_widget_get_modifier_style (GtkWidget            *widget);
void        gtk_widget_modify_fg          (GtkWidget            *widget,
					   GtkStateType          state,
					   const GdkColor       *color);
void        gtk_widget_modify_bg          (GtkWidget            *widget,
					   GtkStateType          state,
					   const GdkColor       *color);
void        gtk_widget_modify_text        (GtkWidget            *widget,
					   GtkStateType          state,
					   const GdkColor       *color);
void        gtk_widget_modify_base        (GtkWidget            *widget,
					   GtkStateType          state,
					   const GdkColor       *color);
void        gtk_widget_modify_cursor      (GtkWidget            *widget,
					   const GdkColor       *primary,
					   const GdkColor       *secondary);
void        gtk_widget_modify_font        (GtkWidget            *widget,
					   PangoFontDescription *font_desc);

#ifndef GTK_DISABLE_DEPRECATED
#define gtk_widget_set_rc_style(widget)          (gtk_widget_set_style (widget, NULL))
#define gtk_widget_restore_default_style(widget) (gtk_widget_set_style (widget, NULL))
#endif

PangoContext *gtk_widget_create_pango_context (GtkWidget   *widget);
PangoContext *gtk_widget_get_pango_context    (GtkWidget   *widget);
PangoLayout  *gtk_widget_create_pango_layout  (GtkWidget   *widget,
					       const gchar *text);

GdkPixbuf    *gtk_widget_render_icon          (GtkWidget   *widget,
                                               const gchar *stock_id,
                                               GtkIconSize  size,
                                               const gchar *detail);

/* handle composite names for GTK_COMPOSITE_CHILD widgets,
 * the returned name is newly allocated.
 */
void   gtk_widget_set_composite_name	(GtkWidget	*widget,
					 const gchar   	*name);
gchar* gtk_widget_get_composite_name	(GtkWidget	*widget);
     
/* Descend recursively and set rc-style on all widgets without user styles */
void       gtk_widget_reset_rc_styles   (GtkWidget      *widget);

/* Push/pop pairs, to change default values upon a widget's creation.
 * This will override the values that got set by the
 * gtk_widget_set_default_* () functions.
 */
void	     gtk_widget_push_colormap	     (GdkColormap *cmap);
void	     gtk_widget_push_composite_child (void);
void	     gtk_widget_pop_composite_child  (void);
void	     gtk_widget_pop_colormap	     (void);

/* widget style properties
 */
void gtk_widget_class_install_style_property        (GtkWidgetClass     *klass,
						     GParamSpec         *pspec);
void gtk_widget_class_install_style_property_parser (GtkWidgetClass     *klass,
						     GParamSpec         *pspec,
						     GtkRcPropertyParser parser);
GParamSpec*  gtk_widget_class_find_style_property   (GtkWidgetClass     *klass,
						     const gchar        *property_name);
GParamSpec** gtk_widget_class_list_style_properties (GtkWidgetClass     *klass,
						     guint              *n_properties);
void gtk_widget_style_get_property (GtkWidget	     *widget,
				    const gchar    *property_name,
				    GValue	     *value);
void gtk_widget_style_get_valist   (GtkWidget	     *widget,
				    const gchar    *first_property_name,
				    va_list         var_args);
void gtk_widget_style_get          (GtkWidget	     *widget,
				    const gchar    *first_property_name,
				    ...) G_GNUC_NULL_TERMINATED;


/* Set certain default values to be used at widget creation time.
 */
void	     gtk_widget_set_default_colormap (GdkColormap *colormap);
GtkStyle*    gtk_widget_get_default_style    (void);
#ifndef GDK_MULTIHEAD_SAFE
GdkColormap* gtk_widget_get_default_colormap (void);
GdkVisual*   gtk_widget_get_default_visual   (void);
#endif

/* Functions for setting directionality for widgets
 */

void             gtk_widget_set_direction         (GtkWidget        *widget,
						   GtkTextDirection  dir);
GtkTextDirection gtk_widget_get_direction         (GtkWidget        *widget);

void             gtk_widget_set_default_direction (GtkTextDirection  dir);
GtkTextDirection gtk_widget_get_default_direction (void);

/* Compositing manager functionality */
gboolean gtk_widget_is_composited (GtkWidget *widget);

/* Counterpart to gdk_window_shape_combine_mask.
 */
void	     gtk_widget_shape_combine_mask (GtkWidget *widget,
					    GdkBitmap *shape_mask,
					    gint       offset_x,
					    gint       offset_y);
void	     gtk_widget_input_shape_combine_mask (GtkWidget *widget,
						  GdkBitmap *shape_mask,
						  gint       offset_x,
						  gint       offset_y);

/* internal function */
void	     gtk_widget_reset_shapes	   (GtkWidget *widget);

/* Compute a widget's path in the form "GtkWindow.MyLabel", and
 * return newly alocated strings.
 */
void	     gtk_widget_path		   (GtkWidget *widget,
					    guint     *path_length,
					    gchar    **path,
					    gchar    **path_reversed);
void	     gtk_widget_class_path	   (GtkWidget *widget,
					    guint     *path_length,
					    gchar    **path,
					    gchar    **path_reversed);

GList* gtk_widget_list_mnemonic_labels  (GtkWidget *widget);
void   gtk_widget_add_mnemonic_label    (GtkWidget *widget,
					 GtkWidget *label);
void   gtk_widget_remove_mnemonic_label (GtkWidget *widget,
					 GtkWidget *label);

void                  gtk_widget_set_tooltip_window    (GtkWidget   *widget,
                                                        GtkWindow   *custom_window);
GtkWindow *gtk_widget_get_tooltip_window    (GtkWidget   *widget);
void       gtk_widget_trigger_tooltip_query (GtkWidget   *widget);
void       gtk_widget_set_tooltip_text      (GtkWidget   *widget,
                                             const gchar *text);
gchar *    gtk_widget_get_tooltip_text      (GtkWidget   *widget);
void       gtk_widget_set_tooltip_markup    (GtkWidget   *widget,
                                             const gchar *markup);
gchar *    gtk_widget_get_tooltip_markup    (GtkWidget   *widget);
void       gtk_widget_set_has_tooltip       (GtkWidget   *widget,
					     gboolean     has_tooltip);
gboolean   gtk_widget_get_has_tooltip       (GtkWidget   *widget);

GType           gtk_requisition_get_type (void) G_GNUC_CONST;
GtkRequisition *gtk_requisition_copy     (const GtkRequisition *requisition);
void            gtk_requisition_free     (GtkRequisition       *requisition);

#if	defined (GTK_TRACE_OBJECTS) && defined (__GNUC__)
#  define gtk_widget_ref gtk_object_ref
#  define gtk_widget_unref gtk_object_unref
#endif	/* GTK_TRACE_OBJECTS && __GNUC__ */

void              _gtk_widget_grab_notify                 (GtkWidget    *widget,
						           gboolean	was_grabbed);

GtkWidgetAuxInfo *_gtk_widget_get_aux_info                (GtkWidget    *widget,
							   gboolean      create);
void              _gtk_widget_propagate_hierarchy_changed (GtkWidget    *widget,
							   GtkWidget    *previous_toplevel);
void              _gtk_widget_propagate_screen_changed    (GtkWidget    *widget,
							   GdkScreen    *previous_screen);
void		  _gtk_widget_propagate_composited_changed (GtkWidget    *widget);

GdkColormap* _gtk_widget_peek_colormap (void);

G_END_DECLS

#endif /* __GTK_WIDGET_H__ */
