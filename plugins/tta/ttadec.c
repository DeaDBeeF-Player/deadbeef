/*
 * ttadec.c
 *
 * Description:	 TTAv1 decoder library for HW players
 * Developed by: Alexander Djourik <ald@true-audio.com>
 *               Pavel Zhilin <pzh@true-audio.com>
 *
 * Copyright (c) 2004 True Audio Software. All rights reserved.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the True Audio Software nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ttalib.h"
#include "ttadec.h"
#include "filter.h"

/******************* static variables and structures *******************/

static unsigned char isobuffers[ISO_BUFFERS_SIZE + 4];
static unsigned char *iso_buffers_end = isobuffers + ISO_BUFFERS_SIZE;
static unsigned int pcm_buffer_size;

static decoder	tta[MAX_NCH];	// decoder state
static int	cache[MAX_NCH];	// decoder cache

tta_info *ttainfo;		// currently playing file info

static unsigned int fframes;	// number of frames in file
static unsigned int framelen;	// the frame length in samples
static unsigned int lastlen;	// the length of the last frame in samples
static unsigned int data_pos;	// currently playing frame index
static unsigned int data_cur;	// the playing position in frame

static int maxvalue;		// output data max value
static unsigned int *seek_table;	// the playing position table
static unsigned int st_state;	//seek table status

static unsigned int frame_crc32;
static unsigned int bit_count;
static unsigned int bit_cache;
static unsigned char *bitpos;

static int read_id3_tags (tta_info *info);

/************************* crc32 functions *****************************/

#define UPDATE_CRC32(x, crc) crc = \
	(((crc>>8) & 0x00FFFFFF) ^ crc32_table[(crc^x) & 0xFF])

static unsigned int 
crc32 (unsigned char *buffer, unsigned int len) {
	unsigned int i;
	unsigned int crc = 0xFFFFFFFF;

	for (i = 0; i < len; i++) UPDATE_CRC32(buffer[i], crc);

	return (crc ^ 0xFFFFFFFF);
}

/************************* bit operations ******************************/

#define GET_BINARY(value, bits) \
	while (bit_count < bits) { \
		if (bitpos == iso_buffers_end) { \
			if (!fread(isobuffers, 1, \
			    ISO_BUFFERS_SIZE, ttainfo->HANDLE)) { \
			    ttainfo->STATE = READ_ERROR; \
			    return -1; } \
			bitpos = isobuffers; } \
		UPDATE_CRC32(*bitpos, frame_crc32); \
		bit_cache |= *bitpos << bit_count; \
		bit_count += 8; \
		bitpos++; } \
	value = bit_cache & bit_mask[bits]; \
	bit_cache >>= bits; \
	bit_count -= bits; \
	bit_cache &= bit_mask[bit_count];

#define GET_UNARY(value) \
	value = 0; \
	while (!(bit_cache ^ bit_mask[bit_count])) { \
		if (bitpos == iso_buffers_end) { \
			if (!fread(isobuffers, 1, \
			    ISO_BUFFERS_SIZE, ttainfo->HANDLE)) { \
			    ttainfo->STATE = READ_ERROR; \
			    return -1; } \
			bitpos = isobuffers; } \
		value += bit_count; \
		bit_cache = *bitpos++; \
		UPDATE_CRC32(bit_cache, frame_crc32); \
		bit_count = 8; } \
	while (bit_cache & 1) { \
		value++; \
		bit_cache >>= 1; \
		bit_count--; } \
	bit_cache >>= 1; \
	bit_count--;

static void init_buffer_read() {
	frame_crc32 = 0xFFFFFFFFUL;
	bit_count = bit_cache = 0;
	bitpos = iso_buffers_end;
}

static int done_buffer_read() {
	unsigned int crc32, rbytes;

	frame_crc32 ^= 0xFFFFFFFFUL;
	rbytes = iso_buffers_end - bitpos;

	if (rbytes < sizeof(int)) {
	    memcpy(isobuffers, bitpos, 4);
	    if (!fread(isobuffers + rbytes, 1,
		ISO_BUFFERS_SIZE - rbytes, ttainfo->HANDLE))
		return -1;
	    bitpos = isobuffers;
	}

	memcpy(&crc32, bitpos, 4);
	crc32 = ENDSWAP_INT32(crc32);
	bitpos += sizeof(int);
    
	if (crc32 != frame_crc32) return -1;

	bit_cache = bit_count = 0;
	frame_crc32 = 0xFFFFFFFFUL;

	return 0;
}

/************************* decoder functions ****************************/

