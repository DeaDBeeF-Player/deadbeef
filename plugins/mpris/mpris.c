/*
    Sound Menu plugin for DeaDBeeF
    Copyright (C) 2010 Robert Y <Decatf@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <time.h>
#include <gio/gio.h>
#include <libindicate/server.h>

#include <deadbeef/deadbeef.h>
#include "../artwork/artwork.h"

#include "mpris-spec.h"

#define trace(...)
#define ENTRY_OBJECT_PATH_PREFIX "/org/mpris/MediaPlayer2/Track/"
#define DESKTOP_FILE "/usr/share/applications/deadbeef.desktop"

static DB_dsp_t plugin;
DB_functions_t *deadbeef;

static short enabled = 0;
DB_artwork_plugin_t *coverart_plugin = NULL;

static IndicateServer *indicate_server;
static GDBusConnection *connection;
static guint name_own_id;
static guint root_id;
static guint player_id;

static GHashTable *player_property_changes = NULL;
static guint player_property_emit_id = 0;
uintptr_t hash_table_mtx;
uintptr_t emit_id_mtx;

double prev_track_pos;


static gboolean check_can_go_next();
static gboolean check_can_go_prev();
void cover_avail_callback  (const char *fname, const char *artist, const char *album, void *user_data);


/* MPRIS root interface */
static void
handle_root_method_call (GDBusConnection *connection,
			 const char *sender,
			 const char *object_path,
			 const char *interface_name,
			 const char *method_name,
			 GVariant *parameters,
			 GDBusMethodInvocation *invocation,
			 gpointer *user_data)
{
	if (g_strcmp0 (object_path, MPRIS_OBJECT_NAME) != 0 ||
	    g_strcmp0 (interface_name, MPRIS_ROOT_INTERFACE) != 0) {
		g_dbus_method_invocation_return_error (invocation,
						       G_DBUS_ERROR,
						       G_DBUS_ERROR_NOT_SUPPORTED,
						       "Method %s.%s not supported",
						       interface_name,
						       method_name);
		return;
	}

	if (g_strcmp0 (method_name, "Raise") == 0) {
        /* TODO: need way to talk to gtkui plugin */
		g_dbus_method_invocation_return_value (invocation, NULL);
	} else if (g_strcmp0 (method_name, "Quit") == 0) {
        deadbeef->quit();
		g_dbus_method_invocation_return_value (invocation, NULL);
	} else {
		g_dbus_method_invocation_return_error (invocation,
						       G_DBUS_ERROR,
						       G_DBUS_ERROR_NOT_SUPPORTED,
						       "Method %s.%s not supported",
						       interface_name,
						       method_name);
	}
}

static GVariant *
get_root_property (GDBusConnection *connection,
		   const char *sender,
		   const char *object_path,
		   const char *interface_name,
		   const char *property_name,
		   GError **error,
		   gpointer *user_data)
{
	if (g_strcmp0 (object_path, MPRIS_OBJECT_NAME) != 0 ||
	    g_strcmp0 (interface_name, MPRIS_ROOT_INTERFACE) != 0) {
		g_set_error (error,
			     G_DBUS_ERROR,
			     G_DBUS_ERROR_NOT_SUPPORTED,
			     "Property %s.%s not supported",
			     interface_name,
			     property_name);
		return NULL;
	}

	if (g_strcmp0 (property_name, "CanQuit") == 0) {
		return g_variant_new_boolean (TRUE);
	} else if (g_strcmp0 (property_name, "CanRaise") == 0) {
		return g_variant_new_boolean (FALSE);
	} else if (g_strcmp0 (property_name, "HasTrackList") == 0) {
		return g_variant_new_boolean (FALSE);
	} else if (g_strcmp0 (property_name, "Identity") == 0) {
		return g_variant_new_string ("DeaDBeef audio player");
	} else if (g_strcmp0 (property_name, "DesktopEntry") == 0) {
        return g_variant_new_string(DESKTOP_FILE);
	} else if (g_strcmp0 (property_name, "SupportedUriSchemes") == 0) {
        /* need some way to fetch these values */
		const char *fake_supported_schemes[] = {
			"file", "http", "cdda", "smb", "sftp", NULL
		};
		return g_variant_new_strv (fake_supported_schemes, -1);
	} else if (g_strcmp0 (property_name, "SupportedMimeTypes") == 0) {
		const char *fake_supported_mimetypes[] = {
			"application/ogg", "audio/x-vorbis+ogg", "audio/x-flac", "audio/mpeg", NULL
		};
		return g_variant_new_strv (fake_supported_mimetypes, -1);
	}

	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return NULL;
}

static const GDBusInterfaceVTable root_vtable =
{
	(GDBusInterfaceMethodCallFunc) handle_root_method_call,
	(GDBusInterfaceGetPropertyFunc) get_root_property,
	NULL
};


/* MPRIS player interface */

