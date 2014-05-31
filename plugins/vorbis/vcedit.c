/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  Ogg Vorbis plugin Ogg edit functions

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <ogg/ogg.h>
#include <deadbeef.h>
#include "vcedit.h"

#define VORBISNAME "Vorbis"
#define TAGMAGIC "\3vorbis"
#define CODEMAGIC "\5vorbis"

#define CHUNKSIZE 4096
#define MAXOGGPAGE 65536

static const char *ogg_codec(ogg_page *og)
{
    typedef struct {
        const char *magic;
        const size_t length;
        const char *codec;
    } codec_t;
    const codec_t codecs[] = {
        {.codec = "Opus",     .length = 19, .magic = "OpusHead"},
        {.codec = "Vorbis",   .length = 30, .magic = "\1vorbis"},
        {.codec = "Flac",     .length = 47, .magic = "\177FLAC"},
        {.codec = "Speex",    .length = 80, .magic = "Speex   "},
        {.codec = "Celt",     .length = 80, .magic = "CELT"}, // obsolete
        {.codec = "MIDI",     .length = 13, .magic = "OggMIDI\0"},
        {.codec = "PCM",      .length = 28, .magic = "PCM     "},
        {.codec = "Theora",   .length = 42, .magic = "\200theora"},
        {.codec = "Daala",    .length = 38, .magic = "\200daala"},
        {.codec = "Dirac",    .length = 5,  .magic = "BBCD\0"},
        {.codec = "Skeleton", .length = 80, .magic = "fishead\0"},
        {.codec = "Kate",     .length = 64, .magic = "\200kate\0\0\0"},
        {.codec = "CMML",     .length = 29, .magic = "CMML\0\0\0\0"},
        {.codec = "YUV4MPEG", .length = 8,  .magic = "YUV4Mpeg"},
        {.codec = "UVS",      .length = 48, .magic = "UVS     "},
        {.codec = "YUV",      .length = 32, .magic = "\0YUV"},
        {.codec = "RGB",      .length = 24, .magic = "\0RGB"},
        {.codec = "JNG",      .length = 48, .magic = "\213JNG\r\n\032\n"},
        {.codec = "MNG",      .length = 48, .magic = "\212MNG\r\n\032\n"},
        {.codec = "PNG",      .length = 48, .magic = "\211PNG\r\n\032\n"},
        {.codec = "Spots",    .length = 52, .magic = "SPOTS\0\0\0"},
        {.codec = NULL}
    };

    for (const codec_t *match = codecs; match->codec; match++)
        if ((size_t)og->body_len >= match->length && !memcmp(og->body, match->magic, strlen(match->codec)))
            return match->codec;

    return "unknown";
}

static void _oggpack_chars(oggpack_buffer *opb, const char *s, size_t length)
{
    while (length--)
        oggpack_write(opb, *s++, 8);
}

static void _oggpack_string(oggpack_buffer *opb, const char *s)
{
    oggpack_write(opb, strlen(s), 32);
    _oggpack_chars(opb, s, strlen(s));
}

static int get_page(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, char **lasterror)
{
    uint16_t chunks_left = MAXOGGPAGE / CHUNKSIZE;
    while (ogg_sync_pageout(oy, og) != 1) {
        char *buffer = ogg_sync_buffer(oy, CHUNKSIZE);
        if (!in || !buffer || !chunks_left--) {
            *lasterror = "can't find Ogg bitstream.";
            return -1;
        }

        const size_t bytes = in->vfs->read(buffer, 1, CHUNKSIZE, in);
        if (!bytes) {
            *lasterror = "unexpected EOF.";
            return 0;
        }

        ogg_sync_wrote(oy, bytes);
    }

    return ogg_page_serialno(og);
}

static bool write_page(FILE *out, ogg_page *og)
{
    return fwrite(og->header, 1, og->header_len, out) != (size_t)og->header_len ||
           fwrite(og->body, 1, og->body_len, out) != (size_t)og->body_len;
}

static int write_page_and_get_next(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og, char **lasterror)
{
    if (write_page(out, og))
        return -1;

    return get_page(in, oy, og, lasterror);
}

static bool write_vorbis_tags(FILE *out, const int serial, const char *vendor, const size_t num_tags, char **tags, ogg_packet *codebooks)
{
    oggpack_buffer opb;
    oggpack_writeinit(&opb);
    _oggpack_chars(&opb, TAGMAGIC, strlen(TAGMAGIC));
    _oggpack_string(&opb, vendor);
    oggpack_write(&opb, num_tags, 32);
    for (size_t i = 0; i < num_tags; i++)
        _oggpack_string(&opb, tags[i]);
    oggpack_write(&opb, 1, 1);
    oggpack_writealign(&opb);


    ogg_stream_state os;
    ogg_stream_init(&os, serial);
    os.b_o_s = 1;
    os.pageno = 1;
    ogg_packet op;
    memset(&op, '\0', sizeof(op));
    op.packet = oggpack_get_buffer(&opb);
    op.bytes = oggpack_bytes(&opb);
    ogg_stream_packetin(&os, &op);
    oggpack_writeclear(&opb);
    ogg_stream_packetin(&os, codebooks);

    ogg_page og;
    while (ogg_stream_flush(&os, &og))
        if (write_page(out, &og))
            return true;

    return ogg_stream_check(&os) || ogg_stream_clear(&os);
}

