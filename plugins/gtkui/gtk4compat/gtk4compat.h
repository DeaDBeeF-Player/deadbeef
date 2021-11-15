//
//  gtk4compat.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 15/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#ifndef gtk4compat_h
#define gtk4compat_h

#include <stdint.h>
#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

typedef struct _GdkColor
{
    guint32 pixel;
    guint16 red;
    guint16 green;
    guint16 blue;
} GdkColor;

typedef GdkButtonEvent GdkEventButton;
typedef GdkMotionEvent GdkEventMotion;
typedef GdkKeyEvent GdkEventKey;
typedef GdkFocusEvent GdkEventFocus;
typedef GdkScrollEvent GdkEventScroll;

#define GTK4_DEF_MISSING(x) typedef struct {} x;

typedef struct {
    int changed_mask;
    int new_window_state;
} GdkEventWindowState;

GTK4_DEF_MISSING(GdkEventConfigure)
GTK4_DEF_MISSING(GdkEventExpose)
GTK4_DEF_MISSING(GtkMenuItem)
GTK4_DEF_MISSING(GdkWindow)
GTK4_DEF_MISSING(GtkTable)
GTK4_DEF_MISSING(GtkTableClass)
GTK4_DEF_MISSING(GtkMenu)
GTK4_DEF_MISSING(GtkRadioButton)
GTK4_DEF_MISSING(GtkBin)
GTK4_DEF_MISSING(GtkCheckMenuItem)
GTK4_DEF_MISSING(GtkContainer)
GTK4_DEF_MISSING(GtkContainerClass)
GTK4_DEF_MISSING(GtkButtonBox)

typedef struct {
    PangoFontDescription *font_desc;
} GtkStyle;

#define GDK_Escape GDK_KEY_Escape
#define GDK_Return GDK_KEY_Return
#define GDK_ISO_Enter GDK_KEY_ISO_Enter
#define GDK_KP_Enter GDK_KEY_KP_Enter
#define GDK_Delete GDK_KEY_Delete
#define GDK_Insert GDK_KEY_Insert

typedef void *GtkMenuDetachFunc;
typedef void *GtkMenuPositionFunc;
typedef int GdkWindowState;
typedef int GtkWindowPosition;
typedef int GtkButtonBoxStyle;

#define GTK_WIN_POS_CENTER_ON_PARENT 0
#define GDK_WINDOW_STATE_MAXIMIZED 0

#define GTK_TYPE_VBOX 0

#define GTK_BUTTONBOX_EXPAND 0

#define GTK_STOCK_CANCEL "Cancel"
#define GTK_STOCK_OPEN "Open"

void gtk_widget_destroy (GtkWidget *widget);
GdkDisplay *gdk_window_get_display(GdkWindow *window);
void gtk_entry_set_text(GtkEntry *entry, const gchar *text);
int gtk_dialog_run(GtkDialog *dialog);
const gchar *gtk_entry_get_text (GtkEntry *entry);
#define GTK_IS_MENU(x) FALSE
#define GTK_MENU(x) ((GtkMenu*)(x))
GtkWidget *gtk_menu_get_attach_widget (GtkMenu *menu);
void gtk_menu_attach_to_widget (GtkMenu *menu, GtkWidget *widget, GtkMenuDetachFunc detacher);
GtkWindow *gtk_widget_get_window(GtkWidget *widget);
int gdk_window_get_state(GtkWindow *window);
#define GDK_WINDOW_STATE_ICONIFIED 0
gboolean gtk_widget_hide_on_delete (GtkWidget *widget);
GdkMonitor *gdk_display_get_monitor_at_window (GdkDisplay *display, GdkWindow *window);
void gtk_widget_show_all (GtkWidget *widget);
#define GTK_BIN(x) ((GtkBin*)(x))
GtkWidget *gtk_bin_get_child(GtkBin *bin);
void gtk_menu_popup (GtkMenu           *menu,
                                    GtkWidget           *parent_menu_shell,
                                    GtkWidget           *parent_menu_item,
                                    GtkMenuPositionFunc    func,
                                    gpointer        data,
                                    guint        button,
                                    guint32        activate_time);
GtkStyle *gtk_widget_get_style (GtkWindow *window);
gboolean gtk_font_button_set_font_name  (GtkFontButton *font_button,
                                                      const gchar   *fontname);
const gchar *         gtk_font_button_get_font_name  (GtkFontButton *font_button);
void       gtk_color_button_get_color      (GtkColorButton *button,
                                            GdkColor       *color);

void       gtk_color_button_set_color      (GtkColorButton *button,
                                            const GdkColor *color);
void       gtk_window_set_position             (GtkWindow           *window,
                                                GtkWindowPosition    position);
void     gtk_window_get_position     (GtkWindow   *window,
                                      gint        *root_x,
                                      gint        *root_y);
void     gtk_window_get_size         (GtkWindow   *window,
                                      gint        *width,
                                      gint        *height);
void     gtk_window_move             (GtkWindow   *window,
                                      gint         x,
                                      gint         y);
void     gtk_window_resize           (GtkWindow   *window,
                                      gint         width,
                                      gint         height);
gboolean   gtk_check_menu_item_get_active        (GtkCheckMenuItem *check_menu_item);
#define GTK_CHECK_MENU_ITEM(x) ((GtkCheckMenuItem *)(x))
void    gtk_container_add         (GtkContainer       *container,
                                   GtkWidget       *widget);
#define GTK_CONTAINER(x) ((GtkContainer *)(x))
#define GTK_BUTTON_BOX(x) ((GtkButtonBox *)(x))
void              gtk_button_box_set_layout          (GtkButtonBox      *widget,
                                                      GtkButtonBoxStyle  layout_style);
GtkWidget* gtk_event_box_new                (void);
gboolean              gtk_widget_get_has_window         (GtkWidget    *widget);

#endif /* gtk4compat_h */
