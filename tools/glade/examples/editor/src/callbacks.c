#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"


/* We create the dialog boxes once and keep pointers to them in static
   variables so we can reuse them. */
static GtkWidget *open_filesel = NULL;
static GtkWidget *save_filesel = NULL;
static GtkWidget *fontsel = NULL;

/* This is the filename of the file currently being edited, in on-disk
   encoding (which may be UTF-8 or the locale's encoding). */
static gchar *current_filename = NULL;

/* This flag is set to TRUE if the file has been changed. We don't actually
   use it here, but the program could be extended to prompt to save changes. */
static gboolean file_changed = FALSE;

/* A key used to store pointers to the main window. */
static const gchar *MainWindowKey = "MainWindowKey";

/* The size of chunks to read in when opening files. */
#define BUFFER_SIZE 8192


/* Static functions. */
static void new_file (GtkWidget *main_window);
static void open_file (GtkWidget *main_window);
static void save_as (GtkWidget *main_window);

static void real_open_file (GtkWidget *main_window,
			    const gchar *filename);
static void real_save_file (GtkWidget *main_window,
			    const gchar *filename);


/***************************************************************************
 * Main Window Signals.
 ***************************************************************************/

/* This is called when the main window gets a "delete_event" signal, which
   typically happens when the user clicks on the close icon on the window's
   title bar. If we didn't handle this, the window would be destroyed but
   our application would not exit. */
gboolean
on_main_window_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  exit (0);

  /* Shouldn't reach here, but we add it to stop compiler warnings. */
  return FALSE;
}


/***************************************************************************
 * File Menu Commands.
 ***************************************************************************/
void
on_New_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  /* We use the Glade utility function lookup_widget() to get a pointer to the
     main window. The first argument is any widget in the window/dialog and
     the second argument is the name of the widget you want to get. */
  main_window = lookup_widget (GTK_WIDGET (menuitem), "main_window");
  new_file (main_window);
}


void
on_Open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  main_window = lookup_widget (GTK_WIDGET (menuitem), "main_window");
  open_file (main_window);
}


void
on_Save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  main_window = lookup_widget (GTK_WIDGET (menuitem), "main_window");

  /* If the current document doesn't have a filename yet, we call save_as
     to show the file selection dialog. */
  if (current_filename == NULL)
    save_as (main_window);
  else
    real_save_file (main_window, current_filename);
}


void
on_Save_As_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  main_window = lookup_widget (GTK_WIDGET (menuitem), "main_window");
  save_as (main_window);
}


void
on_Exit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_exit (0);
}


/***************************************************************************
 * Edit Menu Commands.
 * The GtkTextView class makes this very easy for us by providing action
 * signals that perform clipboard functions.
 ***************************************************************************/

void
on_Cut_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *text, *statusbar;

  text = lookup_widget (GTK_WIDGET (menuitem), "text1");
  g_signal_emit_by_name (G_OBJECT (text), "cut_clipboard", NULL);

  statusbar = lookup_widget (GTK_WIDGET (menuitem), "statusbar1");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Text cut to clipboard.");
}


void
on_Copy_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *text, *statusbar;

  text = lookup_widget (GTK_WIDGET (menuitem), "text1");
  g_signal_emit_by_name (G_OBJECT (text), "copy_clipboard", NULL);

  statusbar = lookup_widget (GTK_WIDGET (menuitem), "statusbar1");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Text copied.");
}


void
on_Paste_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *text, *statusbar;

  text = lookup_widget (GTK_WIDGET (menuitem), "text1");
  g_signal_emit_by_name (G_OBJECT (text), "paste_clipboard", NULL);

  statusbar = lookup_widget (GTK_WIDGET (menuitem), "statusbar1");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Text pasted.");
}


void
on_Clear_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *text, *statusbar;
  GtkTextBuffer *buffer;

  text = lookup_widget (GTK_WIDGET (menuitem), "text1");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);

  statusbar = lookup_widget (GTK_WIDGET (menuitem), "statusbar1");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Text deleted.");
}


/***************************************************************************
 * Toolbar Buttons.
 ***************************************************************************/

void
on_new_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  main_window = lookup_widget (GTK_WIDGET (button), "main_window");
  new_file (main_window);
}


void
on_open_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  main_window = lookup_widget (GTK_WIDGET (button), "main_window");
  open_file (main_window);
}


void
on_save_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  main_window = lookup_widget (GTK_WIDGET (button), "main_window");

  if (current_filename == NULL)
    save_as (main_window);
  else
    real_save_file (main_window, current_filename);
}


/***************************************************************************
 * Font Selection Dialog.
 ***************************************************************************/

/* This is called when the 'Change Font' command is used. We just need to
   show the font selection dialog. */
