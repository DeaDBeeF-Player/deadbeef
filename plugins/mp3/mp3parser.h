#ifndef mp3parser_h
#define mp3parser_h

#include <stdint.h>
#include "../../deadbeef.h"

typedef struct {
    int ver;
    int samplerate;
    int bitrate;
    int nchannels;
    int samples_per_frame;
    int layer;
    int packetlength;
} mp3packet_t;

typedef struct {
    int npackets;
    int64_t seek_sample;
    int lastpacket_valid;
    int valid_packets;

    int64_t valid_packet_pos; // position of the first known valid frame after successful scan

    mp3packet_t prev_packet;
    mp3packet_t ref_packet;

    int64_t lookback_packet_offs;
    int64_t lookback_packet_idx;
    int64_t lookback_packet_positions[10];

    int have_xing;
    int vbr_type;
} mp3info_t;

// returns:
// positive packet size when the packet is valid
// -1: error
int
mp3_parse_packet (mp3packet_t * restrict packet, uint8_t * restrict hdr);

// returns:
// positive file offset of the first valid mpeg frame
// -1: error
int
mp3_parse_file (mp3info_t *info, DB_FILE *fp, int64_t fsize, int startoffs, int endoffs, int64_t seek_to_sample);

#endif /* mp3parser_h */
