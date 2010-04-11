/* WAVE sound file writer for recording 16-bit output during program development */

#ifndef WAVE_WRITER_H
#define WAVE_WRITER_H

#ifdef __cplusplus
	extern "C" {
#endif

/* C interface */
void wave_open( long sample_rate, const char* filename );
void wave_enable_stereo( void );
void wave_write( const short* buf, long count );
long wave_sample_count( void );
void wave_close( void );

#ifdef __cplusplus
	}
#endif

#ifdef __cplusplus
#include <stddef.h>
#include <stdio.h>

/* C++ interface */
class Wave_Writer {
public:
	typedef short sample_t;
	
	// Create sound file with given sample rate (in Hz) and filename.
	// Exits program if there's an error.
	Wave_Writer( long sample_rate, char const* filename = "out.wav" );
	
	// Enable stereo output
	void enable_stereo();
	
	// Append 'count' samples to file. Use every 'skip'th source sample; allows
	// one channel of stereo sample pairs to be written by specifying a skip of 2.
	void write( const sample_t*, long count, int skip = 1 );
	
	// Append 'count' floating-point samples to file. Use every 'skip'th source sample;
	// allows one channel of stereo sample pairs to be written by specifying a skip of 2.
	void write( const float*, long count, int skip = 1 );
	
	// Number of samples written so far
	long sample_count() const;
	
	// Finish writing sound file and close it
	void close();
	
	~Wave_Writer();
public:
	// Deprecated
	void stereo( bool b ) { chan_count = b ? 2 : 1; }
private:
	enum { buf_size = 32768 * 2 };
	unsigned char* buf;
	FILE*   file;
	long    sample_count_;
	long    rate;
	long    buf_pos;
	int     chan_count;
	
	void flush();
};

inline void Wave_Writer::enable_stereo() { chan_count = 2; }

inline long Wave_Writer::sample_count() const { return sample_count_; }

#endif

#endif
