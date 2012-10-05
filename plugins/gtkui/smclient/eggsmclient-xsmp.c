/*
 * Copyright (C) 2007 Novell, Inc.
 *
 * Inspired by various other pieces of code including GsmClient (C)
 * 2001 Havoc Pennington, GnomeClient (C) 1998 Carsten Schaar, and twm
 * session code (C) 1998 The Open Group.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "eggsmclient.h"
#include "eggsmclient-private.h"

#include "eggdesktopfile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/SM/SMlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define EGG_TYPE_SM_CLIENT_XSMP            (egg_sm_client_xsmp_get_type ())
#define EGG_SM_CLIENT_XSMP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_SM_CLIENT_XSMP, EggSMClientXSMP))
#define EGG_SM_CLIENT_XSMP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_SM_CLIENT_XSMP, EggSMClientXSMPClass))
#define EGG_IS_SM_CLIENT_XSMP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_SM_CLIENT_XSMP))
#define EGG_IS_SM_CLIENT_XSMP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_SM_CLIENT_XSMP))
#define EGG_SM_CLIENT_XSMP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_SM_CLIENT_XSMP, EggSMClientXSMPClass))

typedef struct _EggSMClientXSMP        EggSMClientXSMP;
typedef struct _EggSMClientXSMPClass   EggSMClientXSMPClass;

/* These mostly correspond to the similarly-named states in section
 * 9.1 of the XSMP spec. Some of the states there aren't represented
 * here, because we don't need them. SHUTDOWN_CANCELLED is slightly
 * different from the spec; we use it when the client is IDLE after a
 * ShutdownCancelled message, but the application is still interacting
 * and doesn't know the shutdown has been cancelled yet.
 */
typedef enum
{
  XSMP_STATE_IDLE,
  XSMP_STATE_SAVE_YOURSELF,
  XSMP_STATE_INTERACT_REQUEST,
  XSMP_STATE_INTERACT,
  XSMP_STATE_SAVE_YOURSELF_DONE,
  XSMP_STATE_SHUTDOWN_CANCELLED,
  XSMP_STATE_CONNECTION_CLOSED
} EggSMClientXSMPState;

static const char *state_names[] = {
  "idle",
  "save-yourself",
  "interact-request",
  "interact",
  "save-yourself-done",
  "shutdown-cancelled",
  "connection-closed"
};

#define EGG_SM_CLIENT_XSMP_STATE(xsmp) (state_names[(xsmp)->state])

struct _EggSMClientXSMP
{
  EggSMClient parent;

  SmcConn connection;
  char *client_id;

  EggSMClientXSMPState state;
  char **restart_command;
  gboolean set_restart_command;
  int restart_style;

  guint idle;

  /* Current SaveYourself state */
  guint expecting_initial_save_yourself : 1;
  guint need_save_state : 1;
  guint need_quit_requested : 1;
  guint interact_errors : 1;
  guint shutting_down : 1;

  /* Todo list */
  guint waiting_to_set_initial_properties : 1;
  guint waiting_to_emit_quit : 1;
  guint waiting_to_emit_quit_cancelled : 1;
  guint waiting_to_save_myself : 1;

};

struct _EggSMClientXSMPClass
{
  EggSMClientClass parent_class;

};

static void     sm_client_xsmp_startup (EggSMClient *client,
					const char  *client_id);
static void     sm_client_xsmp_set_restart_command (EggSMClient  *client,
						    int           argc,
						    const char  **argv);
static void     sm_client_xsmp_will_quit (EggSMClient *client,
					  gboolean     will_quit);
static gboolean sm_client_xsmp_end_session (EggSMClient         *client,
					    EggSMClientEndStyle  style,
					    gboolean  request_confirmation);

static void xsmp_save_yourself      (SmcConn   smc_conn,
				     SmPointer client_data,
				     int       save_style,
				     Bool      shutdown,
				     int       interact_style,
				     Bool      fast);
static void xsmp_die                (SmcConn   smc_conn,
				     SmPointer client_data);
static void xsmp_save_complete      (SmcConn   smc_conn,
				     SmPointer client_data);
static void xsmp_shutdown_cancelled (SmcConn   smc_conn,
				     SmPointer client_data);
static void xsmp_interact           (SmcConn   smc_conn,
				     SmPointer client_data);

static SmProp *array_prop        (const char    *name,
				  ...);
static SmProp *ptrarray_prop     (const char    *name,
				  GPtrArray     *values);
static SmProp *string_prop       (const char    *name,
				  const char    *value);
static SmProp *card8_prop        (const char    *name,
				  unsigned char  value);

static void set_properties         (EggSMClientXSMP *xsmp, ...);
static void delete_properties      (EggSMClientXSMP *xsmp, ...);

static GPtrArray *generate_command (char       **restart_command,
				    const char  *client_id,
				    const char  *state_file);

static void save_state            (EggSMClientXSMP *xsmp);
static void do_save_yourself      (EggSMClientXSMP *xsmp);
static void update_pending_events (EggSMClientXSMP *xsmp);

static void     ice_init             (void);
static gboolean process_ice_messages (IceConn       ice_conn);
static void     smc_error_handler    (SmcConn       smc_conn,
				      Bool          swap,
				      int           offending_minor_opcode,
				      unsigned long offending_sequence,
				      int           error_class,
				      int           severity,
				      SmPointer     values);

G_DEFINE_TYPE (EggSMClientXSMP, egg_sm_client_xsmp, EGG_TYPE_SM_CLIENT)

