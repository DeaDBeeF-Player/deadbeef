#ifndef mp3parser_h
#define mp3parser_h

#include <stdint.h>
#include "../../deadbeef.h"

enum {
    MP3_PARSE_FULLSCAN = 1,
};

typedef struct {
    uint64_t offs;
    int ver;
    int samplerate;
    int bitrate;
    int nchannels;
    int samples_per_frame;
    int layer;
    int packetlength;
} mp3packet_t;

typedef struct {
    // inputs
    int64_t seek_sample;

    // outputs
    int64_t pcmsample; // sample position corresponding to packet_offs
    int64_t npackets;
    int64_t packet_offs; // stream position of the first known valid frame after successful scan

    int lastpacket_valid;
    int64_t valid_packets;

    mp3packet_t ref_packet; // packet representing the stream format

    int have_xing_header;
    int vbr_type;

    float avg_packetlength;
    int avg_samplerate;
    int avg_samples_per_frame;

    int64_t skipsamples; // how many samples to skip after seek

    int delay;
    int padding;

    uint16_t lamepreset;
    uint32_t lame_musiclength; // file size from beginning of LAME info packet until the last byte of packet with audio, as encoded by Lame

    // intermediates
    mp3packet_t prev_packet;

    int checked_xing_header;
} mp3info_t;

// Params:
// seek_to_sample: -1 means to the end (scan whole file), otherwise a sample to seek to
// When seeking, the packet offset returned will be the one containing seek_to_sample, not accounting for delay.
// `totalsamples` will be set to the sample count at the packet start.
// So the caller is responsible for adding delay to seek_to_sample, and then adjusting skipsamples based on returned "totalsamples".
//
//
// returns:
// 0: success
// -1: error
//
// the caller is supposed to start decoding from info->packet_offs, and skip info->skipsamples samples
int
mp3_parse_file (mp3info_t *info, uint32_t flags, DB_FILE *fp, int64_t fsize, int startoffs, int endoffs, int64_t seek_to_sample);

#endif /* mp3parser_h */
