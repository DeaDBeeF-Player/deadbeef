#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "songinfo.h"
#include "uadeutils.h"
#include "ossupport.h"
#include "amifilemagic.h"
#include "support.h"


static void asciiline(char *dst, unsigned char *buf)
{
	int i, c;
	for (i = 0; i < 16; i++) {
		c = buf[i];
		if (isgraph(c) || c == ' ') {
			dst[i] = c;
		} else {
			dst[i] = '.';
		}
	}
	dst[i] = 0;
}

static int hexdump(char *info, size_t maxlen, char *filename, size_t toread)
{
	FILE *f;
	size_t rb, ret;
	uint8_t *buf;

	assert(maxlen >= 8192);

	f = fopen(filename, "rb");
	if (f == NULL)
		return 0;

	buf = malloc(toread);
	if (buf == NULL)
		return 0;

	rb = 0;
	while (rb < toread) {
		ret = fread(&buf[rb], 1, toread - rb, f);
		if (ret == 0)
			break;
		rb += ret;
	}

	if (rb > 0) {
		size_t roff = 0;
		size_t woff = 0;

		while (roff < rb) {
			int iret;

			if (woff >= maxlen)
				break;

			if (woff + 32 >= maxlen) {
				strcpy(&info[woff], "\nbuffer overflow...\n");
				woff += strlen(&info[woff]);
				break;
			}

			iret = snprintf(&info[woff], maxlen - woff, "%.3zx:  ",
					roff);
			assert(iret > 0);
			woff += iret;

			if (woff >= maxlen)
				break;

			if (roff + 16 > rb) {
				iret = snprintf(&info[woff], maxlen - woff,
						"Aligned line  ");
				assert(iret > 0);
				woff += iret;

			} else {
				char dbuf[17];
				asciiline(dbuf, &buf[roff]);
				iret = snprintf(&info[woff], maxlen - woff,
						"%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  |%s|",
						buf[roff + 0], buf[roff + 1],
						buf[roff + 2], buf[roff + 3],
						buf[roff + 4], buf[roff + 5],
						buf[roff + 6], buf[roff + 7],
						buf[roff + 8], buf[roff + 9],
						buf[roff + 10], buf[roff + 11],
						buf[roff + 12], buf[roff + 13],
						buf[roff + 14], buf[roff + 15],
						dbuf);
				assert(iret > 0);
				woff += iret;
			}

			if (woff >= maxlen)
				break;

			iret = snprintf(&info[woff], maxlen - woff, "\n");
			woff += iret;

			roff += 16;
		}

		if (woff >= maxlen)
			woff = maxlen - 1;
		info[woff] = 0;
	}

	fclose(f);
	free(buf);
	return rb == 0;
}

static size_t find_tag(uint8_t * buf, size_t startoffset, size_t buflen,
		       uint8_t * tag, size_t taglen)
{
	uint8_t *treasure;

	if (startoffset >= buflen)
		return -1;

	treasure = memmem(buf + startoffset, buflen - startoffset, tag, taglen);
	if (treasure == NULL)
		return -1;

	return (size_t) (treasure - buf);
}

static int string_checker(unsigned char *str, size_t off, size_t maxoff)
{
	assert(maxoff > 0);
	while (off < maxoff) {
		if (*str == 0)
			return 1;
		off++;
		str++;
	}
	return 0;
}

/* Wanted Team's loadseg modules */
static void process_WTWT_mod(char *credits, size_t credits_len,
			     unsigned char *buf, size_t len, char *lo,
			     char *hi, int rel)
{
	int offset, txt_offset, chunk;
	char tmpstr[256];

	/* check for Magic ID */
	offset = find_tag((uint8_t *) buf, 0, len, (uint8_t *) lo, 4);
	if (offset == -1)
		return;

	offset =
	    find_tag((uint8_t *) buf, offset + 4, offset + 8, (uint8_t *) hi,
		     4);
	if (offset == -1)
		return;

	chunk = offset - 8;	/* here's where our first chunk should be */
	offset = offset + rel;	/* offset to our info pointers */

	if (chunk < len && offset < len) {
		txt_offset = read_be_u32(buf + offset) + chunk;
		if (txt_offset < len && txt_offset != chunk) {
			if (!string_checker(buf, txt_offset, len))
				return;
			snprintf(tmpstr, sizeof tmpstr, "\nMODULENAME:\n %s\n",
				 buf + txt_offset);
			strlcat(credits, tmpstr, credits_len);

		}
		txt_offset = read_be_u32(buf + offset + 4) + chunk;
		if (txt_offset < len && txt_offset != chunk) {
			if (!string_checker(buf, txt_offset, len))
				return;
			snprintf(tmpstr, sizeof tmpstr, "\nAUTHORNAME:\n %s\n",
				 buf + txt_offset);
			strlcat(credits, tmpstr, credits_len);
		}

		txt_offset = read_be_u32(buf + offset + 8) + chunk;
		if (txt_offset < len && txt_offset != chunk) {
			if (!string_checker(buf, txt_offset, len))
				return;
			snprintf(tmpstr, sizeof tmpstr, "\nSPECIALINFO:\n %s",
				 buf + txt_offset);
			strlcat(credits, tmpstr, credits_len);
		}
	}
}