static void
egg_sm_client_xsmp_init (EggSMClientXSMP *xsmp)
{
  xsmp->state = XSMP_STATE_CONNECTION_CLOSED;
  xsmp->connection = NULL;
  xsmp->restart_style = SmRestartIfRunning;
}

static void
egg_sm_client_xsmp_class_init (EggSMClientXSMPClass *klass)
{
  EggSMClientClass *sm_client_class = EGG_SM_CLIENT_CLASS (klass);

  sm_client_class->startup             = sm_client_xsmp_startup;
  sm_client_class->set_restart_command = sm_client_xsmp_set_restart_command;
  sm_client_class->will_quit           = sm_client_xsmp_will_quit;
  sm_client_class->end_session         = sm_client_xsmp_end_session;
}

EggSMClient *
egg_sm_client_xsmp_new (void)
{
#if GTK_CHECK_VERSION(3,0,0)
  if (!GDK_IS_X11_DISPLAY_MANAGER (gdk_display_manager_get ()))
    return NULL;
#endif

  if (!g_getenv ("SESSION_MANAGER"))
    return NULL;

  return g_object_new (EGG_TYPE_SM_CLIENT_XSMP, NULL);
}

static gboolean
sm_client_xsmp_set_initial_properties (gpointer user_data)
{
  EggSMClientXSMP *xsmp = user_data;
  EggDesktopFile *desktop_file;
  GPtrArray *clone, *restart;
  char pid_str[64];

  if (xsmp->idle)
    {
      g_source_remove (xsmp->idle);
      xsmp->idle = 0;
    }
  xsmp->waiting_to_set_initial_properties = FALSE;

  if (egg_sm_client_get_mode () == EGG_SM_CLIENT_MODE_NO_RESTART)
    xsmp->restart_style = SmRestartNever;

  /* Parse info out of desktop file */
  desktop_file = egg_get_desktop_file ();
  if (desktop_file)
    {
      GError *err = NULL;
      char *cmdline, **argv;
      int argc;

      if (xsmp->restart_style == SmRestartIfRunning)
	{
	  if (egg_desktop_file_get_boolean (desktop_file, 
					    "X-GNOME-AutoRestart", NULL))
	    xsmp->restart_style = SmRestartImmediately;
	}

      if (!xsmp->set_restart_command)
	{
	  cmdline = egg_desktop_file_parse_exec (desktop_file, NULL, &err);
	  if (cmdline && g_shell_parse_argv (cmdline, &argc, &argv, &err))
	    {
	      egg_sm_client_set_restart_command (EGG_SM_CLIENT (xsmp),
						 argc, (const char **)argv);
	      g_strfreev (argv);
	    }
	  else
	    {
	      g_warning ("Could not parse Exec line in desktop file: %s",
			 err->message);
	      g_error_free (err);
	    }
	  g_free (cmdline);
	}
    }

  if (!xsmp->set_restart_command)
    xsmp->restart_command = g_strsplit (g_get_prgname (), " ", -1);

  clone = generate_command (xsmp->restart_command, NULL, NULL);
  restart = generate_command (xsmp->restart_command, xsmp->client_id, NULL);

  g_debug ("Setting initial properties");

  /* Program, CloneCommand, RestartCommand, and UserID are required.
   * ProcessID isn't required, but the SM may be able to do something
   * useful with it.
   */
  g_snprintf (pid_str, sizeof (pid_str), "%lu", (gulong) getpid ());
  set_properties (xsmp,
		  string_prop   (SmProgram, g_get_prgname ()),
		  ptrarray_prop (SmCloneCommand, clone),
		  ptrarray_prop (SmRestartCommand, restart),
		  string_prop   (SmUserID, g_get_user_name ()),
		  string_prop   (SmProcessID, pid_str),
		  card8_prop    (SmRestartStyleHint, xsmp->restart_style),
		  NULL);
  g_ptr_array_free (clone, TRUE);
  g_ptr_array_free (restart, TRUE);

  if (desktop_file)
    {
      set_properties (xsmp,
		      string_prop ("_GSM_DesktopFile", egg_desktop_file_get_source (desktop_file)),
		      NULL);
    }

  update_pending_events (xsmp);
  return FALSE;
}

/* This gets called from two different places: xsmp_die() (when the
 * server asks us to disconnect) and process_ice_messages() (when the
 * server disconnects unexpectedly).
 */
static void
sm_client_xsmp_disconnect (EggSMClientXSMP *xsmp)
{
  SmcConn connection;

  if (!xsmp->connection)
    return;

  g_debug ("Disconnecting");

  connection = xsmp->connection;
  xsmp->connection = NULL;
  SmcCloseConnection (connection, 0, NULL);
  xsmp->state = XSMP_STATE_CONNECTION_CLOSED;

  xsmp->waiting_to_save_myself = FALSE;
  update_pending_events (xsmp);
}