static void
playpause () {
    DB_output_t *output = NULL;
    int state = -1;

    output = deadbeef->get_output();
    if (output == NULL) {
        //printf ("[mpris] playpause(): Could not get output\n");
        return;
    }

    state = output->state();   

    if (state == OUTPUT_STATE_STOPPED) {
        deadbeef->playback_play();
    }
    else if (state == OUTPUT_STATE_PLAYING) {
        deadbeef->playback_pause();
    }
    else if (state == OUTPUT_STATE_PAUSED) {
        deadbeef->playback_play();
    }
    else {
        printf ("[mpris] playpause(): Error\n");
	}    
}

static void 
seek (gint64 offset) 
{
    DB_playItem_t *play_item = NULL;    
    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item == NULL) {
        //printf ("[mpris] seek(): No play item\n");
        return;
    }

    // Play position in seconds
    float playpos = deadbeef->streamer_get_playpos();
    float newpos = playpos+(offset/G_USEC_PER_SEC);

    if (newpos < 0) {
        newpos = 0.f;
    } else if (newpos > play_item->playtime) {
        deadbeef->playback_next();
        return;
    }

    deadbeef->pl_item_unref(play_item);
    deadbeef->streamer_seek (newpos);
}

static GVariant *
get_playback_status (void)
{
	GVariant *v = NULL;
    DB_output_t *output = NULL;
    int state = -1;

    output = deadbeef->get_output();
    if (output == NULL) {
        printf ("[mpris] get_playback_status: Could not get output\n");
        return v;
    }

    state = output->state();

    if (state == OUTPUT_STATE_STOPPED) {
        v = g_variant_new_string ("Stopped");
        //printf ("[mpris] get_playback_status: Stopped\n");
    }
    else if (state == OUTPUT_STATE_PLAYING) {
        v = g_variant_new_string ("Playing");
        //printf ("[mpris] get_playback_status: Playing\n");
    }
    else if (state == OUTPUT_STATE_PAUSED) {
        v = g_variant_new_string ("Paused");
        //printf ("[mpris] get_playback_status: Paused\n");
    }
    else {
        //printf ("[mpris] get_playback_status: Error\n");
	}

	return v;
}

static GVariant *
get_loop_status (void)
{
	GVariant *v = NULL;
    int loop_mode;
    loop_mode = deadbeef->conf_get_int ("playback.loop", 0);

    if (loop_mode == PLAYBACK_MODE_NOLOOP) {
        //printf ("[mpris] get_loop_status: None\n");
        v = g_variant_new_string ("None");
    }
    else if (loop_mode == PLAYBACK_MODE_LOOP_ALL) {
        //printf ("[mpris] get_loop_status: Playlist\n");
        v = g_variant_new_string ("Playlist");
    }
    else if (loop_mode == PLAYBACK_MODE_LOOP_SINGLE) {
        //printf ("[mpris] get_loop_status: Track\n");
        v = g_variant_new_string ("Track");
    }

    return v;
}

static gboolean
set_loop_status (GVariant *value)
{    
    gchar *loop_status;
    g_variant_get (value, "s", &loop_status);

    if (g_strcmp0 (loop_status, "None") == 0) {
        deadbeef->conf_set_int ("playback.loop", PLAYBACK_MODE_NOLOOP);
        return TRUE;
    } else if (g_strcmp0 (loop_status, "Playlist") == 0) {
        deadbeef->conf_set_int ("playback.loop", PLAYBACK_MODE_LOOP_ALL);
        return TRUE;
    } else if (g_strcmp0 (loop_status, "Track") == 0) {
        deadbeef->conf_set_int ("playback.loop", PLAYBACK_MODE_LOOP_SINGLE);
        return TRUE;
    }
    return FALSE;
}

static GVariant *
get_shuffle (void)
{
	GVariant *v = NULL;
    int shuffle_mode;
    shuffle_mode = deadbeef->conf_get_int ("playback.order", 0);

    if (shuffle_mode == PLAYBACK_ORDER_LINEAR) {
        //printf ("[mpris] get_shuffle: None\n");
        v = g_variant_new_boolean (FALSE);
    }
    else if (shuffle_mode == PLAYBACK_ORDER_SHUFFLE) {
        //printf ("[mpris] get_shuffle: Playlist\n");
        v = g_variant_new_boolean (TRUE);
    }
    else if (shuffle_mode == PLAYBACK_ORDER_RANDOM) {
        //printf ("[mpris] get_shuffle: Track\n");
        v = g_variant_new_boolean (TRUE);
    }

    return v;
}

static gboolean
set_shuffle (GVariant *value) 
{
    gboolean shuffle;
    g_variant_get (value, "b", &shuffle);
    
    if (shuffle == FALSE) {
        deadbeef->conf_set_int ("playback.order",PLAYBACK_ORDER_LINEAR);
    } else {
        deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_SHUFFLE);
    }

    return TRUE;
}

