/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-1999  Damon Chaplin
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

#include "gladeconfig.h"

#include "glade.h"
#include "gbwidget.h"

/* I've commented this out to avoid warnings. */
/*static gchar *libname = "GTK+ 1.2";*/

GbWidget *gb_label_init ();
GbWidget *gb_entry_init ();
GbWidget *gb_button_init ();
GbWidget *gb_toggle_button_init ();
GbWidget *gb_check_button_init ();
GbWidget *gb_radio_button_init ();
GbWidget *gb_option_menu_init ();
GbWidget *gb_combo_init ();
GbWidget *gb_combo_box_init ();
GbWidget *gb_combo_box_entry_init ();
GbWidget *gb_list_init ();
GbWidget *gb_clist_init ();
GbWidget *gb_spin_button_init ();
GbWidget *gb_hscale_init ();
GbWidget *gb_vscale_init ();
GbWidget *gb_hruler_init ();
GbWidget *gb_vruler_init ();
GbWidget *gb_hscrollbar_init ();
GbWidget *gb_vscrollbar_init ();
GbWidget *gb_menu_bar_init ();
GbWidget *gb_statusbar_init ();
GbWidget *gb_toolbar_init ();
GbWidget *gb_progress_bar_init ();
GbWidget *gb_arrow_init ();
GbWidget *gb_image_init ();
GbWidget *gb_drawing_area_init ();
GbWidget *gb_hseparator_init ();
GbWidget *gb_vseparator_init ();
GbWidget *gb_hbox_init ();
GbWidget *gb_vbox_init ();
GbWidget *gb_table_init ();
GbWidget *gb_fixed_init ();
GbWidget *gb_hbutton_box_init ();
GbWidget *gb_vbutton_box_init ();
GbWidget *gb_frame_init ();
GbWidget *gb_aspect_frame_init ();
GbWidget *gb_hpaned_init ();
GbWidget *gb_vpaned_init ();
GbWidget *gb_handle_box_init ();
GbWidget *gb_notebook_init ();
GbWidget *gb_alignment_init ();
GbWidget *gb_event_box_init ();
GbWidget *gb_scrolled_window_init ();
GbWidget *gb_viewport_init ();
GbWidget *gb_expander_init ();
GbWidget *gb_curve_init ();
GbWidget *gb_gamma_curve_init ();
GbWidget *gb_color_selection_init ();
GbWidget *gb_preview_init ();
GbWidget *gb_window_init ();
GbWidget *gb_dialog_init ();
GbWidget *gb_file_chooser_button_init ();
GbWidget *gb_file_chooser_widget_init ();
GbWidget *gb_file_chooser_dialog_init ();
GbWidget *gb_file_selection_init ();
GbWidget *gb_color_selection_dialog_init ();
GbWidget *gb_input_dialog_init ();
GbWidget *gb_list_item_init ();
GbWidget *gb_menu_init ();
GbWidget *gb_menu_item_init ();
GbWidget *gb_check_menu_item_init ();
GbWidget *gb_radio_menu_item_init ();
GbWidget *gb_image_menu_item_init ();
GbWidget *gb_separator_menu_item_init ();
GbWidget *gb_ctree_init ();
GbWidget *gb_accel_label_init ();
GbWidget *gb_font_selection_init ();
GbWidget *gb_font_selection_dialog_init ();
GbWidget *gb_calendar_init();
GbWidget *gb_custom_init();
GbWidget *gb_layout_init();
GbWidget *gb_text_view_init();
GbWidget *gb_tree_view_init();
GbWidget *gb_color_button_init();
GbWidget *gb_font_button_init();
GbWidget *gb_tool_item_init ();
GbWidget *gb_tool_button_init ();
GbWidget *gb_toggle_tool_button_init ();
GbWidget *gb_radio_tool_button_init ();
GbWidget *gb_separator_tool_item_init ();
GbWidget *gb_about_dialog_init ();
GbWidget *gb_icon_view_init ();
GbWidget *gb_menu_tool_button_init ();
GbWidget *gb_cell_view_init ();


