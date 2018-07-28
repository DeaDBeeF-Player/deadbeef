/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Alexey Yakovenko and other contributors

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

#include "mp3parser.h"
#include <string.h>
#include <assert.h>

extern DB_functions_t *deadbeef;

#define MIN_PACKET_SAMPLES 384
#define MAX_PACKET_SAMPLES 1152
#define MAX_BITRATE 320
#define MIN_BITRATE 8
#define MIN_SAMPLERATE 8000
#define MAX_SAMPLERATE 48000
#define MIN_PACKET_LENGTH (MIN_PACKET_SAMPLES / 8 * MIN_BITRATE*1000 / MAX_SAMPLERATE)
#define MAX_PACKET_LENGTH (MAX_PACKET_SAMPLES / 8 * MAX_BITRATE*1000 / MIN_SAMPLERATE)
#define MAX_INVALID_BYTES 100000

static const int vertbl[] = {3, -1, 2, 1}; // 3 is 2.5
static const int ltbl[] = { -1, 3, 2, 1 };

// samplerate
static const int srtable[3][4] = {
    {44100, 48000, 32000, -1},
    {22050, 24000, 16000, -1},
    {11025, 12000, 8000, -1},
};

static const int chantbl[4] = { 2, 2, 2, 1 };

// bitrate
static const int brtable[5][16] = {
    { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 },
    { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1 },
    { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1 },
    { 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, -1 },
    { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 }
};

// xing header flags
#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

static int
_parse_packet (mp3packet_t * restrict packet, uint8_t * restrict fb) {
    memset (packet, 0, sizeof (mp3packet_t));
    uint32_t hdr;
    uint8_t sync;

    sync = fb[0];
    if (sync != 0xff) {
        return -1;
    }
    else {
        // 2nd sync byte
        sync = fb[1];
        if ((sync >> 5) != 7) {
            return -1;
        }
    }
    // found frame
    hdr = (0xff<<24) | (sync << 16);
    sync = fb[2];
    hdr |= sync << 8;
    sync = fb[3];
    hdr |= sync;

    // parse header

    // sync bits
    int usync = hdr & 0xffe00000;
    if (usync != 0xffe00000) {
        fprintf (stderr, "fatal error: mp3 header parser is broken\n");
    }

    // mpeg version
    packet->ver = (hdr & (3<<19)) >> 19;
    packet->ver = vertbl[packet->ver];
    if (packet->ver < 0) {
        //        trace ("frame %d bad mpeg version %d\n", nframe, (hdr & (3<<19)) >> 19);
        return -1;
    }

    // layer info
    packet->layer = (hdr & (3<<17)) >> 17;
    packet->layer = ltbl[packet->layer];
    if (packet->layer < 0) {
        //        trace ("frame %d bad layer %d\n", nframe, (hdr & (3<<17)) >> 17);
        return -1;
    }

    // protection bit (crc)
    //        int prot = (hdr & (1<<16)) >> 16;

    packet->bitrate = (hdr & (0x0f<<12)) >> 12;
    int idx = 0;
    if (packet->ver == 1) {
        idx = packet->layer - 1;
    }
    else {
        idx = packet->layer == 1 ? 3 : 4;
    }
    packet->bitrate = brtable[idx][packet->bitrate];
    if (packet->bitrate <= 0) {
        //        trace ("frame %d bad bitrate %d\n", nframe, (hdr & (0x0f<<12)) >> 12);
        return -1;
    }

    packet->samplerate = (hdr & (0x03<<10))>>10;
    packet->samplerate = srtable[packet->ver-1][packet->samplerate];
    if (packet->samplerate < 0) {
        //        trace ("frame %d bad samplerate %d\n", nframe, (hdr & (0x03<<10))>>10);
        return -1;
    }

    // padding
    int padding = (hdr & (0x1 << 9)) >> 9;

    packet->nchannels = (hdr & (0x3 << 6)) >> 6;
    packet->nchannels = chantbl[packet->nchannels];

    // check if channel/bitrate combination is valid for layer2
    if (packet->layer == 2) {
        if ((packet->bitrate <= 56 || packet->bitrate == 80) && packet->nchannels != 1) {
            //            trace ("mp3: bad frame %d: layer %d, channels %d, bitrate %d\n", nframe, layer, nchannels, bitrate);
            return -1;
        }
        if (packet->bitrate >= 224 && packet->nchannels == 1) {
            //            trace ("mp3: bad frame %d: layer %d, channels %d, bitrate %d\n", nframe, layer, nchannels, bitrate);
            return -1;
        }
    }

    // get side info ptr
    //        uint8_t *si = fb + 4;
    //        if (prot) {
    //            si += 2;
    //        }
    //        int data_ptr = ((uint16_t)si[1]) | (((uint16_t)si[1]&0)<<8);

    // calc packet length, number of samples in a frame
    // packetlength
    packet->packetlength = 0;
    packet->bitrate *= 1000;
    packet->samples_per_frame = 0;
    if (packet->samplerate > 0 && packet->bitrate > 0) {
        if (packet->layer == 1) {
            packet->samples_per_frame = 384;
        }
        else if (packet->layer == 2) {
            packet->samples_per_frame = 1152;
        }
        else if (packet->layer == 3) {
            if (packet->ver == 1) {
                packet->samples_per_frame = 1152;
            }
            else {
                packet->samples_per_frame = 576;
            }
        }
        packet->packetlength = packet->samples_per_frame / 8 * packet->bitrate / packet->samplerate + padding;
    }
    else {
        return -1;
    }
    return packet->packetlength;
}