static GVariant *
get_metadata (void) 
{
    GVariantBuilder *b;
    DB_playItem_t *play_item = NULL;

    b = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));

    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item == NULL) {
    //printf ("[mpris] get_metadata: No play item\n");
        return g_variant_builder_end (b);;
    }

    char buf[200];
    int buf_size = sizeof(buf);

	gchar *trackid_str; 
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, DB_COLUMN_FILENUMBER, NULL);
    trackid_str = g_strdup_printf(ENTRY_OBJECT_PATH_PREFIX "%s", buf);
    //printf ("[mpris] get_metadata: trackid %s\n", trackid_str);
    g_variant_builder_add (b, "{sv}", "mpris:trackid", g_variant_new("s", trackid_str));
    g_free(trackid_str);

    gchar *title_str;
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, -1, "%t");
    title_str = g_strdup_printf("%s", buf);
    //printf ("[mpris] get_metadata: title_str %s\n", title_str);
    g_variant_builder_add (b, "{sv}", "xesam:title", g_variant_new("s", title_str));
    g_free(title_str);
    
    gchar *artist_str;
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, -1, "%a");
    artist_str = g_strdup_printf("%s", buf);
    //printf ("[mpris] get_metadata: artist_str %s\n", artist_str);
	const char *artist_strv[] = {artist_str, NULL};
    g_variant_builder_add (b, "{sv}", "xesam:artist", g_variant_new_strv (artist_strv, -1));


    gchar *albumartist_str;
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, -1, "%B");
    albumartist_str = g_strdup_printf("%s", buf);
    //printf ("[mpris] get_metadata: albumartist_str %s\n", albumartist_str);
	const char *albumartist_strv[] = {albumartist_str, NULL};
    g_variant_builder_add (b, "{sv}", "xesam:albumArtist", g_variant_new_strv (albumartist_strv, -1));
    g_free(albumartist_str);

    gchar *album_str;
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, -1, "%b");
    album_str = g_strdup_printf("%s", buf);
    //printf ("[mpris] get_metadata: album_str %s\n", album_str);
    g_variant_builder_add (b, "{sv}", "xesam:album", g_variant_new("s", album_str));

    if (coverart_plugin != NULL) {
        gchar *image_fname = coverart_plugin->get_album_art (play_item->fname, 
                                                                artist_str, 
                                                                album_str, 
                                                                cover_avail_callback, NULL);
        g_free(artist_str);
        g_free(album_str);
        if (image_fname) {
            gchar *art_str =  g_strjoin (NULL, "file://", image_fname, NULL);
            g_variant_builder_add (b, "{sv}", "mpris:artUrl", g_variant_new("s", art_str));
            //printf ("[mpris] get_metadata: image_fname %s\n", art_str);
            g_free (image_fname);
            g_free (art_str);
        }
    }

    gchar *track_num_str;
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, -1, "%n");
    track_num_str = g_strdup_printf("%s", buf);
    gint64 track_num = g_ascii_strtoll (track_num_str, NULL, 10);
    //printf ("[mpris] get_metadata: tracknum %ld\n", (long int) track_num);
    g_variant_builder_add (b, "{sv}", "xesam:trackNumber", g_variant_new("x", track_num));
    g_free(track_num_str);

    gint64 length = play_item->playtime * G_USEC_PER_SEC;
    //printf ("[mpris] get_metadata: length %ld\n", (long int) length);
    g_variant_builder_add (b, "{sv}", "mpris:length", g_variant_new("x", length));

    gchar *genre_str;
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, -1, "%g");
    genre_str = g_strdup_printf("%s", buf);
    //printf ("[mpris] get_metadata: genre_str %s\n", genre_str);
	const char *genre_strv[] = {genre_str, NULL};
    g_variant_builder_add (b, "{sv}", "xesam:genre", g_variant_new_strv (genre_strv, -1));
    g_free(genre_str);

    gchar *comment_str;
    deadbeef->pl_format_title (play_item, -1, buf, buf_size, -1, "%c");
    comment_str = g_strdup_printf("%s", buf);
    //printf ("[mpris] get_metadata: comment_str %s\n", comment_str);
	const char *comment_strv[] = {comment_str, NULL};
    g_variant_builder_add (b, "{sv}", "xesam:comment", g_variant_new_strv (comment_strv, -1));
    g_free(comment_str);

    deadbeef->pl_item_unref(play_item);
    //printf ("[mpris] get_metadata: done\n");

    return g_variant_builder_end (b);
}

static GVariant *
get_position (void)
{
    GVariant *v = NULL;
   
    DB_playItem_t *play_item = NULL;    
    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item == NULL) {
        //printf ("[mpris] get_position: No play item\n");
        return v;
    }

    deadbeef->pl_item_unref(play_item);

    float playpos = deadbeef->streamer_get_playpos();
    return g_variant_new_int64 (playpos*G_USEC_PER_SEC);
}

void
set_position (const gchar* track_id, gint64 position)
{
    DB_playItem_t *play_item = NULL;    
    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item == NULL) {
        //printf ("[mpris] set_position: No play item\n");
        return;
    }
    
    gint64 play_time = play_item->playtime*G_USEC_PER_SEC;    
    if (position > play_time) 
        return;

	char *trackid_str;
    trackid_str = g_strdup_printf(ENTRY_OBJECT_PATH_PREFIX "%s", play_item->fname);
    if (g_strcmp0(trackid_str, track_id) != 0) {
        g_free(trackid_str);
        return;
    }
    g_free(trackid_str);
    deadbeef->pl_item_unref(play_item);

    deadbeef->streamer_seek((float)position/G_USEC_PER_SEC);
}

