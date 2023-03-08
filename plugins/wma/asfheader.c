// this file is a modified version for deadbeef player
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <unistd.h>
#include <deadbeef/deadbeef.h>

#include <libasf/asf.h>

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)
#define DEBUGF trace

#define SKIP_BYTES(fd,x) {\
    if (x > 0) {\
        deadbeef->fseek(fd, x, SEEK_CUR);\
    }\
}

extern DB_functions_t *deadbeef;

enum {
    AFMT_WMAPRO = 1,
    AFMT_WMAVOICE = 2,
};

static inline uint16_t swap16(uint16_t value)
    /*
       result[15..8] = value[ 7..0];
       result[ 7..0] = value[15..8];
       */
{
    return (value >> 8) | (value << 8);
}


static inline uint32_t swap32(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t hi = swap16(value >> 16);
    uint32_t lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

#if WORDS_BIGENDIAN
#define read_uint16be(fd,buf) deadbeef->fread((buf), 2, 1, (fd))
#define read_uint32be(fd,buf) deadbeef->fread((buf), 4, 1, (fd))
#define read_uint64be(fd,buf) deadbeef->fread((buf), 8, 1, (fd))
int read_uint16le(DB_FILE *fd, uint16_t* buf);
int read_uint32le(DB_FILE *fd, uint32_t* buf);
int read_uint64le(DB_FILE *fd, uint64_t* buf);
#define letoh16(x) swap16(x)
#define letoh32(x) swap32(x)
#define htole16(x) swap16(x)
#define htole32(x) swap32(x)
#define betoh16(x) (x)
#define betoh32(x) (x)
#define htobe16(x) (x)
#define htobe32(x) (x)
#define swap_odd_even_be32(x) swap_odd_even32(x)
#define swap_odd_even_le32(x) (x)
#else
#define letoh16(x) (x)
#define letoh32(x) (x)
#define htole16(x) (x)
#define htole32(x) (x)
#define betoh16(x) swap16(x)
#define betoh32(x) swap32(x)
//#define htobe16(x) swap16(x)
//#define htobe32(x) swap32(x)
#define swap_odd_even_be32(x) (x)
#define swap_odd_even_le32(x) swap_odd_even32(x)
int read_uint16be(DB_FILE *fd, uint16_t* buf);
int read_uint32be(DB_FILE *fd, uint32_t* buf);
int read_uint64be(DB_FILE *fd, uint64_t* buf);
#define read_uint16le(fd,buf) deadbeef->fread((buf), 2, 1, (fd))
#define read_uint32le(fd,buf) deadbeef->fread((buf), 4, 1, (fd))
#define read_uint64le(fd,buf) deadbeef->fread((buf), 8, 1, (fd))
#endif


#if !WORDS_BIGENDIAN
/* Read an unsigned 16-bit integer from a big-endian file. */
int read_uint16be(DB_FILE *fd, uint16_t* buf)
{
  size_t n;

  n = deadbeef->fread(buf, 1, 2, fd);
  *buf = betoh16(*buf);
  return n;
}
/* Read an unsigned 32-bit integer from a big-endian file. */
int read_uint32be(DB_FILE *fd, uint32_t* buf)
{
  size_t n;

  n = deadbeef->fread(buf, 1, 4, fd);
  *buf = betoh32(*buf);
  return n;
}
/* Read an unsigned 64-bit integer from a big-endian file. */
int read_uint64be(DB_FILE *fd, uint64_t* buf)
{
  size_t n;
  uint8_t data[8];
  int i;

  n = deadbeef->fread(data, 1, 8, fd);

  for (i=0, *buf=0; i<=7; i++) {
       *buf <<= 8;
       *buf |= data[i];
  }
  return n;
}
#else
/* Read unsigned integers from a little-endian file. */
int read_uint16le(DB_FILE *fd, uint16_t* buf)
{
  size_t n;

  n = deadbeef->fread((char*) buf, 1, 2, fd);
  *buf = letoh16(*buf);
  return n;
}
int read_uint32le(DB_FILE *fd, uint32_t* buf)
{
  size_t n;

  n = deadbeef->fread(buf, 1, 4, fd);
  *buf = letoh32(*buf);
  return n;
}
int read_uint64le(DB_FILE *fd, uint64_t* buf)
{
  size_t n;
  uint8_t data[8];
  int i;

  n = deadbeef->fread(data, 1, 8, fd);

  for (i=7, *buf=0; i>=0; i--) {
       *buf <<= 8;
       *buf |= data[i];
  }

  return n;
}
#endif

/* TODO: Just read the GUIDs into a 16-byte array, and use memcmp to compare */
struct guid_s {
    uint32_t v1;
    uint16_t v2;
    uint16_t v3;
    uint8_t  v4[8];
};
typedef struct guid_s guid_t;

struct asf_object_s {
    guid_t       guid;
    uint64_t     size;
    uint64_t     datalen;
};
typedef struct asf_object_s asf_object_t;

static const guid_t asf_guid_null =
{0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/* top level object guids */

static const guid_t asf_guid_header =
{0x75B22630, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_guid_data =
{0x75B22636, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_guid_index =
{0x33000890, 0xE5B1, 0x11CF, {0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB}};

/* header level object guids */

static const guid_t asf_guid_file_properties =
{0x8cabdca1, 0xa947, 0x11cf, {0x8E, 0xe4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_guid_stream_properties =
{0xB7DC0791, 0xA9B7, 0x11CF, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_guid_content_description =
{0x75B22633, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_guid_extended_content_description =
{0xD2D0A440, 0xE307, 0x11D2, {0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50}};

static const guid_t asf_guid_content_encryption =
{0x2211b3fb, 0xbd23, 0x11d2, {0xb4, 0xb7, 0x00, 0xa0, 0xc9, 0x55, 0xfc, 0x6e}};

static const guid_t asf_guid_extended_content_encryption =
{0x298ae614, 0x2622, 0x4c17, {0xb9, 0x35, 0xda, 0xe0, 0x7e, 0xe9, 0x28, 0x9c}};

/* stream type guids */

static const guid_t asf_guid_stream_type_audio =
{0xF8699E40, 0x5B4D, 0x11CF, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static int asf_guid_match(const guid_t *guid1, const guid_t *guid2)
{
    if((guid1->v1 != guid2->v1) ||
       (guid1->v2 != guid2->v2) ||
       (guid1->v3 != guid2->v3) ||
       (memcmp(guid1->v4, guid2->v4, 8))) {
        return 0;
    }

    return 1;
}

/* Read the 16 byte GUID from a file */
static void asf_readGUID(DB_FILE *fd, guid_t* guid)
{
    read_uint32le(fd, &guid->v1);
    read_uint16le(fd, &guid->v2);
    read_uint16le(fd, &guid->v3);
    deadbeef->fread(guid->v4, 8, 1, fd);
}

static void asf_read_object_header(asf_object_t *obj, DB_FILE *fd)
{
    asf_readGUID(fd, &obj->guid);
    read_uint64le(fd, &obj->size);
    obj->datalen = 0;
}

/* Parse an integer from the extended content object - we always
   convert to an int, regardless of native format.
*/
static int asf_intdecode(DB_FILE *fd, int type, int length)
{
    uint16_t tmp16;
    uint32_t tmp32;
    uint64_t tmp64;

    if (type == 3) {
        read_uint32le(fd, &tmp32);
        SKIP_BYTES(fd,length - 4);
        return (int)tmp32;
    } else if (type == 4) {
        read_uint64le(fd, &tmp64);
        SKIP_BYTES(fd,length - 8);
        return (int)tmp64;
    } else if (type == 5) {
        read_uint16le(fd, &tmp16);
        SKIP_BYTES(fd,length - 2);
        return (int)tmp16;
    }

    return 0;
}

/* Decode a LE utf16 string from a disk buffer into a fixed-sized
   utf8 buffer.
*/
#define MASK   0xC0 /* 11000000 */
#define COMP   0x80 /* 10x      */

static const unsigned char utf8comp[6] =
{
    0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

/* Encode a UCS value as UTF-8 and return a pointer after this UTF-8 char. */
unsigned char* utf8encode(unsigned long ucs, unsigned char *utf8)
{
    int tail = 0;

    if (ucs > 0x7F)
        while (ucs >> (5*tail + 6))
            tail++;

    *utf8++ = (ucs >> (6*tail)) | utf8comp[tail];
    while (tail--)
        *utf8++ = ((ucs >> (6*tail)) & (MASK ^ 0xFF)) | COMP;

    return utf8;
}


static void asf_utf16LEdecode(DB_FILE *fd,
                              uint16_t utf16bytes,
                              unsigned char **utf8,
                              int* utf8bytes
                             )
{
    unsigned long ucs;
    int n;
    unsigned char utf16buf[256];
    unsigned char* utf16 = utf16buf;
    unsigned char* newutf8;

    n = deadbeef->fread(utf16buf, 1, MIN(sizeof(utf16buf), utf16bytes), fd);
    utf16bytes -= n;

    while (n > 0) {
        /* Check for a surrogate pair */
        if (utf16[1] >= 0xD8 && utf16[1] < 0xE0) {
            if (n < 4) {
                /* Run out of utf16 bytes, read some more */
                utf16buf[0] = utf16[0];
                utf16buf[1] = utf16[1];

                n = deadbeef->fread(utf16buf + 2, 1, MIN(sizeof(utf16buf)-2, utf16bytes), fd);
                utf16 = utf16buf;
                utf16bytes -= n;
                n += 2;
            }

            if (n < 4) {
                /* Truncated utf16 string, abort */
                break;
            }
            ucs = 0x10000 + ((utf16[0] << 10) | ((utf16[1] - 0xD8) << 18)
                             | utf16[2] | ((utf16[3] - 0xDC) << 8));
            utf16 += 4;
            n -= 4;
        } else {
            ucs = (utf16[0] | (utf16[1] << 8));
            utf16 += 2;
            n -= 2;
        }

        if (*utf8bytes > 6) {
            newutf8 = utf8encode(ucs, *utf8);
            *utf8bytes -= (newutf8 - *utf8);
            *utf8 += (newutf8 - *utf8);
        }

        /* We have run out of utf16 bytes, read more if available */
        if ((n == 0) && (utf16bytes > 0)) {
            n = deadbeef->fread(utf16buf, 1, MIN(sizeof(utf16buf), utf16bytes), fd);
            utf16 = utf16buf;
            utf16bytes -= n;
        }
    }

    *utf8[0] = 0;
    --*utf8bytes;

    if (utf16bytes > 0) {
        /* Skip any remaining bytes */
        SKIP_BYTES(fd, utf16bytes);
    }
    return;
}

static int
asf_add_disc_meta (DB_playItem_t *it, const char *disc) {
    char *slash = strchr (disc, '/');
    if (slash) {
        // split into track/number
        *slash = 0;
        slash++;
        deadbeef->pl_add_meta (it, "numdiscs", slash);
    }
    deadbeef->pl_add_meta (it, "disc", disc);
    return 0;
}

static int asf_parse_header(DB_FILE *fd, asf_waveformatex_t* wfx, DB_playItem_t *it)
{
    asf_object_t current;
    asf_object_t header;
    uint64_t datalen = 0;
    int i = 0;
    int fileprop = 0;
    uint64_t play_duration = 0;
    uint16_t flags = 0;
    uint32_t subobjects = 0;
    uint8_t utf8buf[512];
    int codectype = 0;
    unsigned char id3buffer[2048];
    unsigned char *id3buf = id3buffer;
    int id3buf_remaining = sizeof(id3buffer);

    memset (&current, 0, sizeof (current));
    memset (&header, 0, sizeof (header));
    memset (&utf8buf, 0, sizeof (utf8buf));
    memset (&id3buffer, 0, sizeof (id3buffer));

    asf_read_object_header((asf_object_t *) &header, fd);

    DEBUGF("header.size=%d\n",(int)header.size);
    if (header.size < 30) {
        /* invalid size for header object */
        return ASF_ERROR_OBJECT_SIZE;
    }

    read_uint32le(fd, &subobjects);

    /* Two reserved bytes - do we need to read them? */
    SKIP_BYTES(fd, 2);

    DEBUGF("Read header - size=%d, subobjects=%d\n",(int)header.size, (int)subobjects);

    if (subobjects > 0) {
        header.datalen = header.size - 30;

        /* TODO: Check that we have datalen bytes left in the file */
        datalen = header.datalen;
        DEBUGF("datalen: %d\n", datalen);

        for (i=0; i<(int)subobjects; i++) {
            DEBUGF("Parsing header object %d - datalen=%d\n",i,(int)datalen);
            if (datalen < 24) {
                DEBUGF("not enough data for reading object\n");
                break;
            }

            asf_read_object_header(&current, fd);

            if (current.size > datalen || current.size < 24) {
                DEBUGF("invalid object size - current.size=%lld, datalen=%lld\n",current.size,datalen);
                break;
            }

            if (asf_guid_match(&current.guid, &asf_guid_file_properties)) {
                    if (current.size < 104)
                        return ASF_ERROR_OBJECT_SIZE;

                    if (fileprop) {
                        /* multiple file properties objects not allowed */
                        return ASF_ERROR_INVALID_OBJECT;
                    }

                    fileprop = 1;
                    
                    /* Get the number of logical packets - uint64_t at offset 32
                     * (Big endian byte order) */
                    SKIP_BYTES(fd, 32);
                    read_uint64le(fd, &wfx->numpackets);
                    read_uint64le(fd, &wfx->play_duration);
                    read_uint64le(fd, &wfx->send_duration);
                    read_uint64le(fd, &wfx->preroll);
                    read_uint32le(fd, &wfx->flags);

//                    DEBUGF("****** length = %lld\n", wfx->play_duration);

                    /* Read the packet size - uint32_t at offset 68 */
//                    SKIP_BYTES(fd, 20);
                    read_uint32le(fd, &wfx->packet_size);
                    read_uint32le(fd, &wfx->max_packet_size);

                    /* Skip bytes remaining in object */
                    SKIP_BYTES(fd, current.size - 24 - 72 - 4);
            } else if (asf_guid_match(&current.guid, &asf_guid_stream_properties)) {
                    guid_t guid;
                    uint32_t propdatalen;

                    if (current.size < 78)
                        return ASF_ERROR_OBJECT_SIZE;

#if 0
                    asf_byteio_getGUID(&guid, current->data);
                    datalen = asf_byteio_getDWLE(current->data + 40);
                    flags = asf_byteio_getWLE(current->data + 48);
#endif

                    asf_readGUID(fd, &guid);

                    SKIP_BYTES(fd, 24);
                    read_uint32le(fd, &propdatalen);
                    SKIP_BYTES(fd, 4);
                    read_uint16le(fd, &flags);

                    if (!asf_guid_match(&guid, &asf_guid_stream_type_audio)) {
                        DEBUGF("Found stream properties for non audio stream, skipping\n");
                        SKIP_BYTES(fd,current.size - 24 - 50);
                    } else if (wfx->audiostream == -1) {
                        SKIP_BYTES(fd, 4);
                        DEBUGF("Found stream properties for audio stream %d\n",flags&0x7f);

                        if (propdatalen < 18) {
                            return ASF_ERROR_INVALID_LENGTH;
                        }

#if 0
                        if (asf_byteio_getWLE(data + 16) > datalen - 16) {
                            return ASF_ERROR_INVALID_LENGTH;
                        }
#endif
                        read_uint16le(fd, &wfx->codec_id);
                        read_uint16le(fd, &wfx->channels);
                        read_uint32le(fd, &wfx->rate);
                        read_uint32le(fd, &wfx->bitrate);
                        wfx->bitrate *= 8;
                        read_uint16le(fd, &wfx->blockalign);
                        read_uint16le(fd, &wfx->bitspersample);
                        read_uint16le(fd, &wfx->datalen);

                        if (wfx->codec_id == ASF_CODEC_ID_WMAV1) {
                            deadbeef->fread(wfx->data, 4, 1, fd);
                            SKIP_BYTES(fd,current.size - 24 - 72 - 4);
                            wfx->audiostream = flags&0x7f;
                        } else if (wfx->codec_id == ASF_CODEC_ID_WMAV2) {
                            deadbeef->fread(wfx->data, 6, 1, fd);
                            SKIP_BYTES(fd,current.size - 24 - 72 - 6);
                            wfx->audiostream = flags&0x7f;
                        } else if (wfx->codec_id == ASF_CODEC_ID_WMAPRO) {
                            /* wma pro decoder needs the extra-data */
                            deadbeef->fread(wfx->data, wfx->datalen, 1, fd);
                            SKIP_BYTES(fd,current.size - 24 - 72 - wfx->datalen);
                            wfx->audiostream = flags&0x7f;
                            /* Correct codectype to redirect playback to the proper .codec */
                            codectype = AFMT_WMAPRO;
                        } else if (wfx->codec_id == ASF_CODEC_ID_WMAVOICE) {
                            deadbeef->fread(wfx->data, wfx->datalen, 1, fd);
                            SKIP_BYTES(fd,current.size - 24 - 72 - wfx->datalen);
                            wfx->audiostream = flags&0x7f;
                            codectype = AFMT_WMAVOICE;
                        } else {
                            trace("Unsupported WMA codec (Lossless, Voice, etc)\n");
                            SKIP_BYTES(fd,current.size - 24 - 72);
                        }
                    }
            } else if (it && asf_guid_match(&current.guid, &asf_guid_content_description)) {
                    /* Object contains five 16-bit string lengths, followed by the five strings:
                       title, artist, copyright, description, rating
                     */
                    uint16_t strlength[5];
                    int i;

                    DEBUGF("Found GUID_CONTENT_DESCRIPTION - size=%d\n",(int)(current.size - 24));

                    /* Read the 5 string lengths - number of bytes included trailing zero */
                    for (i=0; i<5; i++) {
                        read_uint16le(fd, &strlength[i]);
                        DEBUGF("strlength = %u\n",strlength[i]);
                    }

                    if (strlength[0] > 0) {  /* 0 - Title */
                        unsigned char *s = id3buf;
                        asf_utf16LEdecode(fd, strlength[0], &id3buf, &id3buf_remaining);
                        deadbeef->pl_append_meta (it, "title", s);
                    }

                    if (strlength[1] > 0) {  /* 1 - Artist */
                        unsigned char *s = id3buf;
                        asf_utf16LEdecode(fd, strlength[1], &id3buf, &id3buf_remaining);
                        deadbeef->pl_append_meta (it, "artist", s);
                    }

                    SKIP_BYTES(fd, strlength[2]); /* 2 - copyright */

                    if (strlength[3] > 0) {  /* 3 - description */
                        unsigned char *s = id3buf;
                        asf_utf16LEdecode(fd, strlength[3], &id3buf, &id3buf_remaining);
                        deadbeef->pl_append_meta (it, "comment", s);
                    }

                    SKIP_BYTES(fd, strlength[4]); /* 4 - rating */
            } else if (it && asf_guid_match(&current.guid, &asf_guid_extended_content_description)) {
                    uint16_t count;
                    int i;
                    int bytesleft = current.size - 24;
                    DEBUGF("Found GUID_EXTENDED_CONTENT_DESCRIPTION\n");

                    read_uint16le(fd, &count);
                    bytesleft -= 2;
                    DEBUGF("extended metadata count = %u\n",count);

                    for (i=0; i < count; i++) {
                        uint16_t length, type;
                        unsigned char* utf8 = utf8buf;
                        int utf8length = 512;

                        read_uint16le(fd, &length);
                        asf_utf16LEdecode(fd, length, &utf8, &utf8length);
                        bytesleft -= 2 + length;

                        read_uint16le(fd, &type);
                        read_uint16le(fd, &length);
                        trace ("ext md id: %s\n", utf8buf);

                        if (!strcmp("WM/TrackNumber",utf8buf) || !strcmp("WM/Track",utf8buf)) {
                            if (type == 0) {
                                unsigned char *s = id3buf;
                                asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                                deadbeef->pl_append_meta (it, "track", s);
                            } else if ((type >=2) && (type <= 5)) {
                                int tracknum = asf_intdecode(fd, type, length);
                                char n[100];
                                snprintf (n, sizeof (n), "%d", tracknum);
                                deadbeef->pl_append_meta (it, "track", n);
                            } else {
                                SKIP_BYTES(fd, length);
                            }
                        } else if (!strcmp("TotalTracks",utf8buf)) {
                            if (type == 0) {
                                unsigned char *s = id3buf;
                                asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                                deadbeef->pl_append_meta (it, "numtracks", s);
                            } else if ((type >=2) && (type <= 5)) {
                                int tracknum = asf_intdecode(fd, type, length);
                                char n[100];
                                snprintf (n, sizeof (n), "%d", tracknum);
                                deadbeef->pl_append_meta (it, "numtracks", n);
                            } else {
                                SKIP_BYTES(fd, length);
                            }
                        } else if (!strcmp("WM/PartOfSet",utf8buf)) {
                            if (type == 0) {
                                unsigned char *s = id3buf;
                                asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                                asf_add_disc_meta (it, s);
                            } else if ((type >=2) && (type <= 5)) {
                                int num = asf_intdecode(fd, type, length);
                                char n[100];
                                snprintf (n, sizeof (n), "%d", num);
                                deadbeef->pl_replace_meta (it, "disc", n);
                            } else {
                                SKIP_BYTES(fd, length);
                            }
                        } else if ((!strcmp("WM/Genre", utf8buf)) && (type == 0)) {
                            unsigned char *s = id3buf;
                            asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                            deadbeef->pl_append_meta (it, "genre", s);
                        } else if ((!strcmp("WM/AlbumTitle", utf8buf)) && (type == 0)) {
                            unsigned char *s = id3buf;
                            asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                            deadbeef->pl_append_meta (it, "album", s);
                        } else if ((!strcmp("WM/AlbumArtist", utf8buf)) && (type == 0)) {
                            unsigned char *s = id3buf;
                            asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                            deadbeef->pl_append_meta (it, "albumartist", s);
                        } else if ((!strcmp("WM/Composer", utf8buf)) && (type == 0)) {
                            unsigned char *s = id3buf;
                            asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                            deadbeef->pl_append_meta (it, "composer", s);
                        } else if ((!strcasecmp("foobar2000/cuesheet", utf8buf)) && (type == 0)) {
                            unsigned char *s = id3buf;
                            asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                            deadbeef->pl_append_meta (it, "cuesheet", s);
                        } else if (!strcmp("WM/Year", utf8buf)) {
                            if (type == 0) {
                                unsigned char *s = id3buf;
                                asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                                deadbeef->pl_append_meta (it, "year", s);
                            } else if ((type >=2) && (type <= 5)) {
                                int year = asf_intdecode(fd, type, length);
                                char n[100];
                                snprintf (n, sizeof (n), "%d", year);
                                deadbeef->pl_append_meta (it, "year", n);
                            } else {
                                SKIP_BYTES(fd, length);
                            }
                        } else if (!strcmp("WM/OriginalReleaseYear", utf8buf)) {
                            if (type == 0) {
                                unsigned char *s = id3buf;
                                asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                                deadbeef->pl_append_meta (it, "original_release_year", s);
                            } else if ((type >=2) && (type <= 5)) {
                                int year = asf_intdecode(fd, type, length);
                                char n[100];
                                snprintf (n, sizeof (n), "%d", year);
                                deadbeef->pl_append_meta (it, "original_release_year", n);
                            } else {
                                SKIP_BYTES(fd, length);
                            }
                        } else if (!strcmp("WM/OriginalReleaseTime", utf8buf)) {
                            if (type == 0) {
                                unsigned char *s = id3buf;
                                asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                                deadbeef->pl_append_meta (it, "original_release_time", s);
                            } else if ((type >=2) && (type <= 5)) {
                                int date = asf_intdecode(fd, type, length);
                                char n[100];
                                snprintf (n, sizeof (n), "%d", date);
                                deadbeef->pl_append_meta (it, "original_release_time", n);
                            } else {
                                SKIP_BYTES(fd, length);
                            }
                        } else if (!strncmp("replaygain_", utf8buf, 11)) {
                            char *s = utf8buf;
                            char *value = id3buf;
                            asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                            // parse_replaygain(utf8buf, value, id3);
                            if (!strncasecmp (s, "replaygain_album_gain", 21)) {
                                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (value));
                            }
                            else if (!strncasecmp (s, "replaygain_album_peak", 21)) {
                                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (value));
                            }
                            else if (!strncasecmp (s, "replaygain_track_gain", 21)) {
                                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (value));
                            }
                            else if (!strncasecmp (s, "replaygain_track_peak", 21)) {
                                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (value));
                            }
                        } else if (!strcmp("MusicBrainz/Track Id", utf8buf)) {
                            unsigned char *s = id3buf;
                            asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                            deadbeef->pl_append_meta (it, "MusicBrainzId", s);
#ifdef HAVE_ALBUMART
                        } else if (!strcmp("WM/Picture", utf8buf)) {
                            uint32_t datalength, strlength;
                            /* Expected is either "01 00 xx xx 03 yy yy yy yy" or
                             * "03 yy yy yy yy". xx is the size of the WM/Picture 
                             * container in bytes. yy equals the raw data length of 
                             * the embedded image. */
                            SKIP_BYTES(fd, -4);
                            deadbeef->fread(&type, 1, 1, fd);
                            if (type == 1) {
                                SKIP_BYTES(fd, 3);
                                deadbeef->fread(&type, 1, 1, fd);
                                /* In case the parsing will fail in the next step we 
                                 * might at least be able to skip the whole section. */
                                datalength = length - 1;
                            }
                            if (type == 3) {
                                /* Read the raw data length of the embedded image. */
                                read_uint32le(fd, &datalength);
                            
                                /* Reset utf8 buffer */
                                utf8 = utf8buf;
                                utf8length = 512;

                                /* Gather the album art format, this string has a
                                 * double zero-termination. */
                                asf_utf16LEdecode(fd, 32, &utf8, &utf8length);
                                strlength = (strlen(utf8buf) + 2) * 2;
                                SKIP_BYTES(fd, strlength-32);
                                if (!strcmp("image/jpeg", utf8buf)) {
                                    id3->albumart.type = AA_TYPE_JPG;
                                } else if (!strcmp("image/jpg", utf8buf)) {
                                    /* image/jpg is technically invalid,
                                     * but it does occur in the wild */
                                    id3->albumart.type = AA_TYPE_JPG;
                                } else if (!strcmp("image/png", utf8buf)) {
                                    id3->albumart.type = AA_TYPE_PNG;
                                } else {
                                    id3->albumart.type = AA_TYPE_UNKNOWN;
                                }

                                /* Set the album art size and position. */
                                if (id3->albumart.type != AA_TYPE_UNKNOWN) {
                                    id3->albumart.pos  = SKIP_BYTES(fd, 0);
                                    id3->albumart.size = datalength;
                                    id3->has_embedded_albumart = true;
                                }
                            }
                            
                            SKIP_BYTES(fd, datalength);
#endif
                        } else {
                            if (type == 0) { // FIXME: custom fields -- after others work
                                unsigned char *s = id3buf; // FIXME: skip empty
                                asf_utf16LEdecode(fd, length, &id3buf, &id3buf_remaining);
                                if (!strcmp (utf8buf, "MusicBrainz/Track Id")) {
                                    strcpy (utf8buf, "musicbrainz_trackid");
                                }
                                deadbeef->pl_append_meta (it, utf8buf, s);
                            }
                            else {
                                SKIP_BYTES(fd, length);
                            }
                        }
                        bytesleft -= 4 + length;
                    }

                    SKIP_BYTES(fd, bytesleft);
            } else if (asf_guid_match(&current.guid, &asf_guid_content_encryption)
                || asf_guid_match(&current.guid, &asf_guid_extended_content_encryption)) {
                DEBUGF("File is encrypted\n");
                return ASF_ERROR_ENCRYPTED;
            } else {
                DEBUGF("Skipping %d bytes of object\n",(int)(current.size - 24));
                SKIP_BYTES(fd,current.size - 24);
            }

            DEBUGF("Parsed object - size = %d\n",(int)current.size);
            datalen -= current.size;
        }

        if (i != (int)subobjects || datalen != 0) {
            DEBUGF("header data doesn't match given subobject count\n");
            return ASF_ERROR_INVALID_VALUE;
        }

        DEBUGF("%d subobjects read successfully\n", i);
    }

#if 0
    tmp = asf_parse_header_validate(file, &header);
    if (tmp < 0) {
        /* header read ok but doesn't validate correctly */
        return tmp;
    }
#endif

    DEBUGF("header validated correctly\n");

    return 0;
}

int get_asf_metadata(DB_FILE *fd, DB_playItem_t *it, asf_waveformatex_t *wfx, int64_t *first_frame_offset)
{
    int res;
    asf_object_t obj;

    wfx->audiostream = -1;

    res = asf_parse_header(fd, wfx, it);

    if (res < 0) {
        DEBUGF("ASF: parsing error - %d\n",res);
        return 0;
    }

    if (wfx->audiostream == -1) {
        DEBUGF("ASF: No WMA streams found\n");
        return 0;
    }

    asf_read_object_header(&obj, fd);

    if (!asf_guid_match(&obj.guid, &asf_guid_data)) {
        DEBUGF("ASF: No data object found\n");
        return 0;
    }

    /* Store the current file position - no need to parse the header
       again in the codec.  The +26 skips the rest of the data object
       header.
     */
    *first_frame_offset = deadbeef->ftell(fd) + 26;

    if (!fd->vfs->is_streaming ()) {
        // check if we got a fragment
        if (0 != deadbeef->fseek (fd, 26, SEEK_CUR)) {
            DEBUGF("ASF: failed to seek to 1st frame\n");
            return 0;
        }
        int duration = 0;
        int time = asf_get_timestamp(&duration, fd);
        if (time != 0) {
            wfx->first_frame_timestamp = time;
            // need to scan entire file to find out the duration
            int pmn = (int)(time * 0.001f / 60);
            int psc = time * 0.001f - pmn * 60;
            trace ("wma: the file is a fragment, start time: %d:%02d\n", pmn, psc);
            trace ("play_duration %lld, numpackets %lld, packet_size %d\n",  wfx->play_duration, wfx->numpackets, wfx->packet_size);
        }
        if (wfx->play_duration == 0) {
            wfx->preroll = 0;
            wfx->numpackets = 1;
            // calc duration from packets (scan the file)
            wfx->play_duration += duration * 10000;
            while (0 == deadbeef->fseek (fd, *first_frame_offset + wfx->packet_size * wfx->numpackets, SEEK_SET)) {
                time = asf_get_timestamp(&duration, fd);
                if (time < 0) {
                    break;
                }
                wfx->play_duration += duration * 10000;
                wfx->numpackets++;
            }
        }
    }

    return 1;
}
