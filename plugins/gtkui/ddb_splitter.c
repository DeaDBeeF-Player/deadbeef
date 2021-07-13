/*
 * Copyright (c) 2016 Christian Boxdörfer <christian.boxdoerfer@posteo.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include "support.h"
#include "ddb_splitter.h"

/**
 * SECTION: ddb-splitter
 * @title: DdbSplitter
 * @short_description: A container widget similar to GtkPaned, but with
 * the ability to use proportional resizing
 **/

#define DDB_SPLITTER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
            DDB_TYPE_SPLITTER, DdbSplitterPrivate))

/* Property identifiers */
enum
{
    PROP_0,
    PROP_ORIENTATION,
    PROP_SIZE_MODE,
    PROP_PROPORTION,
};

#define DDB_SPLITTER_HANDLE_SIZE 6
#define DDB_SPLITTER_HANDLE_LOCKED_SIZE 3

#if !GTK_CHECK_VERSION(3,0,0)
static void
ddb_splitter_size_request (GtkWidget        *widget,
                           GtkRequisition   *requisition);
#else
static void
ddb_splitter_get_preferred_width (GtkWidget *widget,
                               gint *minimum,
                               gint *natural);

static void
ddb_splitter_get_preferred_height (GtkWidget *widget,
                               gint *minimum,
                               gint *natural);

static void
ddb_splitter_get_preferred_width_for_height (GtkWidget *widget,
                               gint height,
                               gint *minimum,
                               gint *natural);

static void
ddb_splitter_get_preferred_height_for_width (GtkWidget *widget,
                               gint width,
                               gint *minimum,
                               gint *natural);
#endif

static gboolean
ddb_splitter_button_press (GtkWidget *widget,
                        GdkEventButton *event);
static gboolean
ddb_splitter_button_release (GtkWidget *widget,
                             GdkEventButton *event);
static gboolean
ddb_splitter_grab_broken (GtkWidget *widget,
                GdkEventGrabBroken *event);
static void
ddb_splitter_grab_notify (GtkWidget *widget,
                       gboolean   was_grabbed);
static gboolean
ddb_splitter_motion (GtkWidget *widget,
                     GdkEventMotion *event);
#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ddb_splitter_draw (GtkWidget *widget,
                cairo_t   *cr);
#else
static gboolean
ddb_splitter_expose (GtkWidget      *widget,
                  GdkEventExpose *event);
#endif
static void
ddb_splitter_realize (GtkWidget *widget);

static void
ddb_splitter_unrealize (GtkWidget *widget);

static void
ddb_splitter_map (GtkWidget *widget);

static void
ddb_splitter_unmap (GtkWidget *widget);

static void
ddb_splitter_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec);
static void
ddb_splitter_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec);

static void
ddb_splitter_set_orientation (DdbSplitter *splitter,
                              GtkOrientation orientation);

static void
ddb_splitter_size_allocate (GtkWidget *widget,
                            GtkAllocation *allocation);
static void
ddb_splitter_add (GtkContainer *container,
                  GtkWidget *widget);
static void
ddb_splitter_remove (GtkContainer *container,
                     GtkWidget *widget);
static void
ddb_splitter_forall (GtkContainer *container,
                     gboolean include_internals,
                     GtkCallback callback,
                     gpointer callback_data);

struct _DdbSplitterPrivate
{
    GtkWidget *child1;
    GtkWidget *child2;
    GdkWindow *handle;
    GdkRectangle handle_pos;
    gint handle_size;
    gint drag_pos;
    guint in_drag : 1;
    guint position_set : 1;
    guint32 grab_time;

    /* configurable parameters */
    guint child1_size;
    guint child2_size;

    GtkOrientation orientation;
    DdbSplitterSizeMode size_mode;
    gfloat proportion;
};

G_DEFINE_TYPE (DdbSplitter, ddb_splitter, GTK_TYPE_CONTAINER)