static GVariant *
get_canplay (void) 
{
    DB_playItem_t *play_item = NULL;    
    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item == NULL) {
        return g_variant_new_boolean(FALSE);
    }
    deadbeef->pl_item_unref(play_item);    

    return g_variant_new_boolean(TRUE);
}

static GVariant *
get_canpause (void) 
{
    DB_playItem_t *play_item = NULL;    
    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item == NULL) {
        return g_variant_new_boolean(FALSE);
    }
    deadbeef->pl_item_unref(play_item);

    DB_output_t *output = NULL;
    output = deadbeef->get_output();
    if (output != NULL) {
        if (output->state == OUTPUT_STATE_STOPPED) {            
            return g_variant_new_boolean(FALSE);
        }
    }
    //else printf ("[mpris] get_canpause: Could not get output\n");

    return g_variant_new_boolean(TRUE);
}


///
// PropertiesChanged signal
///
/*
static void
emit_property_change (const gchar *name, GVariant *value) 
{
	GError *error = NULL;
    GVariantBuilder *properties;
    GVariant *properties_changed = NULL;
	const char *invalidated[] = { NULL };
    
    properties = g_variant_builder_new (G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add (properties, "{sv}", name, value);

	properties_changed = g_variant_new ("(sa{sv}^as)",
        				    MPRIS_PLAYER_INTERFACE,
		        		    properties,
		        		    invalidated);    
	g_variant_builder_unref (properties);

	g_dbus_connection_emit_signal (connection,
				       NULL,
				       MPRIS_OBJECT_NAME,
				       "org.freedesktop.DBus.Properties",
				       "PropertiesChanged",
				       properties_changed,
				       &error);

	if (error != NULL) {
		printf ("[mpris] Unable to send MPRIS property changes: %s", error->message);
		g_clear_error (&error);
	}
	//printf ("[mpris] Sent property change: %s\n", name);
}
*/

static gboolean
emit_player_properties (gpointer data)
{
    //printf ("[mpris] emit_player_properties\n");

	GError *error = NULL;
    GVariantBuilder *properties;
    GVariant *properties_changed = NULL;
	const char *invalidated[] = { NULL };
	GHashTableIter iter;
	gpointer propname, propvalue;


    deadbeef->mutex_lock(hash_table_mtx);
    properties = g_variant_builder_new (G_VARIANT_TYPE("a{sv}"));
	g_hash_table_iter_init (&iter, player_property_changes);
    while (g_hash_table_iter_next (&iter, &propname, &propvalue)) {
        g_variant_builder_add (properties, "{sv}", propname, propvalue);
    }
	g_hash_table_destroy (player_property_changes);
	player_property_changes = NULL;
    deadbeef->mutex_unlock(hash_table_mtx);

    properties_changed = g_variant_new ("(sa{sv}^as)",
        				    MPRIS_PLAYER_INTERFACE,
                		    properties,
                		    invalidated);    
    g_variant_builder_unref (properties);

    g_dbus_connection_emit_signal (connection,
			           NULL,
			           MPRIS_OBJECT_NAME,
			           "org.freedesktop.DBus.Properties",
			           "PropertiesChanged",
			           properties_changed,
			           &error);

    deadbeef->mutex_lock(emit_id_mtx);
    player_property_emit_id = 0;
    deadbeef->mutex_unlock(emit_id_mtx);

    if (error != NULL) {
	    printf ("[mpris] Unable to send MPRIS property changes: %s", error->message);
	    g_clear_error (&error);
    }

    //printf ("[mpris] emit_player_properties: done %d\n", player_property_emit_id);

    return FALSE;
}


static void 
add_property_change (const gchar *property, GVariant *value) 
{
    //printf ("[mpris] add_property_change\n");

    deadbeef->mutex_lock(hash_table_mtx);
    if (player_property_changes == NULL) {
        player_property_changes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    }
    g_hash_table_insert (player_property_changes, g_strdup (property), value);
    deadbeef->mutex_unlock(hash_table_mtx);

    if (player_property_emit_id == 0) {
        deadbeef->mutex_lock(emit_id_mtx);
        player_property_emit_id = g_idle_add ((GSourceFunc)emit_player_properties, NULL);
        //printf ("[mpris] add_property_change: done\n");
        deadbeef->mutex_unlock(emit_id_mtx);

        return;
    }

    //printf ("[mpris] add_property_change: source exists %d\n", player_property_emit_id);    
}


// Events from DeaDBeef

/*
    PlaybackStatus: Working
    LoopStatus: need event on playback.loop config change
    Rate: not supported
    Shuffle: need event on playback.order config change
    Metadata: Working
    Volume: Working(?)
    MinimumRate: not supported
    MaximumRate: not supported
    CanGoNext: check&emit value on song start
    CanGoPrevious: check&emit value on song start
    CanPlay: not implemented
    CanPause: dunno how to check this
    CanSeek: dunno how to check this
*/

static int 
db_paused (DB_event_t *ev, uintptr_t data) 
{
    //printf ("[mpris] db_paused: PlaybackStatus\n");
    //emit_property_change  ("PlaybackStatus", get_playback_status());
    add_property_change  ("PlaybackStatus", get_playback_status());
    return 0;
}

