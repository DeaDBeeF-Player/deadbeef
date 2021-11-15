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
GTK4_DEF_MISSING(GtkMenuShell)
GTK4_DEF_MISSING(GdkVisual)
GTK4_DEF_MISSING(GtkRcStyle)
GTK4_DEF_MISSING(GtkRadioMenuItem)
GTK4_DEF_MISSING(GtkMisc)
GTK4_DEF_MISSING(GtkAccelGroup)
GTK4_DEF_MISSING(GtkImageMenuItem)
GTK4_DEF_MISSING(GdkDragContext)
GTK4_DEF_MISSING(GtkSelectionData)

typedef struct {
    GObject parent_instance;

    /*< public >*/

    GdkColor fg[5];
    GdkColor bg[5];
    GdkColor light[5];
    GdkColor dark[5];
    GdkColor mid[5];
    GdkColor text[5];
    GdkColor base[5];
    GdkColor text_aa[5];          /* Halfway between text/base */

    GdkColor black;
    GdkColor white;
    PangoFontDescription *font_desc;

    gint xthickness;
    gint ythickness;

    cairo_pattern_t *background[5];

    /*< private >*/

    gint attach_count;

    GdkVisual *visual;
    PangoFontDescription *private_font_desc; /* Font description for style->private_font or %NULL */

    /* the RcStyle from which this style was created */
    GtkRcStyle     *rc_style;

    GSList         *styles;         /* of type GtkStyle* */
    GArray         *property_cache;
    GSList         *icon_factories; /* of type GtkIconFactory* */
} GtkStyle;

#define GDK_Escape GDK_KEY_Escape
#define GDK_Return GDK_KEY_Return
#define GDK_ISO_Enter GDK_KEY_ISO_Enter
#define GDK_KP_Enter GDK_KEY_KP_Enter
#define GDK_Delete GDK_KEY_Delete
#define GDK_Insert GDK_KEY_Insert
#define GDK_x GDK_KEY_x
#define GDK_c GDK_KEY_c
#define GDK_v GDK_KEY_v
#define GDK_Down GDK_KEY_Down
#define GDK_Up GDK_KEY_Up
#define GDK_Page_Down GDK_KEY_Page_Down
#define GDK_Page_Up GDK_KEY_Page_Up
#define GDK_End GDK_KEY_End
#define GDK_Home GDK_KEY_Home

typedef void *GtkMenuDetachFunc;
typedef void *GtkMenuPositionFunc;
typedef void *GtkCallback;
typedef int GdkWindowState;
typedef int GtkWindowPosition;
typedef int GtkButtonBoxStyle;
typedef int GdkAtom;
typedef int GtkShadowType;

#define GTK_WIN_POS_CENTER_ON_PARENT 0
#define GDK_WINDOW_STATE_MAXIMIZED 0

#define GTK_TYPE_VBOX 0
#define GTK_TYPE_HBOX 0

#define GTK_BUTTONBOX_EXPAND 0

#define GTK_STATE_NORMAL 0
#define GTK_STATE_ACTIVE 0
#define GTK_STATE_PRELIGHT 0
#define GTK_STATE_SELECTED 0
#define GTK_STATE_INSENSITIVE 0
#define GTK_STATE_INCONSISTENT 0
#define GTK_STATE_FOCUSED 0

#define GTK_ACCEL_VISIBLE 0
#define GTK_ACCEL_LOCKED  0
#define GTK_ACCEL_MASK    0
typedef int GtkAccelFlags;

typedef struct GtkTargetEntry
{
    gchar *target;
    guint  flags;
    guint  info;
} GtkTargetEntry;

#define GTK_TARGET_SAME_APP 0

#define GTK_ICON_SIZE_INVALID 0
#define GTK_ICON_SIZE_MENU 0
#define GTK_ICON_SIZE_SMALL_TOOLBAR 0
#define GTK_ICON_SIZE_LARGE_TOOLBAR 0
#define GTK_ICON_SIZE_BUTTON 0
#define GTK_ICON_SIZE_DND 0
#define GTK_ICON_SIZE_DIALOG 0

#define GTK_STYLE_CLASS_CELL "cell"
#define GTK_STYLE_CLASS_VIEW "view"

#define GTK_STOCK_CANCEL "Cancel"
#define GTK_STOCK_OPEN "Open"

#define GDK_MOD1_MASK 0
#define GDK_MOD4_MASK 0

#define GDK_2BUTTON_PRESS 0
#define GDK_BUTTON_PRESS 0

#define GTK_SHADOW_ETCHED_IN 0

#define GTK_IS_MENU(x) FALSE
#define GTK_MENU(x) ((GtkMenu*)(x))
#define GDK_WINDOW_STATE_ICONIFIED 0
#define GTK_BIN(x) ((GtkBin*)(x))
#define GTK_CHECK_MENU_ITEM(x) ((GtkCheckMenuItem *)(x))
#define GTK_CONTAINER(x) ((GtkContainer *)(x))
#define GTK_IS_CONTAINER(x) FALSE
#define GTK_BUTTON_BOX(x) ((GtkButtonBox *)(x))
#define GTK_MENU_ITEM(x) ((GtkMenuItem *)(x))
#define GTK_RADIO_MENU_ITEM(x) ((GtkRadioMenuItem *)(x))
#define GTK_MISC(x) ((GtkMisc *)(x))
#define GTK_IMAGE_MENU_ITEM(x) ((GtkImageMenuItem *)(x))