/* Get the info out of the AHX  module data*/
static void process_ahx_mod(char *credits, size_t credits_len,
			    unsigned char *buf, size_t len)
{
	int i;
	size_t offset;
	char tmpstr[256];

	if (len < 13)
		return;

	offset = read_be_u16(buf + 4);

	if (offset >= len)
		return;

	if (!string_checker(buf, offset, len))
		return;

	snprintf(tmpstr, sizeof tmpstr, "\nSong title:     %s\n", buf + offset);
	strlcat(credits, tmpstr, credits_len);

	for (i = 0; i < buf[12]; i++) {
		if (!string_checker(buf, offset, len))
			break;
		offset = offset + 1 + strlen((char *)buf + offset);
		if (offset < len) {
			snprintf(tmpstr, 256, "\n           %s", buf + offset);
			strlcat(credits, tmpstr, credits_len);
		}
	}
}

/* Get the info out of the protracker module data*/
static void process_ptk_mod(char *credits, size_t credits_len, int inst,
			    uint8_t * buf, size_t len)
{
	int i;
	char tmpstr[256];

	if (!string_checker(buf, 0, len))
		return;

	snprintf(tmpstr, 35, "\nSong title:     %s", buf);
	strlcat(credits, tmpstr, credits_len);

	if (inst == 31) {
		if (len >= 0x43c) {
			snprintf(tmpstr, sizeof tmpstr,
				 "\nmax positions:  %d\n", buf[0x3b6]);
			strlcat(credits, tmpstr, credits_len);
		}
	} else {
		if (len >= 0x1da) {
			snprintf(tmpstr, sizeof tmpstr,
				 "\nmax positions:  %d\n", buf[0x1d6]);
			strlcat(credits, tmpstr, credits_len);
		}
	}

	snprintf(tmpstr, sizeof tmpstr, "\nINST - NAME                     SIZE VOL FINE LSTART LSIZE\n");
	strlcat(credits, tmpstr, credits_len);
	if (len >= (0x14 + inst * 0x1e)) {
		for (i = 0; i < inst; i++) {
			if (!string_checker(buf, 0x14 + i * 0x1e, len))
				break;
			snprintf(tmpstr, sizeof tmpstr, "[%2d] - ", i + 1);
			strlcat(credits, tmpstr, credits_len);
			snprintf(tmpstr, 23, "%-23s", buf + 0x14 + (i * 0x1e));
			strlcat(credits, tmpstr, credits_len);
			snprintf(tmpstr, sizeof tmpstr,
				 " %6d  %2d  %2d %6d %6d\n",
				 read_be_u16(buf + 42 + i * 0x1e) * 2,
				 buf[45 + i * 0x1e], buf[44 + i * 0x1e],
				 read_be_u16(buf + 46 + i * 0x1e) * 2,
				 read_be_u16(buf + 48 + i * 0x1e) * 2);
			strlcat(credits, tmpstr, credits_len);
		}
	}
}

