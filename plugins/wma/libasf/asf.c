// this file is a modified version for deadbeef player
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * ASF parsing code based on libasf by Juho Vähä-Herttua
 * http://code.google.com/p/libasf/  libasf itself was based on the ASF
 * parser in VLC - http://www.videolan.org/
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
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "asf.h"
#include <deadbeef/deadbeef.h>
extern DB_functions_t *deadbeef;
//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)
#define DEBUGF trace

#define SKIP_BYTES(fd,x) {\
    if (x > 0) {\
        char buf[x];\
        deadbeef->fread (buf, x, 1, fd);\
    }\
}

/* Read an unaligned 32-bit little endian long from buffer. */
static unsigned long get_long_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* Read an unaligned 16-bit little endian short from buffer. */
static unsigned short get_short_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8);
}

#define GETLEN2b(bits) (((bits) == 0x03) ? 4 : bits)

#define GETVALUE2b(bits, data) \
        (((bits) != 0x03) ? ((bits) != 0x02) ? ((bits) != 0x01) ? \
         0 : *(data) : get_short_le(data) : get_long_le(data))

int asf_read_packet(uint8_t** audiobuf, int* audiobufsize, int* packetlength, 
                    asf_waveformatex_t* wfx, DB_FILE *fp)
{
    uint8_t *audiobuf_mem = *audiobuf;
    uint8_t tmp8, packet_flags, packet_property;
    int stream_id;
    int ec_length, opaque_data, ec_length_type;
    int datalen;
    uint8_t data[18];
    uint8_t* datap;
    uint32_t length;
    uint32_t padding_length;
    /* rockbox: comment 'set but unused' variables
    uint32_t send_time;
    uint16_t duration;
    uint32_t media_object_number;
    uint32_t media_object_offset;
    */
    uint16_t payload_count;
    int payload_length_type;
    uint32_t payload_hdrlen;
    int payload_datalen;
    int multiple;
    uint32_t replicated_length;
    uint32_t bytesread = 0;
    uint8_t* buf;
    size_t bufsize;
    int i;
    /*DEBUGF("Reading new packet at %d bytes ", (int)ci->curpos);*/

    if (deadbeef->fread (&tmp8, 1, 1, fp) == 0) {
        return ASF_ERROR_EOF;
    }
    bytesread++;

    /* TODO: We need a better way to detect endofstream */
    if (tmp8 != 0x82) {
    DEBUGF("Read failed:  packet did not sync\n");
    return -1;
    }


    if (tmp8 & 0x80) {
       ec_length = tmp8 & 0x0f;
       opaque_data = (tmp8 >> 4) & 0x01;
       ec_length_type = (tmp8 >> 5) & 0x03;

       if (ec_length_type != 0x00 || opaque_data != 0 || ec_length != 0x02) {
            DEBUGF("incorrect error correction flags\n");
            return ASF_ERROR_INVALID_VALUE;
       }

       /* Skip ec_data */
       SKIP_BYTES(fp, ec_length);
       bytesread += ec_length;
    } else {
        ec_length = 0;
    }

    if (deadbeef->fread(&packet_flags, 1, 1, fp) == 0) { return ASF_ERROR_EOF; }
    if (deadbeef->fread(&packet_property, 1, 1, fp) == 0) { return ASF_ERROR_EOF; }
    bytesread += 2;

    datalen = GETLEN2b((packet_flags >> 1) & 0x03) +
              GETLEN2b((packet_flags >> 3) & 0x03) +
              GETLEN2b((packet_flags >> 5) & 0x03) + 6;

#if 0
    if (datalen > sizeof(data)) {
        DEBUGF("Unexpectedly long datalen in data - %d\n",datalen);
        return ASF_ERROR_OUTOFMEM;
    }
#endif

    if (deadbeef->fread (data, datalen, 1, fp) == 0) {
        return ASF_ERROR_EOF;
    }

    bytesread += datalen;

    datap = data;
    length = GETVALUE2b((packet_flags >> 5) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 5) & 0x03);
    /* sequence value is not used */
    GETVALUE2b((packet_flags >> 1) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 1) & 0x03);
    padding_length = GETVALUE2b((packet_flags >> 3) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 3) & 0x03);
    /* send_time = get_long_le(datap); */
    datap += 4;
    /* duration = get_short_le(datap); */
    datap += 2;
    /*DEBUGF("and duration %d ms\n", duration);*/

    /* this is really idiotic, packet length can (and often will) be
     * undefined and we just have to use the header packet size as the size
     * value */
    if (!((packet_flags >> 5) & 0x03)) {
         length = wfx->packet_size;
    }

    /* this is also really idiotic, if packet length is smaller than packet
     * size, we need to manually add the additional bytes into padding length
     */
    if (length < wfx->packet_size) {
        padding_length += wfx->packet_size - length;
        length = wfx->packet_size;
    }

    if (length > wfx->packet_size) {
        DEBUGF("packet with too big length value\n");
        return ASF_ERROR_INVALID_LENGTH;
    }

    /* check if we have multiple payloads */
    if (packet_flags & 0x01) {
        if (deadbeef->fread(&tmp8, 1, 1, fp) == 0) {
            return ASF_ERROR_EOF;
        }
        payload_count = tmp8 & 0x3f;
        payload_length_type = (tmp8 >> 6) & 0x03;
        bytesread++;
    } else {
        payload_count = 1;
        payload_length_type = 0x02; /* not used */
    }

    if (length < bytesread) {
        DEBUGF("header exceeded packet size, invalid file - length=%d, bytesread=%d\n",(int)length,(int)bytesread);
        /* FIXME: should this be checked earlier? */
        return ASF_ERROR_INVALID_LENGTH;
    }


    /* We now parse the individual payloads, and move all payloads
       belonging to our audio stream to a contiguous block, starting at
       the location of the first payload.
    */

    *audiobuf = NULL;
    *audiobufsize = 0;
    *packetlength = length - bytesread;

    buf = audiobuf_mem;
    int64_t ret = deadbeef->fread (buf, 1, length-bytesread, fp);