const char *get_error_str (int error) {
	switch (error) {
	case NO_ERROR:      return "No errors found";
	case OPEN_ERROR:    return "Can't open file";
	case FORMAT_ERROR:  return "Not supported file format";
	case FILE_ERROR:    return "File is corrupted";
	case READ_ERROR:    return "Can't read from file";
	case MEMORY_ERROR:  return "Insufficient memory available";
	default:            return "Unknown error code";
	}
}

int open_tta_file (const char *filename, tta_info *info, unsigned int data_offset) {
	unsigned int checksum;
	unsigned int datasize;
	unsigned int origsize;
	FILE *infile;
	tta_hdr ttahdr;

	// clear the memory
	memset (info, 0, sizeof(tta_info));

	// open file
	infile = fopen(filename, "rb");
	if (!infile) {
		info->STATE = OPEN_ERROR;
		return -1;
	}
	info->HANDLE = infile;

	// get file size
	fseek (infile, 0, SEEK_END);
	info->FILESIZE = ftell (infile);
	fseek (infile, 0, SEEK_SET);

	// read id3 tags
	if (!data_offset) {
//		if ((data_offset = skip_id3_tag (info)) < 0) {
		if ((data_offset = read_id3_tags (info)) < 0) {
		    fclose(infile);
		    return -1;
		}
	} else fseek (infile, data_offset, SEEK_SET);

	// read TTA header
	if (fread (&ttahdr, 1, sizeof (ttahdr), infile) == 0) {
		fclose (infile);
		info->STATE = READ_ERROR;
		return -1;
	}

	// check for TTA3 signature
	if (ENDSWAP_INT32(ttahdr.TTAid) != TTA1_SIGN) {
		fclose (infile);
		info->STATE = FORMAT_ERROR;
		return -1;
	}

	ttahdr.CRC32 = ENDSWAP_INT32(ttahdr.CRC32);
	checksum = crc32((unsigned char *) &ttahdr,
	sizeof(tta_hdr) - sizeof(int));
	if (checksum != ttahdr.CRC32) {
		fclose (infile);
		info->STATE = FILE_ERROR;
		return -1;
	}

	ttahdr.AudioFormat = ENDSWAP_INT16(ttahdr.AudioFormat);
	ttahdr.NumChannels = ENDSWAP_INT16(ttahdr.NumChannels);
	ttahdr.BitsPerSample = ENDSWAP_INT16(ttahdr.BitsPerSample);
	ttahdr.SampleRate = ENDSWAP_INT32(ttahdr.SampleRate);
	ttahdr.DataLength = ENDSWAP_INT32(ttahdr.DataLength);

	// check for player supported formats
	if (ttahdr.AudioFormat != WAVE_FORMAT_PCM ||
		ttahdr.NumChannels > MAX_NCH ||
		ttahdr.BitsPerSample > MAX_BPS ||(
		ttahdr.SampleRate != 16000 &&
		ttahdr.SampleRate != 22050 &&
		ttahdr.SampleRate != 24000 &&
		ttahdr.SampleRate != 32000 &&
		ttahdr.SampleRate != 44100 &&
		ttahdr.SampleRate != 48000 &&
		ttahdr.SampleRate != 64000 &&
		ttahdr.SampleRate != 88200 &&
		ttahdr.SampleRate != 96000)) {
		fclose (infile);
		info->STATE = FORMAT_ERROR;
		return -1;
	}

	// fill the File Info
	info->NCH = ttahdr.NumChannels;
	info->BPS = ttahdr.BitsPerSample;
	info->BSIZE = (ttahdr.BitsPerSample + 7)/8;
	info->FORMAT = ttahdr.AudioFormat;
	info->SAMPLERATE = ttahdr.SampleRate;
	info->DATALENGTH = ttahdr.DataLength;
	info->FRAMELEN = (int) (FRAME_TIME * ttahdr.SampleRate);
	info->LENGTH = ttahdr.DataLength / ttahdr.SampleRate;
	info->DATAPOS = data_offset;

        datasize = info->FILESIZE - info->DATAPOS;
        origsize = info->DATALENGTH * info->BSIZE * info->NCH;

	info->COMPRESS = (double) datasize / origsize;
	info->BITRATE = (int) (info->COMPRESS * info->SAMPLERATE *
		info->NCH * info->BPS / 1000);

	return 0;
}

static void rice_init(adapt *rice, unsigned int k0, unsigned int k1) {
	rice->k0 = k0;
	rice->k1 = k1;
	rice->sum0 = shift_16[k0];
	rice->sum1 = shift_16[k1];
}