static void
ddb_splitter_class_init (DdbSplitterClass *klass)
{
    GtkContainerClass *gtkcontainer_class;
    GtkWidgetClass    *gtkwidget_class;
    GObjectClass      *gobject_class;

    /* add our private data to the class */
    g_type_class_add_private (klass, sizeof (DdbSplitterPrivate));

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->get_property = ddb_splitter_get_property;
    gobject_class->set_property = ddb_splitter_set_property;

    gtkwidget_class = GTK_WIDGET_CLASS (klass);
#if !GTK_CHECK_VERSION(3,0,0)
    gtkwidget_class->size_request = ddb_splitter_size_request;
#else
    gtkwidget_class->get_preferred_width = ddb_splitter_get_preferred_width;
    gtkwidget_class->get_preferred_height = ddb_splitter_get_preferred_height;
    gtkwidget_class->get_preferred_width_for_height = ddb_splitter_get_preferred_width_for_height;
    gtkwidget_class->get_preferred_height_for_width = ddb_splitter_get_preferred_height_for_width;
#endif
    gtkwidget_class->size_allocate = ddb_splitter_size_allocate;
    gtkwidget_class->realize = ddb_splitter_realize;
#if GTK_CHECK_VERSION(3,0,0)
    gtkwidget_class->draw = ddb_splitter_draw;
#else
    gtkwidget_class->expose_event = ddb_splitter_expose;
#endif
    gtkwidget_class->unrealize = ddb_splitter_unrealize;
    gtkwidget_class->map = ddb_splitter_map;
    gtkwidget_class->unmap = ddb_splitter_unmap;
    gtkwidget_class->button_press_event = ddb_splitter_button_press;
    gtkwidget_class->button_release_event = ddb_splitter_button_release;
    gtkwidget_class->motion_notify_event = ddb_splitter_motion;
    gtkwidget_class->grab_broken_event = ddb_splitter_grab_broken;
    gtkwidget_class->grab_notify = ddb_splitter_grab_notify;

    gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
    gtkcontainer_class->add = ddb_splitter_add;
    gtkcontainer_class->remove = ddb_splitter_remove;
    gtkcontainer_class->forall = ddb_splitter_forall;

    /**
     * DdbSplitter::size_mode:
     *
     * The size mode of the splitter.
     **/
    g_object_class_install_property (gobject_class,
            PROP_SIZE_MODE,
            g_param_spec_enum ("size-mode",
                "Size mode",
                "The size mode of the splitter widget",
                DDB_SPLITTER_TYPE_SIZE_MODE, DDB_SPLITTER_SIZE_MODE_PROP,
                G_PARAM_READWRITE));
    /**
     * DdbSplitter::orientation:
     *
     * The orientation of the splitter.
     **/
    g_object_class_install_property (gobject_class,
            PROP_ORIENTATION,
            g_param_spec_enum ("orientation",
                "Orientation",
                "The orientation of the splitter widget",
                GTK_TYPE_ORIENTATION, GTK_ORIENTATION_HORIZONTAL,
                G_PARAM_READWRITE));
    /**
     * DdbSplitter::proportion:
     *
     * The percentage of space allocated to the first child.
     **/
    g_object_class_install_property (gobject_class,
            PROP_PROPORTION,
            g_param_spec_float ("proportion",
                "Proportion",
                "The percentage of space allocated to the first child",
                -G_MAXFLOAT, 1.0, -1.0,
                G_PARAM_READWRITE));
}

static void
ddb_splitter_init (DdbSplitter *splitter)
{
    /* grab a pointer on the private data */
    splitter->priv = DDB_SPLITTER_GET_PRIVATE (splitter);

    splitter->priv->orientation = GTK_ORIENTATION_HORIZONTAL;
    splitter->priv->size_mode = DDB_SPLITTER_SIZE_MODE_PROP;
    splitter->priv->drag_pos = -1;
    splitter->priv->in_drag = FALSE;
    splitter->priv->position_set = FALSE;
    splitter->priv->child1 = NULL;
    splitter->priv->child2 = NULL;
    splitter->priv->child1_size = 0;
    splitter->priv->child2_size = 0;
    splitter->priv->handle = NULL;
    splitter->priv->handle_size = DDB_SPLITTER_HANDLE_SIZE;
    splitter->priv->handle_pos.x = -1;
    splitter->priv->handle_pos.y = -1;
    splitter->priv->handle_pos.width = DDB_SPLITTER_HANDLE_SIZE;
    splitter->priv->handle_pos.height = DDB_SPLITTER_HANDLE_SIZE;
    splitter->priv->proportion = 0.5f;
    /* we don't provide our own window */
    gtk_widget_set_can_focus (GTK_WIDGET (splitter), FALSE);
    gtk_widget_set_has_window (GTK_WIDGET (splitter), FALSE);
    gtk_widget_set_redraw_on_allocate (GTK_WIDGET (splitter), FALSE);
}

static void
ddb_splitter_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
    DdbSplitter *splitter = DDB_SPLITTER (object);

    switch (prop_id) {
        case PROP_ORIENTATION:
            g_value_set_enum (value, ddb_splitter_get_orientation (splitter));
            break;

        case PROP_SIZE_MODE:
            g_value_set_enum (value, ddb_splitter_get_size_mode (splitter));
            break;

        case PROP_PROPORTION:
            g_value_set_float (value, ddb_splitter_get_proportion (splitter));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
ddb_splitter_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
    DdbSplitter *splitter = DDB_SPLITTER (object);

    switch (prop_id) {
        case PROP_ORIENTATION:
            ddb_splitter_set_orientation (splitter, g_value_get_enum (value));
            break;

        case PROP_SIZE_MODE:
            ddb_splitter_set_size_mode (splitter, g_value_get_enum (value));
            break;

        case PROP_PROPORTION:
            ddb_splitter_set_proportion (splitter, g_value_get_float (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gboolean
ddb_splitter_button_press (GtkWidget      *widget,
                        GdkEventButton *event)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    if (event->window == splitter->priv->handle
           && event->button == 1
           && event->type == GDK_2BUTTON_PRESS) {
        ddb_splitter_set_proportion (splitter, 0.5f);
        return TRUE;
    }
    if (!splitter->priv->in_drag &&
            (event->window == splitter->priv->handle) && (event->button == 1))
    {
        /* We need a server grab here, not gtk_grab_add(), since
         * we don't want to pass events on to the widget's children */
        if (gdk_pointer_grab (splitter->priv->handle, FALSE,
                    GDK_POINTER_MOTION_HINT_MASK
                    | GDK_BUTTON1_MOTION_MASK
                    | GDK_BUTTON_RELEASE_MASK
                    | GDK_ENTER_NOTIFY_MASK
                    | GDK_LEAVE_NOTIFY_MASK,
                    NULL, NULL,
                    event->time) != GDK_GRAB_SUCCESS)
            return FALSE;

        splitter->priv->in_drag = TRUE;
        splitter->priv->grab_time = event->time;

        if (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            splitter->priv->drag_pos = event->x;
        else
            splitter->priv->drag_pos = event->y;

        return TRUE;
    }

    return FALSE;
}

static void
ddb_splitter_update_cursor (DdbSplitter *splitter)
{
    if (gtk_widget_get_realized (GTK_WIDGET (splitter))) {
        GdkCursor *cursor = NULL;
        if (splitter->priv->size_mode != DDB_SPLITTER_SIZE_MODE_PROP) {
            cursor = NULL;
        }
        else {
            if (splitter->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                cursor = gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (splitter)),
                        GDK_SB_V_DOUBLE_ARROW);
            }
            else {
                cursor = gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (splitter)),
                        GDK_SB_H_DOUBLE_ARROW);
            }
        }

        gdk_window_set_cursor (splitter->priv->handle, cursor);

        if (cursor) {
            gdk_cursor_unref (cursor);
        }
    }
}