static void
sm_client_xsmp_startup (EggSMClient *client,
			const char  *client_id)
{
  EggSMClientXSMP *xsmp = (EggSMClientXSMP *)client;
  SmcCallbacks callbacks;
  char *ret_client_id;
  char error_string_ret[256];

  xsmp->client_id = g_strdup (client_id);

  ice_init ();
  SmcSetErrorHandler (smc_error_handler);

  callbacks.save_yourself.callback      = xsmp_save_yourself;
  callbacks.die.callback                = xsmp_die;
  callbacks.save_complete.callback      = xsmp_save_complete;
  callbacks.shutdown_cancelled.callback = xsmp_shutdown_cancelled;

  callbacks.save_yourself.client_data      = xsmp;
  callbacks.die.client_data                = xsmp;
  callbacks.save_complete.client_data      = xsmp;
  callbacks.shutdown_cancelled.client_data = xsmp;

  client_id = NULL;
  error_string_ret[0] = '\0';
  xsmp->connection =
    SmcOpenConnection (NULL, xsmp, SmProtoMajor, SmProtoMinor,
		       SmcSaveYourselfProcMask | SmcDieProcMask |
		       SmcSaveCompleteProcMask |
		       SmcShutdownCancelledProcMask,
		       &callbacks,
		       xsmp->client_id, &ret_client_id,
		       sizeof (error_string_ret), error_string_ret);

  if (!xsmp->connection)
    {
      g_warning ("Failed to connect to the session manager: %s\n",
		 error_string_ret[0] ?
		 error_string_ret : "no error message given");
      xsmp->state = XSMP_STATE_CONNECTION_CLOSED;
      return;
    }

  /* We expect a pointless initial SaveYourself if either (a) we
   * didn't have an initial client ID, or (b) we DID have an initial
   * client ID, but the server rejected it and gave us a new one.
   */
  if (!xsmp->client_id ||
      (ret_client_id && strcmp (xsmp->client_id, ret_client_id) != 0))
    xsmp->expecting_initial_save_yourself = TRUE;

  if (ret_client_id)
    {
      g_free (xsmp->client_id);
      xsmp->client_id = g_strdup (ret_client_id);
      free (ret_client_id);

      gdk_threads_enter ();
#if !GTK_CHECK_VERSION(2,23,3) && !GTK_CHECK_VERSION(3,0,0)
      gdk_set_sm_client_id (xsmp->client_id);
#else
      gdk_x11_set_sm_client_id (xsmp->client_id);
#endif
      gdk_threads_leave ();

      g_debug ("Got client ID \"%s\"", xsmp->client_id);
    }

  xsmp->state = XSMP_STATE_IDLE;

  /* Do not set the initial properties until we reach the main loop,
   * so that the application has a chance to call
   * egg_set_desktop_file(). (This may also help the session manager
   * have a better idea of when the application is fully up and
   * running.)
   */
  xsmp->waiting_to_set_initial_properties = TRUE;
  xsmp->idle = g_idle_add (sm_client_xsmp_set_initial_properties, client);
}

static void
sm_client_xsmp_set_restart_command (EggSMClient  *client,
				    int           argc,
				    const char  **argv)
{
  EggSMClientXSMP *xsmp = (EggSMClientXSMP *)client;
  int i;

  g_strfreev (xsmp->restart_command);

  xsmp->restart_command = g_new (char *, argc + 1);
  for (i = 0; i < argc; i++)
    xsmp->restart_command[i] = g_strdup (argv[i]);
  xsmp->restart_command[i] = NULL;

  xsmp->set_restart_command = TRUE;
}

static void
sm_client_xsmp_will_quit (EggSMClient *client,
			  gboolean     will_quit)
{
  EggSMClientXSMP *xsmp = (EggSMClientXSMP *)client;

  if (xsmp->state == XSMP_STATE_CONNECTION_CLOSED)
    {
      /* The session manager has already exited! Schedule a quit
       * signal.
       */
      xsmp->waiting_to_emit_quit = TRUE;
      update_pending_events (xsmp);
      return;
    }
  else if (xsmp->state == XSMP_STATE_SHUTDOWN_CANCELLED)
    {
      /* We received a ShutdownCancelled message while the application
       * was interacting; Schedule a quit_cancelled signal.
       */
      xsmp->waiting_to_emit_quit_cancelled = TRUE;
      update_pending_events (xsmp);
      return;
    }

  g_return_if_fail (xsmp->state == XSMP_STATE_INTERACT);

  g_debug ("Sending InteractDone(%s)", will_quit ? "False" : "True");
  SmcInteractDone (xsmp->connection, !will_quit);

  if (will_quit && xsmp->need_save_state)
    save_state (xsmp);

  g_debug ("Sending SaveYourselfDone(%s)", will_quit ? "True" : "False");
  SmcSaveYourselfDone (xsmp->connection, will_quit);
  xsmp->state = XSMP_STATE_SAVE_YOURSELF_DONE;
}

