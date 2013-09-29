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

#ifdef USE_GNOME

#include "glade.h"
#include "gbwidget.h"

/* I've commented this out to avoid warnings. */
/*gchar *libname = "GNOME 1.0";*/

GbWidget *gb_gnome_about_init ();
GbWidget *gb_gnome_app_init ();
GbWidget *gb_gnome_appbar_init ();
GbWidget *gb_gnome_canvas_init ();
GbWidget *gb_gnome_color_picker_init ();
GbWidget *gb_gnome_date_edit_init ();
GbWidget *gb_gnome_dialog_init ();
GbWidget *gb_bonobo_dock_init ();
GbWidget *gb_bonobo_dock_band_init ();
GbWidget *gb_bonobo_dock_item_init ();
GbWidget *gb_gnome_druid_init ();
GbWidget *gb_gnome_druid_page_edge_init ();
GbWidget *gb_gnome_druid_page_standard_init ();
GbWidget *gb_gnome_entry_init ();
GbWidget *gb_gnome_file_entry_init ();
GbWidget *gb_gnome_font_picker_init ();
GbWidget *gb_gnome_href_init ();
GbWidget *gb_gnome_icon_entry_init ();
GbWidget *gb_gnome_icon_list_init ();
GbWidget *gb_gnome_icon_selection_init ();
GbWidget *gb_gnome_message_box_init ();
#if 0
GbWidget *gb_gnome_paper_selector_init ();
#endif
GbWidget *gb_gnome_pixmap_init ();
GbWidget *gb_gnome_pixmap_entry_init ();
GbWidget *gb_gnome_property_box_init ();
GbWidget *gb_bonobo_control_init ();

static GladeWidgetInitData gnome_page[] =
{
  { "GnomeApp", gb_gnome_app_init },
  { "GnomeDruid", gb_gnome_druid_init },
  { "GnomeCanvas", gb_gnome_canvas_init },
  { "GnomeIconSelection", gb_gnome_icon_selection_init },

  { "GnomeIconEntry", gb_gnome_icon_entry_init },
  { "GnomeHRef", gb_gnome_href_init },
  { "GnomeDateEdit", gb_gnome_date_edit_init },
  { "GnomeAppBar", gb_gnome_appbar_init },


#if 0
  { "GnomePaperSelector", gb_gnome_paper_selector_init },
#endif


  { "BonoboWidget", gb_bonobo_control_init },

  { NULL, NULL }
};

static GladeWidgetInitData gnome_deprecated[] =
{
  { "GnomePixmap", gb_gnome_pixmap_init },

  { "GnomeDialog", gb_gnome_dialog_init },
  { "GnomeMessageBox", gb_gnome_message_box_init },
  { "GnomePropertyBox", gb_gnome_property_box_init },
  { "GnomeAbout", gb_gnome_about_init },

  { "GnomeIconList", gb_gnome_icon_list_init },
  { "GnomeEntry", gb_gnome_entry_init },
  { "GnomeFileEntry", gb_gnome_file_entry_init },
  { "GnomePixmapEntry", gb_gnome_pixmap_entry_init },

  { "GnomeColorPicker", gb_gnome_color_picker_init },
  { "GnomeFontPicker", gb_gnome_font_picker_init },

  { NULL, NULL }
};

static GladeWidgetInitData notshown[] =
{
  { "BonoboDock", gb_bonobo_dock_init },
  { "BonoboDockItem", gb_bonobo_dock_item_init },
  { "GnomeDruidPageEdge", gb_gnome_druid_page_edge_init },
  { "GnomeDruidPageStandard", gb_gnome_druid_page_standard_init },

  /* Unfinished. */
  { NULL, NULL }
};

static GladePaletteSectionData sections[] =
{
  /* Note that glade_palette_set_show_gnome_widgets() has some of these
     strings hard-coded now, so keep up-to-date. */
  { N_("_Gnome"), gnome_page },
  { N_("Dep_recated"), gnome_deprecated },
  { "NotShown", notshown },
  { NULL, NULL }
};

GladePaletteSectionData *get_gnome_widgets()
{
	return sections;
}

#endif