static gboolean
ddb_splitter_children_visible (DdbSplitter *splitter)
{
    g_return_val_if_fail (DDB_IS_SPLITTER (splitter), FALSE);
    if (splitter->priv->child1
            && gtk_widget_get_visible (GTK_WIDGET (splitter->priv->child1))
            && splitter->priv->child2
            && gtk_widget_get_visible (GTK_WIDGET (splitter->priv->child2))) {
        return TRUE;
    }
    return FALSE;
}

static gboolean
ddb_splitter_is_child_visible (DdbSplitter *splitter, guint child)
{
    g_return_val_if_fail (DDB_IS_SPLITTER (splitter), FALSE);
    if (child == 0) {
        if (splitter->priv->child1
                && gtk_widget_get_visible (GTK_WIDGET (splitter->priv->child1))) {
            return TRUE;
        }
    }
    else if (child == 1) {
        if (splitter->priv->child2
                && gtk_widget_get_visible (GTK_WIDGET (splitter->priv->child2))) {
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean
ddb_splitter_grab_broken (GtkWidget          *widget,
                       GdkEventGrabBroken *event)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    splitter->priv->in_drag = FALSE;
    splitter->priv->drag_pos = -1;
    splitter->priv->position_set = TRUE;

    return TRUE;
}

static void
stop_drag (DdbSplitter *splitter)
{
    splitter->priv->in_drag = FALSE;
    splitter->priv->drag_pos = -1;
    splitter->priv->position_set = TRUE;
    gdk_display_pointer_ungrab (gtk_widget_get_display (GTK_WIDGET (splitter)),
            splitter->priv->grab_time);
}

static void
ddb_splitter_grab_notify (GtkWidget *widget,
                       gboolean   was_grabbed)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    if (!was_grabbed && splitter->priv->in_drag)
        stop_drag (splitter);
}

static gboolean
ddb_splitter_button_release (GtkWidget      *widget,
                          GdkEventButton *event)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    if (splitter->priv->in_drag && (event->button == 1)) {
        stop_drag (splitter);

        return TRUE;
    }

    return FALSE;
}

static void
update_drag (DdbSplitter *splitter)
{
    gint pos;
    gint handle_size;
    gint size;

    if (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_pointer (GTK_WIDGET (splitter), &pos, NULL);
    else
        gtk_widget_get_pointer (GTK_WIDGET (splitter), NULL, &pos);

    pos -= splitter->priv->drag_pos;

    size = pos;

    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (splitter), &a);
    if (size != splitter->priv->child1_size) {
        if (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
            float gripsize = (float)splitter->priv->handle_pos.width/a.width;
            ddb_splitter_set_proportion (splitter, CLAMP ((float)size/a.width, 0, 1-gripsize));
        }
        else {
            float gripsize = (float)splitter->priv->handle_pos.height/a.height;
            ddb_splitter_set_proportion (splitter, CLAMP ((float)size/a.height, 0, 1-gripsize));
        }
    }
}

static gboolean
ddb_splitter_motion (GtkWidget      *widget,
                  GdkEventMotion *event)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    if (splitter->priv->in_drag) {
        update_drag (splitter);
        return TRUE;
    }

    return FALSE;
}

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ddb_splitter_draw (GtkWidget *widget,
                cairo_t   *cr)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    if (gtk_widget_get_visible (widget) && gtk_widget_get_mapped (widget) &&
            ddb_splitter_children_visible (splitter))
    {
        if (splitter->priv->size_mode == DDB_SPLITTER_SIZE_MODE_PROP) {
            gtk_render_handle (gtk_widget_get_style_context (widget), cr,
                    splitter->priv->handle_pos.x, splitter->priv->handle_pos.y,
                    splitter->priv->handle_pos.width, splitter->priv->handle_pos.height);
        }
        else {
            gtk_render_background (gtk_widget_get_style_context (widget), cr,
                    splitter->priv->handle_pos.x, splitter->priv->handle_pos.y,
                    splitter->priv->handle_pos.width, splitter->priv->handle_pos.height);
        }
    }

    /* Chain up to draw children */
    GTK_WIDGET_CLASS (ddb_splitter_parent_class)->draw (widget, cr);

    return FALSE;
}

#else