static void decoder_init(decoder *tta, int nch, int byte_size) {
	int shift = flt_set[byte_size - 1];
	int i;

	for (i = 0; i < nch; i++) {
		filter_init(&tta[i].fst, shift);
		rice_init(&tta[i].rice, 10, 10);
		tta[i].last = 0;
	}
}

static void seek_table_init (unsigned int *seek_table,
	unsigned int len, unsigned int data_offset) {
	unsigned int *st, frame_len;

	for (st = seek_table; st < (seek_table + len); st++) {
		frame_len = ENDSWAP_INT32(*st);
		*st = data_offset;
		data_offset += frame_len;
	}
}

int set_position (unsigned int pos) {
	unsigned int seek_pos;

	if (pos >= fframes) return 0;
	if (!st_state) {
		ttainfo->STATE = FILE_ERROR;
		return -1;
	}

	seek_pos = ttainfo->DATAPOS + seek_table[data_pos = pos];
	if (fseek(ttainfo->HANDLE, seek_pos, SEEK_SET) < 0) {
		ttainfo->STATE = READ_ERROR;
		return -1;
	}

	data_cur = 0;
	framelen = 0;

	// init bit reader
	init_buffer_read();

	return 0;
}

int player_init (tta_info *info) {
	unsigned int checksum;
	unsigned int data_offset;
	unsigned int st_size;

	ttainfo = info;

	framelen = 0;
	data_pos = 0;
	data_cur = 0;

	lastlen = ttainfo->DATALENGTH % ttainfo->FRAMELEN;
	fframes = ttainfo->DATALENGTH / ttainfo->FRAMELEN + (lastlen ? 1 : 0);
	st_size = (fframes + 1) * sizeof(int);

	seek_table = (unsigned int *) malloc(st_size);
	if (!seek_table) {
		ttainfo->STATE = MEMORY_ERROR;
		return -1;
	}

	// read seek table
	if (!fread(seek_table, st_size, 1, ttainfo->HANDLE)) {
		ttainfo->STATE = READ_ERROR;
		return -1;
	}

	checksum = crc32((unsigned char *) seek_table, st_size - sizeof(int));
	st_state = (checksum == ENDSWAP_INT32(seek_table[fframes]));
	data_offset = sizeof(tta_hdr) + st_size;

	// init seek table
	seek_table_init(seek_table, fframes, data_offset);

	// init bit reader
	init_buffer_read();

	pcm_buffer_size = PCM_BUFFER_LENGTH * ttainfo->BSIZE * ttainfo->NCH;
	maxvalue = (1UL << ttainfo->BPS) - 1;

	return 0;
}

void close_tta_file (tta_info *info) {
	if (info->HANDLE) {
		fclose (info->HANDLE);
		info->HANDLE = NULL;
	}
}

void player_stop () {
	if (seek_table) {
		free(seek_table);
		seek_table = NULL;
	}
}

int get_samples (byte *buffer) {
	unsigned int k, depth, unary, binary;
	byte *p = buffer;
	decoder *dec = tta;
	int *prev = cache;
	int value, res;

	for (res = 0; p < buffer + pcm_buffer_size;) {
		fltst *fst = &dec->fst;
		adapt *rice = &dec->rice;
		int *last = &dec->last;

		if (data_cur == framelen) {
			if (data_pos == fframes) break;
			if (framelen && done_buffer_read()) {
			    if (set_position(data_pos)) return -1;
			    if (res) break;
			}

			if (data_pos == fframes - 1 && lastlen)
				framelen = lastlen;
			else framelen = ttainfo->FRAMELEN;

			decoder_init(tta, ttainfo->NCH, ttainfo->BSIZE);
			data_pos++; data_cur = 0;
		}

		// decode Rice unsigned
		GET_UNARY(unary);

		switch (unary) {
		case 0: depth = 0; k = rice->k0; break;
		default:
			depth = 1; k = rice->k1;
			unary--;
		}

		if (k) {
			GET_BINARY(binary, k);
			value = (unary << k) + binary;
		} else value = unary;

		switch (depth) {
		case 1: 
			rice->sum1 += value - (rice->sum1 >> 4);
			if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
				rice->k1--;
			else if (rice->sum1 > shift_16[rice->k1 + 1])
				rice->k1++;
			value += bit_shift[rice->k0];
		default:
			rice->sum0 += value - (rice->sum0 >> 4);
			if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
				rice->k0--;
			else if (rice->sum0 > shift_16[rice->k0 + 1])
			rice->k0++;
		}

		value = DEC(value);

		// decompress stage 1: adaptive hybrid filter
		hybrid_filter(fst, &value);

		// decompress stage 2: fixed order 1 prediction
		switch (ttainfo->BSIZE) {
		case 1: value += PREDICTOR1(*last, 4); break;	// bps 8
		case 2: value += PREDICTOR1(*last, 5); break;	// bps 16
		case 3: value += PREDICTOR1(*last, 5); break;	// bps 24
		} *last = value;

		// check for errors
		if (abs(value) > maxvalue) {
			unsigned int tail =
				pcm_buffer_size / (ttainfo->BSIZE * ttainfo->NCH) - res;
			memset(buffer, 0, pcm_buffer_size);
			data_cur += tail; res += tail;
			break;
		}

		if (dec < tta + (ttainfo->NCH - 1)) {
			*prev++ = value; dec++;
		} else {
			*prev = value;
			if (ttainfo->NCH > 1) {
				int *r = prev - 1;
				for (*prev += *r/2; r >= cache; r--)
					*r = *(r + 1) - *r;
				for (r = cache; r < prev; r++)
					WRITE_BUFFER(r, ttainfo->BSIZE, p)
			}
			WRITE_BUFFER(prev, ttainfo->BSIZE, p)
			prev = cache;
			data_cur++; res++;
			dec = tta;
		}
	}

	return res;
}