// FIXME: make sure info->packet_offs is set

static uint32_t
extract_i32 (unsigned char *buf)
{
    uint32_t x;
    // big endian extract

    x = buf[0];
    x <<= 8;
    x |= buf[1];
    x <<= 8;
    x |= buf[2];
    x <<= 8;
    x |= buf[3];

    return x;
}

static int
_check_xing_header (mp3info_t *info, mp3packet_t *packet, uint8_t *data, int datasize) {
    const char cookieXing[] = "Xing";
    const char cookieInfo[] = "Info";

    // ignore mpeg version, and try both 17 and 32 byte offsets
    uint8_t *magic = data + 17;

    int havecookie = !memcmp (cookieXing, magic, 4) || !memcmp (cookieInfo, magic, 4);

    if (!havecookie) {
        magic = data + 32;
        havecookie = !memcmp (cookieXing, magic, 4) || !memcmp (cookieInfo, magic, 4);;
    }

    if (!havecookie) {
        return -1;
    }

    data = magic + 4;

    // FIXME: the right place to assign successful result?
    info->packet_offs = packet->offs+packet->packetlength;

    // read flags
    uint32_t flags;
    flags = extract_i32 (data);
    data += 4;

    if (flags & FRAMES_FLAG) {
        // read number of frames
        if (packet->samples_per_frame <= 0) { // FIXME: this should be invalid packet earlier on
            return -1;
        }
        uint32_t nframes = extract_i32 (data);
        info->totalsamples = nframes * packet->samples_per_frame;
        info->have_duration = 1;
        data += 4;
    }
    if (flags & BYTES_FLAG) {
        data += 4;
    }
    if (flags & TOC_FLAG) {
        data += 100;
    }
    if (flags & VBR_SCALE_FLAG) {
        data += 4;
    }
    // lame header
    int lame_header = 0;
    if (!memcmp (data, "LAME", 4)) {
        lame_header = 1;
    }
    data += 4;

    data += 5; // what is this?

    uint8_t rev = *data++;

    switch (rev & 0x0f) {
        case XING_CBR ... XING_ABR2:
            info->vbr_type = rev & 0x0f;
            break;
    }
    if (lame_header) {
        data++; //uint8_t lpf = *data++;

        //3 floats: replay gain

        data += 4; // float rg_peaksignalamp = extract_f32 (buf);

        data += 2; // uint16_t rg_radio = extract_i16 (buf);

        data += 2; // uint16_t rg_audiophile = extract_i16 (buf);

        // skip
        data += 2;

        info->delay = (((uint32_t)data[0]) << 4) | ((((uint32_t)data[1]) & 0xf0)>>4);
        info->padding = ((((uint32_t)data[1])&0x0f)<<8) | ((uint32_t)data[2]);
        data += 3;

        // skip
        data++;

        data++; // uint8_t mp3gain = *data++;

        // lame preset nr
        info->lamepreset = data[1] | (data[0]<<8);
        data += 2;

        // musiclen
        info->lame_musiclength = extract_i32 (data);
        data += 4;
    }

    return 0;
}