static gboolean
ddb_splitter_expose (GtkWidget      *widget,
                  GdkEventExpose *event)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    if (gtk_widget_get_visible (widget) && gtk_widget_get_mapped (widget) &&
            ddb_splitter_children_visible (splitter))
    {
        GtkStateType state;

        if (gtk_widget_is_focus (widget))
            state = GTK_STATE_SELECTED;
        else
            state = gtk_widget_get_state (widget);

        //cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
        if (splitter->priv->size_mode == DDB_SPLITTER_SIZE_MODE_PROP) {
            gtk_paint_handle (gtk_widget_get_style (widget), gtk_widget_get_window (widget),
                    state, GTK_SHADOW_NONE,
                    &splitter->priv->handle_pos, widget, "paned",
                    splitter->priv->handle_pos.x, splitter->priv->handle_pos.y,
                    splitter->priv->handle_pos.width, splitter->priv->handle_pos.height,
                    !splitter->priv->orientation);
        }
        else {
            gtk_paint_box (gtk_widget_get_style (widget), gtk_widget_get_window (widget),
                    state, GTK_SHADOW_NONE,
                    &splitter->priv->handle_pos, widget, "paned",
                    splitter->priv->handle_pos.x, splitter->priv->handle_pos.y,
                    splitter->priv->handle_pos.width, splitter->priv->handle_pos.height);
        }
        //cairo_destroy (cr);
    }

    /* Chain up to draw children */
    GTK_WIDGET_CLASS (ddb_splitter_parent_class)->expose_event (widget, event);

    return FALSE;
}
#endif

static void
ddb_splitter_realize (GtkWidget *widget)
{
    GdkWindowAttr attributes;
    gint attributes_mask;

    gtk_widget_set_realized (widget, TRUE);
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    GdkWindow *parent = gtk_widget_get_parent_window (widget);
    gtk_widget_set_window (widget, parent);
    if (parent) {
        g_object_ref (parent);

        attributes.window_type = GDK_WINDOW_CHILD;
        attributes.wclass = GDK_INPUT_ONLY;
        attributes.x = splitter->priv->handle_pos.x;
        attributes.y = splitter->priv->handle_pos.y;
        attributes.width = splitter->priv->handle_pos.width;
        attributes.height = splitter->priv->handle_pos.height;
        attributes.event_mask = gtk_widget_get_events (widget);
        attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                GDK_BUTTON_RELEASE_MASK |
                GDK_ENTER_NOTIFY_MASK |
                GDK_LEAVE_NOTIFY_MASK |
                GDK_POINTER_MOTION_MASK |
                GDK_POINTER_MOTION_HINT_MASK);
        attributes_mask = GDK_WA_X | GDK_WA_Y;
        if (gtk_widget_is_sensitive (widget)) {
            if (splitter->priv->size_mode != DDB_SPLITTER_SIZE_MODE_PROP) {
                attributes.cursor = NULL;
            }
            else {
                if (splitter->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                    attributes.cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                            GDK_SB_V_DOUBLE_ARROW);
                }
                else {
                    attributes.cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                            GDK_SB_H_DOUBLE_ARROW);
                }
            }
            attributes_mask |= GDK_WA_CURSOR;
        }

        splitter->priv->handle = gdk_window_new (parent,
                &attributes, attributes_mask);
        gdk_window_set_user_data (splitter->priv->handle, splitter);
        if (attributes_mask & GDK_WA_CURSOR && attributes.cursor) {
            gdk_cursor_unref (attributes.cursor);
        }


#if !GTK_CHECK_VERSION(3,0,0)
        widget->style = gtk_style_attach (widget->style, widget->window);
#endif

        if (ddb_splitter_children_visible (splitter)) {
            gdk_window_show (splitter->priv->handle);
        }
    }
}

static void
ddb_splitter_unrealize (GtkWidget *widget)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    if (splitter->priv->handle) {
        gdk_window_set_user_data (splitter->priv->handle, NULL);
        gdk_window_destroy (splitter->priv->handle);
        splitter->priv->handle = NULL;
    }

    GTK_WIDGET_CLASS (ddb_splitter_parent_class)->unrealize (widget);
}

static void
ddb_splitter_map (GtkWidget *widget)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    gdk_window_show (splitter->priv->handle);

    GTK_WIDGET_CLASS (ddb_splitter_parent_class)->map (widget);
}

static void
ddb_splitter_unmap (GtkWidget *widget)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    gdk_window_hide (splitter->priv->handle);

    GTK_WIDGET_CLASS (ddb_splitter_parent_class)->unmap (widget);
}

// Prevent panes from getting smaller than the underlying widget's natural size,
// with priority on left/top pane.
static gfloat
_ddb_splitter_fix_proportion (DdbSplitter *splitter, gfloat proportion) {
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (splitter), &a);
    if (a.x < 0 || a.y < 0) {
        return proportion;
    }

    float pos = proportion * (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL ? a.width : a.height);

    GtkRequisition r1, r2;
#if !GTK_CHECK_VERSION(3,0,0)
    gtk_widget_size_request (splitter->priv->child1, &r1);
    gtk_widget_size_request (splitter->priv->child2, &r2);
#else
    gtk_widget_get_preferred_size(splitter->priv->child1, NULL, &r1);
    gtk_widget_get_preferred_size(splitter->priv->child2, NULL, &r2);
#endif

    if (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
        if (pos > a.width - r2.width) {
            pos = a.width - r2.width;
        }
        if (pos < r1.width) {
            pos = r1.width;
        }
        if (pos >= a.width - splitter->priv->handle_size) {
            pos = a.width - splitter->priv->handle_size;
        }
    }
    else {
        if (pos > a.height - r2.height) {
            pos = a.height - r2.height;
        }
        if (pos < r1.height) {
            pos = r1.height;
        }
        if (pos >= a.height - splitter->priv->handle_size) {
            pos = a.height - splitter->priv->handle_size;
        }
    }

    if (pos < 0) {
        pos = 0;
    }

    return pos / (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL ? a.width : a.height);
}