#define MIN(x,y) ((x)<(y)) ? (x) : (y)
    if (ret >= 0) {
        bufsize = MIN((size_t)ret, length);
    }
    else {
        bufsize = 0;
    }
    if (bufsize == 0) {
        return -1;
    }

    bufsize = length;
    datap = buf;

#define ASF_MAX_REQUEST (1L<<15) /* 32KB */
    if (bufsize != length && length >= ASF_MAX_REQUEST) {
        /* This should only happen with packets larger than 32KB (the
           guard buffer size).  All the streams I've seen have
           relatively small packets less than about 8KB), but I don't
           know what is expected.
        */
        DEBUGF("Could not read packet (requested %d bytes, received %d), curpos=%d, aborting\n",
               (int)length,(int)bufsize,(int)deadbeef->ftell(fp));
        return -1;
    }

    for (i=0; i<payload_count; i++) {
        stream_id = datap[0]&0x7f;
        datap++;
        bytesread++;

        payload_hdrlen = GETLEN2b(packet_property & 0x03) +
                         GETLEN2b((packet_property >> 2) & 0x03) +
                         GETLEN2b((packet_property >> 4) & 0x03);

        //DEBUGF("payload_hdrlen = %d\n",payload_hdrlen);
#if 0
        /* TODO */
        if (payload_hdrlen > size) {
            return ASF_ERROR_INVALID_LENGTH;
        }
#endif
        if (payload_hdrlen > sizeof(data)) {
            DEBUGF("Unexpectedly long datalen in data - %d\n",datalen);
            return ASF_ERROR_OUTOFMEM;
        }

        bytesread += payload_hdrlen;
        /* media_object_number = GETVALUE2b((packet_property >> 4) & 0x03, datap); */
        datap += GETLEN2b((packet_property >> 4) & 0x03);
        /* media_object_offset = GETVALUE2b((packet_property >> 2) & 0x03, datap); */
        datap += GETLEN2b((packet_property >> 2) & 0x03);
        replicated_length = GETVALUE2b(packet_property & 0x03, datap);
        datap += GETLEN2b(packet_property & 0x03);

        /* TODO: Validate replicated_length */
        /* TODO: Is the content of this important for us? */
        datap += replicated_length;
        bytesread += replicated_length;

        multiple = packet_flags & 0x01;


        if (multiple) {
            int x;

            x = GETLEN2b(payload_length_type);

            if (x != 2) {
                /* in multiple payloads datalen should be a word */
                return ASF_ERROR_INVALID_VALUE;
            }

#if 0
            if (skip + tmp > datalen) {
                /* not enough data */
                return ASF_ERROR_INVALID_LENGTH;
            }
#endif
            payload_datalen = GETVALUE2b(payload_length_type, datap);
            datap += x;
            bytesread += x;
        } else {
            payload_datalen = length - bytesread - padding_length;
        }

        if (replicated_length==1)
            datap++;

        if (stream_id == wfx->audiostream)
        {
            if (*audiobuf == NULL) {
                /* The first payload can stay where it is */
                *audiobuf = datap;
                *audiobufsize = payload_datalen;
            } else {
                /* The second and subsequent payloads in this packet
                   that belong to the audio stream need to be moved to be
                   contiguous with the first payload.
                */
                memmove(*audiobuf + *audiobufsize, datap, payload_datalen);
                *audiobufsize += payload_datalen;
            }
        }
        datap += payload_datalen;
        bytesread += payload_datalen;
    }

    if (*audiobuf != NULL)
        return 1;
    else
        return 0;
}


