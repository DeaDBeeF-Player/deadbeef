[+ AutoGen5 template h c +]
/*
** Copyright (C) 2002-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
**	Utility functions to make writing the test suite easier.
**
**	The .c and .h files were generated automagically with Autogen from
**	the files utils.def and utils.tpl.
*/

[+ CASE (suffix) +]
[+ ==  h  +]

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#include <stdint.h>
#include <stdarg.h>

#define SF_COUNT_TO_LONG(x)	((long) (x))
#define	ARRAY_LEN(x)		((int) (sizeof (x)) / (sizeof ((x) [0])))
#define SIGNED_SIZEOF(x)	((int64_t) (sizeof (x)))
#define	NOT(x)				(! (x))

#define	PIPE_INDEX(x)	((x) + 500)
#define	PIPE_TEST_LEN	12345


[+ FOR float_type
+]void gen_windowed_sine_[+ (get "name") +] ([+ (get "name") +] *data, int len, double maximum) ;
[+ ENDFOR float_type
+]

void	create_short_sndfile (const char *filename, int format, int channels) ;

void	check_file_hash_or_die	(const char *filename, uint64_t target_hash, int line_num) ;

void	print_test_name (const char *test, const char *filename) ;

void	dump_data_to_file (const char *filename, const void *data, unsigned int datalen) ;

void	write_mono_file (const char * filename, int format, int srate, float * output, int len) ;

static inline void
exit_if_true (int test, const char *format, ...)
{	if (test)
	{	va_list	argptr ;
		va_start (argptr, format) ;
		vprintf (format, argptr) ;
		va_end (argptr) ;
		exit (1) ;
		} ;
} /* exit_if_true */

/*
**	Functions for saving two vectors of data in an ascii text file which
**	can then be loaded into GNU octave for comparison.
*/

[+ FOR io_type
+]int	oct_save_[+ (get "io_element") +]	(const [+ (get "io_element") +] *a, const [+ (get "io_element") +] *b, int len) ;
[+ ENDFOR io_type
+]

void	delete_file (int format, const char *filename) ;

void	count_open_files (void) ;
void	increment_open_file_count (void) ;
void	check_open_file_count_or_die (int lineno) ;

#ifdef SNDFILE_H

static inline void
sf_info_clear (SF_INFO * info)
{	memset (info, 0, sizeof (SF_INFO)) ;
} /* sf_info_clear */

static inline void
sf_info_setup (SF_INFO * info, int format, int samplerate, int channels)
{	sf_info_clear (info) ;

	info->format = format ;
	info->samplerate = samplerate ;
	info->channels = channels ;
} /* sf_info_setup */


void 	dump_log_buffer (SNDFILE *file) ;
void 	check_log_buffer_or_die (SNDFILE *file, int line_num) ;
int 	string_in_log_buffer (SNDFILE *file, const char *s) ;
void	hexdump_file (const char * filename, sf_count_t offset, sf_count_t length) ;


SNDFILE *test_open_file_or_die
			(const char *filename, int mode, SF_INFO *sfinfo, int allow_fd, int line_num) ;

void 	test_read_write_position_or_die
			(SNDFILE *file, int line_num, int pass, sf_count_t read_pos, sf_count_t write_pos) ;

void	test_seek_or_die
			(SNDFILE *file, sf_count_t offset, int whence, sf_count_t new_pos, int channels, int line_num) ;

[+ FOR read_op +]
[+ FOR io_type
+]void 	test_[+ (get "op_element") +]_[+ (get "io_element") +]_or_die
			(SNDFILE *file, int pass, [+ (get "io_element") +] *test, sf_count_t [+ (get "count_name") +], int line_num) ;
[+ ENDFOR io_type +][+ ENDFOR read_op +]

void
test_read_raw_or_die (SNDFILE *file, int pass, void *test, sf_count_t items, int line_num) ;

[+ FOR write_op +]
[+ FOR io_type
+]void 	test_[+ (get "op_element") +]_[+ (get "io_element") +]_or_die
			(SNDFILE *file, int pass, const [+ (get "io_element") +] *test, sf_count_t [+ (get "count_name") +], int line_num) ;
[+ ENDFOR io_type +][+ ENDFOR write_op +]

void
test_write_raw_or_die (SNDFILE *file, int pass, const void *test, sf_count_t items, int line_num) ;