#if !GTK_CHECK_VERSION(3,0,0)

static void
ddb_splitter_size_request (GtkWidget      *widget,
        GtkRequisition *requisition)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);

    gint border_width = gtk_container_get_border_width (GTK_CONTAINER (splitter));

    GtkRequisition req_c1;
    req_c1.width = 0;
    req_c1.height = 0;
    if (ddb_splitter_is_child_visible (splitter, 0)) {
        gtk_widget_size_request (splitter->priv->child1, &req_c1);
    }

    GtkRequisition req_c2;
    req_c2.width = 0;
    req_c2.height = 0;
    if (ddb_splitter_is_child_visible (splitter, 1)) {
        gtk_widget_size_request (splitter->priv->child2, &req_c2);
    }

    requisition->width = 0;
    requisition->height = 0;
    requisition->width += border_width * 2;
    requisition->height += border_width * 2;

    if (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
        requisition->width += req_c1.width + req_c2.width;
        requisition->height += MAX (req_c1.height, req_c2.height);
    }
    else {
        requisition->width += MAX (req_c1.width, req_c2.width);
        requisition->height += req_c1.height + req_c2.height;
    }

    if (ddb_splitter_children_visible (splitter)) {
        if (splitter->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            requisition->width += splitter->priv->handle_size;
        else
            requisition->height += splitter->priv->handle_size;
    }
}
#else

static void
ddb_splitter_get_preferred_size (GtkWidget *widget,
                                 void (func_get_preferred_size) (GtkWidget *, gint *, gint *),
                                 GtkOrientation orientation,
                                 gint *minimum,
                                 gint *natural)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);
    gint min = 0;
    gint nat = 0;
    gint child1_min = 0;
    gint child1_nat = 0;
    gint child2_min = 0;
    gint child2_nat = 0;

    if (ddb_splitter_is_child_visible (splitter, 0)) {
        func_get_preferred_size (splitter->priv->child1, &child1_min, &child1_nat);
    }
    if (ddb_splitter_is_child_visible (splitter, 1)) {
        func_get_preferred_size (splitter->priv->child2, &child2_min, &child2_nat);
    }

    if (splitter->priv->orientation == orientation) {
        nat = child1_nat + child2_nat;
        if (ddb_splitter_children_visible (splitter)) {
            min += splitter->priv->handle_size;
            nat += splitter->priv->handle_size;
        }
    }
    else {
        nat = MAX (child1_nat, child2_nat);
    }
    *minimum = min;
    *natural = nat;
}

static void
ddb_splitter_get_preferred_width (GtkWidget *widget,
                               gint *minimum,
                               gint *natural)
{
    ddb_splitter_get_preferred_size (widget,
                                     &gtk_widget_get_preferred_width,
                                     GTK_ORIENTATION_HORIZONTAL,
                                     minimum,
                                     natural);
}

static void
ddb_splitter_get_preferred_height (GtkWidget *widget,
                               gint *minimum,
                               gint *natural)
{
    ddb_splitter_get_preferred_size (widget,
                                     &gtk_widget_get_preferred_height,
                                     GTK_ORIENTATION_VERTICAL,
                                     minimum,
                                     natural);
}

static void
ddb_splitter_get_preferred_width_for_height (GtkWidget *widget,
                               gint height,
                               gint *minimum,
                               gint *natural)
{
    ddb_splitter_get_preferred_width (widget, minimum, natural);
}

static void
ddb_splitter_get_preferred_height_for_width (GtkWidget *widget,
                               gint width,
                               gint *minimum,
                               gint *natural)
{
    ddb_splitter_get_preferred_height (widget, minimum, natural);
}
#endif

static gint
ddb_splitter_child2_size_calc (DdbSplitter *splitter,
                               gint num_visible_children,
                               gboolean c1_visible,
                               gint con_size,
                               gint handle_size)
{
    DdbSplitterPrivate *sp_priv = splitter->priv;
    gint size = 0;
    if (sp_priv->size_mode == DDB_SPLITTER_SIZE_MODE_LOCK_C2) {
        // child 2 locked, use saved size
        size = sp_priv->child2_size;
    }
    else {
        if (num_visible_children == 1) {
            // only one child, use full size
            size = con_size;
        }
        else if (c1_visible && sp_priv->size_mode == DDB_SPLITTER_SIZE_MODE_LOCK_C1) {
            // two children and first one is locked, use all space left
            size = con_size - sp_priv->child1_size - handle_size;
        }
        else {
            // two children visible and proportional scaling is active
            size = con_size - sp_priv->child1_size - handle_size;
        }
    }
    return size;
}

static gint
ddb_splitter_child1_size_calc (DdbSplitter *splitter,
                               gint num_visible_children,
                               gboolean c2_visible,
                               gint con_size,
                               gint handle_size)
{
    DdbSplitterPrivate *sp_priv = splitter->priv;
    gint size = 0;
    if (sp_priv->size_mode == DDB_SPLITTER_SIZE_MODE_LOCK_C1) {
        // child 1 locked, use saved size
        size = sp_priv->child1_size;
    }
    else {
        if (num_visible_children == 1) {
            // only one child, use full size
            size = con_size;
        }
        else if (c2_visible && sp_priv->size_mode == DDB_SPLITTER_SIZE_MODE_LOCK_C2) {
            // two children and second one is locked, use all space left
            size = con_size - sp_priv->child2_size - handle_size;
        }
        else {
            // two children visible and proportional scaling is active
            float p = _ddb_splitter_fix_proportion(splitter, sp_priv->proportion);
            size = ceilf (con_size * p);
        }
    }
    return size;
}