/*
 * Description:	 ID3 tags manipulation routines
 *               Provides read access to ID3 tags v1.1, v2.3.x, v2.4.x
 *               Supported ID3v2 frames: Title, Artist, Album, Track,
 *               Year, Genre, Comment.
 *
 * Copyright (c) 2004 Alexander Djourik. All rights reserved.
 *
 */

static unsigned int unpack_sint28 (const char *ptr) {
	unsigned int value = 0;

	if (ptr[0] & 0x80) return 0;

	value =  value       | (ptr[0] & 0x7f);
	value = (value << 7) | (ptr[1] & 0x7f);
	value = (value << 7) | (ptr[2] & 0x7f);
	value = (value << 7) | (ptr[3] & 0x7f);

	return value;
}

static unsigned int unpack_sint32 (const char *ptr) {
	unsigned int value = 0;

	if (ptr[0] & 0x80) return 0;

	value = (value << 8) | ptr[0];
	value = (value << 8) | ptr[1];
	value = (value << 8) | ptr[2];
	value = (value << 8) | ptr[3];

	return value;
}

static int get_frame_id (const char *id) {
	if (!memcmp(id, "TIT2", 4)) return TIT2;	// Title
	if (!memcmp(id, "TPE1", 4)) return TPE1;	// Artist
	if (!memcmp(id, "TALB", 4)) return TALB;	// Album
	if (!memcmp(id, "TRCK", 4)) return TRCK;	// Track
	if (!memcmp(id, "TYER", 4)) return TYER;	// Year
	if (!memcmp(id, "TCON", 4)) return TCON;	// Genre
	if (!memcmp(id, "COMM", 4)) return COMM;	// Comment
	return 0;
}

#if 0

static int skip_id3_tag (tta_info *info) {
	id3v2_tag id3v2;
	int id3v2_size;

	////////////////////////////////////////
	// skip ID3v2 tag
	if (!fread(&id3v2, 1, sizeof(id3v2_tag), info->HANDLE))
		goto read_error;
	
	if (memcmp(id3v2.id, "ID3", 3)) {
		if (fseek (info->HANDLE, 0, SEEK_SET) < 0)
		    goto read_error;
		return 0;
	}

	if (id3v2.size[0] & 0x80) goto file_error;
	id3v2_size = unpack_sint28(id3v2.size);

	id3v2_size += (id3v2.flags &
		ID3_FOOTERPRESENT_FLAG) ? 20 : 10;
	fseek (info->HANDLE, id3v2_size, SEEK_SET);
	info->ID3.size = id3v2_size;

	return id3v2_size;

file_error:
	ttainfo->STATE = FILE_ERROR;
	return -1;

read_error:
	ttainfo->STATE = READ_ERROR;
	return -1;
}

#endif

