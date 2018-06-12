#include "mp3parser.h"
#include <string.h>

extern DB_functions_t *deadbeef;

#define MIN_PACKET_SAMPLES 384
#define MAX_PACKET_SAMPLES 1152
#define MAX_BITRATE 320
#define MIN_BITRATE 8
#define MIN_SAMPLERATE 8000
#define MAX_SAMPLERATE 48000
#define MIN_PACKET_LENGTH (MIN_PACKET_SAMPLES / 8 * MIN_BITRATE / MIN_SAMPLERATE)
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

// vbrmethod constants
#define XING_CBR  1
#define XING_ABR  2
#define XING_VBR1 3
#define XING_VBR2 4
#define XING_VBR3 5
#define XING_VBR4 6
#define XING_CBR2 8
#define XING_ABR2 9
#define DETECTED_VBR 100

// xing header flags
#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

int
mp3_parse_frame (mp3packet_t * restrict packet, uint8_t * restrict fb) {
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
    // }}}

    // {{{ get side info ptr
    //        uint8_t *si = fb + 4;
    //        if (prot) {
    //            si += 2;
    //        }
    //        int data_ptr = ((uint16_t)si[1]) | (((uint16_t)si[1]&0)<<8);
    // }}}

    // {{{ calc packet length, number of samples in a frame
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
        //            if (sample > 0) {
        //                printf ("frame: %d, crc: %d, layer: %d, bitrate: %d, samplerate: %d, filepos: 0x%llX, dataoffs: 0x%X, size: 0x%X\n", nframe, prot, layer, bitrate, samplerate, deadbeef->ftell (buffer->file)-8, data_ptr, packetlength);
        //            }
    }
    else {
        ///        trace ("frame %d samplerate or bitrate is invalid\n", nframe);
        return -1;
    }
    return packet->packetlength;
}

int
mp3_process_packet (mp3info_t *info, mp3packet_t *packet) {
    if (info->vbr_type != DETECTED_VBR
        && (!info->have_xing || !info->vbr_type)
        && info->prev_packet.bitrate
        && info->prev_packet.bitrate != packet->bitrate) {
        info->vbr_type = DETECTED_VBR;
    }

    info->valid_packets++;

    // {{{ update stream parameters, only when sample!=0 or 1st frame
    if (info->seek_sample != 0 || info->npackets == 0)
    {
        if (info->seek_sample == 0 && info->lastpacket_valid) {
            return 0;
        }
        // don't get parameters from frames coming after any bad frame
        int ch = info->ref_packet.nchannels;
        memcpy (&info->ref_packet, packet, sizeof (mp3packet_t));
        if (info->ref_packet.nchannels < ch) {
            info->ref_packet.nchannels = ch;
        }
    }
    // }}}

    info->lastpacket_valid = 1;

    // allow at least 10 lead-in frames, to fill bit-reservoir
    if (info->npackets - info->lookback_packet_idx > 10) {
        info->lookback_packet_offs = info->lookback_packet_positions[0];
        info->lookback_packet_idx++;
    }
    memmove (info->lookback_packet_positions, &info->lookback_packet_positions[1], sizeof (int64_t) * 9);
    info->lookback_packet_positions[9] = framepos;

    // {{{ detect/load xing frame, only on 1st pass
    // try to read xing/info tag (only on initial scans)
    if (sample <= 0 && !buffer->have_xing_header && !checked_xing_header)
    {
        checked_xing_header = 1;
        mp3_check_xing_header (buffer, frame.packetlength, sample, frame.samples_per_frame, frame.samplerate, framepos, fsize);

        if (buffer->have_xing_header) {
            // trust the xing header -- even if requested to scan for precise duration
            buffer->startoffset = offs;
            if (sample <= 0) {
                // parameters have been discovered from xing header, no need to continue
                deadbeef->fseek (buffer->file, buffer->startoffset, SEEK_SET);
                return 0;
            }
            else {
                // skip to the next frame
                continue;
            }
        }
    }
    // }}}

    if (sample == 0) {
        // {{{ update averages, interrupt scan on frame #100
        // calculating apx duration based on 1st 100 frames
        buffer->avg_packetlength += frame.packetlength;
        buffer->avg_samplerate += frame.samplerate;
        buffer->avg_samples_per_frame += frame.samples_per_frame;

        buffer->version = frame.ver;
        buffer->layer = frame.layer;
        buffer->bitrate = frame.bitrate;
        buffer->samplerate = frame.samplerate;
        buffer->packetlength = frame.packetlength;
        if (frame.nchannels > buffer->channels) {
            buffer->channels = frame.nchannels;
        }

        if (buffer->file->vfs->is_streaming ()) {
            if (valid_frames >= 20) {
                deadbeef->fseek (buffer->file, valid_frame_pos, SEEK_SET);
                buffer->duration = -1;
                buffer->totalsamples = 0;
                return 0;
            }
        }
        else if (valid_frames >= 100) {
            deadbeef->fseek (buffer->file, valid_frame_pos, SEEK_SET);
            goto end_scan;
        }
        // }}}
    }
    else {
        // seeking to particular sample, interrupt if reached
        if (sample > 0 && scansamples + frame.samples_per_frame >= sample) {
            deadbeef->fseek (buffer->file, lead_in_frame_pos, SEEK_SET);
            buffer->lead_in_frames = (int)(nframe-lead_in_frame_no);
            buffer->currentsample = sample;
            buffer->skipsamples = sample - scansamples;
            trace ("scan: cursample=%d, frame: %d, skipsamples: %d, filepos: %llX, lead-in frames: %d\n", buffer->currentsample, nframe, buffer->skipsamples, deadbeef->ftell (buffer->file), buffer->lead_in_frames);
            return 0;
        }
    }
    scansamples += frame.samples_per_frame;
    nframe++;
    return 0;
}