static void
ddb_splitter_size_allocate (GtkWidget *widget, GtkAllocation *a_con)
{
    DdbSplitter *splitter = DDB_SPLITTER (widget);
    DdbSplitterPrivate *sp_priv = splitter->priv;
    GtkWidget *c1 = sp_priv->child1;
    GtkWidget *c2 = sp_priv->child2;

    gfloat old_proportion = _ddb_splitter_fix_proportion(splitter, sp_priv->proportion);

    gint border_width = gtk_container_get_border_width (GTK_CONTAINER (splitter));
    gtk_widget_set_allocation (widget, a_con);

    gboolean c1_visible = c1 && gtk_widget_get_visible (c1) ? TRUE : FALSE;
    gboolean c2_visible = c2 && gtk_widget_get_visible (c2) ? TRUE : FALSE;
    guint num_visible_children = c1_visible + c2_visible;

    gint con_width = a_con->width - border_width * 2;
    gint con_height = a_con->height - border_width * 2;
    gint handle_size = num_visible_children > 1 ? sp_priv->handle_size : 0;

    GdkRectangle old_handle_pos = sp_priv->handle_pos;

    GtkAllocation a_c1;
    memset (&a_c1, 0, sizeof (a_c1));
    GtkAllocation a_c2;
    memset (&a_c2, 0, sizeof (a_c2));
    if (sp_priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
        if (c1_visible) {
            // use full height in horitzontal splitter
            a_c1.height = MAX (1, con_height);

            gint width = ddb_splitter_child1_size_calc (splitter,
                                                        num_visible_children,
                                                        c2_visible,
                                                        con_width,
                                                        handle_size);
            a_c1.width = MAX (1, width);
            a_c1.x = a_con->x + border_width;
            a_c1.y = a_con->y + border_width;

            gtk_widget_size_allocate (sp_priv->child1, &a_c1);
            sp_priv->child1_size = a_c1.width;
            if (sp_priv->size_mode != DDB_SPLITTER_SIZE_MODE_PROP) {
                sp_priv->proportion = CLAMP ((float)a_c1.width/con_width, 0.0f, 1.0f);
            }
            sp_priv->handle_pos.x = a_con->x + sp_priv->child1_size + border_width;
            sp_priv->handle_pos.y = a_con->y + border_width;
            sp_priv->handle_pos.width = handle_size;
            sp_priv->handle_pos.height = MAX (1, a_con->height - 2 * border_width);

        }
        if (c2_visible) {
            // use full height in horitzontal splitter
            a_c2.height = MAX (1, con_height);
            gint width = ddb_splitter_child2_size_calc (splitter,
                                                        num_visible_children,
                                                        c1_visible,
                                                        con_width,
                                                        handle_size);
            a_c2.width = MAX (1, width);
            a_c2.x = a_c1.x + a_c1.width + handle_size;
            a_c2.y = a_con->y + border_width;

            gtk_widget_size_allocate (sp_priv->child2, &a_c2);
            sp_priv->child2_size = a_c2.width;
        }
    }
    else {
        if (c1_visible) {
            // use full width in vertical splitter
            a_c1.width = MAX (1, con_width);

            gint height = ddb_splitter_child1_size_calc (splitter,
                                                         num_visible_children,
                                                         c2_visible,
                                                         con_height,
                                                         handle_size);

            a_c1.height = MAX (1, height);
            a_c1.x =a_con->x + border_width;
            a_c1.y = a_con->y + border_width;

            gtk_widget_size_allocate (sp_priv->child1, &a_c1);
            sp_priv->child1_size = a_c1.height;
            if (sp_priv->size_mode != DDB_SPLITTER_SIZE_MODE_PROP) {
                sp_priv->proportion = CLAMP ((float)a_c1.height/con_height, 0.0f, 1.0f);
            }
            sp_priv->handle_pos.x = a_con->x + border_width;
            sp_priv->handle_pos.y = a_con->y + sp_priv->child1_size + border_width;
            sp_priv->handle_pos.width = MAX (1, (gint) a_con->width - 2 * border_width);
            sp_priv->handle_pos.height = handle_size;

        }
        if (c2_visible) {
            // use full width in vertical splitter
            a_c2.width = MAX (1, con_width);

            gint height = ddb_splitter_child2_size_calc (splitter,
                                                         num_visible_children,
                                                         c1_visible,
                                                         con_height,
                                                         handle_size);
            a_c2.height = MAX (1, height);
            a_c2.x = a_con->x + border_width;
            a_c2.y = a_c1.y + a_c1.height + handle_size;

            gtk_widget_size_allocate (sp_priv->child2, &a_c2);
            sp_priv->child2_size = a_c2.height;
        }
    }

    if (!c1_visible && !c2_visible) {
        GtkAllocation child_allocation;

        if (sp_priv->child1)
            gtk_widget_set_child_visible (sp_priv->child1, TRUE);
        if (sp_priv->child2)
            gtk_widget_set_child_visible (sp_priv->child2, TRUE);

        child_allocation.x = a_con->x + border_width;
        child_allocation.y = a_con->y + border_width;
        child_allocation.width = MAX (1, con_width);
        child_allocation.height = MAX (1, con_height);

        if (ddb_splitter_is_child_visible (splitter, 0))
            gtk_widget_size_allocate (sp_priv->child1, &child_allocation);
        else if (ddb_splitter_is_child_visible (splitter, 1))
            gtk_widget_size_allocate (sp_priv->child2, &child_allocation);
    }

    if (gtk_widget_get_mapped (widget) &&
            (old_handle_pos.x != sp_priv->handle_pos.x ||
             old_handle_pos.y != sp_priv->handle_pos.y ||
             old_handle_pos.width != sp_priv->handle_pos.width ||
             old_handle_pos.height != sp_priv->handle_pos.height))
    {
        GdkWindow *window = gtk_widget_get_window (widget);
        gdk_window_invalidate_rect (window, &old_handle_pos, FALSE);
        gdk_window_invalidate_rect (window, &sp_priv->handle_pos, FALSE);
    }

    if (gtk_widget_get_realized (widget)) {
        if (gtk_widget_get_mapped (widget))
            gdk_window_show (sp_priv->handle);

        if (sp_priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
            gdk_window_move_resize (sp_priv->handle,
                    sp_priv->handle_pos.x,
                    sp_priv->handle_pos.y,
                    handle_size,
                    sp_priv->handle_pos.height);
        }
        else {
            gdk_window_move_resize (sp_priv->handle,
                    sp_priv->handle_pos.x,
                    sp_priv->handle_pos.y,
                    sp_priv->handle_pos.width,
                    handle_size);
        }
    }

    g_object_freeze_notify (G_OBJECT (splitter));
    if (sp_priv->proportion != old_proportion) {
        g_object_notify (G_OBJECT (splitter), "proportion");
    }
    g_object_thaw_notify (G_OBJECT (splitter));
}

