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
#ifndef GLADE_PROPERTY_H
#define GLADE_PROPERTY_H

#include "gbwidget.h"
#include "glade_project.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Currently the property editor creates the standard widget properties itself.
   Eventually I want to rewrite it so that all properties are added elsewhere.
   I also want to change it to use only 2 notebook pages - properties and
   signals. The styles and accelerators properties will move to dialog boxes.
   It should also handle setting properties of multiple selected widgets at
   once - only showing the properies which are common to all the selected
   widgets. Though this requires supporting an 'unset' value for each type
   of property, to be used when the properties of the different widgets differ.
*/

extern GtkWidget *win_property;

/* These are the standard widget property names */
extern const gchar*	GbName;
extern const gchar*	GbClass;
extern const gchar*	GbWidth;
extern const gchar*	GbHeight;
extern const gchar*	GbVisible;
extern const gchar*	GbSensitive;
extern const gchar*	GbTooltip;
extern const gchar*	GbCanDefault;
extern const gchar*	GbHasDefault;
extern const gchar*	GbCanFocus;
extern const gchar*	GbHasFocus;
extern const gchar*	GbEvents;
extern const gchar*	GbExtEvents;

/* Event masks */
#define GB_EVENT_MASKS_COUNT 19
extern const gchar*	GbEventMaskSymbols     [GB_EVENT_MASKS_COUNT];
extern const gint	GbEventMaskValues      [GB_EVENT_MASKS_COUNT];

/* X Extension mode choices data, for updating the choice property */
extern const gchar*	GbExtensionModeChoices[];
extern const gint	GbExtensionModeValues[];
extern const gchar*	GbExtensionModeSymbols[];

/* Language-Specific properties. */
/* C-specific properties. */
extern const gchar*	GbCSourceFile;
extern const gchar*	GbCPublic;

/* C++-specific properties. */
extern const gchar*	GbCxxSeparateFile;
extern const gchar*	GbCxxUseHeap;
extern const gchar*	GbCxxSeparateClass;
extern const gchar*	GbCxxVisibility;

extern const gchar*	GbCxxVisibilityChoices[];
extern const gint	GbCxxVisibilityValues[];
extern const gchar*	GbCxxVisibilitySymbols[];

/* Style properties */
extern const gchar*	GbStylePropagate;
extern const gchar*	GbStyleName;
extern const gchar*	GbStyleFont;

/* Signals page */
extern const gchar*	GbSignalName;
extern const gchar*	GbSignalHandler;
extern const gchar*	GbSignalObject;
extern const gchar*	GbSignalAfter;
extern const gchar*	GbSignalData;

/* Accelerators page */
extern const gchar*	GbAccelKey;
extern const gchar*	GbAccelSignal;


void	    property_init		(void);

/* Showing/hiding the property editor window. */
void	    property_show		(GtkWidget	    *widget,
					 gpointer	     data);
gint	    property_hide		(GtkWidget	    *widget,
					 gpointer	     data);

/* Getting/setting the widget whose properties are being edited. */
GtkWidget*  property_get_widget		(void);
void	    property_set_widget		(GtkWidget	    *widget);

/* Updates the window title, i.e. if the widget's name has been changed. */
void	    property_update_title	(void);

/* Adding property pages specific to a particular widget class. */
gint	    property_add_gbwidget_page	(GtkWidget	    *page);
void	    property_hide_gbwidget_page	(void);
void	    property_show_gbwidget_page	(gint		     page);

/* Adding property pages for child packing properties. */
gint	    property_add_child_packing_page	(GtkWidget	    *page);
void	    property_hide_child_packing_page	(void);
void	    property_show_child_packing_page	(gint		     page);

/* Shows the properties specific to the project's source language - C, C++ */
void	    property_show_lang_specific_page	(GladeLanguageType language);


/* When adding properties, we first set the table to add the properties to,
   and the row to start at. The properties can then simply be added using the
   property_add_int() and similar functions below. The row is automatically
   incremented as new properties are added. */
GtkWidget*  property_get_table_position (gint		    *row);
void	    property_set_table_position	(GtkWidget	    *table,
					 gint		     row);


