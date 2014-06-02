/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library

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
#include <stdbool.h>
#include <limits.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <ogg/ogg.h>
#include <deadbeef/deadbeef.h>
#include "oggedit.h"

#define CHUNKSIZE 4096
#define MAXPAGE 65536
#define MAXPAYLOAD 65025

/****************************
 * Ogg
 ****************************/
uint8_t *oggedit_vorbis_channel_map(const int channel_count)
{
    size_t map_size = channel_count * sizeof(uint8_t);
    uint8_t *map = malloc(map_size);
    if (!map)
        return NULL;
    switch(channel_count) {
        case 3:
            return memcpy(map, &(uint8_t[]){0,2,1}, map_size);
        case 5:
            return memcpy(map, &(uint8_t[]){0,2,1,3,4}, map_size);
        case 6:
            return memcpy(map, &(uint8_t[]){0,2,1,4,5,3}, map_size);
        case 7:
            return memcpy(map, &(uint8_t[]){0,2,1,4,5,6,3}, map_size);
        case 8:
            return memcpy(map, &(uint8_t[]){0,2,1,6,7,4,5,3}, map_size);
        default:
            free(map);
            return NULL;
    }
}

const char *oggedit_map_tag(char *key, const char *in_or_out)
{
    typedef struct {
        const char *tag;
        const char *meta;
    } key_t;
    const key_t keys[] = {
        /* Permanent named tags in DeaDBeef */
//        {.tag = "ARTIST",         .meta = "artist"},
//        {.tag = "TITLE",          .meta = "title"},
//        {.tag = "ALBUM",          .meta = "album"},
        {.tag = "DATE",           .meta = "year"},
        {.tag = "TRACKNUMBER",    .meta = "track"},
        {.tag = "TRACKTOTAL",     .meta = "numtracks"},
//        {.tag = "GENRE",          .meta = "genre"},
//        {.tag = "COMPOSER",       .meta = "composer"},
        {.tag = "DISCNUMBER",     .meta = "disc"},
//        {.tag = "COMMENT",        .meta = "comment"},
        /* Vorbis standard tags */
//        {.tag = "ARRANGER",       .meta = "Arranger"},
//        {.tag = "AUTHOR",         .meta = "Author"},
//        {.tag = "CONDUCTOR",      .meta = "Conductor"},
//        {.tag = "ENSEMBLE",       .meta = "Ensemble"},
//        {.tag = "LYRICIST",       .meta = "Lyricist"},
//        {.tag = "PERFORMER",      .meta = "Performer"},
//        {.tag = "PUBLISHER",      .meta = "Publisher"},
//        {.tag = "DISCTOTAL",      .meta = "Disctotal"},
//        {.tag = "OPUS",           .meta = "Opus"},
//        {.tag = "PART",           .meta = "Part"},
//        {.tag = "PARTNUMBER",     .meta = "Partnumber"},
//        {.tag = "VERSION",        .meta = "Version"},
//        {.tag = "DESCRIPTION",    .meta = "Description"},
//        {.tag = "COPYRIGHT",      .meta = "Copyright"},
//        {.tag = "LICENSE",        .meta = "License"},
//        {.tag = "CONTACT",        .meta = "Contact"},
//        {.tag = "ORGANIZATION",   .meta = "Organization"},
//        {.tag = "LOCATION",       .meta = "Location"},
//        {.tag = "EAN/UPN",        .meta = "EAN/UPN"},
//        {.tag = "ISRC",           .meta = "ISRC"},
//        {.tag = "LABEL",          .meta = "Label"},
//        {.tag = "LABELNO",        .meta = "Labelno"},
//        {.tag = "ENCODER",        .meta = "Encoder"},
//        {.tag = "ENCODED-BY",     .meta = "Encoded-by"},
//        {.tag = "ENCODING",       .meta = "Encoding"},
        /* Other tags */
//        {.tag = "ALBUMARTIST",    .meta = "Albumartist"},
//        {.tag = "ALBUM ARTIST",   .meta = "Album artist"},
//        {.tag = "BAND",           .meta = "Band"},
//        {.tag = "COMPILATION",    .meta = "Compilation"},
//        {.tag = "TOTALTRACKS",    .meta = "Totaltracks"},
//        {.tag = "ENCODED_BY",     .meta = "Encoded_by"},
//        {.tag = "ENCODER_OPTIONS",.meta = "Encoder_options"},
        {.tag = NULL}
    };

    /* Mapping for special Deadbeef internal metadata */
    for (const key_t *match = keys; match->tag; match++)
        if (!strcasecmp(*in_or_out == 't' ? match->tag : match->meta, key))
            return *in_or_out == 't' ? match->meta : match->tag;

    /* Upper-case all Vorbis Comment tag names */
    if (*in_or_out == 'm')
        for (size_t i = 0; key[i]; i++)
            key[i] = toupper(key[i]);

    return key;
}