static int extract_codebooks(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, int serial, ogg_packet *codebooks, char **lasterror)
{
    memset(codebooks, '\0', sizeof(codebooks));
    ogg_stream_state os;
    if (ogg_stream_init(&os, serial)) {
        *lasterror = "cannot init Ogg stream.";
        return -1;
    }
    os.pageno = 1;
    os.b_o_s = 1;

    ogg_packet op;
    ogg_stream_pagein(&os, og);
    do {
        while (ogg_stream_packetpeek(&os, NULL) == 0) {
            if ((serial = get_page(in, oy, og, lasterror)) <= 0)
                goto cleanup;
            fprintf(stderr, "Serial: %d, %ld bytes (%s)\n", serial, og->header_len + og->body_len, og->body);
            if (ogg_stream_pagein(&os, og)) {
                *lasterror = "failed to stream page for codebooks packet.";
                goto cleanup;
            }
        }
    } while (ogg_stream_packetout(&os, &op) != 1 || op.bytes < strlen(CODEMAGIC) || memcmp(op.packet, CODEMAGIC, strlen(CODEMAGIC)));

    if (!(codebooks->packet = malloc(op.bytes))) {
        *lasterror = "cannot allocate codebooks packet.";
        goto cleanup;
    }
    codebooks->bytes = op.bytes;
    memcpy(codebooks->packet, op.packet, op.bytes);

cleanup:
    ogg_stream_clear(&os);

    return codebooks->packet ? serial : -1;
}

off_t vcedit_write_metadata(DB_FILE *in, const char *fname, int link, const char *vendor, const int num_tags, char **tags, char **lasterror)
{
    off_t file_size = 0;
    ogg_page og;
    ogg_sync_state oy;
    ogg_sync_init(&oy);

    char outname[PATH_MAX];
    snprintf(outname, PATH_MAX, "%s.temp", fname);
    FILE *out = fopen(outname, "w+b");
    if (!out) {
        *lasterror = "unable to open temporary file for writing.";
        goto cleanup;
    }
    if (!in) {
        *lasterror = "file not opened for reading.";
        goto cleanup;
    }
    struct stat stat_struct;
    if (!stat(fname, &stat_struct))
	chmod(outname, stat_struct.st_mode);

    /* Copy through pages until we reach the right info header */
    int codec_serial = 0;
    int serial = get_page(in, &oy, &og, lasterror);
    while (serial > 0 && !codec_serial) {
        while (serial > 0 && !ogg_page_bos(&og))
            serial = write_page_and_get_next(in, out, &oy,  &og, lasterror);

        while (serial > 0 && ogg_page_bos(&og)) {
            if (link < 1 && !strcmp(ogg_codec(&og), VORBISNAME)) {
                codec_serial = serial;
            }
            serial = write_page_and_get_next(in, out, &oy,  &og, lasterror);
        }
        link--;
    }

    /* Copy additional pages up to our comment header */
    while (serial > 0 && serial != codec_serial)
        serial = write_page_and_get_next(in, out, &oy,  &og, lasterror);
    if (serial <= 0)
        goto cleanup;

    /* Add the codebook packet to our comment header and save it */
    ogg_packet codebooks;
    if ((serial = extract_codebooks(in, &oy, &og, codec_serial, &codebooks, lasterror)) <= 0)
        goto cleanup;
    const bool write_tags = write_vorbis_tags(out, codec_serial, vendor, num_tags, tags, &codebooks);
    ogg_packet_clear(&codebooks);
    if (write_tags) {
        *lasterror = "internal error writing Vorbis comment header packet.";
        goto cleanup;
    }

    /* Blindly copy through the remaining pages */
    serial = get_page(in, &oy, &og, lasterror);
    while (serial > 0)
        serial = write_page_and_get_next(in, out, &oy,  &og, lasterror);
    if (serial < 0)
        goto cleanup;

    fseeko(out, 0, SEEK_END);
    file_size = ftello(out);

cleanup:
    if (in)
        in->vfs->close(in);

    if (out)
        fclose(out);

    ogg_sync_clear(&oy);

    if (file_size <= 0) {
        unlink(outname);
        if (!*lasterror)
            *lasterror = "error writing new file, changes backed out.";
        return -1;
    }

    rename(outname, fname);
    return file_size;
}