static void
ddb_splitter_add (GtkContainer *container, GtkWidget *widget)
{
    DdbSplitter *splitter = DDB_SPLITTER (container);

    if (splitter->priv->child1 && splitter->priv->child2) {
        // Splitter already full
        return;
    }

    gtk_widget_set_parent (widget, GTK_WIDGET (splitter));

    if (!splitter->priv->child1) {
        splitter->priv->child1 = widget;
    }
    else if (!splitter->priv->child2) {
        splitter->priv->child2 = widget;
    }

    /* realize the widget if required */
    if (gtk_widget_get_realized (GTK_WIDGET (container)))
        gtk_widget_realize (widget);

    /* map the widget if required */
    if (gtk_widget_get_visible (GTK_WIDGET (container)) && gtk_widget_get_visible (widget)) {
        if (gtk_widget_get_mapped (GTK_WIDGET (container)))
            gtk_widget_map (widget);
    }

    gtk_widget_queue_resize (GTK_WIDGET (container));
    return;
}

static void
ddb_splitter_remove (GtkContainer *container, GtkWidget *widget)
{
    DdbSplitter *splitter = DDB_SPLITTER (container);

    /* check if the widget was visible */
    gboolean widget_was_visible = gtk_widget_get_visible (widget);

    /* unparent and remove the widget */
    gtk_widget_unparent (widget);
    if (splitter->priv->child1 == widget) {
        splitter->priv->child1 = NULL;
    }
    else if (splitter->priv->child2 == widget) {
        splitter->priv->child2 = NULL;
    }

    if (G_LIKELY (widget_was_visible))
        gtk_widget_queue_resize (GTK_WIDGET (splitter));
}

static void
ddb_splitter_forall (GtkContainer *container,
                     gboolean include_internals,
                     GtkCallback callback,
                     gpointer callback_data)
{
    DdbSplitter *splitter = DDB_SPLITTER (container);
    GtkWidget *c1 = splitter->priv->child1;
    GtkWidget *c2 = splitter->priv->child2;

    if (c1 && GTK_IS_WIDGET (c1)) {
        (*callback) (c1, callback_data);
    }
    if (c2 && GTK_IS_WIDGET (c2)) {
        (*callback) (c2, callback_data);
    }
}

static void
ddb_splitter_remove_child (DdbSplitter *splitter, guint child)
{
    if (child == 1 && splitter->priv->child1) {
        ddb_splitter_remove (GTK_CONTAINER (splitter), splitter->priv->child1);
    }
    else if (child == 2 && splitter->priv->child2) {
        ddb_splitter_remove (GTK_CONTAINER (splitter), splitter->priv->child2);
    }
}

void
ddb_splitter_remove_c1 (DdbSplitter *splitter)
{
    ddb_splitter_remove_child (splitter, 1);
}

void
ddb_splitter_remove_c2 (DdbSplitter *splitter)
{
    ddb_splitter_remove_child (splitter, 2);
}

gboolean
ddb_splitter_add_child_at_pos (DdbSplitter *splitter, GtkWidget *child, guint pos)
{
    if (pos == 0 && !splitter->priv->child1) {
        splitter->priv->child1 = child;
    }
    else if (pos == 1 && !splitter->priv->child2) {
        splitter->priv->child2 = child;
    }
    else {
        return FALSE;
    }

    gtk_widget_set_parent (child, GTK_WIDGET (splitter));
    /* realize the widget if required */
    if (gtk_widget_get_realized (GTK_WIDGET (splitter)))
        gtk_widget_realize (child);

    /* map the widget if required */
    if (gtk_widget_get_visible (GTK_WIDGET (splitter)) && gtk_widget_get_visible (child)) {
        if (gtk_widget_get_mapped (GTK_WIDGET (splitter)))
            gtk_widget_map (child);
    }

    gtk_widget_queue_resize (GTK_WIDGET (splitter));
    return TRUE;
}