static int 
db_track_info_changed (DB_event_t *ev, uintptr_t data) 
{

    //printf ("[mpris] db_track_info_changed\n");
    GVariant *v = NULL;
    DB_output_t *output = NULL;
    int state = -1;

    // Playback state change
    output = deadbeef->get_output();
    if (output == NULL) {
        //printf ("[mpris] db_track_info_changed(): Could not get output\n");
        return 0;
    }

    state = output->state();   
    if (state == OUTPUT_STATE_STOPPED) {
        v = g_variant_new_string ("Stopped");
        //printf ("[mpris] db_track_info_changed: Stopped\n");
    }
    else if (state == OUTPUT_STATE_PLAYING) {
        v = g_variant_new_string ("Playing");
        //printf ("[mpris] db_track_info_changed: Playing\n");
    }
    else if (state == OUTPUT_STATE_PAUSED) {
        v = g_variant_new_string ("Paused");
        //printf ("[mpris] db_track_info_changed: Paused\n");
    }
    else {
        //printf ("[mpris] db_track_info_changed: Error\n");
	}
    //printf ("[mpris] db_track_info_changed: PlaybackStatus\n");
    //emit_property_change  ("PlaybackStatus", v);
    add_property_change  ("PlaybackStatus", v);

    // Seek
    
    // A change of greater than 1 second from the 
    // previous track position is considered a seek.
    // (including the time change from track changes)
    DB_playItem_t *play_item = NULL;    
    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item != NULL) {

        float playpos = deadbeef->streamer_get_playpos();
        float diff = playpos - prev_track_pos;
        if (diff > 1.f || diff < -1.f) {
            //printf ("[mpris] db_track_info_changed(): Seeked: %f\n", diff);

        	GError *error = NULL;
            g_dbus_connection_emit_signal (connection,
			                   NULL,
			                   MPRIS_OBJECT_NAME,
			                   MPRIS_PLAYER_INTERFACE,
			                   "Seeked",
			                   g_variant_new ("(x)", (int)(deadbeef->streamer_get_playpos())*G_USEC_PER_SEC),
			                   &error);
            if (error != NULL) {
	            g_warning ("[mpris] Unable to set MPRIS Seeked signal: %s", error->message);
	            g_clear_error (&error);
            }
        }
        deadbeef->pl_item_unref(play_item);
    }


    // Metadata changes
    v = get_metadata();
    if (v != NULL) {
        //emit_property_change ("Metadata", v);
        add_property_change ("Metadata", v);
    }
    else {
        printf ("[mpris] db_track_info_changed: no track\n");
    }

    return 0;
}

static int 
db_song_changed (DB_event_t *ev, uintptr_t data) 
{
/*
    printf ("[mpris] db_song_changed\n");
    GVariant *v = NULL;
    v = get_metadata();
    if (v != NULL) {
        emit_property_change ("Metadata", v);
    }
*/
    return 0;
}

static int 
db_song_started (DB_event_t *ev, uintptr_t data) 
{
    //printf ("[mpris] db_song_started\n");

    GVariant *v = NULL;
    v = get_metadata();
    if (v != NULL) {
        //emit_property_change ("Metadata", v);
        add_property_change ("Metadata", v);
    }

    gboolean can_go_next = check_can_go_next();
    //emit_property_change ("CanGoNext", g_variant_new_boolean(can_go_next));
    add_property_change ("CanGoNext", g_variant_new_boolean(can_go_next)); 
  
    gboolean can_go_prev = check_can_go_prev();
    //emit_property_change ("CanGoPrev", g_variant_new_boolean(can_go_prev));
    add_property_change ("CanGoPrev", g_variant_new_boolean(can_go_prev));

    return 0;
}

static int 
db_song_finished (DB_event_t *ev, uintptr_t data) 
{
   //printf ("[mpris] db_song_finished\n");
/*
    GVariant *v = NULL;
    v = get_metadata();
    if (v != NULL) {
        emit_property_change ("Metadata", v);
    }
*/
    return 0;
}

static int 
db_volume_changed (DB_event_t *ev, uintptr_t data) 
{
    gdouble volume = deadbeef->volume_get_amp();
    if (volume < 0)
        volume = 0;
    //emit_property_change ("Volume", g_variant_new_double (volume));
    add_property_change ("Volume", g_variant_new_double (volume));
    printf ("[mpris] db_volume_changed: Volume %lf\n", volume);
    return 0;
}

static int
db_frameupdate  (DB_event_t *ev, uintptr_t data) 
{

    DB_playItem_t *play_item = NULL;    
    play_item = deadbeef->streamer_get_playing_track ();
    if (play_item != NULL) {
        prev_track_pos = deadbeef->streamer_get_playpos();
        deadbeef->pl_item_unref(play_item);
    }
    return 0;
}

static int
db_playlist_switched (DB_event_t *ev, uintptr_t data) 
{
    //printf ("[mpris] db_playlist_switched\n");
    return 0;
}

static int 
db_activate (DB_event_t *ev, uintptr_t data) 
{
    printf ("[mpris] db_activate()\n");
    return 0;
}