int asf_get_timestamp(int *duration, DB_FILE *fp)
{
    uint8_t tmp8, packet_flags, packet_property;
    int ec_length, opaque_data, ec_length_type;
    int datalen;
    uint8_t data[18];
    uint8_t* datap;
    /* rockbox: comment 'set but unused' variables
    uint32_t length;
    uint32_t padding_length;
    */
    uint32_t send_time;
    static int packet_count = 0;

    uint32_t bytesread = 0;
    packet_count++;
    if (deadbeef->fread(&tmp8, 1, 1, fp) == 0) {
        DEBUGF("ASF ERROR (EOF?)\n");
        return ASF_ERROR_EOF;
    }
    bytesread++;

    /* TODO: We need a better way to detect endofstream */
    if (tmp8 != 0x82) {
        DEBUGF("Get timestamp:  Detected end of stream\n");
        return ASF_ERROR_EOF;
    }


    if (tmp8 & 0x80) {
        ec_length = tmp8 & 0x0f;
        opaque_data = (tmp8 >> 4) & 0x01;
        ec_length_type = (tmp8 >> 5) & 0x03;

        if (ec_length_type != 0x00 || opaque_data != 0 || ec_length != 0x02) {
             DEBUGF("incorrect error correction flags\n");
             return ASF_ERROR_INVALID_VALUE;
        }

        /* Skip ec_data */
        SKIP_BYTES(fp, ec_length);
        bytesread += ec_length;
    } else {
        ec_length = 0;
    }

    if (deadbeef->fread(&packet_flags, 1, 1, fp) == 0) {
        DEBUGF("Detected end of stream 2\n");
        return ASF_ERROR_EOF;
    }

    if (deadbeef->fread(&packet_property, 1, 1, fp) == 0) {
        DEBUGF("Detected end of stream3\n");
        return ASF_ERROR_EOF;
    }
    bytesread += 2;

    datalen = GETLEN2b((packet_flags >> 1) & 0x03) +
              GETLEN2b((packet_flags >> 3) & 0x03) +
              GETLEN2b((packet_flags >> 5) & 0x03) + 6;

    if (deadbeef->fread(data, datalen, 1, fp) == 0) {
        DEBUGF("Detected end of stream4\n");
        return ASF_ERROR_EOF;
    }

    bytesread += datalen;

    datap = data;
    /* length = GETVALUE2b((packet_flags >> 5) & 0x03, datap); */
    datap += GETLEN2b((packet_flags >> 5) & 0x03);

    /* sequence value is not used */
    GETVALUE2b((packet_flags >> 1) & 0x03, datap);
    datap += GETLEN2b((packet_flags >> 1) & 0x03);
    /* padding_length = GETVALUE2b((packet_flags >> 3) & 0x03, datap); */
    datap += GETLEN2b((packet_flags >> 3) & 0x03);
    send_time = get_long_le(datap);
    datap += 4;
    *duration = get_short_le(datap);

    /*the asf_get_timestamp function advances us 12-13 bytes past the packet start,
      need to undo this here so that we stay synced with the packet*/
    deadbeef->fseek (fp, -bytesread, SEEK_CUR);

    return send_time;
}