/* Note: Unless stated otherwise, values passed to property_set_XXX() will
   not be changed (and copied if needed), and you should copy values returned
   from property_get_XXX() calls if you need them (or ref pixmaps/bitmaps). */
void	    property_add_string		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
gchar*	    property_get_string		(const gchar	    *property_name,
					 GtkWidget	    *actual_widget,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_string		(const gchar	    *property_name,
					 const gchar	    *value);
void	    property_set_translatable_string (const gchar   *property_name,
					      const gchar   *value,
					      GtkWidget     *widget);

void	    property_add_text		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 gint		     visible_lines);
/* Note: string returned from property_get_text must be freed with g_free() */
gchar*	    property_get_text		(const gchar	    *property_name,
					 GtkWidget	    *actual_widget,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_text		(const gchar	    *property_name,
					 const gchar	    *value);
void	    property_set_translatable_text (const gchar     *property_name,
					    const gchar     *value,
					    GtkWidget       *widget);

void	    property_add_int		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
void	    property_add_int_range	(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 gint		     min,
					 gint		     max,
					 gint		     step_increment,
					 gint		     page_increment,
					 gint		     climb_rate);
void	    property_add_optional_int_range (const gchar    *property_name,
					     const gchar    *label,
					     const gchar    *tooltip,
					     gint	     min,
					     gint	     max,
					     gint	     step_increment,
					     gint	     page_increment,
					     gint	     climb_rate,
					     GtkCallback     callback);
gint	    property_get_int		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
gint	    property_get_optional_int	(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply,
					 gboolean	    *is_set);
void	    property_set_int		(const gchar	    *property_name,
					 gint		     value);
void	    property_set_optional_int	(const gchar	    *property_name,
					 gint		     value,
					 gboolean	     is_set);

void	    property_add_float		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
void	    property_add_float_range	(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 gfloat		     min,
					 gfloat		     max,
					 gfloat		     step_increment,
					 gfloat		     page_increment,
					 gfloat		     climb_rate,
					 gint		     decimal_digits);
gfloat	    property_get_float		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_float		(const gchar	    *property_name,
					 gfloat		     value);

void	    property_add_bool		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
gboolean    property_get_bool		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
/* property_set_bool() takes an int so we can pass it GTK values which aren't
   really gbooleans and we can also use results of ANDing bit masks etc. */
void	    property_set_bool		(const gchar	    *property_name,
					 gint		     value);

void	    property_add_choice		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 const gchar	   **choices);
gchar*	    property_get_choice		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_choice		(const gchar	    *property_name,
					 gint		     value);

void	    property_add_combo		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 GList		    *choices);
gchar*	    property_get_combo		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_combo		(const gchar	    *property_name,
					 const gchar	    *value);
void	    property_set_combo_strings	(const gchar	    *property_name,
					 GList		    *choices);

void	    property_add_color		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
GdkColor*   property_get_color		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_color		(const gchar	    *property_name,
					 GdkColor	    *value);

void	    property_add_bgpixmap	(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
GdkPixmap*  property_get_bgpixmap	(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply,
					 gchar		   **filename);
void	    property_set_bgpixmap	(const gchar	    *property_name,
					 GdkPixmap	    *gdkpixmap,
					 const gchar	    *filename);

/* A dialog property is an Entry with a button on the right of it which, when
   clicked, calls the callback given in the property_add_dialog() function.
   This callback should show an appropriate dialog box for editing the
   property. Then, the dialog should change the text in the Entry.
   If the property value can't be completely represented using text, the value
   should be placed in the Entry's object data hash under GbDialogValueKey,
   (In this case the the Entry should not be editable, as set in
   property_add_dialog) */
void	    property_add_dialog		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 gboolean	     editable,
					 GtkCallback	     callback);
gpointer    property_get_dialog		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_dialog		(const gchar	    *property_name,
					 const gchar	    *string,
					 gconstpointer	     value);