// non callback emit helper functions
static gboolean
check_can_go_next() {


    int loop_mode = deadbeef->conf_get_int ("playback.loop", 0);
    if (loop_mode == PLAYBACK_MODE_NOLOOP) {
        int playlist_size = deadbeef->pl_getcount(deadbeef->plt_get_curr());
    
        // Check if playlist is on last track
        DB_playItem_t *play_item = NULL;    
        play_item = deadbeef->streamer_get_playing_track ();
        if (play_item == NULL) {
            deadbeef->pl_item_unref(play_item);            
            return FALSE;
        }

        int pl_current = deadbeef->pl_get_idx_of(play_item);        
        if (pl_current == playlist_size) {
            deadbeef->pl_item_unref(play_item);
            return FALSE;   
        }
        deadbeef->pl_item_unref(play_item);
        //printf ("[mpris] check_cangonext() plt count: %d plt current: %d\n", playlist_size, pl_current);
    }

    return TRUE;
}

static gboolean
check_can_go_prev() {

    int loop_mode = deadbeef->conf_get_int ("playback.loop", 0);
    if (loop_mode == PLAYBACK_MODE_NOLOOP) {
        // Check if playlist is on first track
        DB_playItem_t *play_item = NULL;    
        play_item = deadbeef->streamer_get_playing_track ();
        if (play_item == NULL) {
            return FALSE;
        }

        int pl_current = deadbeef->pl_get_idx_of(play_item);
        if (pl_current == 0) {
            deadbeef->pl_item_unref(play_item);
            return TRUE;
        }
        deadbeef->pl_item_unref(play_item);
        //printf ("[mpris] check_cangoprev() plt current: %d\n", pl_current);
    }
    return TRUE;
}

// Artwork plugin callback
void cover_avail_callback  (const char *fname, const char *artist, const char *album, void *user_data) {
    //printf ("[mpris] cover_avail_callback\n");
    GVariant *v = get_metadata();
    //emit_property_change ("Metadata", v);
    add_property_change ("Metadata", v);
}


///
// MediaPlayer2.Player handlers
///
static void
handle_result (GDBusMethodInvocation *invocation)
{
	g_dbus_method_invocation_return_error_literal (invocation,
						       G_DBUS_ERROR,
						       G_DBUS_ERROR_FAILED,
						       "Unknown error");
}

static void
handle_player_method_call (GDBusConnection *connection,
			   const char *sender,
			   const char *object_path,
			   const char *interface_name,
			   const char *method_name,
			   GVariant *parameters,
			   GDBusMethodInvocation *invocation,
			   gpointer *user_data)
{

	if (g_strcmp0 (object_path, MPRIS_OBJECT_NAME) != 0 ||
	    g_strcmp0 (interface_name, MPRIS_PLAYER_INTERFACE) != 0) {
		g_dbus_method_invocation_return_error (invocation,
						       G_DBUS_ERROR,
						       G_DBUS_ERROR_NOT_SUPPORTED,
						       "Method %s.%s not supported",
						       interface_name,
						       method_name);
		return;
	}

	if (g_strcmp0 (method_name, "Next") == 0) {
        deadbeef->playback_next();
        handle_result (invocation);
	} else if (g_strcmp0 (method_name, "Previous") == 0) {
        deadbeef->playback_prev();
        handle_result (invocation);
	} else if (g_strcmp0 (method_name, "PlayPause") == 0) {
        playpause();
        handle_result (invocation);
	} else if (g_strcmp0 (method_name, "Stop") == 0) {
        deadbeef->playback_stop();
        handle_result (invocation);
	} else if (g_strcmp0 (method_name, "Play") == 0) {
        deadbeef->playback_play();
        handle_result (invocation);
	} else if (g_strcmp0 (method_name, "Seek") == 0) {
        gint64 offset;
        g_variant_get (parameters, "(x)", &offset);
        seek (offset);
        handle_result (invocation);
	} else if (g_strcmp0 (method_name, "SetPosition") == 0) {
        gint64 position;
        const gchar *track_id;
        g_variant_get (parameters, "&ox", &track_id, &position);
        set_position (track_id, position);
        handle_result (invocation);
	} else if (g_strcmp0 (method_name, "OpenUri") == 0) {
        /* TODO: implement this */
		const char *uri;
		g_variant_get (parameters, "(&s)", &uri);

		g_dbus_method_invocation_return_error (invocation,
						       G_DBUS_ERROR,
						       G_DBUS_ERROR_NOT_SUPPORTED,
						       "Method %s.%s not supported",
						       interface_name,
						       method_name);
        
    } else {
		g_dbus_method_invocation_return_error (invocation,
						       G_DBUS_ERROR,
						       G_DBUS_ERROR_NOT_SUPPORTED,
						       "Method %s.%s not supported",
						       interface_name,
						       method_name);
    }
}

