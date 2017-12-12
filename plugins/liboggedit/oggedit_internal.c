/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library internal functions

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
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include "oggedit_internal.h"

static char *cat_string(char *dest, const char *src, const char *sep)
{
    char *more = realloc(dest, strlen(dest) + strlen(src) + strlen(sep) + 1);
    if (!more) {
        free(dest);
        return NULL;
    }
    return strcat(strcat(more, sep), src);
}

static int64_t int32_to_unsigned(int32_t value)
{
    /* Represent a 32-bit numeric field as a positive signed integer, maintaining order */
    return value < 0 ? value + UINT32_MAX + 1 : value;
}

static const char *codec_name(ogg_page *og)
{
    typedef struct {
        const unsigned length;
        const char *codec;
        const char *magic;
    } codec_t;
    const codec_t codecs[] = {
        {.codec = OPUSNAME,   .length = 19, .magic = "OpusHead"},
        {.codec = VORBISNAME, .length = 30, .magic = "\1vorbis"},
        {.codec = FLACNAME,   .length = 47, .magic = "\177FLAC"},
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
        if (og->body_len >= match->length && !memcmp(og->body, match->magic, strlen(match->codec)))
            return match->codec;

    return "unknown";
}

void _oggpack_chars(oggpack_buffer *opb, const char *s, size_t length)
{
//    oggpack_writecopy(opb, s, length * 8); currently broken
    while (length--)
        oggpack_write(opb, *s++, 8);
}

void _oggpack_string(oggpack_buffer *opb, const char *s)
{
    oggpack_write(opb, strlen(s), 32);
    _oggpack_chars(opb, s, strlen(s));
}

void _oggpackB_string(oggpack_buffer *opb, const char *s)
{
    oggpackB_write(opb, strlen(s), 32);
    _oggpack_chars(opb, s, strlen(s));
}

static bool ensure_directory(const char *path)
{
    struct stat stat_struct;
    if (!stat(path, &stat_struct))
        return S_ISDIR(stat_struct.st_mode);

    if (errno != ENOENT)
        return false;

    char* dir = strdup(path);
    if (!dir)
        return false;

    const int is_dir = ensure_directory(dirname(dir));
    free(dir);

    return is_dir && !mkdir(path, 0755);
}

FILE *open_new_file(const char *outname)
{
    char outpath[PATH_MAX];
    strcpy(outpath, outname);
    if (!ensure_directory(dirname(outpath)))
        return NULL;

    unlink(outname);
    return fopen(outname, "wbx");
}

int open_temp_file(const char *fname, char *tempname, FILE **out)
{
    snprintf(tempname, PATH_MAX, "%s.temp", fname);
    unlink(tempname);
    if (!(*out = freopen(tempname, "abx", *out)))
        return OGGEDIT_CANNOT_OPEN_TEMPORARY_FILE;

    struct stat stat_struct;
    if (!stat(fname, &stat_struct)) {
        chmod(tempname, stat_struct.st_mode);
    }

    return 0;
}

off_t file_size(const char *fname)
{
    struct stat sb;
    if (stat(fname, &sb))
        return OGGEDIT_STAT_FAILED;

    return sb.st_size;
}

void cleanup(DB_FILE *in, FILE *out, ogg_sync_state *oy, void *buffer)
{
    if (in)
        in->vfs->close(in);

    if (out)
        fclose(out);

    ogg_sync_clear(oy);

    if (buffer)
        free(buffer);
}

static bool is_data_page(ogg_page *og, int64_t codec_serial, int64_t serial)
{
    return ogg_page_granulepos(og) != 0 && serial == codec_serial;
}

static int64_t get_page(DB_FILE *in, ogg_sync_state *oy, ogg_page *og)
{
    uint16_t chunks_left = MAXPAGE / CHUNKSIZE;
    while (ogg_sync_pageout(oy, og) != 1) {
        char *buffer = ogg_sync_buffer(oy, CHUNKSIZE);
        if (!in || !buffer || !chunks_left--)
            return OGGEDIT_CANT_FIND_STREAM;

        const size_t bytes = in->vfs->read(buffer, 1, CHUNKSIZE, in);
        if (!bytes)
            return OGGEDIT_EOF;

        ogg_sync_wrote(oy, bytes);
    }

    return int32_to_unsigned(ogg_page_serialno(og));
}

static int64_t skip_to_bos(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, const off_t offset)
{
    if (!in)
        return OGGEDIT_FILE_NOT_OPEN;

    if (in->vfs->seek(in, offset, SEEK_SET))
        return OGGEDIT_SEEK_FAILED;

    ogg_sync_reset(oy);
    int64_t serial;
    do
        serial = get_page(in, oy, og);
    while (serial > OGGEDIT_EOF && !ogg_page_bos(og));

    return serial;
}

static int64_t skip_to_codec(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, const off_t offset, const char *codec)
{
    int64_t serial = skip_to_bos(in, oy, og, offset);
    while (serial > OGGEDIT_EOF && strcmp(codec_name(og), codec))
        serial = get_page(in, oy, og);

    return serial;
}

static int64_t skip_to_header(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, int64_t serial, const int64_t codec_serial)
{
    while (serial > OGGEDIT_EOF && (ogg_page_bos(og) || serial != codec_serial))
        serial = get_page(in, oy, og);

    return serial;
}

static off_t sync_tell(DB_FILE *in, ogg_sync_state *oy, ogg_page *og)
{
    return in->vfs->tell(in) - oy->fill + oy->returned - og->header_len - og->body_len;
}

static bool write_page(FILE *out, ogg_page *og)
{
    if (fwrite(og->header, 1, og->header_len, out) != (size_t)og->header_len)
        return false;

    if (fwrite(og->body, 1, og->body_len, out) != (size_t)og->body_len)
        return false;

    return true;
}

static int64_t write_page_and_get_next(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og)
{
    if (!write_page(out, og))
        return OGGEDIT_WRITE_ERROR;

    return get_page(in, oy, og);
}

int64_t copy_up_to_codec(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og, const off_t start_offset, const off_t link_offset, const char *codec)
{
    int64_t serial = skip_to_bos(in, oy, og, start_offset);

    if (fseek(out, sync_tell(in, oy, og), SEEK_SET))
        return OGGEDIT_SEEK_FAILED;

    while (serial > OGGEDIT_EOF && (sync_tell(in, oy, og) < link_offset || !ogg_page_bos(og) || strcmp(codec_name(og), codec)))
        serial = write_page_and_get_next(in, out, oy, og);

    return serial;
}

int64_t copy_up_to_header(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og, const int64_t codec_serial)
{
    int64_t serial;
    do
        serial = write_page_and_get_next(in, out, oy, og);
    while (serial > OGGEDIT_EOF && serial != codec_serial);
    return serial;
}

int64_t flush_stream(FILE *out, ogg_stream_state *os)
{
    ogg_page og;
#ifdef HAVE_OGG_STREAM_FLUSH_FILL
    while (ogg_stream_flush_fill(os, &og, MAXPAYLOAD))
#else
    while (ogg_stream_flush(os, &og))
#endif
        if (!write_page(out, &og))
            return OGGEDIT_WRITE_ERROR;

    const int64_t pageno = ogg_stream_check(os) ? OGGEDIT_FLUSH_FAILED : int32_to_unsigned(ogg_page_pageno(&og));
    ogg_stream_clear(os);
    return pageno;
}

char *codec_names(DB_FILE *in, ogg_sync_state *oy, const off_t link_offset)
{
    ogg_page og;
    int64_t serial = skip_to_bos(in, oy, &og, link_offset);
    char *codecs = strdup("Ogg");
    while (codecs && serial > OGGEDIT_EOF && ogg_page_bos(&og)) {
        codecs = cat_string(codecs, codec_name(&og), strcmp(codecs, "Ogg") ? "/" : " ");
        serial = get_page(in, oy, &og);
    }

    if (serial <= OGGEDIT_EOF) {
        free(codecs);
        return NULL;
    }

    return codecs;
}

off_t codec_stream_size(DB_FILE *in, ogg_sync_state *oy, const off_t start_offset, const off_t end_offset, const char *codec)
{
    /* Find codec serial and any other codecs */
    bool multiplex = false;
    ogg_page og;
    int64_t codec_serial = -1;
    int64_t serial = skip_to_bos(in, oy, &og, start_offset);
    while (serial > OGGEDIT_EOF && ogg_page_bos(&og)) {
        if (strcmp(codec_name(&og), codec))
            multiplex = true;
        else
            codec_serial = serial;
        serial = get_page(in, oy, &og);
    }

    /* Skip to the first codec data page */
    while (serial > OGGEDIT_EOF && !(ogg_page_granulepos(&og) > 0 && serial == codec_serial))
    //while (serial > OGGEDIT_EOF && !is_data_page(&og, codec_serial, serial))
        serial = get_page(in, oy,  &og);
    if (serial <= OGGEDIT_EOF)
        return serial;

    off_t stream_size = 0;
    if (multiplex) {
        /* Add up all the codec stream pages until EOF or a new link */
        while (serial > OGGEDIT_EOF && !ogg_page_bos(&og)) {
            if (serial == codec_serial)
                stream_size += og.header_len + og.body_len;
            serial = get_page(in, oy, &og);
        }
    }
    else {
        /* Find the exact offsets of the start and end of the audio */
        stream_size -= sync_tell(in, oy, &og);
        if (in->vfs->seek(in, end_offset, end_offset == 0 ? SEEK_END : SEEK_SET))
            return OGGEDIT_SEEK_FAILED;
        stream_size += in->vfs->tell(in);
        ogg_sync_reset(oy);
        while ((serial = get_page(in, oy, &og)) > OGGEDIT_EOF && !ogg_page_bos(&og))
            stream_size += og.header_len + og.body_len;
    }
    if (serial < OGGEDIT_EOF)
        return serial;

    return stream_size;
}

char *parse_vendor(const ogg_packet *op, const size_t magic_length)
{
    if (op->bytes < magic_length + 4)
        return NULL;

    const uint8_t *p = op->packet + magic_length;
    const uint32_t vendor_length = *p | *(p+1)<<8 | *(p+2)<<16 | *(p+3)<<24;
    if (op->bytes < magic_length + 4 + vendor_length)
        return NULL;

    char *vendor = calloc(vendor_length+1, 1);
    if (vendor)
        memcpy(vendor, p+4, vendor_length);
    return vendor;
}

int64_t init_read_stream(DB_FILE *in, ogg_sync_state *oy, ogg_stream_state *os, ogg_page *og, const off_t offset, const char *codec)
{
    int64_t serial = skip_to_codec(in, oy, og, offset, codec);
    serial = skip_to_header(in, oy, og, serial, serial);
    if (serial <= OGGEDIT_EOF)
        return serial;

    if (ogg_stream_init(os, (uint32_t)serial))
        return OGGEDIT_FAILED_TO_INIT_STREAM;

    os->b_o_s = 1;
    ogg_stream_pagein(os, og);

    return OGGEDIT_OK;
}

int64_t read_packet(DB_FILE *in, ogg_sync_state *oy, ogg_stream_state *os, ogg_page *og, ogg_packet *header, int64_t pages)
{
    ogg_packet op;
    do {
        while (ogg_stream_packetpeek(os, NULL) == 0) {
            const int64_t serial = get_page(in, oy, og);
            if (serial <= OGGEDIT_EOF) {
                return serial;
            }
            if ((uint32_t)os->serialno == (uint32_t)serial) {
                pages++;
                ogg_stream_pagein(os, og);
            }
        }
        if (ogg_stream_check(os))
            return OGGEDIT_FAILED_TO_STREAM_PAGE_FOR_PACKET;
    } while (ogg_stream_packetout(os, &op) != 1);

    memset(header, '\0', sizeof(*header));
    if (!header || !(header->packet = malloc(op.bytes))) {
        free(header);
        return OGGEDIT_ALLOCATION_FAILURE;
    }

    header->bytes = op.bytes;
    memcpy(header->packet, op.packet, op.bytes);
    return pages;
}

ogg_packet *fill_vc_packet(const char *magic, const size_t magic_length, const char *vendor, const size_t num_tags, char **tags, const bool framing, const size_t padding, ogg_packet *op)
{
    oggpack_buffer opb;
    oggpack_writeinit(&opb);
    _oggpack_chars(&opb, magic, magic_length);
    _oggpack_string(&opb, vendor);
    oggpack_write(&opb, num_tags, 32);
    for (size_t i = 0; i < num_tags; i++)
        _oggpack_string(&opb, tags[i]);
    if (framing) {
        oggpack_write(&opb, 1, 1);
        oggpack_writealign(&opb);
    }
#ifdef HAVE_OGG_STREAM_FLUSH_FILL
    for (size_t i = 0; i < padding; i++)
        oggpack_write(&opb, 0, 8); // use oggpack_writecopy when it is fixed
#endif
    if (oggpack_writecheck(&opb))
        return NULL;

    if (op) {
        memset(op, '\0', sizeof(*op));
        op->bytes = oggpack_bytes(&opb);
        op->packet = malloc(op->bytes);
        if (op->packet) {
            memcpy(op->packet, oggpack_get_buffer(&opb), op->bytes);
        }
    }
    oggpack_writeclear(&opb);

    if (!op->packet)
        return NULL;

    return op;
}

size_t vc_size(const char *vendor, size_t num_tags, char **tags)
{
    size_t metadata_size = 4 + strlen(vendor) + 4;
    for (size_t i = 0; i < num_tags; i++)
        metadata_size += strlen(tags[i]) + 4;
    return metadata_size;
}

int64_t copy_remaining_pages(DB_FILE *in, FILE *out, ogg_sync_state *oy, const int64_t codec_serial, uint32_t pageno)
{
    /* Skip past the codec header packets */
    ogg_page og;
    int64_t serial;
    do
        serial = get_page(in, oy, &og);
    while (serial > OGGEDIT_EOF && serial == codec_serial && ogg_page_granulepos(&og) <= 0);
    //while (serial > OGGEDIT_EOF && is_data_page(&og, codec_serial, serial));
    if (serial <= OGGEDIT_EOF)
        return serial;

    /* Renumber the rest of this link */
    while (serial > OGGEDIT_EOF && !ogg_page_bos(&og)) {
        if (serial == codec_serial && ogg_page_pageno(&og) != ++pageno) {
            og.header[18] = pageno & 0xFF;
            og.header[19] = pageno >> 8 & 0xFF;
            og.header[20] = pageno >> 16 & 0xFF;
            og.header[21] = pageno >> 24 & 0xFF;
            ogg_page_checksum_set(&og);
        }
        serial = write_page_and_get_next(in, out, oy,  &og);
    }

    /* Blindly copy remaining links */
    while (serial > OGGEDIT_EOF)
        serial = write_page_and_get_next(in, out, oy,  &og);
    if (serial < OGGEDIT_EOF)
        return serial;

    return OGGEDIT_OK;
}

int64_t write_all_streams(DB_FILE *in, FILE *out, ogg_sync_state *oy, const off_t offset)
{
    /* Copy BOS page(s) */
    ogg_page og;
    int64_t serial = skip_to_bos(in, oy, &og, offset);
    while (serial > OGGEDIT_EOF && ogg_page_bos(&og))
        serial = write_page_and_get_next(in, out, oy, &og);
    if (serial <= OGGEDIT_EOF)
        return serial;

    /* Copy all pages until EOF or next link */
    while (serial > OGGEDIT_EOF && !ogg_page_bos(&og))
        serial = write_page_and_get_next(in, out, oy, &og);
    if (serial < OGGEDIT_EOF)
        return serial;

    return OGGEDIT_OK;
}

int64_t write_one_stream(DB_FILE *in, FILE *out, ogg_sync_state *oy, const off_t offset, const char *codec)
{
    /* Find codec BOS page */
    ogg_page og;
    const int64_t codec_serial = skip_to_codec(in, oy, &og, offset, codec);
    if (codec_serial <= OGGEDIT_EOF)
        return codec_serial;

    /* Write it and skip the other BOS pages */
    int64_t serial = write_page_and_get_next(in, out, oy, &og);
    if ((serial = skip_to_header(in, oy, &og, serial, codec_serial)) <= OGGEDIT_EOF)
        return serial;

    /* Copy all codec pages until EOF or next link */
    while (serial > OGGEDIT_EOF && !ogg_page_bos(&og)) {
        if (serial == codec_serial && !write_page(out, &og))
            return OGGEDIT_WRITE_ERROR;
        serial = get_page(in, oy, &og);
    }
    if (serial < OGGEDIT_EOF)
        return serial;

    return OGGEDIT_OK;
}