void
on_Font_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *main_window;

  main_window = lookup_widget (GTK_WIDGET (menuitem), "main_window");
  if (!fontsel)
    {
      fontsel = create_font_selection ();
      gtk_object_set_data (GTK_OBJECT (fontsel), MainWindowKey, main_window);

      /* Make sure the dialog doesn't disappear behind the main window. */
      gtk_window_set_transient_for (GTK_WINDOW (fontsel), 
				    GTK_WINDOW (main_window));
    }
  gtk_widget_show (fontsel);

  /* We make sure the dialog is visible. */
  gtk_window_present (GTK_WINDOW (fontsel));
}


gboolean
on_fontsel_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (widget));
  return TRUE;
}


void
on_fontsel_ok_button_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *fontsel;

  /* We do the same thing as apply, but we close the dialog after. */
  on_fontsel_apply_button_clicked (button, NULL);
  fontsel = gtk_widget_get_toplevel (GTK_WIDGET (button));
  gtk_widget_hide (fontsel);
}


void
on_fontsel_cancel_button_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


void
on_fontsel_apply_button_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *fontsel, *main_window, *text, *statusbar;
  gchar *font_name;
  PangoFontDescription *font_desc;

  fontsel = gtk_widget_get_toplevel (GTK_WIDGET (button));
  main_window = gtk_object_get_data (GTK_OBJECT (fontsel), MainWindowKey);
  text = lookup_widget (main_window, "text1");
  statusbar = lookup_widget (main_window, "statusbar1");

  font_name = gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG (fontsel));
  font_desc = pango_font_description_from_string (font_name);
  g_free (font_name);

  /* We copy the current style, and update the font. */
  gtk_widget_modify_font (text, font_desc);
  pango_font_description_free (font_desc);

  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Font updated.");
}


/***************************************************************************
 * Open File Selection Dialog.
 ***************************************************************************/

void
on_open_filesel_ok_button_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *filesel, *main_window;
  const gchar *filename;

  filesel = gtk_widget_get_toplevel (GTK_WIDGET (button));
  main_window = gtk_object_get_data (GTK_OBJECT (filesel), MainWindowKey);
  gtk_widget_hide (filesel);
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
  real_open_file (main_window, filename);
}


void
on_open_filesel_cancel_button_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


gboolean
on_open_filesel_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (widget));
  return TRUE;
}


/***************************************************************************
 * Save File Selection Dialog.
 ***************************************************************************/

void
on_save_filesel_ok_button_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *filesel, *main_window;
  const gchar *filename;

  filesel = gtk_widget_get_toplevel (GTK_WIDGET (button));
  main_window = gtk_object_get_data (GTK_OBJECT (filesel), MainWindowKey);
  gtk_widget_hide (filesel);
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
  real_save_file (main_window, filename);
}


void
on_save_filesel_cancel_button_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


gboolean
on_save_filesel_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (widget));
  return TRUE;
}


/***************************************************************************
 * About Dialog.
 ***************************************************************************/

void
on_About_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  static GtkWidget *about = NULL;

  if (about == NULL) 
    {
      GtkWidget *main_window;

      about = create_about_dialog ();
      /* set the widget pointer to NULL when the widget is destroyed */
      g_signal_connect (G_OBJECT (about),
			"destroy",
			G_CALLBACK (gtk_widget_destroyed),
			&about);

      main_window = lookup_widget (GTK_WIDGET (menuitem), "main_window");
      /* Make sure the dialog doesn't disappear behind the main window. */
      gtk_window_set_transient_for (GTK_WINDOW (about), 
				    GTK_WINDOW (main_window));
      /* Do not allow user to resize the dialog */
      gtk_window_set_resizable (GTK_WINDOW (about), FALSE);
    }

  /* Make sure the dialog is visible. */
  gtk_window_present (GTK_WINDOW (about));
}


void
on_about_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}


/***************************************************************************
 * Text Widget signals.
 ***************************************************************************/

/*
 * Text changed callback. This signal is emitted whenever the text in the
 * GtkTextView changes. We don't use this at present, but it could be used to
 * prompt to save changes before opening new files or quitting.
 */
void
on_text_changed                        (GtkTextBuffer   *buffer,
                                        gpointer         user_data)
{
  file_changed = TRUE;
}


/***************************************************************************
 * Private functions.
 ***************************************************************************/

/* This sets the window title according to the current filename. */
void
set_window_title (GtkWidget *main_window)
{
  gchar *title, *filename_utf8 = NULL;

  /* We need to convert the filename to UTF-8. */
  if (current_filename)
    filename_utf8 = g_filename_to_utf8 (current_filename, -1, NULL, NULL,
					NULL);

  title = g_strdup_printf ("Editor: %s\n",
			   filename_utf8 ? g_basename (filename_utf8)
			   : "untitled");

  gtk_window_set_title (GTK_WINDOW (main_window), title);
  g_free (title);
  g_free (filename_utf8);
}


