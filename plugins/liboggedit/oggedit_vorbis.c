/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library Vorbis functions

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

#define VCMAGIC "\3vorbis"
#define CODEMAGIC "\5vorbis"

off_t oggedit_vorbis_stream_info(DB_FILE *in, const off_t start_offset, const off_t end_offset, char **codecs)
{
    ogg_sync_state oy;
    ogg_sync_init(&oy);
    *codecs = codec_names(in, &oy, start_offset);
    const off_t stream_size = codec_stream_size(in, &oy, start_offset, end_offset, VORBISNAME);
    cleanup(in, NULL, &oy, NULL);
    return stream_size;
}

static ptrdiff_t check_vorbis_headers(DB_FILE *in, ogg_sync_state *oy, const off_t offset, char **vendor, ogg_packet *codebooks)
{
    ogg_stream_state os;
    ogg_page og;
    const int64_t serial = init_read_stream(in, oy, &os, &og, offset, VORBISNAME);
    if (serial <= OGGEDIT_EOF)
        return serial;

    ogg_packet vc;
    int64_t pages = read_packet(in, oy, &os, &og, &vc, 1);
    if (pages > OGGEDIT_EOF)
        pages = read_packet(in, oy, &os, &og, codebooks, pages);
    ogg_stream_clear(&os);
    if (pages <= OGGEDIT_EOF)
        return pages;

    if (vc.bytes > strlen(VCMAGIC) && !memcmp(vc.packet, VCMAGIC, strlen(VCMAGIC)) &&
        codebooks->bytes > strlen(CODEMAGIC) && !memcmp(codebooks->packet, CODEMAGIC, strlen(CODEMAGIC)))
        *vendor = parse_vendor(&vc, strlen(VCMAGIC));
    free(vc.packet);
    if (!*vendor)
        return OGGEDIT_CANNOT_PARSE_HEADERS;

#ifdef HAVE_OGG_STREAM_FLUSH_FILL
    if ((vc.bytes + codebooks->bytes) < MAXPAYLOAD * (pages-1))
        return 4; // prevent in-place write if the packets are split over too many pages
#else
    return 4; // not safe to pad without ogg_stream_flush_fill
#endif

    return vc.bytes;
}

static long write_vorbis_tags(FILE *out, const int64_t serial, const char *vendor, const size_t num_tags, char **tags, const size_t padding, ogg_packet *codebooks)
{
    ogg_packet op;
    if (!fill_vc_packet(VCMAGIC, strlen(VCMAGIC), vendor, num_tags, tags, true, padding, &op))
        return OGGEDIT_ALLOCATION_FAILURE;

    ogg_stream_state os;
    if (ogg_stream_init(&os, (uint32_t)serial))
        return OGGEDIT_FAILED_TO_INIT_STREAM;
    os.b_o_s = 1;
    os.pageno = 1;
    ogg_stream_packetin(&os, &op);
    ogg_stream_packetin(&os, codebooks);

    ogg_packet_clear(&op);
    return flush_stream(out, &os);
}

off_t oggedit_write_vorbis_metadata(DB_FILE *in, const char *fname, const off_t offset, const size_t stream_size, const int num_tags, char **tags)
{
    off_t res;
    char tempname[PATH_MAX] = "";
    char *vendor = NULL;
    ogg_packet codebooks;
    memset(&codebooks, '\0', sizeof(codebooks));
    ogg_sync_state oy;
    ogg_sync_init(&oy);

    /* Original file must be writable whichever way we update it */
    FILE *out = fopen(fname, "r+b");
    if (!out) {
        res = OGGEDIT_CANNOT_UPDATE_FILE;
        goto cleanup;
    }

    /* See if we can write the tags packet directly into the existing file ... */
    const ptrdiff_t tags_packet_size = check_vorbis_headers(in, &oy, offset, &vendor, &codebooks);
    if (tags_packet_size <= OGGEDIT_EOF) {
        res = tags_packet_size;
        goto cleanup;
    }
    const size_t metadata_size = strlen(VCMAGIC) + vc_size(vendor, num_tags, tags) + 1;
    ptrdiff_t padding = tags_packet_size - metadata_size;
    const off_t file_size_k = in->vfs->getlength(in) / 1000;
    const size_t stream_size_k = stream_size ? stream_size / 1000 : file_size_k;
    if (file_size_k < 100 || padding < 0 || padding > file_size_k/10+stream_size_k+metadata_size) {
        res = open_temp_file(fname, tempname, &out);
        if (res) {
            goto cleanup;
        }
    }

    /* Re-pad if writing the whole file */
    if (*tempname)
        padding = stream_size_k < 90 ? 0 : stream_size_k < 1000 ? 128 : stream_size_k < 10000 ? 1024 : 8192;

    /* Write pages until the correct comment header */
    ogg_page og;
    int64_t vorbis_serial = copy_up_to_codec(in, out, &oy, &og, *tempname ? 0 : offset, offset, VORBISNAME);
    if (vorbis_serial <= OGGEDIT_EOF) {
        res = vorbis_serial;
        goto cleanup;
    }
    vorbis_serial = copy_up_to_header(in, out, &oy, &og, vorbis_serial);
    if (vorbis_serial <= OGGEDIT_EOF) {
        res = vorbis_serial;
        goto cleanup;
    }
    const long pageno = write_vorbis_tags(out, vorbis_serial, vendor, num_tags, tags, padding, &codebooks);
    if (pageno < OGGEDIT_EOF) {
        res = pageno;
        goto cleanup;
    }

    /* If we have tempfile, copy the remaining pages */
    if (*tempname) {
        vorbis_serial = copy_remaining_pages(in, out, &oy, vorbis_serial, (uint32_t)pageno);
        if (vorbis_serial <= OGGEDIT_EOF) {
            res = vorbis_serial;
            goto cleanup;
        }

        fclose (out);
        out = NULL;
        if (rename(tempname, fname)) {
            res = OGGEDIT_RENAME_FAILED;
            goto cleanup;
        }
    }

    res = file_size(fname);

cleanup:
    ogg_packet_clear(&codebooks);
    cleanup(in, out, &oy, vendor);
    if (res < OGGEDIT_OK)
        unlink(tempname);
    return res;
}
