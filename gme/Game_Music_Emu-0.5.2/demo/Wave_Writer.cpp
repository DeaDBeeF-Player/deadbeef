// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "Wave_Writer.h"

#include <assert.h>
#include <stdlib.h>

/* Copyright (C) 2003-2006 by Shay Green. Permission is hereby granted, free
of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the
following conditions: The above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

const int header_size = 0x2C;

static void exit_with_error( const char* str )
{
	printf( "Error: %s\n", str ); getchar();
	exit( EXIT_FAILURE );
}

Wave_Writer::Wave_Writer( long sample_rate, const char* filename )
{
	sample_count_ = 0;
	rate = sample_rate;
	buf_pos = header_size;
	chan_count = 1;
	
	buf = (unsigned char*) malloc( buf_size * sizeof *buf );
	if ( !buf )
		exit_with_error( "Out of memory" );
	
	file = fopen( filename, "wb" );
	if ( !file )
		exit_with_error( "Couldn't open WAVE file for writing" );
	
	setvbuf( file, 0, _IOFBF, 32 * 1024L );
}

void Wave_Writer::flush()
{
	if ( buf_pos && !fwrite( buf, buf_pos, 1, file ) )
		exit_with_error( "Couldn't write WAVE data" );
	buf_pos = 0;
}

void Wave_Writer::write( const sample_t* in, long remain, int skip )
{
	sample_count_ += remain;
	while ( remain )
	{
		if ( buf_pos >= buf_size )
			flush();
		
		long n = (buf_size - buf_pos) / sizeof (sample_t);
		if ( n > remain )
			n = remain;
		remain -= n;
		
		// convert to lsb first format
		unsigned char* p = &buf [buf_pos];
		while ( n-- )
		{
			int s = *in;
			in += skip;
			*p++ = (unsigned char) s;
			*p++ = (unsigned char) (s >> 8);
		}
		
		buf_pos = p - buf;
		assert( buf_pos <= buf_size );
	}
}


void Wave_Writer::write( const float* in, long remain, int skip )
{
	sample_count_ += remain;
	while ( remain )
	{
		if ( buf_pos >= buf_size )
			flush();
		
		long n = (buf_size - buf_pos) / sizeof (sample_t);
		if ( n > remain )
			n = remain;
		remain -= n;
		
		// convert to lsb first format
		unsigned char* p = &buf [buf_pos];
		while ( n-- )
		{
			long s = (long) (*in * 0x7FFF);
			in += skip;
			if ( (short) s != s )
				s = 0x7FFF - (s >> 24); // clamp to 16 bits
			*p++ = (unsigned char) s;
			*p++ = (unsigned char) (s >> 8);
		}
		
		buf_pos = p - buf;
		assert( buf_pos <= buf_size );
	}
}

void Wave_Writer::close()
{
	if ( file )
	{
		flush();
		
		// generate header
		long ds = sample_count_ * sizeof (sample_t);
		long rs = header_size - 8 + ds;
		int frame_size = chan_count * sizeof (sample_t);
		long bps = rate * frame_size;
		unsigned char header [header_size] =
		{
			'R','I','F','F',
			rs,rs>>8,           // length of rest of file
			rs>>16,rs>>24,
			'W','A','V','E',
			'f','m','t',' ',
			0x10,0,0,0,         // size of fmt chunk
			1,0,                // uncompressed format
			chan_count,0,       // channel count
			rate,rate >> 8,     // sample rate
			rate>>16,rate>>24,
			bps,bps>>8,         // bytes per second
			bps>>16,bps>>24,
			frame_size,0,       // bytes per sample frame
			16,0,               // bits per sample
			'd','a','t','a',
			ds,ds>>8,ds>>16,ds>>24// size of sample data
			// ...              // sample data
		};
		
		// write header
		fseek( file, 0, SEEK_SET );
		fwrite( header, sizeof header, 1, file );
		
		fclose( file );
		file = 0;
		free( buf );
	}
}

Wave_Writer::~Wave_Writer()
{
	close();
}

// C interface

static Wave_Writer* ww;

void wave_open( long sample_rate, const char* filename )
{
	ww = new Wave_Writer( sample_rate, filename );
	assert( ww );
}

void wave_enable_stereo() { ww->enable_stereo(); }

long wave_sample_count() { return ww->sample_count(); }
 
void wave_write( const short* buf, long count ) { ww->write( buf, count ); }

void wave_close()
{
	delete ww;
	ww = 0;
}