void	    property_add_filename	(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
void	    property_add_filename_with_combo (const gchar * property_name,
					      const gchar * label,
					      const gchar * tooltip,
					      GList * choices);
gchar*	    property_get_filename	(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_filename	(const gchar	    *property_name,
					 const gchar	    *value);

void	    property_add_font		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
GdkFont*    property_get_font		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply,
					 gchar		   **xlfd_fontname);
void	    property_set_font		(const gchar	    *property_name,
					 GdkFont	    *font,
					 const gchar	    *xlfd_fontname);

void	    property_add_command	(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 const gchar	    *command,
					 GtkSignalFunc	     callback);

/* A stock item is a GTK+ stock item that has a label and an icon with the
   given size. Any stock items that don't have labels or don't have an icon
   with the given size are not displayed in the popup list.
   You can change the icon size dynamically, e.g. based on another property.
   The popup list will be recreated, and if the stock item currently used does
   not have the given icon size, the stock item will be set to 'None'.
   Using GLADE_ICON_SIZE_ANY means any icon size is OK. */
#define	GLADE_ICON_SIZE_ANY	255

void	    property_add_stock_item	(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 GtkIconSize	     icon_size);
gchar*	    property_get_stock_item	(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_stock_item	(const gchar	    *property_name,
					 const gchar	    *stock_id);
void	    property_set_stock_item_icon_size (const gchar  *property_name,
					       GtkIconSize   icon_size);


/* An icon is a GTK+ stock icon with the given icon size, or a user-specified
   icon. It will show a GtkCombo, with a '...' button beside it for opening
   a file selection dialog, so the user can specify an icon file.
   As with stock item, you can change the icon size dynamically, and the popup
   list of stock icons will be recreated. The file selection can be shown or
   hidden with property_set_icon_filesel(). */
void	    property_add_icon		(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip,
					 GtkIconSize	     icon_size);
gchar*	    property_get_icon		(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_icon		(const gchar	    *property_name,
					 const gchar	    *icon);
void	    property_set_icon_size	(const gchar	    *property_name,
					 GtkIconSize	     icon_size);
void	    property_set_icon_filesel	(const gchar	    *property_name,
					 gboolean	     filesel);

void	    property_add_named_icon	(const gchar	    *property_name,
					 const gchar	    *label,
					 const gchar	    *tooltip);
gchar*	    property_get_named_icon	(const gchar	    *property_name,
					 GtkWidget	    *to_apply,
					 gboolean	    *apply);
void	    property_set_named_icon	(const gchar	    *property_name,
					 const gchar	    *icon_name);


#define GB_ADJUST_DEFAULT_LABELS 1
#define GB_ADJUST_H_LABELS	 2
#define GB_ADJUST_V_LABELS	 3
void	    property_add_adjustment	(const gchar	    *Values[],
					 gint		     label_type);

/* Returns the widget used for displaying a property value. */
GtkWidget*  property_get_value_widget	(const gchar	    *property_name);

void	    property_set_sensitive	(const gchar	    *property_name,
					 gboolean	     sensitive);
void	    property_set_sensitive_full (const gchar	    *property_name,
					 gboolean	     label_sensitive,
					 gboolean	     value_sensitive,
					 gboolean	     button_visible);
void	    property_set_visible	(const gchar	    *property_name,
					 gboolean	     visible);

/* This isn't currently used. It changes the style so that text is shown in
   red to indicate an invalid property value. */
void	    property_set_valid		(const gchar	    *property_name,
					 gboolean	     valid);

gchar*	    property_events_value_to_string	(gint	      event_mask);
gint	    property_events_string_to_value	(const gchar *event_string);

/* Turn on/off auto-apply which applies changes as they are made in the
   property editor. You may need to turn this off if you are updating
   properties, otherwise the properties will be applied again. */
void	    property_set_auto_apply	(gboolean	     value);


void	    property_clear_accelerators	(void);
void	    property_add_accelerator	(GladeAccelerator    *accel);
gboolean    property_is_accel_clist	(GtkWidget	    *widget);
GList*      property_get_accelerators	(void);

void	    property_clear_signals	(void);
void	    property_add_signal		(GladeSignal	    *signal);
gboolean    property_is_signal_clist	(GtkWidget	    *widget);
GList*      property_get_signals	(void);


/* This is to allow a widget's label to be typed in while the mouse hovers
   over the widget. I'm not sure how well this will work yet. */
void	    property_redirect_key_press (GdkEventKey	    *event);
gboolean    property_is_typing_over_widget (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_PROPERTY_H */