const char *oggedit_album_art_type(const int type)
{
    switch (type) {
        case 1:
            return "32x32 pixels file icon";
        case 2:
            return "other file icon";
        case 3:
            return "front cover";
        case 4:
            return "back cover";
        case 5:
            return "leaflet page";
        case 6:
            return "media";
        case 7:
            return "lead artist/lead performer/soloist";
        case 8:
            return "artist/performer";
        case 9:
            return "conductor";
        case 10:
            return "band/orchestra";
        case 11:
            return "composer";
        case 12:
            return "lyricist/text writer";
        case 13:
            return "recording location";
        case 14:
            return "during recording";
        case 15:
            return "during performance";
        case 16:
            return "movie/video screen capture";
        case 17:
            return "bright coloured fish";
        case 18:
            return "illustration";
        case 19:
            return "band/artist logotype";
        case 20:
            return "publisher/studio logotype";
        default:
            return "other";
    }
}

static char *cat_string(char *dest, const char *src, const char *sep)
{
    char *more = realloc(dest, strlen(dest) + strlen(src) + strlen(sep) + 1);
    if (!more) {
        free(dest);
        return NULL;
    }
    return strcat(strcat(more, sep), src);
}

static const char *codec_name(ogg_page *og)
{
    typedef struct {
        const size_t length;
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
        if ((size_t)og->body_len >= match->length && !memcmp(og->body, match->magic, strlen(match->codec)))
            return match->codec;

    return "unknown";
}

static char *btoa(const unsigned char *binary, const size_t binary_length)
{
    const char b64[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
        '0','1','2','3','4','5','6','7','8','9','+','/'
    };

    char *ascii_base64 = malloc((binary_length-1)/3 * 4 + 5);
    if (!ascii_base64)
        return NULL;

    char *p = ascii_base64;
    const unsigned char *q = binary;
    const unsigned char *binary_end = binary + binary_length;
    while (q+2 < binary_end) {
        uint32_t chunk = q[0]<<16 | q[1]<<8 | q[2];
        p[3] = b64[chunk & 0x3f];
        p[2] = b64[chunk>>6 & 0x3f];
        p[1] = b64[chunk>>12 & 0x3f];
        p[0] = b64[chunk>>18 & 0x3f];
        p+=4, q+=3;
    }

    if (q < binary_end) {
        uint16_t remainder = q[0]<<8 | (q+1 == binary_end ? '\0' : q[1]);
        p[3] = '=';
        p[2] = q+1 == binary_end ? '=' : b64[remainder<<2 & 0x3f];
        p[1] = b64[remainder>>4 & 0x3f];
        p[0] = b64[remainder>>10 & 0x3f];
        p+=4;
    }
    *p = '\0';

    return ascii_base64;
}

static void _oggpack_chars(oggpack_buffer *opb, const char *s, size_t length)
{
//    oggpack_writecopy(opb, s, length * 8); currently broken
    while (length--)
        oggpack_write(opb, *s++, 8);
}

static void _oggpack_string(oggpack_buffer *opb, const char *s)
{
    oggpack_write(opb, strlen(s), 32);
    _oggpack_chars(opb, s, strlen(s));
}

static void _oggpackB_string(oggpack_buffer *opb, const char *s)
{
    oggpackB_write(opb, strlen(s), 32);
    _oggpack_chars(opb, s, strlen(s));
}

char *oggedit_album_art_tag(DB_FILE *fp, int *res)
{
    if (!fp) {
        *res = OGGEDIT_FILE_NOT_OPEN;
        return NULL;
    }

    const int64_t data_length = fp->vfs->getlength(fp);
    if (data_length < 50 || data_length > 10000000) {
        fp->vfs->close(fp);
        *res = OGGEDIT_BAD_FILE_LENGTH;
        return NULL;
    }

    char *data = malloc(data_length);
    if (!data) {
        fp->vfs->close(fp);
        *res = OGGEDIT_ALLOCATION_FAILURE;
        return NULL;
    }

    const size_t data_read = fp->vfs->read(data, 1, data_length, fp);
    fp->vfs->close(fp);
    if (data_read != data_length) {
        free(data);
        *res = OGGEDIT_CANT_READ_IMAGE_FILE;
        return NULL;
    }

    oggpack_buffer opb;
    oggpackB_writeinit(&opb);
    oggpackB_write(&opb, 3, 32);
    _oggpackB_string(&opb, memcmp(data, "\x89PNG\x0D\x0A\x1A\x0A", 8) ? "image/jpeg" : "image/png");
    _oggpackB_string(&opb, "Album art added from DeaDBeeF");
    oggpackB_write(&opb, 0, 32);
    oggpackB_write(&opb, 0, 32);
    oggpackB_write(&opb, 0, 32);
    oggpackB_write(&opb, 0, 32);
    oggpackB_write(&opb, data_length, 32);
    _oggpack_chars(&opb, data, data_length);
    free(data);
    if (oggpack_writecheck(&opb)) {
        *res = OGGEDIT_ALLOCATION_FAILURE;
        return NULL;
    }

    char *tag = btoa(oggpackB_get_buffer(&opb), oggpackB_bytes(&opb));
    oggpackB_writeclear(&opb);
    return tag;
}

static bool ensure_directory(const char *path)
{
    struct stat stat_struct;
    if (!stat(path, &stat_struct))
        return !S_ISDIR(stat_struct.st_mode);

    if (errno != ENOENT)
        return false;

    char* dir = strdup(path);
    if (!dir)
        return false;

    const int bad_dir = ensure_directory(dirname(dir));
    free(dir);

    return !bad_dir && !mkdir(path, 0777);
}

static int finish_temp_file(const char *tempname, const char *fname)
{
    return rename(tempname, fname) ? OGGEDIT_RENAME_FAILED : 0;
}

static int open_temp_file(const char *fname, char *tempname, FILE **out)
{
    snprintf(tempname, PATH_MAX, "%s.temp", fname);
    unlink(tempname);
    if (!(*out = freopen(tempname, "abx", *out)))
        return OGGEDIT_CANNOT_OPEN_TEMPORARY_FILE;

    struct stat stat_struct;
    if (!stat(fname, &stat_struct))
        chmod(tempname, stat_struct.st_mode);

    return 0;
}

static FILE *open_new_file(const char *outname)
{
    char outpath[PATH_MAX];
    strcpy(outpath, outname);
    if (!ensure_directory(dirname(outpath)))
        return NULL;

    unlink(outname);
    return fopen(outname, "wbx");
}

static off_t file_size(const char *fname)
{
    struct stat sb;
    if (stat(fname, &sb))
        return OGGEDIT_STAT_FAILED;

    return sb.st_size;
}

static void cleanup(DB_FILE *in, FILE *out, ogg_sync_state *oy, void *buffer)
{
    if (in)
        in->vfs->close(in);

    if (out)
        fclose(out);

    ogg_sync_clear(oy);

    if (buffer)
        free(buffer);
}

static int get_page(DB_FILE *in, ogg_sync_state *oy, ogg_page *og)
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

    return ogg_page_serialno(og);
}

