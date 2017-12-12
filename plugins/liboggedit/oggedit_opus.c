/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library Opus functions

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "oggedit_internal.h"

#define TAGMAGIC "OpusTags"

int oggedit_write_opus_file(DB_FILE *in, const char *outname, const off_t offset, const bool all_streams)
{
    FILE *out = open_new_file(outname);
    if (!out)
        return OGGEDIT_CANNOT_OPEN_OUTPUT_FILE;

    ogg_sync_state oy;
    ogg_sync_init(&oy);

    int res;
    if (all_streams)
        res = write_one_stream(in, out, &oy, offset, OPUSNAME);
    else
        res = write_all_streams(in, out, &oy, offset);

    cleanup(in, out, &oy, NULL);

    if (res <= OGGEDIT_EOF)
        unlink(outname);

    return res;
}

off_t oggedit_opus_stream_info(DB_FILE *in, const off_t start_offset, const off_t end_offset, char **codecs)
{
    ogg_sync_state oy;
    ogg_sync_init(&oy);
    *codecs = codec_names(in, &oy, start_offset);
    const off_t stream_size = codec_stream_size(in, &oy, start_offset, end_offset, OPUSNAME);
    cleanup(in, NULL, &oy, NULL);
    return stream_size;
}

static long check_opus_header(DB_FILE *in, ogg_sync_state *oy, const off_t offset, char **vendor)
{
    ogg_stream_state os;
    ogg_page og;
    const int64_t serial = init_read_stream(in, oy, &os, &og, offset, OPUSNAME);
    if (serial <= OGGEDIT_EOF)
        return serial;

    ogg_packet op;
    const long pages = read_packet(in, oy, &os, &og, &op, 1);
    ogg_stream_clear(&os);
    if (pages <= OGGEDIT_EOF)
        return pages;

    if (op.bytes > strlen(TAGMAGIC) && !memcmp(op.packet, TAGMAGIC, strlen(TAGMAGIC)))
        *vendor = parse_vendor(&op, strlen(TAGMAGIC));
    free(op.packet);
    if (!*vendor)
        return OGGEDIT_CANNOT_PARSE_HEADERS;

#ifdef HAVE_OGG_STREAM_FLUSH_FILL
    if (op.bytes < MAXPAYLOAD * (pages-1))
        return 4; // prevent in-place write if the packet is weirdly split into too many pages
#else
    return 4; // not safe to pad without ogg_stream_flush_fill
#endif

    return op.bytes;
}

static int64_t write_opus_tags(FILE *out, const int64_t serial, const char *vendor, const uint32_t num_tags, char **tags, const size_t padding)
{
    ogg_packet op;
    if (!fill_vc_packet(TAGMAGIC, strlen(TAGMAGIC), vendor, num_tags, tags, false, padding, &op))
        return OGGEDIT_ALLOCATION_FAILURE;

    ogg_stream_state os;
    if (ogg_stream_init(&os, serial))
        return OGGEDIT_FAILED_TO_INIT_STREAM;
    os.b_o_s = 1;
    os.pageno = 1;
    ogg_stream_packetin(&os, &op);

    ogg_packet_clear(&op);
    return flush_stream(out, &os);
}

off_t oggedit_write_opus_metadata(DB_FILE *in, const char *fname, const off_t offset, const off_t stream_size, const int output_gain, const uint32_t num_tags, char **tags)
{
    off_t res;
    char tempname[PATH_MAX] = "";
    char *vendor = NULL;
    ogg_sync_state oy;
    ogg_sync_init(&oy);

    /* Original file must be writable whichever way we update it */
    FILE *out = fopen(fname, "r+b");
    if (!out) {
        res = OGGEDIT_CANNOT_UPDATE_FILE;
        goto cleanup;
    }

    /* Should we write the tags packet directly into the existing file ... */
    const long tags_packet_size = check_opus_header(in, &oy, offset, &vendor);
    if (tags_packet_size <= OGGEDIT_EOF) {
        res = tags_packet_size;
        goto cleanup;
    }
    const size_t metadata_size = strlen(TAGMAGIC) + vc_size(vendor, num_tags, tags);
    size_t padding = tags_packet_size - metadata_size;
    const off_t file_size_k = in->vfs->getlength(in) / 1000;
    const off_t stream_size_k = stream_size ? stream_size / 1000 : file_size_k;
    if (file_size_k < 100 || padding < 0 || padding > file_size_k/10+stream_size_k+metadata_size) {
        res = open_temp_file(fname, tempname, &out);
        if (res) {
            goto cleanup;
        }
    }

    /* Re-pad if writing the whole file */
    if (*tempname)
        padding = stream_size_k < 90 ? 0 : stream_size_k < 1000 ? 128 : stream_size_k < 10000 ? 1024 : 8192;

    /* Write pages until we reach the correct OpusHead, then write OpusTags */
    ogg_page og;
    int64_t opus_serial = copy_up_to_codec(in, out, &oy, &og, *tempname ? 0 : offset, offset, OPUSNAME);
    if (opus_serial <= OGGEDIT_EOF) {
        res = opus_serial;
        goto cleanup;
    }
    if (output_gain > INT_MIN) {
        og.body[16] = output_gain & 0xFF;
        og.body[17] = output_gain >> 8 & 0xFF;
        ogg_page_checksum_set(&og);
    }
    opus_serial = copy_up_to_header(in, out, &oy, &og, opus_serial);
    if (opus_serial <= OGGEDIT_EOF) {
        res = opus_serial;
        goto cleanup;
    }
    const int64_t pageno = write_opus_tags(out, opus_serial, vendor, num_tags, tags, padding);
    if (pageno < OGGEDIT_EOF) {
        res = pageno;
        goto cleanup;
    }

    /* If we have tempfile, copy the remaining pages */
    if (*tempname) {
        opus_serial = copy_remaining_pages(in, out, &oy, opus_serial, pageno);
        if (opus_serial <= OGGEDIT_EOF) {
            res = opus_serial;
            goto cleanup;
        }
        if (rename(tempname, fname)) {
            res = OGGEDIT_RENAME_FAILED;
            goto cleanup;
        }
    }

    res = file_size(fname);

cleanup:
    cleanup(in, out, &oy, vendor);
    if (res < OGGEDIT_OK)
        unlink(tempname);
    return res;
}
/*
struct timeval timeval;
gettimeofday(&timeval, NULL);
int usecs = timeval.tv_sec*1000000 + timeval.tv_usec;
gettimeofday(&timeval, NULL);
usecs = timeval.tv_sec*1000000 + timeval.tv_usec - usecs;
fprintf(stderr, "%d micro-seconds\n", usecs);
*/
