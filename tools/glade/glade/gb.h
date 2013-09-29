/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998  Damon Chaplin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef GLADE_GB_H
#define GLADE_GB_H

/* This header file is included by all gbwidgets in the gbwidget directory,
   so if we add any header files to Glade, we only have to change this. */

#include "gladeconfig.h"

#include <string.h>

#include "editor.h"
#include "gbwidget.h"
#include "glade_project.h"
#include "load.h"
#include "property.h"
#include "save.h"
#include "source.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* The package name we need to pass to dgettext to get GTK+'s translated
   strings. */
#define GLADE_GTK_GETTEXT_PACKAGE	"gtk20"


  /* The index of the dialog hint in GbTypeHintChoices. */
#define GLADE_TYPE_HINT_DIALOG_INDEX	1

/* This is the error margin we use for floating-point comparisons. */
#define GLADE_EPSILON	0.000001

/* This value is copied from gnome-entry.c. */
#define GLADE_DEFAULT_MAX_HISTORY_SAVED	10

/* Keys used to store a pointer to the parent widget. Currently we only need
   this for the GtkCombo popup window, as there is no way to step up to the
   GtkCombo. */
extern const gchar *GladeParentKey;

/* Keys used to store object data. */
extern const gchar *GladeButtonStockIDKey;
extern const gchar *GladeDialogResponseIDKey;

extern const gchar *GladeToolButtonStockIDKey;
extern const gchar *GladeToolButtonIconKey;

/* The moniker key, only used for Bonobo controls. */
extern const gchar *Moniker;

extern const gint GladeReliefChoicesSize;
extern const gchar *GladeReliefChoices[];
extern const gint   GladeReliefValues[];
extern const gchar *GladeReliefSymbols[];

extern const gint GladeShadowChoicesSize;
extern const gchar *GladeShadowChoices[];
extern const gint   GladeShadowValues[];
extern const gchar *GladeShadowSymbols[];

extern const gint GladeCornerChoicesSize;
extern const gchar *GladeCornerChoices[];
extern const gint   GladeCornerValues[];
extern const gchar *GladeCornerSymbols[];

/* Special child names. */
extern const gchar *GladeChildDialogVBox;
extern const gchar *GladeChildDialogActionArea;

extern const gchar *GladeChildOKButton;
extern const gchar *GladeChildCancelButton;
extern const gchar *GladeChildApplyButton;
extern const gchar *GladeChildHelpButton;
extern const gchar *GladeChildSaveButton;
extern const gchar *GladeChildCloseButton;

extern const gchar *GladeChildMenuItemImage;

extern const gchar *GladeChildComboEntry;
extern const gchar *GladeChildComboList;

extern const gchar *GladeChildFontSelection;
extern const gchar *GladeChildColorSelection;

extern const gchar *GladeChildGnomeAppDock;
extern const gchar *GladeChildGnomeAppBar;
extern const gchar *GladeChildGnomeEntry;
extern const gchar *GladeChildGnomePBoxNotebook;
extern const gchar *GladeChildGnomeDruidVBox;

extern const gchar *GladeChildBonoboWindowDock;
extern const gchar *GladeChildBonoboWindowAppBar;

/* These aren't saved in the XML. */
extern const gchar *GladeChildCListTitle;



/* GtkFixed/GtkLayout Child property names. */
extern const gchar *GladeFixedChildX;
extern const gchar *GladeFixedChildY;
extern const gchar *GladeLayoutChildX;
extern const gchar *GladeLayoutChildY;


/* Information on the stock GTK+ responses. */
typedef struct _GladeDialogResponse GladeDialogResponse;
struct _GladeDialogResponse
{
  gchar *name;
  gint response_id;
  /* This is the stock id that usually goes with the response id. If the user
     sets the stock id to this, then we automatically set the response id. */
  gchar *stock_id;
};

typedef struct _GladeFindGroupData GladeFindGroupData;
struct _GladeFindGroupData {
  GSList *group;
  GtkWidget *found_widget;
};

extern GladeDialogResponse GladeStockResponses[];
extern const gint GladeStockResponsesSize;

/* Some common functions. */
void	    gb_box_set_size			(GtkWidget *widget,
						 gint size);
void	    gb_box_create_child_properties	(GtkWidget * widget,
						 GbWidgetCreateChildArgData * data);
void	    gb_box_get_child_properties		(GtkWidget *widget,
						 GtkWidget *child,
						 GbWidgetGetArgData *data);
void	    gb_box_set_child_properties		(GtkWidget * widget,
						 GtkWidget * child,
						 GbWidgetSetArgData * data);
void	    gb_box_create_popup_menu		(GtkWidget * widget,
						 GbWidgetCreateMenuData * data);
