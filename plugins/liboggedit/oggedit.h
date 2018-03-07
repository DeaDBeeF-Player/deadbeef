/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library header

  Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#ifndef __OGGEDIT_H
#define __OGGEDIT_H

#define ALBUM_ART_KEY "METADATA_BLOCK_PICTURE"
#define ALBUM_ART_META "metadata_block_picture"

#define OGGEDIT_OK 1
/* End Of File, usually unexpected */
#define OGGEDIT_EOF 0
/* Generic error or system call return value */
#define OGGEDIT_FAULT -1
/* Couldn't sync to Ogg pages, either not an Ogg file, or highly corrupt */
#define OGGEDIT_CANT_FIND_STREAM -2
/* Input file not opened for reading, unlikely */
#define OGGEDIT_FILE_NOT_OPEN -3
/* Seeking in the input file failed, fatal I/O error */
#define OGGEDIT_SEEK_FAILED -4
/* Memory allocation failure */
#define OGGEDIT_ALLOCATION_FAILURE -5
/* Couldn't extract the vendor from a header packet, file probably corrupt or not the right codec */
#define OGGEDIT_CANNOT_PARSE_HEADERS -6
/* Error adding pages to a stream to read a packet, badly corrupt or fatal Ogg internal error */
#define OGGEDIT_FAILED_TO_STREAM_PAGE_FOR_PACKET -7
/* Error adding packets to a stream to write pages, usually a fatal Ogg internal error */
#define OGGEDIT_FAILED_TO_STREAM_PACKET_FOR_PAGE -8
/* ogg_stream_init() failed, fatal Ogg internal error */
#define OGGEDIT_FAILED_TO_INIT_STREAM -9
/* New file cannot be opened for writing, invalid path or no permissions */
#define OGGEDIT_CANNOT_OPEN_OUTPUT_FILE -10
/* File cannot be opened for writing, probably no write permission */
#define OGGEDIT_CANNOT_UPDATE_FILE -11
/* Temporary file cannot be created (no permissions?) */
#define OGGEDIT_CANNOT_OPEN_TEMPORARY_FILE -12
/* Stat comment failed */
#define OGGEDIT_STAT_FAILED -13
/* I/O error writing to file (disk full?) */
#define OGGEDIT_WRITE_ERROR -14
/* Failed to flush pages from a stream, fatal Ogg internal error */
#define OGGEDIT_FLUSH_FAILED -15
/* Renaming temporary file failed (usually sticky bit or directory permissions) */
#define OGGEDIT_RENAME_FAILED -16
/* Image file length not acceptable */
#define OGGEDIT_BAD_FILE_LENGTH -100
/* Cannot read data from image file */
#define OGGEDIT_CANT_READ_IMAGE_FILE -101

/* oggedit_utils.c */
uint8_t *oggedit_vorbis_channel_map(const unsigned channel_count);

// map deadbeef key to vorbiscomment key
// NOTE: this function may modify the key value, e.g. when upper-casing
const char *oggedit_map_tag(char *key, const char *in_or_out);

/* oggedit_art.c */
const char *oggedit_album_art_type(const uint32_t type);
char *oggedit_album_art_tag(DB_FILE *fp, int *res);

/* oggedit_flac.c */
off_t oggedit_flac_stream_info(DB_FILE *in, const off_t start_offset, const off_t end_offset);
off_t oggedit_write_flac_metadata(DB_FILE *in, const char *fname, const off_t offset, const int num_tags, char **tags);

/* oggedit_vorbis.c */
off_t oggedit_vorbis_stream_info(DB_FILE *in, const off_t start_offset, const off_t end_offset, char **codecs);
off_t oggedit_write_vorbis_metadata(DB_FILE *in, const char *fname, const off_t offset, const size_t stream_size, const int num_tags, char **tags);

/* oggedit_opus.c */
int oggedit_write_opus_file(DB_FILE *in, const char *outname, const off_t offset, const bool all_streams);
off_t oggedit_opus_stream_info(DB_FILE *in, const off_t start_offset, const off_t end_offset, char **codecs);
off_t oggedit_write_opus_metadata(DB_FILE *in, const char *fname, const off_t offset, const off_t stream_size, const int output_gain, const uint32_t num_tags, char **tags);

#endif /* __OGGEDIT_H */