/**
 * ddb_splitter_get_size_mode:
 * @splitter : a #DdbSplitter.
 *
 * Returns the size mode of the splitter
 *
 * Returns: the size mode of @splitter.
 **/
DdbSplitterSizeMode
ddb_splitter_get_size_mode (const DdbSplitter *splitter)
{
    g_return_val_if_fail (DDB_IS_SPLITTER (splitter), DDB_SPLITTER_SIZE_MODE_PROP);
    return splitter->priv->size_mode;
}

/**
 * ddb_splitter_set_size_mode:
 * @splitter    : a #DdbSplitter.
 * @size_mode : The size_mode of the splitter.
 *
 * Sets the size_mode of the @splitter
 **/
void
ddb_splitter_set_size_mode (DdbSplitter *splitter, DdbSplitterSizeMode size_mode)
{
    g_return_if_fail (DDB_IS_SPLITTER (splitter));

    if (G_LIKELY (splitter->priv->size_mode != size_mode)) {
        splitter->priv->size_mode = size_mode;
        if (size_mode == DDB_SPLITTER_SIZE_MODE_LOCK_C1
            || size_mode == DDB_SPLITTER_SIZE_MODE_LOCK_C2) {
            splitter->priv->handle_size = DDB_SPLITTER_HANDLE_LOCKED_SIZE;
        }
        else {
            splitter->priv->handle_size = DDB_SPLITTER_HANDLE_SIZE;
        }
        ddb_splitter_update_cursor (splitter);
        gtk_widget_queue_resize (GTK_WIDGET (splitter));
        g_object_notify (G_OBJECT (splitter), "size_mode");
    }
}

/**
 * ddb_splitter_get_orientation:
 * @splitter : a #DdbSplitter.
 *
 * Returns the orientation of the splitter
 *
 * Returns: the orientation of @splitter.
 **/
GtkOrientation
ddb_splitter_get_orientation (const DdbSplitter *splitter)
{
    g_return_val_if_fail (DDB_IS_SPLITTER (splitter), GTK_ORIENTATION_HORIZONTAL);
    return splitter->priv->orientation;
}

/**
 * ddb_splitter_set_orientation:
 * @splitter    : a #DdbSplitter.
 * @orientation : The orientation of the splitter.
 *
 * Sets the orientation of the @splitter
 **/
void
ddb_splitter_set_orientation (DdbSplitter *splitter, GtkOrientation orientation)
{
    g_return_if_fail (DDB_IS_SPLITTER (splitter));

    if (G_LIKELY (splitter->priv->orientation != orientation)) {
        splitter->priv->orientation = orientation;
        gtk_widget_queue_resize (GTK_WIDGET (splitter));
        g_object_notify (G_OBJECT (splitter), "orientation");
    }
}

/**
 * ddb_splitter_get_proportion:
 * @splitter : a #DdbSplitter.
 *
 * Returns the proportion of the splitter
 *
 * Returns: the proportion of the @splitter.
 **/
gfloat
ddb_splitter_get_proportion (const DdbSplitter *splitter)
{
    g_return_val_if_fail (DDB_IS_SPLITTER (splitter), 0.f);
    return splitter->priv->proportion;
}

/**
 * ddb_splitter_set_proportion:
 * @splitter    : a #DdbSplitter.
 * @proportion : The proportion how the child should be arranged.
 *
 * Sets the proportion of the @splitter
 **/
void
ddb_splitter_set_proportion (DdbSplitter *splitter, gfloat proportion)
{
    g_return_if_fail (DDB_IS_SPLITTER (splitter));

    if (splitter->priv->size_mode == DDB_SPLITTER_SIZE_MODE_PROP
           && G_LIKELY (splitter->priv->proportion != proportion))
    {
        splitter->priv->proportion = proportion;
        gtk_widget_queue_resize (GTK_WIDGET (splitter));
        g_object_notify (G_OBJECT (splitter), "proportion");
    }
}

guint
ddb_splitter_get_child1_size (DdbSplitter *splitter)
{
    return splitter->priv->child1_size;
}

guint
ddb_splitter_get_child2_size (DdbSplitter *splitter)
{
    return splitter->priv->child2_size;
}

void
ddb_splitter_set_child1_size (DdbSplitter *splitter, guint size)
{
    g_return_if_fail (DDB_IS_SPLITTER (splitter));

    splitter->priv->child1_size = size;
    gtk_widget_queue_resize (GTK_WIDGET (splitter));
}

void
ddb_splitter_set_child2_size (DdbSplitter *splitter, guint size)
{
    g_return_if_fail (DDB_IS_SPLITTER (splitter));

    splitter->priv->child2_size = size;
    gtk_widget_queue_resize (GTK_WIDGET (splitter));
}

/* Return a new PSquare cast to a GtkWidget */
GtkWidget *
ddb_splitter_new (GtkOrientation orientation)
{
    return GTK_WIDGET (g_object_new (ddb_splitter_get_type (), "orientation", orientation, NULL));
}
