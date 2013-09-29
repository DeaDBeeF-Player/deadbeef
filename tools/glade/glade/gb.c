/*  Gtk+ User Interface Builder
 *  Copyright (C) 2001  Damon Chaplin
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

/*
 * Common data.
 */

#include <config.h>

#include <gtk/gtkenums.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkstock.h>
#include "gb.h"


/* Keys used to store a pointer to the parent widget. Currently we only need
   this for the GtkCombo popup window, as there is no way to step up to the
   GtkCombo. */
const gchar *GladeParentKey		= "GladeParentKey";


/* Keys used to store object data. */
const gchar *GladeButtonStockIDKey	= "GladeButtonStockIDKey";
const gchar *GladeDialogResponseIDKey	= "GladeDialogResponseIDKey";

const gchar *GladeToolButtonStockIDKey	= "GladeToolButtonStockIDKey";
const gchar *GladeToolButtonIconKey	= "GladeToolButtonIconKey";


/* Special child names. */
const gchar *GladeChildDialogVBox	= "vbox";
const gchar *GladeChildDialogActionArea	= "action_area";

const gchar *GladeChildOKButton		= "ok_button";
const gchar *GladeChildCancelButton	= "cancel_button";
const gchar *GladeChildApplyButton	= "apply_button";
const gchar *GladeChildHelpButton	= "help_button";
const gchar *GladeChildSaveButton	= "save_button";
const gchar *GladeChildCloseButton	= "close_button";

const gchar *GladeChildMenuItemImage	= "image";

const gchar *GladeChildComboEntry	= "entry";
const gchar *GladeChildComboList	= "list";

const gchar *GladeChildFontSelection	= "font_selection";
const gchar *GladeChildColorSelection	= "color_selection";

const gchar *GladeChildGnomeAppDock	= "dock";
const gchar *GladeChildGnomeAppBar	= "appbar";
const gchar *GladeChildGnomeEntry	= "entry";
const gchar *GladeChildGnomePBoxNotebook= "notebook";
const gchar *GladeChildGnomeDruidVBox	= "vbox";

const gchar *GladeChildBonoboWindowDock		= "dock";
const gchar *GladeChildBonoboWindowAppBar	= "appbar";

/* These aren't saved in the XML. */
const gchar *GladeChildCListTitle	= "clist_title";


/* Note the trailing NULL in Choices is for the property_add_choice() call,
   though it isn't included in the ChoicesSize. */

/*
 * Relief Choices.
 */
const gchar *GladeReliefChoices[] =
{
  "Normal",
  "Half",
  "None",
  NULL
};
const gint GladeReliefValues[] =
{
  GTK_RELIEF_NORMAL,
  GTK_RELIEF_HALF,
  GTK_RELIEF_NONE
};
const gchar *GladeReliefSymbols[] =
{
  "GTK_RELIEF_NORMAL",
  "GTK_RELIEF_HALF",
  "GTK_RELIEF_NONE"
};
const int GladeReliefChoicesSize = G_N_ELEMENTS (GladeReliefValues);


/*
 * Shadow Choices.
 */
const gchar *GladeShadowChoices[] =
{ "None", "In", "Out", "Etched In", "Etched Out", NULL};
const gint GladeShadowValues[] =
{
  GTK_SHADOW_NONE,
  GTK_SHADOW_IN,
  GTK_SHADOW_OUT,
  GTK_SHADOW_ETCHED_IN,
  GTK_SHADOW_ETCHED_OUT
};
const gchar *GladeShadowSymbols[] =
{
  "GTK_SHADOW_NONE",
  "GTK_SHADOW_IN",
  "GTK_SHADOW_OUT",
  "GTK_SHADOW_ETCHED_IN",
  "GTK_SHADOW_ETCHED_OUT"
};
const int GladeShadowChoicesSize = G_N_ELEMENTS (GladeShadowValues);


/*
 * Corner Choices.
 */
const gchar *GladeCornerChoices[] =
{ "Top Left", "Bottom Left", "Top Right", "Bottom Right", NULL};
const gint GladeCornerValues[] =
{
  GTK_CORNER_TOP_LEFT,
  GTK_CORNER_BOTTOM_LEFT,
  GTK_CORNER_TOP_RIGHT,
  GTK_CORNER_BOTTOM_RIGHT
};
const gchar *GladeCornerSymbols[] =
{
  "GTK_CORNER_TOP_LEFT",
  "GTK_CORNER_BOTTOM_LEFT",
  "GTK_CORNER_TOP_RIGHT",
  "GTK_CORNER_BOTTOM_RIGHT"
};
const int GladeCornerChoicesSize = G_N_ELEMENTS (GladeCornerValues);


GladeDialogResponse GladeStockResponses[] = {
  { "GTK_RESPONSE_OK",	   GTK_RESPONSE_OK,	GTK_STOCK_OK },
  { "GTK_RESPONSE_CANCEL", GTK_RESPONSE_CANCEL,	GTK_STOCK_CANCEL },
  { "GTK_RESPONSE_CLOSE",  GTK_RESPONSE_CLOSE,	GTK_STOCK_CLOSE },
  { "GTK_RESPONSE_YES",	   GTK_RESPONSE_YES,	GTK_STOCK_YES },
  { "GTK_RESPONSE_NO",	   GTK_RESPONSE_NO,	GTK_STOCK_NO },
  { "GTK_RESPONSE_APPLY",  GTK_RESPONSE_APPLY,	GTK_STOCK_APPLY },
  { "GTK_RESPONSE_HELP",   GTK_RESPONSE_HELP,	GTK_STOCK_HELP },
  { "GTK_RESPONSE_REJECT", GTK_RESPONSE_REJECT,	NULL },
  { "GTK_RESPONSE_ACCEPT", GTK_RESPONSE_ACCEPT,	NULL }
};
const gint GladeStockResponsesSize = G_N_ELEMENTS (GladeStockResponses);