static int skip_to_bos(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, const off_t offset)
{
    if (!in)
        return OGGEDIT_FILE_NOT_OPEN;

    if (in->vfs->seek(in, offset, SEEK_SET))
        return OGGEDIT_SEEK_FAILED;

    ogg_sync_reset(oy);
    int serial;
    do
        serial = get_page(in, oy, og);
    while (serial > OGGEDIT_EOF && !ogg_page_bos(og));

    return serial;
}

static int skip_to_codec(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, const off_t offset, const char *codec)
{
    int serial = skip_to_bos(in, oy, og, offset);
    while (serial > OGGEDIT_EOF && strcmp(codec_name(og), codec))
        serial = get_page(in, oy, og);

    return serial;
}

static int skip_to_header(DB_FILE *in, ogg_sync_state *oy, ogg_page *og, int serial, const int codec_serial)
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

static int write_page_and_get_next(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og)
{
    if (!write_page(out, og))
        return OGGEDIT_WRITE_ERROR;

    return get_page(in, oy, og);
}

static int copy_up_to_codec(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og, const off_t start_offset, const off_t link_offset, const char *codec)
{
    int serial = skip_to_bos(in, oy, og, start_offset);

    if (fseek(out, sync_tell(in, oy, og), SEEK_SET))
        return OGGEDIT_SEEK_FAILED;

    while (serial > OGGEDIT_EOF && (sync_tell(in, oy, og) < link_offset || !ogg_page_bos(og) || strcmp(codec_name(og), codec)))
        serial = write_page_and_get_next(in, out, oy, og);

    return serial;
}