/* Get the info out of the digibooster module data*/
static void process_digi_mod(char *credits, size_t credits_len,
			     uint8_t * buf, size_t len)
{
	int i;
	char tmpstr[256];

	if (len < (642 + 0x30 * 0x1e))
		return;

	if (!string_checker(buf, 610, len))
		return;

	snprintf(tmpstr, 0x2f, "\nSong title:     %s \n", buf + 610);
	strlcat(credits, tmpstr, credits_len);

	snprintf(tmpstr, sizeof tmpstr, "max positions:  %d\n", buf[47]);
	strlcat(credits, tmpstr, credits_len);

	snprintf(tmpstr, sizeof tmpstr, "\nINST - NAME                                 SIZE VOL  FINE      LSTART       LSIZE\n");
	strlcat(credits, tmpstr, credits_len);
	if (len >= (642 + 0x1f * 0x1e)) {
		for (i = 0; i < 0x1f; i++) {
			if (!string_checker(buf, 642 + i * 0x1e, len))
				break;
			snprintf(tmpstr, sizeof tmpstr, "[%2d] - ", i + 1);
			strlcat(credits, tmpstr, credits_len);
			snprintf(tmpstr, 30, "%-30s", buf + 642 + (i * 0x1e));
			strlcat(credits, tmpstr, credits_len);
			snprintf(tmpstr, sizeof tmpstr,
				 " %11d  %2d   %3d %11d %11d\n",
				 read_be_u32(buf + 176 + i * 4), buf[548 + i],
				 buf[579 + i], read_be_u32(buf + 300 + i * 4),
				 read_be_u32(buf + 424 + i * 4));
			strlcat(credits, tmpstr, credits_len);
		}
	}
}

/* Get the info out of custom song. FIX ME, clean this function. */
static void process_custom(char *credits, size_t credits_len,
			   unsigned char *buf, size_t len)
{
	char tmpstr[1024];
	unsigned char *hunk;
	unsigned char *tag_table;
	int hunk_size;
	int table_size;

	int i;
	int offset;
	unsigned int x, y;
	unsigned char startpattern[4] = { 0x70, 0xff, 0x4e, 0x75 };

	if (len < 4)
		return;

	if (read_be_u32(buf) != 0x000003f3)
		return;

	i = find_tag(buf, 0, len, startpattern, sizeof startpattern);
	if (i == -1 || (i + 12) >= len)
		return;

	if (strncmp((char *)buf + i + 4, "DELIRIUM", 8) != 0 &&
	    strncmp((char *)buf + i + 4, "EPPLAYER", 8) != 0) {
		return;
	}

	/* Hunk found */
	hunk = buf + i;
	hunk_size = len - i;

	if (16 + i + 5 >= hunk_size)
		return;

	/* Check if $VER is available */
	if (!memcmp(&hunk[16], "$VER:", 5)) {
		offset = 16 + 5;
		while (offset < hunk_size) {
			if (memcmp(&hunk[offset], " ", 1))
				break;
			offset++;
		}
		if (offset >= hunk_size)
			return;

		if ((offset + strlen((char *)hunk + offset) + 1) >
		    ((size_t) hunk_size))
			return;

		snprintf(tmpstr, sizeof tmpstr, "\nVERSION:\n%s\n\n",
			 hunk + offset);
		strlcat(credits, tmpstr, credits_len);
	}

	offset = read_be_u32(hunk + 12);
	if (offset < 0) {
		return;
	}

	tag_table = hunk + offset;

	if (tag_table >= &buf[len])
		return;

	table_size = ((int)(&buf[len] - tag_table)) / 8;

	if (table_size <= 0)
		return;

	/* check all tags in this loop */
	for (i = 0; i < table_size; i += 2) {
		x = read_be_u32(tag_table + 4 * i);
		y = read_be_u32(tag_table + 4 * (i + 1));

		if (!x)
			break;

		switch (x) {
		case 0x8000445a:
			if (y >= ((unsigned int)hunk_size))
				return;
			if ((y + strlen((char *)hunk + y) + 1) >
			    ((size_t) hunk_size))
				return;
			snprintf(tmpstr, sizeof tmpstr, "\nCREDITS:\n%s\n\n",
				 hunk + y);
			strlcat(credits, tmpstr, credits_len);
			break;
		default:
			break;
		}
	}
}

/* 
 * Get the info out of the Deltamusic 2 module data
 */
static void process_dm2_mod(char *credits, size_t credits_len,
			    unsigned char *buf, size_t len)
{
	char tmpstr[256];
	if (!string_checker(buf, 0x148, len))
		return;
	snprintf(tmpstr, sizeof tmpstr, "\nRemarks:\n%s", buf + 0x148);
	strlcat(credits, tmpstr, credits_len);
}

