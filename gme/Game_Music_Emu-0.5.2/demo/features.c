/* C example that opens any music file type, opens an m3u playlist if present,
prints its info and voice names, customizes the sound, and fades a track out.
Records to "out.wav". */

static char filename [] = "test.nsf"; /* opens this file (can be any music type) */
static char playlist [] = "test.m3u"; /* uses this playlist, if present*/

#include "gme/gme.h"

#include "Wave_Writer.h" /* wave_ functions for writing sound file */
#include <stdlib.h>
#include <stdio.h>

void handle_error( const char* );

/* Example of loading from memory, which would be useful if using a zip file or
other custom format. In this example it's silly because we could just use
gme_load( &emu, sample_rate, path, 0 ).  */
Music_Emu* load_file( const char* path, long sample_rate )
{
	Music_Emu* emu;
	char* data;
	long  size;
	
	/* Read file data into memory. You might read the data from a zip file or
	other compressed format. */
	FILE* in = fopen( path, "rb" );
	if ( !in )
		handle_error( "Couldn't open file" );
	fseek( in, 0, SEEK_END );
	size = ftell( in );
	rewind( in );
	
	data = malloc( size );
	if ( !data )
		handle_error( "Out of memory" );
	if ( fread( data, size, 1, in ) <= 0 )
		handle_error( "Read error" );
	fclose( in );
	
	handle_error( gme_open_data( data, size, &emu, sample_rate ) );
	free( data ); /* a copy is made of the data */
	return emu;
}

/* Print any warning for most recent emulator action (load, start_track, play) */
void print_warning( Music_Emu* emu )
{
	const char* warning = gme_warning( emu );
	if ( warning )
		printf( "**** Warning: %s\n\n", warning );
}

static char my_data [] = "Our cleanup function was called";

/* Example cleanup function automatically called when emulator is deleted. */
static void my_cleanup( void* my_data )
{
	printf( "\n%s\n", (char*) my_data );
}

int main()
{
	long sample_rate = 44100;
	int track = 0; /* index of track to play (0 = first) */
	int i;
	
	/* Load file into emulator */
	Music_Emu* emu = load_file( filename, sample_rate );
	print_warning( emu );
	
	/* Register cleanup function and confirmation string as data */
	gme_set_user_data( emu, my_data );
	gme_set_user_cleanup( emu, my_cleanup );
	
	/* Load .m3u playlist file. All tracks are assumed to use current file.
	We ignore error here in case there is no m3u file present. */
	gme_load_m3u( emu, playlist );
	print_warning( emu );
	
	/* Get and print main info for track */
	{
		track_info_t info;
		handle_error( gme_track_info( emu, &info, track ) );
		printf( "System   : %s\n", info.system );
		printf( "Game     : %s\n", info.game );
		printf( "Author   : %s\n", info.author );
		printf( "Copyright: %s\n", info.copyright );
		printf( "Comment  : %s\n", info.comment );
		printf( "Dumper   : %s\n", info.dumper );
		printf( "Tracks   : %d\n", (int) info.track_count );
		printf( "\n" );
		printf( "Track    : %d\n", (int) track + 1 );
		printf( "Name     : %s\n", info.song );
		printf( "Length   : %ld:%02ld",
				(long) info.length / 1000 / 60, (long) info.length / 1000 % 60 );
		if ( info.loop_length != 0 )
			printf( " (endless)" );
		printf( "\n\n" );
	}
	
	/* Print voice names */
	for ( i = 0; i < gme_voice_count( emu ); i++ )
		printf( "Voice %d: %s\n", i, gme_voice_names( emu ) [i] );
	
	/* Add some stereo enhancement */
	gme_set_stereo_depth( emu, 0.20 );
	
	/* Adjust equalizer for crisp, bassy sound */
	{
		gme_equalizer_t eq;
		eq.treble = 0.0;
		eq.bass   = 20;
		gme_set_equalizer( emu, &eq );
	}
	
	/* Start track and begin fade at 10 seconds */
	handle_error( gme_start_track( emu, track ) );
	print_warning( emu );
	gme_set_fade( emu, 10 * 1000L );
	
	/* Record track until it ends */
	wave_open( sample_rate, "out.wav" );
	wave_enable_stereo();
	while ( !gme_track_ended( emu ) )
	{
		#define buf_size 1024
		short buf [buf_size];
		handle_error( gme_play( emu, buf_size, buf ) );
		print_warning( emu );
		wave_write( buf, buf_size );
	}
	
	/* Cleanup */
	gme_delete( emu );
	wave_close();
	
	getchar();
	return 0;
}

void handle_error( const char* str )
{
	if ( str )
	{
		printf( "Error: %s\n", str ); getchar();
		exit( EXIT_FAILURE );
	}
}