// returns 0 to continue, 1 to stop (seek position reached, or another reason to stop scanning)
static int
_process_packet (mp3info_t *info, mp3packet_t *packet, int64_t seek_to_sample) {
    if (info->vbr_type != DETECTED_VBR
        && (!info->have_xing_header || !info->vbr_type)
        && info->prev_packet.bitrate
        && info->prev_packet.bitrate != packet->bitrate) {
        info->vbr_type = DETECTED_VBR;
    }

    info->valid_packets++;

    // update stream parameters, only when sample!=0 or 1st frame
    if (seek_to_sample != 0 || info->npackets == 0)
    {
        // don't get parameters from frames coming after any bad frame
        int ch = info->ref_packet.nchannels;
        memcpy (&info->ref_packet, packet, sizeof (mp3packet_t));
        if (info->ref_packet.nchannels < ch) {
            info->ref_packet.nchannels = ch;
        }
    }

    info->lastpacket_valid = 1;

    if (seek_to_sample == 0) {
        // update averages, interrupt scan on frame #100 <-- FIXME
        // calculating apx duration based on 1st 100 frames
        info->avg_packetlength += packet->packetlength;
        info->avg_samplerate += packet->samplerate;
        info->avg_samples_per_frame += packet->samples_per_frame;

        int ch = info->ref_packet.nchannels;
        memcpy (&info->ref_packet, packet, sizeof (mp3packet_t));
        if (info->ref_packet.nchannels < ch) {
            info->ref_packet.nchannels = ch;
        }

#if 0 // FIXME
        if (!info->is_seekable) {
            if (info->valid_frames >= 20) {
                deadbeef->fseek (buffer->file, valid_frame_pos, SEEK_SET);
                buffer->duration = -1;
                buffer->totalsamples = 0;
                return 0;
            }
        }
        else if (valid_frames >= 100)
#endif
        {
            return 1;
        }
    }

    if (seek_to_sample > 0) {
        info->pcmsample += packet->samples_per_frame;
    }
    if (!info->have_duration) {
        info->totalsamples += packet->samples_per_frame;
    }
    info->npackets++;
    return 0;
}