static int copy_up_to_header(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og, const int codec_serial)
{
    int serial;
    do
        serial = write_page_and_get_next(in, out, oy, og);
    while (serial > OGGEDIT_EOF && serial != codec_serial);
    return serial;
}

static long flush_stream(FILE *out, ogg_stream_state *os)
{
    ogg_page og;
    while (ogg_stream_flush_fill(os, &og, MAXPAYLOAD))
        if (!write_page(out, &og))
            return OGGEDIT_WRITE_ERROR;
    const long pageno = ogg_stream_check(os) ? OGGEDIT_FLUSH_FAILED : ogg_page_pageno(&og);
    ogg_stream_clear(os);
    return pageno;
}

static char *codec_names(DB_FILE *in, ogg_sync_state *oy, const off_t link_offset, int *res)
{
    ogg_page og;
    *res = skip_to_bos(in, oy, &og, link_offset);
    char *codecs = strdup("Ogg");
    while (codecs && *res > OGGEDIT_EOF && ogg_page_bos(&og)) {
        codecs = cat_string(codecs, codec_name(&og), strcmp(codecs, "Ogg") ? "/" : " ");
        *res = get_page(in, oy, &og);
    }
    if (!*codecs) {
        *res = OGGEDIT_ALLOCATION_FAILURE;
        return NULL;
    }
    if (*res <= OGGEDIT_EOF) {
        free(codecs);
        return NULL;
    }

    *res = OGGEDIT_OK;
    return codecs;
}