static GVariant *
get_player_property (GDBusConnection *connection,
		     const char *sender,
		     const char *object_path,
		     const char *interface_name,
		     const char *property_name,
		     GError **error,
		     gpointer *user_data)
{

    //printf ("[mpris] get_player_property\n");
    if (g_strcmp0 (property_name, "PlaybackStatus") == 0) {
        return get_playback_status();
    } else if (g_strcmp0 (property_name, "LoopStatus") == 0) {
		return get_loop_status ();
	} else if (g_strcmp0 (property_name, "Rate") == 0) {
		return g_variant_new_double (1.0);
	} else if (g_strcmp0 (property_name, "Shuffle") == 0) {
		return get_shuffle ();
    } else if (g_strcmp0 (property_name, "Metadata") == 0) {
        //printf ("[mpris] get_player_property: Metadata\n");
        return get_metadata ();
    } else if (g_strcmp0 (property_name, "Volume") == 0) {
        gdouble volume = deadbeef->volume_get_amp();
        return g_variant_new_double(volume);
    } else if (g_strcmp0 (property_name, "Position") == 0) {
        return get_position();
    } else if (g_strcmp0 (property_name, "MinimumRate") == 0) {
        return g_variant_new_double (1.0);
    } else if (g_strcmp0 (property_name, "MaximumRate") == 0) {
        return g_variant_new_double (1.0);
    } else if (g_strcmp0 (property_name, "CanGoNext") == 0) {
        return g_variant_new_boolean (TRUE);
    } else if (g_strcmp0 (property_name, "CanGoPrevious") == 0) {
        return g_variant_new_boolean (TRUE);
    } else if (g_strcmp0 (property_name, "CanPlay") == 0) {
        return get_canplay();
    } else if (g_strcmp0 (property_name, "CanPause") == 0) {
        return get_canpause();
    } else if (g_strcmp0 (property_name, "CanSeek") == 0) {
        /* Not sure how to check if a track is seekable 
            return true anyways */
        return g_variant_new_boolean(TRUE);
    } else if (g_strcmp0 (property_name, "CanControl") == 0) {
        return g_variant_new_boolean(TRUE);
    }

	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return NULL;
}

static gboolean
set_player_property (GDBusConnection *connection,
		     const char *sender,
		     const char *object_path,
		     const char *interface_name,
		     const char *property_name,
		     GVariant *value,
		     GError **error,
		     gpointer *user_data)
{

	if (g_strcmp0 (object_path, MPRIS_OBJECT_NAME) != 0 ||
	    g_strcmp0 (interface_name, MPRIS_PLAYER_INTERFACE) != 0) {
		g_set_error (error,
			     G_DBUS_ERROR,
			     G_DBUS_ERROR_NOT_SUPPORTED,
			     "%s:%s not supported",
			     object_path,
			     interface_name);
		return FALSE;
	}

	if (g_strcmp0 (property_name, "LoopStatus") == 0) {
        return set_loop_status(value);
    } else if (g_strcmp0 (property_name, "Rate") == 0) {
		g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "[mpris] Can't modify playback rate");
        return FALSE;
    } else if (g_strcmp0 (property_name, "Shuffle") == 0) {
        set_shuffle (value);
        return TRUE;
    } else if (g_strcmp0 (property_name, "Volume") == 0) {
        gdouble volume;
        volume = g_variant_get_double (value);
        if (volume < 0) volume = 0;
        deadbeef->volume_set_amp((float)volume);
        printf ("[mpris] set_player_property: volume %lf\n", volume);
        return TRUE;
    }

	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return FALSE;
}

static const GDBusInterfaceVTable player_vtable =
{
	(GDBusInterfaceMethodCallFunc) handle_player_method_call,
	(GDBusInterfaceGetPropertyFunc) get_player_property,
	(GDBusInterfaceSetPropertyFunc) set_player_property,
};


/* DeaDBeef plugin */

static void
name_acquired_cb (GDBusConnection *connection, const char *name, gpointer *user_data)
{
	trace ("[mpris] Successfully acquired dbus name %s\n", name);
}

static void
name_lost_cb (GDBusConnection *connection, const char *name, gpointer *user_data)
{
	trace ("[mpris] Lost dbus name %s\n", name);
}