int
mp3_parse_file (mp3info_t *info, uint32_t flags, DB_FILE *fp, int64_t fsize, int startoffs, int endoffs, int64_t seek_to_sample) {
    memset (info, 0, sizeof (mp3info_t));

    if (seek_to_sample > 0) {
        // add 9 extra packets to fill bit-reservoir
        seek_to_sample -= MAX_PACKET_SAMPLES*10;
        if (seek_to_sample < 0) {
            seek_to_sample = 0;
        }
    }

    int err = -1;

    deadbeef->fseek (fp, startoffs, SEEK_SET);

    // FIXME: radio
    fsize -= startoffs + endoffs;

    int bufsize = 8*1024;
    assert (bufsize >= MAX_PACKET_LENGTH);
    if (bufsize > fsize) {
        bufsize = (int)fsize;
    }

    if (bufsize < MIN_PACKET_LENGTH) {
        return -1; // file too small
    }

    uint8_t *buffer = malloc (bufsize);
    if (!buffer) {
        return -1;
    }

    mp3packet_t packet;

    int remaining = 0;
    int64_t offs = startoffs;
    int64_t fileoffs = 0;

    int eof = 0;

    while (fsize > 0) {
        int64_t readsize = bufsize-remaining;
        if (fileoffs + readsize >= fsize) {
            readsize = fsize - fileoffs;
            eof = 1;
        }

        if (readsize <= 0) {
            break;
        }

        assert (remaining >= 0);
        assert (remaining <= bufsize);
        assert (readsize <= bufsize-remaining);
        if (deadbeef->fread (buffer+remaining, 1, readsize, fp) != readsize) {
            goto error;
        }
        fileoffs += readsize;

        remaining = (int)(remaining + readsize);

        uint8_t *bufptr = buffer;

        while (remaining >= MAX_PACKET_LENGTH || (eof && remaining >= MIN_PACKET_LENGTH)) {
            int res = _parse_packet (&packet, bufptr);
            if (res < 0) {
                // bail if a valid packet could not be found at the start of stream
                if (!info->valid_packets && offs - startoffs > MAX_INVALID_BYTES) {
                    goto error;
                }

                // bad packet in the stream, try to resync
                memset (&info->prev_packet, 0, sizeof (mp3packet_t));
                info->packet_offs = -1;
                info->lastpacket_valid = 0;
                if (info->pcmsample == 0) {
                    info->valid_packets = 0;
                }

                offs++;

                remaining--;
                bufptr++;
                continue;
            }
            else {
                // EOF
                if (fsize >= 0 && offs + res > fsize) {
                    if (info->valid_packets) {
                        goto end;
                    }
                    else {
                        goto error;
                    }
                }

                packet.offs = offs;

                if (!info->packet_offs || seek_to_sample >= 0) {
                    info->packet_offs = offs;
                }

                // try to read xing/info tag (only on initial scans)
                int got_xing = 0;
                if (!info->have_xing_header && !info->checked_xing_header)
                {
                    // need whole packet for checking xing!

                    info->checked_xing_header = 1;
                    int xingres = _check_xing_header (info, &packet, bufptr+4, remaining-4);
                    if (!xingres) {
                        got_xing = 1;
                        info->have_xing_header = 1;
                        // trust the xing header -- even if requested to scan for precise duration
                        // parameters have been discovered from xing header, no need to continue
                        memcpy (&info->ref_packet, &packet, sizeof (mp3packet_t));

                        if (seek_to_sample > 0) {
                            assert (remaining >= res);
                            remaining -= res;
                            bufptr += res;
                            offs += res;
                            continue;
                        }
                        else if (!(flags & MP3_PARSE_FULLSCAN)) {
                            goto end;
                        }
                        else {
                            // reset counters for full scan
                            info->pcmsample = 0;
                            info->delay = 0;
                            info->padding = 0;
                            memset (&info->ref_packet, 0, sizeof (mp3packet_t));
                        }
                    }
                }

                if (!got_xing) {
                    // interrupt if the current packet contains the sample being seeked to
                    if (seek_to_sample > 0 && info->pcmsample+packet.samples_per_frame >= seek_to_sample) {
                        goto end;
                    }

                    if (_process_packet (info, &packet, seek_to_sample) > 0) {
                        goto end;
                    }
                    printf ("frame %lld, sr %d, totalsamples %lld\n", info->npackets-1, packet.samplerate, info->totalsamples);
                    memcpy (&info->prev_packet, &packet, sizeof (packet));
                }

                assert (remaining >= res);
                remaining -= res;
                bufptr += res;
                offs += res;
            }
        }
        if (remaining > 0) {
            memmove (buffer, bufptr, remaining);
        }
    }

end:

    info->delay += 529;
    if (info->padding > 529) {
        info->padding -= 529;
    }

    if (seek_to_sample == -1) {
        info->have_duration = 1;
    }
    err = 0;
error:
    if (buffer) {
        free (buffer);
        buffer = NULL;
    }

    return err;
}