static gboolean
sm_client_xsmp_end_session (EggSMClient         *client,
			    EggSMClientEndStyle  style,
			    gboolean             request_confirmation)
{
  EggSMClientXSMP *xsmp = (EggSMClientXSMP *)client;
  int save_type;

  /* To end the session via XSMP, we have to send a
   * SaveYourselfRequest. We aren't allowed to do that if anything
   * else is going on, but we don't want to expose this fact to the
   * application. So we do our best to patch things up here...
   *
   * In the worst case, this method might block for some length of
   * time in process_ice_messages, but the only time that code path is
   * honestly likely to get hit is if the application tries to end the
   * session as the very first thing it does, in which case it
   * probably won't actually block anyway. It's not worth gunking up
   * the API to try to deal nicely with the other 0.01% of cases where
   * this happens.
   */

  while (xsmp->state != XSMP_STATE_IDLE ||
	 xsmp->expecting_initial_save_yourself)
    {
      /* If we're already shutting down, we don't need to do anything. */
      if (xsmp->shutting_down)
	return TRUE;

      switch (xsmp->state)
	{
	case XSMP_STATE_CONNECTION_CLOSED:
	  return FALSE;

	case XSMP_STATE_SAVE_YOURSELF:
	  /* Trying to log out from the save_state callback? Whatever.
	   * Abort the save_state.
	   */
	  SmcSaveYourselfDone (xsmp->connection, FALSE);
	  xsmp->state = XSMP_STATE_SAVE_YOURSELF_DONE;
	  break;

	case XSMP_STATE_INTERACT_REQUEST:
	case XSMP_STATE_INTERACT:
	case XSMP_STATE_SHUTDOWN_CANCELLED:
	  /* Already in a shutdown-related state, just ignore
	   * the new shutdown request...
	   */
	  return TRUE;

	case XSMP_STATE_IDLE:
	  if (xsmp->waiting_to_set_initial_properties)
	    sm_client_xsmp_set_initial_properties (xsmp);

	  if (!xsmp->expecting_initial_save_yourself)
	    break;
	  /* else fall through */

	case XSMP_STATE_SAVE_YOURSELF_DONE:
	  /* We need to wait for some response from the server.*/
	  process_ice_messages (SmcGetIceConnection (xsmp->connection));
	  break;

	default:
	  /* Hm... shouldn't happen */
	  return FALSE;
	}
    }

  /* xfce4-session will do the wrong thing if we pass SmSaveGlobal and
   * the user chooses to save the session. But gnome-session will do
   * the wrong thing if we pass SmSaveBoth and the user chooses NOT to
   * save the session... Sigh.
   */
  if (!strcmp (SmcVendor (xsmp->connection), "xfce4-session"))
    save_type = SmSaveBoth;
  else
    save_type = SmSaveGlobal;

  g_debug ("Sending SaveYourselfRequest(SmSaveGlobal, Shutdown, SmInteractStyleAny, %sFast)", request_confirmation ? "!" : "");
  SmcRequestSaveYourself (xsmp->connection,
			  save_type,
			  True, /* shutdown */
			  SmInteractStyleAny,
			  !request_confirmation, /* fast */
			  True /* global */);
  return TRUE;
}

static gboolean
idle_do_pending_events (gpointer data)
{
  EggSMClientXSMP *xsmp = data;
  EggSMClient *client = data;

  gdk_threads_enter ();

  xsmp->idle = 0;

  if (xsmp->waiting_to_emit_quit)
    {
        fprintf (stderr, "deadbeef: egg_sm_client_quit\n");
      xsmp->waiting_to_emit_quit = FALSE;
      egg_sm_client_quit (client);
      goto out;
    }

  if (xsmp->waiting_to_emit_quit_cancelled)
    {
      xsmp->waiting_to_emit_quit_cancelled = FALSE;
      egg_sm_client_quit_cancelled (client);
      xsmp->state = XSMP_STATE_IDLE;
    }

  if (xsmp->waiting_to_save_myself)
    {
      xsmp->waiting_to_save_myself = FALSE;
      do_save_yourself (xsmp);
    }

 out:
  gdk_threads_leave ();
  return FALSE;
}

static void
update_pending_events (EggSMClientXSMP *xsmp)
{
  gboolean want_idle =
    xsmp->waiting_to_emit_quit ||
    xsmp->waiting_to_emit_quit_cancelled ||
    xsmp->waiting_to_save_myself;

  if (want_idle)
    {
      if (xsmp->idle == 0)
	xsmp->idle = g_idle_add (idle_do_pending_events, xsmp);
    }
  else
    {
      if (xsmp->idle != 0)
	g_source_remove (xsmp->idle);
      xsmp->idle = 0;
    }
}

static void
fix_broken_state (EggSMClientXSMP *xsmp, const char *message,
		  gboolean send_interact_done,
		  gboolean send_save_yourself_done)
{
  g_warning ("Received XSMP %s message in state %s: client or server error",
	     message, EGG_SM_CLIENT_XSMP_STATE (xsmp));

  /* Forget any pending SaveYourself plans we had */
  xsmp->waiting_to_save_myself = FALSE;
  update_pending_events (xsmp);

  if (send_interact_done)
    SmcInteractDone (xsmp->connection, False);
  if (send_save_yourself_done)
    SmcSaveYourselfDone (xsmp->connection, True);

  xsmp->state = send_save_yourself_done ? XSMP_STATE_SAVE_YOURSELF_DONE : XSMP_STATE_IDLE;
}

/* SM callbacks */

