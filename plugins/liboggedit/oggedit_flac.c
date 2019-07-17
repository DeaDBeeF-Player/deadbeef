/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library Flac functions

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

#define PADTYPE 0x01
#define VCTYPE 0x04
#define LASTBLOCK 0x80

off_t oggedit_flac_stream_info(DB_FILE *in, const off_t start_offset, const off_t end_offset)
{
    ogg_sync_state oy;
    ogg_sync_init(&oy);
    const off_t stream_size = codec_stream_size(in, &oy, start_offset, end_offset, FLACNAME);
    cleanup(in, NULL, &oy, NULL);
    return stream_size;
}

static void clear_header_list(ogg_packet **headers)
{
    if (headers) {
        for (ogg_packet **header = headers; *header; header++) {
            ogg_packet_clear(*header);
            free(*header);
        }
        free(headers);
    }
}

static ogg_packet **headers_alloc(ogg_packet **headers, const size_t packets)
{
    ogg_packet **new_headers = realloc(headers, (packets+2) * sizeof(ogg_packet *));
    if (!new_headers) {
        clear_header_list(headers);
        return NULL;
    }

    if (!(new_headers[packets] = malloc(sizeof(ogg_packet)))) {
        clear_header_list(new_headers);
        return NULL;
    }

    new_headers[packets + 1] = NULL;
    return new_headers;
}

static ogg_packet **metadata_block_packets(DB_FILE *in, ogg_sync_state *oy, const off_t offset, char **vendor, int *res)
{
    ogg_stream_state os;
    ogg_page og;
    if ((*res = init_read_stream(in, oy, &os, &og, offset, FLACNAME)) <= OGGEDIT_EOF)
        return NULL;

    int pages = 1;
    size_t packets = 0;
    ogg_packet **headers = NULL;
    while ((headers = headers_alloc(headers, packets)) &&
           (pages = read_packet(in, oy, &os, &og, headers[packets], pages)) > OGGEDIT_EOF &&
           (headers[packets++]->packet[0] & LASTBLOCK) != LASTBLOCK);

    ogg_stream_clear(&os);

    if (!headers)
        *res = OGGEDIT_ALLOCATION_FAILURE;
    else if (!packets || (headers[0]->packet[0] & 0x3F) != VCTYPE)
        *res = OGGEDIT_CANNOT_PARSE_HEADERS;
    else
        *res = pages;
    if (*res <= OGGEDIT_EOF) {
        clear_header_list(headers);
        return NULL;
    }

    *vendor = parse_vendor(headers[0], 4);
#ifdef HAVE_OGG_STREAM_FLUSH_FILL
    size_t bytes = 0;
    for (size_t i = 0; i < packets; i++)
        bytes += headers[i]->bytes;
    if (bytes < MAXPAYLOAD * (pages-1))
        headers[0]->bytes = 4;
#else
    headers[0]->bytes = 4; // not safe to pad without ogg_stream_flush_fill
#endif

    *res = OGGEDIT_OK;
    return headers;
}

static long write_metadata_block_packets(FILE *out, const int64_t serial, const char *vendor, const size_t num_tags, char **tags, size_t padding, ogg_packet **metadata)
{
    const size_t header_length = vc_size(vendor, num_tags, tags);
    if (header_length > (2<<24))
        return OGGEDIT_ALLOCATION_FAILURE;

    char magic[4];
    magic[0] = VCTYPE;
    magic[1] = header_length >> 16 & 0xFF;
    magic[2] = header_length >> 8 & 0xFF;
    magic[3] = header_length & 0xFF;
    ogg_packet_clear(metadata[0]);
    if (!fill_vc_packet(magic, 4, vendor, num_tags, tags, false, padding, metadata[0]))
        return OGGEDIT_ALLOCATION_FAILURE;

    ogg_stream_state os;
    if (ogg_stream_init(&os, serial))
        return OGGEDIT_FAILED_TO_INIT_STREAM;
    os.b_o_s = 1;
    os.pageno = 1;

    for (int i = 0; metadata[i]; i++) {
        if (!metadata[i+1])
            metadata[i]->packet[0] |= LASTBLOCK;
        ogg_stream_packetin(&os, metadata[i]);
    }

    return flush_stream(out, &os);
}

off_t oggedit_write_flac_metadata(DB_FILE *in, const char *fname, const off_t offset, const int num_tags, char **tags)
{
    off_t res;
    char tempname[PATH_MAX] = "";
    ogg_packet **headers = NULL;
    char *vendor = NULL;
    ogg_sync_state oy;
    ogg_sync_init(&oy);

    /* Original file must be writable whichever way we update it */
    FILE *out = fopen(fname, "r+b");
    if (!out) {
        res = OGGEDIT_CANNOT_UPDATE_FILE;
        goto cleanup;
    }

    /* See if we can write the tags packet directly into the existing file ... */
    if (!(headers = metadata_block_packets(in, &oy, offset, &vendor, (int *)&res)))
        goto cleanup;
    const off_t stream_size_k = in->vfs->getlength(in) / 1000; // use file size for now
    const size_t metadata_size = 4 + vc_size(vendor, num_tags, tags);
    ptrdiff_t padding = headers[0]->bytes - metadata_size;
    if (stream_size_k < 1000 || padding < 0 || (headers[1] && padding > 0) || padding > stream_size_k+metadata_size) {
        res = open_temp_file(fname, tempname, &out);
        if (res) {
            goto cleanup;
        }
    }

    /* Re-pad if writing the whole file */
    if (*tempname) {
        size_t i = 0, j = 0;
        while (headers[i]) {
            headers[j++] = headers[i];
            while (headers[++i] && (headers[i]->packet[0] & 0x3F) == PADTYPE) {
                ogg_packet_clear(headers[i]);
                free(headers[i]);
            }
        }
        headers[j] = NULL;
        padding = headers[1] || stream_size_k < 900 ? 0 : stream_size_k < 10000 ? 1024 : stream_size_k < 100000 ? 8192 : 65536;
    }

    /* Write pages until we reach the correct comment header */
    ogg_page og;
    int64_t flac_serial = copy_up_to_codec(in, out, &oy, &og, *tempname ? 0 : offset, offset, FLACNAME);
    if (flac_serial <= OGGEDIT_EOF) {
        res = flac_serial;
        goto cleanup;
    }
    flac_serial = copy_up_to_header(in, out, &oy, &og, flac_serial);
    if (flac_serial <= OGGEDIT_EOF) {
        res = flac_serial;
        goto cleanup;
    }
    const long pageno = write_metadata_block_packets(out, flac_serial, vendor, num_tags, tags, padding, headers);
    if (pageno < OGGEDIT_EOF) {
        res = pageno;
        goto cleanup;
    }

    /* If we have tempfile, copy the remaining pages */
    if (*tempname) {
        flac_serial = copy_remaining_pages(in, out, &oy, flac_serial, pageno);
        if (flac_serial <= OGGEDIT_EOF) {
            res = flac_serial;
            goto cleanup;
        }
        if (rename(tempname, fname)) {
            res = OGGEDIT_RENAME_FAILED;
            goto cleanup;
        }
    }

    res = file_size(fname);

cleanup:
    clear_header_list(headers);
    cleanup(in, out, &oy, vendor);
    if (res < OGGEDIT_OK)
        unlink(tempname);
    return res;
}