static int process_module(char *credits, size_t credits_len, char *filename)
{
	FILE *modfile;
	struct stat st;
	size_t modfilelen;
	unsigned char *buf;
	char pre[11];
	char tmpstr[256];
	size_t rb;

	if (!(modfile = fopen(filename, "rb")))
		return 0;

	if (fstat(fileno(modfile), &st))
		return 0;

	modfilelen = st.st_size;

	if ((buf = malloc(modfilelen)) == NULL) {
		fprintf(stderr, "uade: can't allocate mem in process_module()");
		fclose(modfile);
		return 0;
	}

	rb = 0;
	while (rb < modfilelen) {
		size_t ret = fread(&buf[rb], 1, modfilelen - rb, modfile);
		if (ret == 0)
			break;
		rb += ret;
	}

	fclose(modfile);

	if (rb < modfilelen) {
		fprintf(stderr, "uade: song info could not read %s fully\n",
			filename);
		free(buf);
		return 0;
	}

	snprintf(tmpstr, sizeof tmpstr, "UADE2 MODINFO:\n\nFile name:      %s\nFile length:    %zd bytes\n", filename, modfilelen);
	strlcpy(credits, tmpstr, credits_len);

	/* Get filetype in pre */
	uade_filemagic(buf, modfilelen, pre, modfilelen, filename, 0);

	snprintf(tmpstr, sizeof tmpstr, "File prefix:    %s.*\n", pre);
	strlcat(credits, tmpstr, credits_len);

	if (strcasecmp(pre, "CUST") == 0) {
		/* CUST */
		process_custom(credits, credits_len, buf, modfilelen);

	} else if (strcasecmp(pre, "DM2") == 0) {
		/* DM2 */
		process_dm2_mod(credits, credits_len, buf, modfilelen);

	} else if (strcasecmp(pre, "DIGI") == 0) {
		/* DIGIBooster */
		process_digi_mod(credits, credits_len, buf, modfilelen);

	} else if ((strcasecmp(pre, "AHX") == 0) ||
		   (strcasecmp(pre, "THX") == 0)) {
		/* AHX */
		process_ahx_mod(credits, credits_len, buf, modfilelen);

	} else if ((strcasecmp(pre, "MOD15") == 0) ||
		   (strcasecmp(pre, "MOD15_UST") == 0) ||
		   (strcasecmp(pre, "MOD15_MST") == 0) ||
		   (strcasecmp(pre, "MOD15_ST-IV") == 0)) {
		/*MOD15 */
		process_ptk_mod(credits, credits_len, 15, buf, modfilelen);

	} else if ((strcasecmp(pre, "MOD") == 0) ||
		   (strcasecmp(pre, "MOD_DOC") == 0) ||
		   (strcasecmp(pre, "MOD_NTK") == 0) ||
		   (strcasecmp(pre, "MOD_NTK1") == 0) ||
		   (strcasecmp(pre, "MOD_NTK2") == 0) ||
		   (strcasecmp(pre, "MOD_FLT4") == 0) ||
		   (strcasecmp(pre, "MOD_FLT8") == 0) ||
		   (strcasecmp(pre, "MOD_ADSC4") == 0) ||
		   (strcasecmp(pre, "MOD_ADSC8") == 0) ||
		   (strcasecmp(pre, "MOD_COMP") == 0) ||
		   (strcasecmp(pre, "MOD_NTKAMP") == 0) ||
		   (strcasecmp(pre, "PPK") == 0) ||
		   (strcasecmp(pre, "MOD_PC") == 0) ||
		   (strcasecmp(pre, "ICE") == 0) ||
		   (strcasecmp(pre, "ADSC") == 0)) {
		 /*MOD*/
		    process_ptk_mod(credits, credits_len, 31, buf, modfilelen);
	} else if (strcasecmp(pre, "DL") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "UNCL",
				 "EART", 0x28);
	} else if (strcasecmp(pre, "BSS") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "BEAT",
				 "HOVE", 0x1c);
	} else if (strcasecmp(pre, "GRAY") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "FRED",
				 "GRAY", 0x10);
	} else if (strcasecmp(pre, "JMF") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "J.FL",
				 "OGEL", 0x14);
	} else if (strcasecmp(pre, "SPL") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "!SOP",
				 "ROL!", 0x10);
	} else if (strcasecmp(pre, "HD") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "H.DA",
				 "VIES", 24);
	} else if (strcasecmp(pre, "RIFF") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "RIFF",
				 "RAFF", 0x14);
	} else if (strcasecmp(pre, "FP") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "F.PL",
				 "AYER", 0x8);
	} else if (strcasecmp(pre, "CORE") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "S.PH",
				 "IPPS", 0x20);
	} else if (strcasecmp(pre, "BDS") == 0) {
		process_WTWT_mod(credits, credits_len, buf, modfilelen, "DAGL",
				 "ISH!", 0x14);
	}

	free(buf);

	return 0;
}