/* This creates a new document, by clearing the text widget and setting the
   current filename to NULL. */
static void
new_file (GtkWidget *main_window)
{
  GtkWidget *text, *statusbar;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;

  text = lookup_widget (GTK_WIDGET (main_window), "text1");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_delete (buffer, &start, &end);

  g_free (current_filename);
  current_filename = NULL;
  file_changed = FALSE;
  set_window_title (main_window);

  statusbar = lookup_widget (main_window, "statusbar1");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "New file.");
}


/* This shows the file selection dialog to open a file. */
static void
open_file (GtkWidget *main_window)
{
  /* We use the same file selection widget each time, so first
     of all we create it if it hasn't already been created. */
  if (open_filesel == NULL)
    {
      open_filesel = create_open_file_selection ();
      /* Make sure the dialog doesn't disappear behind the main window. */
      gtk_window_set_transient_for (GTK_WINDOW (open_filesel), 
				    GTK_WINDOW (main_window));
    }

  /* We save a pointer to the main window inside the file selection's
     data list, so we can get it easily in the callbacks. */
  gtk_object_set_data (GTK_OBJECT (open_filesel), MainWindowKey, main_window);

  /* We make sure the dialog is visible. */
  gtk_window_present (GTK_WINDOW (open_filesel));
}


/* This shows the file selection dialog to save a file. */
static void
save_as (GtkWidget *main_window)
{
  if (save_filesel == NULL)
    {
      save_filesel = create_save_file_selection ();
      /* Make sure the dialog doesn't disappear behind the main window. */
      gtk_window_set_transient_for (GTK_WINDOW (save_filesel), 
				    GTK_WINDOW (main_window));
    }

  gtk_object_set_data (GTK_OBJECT (save_filesel), MainWindowKey, main_window);

  /* If the current document has a filename we use that as the default. */
  if (current_filename)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (save_filesel),
				     current_filename);

  /* We make sure the dialog is visible. */
  gtk_window_present (GTK_WINDOW (save_filesel));
}


/* This loads the given file, which is in on-disk encoding (which may not
   be UTF-8). */
static void
real_open_file (GtkWidget *main_window, const gchar *filename)
{
  GtkWidget *text, *statusbar;
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;
  FILE *fp;
  gchar buffer[BUFFER_SIZE];
  gint bytes_read;

  text = lookup_widget (main_window, "text1");
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  statusbar = lookup_widget (main_window, "statusbar1");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  gtk_text_buffer_delete (text_buffer, &start, &end);
  g_free (current_filename);
  current_filename = NULL;
  file_changed = FALSE;

  fp = fopen (filename, "r");
  if (fp == NULL)
    {
      gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Couldn't open file.");
      return;
    }

  for (;;)
    {
      bytes_read = fread (buffer, sizeof (gchar), BUFFER_SIZE, fp);

      if (bytes_read > 0)
	{
	  gtk_text_buffer_get_end_iter (text_buffer, &end);
	  gtk_text_buffer_insert (text_buffer, &end, buffer, bytes_read);
	}

      if (bytes_read != BUFFER_SIZE && (feof (fp) || ferror (fp)))
	break;
    }

  /* If there is an error while loading, we reset everything to a good state.
   */
  if (ferror (fp))
    {
      fclose (fp);
      gtk_text_buffer_get_bounds (text_buffer, &start, &end);
      gtk_text_buffer_delete (text_buffer, &start, &end);
      set_window_title (main_window);
      gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Error loading file.");
      return;
    }

  fclose (fp);

  current_filename = g_strdup (filename);
  set_window_title (main_window);
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "File opened.");
}


/* This saves the file, which is in on-disk encoding (which may not
   be UTF-8). */
static void
real_save_file (GtkWidget *main_window, const gchar *filename)
{
  GtkWidget *text, *statusbar;
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;
  gchar *data;
  FILE *fp;
  gint bytes_written, len;

  if (current_filename == NULL || strcmp (current_filename, filename))
    {
      g_free (current_filename);
      current_filename = g_strdup (filename);
      set_window_title (main_window);
    }

  text = lookup_widget (GTK_WIDGET (main_window), "text1");
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  statusbar = lookup_widget (main_window, "statusbar1");

  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  data = gtk_text_buffer_get_text (text_buffer, &start, &end, TRUE);

  fp = fopen (filename, "w");
  if (fp == NULL)
    {
      g_free (data);
      return;
    }

  len = strlen (data);
  bytes_written = fwrite (data, sizeof (gchar), len, fp);
  fclose (fp);

  g_free (data);

  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), 1);
  if (len != bytes_written)
    {
      gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "Error saving file.");
      return;
    }

  file_changed = FALSE;
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), 1, "File saved.");
}