void	    gb_box_write_add_child_source	(GtkWidget * parent,
						 const gchar * parent_name,
						 GtkWidget * child,
						 GbWidgetWriteSourceData * data);

void	    gb_button_get_standard_properties	(GtkWidget * widget,
						 GbWidgetGetArgData * data,
						 gchar *stock_id_p,
						 gchar *label_p,
						 gchar *icon_p,
						 gchar *relief_p,
						 gchar *focus_on_click_p);
void	    gb_button_set_standard_properties	(GtkWidget * widget,
						 GbWidgetSetArgData * data,
						 gchar *stock_id_p,
						 gchar *label_p,
						 gchar *icon_p,
						 gchar *relief_p,
						 gchar *focus_on_click_p);
void	    gb_button_create_popup_menu		(GtkWidget * widget,
						 GbWidgetCreateMenuData * data);
void	    gb_button_find_radio_group		(GtkWidget *widget,
						 GladeFindGroupData *find_data);
void	    gb_button_write_standard_source	(GtkWidget * widget,
						 GbWidgetWriteSourceData * data,
						 const gchar *label_p);
void	    gb_button_destroy			(GtkWidget * widget,
						 GbWidgetDestroyData * data);

char*	    gb_dialog_response_id_to_string	(gint response_id);
gint	    gb_dialog_response_id_from_string	(const gchar *response_id);


void	    gb_frame_create_popup_menu		(GtkWidget *widget,
						 GbWidgetCreateMenuData *data);
void	    gb_frame_add_child			(GtkWidget *widget,
						 GtkWidget *child,
						 GbWidgetSetArgData *data);
void	    gb_frame_get_child_properties	(GtkWidget *widget,
						 GtkWidget *child,
						 GbWidgetGetArgData *data);
void	    gb_frame_write_add_child_source	(GtkWidget * parent,
						 const gchar *parent_name,
						 GtkWidget *child,
						 GbWidgetWriteSourceData * data);


void	    gb_label_create_standard_properties (GtkWidget * widget,
						 GbWidgetCreateArgData * data,
						 const char *label_p,
						 const char *use_underline_p,
						 const char *use_markup_p,
						 const char *justify_p,
						 const char *wrap_p,
						 const char *selectable_p,
						 const char *xalign_p,
						 const char *yalign_p,
						 const char *xpad_p,
						 const char *ypad_p,
						 const char *focus_target_p,
						 const char *ellipsize_p,
						 const char *width_chars_p,
						 const char *single_line_mode_p,
						 const char *angle_p);


void	    gb_label_get_standard_properties	(GtkWidget * widget,
						 GbWidgetGetArgData * data,
						 const char *label_p,
						 const char *use_underline_p,
						 const char *use_markup_p,
						 const char *justify_p,
						 const char *wrap_p,
						 const char *selectable_p,
						 const char *xalign_p,
						 const char *yalign_p,
						 const char *xpad_p,
						 const char *ypad_p,
						 const char *focus_target_p,
						 const char *ellipsize_p,
						 const char *width_chars_p,
						 const char *single_line_mode_p,
						 const char *angle_p);

void	    gb_label_set_standard_properties	(GtkWidget * widget,
						 GbWidgetSetArgData * data,
						 const char *label_p,
						 const char *use_underline_p,
						 const char *use_markup_p,
						 const char *justify_p,
						 const char *wrap_p,
						 const char *selectable_p,
						 const char *xalign_p,
						 const char *yalign_p,
						 const char *xpad_p,
						 const char *ypad_p,
						 const char *focus_target_p,
						 const char *ellipsize_p,
						 const char *width_chars_p,
						 const char *single_line_mode_p,
						 const char *angle_p);

void	    gb_label_write_standard_source	(GtkWidget * widget,
						 GbWidgetWriteSourceData *data,
						 const char *label_p,
						 const char *use_underline_p,
						 const char *use_markup_p,
						 const char *justify_p,
						 const char *wrap_p,
						 const char *selectable_p,
						 const char *xalign_p,
						 const char *yalign_p,
						 const char *xpad_p,
						 const char *ypad_p,
						 const char *focus_target_p,
						 const char *ellipsize_p,
						 const char *width_chars_p,
						 const char *single_line_mode_p,
						 const char *angle_p);



void	    gb_paned_create_child_properties	(GtkWidget * widget,
						 GbWidgetCreateChildArgData * data);
void	    gb_paned_get_child_properties	(GtkWidget *widget,
						 GtkWidget *child,
						 GbWidgetGetArgData *data);
void	    gb_paned_set_child_properties	(GtkWidget * widget,
						 GtkWidget * child,
						 GbWidgetSetArgData * data);