[+ FOR io_type
+]void compare_[+ (get "io_element") +]_or_die (const [+ (get "io_element") +] *left, const [+ (get "io_element") +] *right, unsigned count, int line_num) ;
[+ ENDFOR io_type +]


void	gen_lowpass_noise_float (float *data, int len) ;

sf_count_t		file_length (const char * fname) ;
sf_count_t		file_length_fd (int fd) ;

#endif

#ifdef __cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

[+  ==  c  +]

#include "sfconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if (HAVE_DECL_S_IRGRP == 0)
#include <sf_unistd.h>
#endif

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sndfile.h>

#include "utils.h"

#ifndef	M_PI
#define	M_PI		3.14159265358979323846264338
#endif

#define	LOG_BUFFER_SIZE		2048

/*
**	Neat solution to the Win32/OS2 binary file flage requirement.
**	If O_BINARY isn't already defined by the inclusion of the system
**	headers, set it to zero.
*/
#ifndef O_BINARY
#define O_BINARY 0
#endif

[+ FOR float_type +]
void
gen_windowed_sine_[+ (get "name") +] ([+ (get "name") +] *data, int len, double maximum)
{	int k ;

	memset (data, 0, len * sizeof ([+ (get "name") +])) ;
	/*
	**	Choose a frequency of 1/32 so that it aligns perfectly with a DFT
	**	bucket to minimise spreading of energy over more than one bucket.
	**	Also do not want to make the frequency too high as some of the
	**	codecs (ie gsm610) have a quite severe high frequency roll off.
	*/
	len /= 2 ;

	for (k = 0 ; k < len ; k++)
	{	data [k] = sin (2.0 * k * M_PI * 1.0 / 32.0 + 0.4) ;

		/* Apply Hanning Window. */
		data [k] *= maximum * (0.5 - 0.5 * cos (2.0 * M_PI * k / ((len) - 1))) ;
		}

	return ;
} /* gen_windowed_sine_[+ (get "name") +] */
[+ ENDFOR float_type +]

void
create_short_sndfile (const char *filename, int format, int channels)
{	short data [2 * 3 * 4 * 5 * 6 * 7] = { 0, } ;
	SNDFILE *file ;
	SF_INFO sfinfo ;

	sfinfo.samplerate = 44100 ;
	sfinfo.channels = channels ;
	sfinfo.format = format ;

	if ((file = sf_open (filename, SFM_WRITE, &sfinfo)) == NULL)
	{	printf ("Error (%s, %d) : sf_open failed : %s\n", __FILE__, __LINE__, sf_strerror (file)) ;
		exit (1) ;
		} ;

	sf_write_short (file, data, ARRAY_LEN (data)) ;

	sf_close (file) ;
} /* create_short_sndfile */

void
check_file_hash_or_die (const char *filename, uint64_t target_hash, int line_num)
{	static unsigned char buf [4096] ;
	uint64_t	cksum ;
	FILE 		*file ;
	int			k, read_count ;

	memset (buf, 0, sizeof (buf)) ;

	/* The 'b' in the mode string means binary for Win32. */
	if ((file = fopen (filename, "rb")) == NULL)
	{	printf ("\n\nLine %d: could not open file '%s'\n\n", line_num, filename) ;
		exit (1) ;
		} ;

	cksum = 0 ;

	while ((read_count = fread (buf, 1, sizeof (buf), file)))
		for (k = 0 ; k < read_count ; k++)
			cksum = cksum * 511 + buf [k] ;

	fclose (file) ;

	if (target_hash == 0)
	{	printf (" 0x%016" PRIx64 "\n", cksum) ;
		return ;
		} ;

	if (cksum != target_hash)
	{	printf ("\n\nLine %d: incorrect hash value 0x%016" PRIx64 " should be 0x%016" PRIx64 ".\n\n", line_num, cksum, target_hash) ;
		exit (1) ;
		} ;

	return ;
} /* check_file_hash_or_die */

void
print_test_name (const char *test, const char *filename)
{	int count ;

	if (test == NULL)
	{	printf (__FILE__ ": bad test of filename parameter.\n") ;
		exit (1) ;
		} ;

	if (filename == NULL || strlen (filename) == 0)
	{	printf ("    %-30s : ", test) ;
		count = 25 ;
		}
	else
	{	printf ("    %-30s : %s ", test, filename) ;
		count = 24 - strlen (filename) ;
		} ;

	while (count -- > 0)
		putchar ('.') ;
	putchar (' ') ;

	fflush (stdout) ;
} /* print_test_name */