void gtk_widget_destroy (GtkWidget *widget);
GdkDisplay *gdk_window_get_display(GdkWindow *window);
void gtk_entry_set_text(GtkEntry *entry, const gchar *text);
int gtk_dialog_run(GtkDialog *dialog);
const gchar *gtk_entry_get_text (GtkEntry *entry);
GtkWidget *gtk_menu_get_attach_widget (GtkMenu *menu);
void gtk_menu_attach_to_widget (GtkMenu *menu, GtkWidget *widget, GtkMenuDetachFunc detacher);
GtkWindow *gtk_widget_get_window(GtkWidget *widget);
int gdk_window_get_state(GtkWindow *window);
gboolean gtk_widget_hide_on_delete (GtkWidget *widget);
GdkMonitor *gdk_display_get_monitor_at_window (GdkDisplay *display, GdkWindow *window);
void gtk_widget_show_all (GtkWidget *widget);
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
void    gtk_container_add         (GtkContainer       *container,
                                   GtkWidget       *widget);
void              gtk_button_box_set_layout          (GtkButtonBox      *widget,
                                                      GtkButtonBoxStyle  layout_style);
GtkWidget* gtk_event_box_new                (void);
gboolean              gtk_widget_get_has_window         (GtkWidget    *widget);
cairo_t  * gdk_cairo_create             (GdkWindow          *window);
void     gtk_container_foreach      (GtkContainer       *container,
                                     GtkCallback         callback,
                                     gpointer            callback_data);
void                  gtk_widget_set_app_paintable      (GtkWidget    *widget,
                                                         gboolean      app_paintable);
GtkWidget* gtk_separator_menu_item_new               (void);
GtkWidget* gtk_menu_new              (void);
GtkWidget* gtk_menu_item_new_with_mnemonic    (const gchar         *label);
void                   gtk_tree_view_set_rules_hint                (GtkTreeView               *tree_view,
                                                                    gboolean                   setting);
void        gtk_box_pack_start          (GtkBox         *box,
                                         GtkWidget      *child,
                                         gboolean        expand,
                                         gboolean        fill,
                                         guint           padding);
void       gtk_menu_item_set_submenu          (GtkMenuItem         *menu_item,
                                               GtkWidget           *submenu);
void    gtk_container_forall             (GtkContainer *container,
                                          GtkCallback   callback,
                                          gpointer        callback_data);
void    gtk_container_remove         (GtkContainer       *container,
                                      GtkWidget       *widget);
GtkWidget* gtk_radio_menu_item_new_with_mnemonic             (GSList           *group,
                                                              const gchar      *label);
GSList*    gtk_radio_menu_item_get_group                     (GtkRadioMenuItem *radio_menu_item);
void       gtk_check_menu_item_set_active        (GtkCheckMenuItem *check_menu_item,
                                                  gboolean          is_active);
void    gtk_misc_set_padding   (GtkMisc *misc,
                                gint     xpad,
                                gint     ypad);
GtkWidget* gtk_image_menu_item_new_with_mnemonic (const gchar      *label);
GtkAccelGroup* gtk_accel_group_new                (void);
void       gtk_widget_add_accelerator      (GtkWidget           *widget,
                                            const gchar         *accel_signal,
                                            GtkAccelGroup       *accel_group,
                                            guint                accel_key,
                                            GdkModifierType      accel_mods,
                                            GtkAccelFlags        accel_flags);
void         gtk_container_child_set            (GtkContainer       *container,
                                                 GtkWidget       *child,
                                                 const gchar       *first_prop_name,
                                                 ...) G_GNUC_NULL_TERMINATED;
GtkWidget* gtk_check_menu_item_new_with_label    (const gchar      *label);
GtkWidget* gtk_image_new_from_stock     (const gchar     *stock_id,
                                         GtkIconSize      size);

gboolean gtk_file_chooser_set_current_folder_uri (GtkFileChooser *chooser,
                                                  const gchar    *uri);
void       gtk_image_menu_item_set_image         (GtkImageMenuItem *image_menu_item,
                                                  GtkWidget        *image);

void       gtk_menu_popup_at_pointer      (GtkMenu             *menu,
                                           const GdkEvent      *trigger_event);
void       gtk_check_menu_item_toggled           (GtkCheckMenuItem *check_menu_item);
GtkWidget* gtk_check_menu_item_new_with_mnemonic (const gchar      *label);
void    gtk_container_set_border_width     (GtkContainer       *container,
                                            guint            border_width);
void       gtk_widget_set_can_default     (GtkWidget           *widget,
                                           gboolean             can_default);
GdkAtom       gtk_selection_data_get_target    (const GtkSelectionData *selection_data);
void     gtk_selection_data_set      (GtkSelectionData     *selection_data,
                                      GdkAtom               type,
                                      gint                  format,
                                      const guchar         *data,
                                      gint                  length);
void           gtk_scrolled_window_set_shadow_type   (GtkScrolledWindow *scrolled_window,
                                                      GtkShadowType      type);
void gtk_drag_source_set  (GtkWidget            *widget,
                           GdkModifierType       start_button_mask,
                           const GtkTargetEntry *targets,
                           gint                  n_targets,
                           GdkDragAction         actions);

void        gtk_box_query_child_packing (GtkBox         *box,
                                         GtkWidget      *child,
                                         gboolean       *expand,
                                         gboolean       *fill,
                                         guint          *padding,
                                         GtkPackType    *pack_type);
#endif /* gtk4compat_h */
