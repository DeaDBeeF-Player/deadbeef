/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-1999  Damon Chaplin
 *  Copyright (C) 2001  Carlos Perelló Marín <carlos@gnome-db.org>
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

#ifdef USE_GNOME_DB

#include "glade.h"
#include "gbwidget.h"

/* I've commented this out to avoid warnings. */
/*gchar *libname = "GNOME 1.0";*/

GbWidget *gb_gnome_db_combo_init ();
GbWidget *gb_gnome_db_connection_properties_init();
GbWidget *gb_gnome_db_data_source_selector_init ();
GbWidget *gb_gnome_db_dsn_config_druid_init ();
GbWidget *gb_gnome_db_dsnconfig_init ();
GbWidget *gb_gnome_db_editor_init();
GbWidget *gb_gnome_db_error_init ();
GbWidget *gb_gnome_db_errordlg_init ();
GbWidget *gb_gnome_db_form_init ();
GbWidget *gb_gnome_db_gray_bar_init();
GbWidget *gb_gnome_db_grid_init ();
GbWidget *gb_gnome_db_login_init ();
GbWidget *gb_gnome_db_logindlg_init ();
GbWidget *gb_gnome_db_provider_selector_init ();
GbWidget *gb_gnome_db_table_editor_init ();

static GladeWidgetInitData gnome_db[] = {
        { "GnomeDbLoginDialog", gb_gnome_db_logindlg_init },
	{ "GnomeDbLogin",       gb_gnome_db_login_init },
        { "GnomeDbErrorDialog", gb_gnome_db_errordlg_init }, 
        { "GnomeDbError",       gb_gnome_db_error_init },

        { "GnomeDbDsnConfig",   gb_gnome_db_dsnconfig_init },
	{ "GnomeDbDsnConfigDruid",	gb_gnome_db_dsn_config_druid_init },
	{ "GnomeDbProviderSelector",	gb_gnome_db_provider_selector_init },

	{ "GnomeDbDataSourceSelector",	gb_gnome_db_data_source_selector_init },
	{ "GnomeDbTableEditor",	gb_gnome_db_table_editor_init },
	{ "GnomeDbForm",	gb_gnome_db_form_init },

        { "GnomeDbGrid",        gb_gnome_db_grid_init },
        { "GnomeDbCombo",       gb_gnome_db_combo_init },
	{ "GnomeDbGrayBar",	gb_gnome_db_gray_bar_init },

	{ "GnomeDbEditor",	gb_gnome_db_editor_init },
	{ "GnomeDbConnectionProperties",	gb_gnome_db_connection_properties_init },


	/* These may be added back at some point. */
        /*{ "GnomeDbIconList",    gb_gnome_db_iconlist_init },*/

	{ NULL, NULL }  
};


static GladeWidgetInitData notshown[] =
{
  { NULL, NULL }
};


static GladePaletteSectionData sections[] =
{
  /* Note that glade_palette_set_show_gnome_widgets() has some of these
     strings hard-coded now, so keep up-to-date. */
  { "Gnome _DB", gnome_db },
  { "NotShown", notshown },
  { NULL, NULL }
};


GladePaletteSectionData *get_gnome_db_widgets()
{
	return sections;
}

#endif