static void
xsmp_save_yourself (SmcConn   smc_conn,
		    SmPointer client_data,
		    int       save_type,
		    Bool      shutdown,
		    int       interact_style,
		    Bool      fast)
{
  EggSMClientXSMP *xsmp = client_data;
  gboolean wants_quit_requested;

  g_debug ("Received SaveYourself(%s, %s, %s, %s) in state %s",
	   save_type == SmSaveLocal ? "SmSaveLocal" :
	   save_type == SmSaveGlobal ? "SmSaveGlobal" : "SmSaveBoth",
	   shutdown ? "Shutdown" : "!Shutdown",
	   interact_style == SmInteractStyleAny ? "SmInteractStyleAny" :
	   interact_style == SmInteractStyleErrors ? "SmInteractStyleErrors" :
	   "SmInteractStyleNone", fast ? "Fast" : "!Fast",
	   EGG_SM_CLIENT_XSMP_STATE (xsmp));

  if (xsmp->state != XSMP_STATE_IDLE &&
      xsmp->state != XSMP_STATE_SHUTDOWN_CANCELLED)
    {
      fix_broken_state (xsmp, "SaveYourself", FALSE, TRUE);
      return;
    }

  if (xsmp->waiting_to_set_initial_properties)
    sm_client_xsmp_set_initial_properties (xsmp);

  /* If this is the initial SaveYourself, ignore it; we've already set
   * properties and there's no reason to actually save state too.
   */
  if (xsmp->expecting_initial_save_yourself)
    {
      xsmp->expecting_initial_save_yourself = FALSE;

      if (save_type == SmSaveLocal &&
	  interact_style == SmInteractStyleNone &&
	  !shutdown && !fast)
	{
	  g_debug ("Sending SaveYourselfDone(True) for initial SaveYourself");
	  SmcSaveYourselfDone (xsmp->connection, True);
	  /* As explained in the comment at the end of
	   * do_save_yourself(), SAVE_YOURSELF_DONE is the correct
	   * state here, not IDLE.
	   */
	  xsmp->state = XSMP_STATE_SAVE_YOURSELF_DONE;
	  return;
	}
      else
	g_warning ("First SaveYourself was not the expected one!");
    }

  /* Even ignoring the "fast" flag completely, there are still 18
   * different combinations of save_type, shutdown and interact_style.
   * We interpret them as follows:
   *
   *   Type  Shutdown  Interact	 Interpretation
   *     G      F       A/E/N  	 do nothing (1)
   *     G      T         N    	 do nothing (1)*
   *     G      T        A/E   	 quit_requested (2)
   *    L/B     F       A/E/N  	 save_state (3)
   *    L/B     T         N    	 save_state (3)*
   *    L/B     T        A/E   	 quit_requested, then save_state (4)
   *
   *   1. Do nothing, because the SM asked us to do something
   *      uninteresting (save open files, but then don't quit
   *      afterward) or rude (save open files without asking the user
   *      for confirmation).
   *
   *   2. Request interaction and then emit ::quit_requested. This
   *      perhaps isn't quite correct for the SmInteractStyleErrors
   *      case, but we don't care.
   *
   *   3. Emit ::save_state. The SmSaveBoth SaveYourselfs in these
   *      rows essentially get demoted to SmSaveLocal, because their
   *      Global halves correspond to "do nothing".
   *
   *   4. Request interaction, emit ::quit_requested, and then emit
   *      ::save_state after interacting. This is the SmSaveBoth
   *      equivalent of #2, but we also promote SmSaveLocal shutdown
   *      SaveYourselfs to SmSaveBoth here, because we want to give
   *      the user a chance to save open files before quitting.
   *
   * (* It would be nice if we could do something useful when the
   * session manager sends a SaveYourself with shutdown True and
   * SmInteractStyleNone. But we can't, so we just pretend it didn't
   * even tell us it was shutting down. The docs for ::quit mention
   * that it might not always be preceded by ::quit_requested.)
   */

  /* As an optimization, we don't actually request interaction and
   * emit ::quit_requested if the application isn't listening to the
   * signal.
   */
  wants_quit_requested = g_signal_has_handler_pending (xsmp, g_signal_lookup ("quit_requested", EGG_TYPE_SM_CLIENT), 0, FALSE);

  xsmp->need_save_state     = (save_type != SmSaveGlobal);
  xsmp->need_quit_requested = (shutdown && wants_quit_requested &&
			       interact_style != SmInteractStyleNone);
  xsmp->interact_errors     = (interact_style == SmInteractStyleErrors);

  xsmp->shutting_down       = shutdown;

  do_save_yourself (xsmp);
}

static void
do_save_yourself (EggSMClientXSMP *xsmp)
{
  if (xsmp->state == XSMP_STATE_SHUTDOWN_CANCELLED)
    {
      /* The SM cancelled a previous SaveYourself, but we haven't yet
       * had a chance to tell the application, so we can't start
       * processing this SaveYourself yet.
       */
      xsmp->waiting_to_save_myself = TRUE;
      update_pending_events (xsmp);
      return;
    }

  if (xsmp->need_quit_requested)
    {
      xsmp->state = XSMP_STATE_INTERACT_REQUEST;

      g_debug ("Sending InteractRequest(%s)",
	       xsmp->interact_errors ? "Error" : "Normal");
      SmcInteractRequest (xsmp->connection,
			  xsmp->interact_errors ? SmDialogError : SmDialogNormal,
			  xsmp_interact,
			  xsmp);
      return;
    }

  if (xsmp->need_save_state)
    {
      save_state (xsmp);

      /* Though unlikely, the client could have been disconnected
       * while the application was saving its state.
       */
      if (!xsmp->connection)
	 return;
    }

  g_debug ("Sending SaveYourselfDone(True)");
  SmcSaveYourselfDone (xsmp->connection, True);

  /* The client state diagram in the XSMP spec says that after a
   * non-shutdown SaveYourself, we go directly back to "idle". But
   * everything else in both the XSMP spec and the libSM docs
   * disagrees.
   */
  xsmp->state = XSMP_STATE_SAVE_YOURSELF_DONE;
}

