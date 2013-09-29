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

#ifdef GLADE_STYLE_SUPPORT

#include "gladeconfig.h"

#include "glade_project.h"
#include "styles.h"
#include "gbwidget.h"


gchar *GbStateNames[] =
{"NORMAL", "ACTIVE", "PRELIGHT",
 "SELECTED", "INSENSITIVE"};
gchar *GbColorNames[] =
{"fg", "bg", "text", "base"};
gchar *GbBgPixmapName = "bg_pixmap";

/* The style data hash stores named GbStyles with the name as the key.
   Note that the GbStyle name field is also used as the key so be careful when
   freeing or updating it. */
GHashTable *gb_style_hash = NULL;

/* The default style of created widgets. We need to use a separate default
   style for the widgets created so that the Glade UI doesn't change. */
GbStyle *gb_widget_default_gb_style;


struct GbUpdateStyleData
  {
    GbStyle *old_gbstyle;
    GbStyle *new_gbstyle;
  };

static void update_style (GtkWidget * component,
			  struct GbUpdateStyleData *data);

/*************************************************************************
 * Functions for creating/copying/destroying GbStyleData structs
 *************************************************************************/

static void
reset_gb_styles_callback (gchar * key, GbStyle * gbstyle, gpointer data)
{
  gb_widget_destroy_gb_style (gbstyle, FALSE);
}

void
gb_widget_reset_gb_styles ()
{
  if (gb_style_hash)
    {
      g_hash_table_foreach (gb_style_hash, (GHFunc) reset_gb_styles_callback,
			    NULL);
      g_hash_table_destroy (gb_style_hash);
    }


  /* Create new GbStyle hash */
  gb_style_hash = g_hash_table_new (g_str_hash, g_str_equal);

  /* Create default GbStyle */
  gb_widget_default_gb_style = gb_widget_new_gb_style ();
  gb_widget_default_gb_style->name = g_strdup (GB_STYLE_DEFAULT);
  gb_widget_default_gb_style->xlfd_fontname = g_strdup (GB_DEFAULT_XLFD_FONTNAME);
  gb_widget_default_gb_style->style = gtk_widget_get_default_style ();
  g_hash_table_insert (gb_style_hash, gb_widget_default_gb_style->name,
		       gb_widget_default_gb_style);
}


GbStyle *
gb_widget_new_gb_style ()
{
  gint i;
  GbStyle *gbstyle = g_new (GbStyle, 1);
  gbstyle->name = NULL;
  gbstyle->xlfd_fontname = NULL;
  for (i = 0; i < GB_NUM_STYLE_STATES; i++)
    {
      gbstyle->bg_pixmap_filenames[i] = NULL;
    }
  gbstyle->style = NULL;
  gbstyle->ref_count = 0;
  return gbstyle;
}


GbStyle *
gb_widget_copy_gb_style (GbStyle * gbstyle)
{
  gint i;
  GbStyle *gbstyle_copy = g_new (GbStyle, 1);
  gbstyle_copy->name = g_strdup (gbstyle->name);
  gbstyle_copy->xlfd_fontname = g_strdup (gbstyle->xlfd_fontname);
  for (i = 0; i < GB_NUM_STYLE_STATES; i++)
    {
      gbstyle_copy->bg_pixmap_filenames[i]
	= g_strdup (gbstyle->bg_pixmap_filenames[i]);
    }
  gbstyle_copy->style = gbstyle->style;
  gtk_style_ref (gbstyle_copy->style);
  gbstyle_copy->ref_count = 0;
  return gbstyle_copy;
}


void
gb_widget_ref_gb_style (GbStyle * gbstyle)
{
  gbstyle->ref_count++;
}


void
gb_widget_unref_gb_style (GbStyle * gbstyle)
{
  gbstyle->ref_count--;
  /* Don't destroy named GbStyles - let user do that explicitly. */
  if (gbstyle->ref_count <= 0 && gbstyle->name == NULL)
    {
      gb_widget_destroy_gb_style (gbstyle, TRUE);
    }
}


void
gb_widget_destroy_gb_style (GbStyle * gbstyle, gboolean remove_from_hash)
{
  gint i;

  MSG1 ("Destroying gbstyle:%s", gbstyle->name);

  /* Remove the GbStyle from the hash */
  if (remove_from_hash && gbstyle->name)
    g_hash_table_remove (gb_style_hash, gbstyle->name);

  /* Now free all the memory used */
  g_free (gbstyle->name);
  g_free (gbstyle->xlfd_fontname);
  for (i = 0; i < GB_NUM_STYLE_STATES; i++)
    {
      g_free (gbstyle->bg_pixmap_filenames[i]);
    }
  if (gbstyle->style)
    gtk_style_unref (gbstyle->style);
  g_free (gbstyle);
}



/* This steps through all widgets in all components and updates the GbStyle
   and GtkStyles as appropriate. */
void
gb_widget_update_gb_styles (GbStyle * old_gbstyle, GbStyle * new_gbstyle)
{
  struct GbUpdateStyleData data;
  data.old_gbstyle = old_gbstyle;
  data.new_gbstyle = new_gbstyle;
  glade_project_foreach_component (current_project,
				   (GtkCallback) update_style, &data);
}


static void
update_style (GtkWidget * widget, struct GbUpdateStyleData *data)
{
#if 0
  GladeWidgetData *wdata;

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);

  if (wdata && wdata->gbstyle == data->old_gbstyle)
    {
      wdata->gbstyle = data->new_gbstyle;
      if (widget->style != wdata->gbstyle->style)
	{
	  gtk_widget_set_style (widget, wdata->gbstyle->style);
	  gtk_widget_queue_resize (widget);
	  gtk_widget_queue_draw (widget);
	}
    }
  if (GTK_IS_CONTAINER (widget))
    {
      gtk_container_forall (GTK_CONTAINER (widget), (GtkCallback) update_style,
			    data);
    }
#endif
}


#endif