void
dump_data_to_file (const char *filename, const void *data, unsigned int datalen)
{	FILE *file ;

	if ((file = fopen (filename, "wb")) == NULL)
	{	printf ("\n\nLine %d : could not open file : %s\n\n", __LINE__, filename) ;
		exit (1) ;
		} ;

	if (fwrite (data, 1, datalen, file) != datalen)
	{	printf ("\n\nLine %d : fwrite failed.\n\n", __LINE__) ;
		exit (1) ;
		} ;

	fclose (file) ;

} /* dump_data_to_file */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
*/

static char octfilename [] = "error.dat" ;

[+ FOR io_type
+]int
oct_save_[+ (get "io_element") +]	(const [+ (get "io_element") +] *a, const [+ (get "io_element") +] *b, int len)
{	FILE 	*file ;
	int		k ;

	if (! (file = fopen (octfilename, "w")))
		return 1 ;

	fprintf (file, "# Not created by Octave\n") ;

	fprintf (file, "# name: a\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, [+ (get "format_str") +] "\n", a [k]) ;

	fprintf (file, "# name: b\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, [+ (get "format_str") +] "\n", b [k]) ;

	fclose (file) ;
	return 0 ;
} /* oct_save_[+ (get "io_element") +] */
[+ ENDFOR io_type
+]

void
check_log_buffer_or_die (SNDFILE *file, int line_num)
{	static char	buffer [LOG_BUFFER_SIZE] ;
	int			count ;

	memset (buffer, 0, sizeof (buffer)) ;

	/* Get the log buffer data. */
	count = sf_command	(file, SFC_GET_LOG_INFO, buffer, LOG_BUFFER_SIZE) ;

	if (LOG_BUFFER_SIZE - count < 2)
	{	printf ("\n\nLine %d : Possible long log buffer.\n", line_num) ;
		exit (1) ;
		}

	/* Look for "Should" */
	if (strstr (buffer, "ould"))
	{	printf ("\n\nLine %d : Log buffer contains `ould'. Dumping.\n", line_num) ;
		puts (buffer) ;
		exit (1) ;
		} ;

	/* Look for "**" */
	if (strstr (buffer, "*"))
	{	printf ("\n\nLine %d : Log buffer contains `*'. Dumping.\n", line_num) ;
		puts (buffer) ;
		exit (1) ;
		} ;

	/* Look for "Should" */
	if (strstr (buffer, "nknown marker"))
	{	printf ("\n\nLine %d : Log buffer contains `nknown marker'. Dumping.\n", line_num) ;
		puts (buffer) ;
		exit (1) ;
		} ;

	return ;
} /* check_log_buffer_or_die */

int
string_in_log_buffer (SNDFILE *file, const char *s)
{	static char	buffer [LOG_BUFFER_SIZE] ;
	int			count ;

	memset (buffer, 0, sizeof (buffer)) ;

	/* Get the log buffer data. */
	count = sf_command	(file, SFC_GET_LOG_INFO, buffer, LOG_BUFFER_SIZE) ;

	if (LOG_BUFFER_SIZE - count < 2)
	{	printf ("Possible long log buffer.\n") ;
		exit (1) ;
		}

	/* Look for string */
	return strstr (buffer, s) ? SF_TRUE : SF_FALSE ;
} /* string_in_log_buffer */

void
hexdump_file (const char * filename, sf_count_t offset, sf_count_t length)
{
	FILE * file ;
	char buffer [16] ;
	int k, m, ch, readcount ;

	if (length > 1000000)
	{	printf ("\n\nError : length (%ld) too long.\n\n", SF_COUNT_TO_LONG (offset)) ;
		exit (1) ;
		} ;

	if ((file = fopen (filename, "r")) == NULL)
	{	printf ("\n\nError : hexdump_file (%s) could not open file for read.\n\n", filename) ;
		exit (1) ;
		} ;

	if (fseek (file, offset, SEEK_SET) != 0)
	{	printf ("\n\nError : fseek(file, %ld, SEEK_SET) failed : %s\n\n", SF_COUNT_TO_LONG (offset), strerror (errno)) ;
		exit (1) ;
		} ;

	puts ("\n\n") ;

	for (k = 0 ; k < length ; k+= sizeof (buffer))
	{	readcount = fread (buffer, 1, sizeof (buffer), file) ;

		printf ("%08lx : ", SF_COUNT_TO_LONG (offset + k)) ;

		for (m = 0 ; m < readcount ; m++)
			printf ("%02x ", buffer [m] & 0xFF) ;

		for (m = readcount ; m < SIGNED_SIZEOF (buffer) ; m++)
			printf ("   ") ;

		printf ("  ") ;
		for (m = 0 ; m < readcount ; m++)
		{	ch = isprint (buffer [m]) ? buffer [m] : '.' ;
			putchar (ch) ;
			} ;

		if (readcount < SIGNED_SIZEOF (buffer))
			break ;

		putchar ('\n') ;
		} ;

	puts ("\n") ;

	fclose (file) ;
} /* hexdump_file */

void
dump_log_buffer (SNDFILE *file)
{	static char	buffer [LOG_BUFFER_SIZE] ;

	memset (buffer, 0, sizeof (buffer)) ;

	/* Get the log buffer data. */
	sf_command	(file, SFC_GET_LOG_INFO, buffer, LOG_BUFFER_SIZE) ;

	if (strlen (buffer) < 1)
		puts ("Log buffer empty.\n") ;
	else
		puts (buffer) ;

	return ;
} /* dump_log_buffer */

SNDFILE *
test_open_file_or_die (const char *filename, int mode, SF_INFO *sfinfo, int allow_fd, int line_num)
{	static int count = 0 ;

	SNDFILE *file ;
	const char *modestr, *func_name ;
	int oflags = 0, omode = 0, err ;

	/*
	** Need to test both sf_open() and sf_open_fd().
	** Do so alternately.
	*/
	switch (mode)
	{	case SFM_READ :
				modestr = "SFM_READ" ;
				oflags = O_RDONLY | O_BINARY ;
				omode = 0 ;
				break ;

		case SFM_WRITE :
				modestr = "SFM_WRITE" ;
				oflags = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP ;
				break ;

		case SFM_RDWR :
				modestr = "SFM_RDWR" ;
				oflags = O_RDWR | O_CREAT | O_BINARY ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP ;
				break ;
		default :
				printf ("\n\nLine %d: Bad mode.\n", line_num) ;
				fflush (stdout) ;
				exit (1) ;
		} ;

	if (OS_IS_WIN32)
	{	/* Windows does not understand and ignores the S_IRGRP flag, but Wine
		** gives a run time warning message, so just clear it.
		*/
		omode &= ~S_IRGRP ;
		} ;

	if (allow_fd && ((++count) & 1) == 1)
	{	int fd ;

		/* Only use the three argument open() function if omode != 0. */
		fd = (omode == 0) ? open (filename, oflags) : open (filename, oflags, omode) ;

		if (fd < 0)
		{	printf ("\n\n%s : open failed : %s\n", __func__, strerror (errno)) ;
			exit (1) ;
			} ;

		func_name = "sf_open_fd" ;
		file = sf_open_fd (fd, mode, sfinfo, SF_TRUE) ;
		}
	else
	{	func_name = "sf_open" ;
		file = sf_open (filename, mode, sfinfo) ;
		} ;

	if (file == NULL)
	{	printf ("\n\nLine %d: %s (%s) failed : %s\n\n", line_num, func_name, modestr, sf_strerror (NULL)) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	err = sf_error (file) ;
	if (err != SF_ERR_NO_ERROR)
	{	printf ("\n\nLine %d : sf_error : %s\n\n", line_num, sf_error_number (err)) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	return file ;
} /* test_open_file_or_die */

void
test_read_write_position_or_die (SNDFILE *file, int line_num, int pass, sf_count_t read_pos, sf_count_t write_pos)
{	sf_count_t pos ;

	/* Check the current read position. */
	if (read_pos >= 0 && (pos = sf_seek (file, 0, SEEK_CUR | SFM_READ)) != read_pos)
	{	printf ("\n\nLine %d ", line_num) ;
		if (pass > 0)
			printf ("(pass %d): ", pass) ;
		printf ("Read position (%ld) should be %ld.\n", SF_COUNT_TO_LONG (pos), SF_COUNT_TO_LONG (read_pos)) ;
		exit (1) ;
		} ;

	/* Check the current write position. */
	if (write_pos >= 0 && (pos = sf_seek (file, 0, SEEK_CUR | SFM_WRITE)) != write_pos)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : Write position (%ld) should be %ld.\n",
						SF_COUNT_TO_LONG (pos), SF_COUNT_TO_LONG (write_pos)) ;
		exit (1) ;
		} ;

	return ;
} /* test_read_write_position */

void
test_seek_or_die (SNDFILE *file, sf_count_t offset, int whence, sf_count_t new_pos, int channels, int line_num)
{	sf_count_t	position ;
	const char	*channel_name, *whence_name ;

	switch (whence)
	{	case SEEK_SET :
				whence_name = "SEEK_SET" ;
				break ;
		case SEEK_CUR :
				whence_name = "SEEK_CUR" ;
				break ;
		case SEEK_END :
				whence_name = "SEEK_END" ;
				break ;

		/* SFM_READ */
		case SEEK_SET | SFM_READ :
				whence_name = "SFM_READ | SEEK_SET" ;
				break ;
		case SEEK_CUR | SFM_READ :
				whence_name = "SFM_READ | SEEK_CUR" ;
				break ;
		case SEEK_END | SFM_READ :
				whence_name = "SFM_READ | SEEK_END" ;
				break ;

		/* SFM_WRITE */
		case SEEK_SET | SFM_WRITE :
				whence_name = "SFM_WRITE | SEEK_SET" ;
				break ;
		case SEEK_CUR | SFM_WRITE :
				whence_name = "SFM_WRITE | SEEK_CUR" ;
				break ;
		case SEEK_END | SFM_WRITE :
				whence_name = "SFM_WRITE | SEEK_END" ;
				break ;

		default :
				printf ("\n\nLine %d: bad whence parameter.\n", line_num) ;
				exit (1) ;
		} ;

	channel_name = (channels == 1) ? "Mono" : "Stereo" ;

	if ((position = sf_seek (file, offset, whence)) != new_pos)
	{	printf ("\n\nLine %d : %s : sf_seek (file, %ld, %s) returned %ld (should be %ld).\n\n",
					line_num, channel_name, SF_COUNT_TO_LONG (offset), whence_name,
					SF_COUNT_TO_LONG (position), SF_COUNT_TO_LONG (new_pos)) ;
		exit (1) ;
		} ;

} /* test_seek_or_die */

[+ FOR read_op +]
[+ FOR io_type +]
void
test_[+ (get "op_element") +]_[+ (get "io_element") +]_or_die (SNDFILE *file, int pass, [+ (get "io_element") +] *test, sf_count_t [+ (get "count_name") +], int line_num)
{	sf_count_t count ;

	if ((count = sf_[+ (get "op_element") +]_[+ (get "io_element") +] (file, test, [+ (get "count_name") +])) != [+ (get "count_name") +])
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_[+ (get "op_element") +]_[+ (get "io_element") +] failed with short [+ (get "op_element") +] (%ld => %ld).\n",
						SF_COUNT_TO_LONG ([+ (get "count_name") +]), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_[+ (get "op_element") +]_[+ (get "io_element") +]_or_die */
[+ ENDFOR io_type +][+ ENDFOR read_op +]

void
test_read_raw_or_die (SNDFILE *file, int pass, void *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_read_raw (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_read_raw failed with short read (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_read_raw_or_die */

[+ FOR write_op +]
[+ FOR io_type +]
void
test_[+ (get "op_element") +]_[+ (get "io_element") +]_or_die (SNDFILE *file, int pass, const [+ (get "io_element") +] *test, sf_count_t [+ (get "count_name") +], int line_num)
{	sf_count_t count ;

	if ((count = sf_[+ (get "op_element") +]_[+ (get "io_element") +] (file, test, [+ (get "count_name") +])) != [+ (get "count_name") +])
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_[+ (get "op_element") +]_[+ (get "io_element") +] failed with short [+ (get "op_element") +] (%ld => %ld).\n",
						SF_COUNT_TO_LONG ([+ (get "count_name") +]), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_[+ (get "op_element") +]_[+ (get "io_element") +]_or_die */
[+ ENDFOR io_type +][+ ENDFOR write_op +]

void
test_write_raw_or_die (SNDFILE *file, int pass, const void *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_write_raw (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_write_raw failed with short write (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_write_raw_or_die */


[+ FOR io_type
+]void
compare_[+ (get "io_element") +]_or_die (const [+ (get "io_element") +] *left, const [+ (get "io_element") +] *right, unsigned count, int line_num)
{
	unsigned k ;

	for (k = 0 ; k < count ;k++)
		if (left [k] != right [k])
		{	printf ("\n\nLine %d : Error at index %d, " [+ (get "format_str") +] " should be " [+ (get "format_str") +] ".\n\n", line_num, k, left [k], right [k]) ;
			exit (1) ;
			} ;

	return ;
} /* compare_[+ (get "io_element") +]_or_die */
[+ ENDFOR io_type +]


void
delete_file (int format, const char *filename)
{	char rsrc_name [512], *fname ;

	unlink (filename) ;

	if ((format & SF_FORMAT_TYPEMASK) != SF_FORMAT_SD2)
		return ;

	/*
	** Now try for a resource fork stored as a separate file.
	** Grab the un-adulterated filename again.
	*/
	snprintf (rsrc_name, sizeof (rsrc_name), "%s", filename) ;

	if ((fname = strrchr (rsrc_name, '/')) != NULL)
		fname ++ ;
	else if ((fname = strrchr (rsrc_name, '\\')) != NULL)
		fname ++ ;
	else
		fname = rsrc_name ;

	memmove (fname + 2, fname, strlen (fname) + 1) ;
	fname [0] = '.' ;
	fname [1] = '_' ;

	unlink (rsrc_name) ;
} /* delete_file */

static int allowed_open_files = -1 ;

void
count_open_files (void)
{
#if OS_IS_WIN32
	return ;
#else
	int k, count = 0 ;
	struct stat statbuf ;

	if (allowed_open_files > 0)
		return ;

	for (k = 0 ; k < 1024 ; k++)
		if (fstat (k, &statbuf) == 0)
			count ++ ;

	allowed_open_files = count ;
#endif
} /* count_open_files */

void
increment_open_file_count (void)
{	allowed_open_files ++ ;
} /* increment_open_file_count */

void
check_open_file_count_or_die (int lineno)
{
#if OS_IS_WIN32
	lineno = 0 ;
	return ;
#else
	int k, count = 0 ;
	struct stat statbuf ;

	if (allowed_open_files < 0)
		count_open_files () ;

	for (k = 0 ; k < 1024 ; k++)
		if (fstat (k, &statbuf) == 0)
			count ++ ;

	if (count > allowed_open_files)
	{	printf ("\nLine %d : number of open files (%d) > allowed (%d).\n\n", lineno, count, allowed_open_files) ;
		exit (1) ;
		} ;
#endif
} /* check_open_file_count_or_die */

void
write_mono_file (const char * filename, int format, int srate, float * output, int len)
{	SNDFILE * file ;
	SF_INFO sfinfo ;

	memset (&sfinfo, 0, sizeof (sfinfo)) ;

	sfinfo.samplerate = srate ;
	sfinfo.channels = 1 ;
	sfinfo.format = format ;

	if ((file = sf_open (filename, SFM_WRITE, &sfinfo)) == NULL)
	{	printf ("sf_open (%s) : %s\n", filename, sf_strerror (NULL)) ;
		exit (1) ;
		} ;

	sf_write_float (file, output, len) ;

	sf_close (file) ;
} /* write_mono_file */

void
gen_lowpass_noise_float (float *data, int len)
{	int32_t value = 0x1243456 ;
	double sample, last_val = 0.0 ;
	int k ;

	for (k = 0 ; k < len ; k++)
	{	/* Not a crypto quality RNG. */
		value = 11117 * value + 211231 ;
		value = 11117 * value + 211231 ;
		value = 11117 * value + 211231 ;

		sample = value / (0x7fffffff * 1.000001) ;
		sample = 0.2 * sample - 0.9 * last_val ;

		data [k] = last_val = sample ;
		} ;

} /* gen_lowpass_noise_float */


/*
**	Windows is fucked.
**	If a file is opened R/W and data is written to it, then fstat will return
**	the correct file length, but stat will return zero.
*/

sf_count_t
file_length (const char * fname)
{	struct stat data ;

	if (stat (fname, &data) != 0)
		return 0 ;

	return (sf_count_t) data.st_size ;
} /* file_length */

sf_count_t
file_length_fd (int fd)
{	struct stat data ;

	memset (&data, 0, sizeof (data)) ;
	if (fstat (fd, &data) != 0)
		return 0 ;

	return (sf_count_t) data.st_size ;
} /* file_length_fd */


[+ ESAC +]