void	    gb_paned_write_add_child_source	(GtkWidget * parent,
						 const gchar *parent_name,
						 GtkWidget *child,
						 GbWidgetWriteSourceData * data);

GSList *    gb_radio_button_reset_radio_group	(GtkWidget * widget);
void	    gb_radio_button_update_radio_group	(GSList * group);

void	    gb_table_update_placeholders	(GtkWidget * table,
						 gint rows,
						 gint cols);

void	    gb_tool_button_get_standard_properties (GtkWidget *widget,
						    GbWidgetGetArgData * data,
						    gchar *stock_id_p,
						    gchar *label_p,
						    gchar *icon_p,
						    gchar *visible_horz_p,
						    gchar *visible_vert_p,
						    gchar *is_important_p);
void	    gb_tool_button_set_standard_properties (GtkWidget *widget,
						    GbWidgetSetArgData * data,
						    gchar *stock_id_p,
						    gchar *label_p,
						    gchar *icon_p,
						    gchar *visible_horz_p,
						    gchar *visible_vert_p,
						    gchar *is_important_p);
void	    gb_tool_button_destroy		(GtkWidget * widget,
						 GbWidgetDestroyData * data);


void	    gb_window_create_standard_properties(GtkWidget * widget,
						 GbWidgetCreateArgData * data,
						 gchar *title_p,
						 gchar *type_p,
						 gchar *position_p,
						 gchar *modal_p,
						 gchar *default_width_p,
						 gchar *default_height_p,
						 gchar *shrink_p,
						 gchar *grow_p,
						 gchar *auto_shrink_p,
						 gchar *icon_name_p,
						 gchar *focus_on_map_p,
						 gchar *resizable_p,
						 gchar *destroy_with_parent_p,
						 gchar *icon_p,
						 gchar *role_p,
						 gchar *type_hint_p,
						 gchar *skip_taskbar_p,
						 gchar *skip_pager_p,
						 gchar *decorated_p,
						 gchar *gravity_p,
						 gchar *urgency_p);
void	    gb_window_get_standard_properties	(GtkWidget * widget,
						 GbWidgetGetArgData * data,
						 gchar *title_p,
						 gchar *type_p,
						 gchar *position_p,
						 gchar *modal_p,
						 gchar *default_width_p,
						 gchar *default_height_p,
						 gchar *shrink_p,
						 gchar *grow_p,
						 gchar *auto_shrink_p,
						 gchar *icon_name_p,
						 gchar *focus_on_map_p,
						 gchar *resizable_p,
						 gchar *destroy_with_parent_p,
						 gchar *icon_p,
						 gchar *role_p,
						 gchar *type_hint_p,
						 gchar *skip_taskbar_p,
						 gchar *skip_pager_p,
						 gchar *decorated_p,
						 gchar *gravity_p,
						 gchar *urgency_p);
void	    gb_window_set_standard_properties	(GtkWidget * widget,
						 GbWidgetSetArgData * data,
						 gchar *title_p,
						 gchar *type_p,
						 gchar *position_p,
						 gchar *modal_p,
						 gchar *default_width_p,
						 gchar *default_height_p,
						 gchar *shrink_p,
						 gchar *grow_p,
						 gchar *auto_shrink_p,
						 gchar *icon_name_p,
						 gchar *focus_on_map_p,
						 gchar *resizable_p,
						 gchar *destroy_with_parent_p,
						 gchar *icon_p,
						 gchar *role_p,
						 gchar *type_hint_p,
						 gchar *skip_taskbar_p,
						 gchar *skip_pager_p,
						 gchar *decorated_p,
						 gchar *gravity_p,
						 gchar *urgency_p);
void	    gb_window_write_standard_source	(GtkWidget * widget,
						 GbWidgetWriteSourceData *data,
						 gchar *title_p,
						 gchar *type_p,
						 gchar *position_p,
						 gchar *modal_p,
						 gchar *default_width_p,
						 gchar *default_height_p,
						 gchar *shrink_p,
						 gchar *grow_p,
						 gchar *auto_shrink_p,
						 gchar *icon_name_p,
						 gchar *focus_on_map_p,
						 gchar *resizable_p,
						 gchar *destroy_with_parent_p,
						 gchar *icon_p,
						 gchar *role_p,
						 gchar *type_hint_p,
						 gchar *skip_taskbar_p,
						 gchar *skip_pager_p,
						 gchar *decorated_p,
						 gchar *gravity_p,
						 gchar *urgency_p);
void	    gb_window_destroy			(GtkWidget * widget,
						 GbWidgetDestroyData * data);


gboolean    gb_set_scroll_adjustments_hook	(GSignalInvocationHint  *ihint,
						 guint			n_param_values,
						 const GValue	       *param_values,
						 gpointer		data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_GB_H */