static int read_id3_tags (tta_info *info) {
	id3v1_tag id3v1;
	id3v2_tag id3v2;
	id3v2_frame frame_header;
	int id3v2_size;
	char *buffer = NULL;
	char *ptr;

	////////////////////////////////////////
	// ID3v1 support
	if (fseek (info->HANDLE, -(int) sizeof(id3v1_tag),
		SEEK_END) < 0) goto read_error;

	if (!fread (&id3v1, sizeof(id3v1_tag), 1,
		info->HANDLE)) goto read_error;

	if (!memcmp (id3v1.id, "TAG", 3)) {
		memcpy(info->ID3.title, id3v1.title, 30);
		memcpy(info->ID3.artist, id3v1.artist, 30);
		memcpy(info->ID3.album, id3v1.album, 30);
		memcpy(info->ID3.year, id3v1.year, 4);
		memcpy(info->ID3.comment, id3v1.comment, 28);

		if (id3v1.genre > GENRES-1) id3v1.genre = 12;
		sprintf(info->ID3.track, "%02d", id3v1.track);
		if (id3v1.genre && id3v1.genre != 0xFF)
		    sprintf(info->ID3.genre, "%s", genre[id3v1.genre]);
		info->ID3.id3has |= 1;
	}

	if (fseek (info->HANDLE, 0, SEEK_SET) < 0)
		goto read_error;

	////////////////////////////////////////
	// ID3v2 minimal support
	if (!fread(&id3v2, 1, sizeof(id3v2_tag), info->HANDLE))
		goto read_error;
	
	if (memcmp(id3v2.id, "ID3", 3)) {
		if (fseek (info->HANDLE, 0, SEEK_SET) < 0)
		    goto read_error;
		return 0;
	}

	if (id3v2.size[0] & 0x80) goto file_error;
	id3v2_size = unpack_sint28(id3v2.size);

	if (!(buffer = (unsigned char *) malloc (id3v2_size))) {
		ttainfo->STATE = MEMORY_ERROR;
		goto read_done;
	}

	if ((id3v2.flags & ID3_UNSYNCHRONISATION_FLAG) ||
		(id3v2.flags & ID3_EXPERIMENTALTAG_FLAG) ||
		(id3v2.version < 3)) goto read_done;

	if (!fread(buffer, 1, id3v2_size, info->HANDLE)) {
		free (buffer);
		goto read_error;
	}

	ptr = buffer;

	// skip extended header if present
	if (id3v2.flags & ID3_EXTENDEDHEADER_FLAG) {
		int offset = unpack_sint32(ptr);
		ptr += offset;
	}

	// read id3v2 frames
	while (ptr - buffer < id3v2_size) {
		char *data = NULL;
		int data_size, frame_id;
		int size = 0;

		// get frame header
		memcpy(&frame_header, ptr, sizeof(id3v2_frame));
		ptr += sizeof(id3v2_frame);
		data_size = unpack_sint32(frame_header.size);

		// skip unsupported frames
		if (!(frame_id = get_frame_id(frame_header.id)) ||
			frame_header.flags & FRAME_COMPRESSION_FLAG ||
			frame_header.flags & FRAME_ENCRYPTION_FLAG ||
			frame_header.flags & FRAME_UNSYNCHRONISATION_FLAG || (
			*ptr != FIELD_TEXT_ISO_8859_1 &&
			*ptr != FIELD_TEXT_UTF_8)) {
			ptr += data_size;
			continue;
		}

		data_size--; ptr++;

		switch (frame_id) {
		case TIT2:	data = info->ID3.title;
				size = sizeof(info->ID3.title) - 1; break;
		case TPE1:	data = info->ID3.artist;
				size = sizeof(info->ID3.artist) - 1; break;
		case TALB:	data = info->ID3.album;
				size = sizeof(info->ID3.album) - 1; break;
		case TRCK:	data = info->ID3.track;
				size = sizeof(info->ID3.track) - 1; break;
		case TYER:	data = info->ID3.year;
				size = sizeof(info->ID3.year) - 1; break;
		case TCON:	data = info->ID3.genre;
				size = sizeof(info->ID3.genre) - 1; break;
		case COMM:	data = info->ID3.comment;
				size = sizeof(info->ID3.comment) - 1;
				data_size -= 3; ptr += 3;

				// skip zero short description
				if (*ptr == 0) { data_size--; ptr++; }
				break;
		}

		if (data_size < size) size = data_size;
		memcpy(data, ptr, size); data[size] = '\0';
		ptr += data_size;
	}

	info->ID3.id3has |= 2;

read_done:
	if (buffer) free(buffer);

	id3v2_size += (id3v2.flags &
		ID3_FOOTERPRESENT_FLAG) ? 20 : 10;
	fseek (info->HANDLE, id3v2_size, SEEK_SET);
	info->ID3.size = id3v2_size;

	return id3v2_size;

file_error:
	ttainfo->STATE = FILE_ERROR;
	return -1;

read_error:
	ttainfo->STATE = READ_ERROR;
	return -1;
}

/* eof */