static off_t codec_stream_size(DB_FILE *in, ogg_sync_state *oy, const off_t start_offset, const off_t end_offset, const char *codec)
{
    /* Find codec serial and any other codecs */
    bool multiplex = false;
    ogg_page og;
    int codec_serial = -1;
    int serial = skip_to_bos(in, oy, &og, start_offset);
    while (serial > OGGEDIT_EOF && ogg_page_bos(&og)) {
        if (strcmp(codec_name(&og), codec))
            multiplex = true;
        else
            codec_serial = serial;
        serial = get_page(in, oy, &og);
    }

    /* Skip to the first codec data page */
    while (serial > OGGEDIT_EOF && !(ogg_page_granulepos(&og) > 0 && serial == codec_serial))
        if ((serial = get_page(in, oy,  &og)) <= OGGEDIT_EOF)
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

static char *parse_vendor(const ogg_packet *op, const size_t magic_length)
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

static int init_read_stream(DB_FILE *in, ogg_sync_state *oy, ogg_stream_state *os, ogg_page *og, const off_t offset, const char *codec)
{
    int serial = skip_to_codec(in, oy, og, offset, codec);
    serial = skip_to_header(in, oy, og, serial, serial);
    if (serial <= OGGEDIT_EOF)
        return serial;

    if (ogg_stream_init(os, serial))
        return OGGEDIT_FAILED_TO_INIT_STREAM;

    os->b_o_s = 1;
    ogg_stream_pagein(os, og);

    return OGGEDIT_OK;
}

static int read_packet(DB_FILE *in, ogg_sync_state *oy, ogg_stream_state *os, ogg_page *og, ogg_packet *header, int pages)
{
    ogg_packet op;
    do {
        while (ogg_stream_packetpeek(os, NULL) == 0) {
            const int serial = get_page(in, oy, og);
            if (serial <= OGGEDIT_EOF) {
                return serial;
            }
            if (os->serialno == serial) {
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

static ogg_packet *fill_vc_packet(const char *magic, const size_t magic_length, const char *vendor, const size_t num_tags, char **tags, const bool framing, const size_t padding, ogg_packet *op)
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
    for (size_t i = 0; i < padding; i++)
        oggpack_write(&opb, 0, 8); // use oggpack_writecopy when it is fixed
    if (oggpack_writecheck(&opb))
        return NULL;

    if (op) {
        memset(op, '\0', sizeof(*op));
        op->bytes = oggpack_bytes(&opb);
        if (op->packet = malloc(op->bytes))
            memcpy(op->packet, oggpack_get_buffer(&opb), op->bytes);
    }
    oggpack_writeclear(&opb);

    if (!op->packet)
        return NULL;

    return op;
}

static size_t vc_size(const char *vendor, size_t num_tags, char **tags)
{
    size_t metadata_size = 4 + strlen(vendor) + 4;
    for (size_t i = 0; i < num_tags; i++)
        metadata_size += strlen(tags[i]) + 4;
    return metadata_size;
}

static int copy_remaining_pages(DB_FILE *in, FILE *out, ogg_sync_state *oy, const int codec_serial, uint32_t pageno)
{
    /* Skip past the codec header packets */
    ogg_page og;
    int serial;
    do
        serial = get_page(in, oy, &og);
    while (serial > OGGEDIT_EOF && serial == codec_serial && ogg_page_granulepos(&og) <= 0);

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

    return serial;
}

static int write_all_streams(DB_FILE *in, FILE *out, ogg_sync_state *oy, const off_t offset)
{
    /* Copy BOS page(s) */
    ogg_page og;
    int serial = skip_to_bos(in, oy, &og, offset);
    while (serial > OGGEDIT_EOF && ogg_page_bos(&og))
        serial = write_page_and_get_next(in, out, oy, &og);
    if (serial <= OGGEDIT_EOF)
        return serial;

    /* Copy all pages until EOF or next link */
    while (serial > OGGEDIT_EOF && !ogg_page_bos(&og))
        if ((serial = write_page_and_get_next(in, out, oy, &og)) < OGGEDIT_EOF)
            return serial;

    return OGGEDIT_OK;
}

static int write_one_stream(DB_FILE *in, FILE *out, ogg_sync_state *oy, const off_t offset, const char *codec)
{
    /* Find codec BOS page */
    ogg_page og;
    const int codec_serial = skip_to_codec(in, oy, &og, offset, codec);
    if (codec_serial <= OGGEDIT_EOF)
        return codec_serial;

    /* Write it and skip the other BOS pages */
    int serial = write_page_and_get_next(in, out, oy, &og);
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

int oggedit_write_file(DB_FILE *in, const char *outname, const off_t offset, const char *codec)
{
    FILE *out = open_new_file(outname);
    if (!out)
        return OGGEDIT_CANNOT_OPEN_OUTPUT_FILE;

    ogg_sync_state oy;
    ogg_sync_init(&oy);

    int res;
    if (codec)
        res = write_one_stream(in, out, &oy, offset, codec);
    else
        res = write_all_streams(in, out, &oy, offset);

    cleanup(in, out, &oy, NULL);

    if (res <= OGGEDIT_EOF)
        unlink(outname);

    return res;
}

off_t oggedit_stream_info(DB_FILE *in, const off_t start_offset, const off_t end_offset, const char *codec, char **codecs)
{
    int res;
    ogg_sync_state oy;
    ogg_sync_init(&oy);
    *codecs = codec_names(in, &oy, start_offset, &res);
    const off_t stream_size = codec_stream_size(in, &oy, start_offset, end_offset, codec);
    cleanup(in, NULL, &oy, NULL);
    return stream_size;
}

/****************************
 * FLAC
 ****************************/
#define PADTYPE 0x01
#define VCTYPE 0x04
#define FLACLAST 0x80

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

static ogg_packet **flac_headers_alloc(ogg_packet **headers, const size_t packets)
{
    ogg_packet **new_headers = realloc(headers, (packets+2) * sizeof(headers));
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
    while ((headers = flac_headers_alloc(headers, packets)) &&
           (pages = read_packet(in, oy, &os, &og, headers[packets], pages)) > OGGEDIT_EOF &&
           (headers[packets++]->packet[0] & FLACLAST) != FLACLAST);

    ogg_stream_clear(&os);

    if (!headers)
        *res = OGGEDIT_ALLOCATION_FAILURE;
    else if (!packets || headers[0]->packet[0] & 0x3F != VCTYPE)
        *res = OGGEDIT_CANNOT_PARSE_HEADERS;
    else
        *res = pages;
    if (*res <= OGGEDIT_EOF) {
        clear_header_list(headers);
        return NULL;
    }

    *vendor = parse_vendor(headers[0], 4);
    size_t bytes = 0;
    for (size_t i = 0; i < packets; i++)
        bytes += headers[i]->bytes;
    if (bytes < MAXPAYLOAD * (pages-1))
        headers[0]->bytes = 4;

    *res = OGGEDIT_OK;
    return headers;
}

static long write_metadata_block_packets(FILE *out, const int serial, const char *vendor, const size_t num_tags, char **tags, size_t padding, ogg_packet **metadata)
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
            metadata[i]->packet[0] |= FLACLAST;
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
    if (stream_size_k < 1000 || padding < 0 || headers[1] && padding > 0 || padding > stream_size_k+metadata_size)
        if (res = open_temp_file(fname, tempname, &out))
            goto cleanup;

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
    const int flac_serial = copy_up_to_codec(in, out, &oy, &og, *tempname ? 0 : offset, offset, FLACNAME);
    if (flac_serial <= OGGEDIT_EOF) {
        res = flac_serial;
        goto cleanup;
    }
    if ((res = copy_up_to_header(in, out, &oy, &og, flac_serial)) <= OGGEDIT_EOF)
        goto cleanup;
    const long pageno = write_metadata_block_packets(out, flac_serial, vendor, num_tags, tags, padding, headers);
    if (pageno < OGGEDIT_EOF) {
        res = pageno;
        goto cleanup;
    }

    /* If we have tempfile, copy the remaining pages */
    if (*tempname)
        if ((res = copy_remaining_pages(in, out, &oy, flac_serial, pageno)) < OGGEDIT_EOF || (res = finish_temp_file(tempname, fname)))
            goto cleanup;

    res = file_size(fname);

cleanup:
    clear_header_list(headers);
    cleanup(in, out, &oy, vendor);
    if (res < OGGEDIT_OK)
        unlink(tempname);
    return res;
}

/****************************
 * Vorbis
 ****************************/
#define VCMAGIC "\3vorbis"
#define CODEMAGIC "\5vorbis"

static ptrdiff_t check_vorbis_headers(DB_FILE *in, ogg_sync_state *oy, const off_t offset, char **vendor, ogg_packet *codebooks)
{
    ogg_stream_state os;
    ogg_page og;
    const int serial = init_read_stream(in, oy, &os, &og, offset, VORBISNAME);
    if (serial <= OGGEDIT_EOF)
        return serial;

    ogg_packet vc;
    int pages = read_packet(in, oy, &os, &og, &vc, 1);
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

    if ((vc.bytes + codebooks->bytes) < MAXPAYLOAD * (pages-1))
        return 4; // prevent in-place write if the packets are split over too many pages

    return vc.bytes;
}

static long write_vorbis_tags(FILE *out, const int serial, const char *vendor, const size_t num_tags, char **tags, const size_t padding, ogg_packet *codebooks)
{
    ogg_packet op;
    if (!fill_vc_packet(VCMAGIC, strlen(VCMAGIC), vendor, num_tags, tags, true, padding, &op))
        return OGGEDIT_ALLOCATION_FAILURE;

    ogg_stream_state os;
    if (ogg_stream_init(&os, serial))
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
    if (file_size_k < 100 || padding < 0 || padding > file_size_k/10+stream_size_k+metadata_size)
        if (res = open_temp_file(fname, tempname, &out))
            goto cleanup;

    /* Re-pad if writing the whole file */
    if (*tempname)
        padding = stream_size_k < 90 ? 0 : stream_size_k < 1000 ? 128 : stream_size_k < 10000 ? 1024 : 8192;

    /* Write pages until the correct comment header */
    ogg_page og;
    const int vorbis_serial = copy_up_to_codec(in, out, &oy, &og, *tempname ? 0 : offset, offset, VORBISNAME);
    if (vorbis_serial <= OGGEDIT_EOF) {
        res = vorbis_serial;
        goto cleanup;
    }
    if ((res = copy_up_to_header(in, out, &oy, &og, vorbis_serial)) <= OGGEDIT_EOF)
        goto cleanup;
    const long pageno = write_vorbis_tags(out, vorbis_serial, vendor, num_tags, tags, padding, &codebooks);
    if (pageno < OGGEDIT_EOF) {
        res = pageno;
        goto cleanup;
    }

    /* If we have tempfile, copy the remaining pages */
    if (*tempname)
        if ((res = copy_remaining_pages(in, out, &oy, vorbis_serial, pageno)) < OGGEDIT_EOF || (res = finish_temp_file(tempname, fname)))
            goto cleanup;

    res = file_size(fname);

cleanup:
    ogg_packet_clear(&codebooks);
    cleanup(in, out, &oy, vendor);
    if (res < OGGEDIT_OK)
        unlink(tempname);
    return res;
}

/****************************
 * Opus
 ****************************/
#define TAGMAGIC "OpusTags"

static ptrdiff_t check_opus_header(DB_FILE *in, ogg_sync_state *oy, const off_t offset, char **vendor)
{
    ogg_stream_state os;
    ogg_page og;
    const int serial = init_read_stream(in, oy, &os, &og, offset, OPUSNAME);
    if (serial <= OGGEDIT_EOF)
        return serial;

    ogg_packet op;
    const int pages = read_packet(in, oy, &os, &og, &op, 1);
    ogg_stream_clear(&os);
    if (pages <= OGGEDIT_EOF)
        return pages;

    if (op.bytes > strlen(TAGMAGIC) && !memcmp(op.packet, TAGMAGIC, strlen(TAGMAGIC)))
        *vendor = parse_vendor(&op, strlen(TAGMAGIC));
    free(op.packet);
    if (!*vendor)
        return OGGEDIT_CANNOT_PARSE_HEADERS;

    if (op.bytes < MAXPAYLOAD * (pages-1))
        return 4; // prevent in-place write if the packet is weirdly split into too many pages

    return op.bytes;
}

static long write_opus_tags(FILE *out, const int serial, const char *vendor, const size_t num_tags, char **tags, const size_t padding)
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

off_t oggedit_write_opus_metadata(DB_FILE *in, const char *fname, const off_t offset, const size_t stream_size, const int output_gain, const int num_tags, char **tags)
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
    const ptrdiff_t tags_packet_size = check_opus_header(in, &oy, offset, &vendor);
    if (tags_packet_size <= OGGEDIT_EOF) {
        res = tags_packet_size;
        goto cleanup;
    }
    const size_t metadata_size = strlen(TAGMAGIC) + vc_size(vendor, num_tags, tags);
    ptrdiff_t padding = tags_packet_size - metadata_size;
    const off_t file_size_k = in->vfs->getlength(in) / 1000;
    const size_t stream_size_k = stream_size ? stream_size / 1000 : file_size_k;
    if (file_size_k < 100 || padding < 0 || padding > file_size_k/10+stream_size_k+metadata_size)
        if (res = open_temp_file(fname, tempname, &out))
            goto cleanup;

    /* Re-pad if writing the whole file */
    if (*tempname)
        padding = stream_size_k < 90 ? 0 : stream_size_k < 1000 ? 128 : stream_size_k < 10000 ? 1024 : 8192;

    /* Write pages until we reach the correct OpusHead, then write OpusTags */
    ogg_page og;
    const int opus_serial = copy_up_to_codec(in, out, &oy, &og, *tempname ? 0 : offset, offset, OPUSNAME);
    if (opus_serial <= OGGEDIT_EOF) {
        res = opus_serial;
        goto cleanup;
    }
    if (output_gain > INT_MIN) {
        og.body[16] = output_gain & 0xFF;
        og.body[17] = output_gain >> 8 & 0xFF;
        ogg_page_checksum_set(&og);
    }
    if ((res = copy_up_to_header(in, out, &oy, &og, opus_serial)) <= OGGEDIT_EOF)
        goto cleanup;
    const long pageno = write_opus_tags(out, opus_serial, vendor, num_tags, tags, (size_t)padding);
    if (pageno < OGGEDIT_EOF) {
        res = pageno;
        goto cleanup;
    }

    /* If we have tempfile, copy the remaining pages */
    if (*tempname)
        if ((res = copy_remaining_pages(in, out, &oy, opus_serial, pageno)) < OGGEDIT_EOF || (res = finish_temp_file(tempname, fname)))
            goto cleanup;

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
int usecs = timeval.tv_sec* 1000000 + timeval.tv_usec;
gettimeofday(&timeval, NULL);
usecs = timeval.tv_sec* 1000000 + timeval.tv_usec - usecs;
fprintf(stderr, "%d micro-seconds\n", usecs);
*/
