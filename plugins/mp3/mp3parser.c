/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Oleksiy Yakovenko and other contributors

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
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

//#define PERFORMANCE_STATS 1

extern DB_functions_t *deadbeef;

#define MIN_PACKET_SAMPLES 384
#define MAX_PACKET_SAMPLES 1152
#define MAX_BITRATE 320
#define MIN_BITRATE 8
#define MIN_SAMPLERATE 8000
#define MAX_SAMPLERATE 48000
#define MIN_PACKET_LENGTH (MIN_PACKET_SAMPLES / 8 * MIN_BITRATE*1000 / MAX_SAMPLERATE)
#define MAX_PACKET_LENGTH 1441
#define MAX_INVALID_BYTES 1000000
#define MAX_INVALID_BYTES_STREAM 1000
#define MAX_FREEFORMAT_PACKETS 10

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

// -1 recoverable failure
// -2 freeformat (bitrate=0) stream detected
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
    hdr = 0xff000000 | (sync << 16);
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
    if (packet->bitrate == 0) {
        return -2; // freeformat not supported
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
        else {
            return -1;
        }
        packet->packetlength = packet->samples_per_frame / 8 * packet->bitrate / packet->samplerate + padding;
        if (packet->packetlength > MAX_PACKET_LENGTH) {
            return -1;
        }
    }
    else {
        return -1;
    }

    return packet->packetlength;
}

#define READ_UINT8() ({if (buffer_size < 1) return -1; uint8_t _temp8 = *buffer; buffer++; buffer_size--; _temp8;})
#define READ_UINT32() ({if (buffer_size < 4) return -1;  uint32_t _temp32 = (uint32_t)buffer[3] | ((uint32_t)buffer[2]<<8) | ((uint32_t)buffer[1]<<16) | ((uint32_t)buffer[0]<<24); buffer+=4; buffer_size-=4; _temp32;})