static gboolean 
mpris_begin ()
{
    indicate_server = indicate_server_ref_default ();
    indicate_server_set_type (indicate_server, "music.deadbeef");
    indicate_server_set_desktop_file (indicate_server, DESKTOP_FILE);
    indicate_server_show (indicate_server);

    GError *error = NULL;
    GDBusInterfaceInfo *ifaceinfo;
    GDBusNodeInfo *node_info;

    connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
    if (error != NULL) {
        g_warning ("[mpris] Unnable to connect to D-Bus session bus: %s", error->message);
        return FALSE;
    }

    // Introspection data
    node_info = g_dbus_node_info_new_for_xml (mpris_introspection_xml, &error);
    if (error != NULL) {
        g_warning ("[mpris] Unnable to read MPRIS interface specification: %s", error->message);
        return -1;
    }
    
    // Root interface
    ifaceinfo = g_dbus_node_info_lookup_interface (node_info, MPRIS_ROOT_INTERFACE);
    root_id = g_dbus_connection_register_object (connection, 
                                                    MPRIS_OBJECT_NAME, 
                                                    ifaceinfo,
                                                    &root_vtable,
                                                    NULL,
                                                    NULL,
                                                    &error);
    if (error != NULL) {
        g_warning ("[mpris] Unnable to register MPRIS root interface: %s", error->message);
        return FALSE;
    }

    // Player interface
    ifaceinfo = g_dbus_node_info_lookup_interface (node_info, MPRIS_PLAYER_INTERFACE);
    player_id = g_dbus_connection_register_object (connection,
                                                    MPRIS_OBJECT_NAME,
                                                    ifaceinfo,
                                                    &player_vtable,
                                                    NULL,
                                                    NULL,
                                                    &error);
	if (error != NULL) {
		g_warning ("[mpris] Unable to register MPRIS player interface: %s", error->message);
		g_error_free (error);
        return FALSE;
	}    

	name_own_id = g_bus_own_name (G_BUS_TYPE_SESSION,
					      MPRIS_BUS_NAME_PREFIX ".deadbeef",
					      G_BUS_NAME_OWNER_FLAGS_NONE,
					      NULL,
					      (GBusNameAcquiredCallback) name_acquired_cb,
					      (GBusNameLostCallback) name_lost_cb,
					      NULL,
					      NULL);


    // DeaDBeef event callbacks
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (db_paused), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (db_track_info_changed), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (db_song_changed), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGSTARTED, DB_CALLBACK (db_song_started), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGFINISHED, DB_CALLBACK (db_song_finished), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (db_volume_changed), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (db_activate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTSWITCH, DB_CALLBACK (db_playlist_switched), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (db_frameupdate), 0);

    //
    player_property_changes = NULL;
    player_property_emit_id = 0;
    hash_table_mtx = deadbeef->mutex_create();
    emit_id_mtx = deadbeef->mutex_create();

    //printf ("[mpris] begin %d\n", name_own_id);
    return TRUE;
}

static void
mpris_end ()
{
    if (coverart_plugin) {
        //coverart_plugin->plugin.plugin.stop ();
        coverart_plugin = NULL;
    }
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (db_paused), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (db_track_info_changed), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (db_song_changed), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGSTARTED, DB_CALLBACK (db_song_started), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGFINISHED, DB_CALLBACK (db_song_finished), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (db_volume_changed), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (db_activate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTSWITCH, DB_CALLBACK (db_playlist_switched), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (db_frameupdate), 0);

    if (root_id != 0) {
        g_dbus_connection_unregister_object (connection, root_id);
        root_id = 0;
    }

    if (player_id != 0) {
        g_dbus_connection_unregister_object (connection, player_id);
        player_id = 0;
    }

    if (name_own_id != 0) {
         g_bus_unown_name (name_own_id);
        name_own_id = 0;
    }

    indicate_server_hide (indicate_server);

    deadbeef->mutex_lock(emit_id_mtx);    
	if (player_property_emit_id != 0) {
		g_source_remove (player_property_emit_id);
		player_property_emit_id = 0;
	}
    deadbeef->mutex_unlock(emit_id_mtx);    
    deadbeef->mutex_free (emit_id_mtx);

    deadbeef->mutex_lock(hash_table_mtx);
	if (player_property_changes != NULL) {
		g_hash_table_destroy (player_property_changes);
		player_property_changes = NULL;
	}
    deadbeef->mutex_unlock(hash_table_mtx);    
    deadbeef->mutex_free (hash_table_mtx);

    //printf ("[mpris] end\n");
}


static int
mpris_connect (void) {
    //printf ("[mpris] mpris_connect\n");
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->id && !g_strcmp0 (p->id, "cover_loader")) {
            trace ("gtkui: found cover-art loader plugin\n");
            coverart_plugin = (DB_artwork_plugin_t *)p;
            break;
        }
    }
    return FALSE;
}

static int
mpris_on_configchanged (DB_event_t *ev, uintptr_t data) {
    int e = deadbeef->conf_get_int ("mpris.enable", 0);
    //printf ("[mpris] configchanged(): enable %d\n", e);
    if (e != enabled) {
        if (e) {
            mpris_begin ();
            mpris_connect ();
            db_track_info_changed (NULL, 0);
        } else {
            mpris_end ();
        }
        enabled = e;
    }
    return 0;
}

static int
mpris_plugin_start (void) {

    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (mpris_on_configchanged), 0);

    int e = deadbeef->conf_get_int ("mpris.enable", 0);    
    if (e != enabled) {
        enabled = e;
        if (enabled == 1) {
                mpris_begin ();
        }
    }

    return 0;
}

static int
mpris_plugin_stop (void) {

    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (mpris_on_configchanged), 0);
       
    if (enabled == 1) {
        mpris_end ();
    }

    return 0;
}

static const char settings_dlg[] =
    "property \"Enable\" checkbox mpris.enable 0;\n"
;

static DB_dsp_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.id = "Sound Menu",
    .plugin.name = "Sound Menu",
    .plugin.descr = "Ubuntu Sound Menu plugin",
    .plugin.author = "Robert Y",
    .plugin.email = "Decatf@gmail.com",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = mpris_plugin_start,
    .plugin.stop = mpris_plugin_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.connect = mpris_connect,
};

DB_plugin_t *
mpris_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
