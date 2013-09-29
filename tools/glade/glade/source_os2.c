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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <locale.h>

#include "gladeconfig.h"

#include "gbwidget.h"
#include "glade_project.h"
#include "source.h"
#include "utils.h"
#ifdef HAVE_OS2_H

extern FILE * create_file_if_not_exist (gchar *filename,
					GladeStatusCode  *status);

static void source_write_makefile_simple (gchar *makefilename)
{
  FILE *fp;

  if (glade_util_file_exists (makefilename))
    return;
  
  fp = glade_util_fopen (makefilename, "wt");
  if (fp == NULL)
    return;

  fprintf (fp,"#\n");
  fprintf (fp,"# This is an example makefile for OS/2\n");
  fprintf (fp,"#\n");
  fprintf (fp,"CC=gcc\n");
  fprintf (fp,"GTKLIBS=-L$(X11ROOT)/XFree86/lib -lgtk -lgdk -lglib\n");
#ifdef USE_GNOME
  fprintf (fp,"GNOMELIBS=-lgnome -lgnomeui -lgdk_imlib\n");
  fprintf (fp,"PACK=-DPACKAGE=\\\"gladtst\\\" -DVERSION=\\\"0.0.1\\\"\n");
#endif
  fprintf (fp,"DIRS=-DPACKAGE_DATA_DIR=\\\".\\\" -DPACKAGE_SOURCE_DIR=\\\".\\\"\n");
  fprintf (fp,"OBJS=main.o support.o interface.o callbacks.o\n");
  fprintf (fp,"CFLAGS=-Zmtd -D__ST_MT_ERRNO__ -I. -I$(X11ROOT)/XFree86/include");  
  fprintf (fp," $(DIRS)");
#ifdef USE_GNOME
  fprintf (fp," $(PACK)");
#endif
  fprintf (fp," \n");
  fprintf (fp,"\n");
  fprintf (fp,"all: gladetst.exe\n");
  fprintf (fp,"\n");
  fprintf (fp,"%%.o : %%.c\n");
  fprintf (fp,"\t$(CC) $(CFLAGS) -c $<\n");
  fprintf (fp,"\n");
  fprintf (fp,"gladetst.exe: $(OBJS)\n");
  fprintf (fp,"\t$(CC) -Zmtd -o $@ $(OBJS) $(GTKLIBS)");
#ifdef USE_GNOME
  fprintf (fp," $(GNOMELIBS)");
#endif
  fprintf (fp," gladetst.def\n");
  fclose (fp);
}

static void source_write_os2_def_file (gchar *filename)
{
  FILE *fp;
  
  if (glade_util_file_exists (filename))
    return;

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
      return;

  fprintf (fp,"NAME gladetst WINDOWCOMPAT\n");
  fprintf (fp,"HEAPSIZE  0x10000\n");
  fprintf (fp,"STACKSIZE 0x10000\n");
  fclose (fp);
}

void source_write_os2_files(GbWidgetWriteSourceData * data)
{
  gchar *directory;
  gchar *filename;

  directory = glade_project_get_source_directory (data->project);

  filename = glade_util_make_absolute_path (directory, "makefile.os2");
  source_write_makefile_simple (filename);
  g_free (filename);

  filename = glade_util_make_absolute_path (directory, "gladetst.def");
  source_write_os2_def_file (filename);
  g_free (filename);
}
#endif