static int
_check_xing_header (mp3info_t *info, mp3packet_t *packet, uint8_t *buffer, int buffer_size) {
    const char cookieXing[] = "Xing";
    const char cookieInfo[] = "Info";

    // ignore mpeg version, and try both 17 and 32 byte offsets
    buffer += 17;
    buffer_size -= 17;
    if (buffer_size < 4) {
        return -1;
    }

    int havecookie = !memcmp (cookieXing, buffer, 4) || !memcmp (cookieInfo, buffer, 4);

    if (!havecookie) {
        buffer += 15;
        buffer_size -= 15;
        if (buffer_size < 4) {
            return -1;
        }

        havecookie = !memcmp (cookieXing, buffer, 4) || !memcmp (cookieInfo, buffer, 4);;
    }

    if (!havecookie) {
        return -1;
    }

    buffer += 4;
    buffer_size -= 4;

    info->packet_offs = packet->offs+packet->packetlength;

    // read flags
    uint32_t flags = READ_UINT32();

    if (flags & FRAMES_FLAG) {
        if (buffer_size < 4) {
            return -1;
        }
        // read number of frames
        uint32_t nframes = READ_UINT32();
        info->totalsamples = nframes * packet->samples_per_frame;
        info->have_duration = 1;
        info->have_xing_nframes = 1;
    }
    if (flags & BYTES_FLAG) {
        READ_UINT32();
    }
    if (flags & TOC_FLAG) {
        if (buffer_size < 100) {
            return -1;
        }
        buffer += 100;
        buffer_size -= 100;
    }
    if (flags & VBR_SCALE_FLAG) {
        READ_UINT32();
    }
    if (buffer_size < 4) {
        return -1;
    }
    // lame header
    int lame_header = 0;
    if (!memcmp (buffer, "LAME", 4)) {
        lame_header = 1;
    }
    buffer += 4;
    buffer_size -= 4;

    if (buffer_size < 5) {
        return -1;
    }

    buffer += 5; // what is this?
    buffer_size -= 5;

    if (buffer_size < 1) {
        return -1;
    }
    uint8_t rev = READ_UINT8();

    switch (rev & 0x0f) {
        case XING_CBR ... XING_ABR2:
            info->vbr_type = rev & 0x0f;
            break;
    }
    if (lame_header) {
        if (buffer_size < 22) {
            return -1;
        }
        buffer += 1; //uint8_t lpf = *data++;
        buffer_size -= 1;

        //3 floats: replay gain

        buffer += 4; // float rg_peaksignalamp = extract_f32 (buf);
        buffer_size -= 4;

        buffer += 2; // uint16_t rg_radio = extract_i16 (buf);
        buffer_size -= 2;

        buffer += 2; // uint16_t rg_audiophile = extract_i16 (buf);
        buffer_size -= 2;

        // skip
        buffer += 2;
        buffer_size -= 2;

        info->delay = (((uint32_t)buffer[0]) << 4) | ((((uint32_t)buffer[1]) & 0xf0)>>4);
        info->padding = ((((uint32_t)buffer[1])&0x0f)<<8) | ((uint32_t)buffer[2]);
        buffer += 3;
        buffer_size -= 3;

        // skip
        buffer++;
        buffer_size -= 1;

        buffer++; // uint8_t mp3gain = *data++;
        buffer_size -= 1;

        // lame preset nr
        info->lamepreset = buffer[1] | (buffer[0]<<8);
        buffer += 2;
        buffer_size -= 2;

        // musiclen
        info->lame_musiclength = READ_UINT32();
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

    info->avg_packetlength += packet->packetlength;
    info->avg_samples_per_frame += packet->samples_per_frame;
    info->avg_bitrate += packet->bitrate;

    if (seek_to_sample == 0) {
        int ch = info->ref_packet.nchannels;
        memcpy (&info->ref_packet, packet, sizeof (mp3packet_t));
        if (info->ref_packet.nchannels < ch) {
            info->ref_packet.nchannels = ch;
        }
        return 1;
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

static int
_packet_same_fmt (mp3packet_t *ref_packet, mp3packet_t *packet) {
    return packet->layer == ref_packet->layer
        && packet->nchannels == ref_packet->nchannels
        && packet->samplerate == ref_packet->samplerate
        && packet->ver == ref_packet->ver;
}

int
mp3_parse_file (mp3info_t *info, uint32_t flags, DB_FILE *fp, int64_t fsize, int startoffs, int endoffs, int64_t seek_to_sample) {
#if PERFORMANCE_STATS
    struct timeval start_tv;
    struct timeval end_tv;
    gettimeofday (&start_tv, NULL);
#endif

    memset (info, 0, sizeof (mp3info_t));
    info->fsize = fsize;
    info->datasize = fsize-startoffs-endoffs;

    if (seek_to_sample > 0) {
        // add 9 extra packets to fill bit-reservoir
        seek_to_sample -= MAX_PACKET_SAMPLES*10;
        if (seek_to_sample < 0) {
            seek_to_sample = 0;
        }
    }

    int err = -1;

    deadbeef->fseek (fp, startoffs, SEEK_SET);
    info->num_seeks++;

    int64_t datasize = fsize;
    if (datasize > 0) {
        datasize -= startoffs+endoffs;
    }

    if (fsize < 0) {
        info->checked_xing_header = 1; // ignore Info tag in streams
    }

    info->is_streaming = fp->vfs->is_streaming ();

    mp3packet_t packet;

    int64_t offs = startoffs;
    int64_t fileoffs = startoffs;
    int64_t lastpacketoffs = -1;

    int prev_br = -1;
    int prev_length = -1;
    int variable_packets = 0;
    int freeformat_packets = 0;

    while (fsize > 0 || fsize < 0) {
        int64_t readsize = 4; // fe ff + frame header
        if (fsize > 0 && offs + readsize >= fsize - endoffs) {
            readsize = fsize - endoffs - offs;
        }

        if (readsize <= 0) {
            break;
        }

        uint8_t fhdr[4];

        if (fileoffs != offs) {
            deadbeef->fseek (fp, offs, SEEK_SET);
            info->num_seeks++;
            fileoffs = offs;
        }
        if (deadbeef->fread (fhdr, 1, readsize, fp) != readsize) {
            goto error;
        }
        info->num_reads++;
        info->bytes_read += readsize;
        fileoffs += readsize;

        int res = _parse_packet (&packet, fhdr);
        if (res < 0 || (info->npackets && !_packet_same_fmt (&info->ref_packet, &packet))) {
            if (res == -2 && info->valid_packets == 0) {
                freeformat_packets++;
            }

            if (freeformat_packets > MAX_FREEFORMAT_PACKETS) {
                goto error; // ignore freeformat streams
            }

            // bail if a valid packet could not be found at the start of stream
            if (!info->valid_packets && offs - startoffs > MAX_INVALID_BYTES) {
                goto error;
            }

            if (info->is_streaming && offs - startoffs > MAX_INVALID_BYTES_STREAM) {
                goto error;
            }

            // if there are valid packets, but there's a large fill of invalid data -- assume EOF,
            // otherwise this may hang for a very long time.
            // Example scenario: 150MB mp3 file filled with zeroes in 2nd half.
            if (lastpacketoffs != -1 && offs - lastpacketoffs > MAX_INVALID_BYTES) {
                goto end;
            }

            // prevent misdetected garbage packets to be used as ref_packet
            if (info->npackets == 1) {
                info->npackets = 0;
            }

            // bad packet in the stream, try to resync
            memset (&info->prev_packet, 0, sizeof (mp3packet_t));
            info->packet_offs = -1;
            info->lastpacket_valid = 0;
            if (info->pcmsample == 0 && seek_to_sample != -1) {
                info->valid_packets = 0;
            }

            offs++;
            continue;
        }
        else {
            // EOF
            if (fsize >= 0 && offs + res > fsize - endoffs) {
                if (info->valid_packets) {
                    goto end;
                }
                else {
                    goto error;
                }
            }

            packet.offs = offs;
            lastpacketoffs = offs;

            if (!info->packet_offs || seek_to_sample >= 0) {
                info->packet_offs = offs;
            }

            // try to read xing/info tag (only on initial scans)
            int got_xing = 0;
            if (!info->have_xing_header && !info->checked_xing_header)
            {
                // need whole packet for checking xing!

                info->checked_xing_header = 1;
                uint8_t xinghdr[packet.packetlength];
                if (deadbeef->fread (xinghdr, 1, sizeof (xinghdr), fp) != sizeof (xinghdr)) {
                    return -1;
                }
                info->num_reads++;
                info->bytes_read += sizeof (xinghdr);

                fileoffs += sizeof (xinghdr);
                int xingres = _check_xing_header (info, &packet, xinghdr, (int)sizeof (xinghdr));
                if (!xingres) {
                    got_xing = 1;
                    info->have_xing_header = 1;
                    // trust the xing header -- even if requested to scan for precise duration
                    // parameters have been discovered from xing header, no need to continue
                    memcpy (&info->ref_packet, &packet, sizeof (mp3packet_t));

                    if (seek_to_sample > 0) {
                        offs += res;
                        continue;
                    }
                    else if (flags & MP3_PARSE_FULLSCAN) {
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
                memcpy (&info->prev_packet, &packet, sizeof (packet));
            }

            if (!variable_packets && (prev_br != -1 || prev_length != -1)) {
                if (prev_br != packet.bitrate || prev_length != packet.packetlength) {
                    variable_packets = 1;
                }
            }
            prev_br = packet.bitrate;
            prev_length = packet.packetlength;

            // Even if we already got everything we need from lame header,
            // we still need to fetch a few packets to get averages right.
            // 200 packets give a pretty accurate value, and correspond
            // to less than 40KB or data
            if (info->have_xing_nframes && seek_to_sample < 0 && !(flags & MP3_PARSE_FULLSCAN) && info->npackets >= 200) {
                goto end;
            }
            // Calculate CBR duration from file size
            else if (!variable_packets && !info->have_xing_header && seek_to_sample < 0 && !(flags & MP3_PARSE_FULLSCAN) && info->npackets >= 200) {
                // calculate total number of packets from file size
                // 8876 - 9485
                int64_t npackets = ceil(datasize/(float)info->ref_packet.packetlength);
                info->totalsamples = npackets * info->ref_packet.samples_per_frame;
                info->have_duration = 1;
                goto end;
            }


            // Handle infinite streams
            if (fsize < 0 && info->npackets >= 20) {
                goto end;
            }

            offs += res;

            // for streaming tracks -- approximate duration
            if (seek_to_sample == -1
                && fsize > 0
                && offs > (startoffs+50000)
                && (info->is_streaming || (flags&MP3_PARSE_ESTIMATE_DURATION))) {
                if (!info->valid_packets) {
                    goto error;
                }

                info->avg_packetlength = floor (info->avg_packetlength/info->valid_packets);
                info->avg_samples_per_frame /= info->valid_packets;
                info->npackets = datasize / info->avg_packetlength;
                info->avg_bitrate /= info->valid_packets;
                if (info->have_xing_header) {
                    info->npackets--;
                }
                else {
                    info->totalsamples = info->npackets * info->avg_samples_per_frame;
                }
                info->have_duration = 1;

                goto end_noaverages;
            }
        }
    }

end:

    if (info->npackets > 0) {
        info->avg_samples_per_frame /= info->npackets;
        info->avg_packetlength /= info->npackets;
        info->avg_bitrate /= info->npackets;
        if (offs < datasize && info->avg_packetlength != 0) {
            info->npackets = datasize / info->avg_packetlength;
            if (info->have_xing_header) {
                info->npackets--;
            }
        }
    }

end_noaverages:

    if (fsize >= 0) {
        info->delay += 529;
        if (info->padding > 529) {
            info->padding -= 529;
        }

        if (seek_to_sample == -1) {
            info->have_duration = 1;
        }
    }
    else {
        info->totalsamples = -1;
    }
    err = 0;
error:
#if PERFORMANCE_STATS
    gettimeofday (&end_tv, NULL);
    float elapsed = (end_tv.tv_sec-start_tv.tv_sec) + (end_tv.tv_usec - start_tv.tv_usec) / 1000000.f;
    printf ("mp3 stats:\nSeeks: %llu\nReads: %llu\nBytes: %llu\nTime: %f sec\n", info->num_seeks, info->num_reads, info->bytes_read, elapsed);
#endif
    return err;
}