int
mp3_parse_file (mp3info_t *info, DB_FILE *fp, int64_t fsize, int startoffs, int endoffs, int64_t seek_to_sample) {
    deadbeef->fseek (fp, startoffs, SEEK_SET);

    // FIXME: radio
    fsize -= startoffs + endoffs;

    int bufsize = 512*1024;
    if (bufsize > fsize) {
        bufsize = (int)fsize;
    }

    if (bufsize < MIN_PACKET_LENGTH) {
        return -1; // file too small
    }

    uint8_t *buffer = malloc (bufsize);

    mp3packet_t packet;

    int remaining = 0;
    int64_t offs = 0;

    while (fsize > 0) {
        if (deadbeef->fread (buffer+remaining, 1, bufsize-remaining, fp) != bufsize-remaining) {
            return -1; // read error
        }

        remaining += bufsize;

        uint8_t *bufptr = buffer + bufsize - remaining;

        while (remaining > 4) {
            memcpy (bufptr, bufptr, 4);
            int res = mp3_parse_frame (&packet, bufptr);
            if (res < 0) {
                // invalid frame
                memset (&info->prev_packet, 0, sizeof (mp3packet_t));
                info->valid_packet_pos = -1;
                info->lastpacket_valid = 0;
                if (info->nsamples == 0) {
                    info->valid_packets = 0;
                }

                offs++;

                if (offs - startoffs > MAX_INVALID_BYTES) {
                    return -1;
                }

                remaining--;
                bufptr++;
                continue;
            }
            else {
                // EOF?
                if (fsize >= 0 && offs + res > fsize) {
                    return info->valid_packets ? 0 : -1; // EOF
                }

                if (info->valid_packet_pos == -1) {
                    info->valid_packet_pos = offs;
                }

                mp3_process_packet (info, &packet);
                memcpy (&info->prev_packet, &packet, sizeof (packet));

                remaining -= res;
                if (remaining <= 0) {
                    break;
                }
                bufptr += res;
                offs += res;
            }
        }
        if (remaining > 0) {
            memmove (buffer, bufptr, remaining);
        }
    }

    return 0;
}