int uade_generate_song_title(char *title, size_t dstlen,
			     struct uade_state *state)
{
	size_t srcoffs;
	size_t dstoffs;
	size_t srclen;
	char *format;
	char *bname;
	char p[64];
	char *default_format = "%F %X [%P]";
	struct uade_song *us = state->song;
	struct uade_config *uc = &state->config;

	/* %A min subsong
	   %B cur subsong
	   %C max subsong
	   %F file base name (us->module_filename)
	   %P player name
	   %T title
	   %X print subsong info if more than one subsong exist
	 */

	format = uc->song_title;

	if (format == NULL)
		format = default_format;

	if (strcmp("default", format) == 0)
		format = default_format;

	if ((srclen = strlen(format)) == 0) {
		fprintf(stderr, "Warning: empty song_title format string.\n");
		return 1;
	}

	if (dstlen == 0)
		return 1;

	if (strlen(us->module_filename) == 0)
		return 1;

	bname = xbasename(us->module_filename);

	p[0] = 0;
	if (us->formatname[0] == 0) {
		if (us->playername[0] == 0) {
			strlcpy(p, "Custom", sizeof p);
		} else {
			strlcpy(p, us->playername, sizeof p);
		}
	} else {
		if (strncmp(us->formatname, "type: ", 6) == 0) {
			strlcpy(p, us->formatname + 6, sizeof p);
		} else {
			strlcpy(p, us->formatname, sizeof p);
		}
	}

	srcoffs = dstoffs = 0;

	title[0] = 0;

	while (dstoffs < dstlen) {
		char c;
		if (srcoffs >= srclen)
			break;

		if ((c = format[srcoffs]) == 0)
			break;

		if (c != '%') {
			title[dstoffs++] = format[srcoffs++];
		} else {
			size_t inc;
			char *dat = NULL;
			char tmp[32];

			if ((srcoffs + 1) >= srclen) {
				fprintf(stderr, "Error: no identifier given in song title format: %s\n", format);
				title[dstoffs] = 0;
				return 1;
			}

			c = format[srcoffs + 1];

			switch (c) {
			case 'A':
				snprintf(tmp, sizeof tmp, "%d",
					 us->min_subsong);
				dat = tmp;
				break;
			case 'B':
				snprintf(tmp, sizeof tmp, "%d",
					 us->cur_subsong);
				dat = tmp;
				break;
			case 'C':
				snprintf(tmp, sizeof tmp, "%d",
					 us->max_subsong);
				dat = tmp;
				break;
			case 'F':
				dat = bname;
				break;
			case 'P':
				dat = p;
				break;
			case 'T':
				dat = us->modulename;
				if (strcmp("<no songtitle>", dat) == 0)
					dat[0] = 0;
				if (dat[0] == 0)
					dat = bname;
				break;
			case 'X':
				if (us->min_subsong == us->max_subsong) {
					tmp[0] = 0;
				} else {
					snprintf(tmp, sizeof tmp, "(%d/%d)",
						 us->cur_subsong,
						 us->max_subsong);
				}
				dat = tmp;
				break;
			default:
				fprintf(stderr,
					"Unknown identifier %%%c in song_title format: %s\n",
					c, format);
				title[dstoffs] = 0;
				return 1;
			}
			inc = strlcpy(&title[dstoffs], dat, dstlen - dstoffs);
			srcoffs += 2;
			dstoffs += inc;
		}
	}

	if (dstoffs < dstlen)
		title[dstoffs] = 0;
	else
		title[dstlen - 1] = 0;

	return 0;
}

/* Returns zero on success, non-zero otherwise. */
int uade_song_info(char *info, size_t maxlen, char *filename,
		   enum song_info_type type)
{
	switch (type) {
	case UADE_MODULE_INFO:
		return process_module(info, maxlen, filename);
	case UADE_HEX_DUMP_INFO:
		return hexdump(info, maxlen, filename, 2048);
	default:
		fprintf(stderr, "Illegal info requested.\n");
		exit(-1);
	}
	return 0;
}