static GladeWidgetInitData gtk_standard[] =
{
  { "GtkWindow", gb_window_init },
  { "GtkMenuBar", gb_menu_bar_init },
  { "GtkToolbar", gb_toolbar_init },
  { "GtkHandleBox", gb_handle_box_init },

  { "GtkToolButton", gb_tool_button_init },
  { "GtkToggleToolButton", gb_toggle_tool_button_init },
  { "GtkRadioToolButton", gb_radio_tool_button_init },
  { "GtkSeparatorToolItem", gb_separator_tool_item_init },

  { "GtkLabel", gb_label_init },
  { "GtkEntry", gb_entry_init },
  { "GtkComboBoxEntry", gb_combo_box_entry_init },
  { "GtkTextView", gb_text_view_init },

  { "GtkButton", gb_button_init },
  { "GtkToggleButton", gb_toggle_button_init },
  { "GtkCheckButton", gb_check_button_init },
  { "GtkRadioButton", gb_radio_button_init },

  { "GtkComboBox", gb_combo_box_init },
  { "GtkSpinButton", gb_spin_button_init },
  { "GtkTreeView", gb_tree_view_init },
  { "GtkIconView", gb_icon_view_init },

  { "GtkHSeparator", gb_hseparator_init },
  { "GtkVSeparator", gb_vseparator_init },
  { "GtkImage", gb_image_init },
  { "GtkDrawingArea", gb_drawing_area_init },

  { "GtkDialog", gb_dialog_init },
  { "GtkFileChooserDialog", gb_file_chooser_dialog_init },
  { "GtkColorSelectionDialog", gb_color_selection_dialog_init },
  { "GtkFontSelectionDialog", gb_font_selection_dialog_init },

  { "GtkHBox", gb_hbox_init },
  { "GtkVBox", gb_vbox_init },
  { "GtkTable", gb_table_init },
  { "GtkFixed", gb_fixed_init },

  { "GtkHButtonBox", gb_hbutton_box_init },
  { "GtkVButtonBox", gb_vbutton_box_init },
  { "GtkHPaned", gb_hpaned_init },
  { "GtkVPaned", gb_vpaned_init },

  { "GtkNotebook", gb_notebook_init },
  { "GtkFrame", gb_frame_init },
  { "GtkScrolledWindow", gb_scrolled_window_init },
  { "GtkStatusbar", gb_statusbar_init },


  { NULL, NULL }
};


static GladeWidgetInitData gtk_advanced[] =
{
  { "GtkAboutDialog", gb_about_dialog_init },
  { "GtkInputDialog", gb_input_dialog_init },
  { "GtkMenuToolButton", gb_menu_tool_button_init },
  { "GtkToolItem", gb_tool_item_init },

  { "GtkHScale", gb_hscale_init },
  { "GtkVScale", gb_vscale_init },
  { "GtkHRuler", gb_hruler_init },
  { "GtkVRuler", gb_vruler_init },

  { "GtkAlignment", gb_alignment_init },
  { "GtkEventBox", gb_event_box_init },
  { "GtkCalendar", gb_calendar_init },
  { "GtkProgressBar", gb_progress_bar_init },

  { "GtkLayout", gb_layout_init },
  { "GtkAspectFrame", gb_aspect_frame_init },
  { "GtkArrow", gb_arrow_init },
  { "GtkExpander", gb_expander_init },


  { "GtkCurve", gb_curve_init },
  { "GtkGammaCurve", gb_gamma_curve_init },
  { "GtkHScrollbar", gb_hscrollbar_init },
  { "GtkVScrollbar", gb_vscrollbar_init },

  { "GtkFileChooserWidget", gb_file_chooser_widget_init },
  { "GtkColorSelection", gb_color_selection_init },
  { "GtkFontSelection", gb_font_selection_init },
  { "GtkCellView", gb_cell_view_init },

  { "GtkFileChooserButton", gb_file_chooser_button_init },
  { "GtkColorButton", gb_color_button_init },
  { "GtkFontButton", gb_font_button_init },
  { "GtkMenu", gb_menu_init },

  { "GtkViewport", gb_viewport_init },

  { "Custom", gb_custom_init }, /* Our special custom widget. */

  { NULL, NULL }
};

static GladeWidgetInitData gtk_deprecated[] =
{
  { "GtkCList", gb_clist_init },
  { "GtkCTree", gb_ctree_init },
  { "GtkList", gb_list_init },
  { "GtkCombo", gb_combo_init },

  { "GtkFileSelection", gb_file_selection_init },
  { "GtkOptionMenu", gb_option_menu_init },
  { "GtkPreview", gb_preview_init },

  { NULL, NULL }
};

static GladeWidgetInitData notshown[] =
{
  { "GtkAccelLabel", gb_accel_label_init },
  { "GtkListItem", gb_list_item_init },
  { "GtkMenuItem", gb_menu_item_init },
  { "GtkCheckMenuItem", gb_check_menu_item_init },
  { "GtkRadioMenuItem", gb_radio_menu_item_init },
  { "GtkImageMenuItem", gb_image_menu_item_init },
  { "GtkSeparatorMenuItem", gb_separator_menu_item_init },

  { NULL, NULL }
};

static GladePaletteSectionData sections[] =
{
  /* Note that glade_palette_set_show_gnome_widgets() has some of these
     strings hard-coded now, so keep up-to-date. */
  { N_("GTK+ _Basic"), gtk_standard },
  { N_("GTK+ _Additional"), gtk_advanced },
  { N_("Dep_recated"), gtk_deprecated },
  { "NotShown", notshown },
  { NULL, NULL }
};

GladePaletteSectionData *get_gtk_widgets()
{
	return sections;
}