static void
save_state (EggSMClientXSMP *xsmp)
{
  GKeyFile *state_file;
  char *state_file_path, *data;
  EggDesktopFile *desktop_file;
  GPtrArray *restart;
  int offset, fd;

  /* We set xsmp->state before emitting save_state, but our caller is
   * responsible for setting it back afterward.
   */
  xsmp->state = XSMP_STATE_SAVE_YOURSELF;

  state_file = egg_sm_client_save_state ((EggSMClient *)xsmp);
  if (!state_file)
    {
      restart = generate_command (xsmp->restart_command, xsmp->client_id, NULL);
      set_properties (xsmp,
		      ptrarray_prop (SmRestartCommand, restart),
		      NULL);
      g_ptr_array_free (restart, TRUE);
      delete_properties (xsmp, SmDiscardCommand, NULL);
      return;
    }

  desktop_file = egg_get_desktop_file ();
  if (desktop_file)
    {
      GKeyFile *merged_file;
      char *desktop_file_path;

      merged_file = g_key_file_new ();
      desktop_file_path =
	g_filename_from_uri (egg_desktop_file_get_source (desktop_file),
			     NULL, NULL);
      if (desktop_file_path &&
	  g_key_file_load_from_file (merged_file, desktop_file_path,
				     G_KEY_FILE_KEEP_COMMENTS |
				     G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
	{
	  guint g, k, i;
	  char **groups, **keys, *value, *exec;

	  groups = g_key_file_get_groups (state_file, NULL);
	  for (g = 0; groups[g]; g++)
	    {
	      keys = g_key_file_get_keys (state_file, groups[g], NULL, NULL);
	      for (k = 0; keys[k]; k++)
		{
		  value = g_key_file_get_value (state_file, groups[g],
						keys[k], NULL);
		  if (value)
		    {
		      g_key_file_set_value (merged_file, groups[g],
					    keys[k], value);
		      g_free (value);
		    }
		}
	      g_strfreev (keys);
	    }
	  g_strfreev (groups);

	  g_key_file_free (state_file);
	  state_file = merged_file;

	  /* Update Exec key using "--sm-client-state-file %k" */
	  restart = generate_command (xsmp->restart_command,
				      NULL, "%k");
	  for (i = 0; i < restart->len; i++)
	    restart->pdata[i] = g_shell_quote (restart->pdata[i]);
	  g_ptr_array_add (restart, NULL);
	  exec = g_strjoinv (" ", (char **)restart->pdata);
	  g_strfreev ((char **)restart->pdata);
	  g_ptr_array_free (restart, FALSE);

	  g_key_file_set_string (state_file, EGG_DESKTOP_FILE_GROUP,
				 EGG_DESKTOP_FILE_KEY_EXEC,
				 exec);
	  g_free (exec);
	}
      else
	desktop_file = NULL;

      g_free (desktop_file_path);
    }

  /* Now write state_file to disk. (We can't use mktemp(), because
   * that requires the filename to end with "XXXXXX", and we want
   * it to end with ".desktop".)
   */

  data = g_key_file_to_data (state_file, NULL, NULL);
  g_key_file_free (state_file);

  offset = 0;
  while (1)
    {
      state_file_path = g_strdup_printf ("%s%csession-state%c%s-%ld.%s",
					 g_get_user_config_dir (),
					 G_DIR_SEPARATOR, G_DIR_SEPARATOR,
					 g_get_prgname (),
					 (long)time (NULL) + offset,
					 desktop_file ? "desktop" : "state");

      fd = open (state_file_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
      if (fd == -1)
	{
	  if (errno == EEXIST)
	    {
	      offset++;
	      g_free (state_file_path);
	      continue;
	    }
	  else if (errno == ENOTDIR || errno == ENOENT)
	    {
	      char *sep = strrchr (state_file_path, G_DIR_SEPARATOR);

	      *sep = '\0';
	      if (g_mkdir_with_parents (state_file_path, 0755) != 0)
		{
		  g_warning ("Could not create directory '%s'",
			     state_file_path);
		  g_free (state_file_path);
		  state_file_path = NULL;
		  break;
		}

	      continue;
	    }

	  g_warning ("Could not create file '%s': %s",
		     state_file_path, g_strerror (errno));
	  g_free (state_file_path);
	  state_file_path = NULL;
	  break;
	}

      close (fd);
      g_file_set_contents (state_file_path, data, -1, NULL);
      break;
    }
  g_free (data);

  restart = generate_command (xsmp->restart_command, xsmp->client_id,
			      state_file_path);
  set_properties (xsmp,
		  ptrarray_prop (SmRestartCommand, restart),
		  NULL);
  g_ptr_array_free (restart, TRUE);

  if (state_file_path)
    {
      set_properties (xsmp,
		      array_prop (SmDiscardCommand,
				  "/bin/rm", "-rf", state_file_path,
				  NULL),
		      NULL);
      g_free (state_file_path);
    }
}

static void
xsmp_interact (SmcConn   smc_conn,
	       SmPointer client_data)
{
  EggSMClientXSMP *xsmp = client_data;
  EggSMClient *client = client_data;

  g_debug ("Received Interact message in state %s",
	   EGG_SM_CLIENT_XSMP_STATE (xsmp));

  if (xsmp->state != XSMP_STATE_INTERACT_REQUEST)
    {
      fix_broken_state (xsmp, "Interact", TRUE, TRUE);
      return;
    }

  xsmp->state = XSMP_STATE_INTERACT;
  egg_sm_client_quit_requested (client);
}

static void
xsmp_die (SmcConn   smc_conn,
	  SmPointer client_data)
{
  EggSMClientXSMP *xsmp = client_data;
  EggSMClient *client = client_data;

  g_debug ("Received Die message in state %s",
	   EGG_SM_CLIENT_XSMP_STATE (xsmp));

  sm_client_xsmp_disconnect (xsmp);
  egg_sm_client_quit (client);
}

static void
xsmp_save_complete (SmcConn   smc_conn,
		    SmPointer client_data)
{
  EggSMClientXSMP *xsmp = client_data;

  g_debug ("Received SaveComplete message in state %s",
	   EGG_SM_CLIENT_XSMP_STATE (xsmp));

  if (xsmp->state == XSMP_STATE_SAVE_YOURSELF_DONE)
    xsmp->state = XSMP_STATE_IDLE;
  else
    fix_broken_state (xsmp, "SaveComplete", FALSE, FALSE);
}

static void
xsmp_shutdown_cancelled (SmcConn   smc_conn,
			 SmPointer client_data)
{
  EggSMClientXSMP *xsmp = client_data;
  EggSMClient *client = client_data;

  g_debug ("Received ShutdownCancelled message in state %s",
	   EGG_SM_CLIENT_XSMP_STATE (xsmp));

  xsmp->shutting_down = FALSE;

  if (xsmp->state == XSMP_STATE_SAVE_YOURSELF_DONE)
    {
      /* We've finished interacting and now the SM has agreed to
       * cancel the shutdown.
       */
      xsmp->state = XSMP_STATE_IDLE;
      egg_sm_client_quit_cancelled (client);
    }
  else if (xsmp->state == XSMP_STATE_SHUTDOWN_CANCELLED)
    {
      /* Hm... ok, so we got a shutdown SaveYourself, which got
       * cancelled, but the application was still interacting, so we
       * didn't tell it yet, and then *another* SaveYourself arrived,
       * which we must still be waiting to tell the app about, except
       * that now that SaveYourself has been cancelled too! Dizzy yet?
       */
      xsmp->waiting_to_save_myself = FALSE;
      update_pending_events (xsmp);
    }
  else
    {
      g_debug ("Sending SaveYourselfDone(False)");
      SmcSaveYourselfDone (xsmp->connection, False);

      if (xsmp->state == XSMP_STATE_INTERACT)
	{
	  /* The application is currently interacting, so we can't
	   * tell it about the cancellation yet; we will wait until
	   * after it calls egg_sm_client_will_quit().
	   */
	  xsmp->state = XSMP_STATE_SHUTDOWN_CANCELLED;
	}
      else
	{
	  /* The shutdown was cancelled before the application got a
	   * chance to interact.
	   */
	  xsmp->state = XSMP_STATE_IDLE;
	}
    }
}

/* Utilities */

/* Create a restart/clone/Exec command based on @restart_command.
 * If @client_id is non-%NULL, add "--sm-client-id @client_id".
 * If @state_file is non-%NULL, add "--sm-client-state-file @state_file".
 *
 * None of the input strings are g_strdup()ed; the caller must keep
 * them around until it is done with the returned GPtrArray, and must
 * then free the array, but not its contents.
 */
static GPtrArray *
generate_command (char **restart_command, const char *client_id,
		  const char *state_file)
{
  GPtrArray *cmd;
  int i;

  cmd = g_ptr_array_new ();
  g_ptr_array_add (cmd, restart_command[0]);

  if (client_id)
    {
      g_ptr_array_add (cmd, (char *)"--sm-client-id");
      g_ptr_array_add (cmd, (char *)client_id);
    }

  if (state_file)
    {
      g_ptr_array_add (cmd, (char *)"--sm-client-state-file");
      g_ptr_array_add (cmd, (char *)state_file);
    }

  for (i = 1; restart_command[i]; i++)
    g_ptr_array_add (cmd, restart_command[i]);

  return cmd;
}

/* Takes a NULL-terminated list of SmProp * values, created by
 * array_prop, ptrarray_prop, string_prop, card8_prop, sets them, and
 * frees them.
 */
static void
set_properties (EggSMClientXSMP *xsmp, ...)
{
  GPtrArray *props;
  SmProp *prop;
  va_list ap;
  guint i;

  props = g_ptr_array_new ();

  va_start (ap, xsmp);
  while ((prop = va_arg (ap, SmProp *)))
    g_ptr_array_add (props, prop);
  va_end (ap);

  if (xsmp->connection)
    {
      SmcSetProperties (xsmp->connection, props->len,
			(SmProp **)props->pdata);
    }

  for (i = 0; i < props->len; i++)
    {
      prop = props->pdata[i];
      g_free (prop->vals);
      g_free (prop);
    }
  g_ptr_array_free (props, TRUE);
}

/* Takes a NULL-terminated list of property names and deletes them. */
static void
delete_properties (EggSMClientXSMP *xsmp, ...)
{
  GPtrArray *props;
  char *prop;
  va_list ap;

  if (!xsmp->connection)
    return;

  props = g_ptr_array_new ();

  va_start (ap, xsmp);
  while ((prop = va_arg (ap, char *)))
    g_ptr_array_add (props, prop);
  va_end (ap);

  SmcDeleteProperties (xsmp->connection, props->len,
		       (char **)props->pdata);

  g_ptr_array_free (props, TRUE);
}

/* Takes an array of strings and creates a LISTofARRAY8 property. The
 * strings are neither dupped nor freed; they need to remain valid
 * until you're done with the SmProp.
 */
static SmProp *
array_prop (const char *name, ...) 
{
  SmProp *prop;
  SmPropValue pv;
  GArray *vals;
  char *value;
  va_list ap;

  prop = g_new (SmProp, 1);
  prop->name = (char *)name;
  prop->type = (char *)SmLISTofARRAY8;

  vals = g_array_new (FALSE, FALSE, sizeof (SmPropValue));

  va_start (ap, name);
  while ((value = va_arg (ap, char *)))
    {
      pv.length = strlen (value);
      pv.value = value;
      g_array_append_val (vals, pv);
    }

  prop->num_vals = vals->len;
  prop->vals = (SmPropValue *)vals->data;

  g_array_free (vals, FALSE);

  return prop;
}

/* Takes a GPtrArray of strings and creates a LISTofARRAY8 property.
 * The array contents are neither dupped nor freed; they need to
 * remain valid until you're done with the SmProp.
 */
static SmProp *
ptrarray_prop (const char *name, GPtrArray *values)
{
  SmProp *prop;
  SmPropValue pv;
  GArray *vals;
  guint i;

  prop = g_new (SmProp, 1);
  prop->name = (char *)name;
  prop->type = (char *)SmLISTofARRAY8;

  vals = g_array_new (FALSE, FALSE, sizeof (SmPropValue));

  for (i = 0; i < values->len; i++)
    {
      pv.length = strlen (values->pdata[i]);
      pv.value = values->pdata[i];
      g_array_append_val (vals, pv);
    }

  prop->num_vals = vals->len;
  prop->vals = (SmPropValue *)vals->data;

  g_array_free (vals, FALSE);

  return prop;
}

/* Takes a string and creates an ARRAY8 property. The string is
 * neither dupped nor freed; it needs to remain valid until you're
 * done with the SmProp.
 */
static SmProp *
string_prop (const char *name, const char *value)
{
  SmProp *prop;

  prop = g_new (SmProp, 1);
  prop->name = (char *)name;
  prop->type = (char *)SmARRAY8;

  prop->num_vals = 1;
  prop->vals = g_new (SmPropValue, 1);

  prop->vals[0].length = strlen (value);
  prop->vals[0].value = (char *)value;

  return prop;
}

/* Takes a char and creates a CARD8 property. */
static SmProp *
card8_prop (const char *name, unsigned char value)
{
  SmProp *prop;
  char *card8val;

  /* To avoid having to allocate and free prop->vals[0], we cheat and
   * make vals a 2-element-long array and then use the second element
   * to store value.
   */

  prop = g_new (SmProp, 1);
  prop->name = (char *)name;
  prop->type = (char *)SmCARD8;

  prop->num_vals = 1;
  prop->vals = g_new (SmPropValue, 2);
  card8val = (char *)(&prop->vals[1]);
  card8val[0] = value;

  prop->vals[0].length = 1;
  prop->vals[0].value = card8val;

  return prop;
}

/* ICE code. This makes no effort to play nice with anyone else trying
 * to use libICE. Fortunately, no one uses libICE for anything other
 * than SM. (DCOP uses ICE, but it has its own private copy of
 * libICE.)
 *
 * When this moves to gtk, it will need to be cleverer, to avoid
 * tripping over old apps that use GnomeClient or that use libSM
 * directly.
 */

#include <X11/ICE/ICElib.h>
#include <fcntl.h>

static void        ice_error_handler    (IceConn        ice_conn,
					 Bool           swap,
					 int            offending_minor_opcode,
					 unsigned long  offending_sequence,
					 int            error_class,
					 int            severity,
					 IcePointer     values);
static void        ice_io_error_handler (IceConn        ice_conn);
static void        ice_connection_watch (IceConn        ice_conn,
					 IcePointer     client_data,
					 Bool           opening,
					 IcePointer    *watch_data);

static void
ice_init (void)
{
  IceSetIOErrorHandler (ice_io_error_handler);
  IceSetErrorHandler (ice_error_handler);
  IceAddConnectionWatch (ice_connection_watch, NULL);
}

static gboolean
process_ice_messages (IceConn ice_conn)
{
  IceProcessMessagesStatus status;

  gdk_threads_enter ();
  status = IceProcessMessages (ice_conn, NULL, NULL);
  gdk_threads_leave ();

  switch (status)
    {
    case IceProcessMessagesSuccess:
      return TRUE;

    case IceProcessMessagesIOError:
      sm_client_xsmp_disconnect (IceGetConnectionContext (ice_conn));
      return FALSE;

    case IceProcessMessagesConnectionClosed:
      return FALSE;

    default:
      g_assert_not_reached ();
    }
}

static gboolean
ice_iochannel_watch (GIOChannel   *channel,
		     GIOCondition  condition,
		     gpointer      client_data)
{
  return process_ice_messages (client_data);
}

static void
ice_connection_watch (IceConn     ice_conn,
		      IcePointer  client_data,
		      Bool        opening,
		      IcePointer *watch_data)
{
  guint watch_id;

  if (opening)
    {
      GIOChannel *channel;
      int fd = IceConnectionNumber (ice_conn);

      fcntl (fd, F_SETFD, fcntl (fd, F_GETFD, 0) | FD_CLOEXEC);
      channel = g_io_channel_unix_new (fd);
      watch_id = g_io_add_watch (channel, G_IO_IN | G_IO_ERR,
				 ice_iochannel_watch, ice_conn);
      g_io_channel_unref (channel);

      *watch_data = GUINT_TO_POINTER (watch_id);
    }
  else
    {
      watch_id = GPOINTER_TO_UINT (*watch_data);
      g_source_remove (watch_id);
    }
}

static void
ice_error_handler (IceConn       ice_conn,
		   Bool          swap,
		   int           offending_minor_opcode,
		   unsigned long offending_sequence,
		   int           error_class,
		   int           severity,
		   IcePointer    values)
{
  /* Do nothing */
} 

static void
ice_io_error_handler (IceConn ice_conn)
{
  /* Do nothing */
} 

static void
smc_error_handler (SmcConn       smc_conn,
                   Bool          swap,
                   int           offending_minor_opcode,
                   unsigned long offending_sequence,
                   int           error_class,
                   int           severity,
                   SmPointer     values)
{
  /* Do nothing */
}