/*entry point for seeks*/
int asf_seek(int ms, asf_waveformatex_t* wfx, DB_FILE *fp, int64_t first_frame_offset, int *skip_ms)
{
    int time, duration=0, delta, temp, count=0;

    /*estimate packet number from bitrate*/
    int64_t datasize = deadbeef->fgetlength (fp) - first_frame_offset;

    int initial_packet = (deadbeef->ftell (fp) - first_frame_offset) / wfx->packet_size;
    int packet_num = (((int64_t)ms)*(wfx->bitrate>>3))/wfx->packet_size/1000;
    int last_packet = datasize / wfx->packet_size;

    if (packet_num > last_packet) {
        packet_num = last_packet;
    }

    /*calculate byte address of the start of that packet*/
    int64_t packet_offset = packet_num*wfx->packet_size;
    trace ("initial_packet: %d\n", initial_packet);
    trace ("packet_num: %d\n", packet_num);
    trace ("last_packet: %d\n", last_packet);
    trace ("packet_offset: %lld\n", packet_offset);

    /*seek to estimated packet*/
    deadbeef->fseek (fp, first_frame_offset+packet_offset, SEEK_SET);
    temp = ms;
    while (1)
    {
        /*for very large files it can be difficult and unimportant to find the exact packet*/
        count++;

        /*check the time stamp of our packet*/
        int64_t pos = deadbeef->ftell (fp);
        time = asf_get_timestamp(&duration, fp) - wfx->first_frame_timestamp;
//        DEBUGF("time %d ms with duration %d\n", time, duration);

        if (time < 0) {
            /*unknown error, try to recover*/
            DEBUGF("UKNOWN SEEK ERROR\n");
            deadbeef->fseek (fp, first_frame_offset+initial_packet*wfx->packet_size, SEEK_SET);
            *skip_ms = 0;
            /*seek failed so return time stamp of the initial packet*/
            return -1;
        }

        DEBUGF("time: %d, duration: %d (ms: %d)\n", time, duration, ms);
        if (time <= ms && (time+duration>=ms || count > 10)) {
            int mn = (int)(ms * 0.001f / 60);
            int sc = ms * 0.001f - mn * 60;
            int pmn = (int)(time * 0.001f / 60);
            int psc = time * 0.001f - pmn * 60;
            DEBUGF("Found our packet! Now at %d packet, time %d (%d:%d), ms %d (%d:%d), duration %d, count %d\n", packet_num, time, pmn, psc, ms, mn, sc, duration, count);
            deadbeef->fseek (fp, pos, SEEK_SET);
            *skip_ms = ms > time ? ms - time : 0;
            return time;
        } else {
            /*seek again*/
            delta = ms-time;
            /*estimate new packet number from bitrate and our current position*/
            temp += delta;
            packet_num = ((temp/1000)*(wfx->bitrate>>3) - (wfx->packet_size>>1))/wfx->packet_size;  //round down!
            packet_offset = packet_num*wfx->packet_size;
            deadbeef->fseek (fp, first_frame_offset+packet_offset, SEEK_SET);
        }
    }
}

